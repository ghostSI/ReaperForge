//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/module_win32.cpp
// Created by  : Steinberg, 08/2016
// Description : hosting module classes (win32 implementation)
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2022, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "public.sdk/source/vst/utility/optional.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "base/source/fbuffer.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <iostream>

#if SMTG_CPP17
#if __has_include(<filesystem>)
#define USE_FILESYSTEM 1
#elif __has_include(<experimental/filesystem>)
#define USE_FILESYSTEM 0
#endif
#else
#define USE_FILESYSTEM 0
#endif

#if USE_FILESYSTEM == 1
#include <filesystem>
namespace filesystem = std::filesystem;
#else
// The <experimental/filesystem> header is deprecated. It is superseded by the C++17 <filesystem>
// header. You can define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING to silence the
// warning, otherwise the build will fail in VS2020 16.3.0
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#endif

#pragma comment(lib, "Shell32")

//------------------------------------------------------------------------
extern "C" {
  using InitModuleFunc = bool (PLUGIN_API*) ();
  using ExitModuleFunc = bool (PLUGIN_API*) ();
}

//------------------------------------------------------------------------
namespace VST3 {
  namespace Hosting {

    //------------------------------------------------------------------------
    namespace {

#if SMTG_OS_WINDOWS_ARM
#define USE_OLE 0
#if SMTG_PLATFORM_64
      constexpr auto architectureString = "arm_64-win";
#else
      constexpr auto architectureString = "arm-win";
#endif
#else
#define USE_OLE !USE_FILESYSTEM
#if SMTG_PLATFORM_64
      constexpr auto architectureString = "x86_64-win";
#else
      constexpr auto architectureString = "x86-win";
#endif
#endif

#if USE_OLE
      //------------------------------------------------------------------------
      struct Ole
      {
        static Ole& instance()
        {
          static Ole gInstance;
          return gInstance;
        }

      private:
        Ole() { OleInitialize(nullptr); }
        ~Ole() { OleUninitialize(); }
      };
#endif // USE_OLE

      //------------------------------------------------------------------------
      class Win32Module : public Module
      {
      public:
        template <typename T>
        T getFunctionPointer(const char* name)
        {
          return reinterpret_cast<T> (GetProcAddress(mModule, name));
        }

        ~Win32Module() override
        {
          factory = PluginFactory(nullptr);

          if (mModule)
          {
            // ExitDll is optional
            if (auto dllExit = getFunctionPointer<ExitModuleFunc>("ExitDll"))
              dllExit();

            FreeLibrary((HMODULE)mModule);
          }
        }

        bool load(const std::string& inPath, std::string& errorDescription) override
        {
          filesystem::path p(inPath);
          auto filename = p.filename();
          p /= "Contents";
          p /= architectureString;
          p /= filename;
          auto wideStr = StringConvert::convert(p.string());
          mModule = LoadLibraryW(reinterpret_cast<LPCWSTR> (wideStr.data()));
          if (!mModule)
          {
            wideStr = StringConvert::convert(inPath);
            mModule = LoadLibraryW(reinterpret_cast<LPCWSTR> (wideStr.data()));
            if (!mModule)
            {
              auto lastError = GetLastError();
              LPVOID lpMessageBuffer;
              FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&lpMessageBuffer, 0, nullptr);
              errorDescription = "LoadLibray failed: " + std::string((char*)lpMessageBuffer);
              LocalFree(lpMessageBuffer);

              return false;
            }
          }
          auto factoryProc = getFunctionPointer<GetFactoryProc>("GetPluginFactory");
          if (!factoryProc)
          {
            errorDescription = "The dll does not export the required 'GetPluginFactory' function";
            return false;
          }
          // InitDll is optional
          auto dllEntry = getFunctionPointer<InitModuleFunc>("InitDll");
          if (dllEntry && !dllEntry())
          {
            errorDescription = "Calling 'InitDll' failed";
            return false;
          }
          auto f = Steinberg::FUnknownPtr<Steinberg::IPluginFactory>(owned(factoryProc()));
          if (!f)
          {
            errorDescription = "Calling 'GetPluginFactory' returned nullptr";
            return false;
          }
          factory = PluginFactory(f);
          return true;
        }

        HINSTANCE mModule{ nullptr };
      };

      //------------------------------------------------------------------------
      bool checkVST3Package(const filesystem::path& p, filesystem::path* result = nullptr)
      {
        auto path = p;
        path /= "Contents";
        path /= architectureString;
        path /= p.filename();
        auto hFile = CreateFileW(reinterpret_cast<LPCWSTR> (path.c_str()), GENERIC_READ, FILE_SHARE_READ,
          nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE)
        {
          CloseHandle(hFile);
          if (result)
            *result = path;
          return true;
        }
        return false;
      }

      //------------------------------------------------------------------------
      bool isFolderSymbolicLink(const filesystem::path& p)
      {
#if USE_FILESYSTEM
        if (/*filesystem::exists (p) &&*/ filesystem::is_symlink(p))
          return true;
#else
        std::wstring wString = p.generic_wstring();
        auto attrib = GetFileAttributesW(reinterpret_cast<LPCWSTR> (wString.c_str()));
        if (attrib & FILE_ATTRIBUTE_REPARSE_POINT)
        {
          auto hFile = CreateFileW(reinterpret_cast<LPCWSTR> (wString.c_str()), GENERIC_READ,
            FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
          if (hFile == INVALID_HANDLE_VALUE)
            return true;
          else
            CloseHandle(hFile);
        }
#endif
        return false;
      }

      //------------------------------------------------------------------------
      Optional<std::string> getKnownFolder(REFKNOWNFOLDERID folderID)
      {
        PWSTR wideStr{};
        if (FAILED(SHGetKnownFolderPath(folderID, 0, nullptr, &wideStr)))
          return {};
        return StringConvert::convert(wideStr);
      }

      //------------------------------------------------------------------------
      VST3::Optional<filesystem::path> resolveShellLink(const filesystem::path& p)
      {
#if USE_FILESYSTEM
        return { filesystem::read_symlink(p).lexically_normal() };
#else
#if USE_OLE
        Ole::instance();

        IShellLink* shellLink = nullptr;
        if (!SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
          IID_IShellLink, reinterpret_cast<LPVOID*> (&shellLink))))
          return {};

        IPersistFile* persistFile = nullptr;
        if (!SUCCEEDED(
          shellLink->QueryInterface(IID_IPersistFile, reinterpret_cast<void**> (&persistFile))))
          return {};

        if (!SUCCEEDED(persistFile->Load(p.wstring().data(), STGM_READ)))
          return {};

        if (!SUCCEEDED(shellLink->Resolve(nullptr, MAKELONG(SLR_NO_UI, 500))))
          return {};

        WCHAR resolvedPath[MAX_PATH];
        if (!SUCCEEDED(shellLink->GetPath(resolvedPath, MAX_PATH, nullptr, SLGP_SHORTPATH)))
          return {};

        std::wstring longPath;
        longPath.resize(MAX_PATH);
        auto numChars =
          GetLongPathNameW(resolvedPath, const_cast<wchar_t*> (longPath.data()), MAX_PATH);
        if (!numChars)
          return {};
        longPath.resize(numChars);

        persistFile->Release();
        shellLink->Release();

        return { filesystem::path(longPath) };
#else
        // TODO for ARM
        return {};
#endif
#endif
      }

      //------------------------------------------------------------------------
      void findFilesWithExt(const filesystem::path& path, const std::string& ext,
        Module::PathList& pathList, bool recursive = true)
      {
        for (auto& p : filesystem::directory_iterator(path))
        {
#if USE_FILESYSTEM
          filesystem::path finalPath(p);
          if (isFolderSymbolicLink(p))
          {
            if (auto res = resolveShellLink(p))
            {
              finalPath = *res;
              if (!filesystem::exists(finalPath))
                continue;
            }
            else
              continue;
          }
          const auto& cpExt = finalPath.extension();
          if (cpExt == ext)
          {
            filesystem::path vstPath(finalPath);
            if (checkVST3Package(vstPath))
            {
              pathList.push_back(vstPath.generic_string());
              continue;
            }
          }

          if (filesystem::is_directory(finalPath))
          {
            if (recursive)
              findFilesWithExt(finalPath, ext, pathList, recursive);
          }
          else if (cpExt == ext)
            pathList.push_back(finalPath.generic_string());
#else
          const auto& cp = p.path();
          const auto& cpExt = cp.extension();
          if (cpExt == ext)
          {
            if ((p.status().type() == filesystem::file_type::directory) ||
              isFolderSymbolicLink(p))
            {
              filesystem::path finalPath(p);
              if (checkVST3Package(finalPath))
              {
                pathList.push_back(finalPath.generic_u8string());
                continue;
              }
              findFilesWithExt(cp, ext, pathList, recursive);
            }
            else
              pathList.push_back(cp.generic_u8string());
          }
          else if (recursive)
          {
            if (p.status().type() == filesystem::file_type::directory)
            {
              findFilesWithExt(cp, ext, pathList, recursive);
            }
            else if (cpExt == ".lnk")
            {
              if (auto resolvedLink = resolveShellLink(cp))
              {
                if (resolvedLink->extension() == ext)
                {
                  if (filesystem::is_directory(*resolvedLink) ||
                    isFolderSymbolicLink(*resolvedLink))
                  {
                    filesystem::path finalPath(*resolvedLink);
                    if (checkVST3Package(finalPath))
                    {
                      pathList.push_back(finalPath.generic_u8string());
                      continue;
                    }
                    findFilesWithExt(*resolvedLink, ext, pathList, recursive);
                  }
                  else
                    pathList.push_back(resolvedLink->generic_u8string());
                }
                else if (filesystem::is_directory(*resolvedLink))
                {
                  const auto& str = resolvedLink->generic_u8string();
                  if (cp.generic_u8string().compare(0, str.size(), str.data(),
                    str.size()) != 0)
                    findFilesWithExt(*resolvedLink, ext, pathList, recursive);
                }
              }
            }
          }
#endif
        }
      }

      //------------------------------------------------------------------------
      void findModules(const filesystem::path& path, Module::PathList& pathList)
      {
        if (filesystem::exists(path))
          findFilesWithExt(path, ".vst3", pathList);
      }

      //------------------------------------------------------------------------
      Optional<filesystem::path> getContentsDirectoryFromModuleExecutablePath(
        const std::string& modulePath)
      {
        filesystem::path path(modulePath);

        path = path.parent_path();
        if (path.filename() != architectureString)
          return {};
        path = path.parent_path();
        if (path.filename() != "Contents")
          return {};

        return Optional<filesystem::path> {std::move(path)};
      }

      //------------------------------------------------------------------------
    } // anonymous

    //------------------------------------------------------------------------
    Module::Ptr Module::create(const std::string& path, std::string& errorDescription)
    {
      auto _module = std::make_shared<Win32Module>();
      if (_module->load(path, errorDescription))
      {
        _module->path = path;
        auto it = std::find_if(path.rbegin(), path.rend(),
          [](const std::string::value_type& c) { return c == '/'; });
        if (it != path.rend())
          _module->name = { it.base(), path.end() };
        return _module;
      }
      return nullptr;
    }

    //------------------------------------------------------------------------
    Module::PathList Module::getModulePaths()
    {
      // find plug-ins located in common/VST3
      PathList list;
      if (auto knownFolder = getKnownFolder(FOLDERID_UserProgramFilesCommon))
      {
        filesystem::path p(*knownFolder);
        p.append("VST3");
        findModules(p, list);
      }

      if (auto knownFolder = getKnownFolder(FOLDERID_ProgramFilesCommon))
      {
        filesystem::path p(*knownFolder);
        p.append("VST3");
        findModules(p, list);
      }

      // find plug-ins located in VST3 (application folder)
      WCHAR modulePath[MAX_PATH + 1];
      GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
      auto appPath = StringConvert::convert(modulePath);
      filesystem::path path(appPath);
      path = path.parent_path();
      path = path.append("VST3");
      findModules(path, list);

      return list;
    }

    //------------------------------------------------------------------------
    Optional<std::string> Module::getModuleInfoPath(const std::string& modulePath)
    {
      auto path = getContentsDirectoryFromModuleExecutablePath(modulePath);
      if (!path)
      {
        filesystem::path p;
        if (!checkVST3Package({ modulePath }, &p))
          return {};
        p = p.parent_path();
        p = p.parent_path();
        path = Optional<filesystem::path>{ p };
      }
      *path /= "moduleinfo.json";
      if (filesystem::exists(*path))
      {
        return { path->generic_string() };
      }
      return {};
    }

    //------------------------------------------------------------------------
    Module::SnapshotList Module::getSnapshots(const std::string& modulePath)
    {
      SnapshotList result;
      auto path = getContentsDirectoryFromModuleExecutablePath(modulePath);
      if (!path)
      {
        filesystem::path p;
        if (!checkVST3Package({ modulePath }, &p))
          return result;
        p = p.parent_path();
        p = p.parent_path();
        path = Optional<filesystem::path>(p);
      }

      *path /= "Resources";
      *path /= "Snapshots";

      if (filesystem::exists(*path) == false)
        return result;

      PathList pngList;
      findFilesWithExt(*path, ".png", pngList, false);
      for (auto& png : pngList)
      {
        filesystem::path p(png);
        auto filename = p.filename().generic_string();
        auto uid = Snapshot::decodeUID(filename);
        if (!uid)
          continue;
        auto scaleFactor = 1.;
        if (auto decodedScaleFactor = Snapshot::decodeScaleFactor(filename))
          scaleFactor = *decodedScaleFactor;

        Module::Snapshot::ImageDesc desc;
        desc.scaleFactor = scaleFactor;
        desc.path = std::move(png);
        bool found = false;
        for (auto& entry : result)
        {
          if (entry.uid != *uid)
            continue;
          found = true;
          entry.images.emplace_back(std::move(desc));
          break;
        }
        if (found)
          continue;
        Module::Snapshot snapshot;
        snapshot.uid = *uid;
        snapshot.images.emplace_back(std::move(desc));
        result.emplace_back(std::move(snapshot));
      }
      return result;
    }

    //------------------------------------------------------------------------
  } // Hosting
} // VST3




#include "vst3.h"

#ifdef SUPPORT_VST3

#include "base64.h"
#include "global.h"
#include "version.h"

#include <SDL2/SDL_syswm.h>

#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <public.sdk/source/vst/hosting/hostclasses.h>
#include <public.sdk/source/vst/hosting/module.h>

#include <filesystem>
#include <functional>
#include <thread>

class ParameterValueQueue final : public Steinberg::Vst::IParamValueQueue
{
  struct Point
  {
    Steinberg::int32 sampleOffset;
    Steinberg::Vst::ParamValue paramValue;

    bool operator <(const Point& other) const
    {
      return sampleOffset < other.sampleOffset;
    }
  };

  bool mInitialized{ false };
  Steinberg::Vst::ParamID mId;
  std::vector<Point> mPoints;

public:

  ParameterValueQueue()
  {
    FUNKNOWN_CTOR
  }
  virtual ~ParameterValueQueue()
  {
    assert(!mInitialized);
    FUNKNOWN_DTOR;
  }

  void initialize(Steinberg::Vst::ParamID id)
  {
    assert(!mInitialized);//misuse

    mInitialized = true;
    mId = id;
  }

  bool isInitialized() const noexcept
  {
    return mInitialized;
  }

  void dispose() noexcept
  {
    mInitialized = false;
    mPoints.clear();
  }


  Steinberg::Vst::ParamID PLUGIN_API getParameterId() override
  {
    assert(mInitialized);
    return mId;
  }

  Steinberg::int32 PLUGIN_API getPointCount() override
  {
    assert(mInitialized);
    return static_cast<int>(mPoints.size());
  }

  Steinberg::tresult PLUGIN_API getPoint(Steinberg::int32 index, Steinberg::int32& sampleOffset, Steinberg::Vst::ParamValue& value) override
  {
    assert(mInitialized);
    if (index >= 0 && index < mPoints.size())
    {
      sampleOffset = mPoints[index].sampleOffset;
      value = mPoints[index].paramValue;
      return Steinberg::kResultOk;
    }
    return Steinberg::kInvalidArgument;
  }

  Steinberg::tresult PLUGIN_API addPoint(Steinberg::int32 sampleOffset, Steinberg::Vst::ParamValue value, Steinberg::int32& index) override
  {
    //store points sorted by time value

    assert(mInitialized);

    auto newPoint = Point{ sampleOffset, value };
    const auto it =
      std::lower_bound(mPoints.begin(), mPoints.end(), newPoint);

    if (it == mPoints.end())
    {
      index = mPoints.size();
      mPoints.push_back(std::move(newPoint));
    }
    else
    {
      if (it->sampleOffset == sampleOffset)
        it->paramValue = value;
      else
        mPoints.insert(it, std::move(newPoint));
      index = std::distance(mPoints.begin(), it);
    }

    return Steinberg::kResultOk;
  }

  DECLARE_FUNKNOWN_METHODS
};
IMPLEMENT_FUNKNOWN_METHODS(ParameterValueQueue, Steinberg::Vst::IParamValueQueue, Steinberg::Vst::IParamValueQueue::iid)

//! Helper object. Global instance provides ParameterChanges
//! with ParameterValueQueue objects
class ParameterQueuePool final
{
  std::vector<Steinberg::IPtr<ParameterValueQueue>> mPool;
public:

  class QueueCleanup {
  public:
    void operator()(ParameterValueQueue* queue)
    {
      queue->dispose();
      queue->release();
    }
  };

  using ParameterValueQueuePtr = std::unique_ptr<
    ParameterValueQueue,
    QueueCleanup
  >;

  static ParameterQueuePool& Instance()
  {
    static ParameterQueuePool instance;
    return instance;
  }

  ParameterValueQueuePtr Get(Steinberg::Vst::ParamID id)
  {
    Steinberg::IPtr<ParameterValueQueue> queue;
    for (auto& q : mPool)
    {
      if (!q->isInitialized())
      {
        queue = q;
        break;
      }
    }
    if (queue == nullptr)
    {
      queue = new ParameterValueQueue;
      mPool.push_back(queue);
    }
    queue->initialize(id);
    queue->addRef();
    return { queue.get(), QueueCleanup { } };
  }

  //Cleanup
  void Reset() noexcept
  {
    mPool.clear();
  }

};

class ParameterChanges final : public Steinberg::Vst::IParameterChanges
{
  std::vector<ParameterQueuePool::ParameterValueQueuePtr> mParamQueues;

public:

  ParameterChanges()
  {
    FUNKNOWN_CTOR
  }

  virtual ~ParameterChanges()
  {
    clear();
    FUNKNOWN_DTOR;
  }

  Steinberg::int32 PLUGIN_API getParameterCount() override
  {
    return static_cast<Steinberg::int32>(mParamQueues.size());
  }

  Steinberg::Vst::IParamValueQueue* PLUGIN_API getParameterData(Steinberg::int32 index) override
  {
    if (index >= 0 && index < mParamQueues.size())
      return mParamQueues[index].get();
    return nullptr;
  }

  Steinberg::Vst::IParamValueQueue* PLUGIN_API addParameterData(const Steinberg::Vst::ParamID& id, Steinberg::int32& index) override
  {
    using namespace Steinberg;
    {
      for (int32 i = 0, count = static_cast<int32>(mParamQueues.size()); i < count; ++i)
      {
        auto& queue = mParamQueues[i];
        if (queue->getParameterId() == id)
        {
          index = i;
          return queue.get();
        }
      }
    }
    auto queue = ParameterQueuePool::Instance().Get(id);
    index = static_cast<int32>(mParamQueues.size());
    mParamQueues.push_back(std::move(queue));
    return mParamQueues.back().get();
  }

  void clear()
  {
    mParamQueues.clear();
  }

  DECLARE_FUNKNOWN_METHODS
};
IMPLEMENT_FUNKNOWN_METHODS(ParameterChanges, Steinberg::Vst::IParameterChanges, Steinberg::Vst::IParameterChanges::iid)

class ComponentHandler : public Steinberg::Vst::IComponentHandler
{
  std::atomic<Steinberg::Vst::IParameterChanges*> mFirst{ nullptr };
  std::atomic<Steinberg::Vst::IParameterChanges*> mSecond{ nullptr };
  std::atomic<Steinberg::Vst::IParameterChanges*> mPendingChanges{ nullptr };

public:
  using PendingChangesPtr = std::unique_ptr<
    Steinberg::Vst::IParameterChanges,
    std::function<void(Steinberg::Vst::IParameterChanges*)>>;

  ComponentHandler()
  {
    //We use two instances of ParameterChanges to ensure
    //that read/write is lock and wait free.
    //Once parameter value change is requested, they are
    //written either to the pending outgoing parameters,
    //if they are not yet requested, or to any of those
    //two instances whichever of them is available.
    //When done writing, parameter changes become pending.

    using namespace Steinberg;

    FUNKNOWN_CTOR
      auto first = new ParameterChanges;
    auto second = new ParameterChanges;
    if (first && second)
    {
      mFirst = first;
      mSecond = second;
    }
  }
  virtual ~ComponentHandler()
  {
    Steinberg::Vst::IParameterChanges* ptr{ nullptr };
    if (ptr = mFirst.load())
      ptr->release();
    if (ptr = mSecond.load())
      ptr->release();
    if (ptr = mPendingChanges.load())
      ptr->release();

    FUNKNOWN_DTOR;
  }

  Steinberg::tresult PLUGIN_API beginEdit(Steinberg::Vst::ParamID id) override
  {
    return Steinberg::kResultOk;
  }

  Steinberg::tresult PLUGIN_API performEdit(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized) override
  {
    using namespace Steinberg;

    //Try grab pending changes first (processing thread will not
    //see them until they are applied)
    auto lock = mPendingChanges.exchange(nullptr);
    if (lock == nullptr)
    {
      //There is no pending changes, then try grab
      //any of the containers. At least one of them always should be
      //available, but another one may be grabbed by the processing thread
      lock = mFirst.exchange(nullptr);
      if (lock == nullptr)
        lock = mSecond.exchange(nullptr);

      //Grabbed object may contain some old data, clean it.
      //We surely don't want to delay the processing thread with cleanup
      //routines
      static_cast<ParameterChanges*>(lock)->clear();
    }

    int32 index;
    tresult result = kInternalError;

    if (const auto queue = lock->addParameterData(id, index))
      //Since we don't yet have a support for automation,
      //sampleOffset is always 0
      result = queue->addPoint(0, valueNormalized, index);
    //else
    // for some unknown reason we have failed to
    // find/create appropriate queue for this parameter

    //now processing thread can see changes
    mPendingChanges.exchange(lock);

    return result;
  }

  Steinberg::tresult PLUGIN_API endEdit(Steinberg::Vst::ParamID id) override
  {
    return Steinberg::kResultOk;
  }

  Steinberg::tresult PLUGIN_API restartComponent(Steinberg::int32 flags) override
  {
    return Steinberg::kNotImplemented;
  }

  PendingChangesPtr getPendingChanges()
  {
    using namespace Steinberg;

    const auto pendingChanges = mPendingChanges.exchange(nullptr);
    if (pendingChanges != nullptr)
    {
      return PendingChangesPtr{ pendingChanges,[handler = IPtr { this }](Vst::IParameterChanges* ptr)
      {
        //once the processing thread done reading changes
        //pointer is moved back
        ptr = handler->mFirst.exchange(ptr);
        if (ptr != nullptr)
           handler->mSecond.exchange(ptr);
     } };
    }
    return { nullptr };
  }


  DECLARE_FUNKNOWN_METHODS
};

IMPLEMENT_FUNKNOWN_METHODS(ComponentHandler, Steinberg::Vst::IComponentHandler, Steinberg::Vst::IComponentHandler::iid)

class ConnectionProxy final : public Steinberg::Vst::IConnectionPoint
{
  std::thread::id mThreadId;

  Steinberg::Vst::IConnectionPoint* mSource{ nullptr };
  Steinberg::Vst::IConnectionPoint* mTarget{ nullptr };
public:

  DECLARE_FUNKNOWN_METHODS;

  ConnectionProxy(Steinberg::Vst::IConnectionPoint* source)
    : mSource(source)
  {
    mThreadId = std::this_thread::get_id();
    FUNKNOWN_CTOR;
  }
  virtual ~ConnectionProxy()
  {
    FUNKNOWN_DTOR;
  }

  Steinberg::tresult PLUGIN_API connect(IConnectionPoint* other) override
  {
    if (other == nullptr)
      return Steinberg::kInvalidArgument;
    else if (mTarget != nullptr)
      return Steinberg::kResultFalse;

    //Looks a bit awkward, but the source can send messages to
    //the target during connection
    mTarget = other;
    auto result = mSource->connect(this);
    if (result != Steinberg::kResultOk)
      mTarget = nullptr;
    return result;
  }


  Steinberg::tresult PLUGIN_API disconnect(IConnectionPoint* other) override
  {
    if (other == nullptr)
      return Steinberg::kInvalidArgument;
    else if (other != mTarget)
      return Steinberg::kResultFalse;

    auto result = mSource->disconnect(this);
    if (result == Steinberg::kResultOk)
      mTarget = nullptr;
    return result;
  }

  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override
  {
    if (mTarget == nullptr ||
      std::this_thread::get_id() != mThreadId)
      return Steinberg::kResultFalse;

    return mTarget->notify(message);
  }
};

IMPLEMENT_FUNKNOWN_METHODS(ConnectionProxy, Steinberg::Vst::IConnectionPoint, Steinberg::Vst::IConnectionPoint::iid);


struct Vst3Plugin
{
  std::shared_ptr<VST3::Hosting::Module> module;
  std::vector<VST3::Hosting::ClassInfo> classInfos;

  Steinberg::Vst::ProcessSetup processSetup;

  Steinberg::IPtr<Steinberg::Vst::IComponent> effectComponent;
  Steinberg::IPtr<Steinberg::Vst::IEditController> editController;
  Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioProcessor;
  Steinberg::IPtr<ComponentHandler> componentHandler;
  Steinberg::IPtr<Steinberg::Vst::IConnectionPoint> componentConnectionProxy;
  Steinberg::IPtr<Steinberg::Vst::IConnectionPoint> controllerConnectionProxy;
  Steinberg::IPlugView* plugView{};

  HWND hwnd{};
};

static std::vector<Vst3Plugin> vst3Plugins;

static void loadPluginInstance(Vst3Plugin& vst3Plugin)
{
  assert(vst3Plugin.editController);

  const i32 parameterCount = vst3Plugin.editController->getParameterCount();
  for (i32 i = 0; i < parameterCount; ++i)
  {
    Steinberg::Vst::ParameterInfo parameterInfo{};
    const Steinberg::tresult result = vst3Plugin.editController->getParameterInfo(i, parameterInfo);
    assert(result == Steinberg::kResultOk);

    assert(!(parameterInfo.flags & Steinberg::Vst::ParameterInfo::kIsReadOnly));

    const double value = vst3Plugin.editController->getParamNormalized(parameterInfo.id);
  }
}

#define INT32_SWAP(val) \
   ((i32) ( \
    (((u32) (val) & (u32) 0x000000ffU) << 24) | \
    (((u32) (val) & (u32) 0x0000ff00U) <<  8) | \
    (((u32) (val) & (u32) 0x00ff0000U) >>  8) | \
    (((u32) (val) & (u32) 0xff000000U) >> 24)))

static Steinberg::Vst::HostApplication context;

static void activateBuses(Steinberg::Vst::IComponent* effectComponent)
{
  {
    const i32 inputCount = effectComponent->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kInput);
    for (i32 i = 0; i < inputCount; ++i)
    {
      Steinberg::Vst::BusInfo busInfo;
      const Steinberg::tresult result = effectComponent->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, busInfo);
      assert(result == Steinberg::kResultOk);
      if (busInfo.flags & Steinberg::Vst::BusInfo::kDefaultActive)
        effectComponent->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kInput, i, true);
    }
  }

  {
    const i32 outputCount = effectComponent->getBusCount(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput);
    for (i32 i = 0; i < outputCount; ++i)
    {
      Steinberg::Vst::BusInfo busInfo;
      const Steinberg::tresult result = effectComponent->getBusInfo(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, busInfo);
      assert(result == Steinberg::kResultOk);
      if (busInfo.flags & Steinberg::Vst::BusInfo::kDefaultActive)
        effectComponent->activateBus(Steinberg::Vst::kAudio, Steinberg::Vst::kOutput, i, true);
    }
  }
}

void Vst3::init()
{
  const std::vector<std::string> paths = string::split(Global::settings.vst3Path, ';');
  for (const std::string& path : paths)
  {
    for (const auto& file : std::filesystem::directory_iterator(std::filesystem::path(Global::settings.vst3Path)))
    {
      if (file.path().extension() != std::filesystem::path(".vst3"))
        continue;

      vst3Plugins.push_back(Vst3Plugin());
      Vst3Plugin& vst3Plugin = vst3Plugins[vst3Plugins.size() - 1];
      vst3Plugin.processSetup.processMode = Steinberg::Vst::kOffline;
      vst3Plugin.processSetup.symbolicSampleSize = Steinberg::Vst::kSample32;
      vst3Plugin.processSetup.maxSamplesPerBlock = 8192;
      vst3Plugin.processSetup.sampleRate = f64(Global::settings.audioSampleRate);

      {
        std::string error;
        vst3Plugin.module = VST3::Hosting::Module::create(file.path().string(), error);
        assert(error.size() == 0);
      }

      const VST3::Hosting::PluginFactory& pluginFactory = vst3Plugin.module->getFactory();
      vst3Plugin.classInfos = pluginFactory.classInfos();
      for (u64 i = 0; i < vst3Plugin.classInfos.size(); ++i)
      {
        const VST3::Hosting::ClassInfo& classInfo = vst3Plugin.classInfos[i];
        auto a = classInfo.ID().toString();
        if (i != 0)
          continue;

        Global::pluginNames.push_back("VST3 " + classInfo.name());

        vst3Plugin.effectComponent = pluginFactory.createInstance<Steinberg::Vst::IComponent>(classInfo.ID());
        assert(vst3Plugin.effectComponent != nullptr);

        const Steinberg::tresult result = vst3Plugin.effectComponent->initialize(&context);
        assert(result == Steinberg::kResultOk);

        vst3Plugin.audioProcessor = Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor>(vst3Plugin.effectComponent);
        assert(vst3Plugin.audioProcessor != nullptr);

        activateBuses(vst3Plugin.effectComponent);

        vst3Plugin.editController = Steinberg::FUnknownPtr<Steinberg::Vst::IEditController>(vst3Plugin.effectComponent);

        if (!vst3Plugin.editController)
        {
          Steinberg::TUID controllerCID;
          {
            const Steinberg::tresult result = vst3Plugin.effectComponent->getControllerClassId(controllerCID);
            assert(result == Steinberg::kResultOk);
          }
          {
            vst3Plugin.editController = pluginFactory.createInstance<Steinberg::Vst::IEditController>(VST3::UID(controllerCID));
            assert(vst3Plugin.editController);
          }
          {
            const Steinberg::tresult result = vst3Plugin.editController->initialize(&context);
            assert(result == Steinberg::kResultOk);
          }
        }

        vst3Plugin.componentHandler = new ComponentHandler;
        vst3Plugin.editController->setComponentHandler(vst3Plugin.componentHandler);

        auto componentConnectionPoint = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint>{ vst3Plugin.effectComponent };
        auto controllerConnectionPoint = Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint>{ vst3Plugin.editController };

        if (componentConnectionPoint && controllerConnectionPoint)
        {
          vst3Plugin.componentConnectionProxy = new ConnectionProxy(componentConnectionPoint);
          vst3Plugin.controllerConnectionProxy = new ConnectionProxy(controllerConnectionPoint);

          vst3Plugin.componentConnectionProxy->connect(controllerConnectionPoint);
          vst3Plugin.controllerConnectionProxy->connect(componentConnectionPoint);
        }



        assert(vst3Plugin.audioProcessor->canProcessSampleSize(vst3Plugin.processSetup.symbolicSampleSize) == Steinberg::kResultOk);
        {
          const Steinberg::tresult result = vst3Plugin.audioProcessor->setupProcessing(vst3Plugin.processSetup);
          assert(result == Steinberg::kResultOk);
        }
        {
          const Steinberg::tresult result = vst3Plugin.effectComponent->setActive(true);
          assert(result == Steinberg::kResultOk);
        }
        {
          /*const Steinberg::tresult result =*/ vst3Plugin.audioProcessor->setProcessing(true);
          //assert(result == Steinberg::kResultOk);
        }
        {
          const u32 latency = vst3Plugin.audioProcessor->getLatencySamples();
          assert(latency < 512);
        }
      }
    }
  }
}

//class PlugFrame final : public Steinberg::IPlugFrame
//{
//  HWND parent;
//  PlugFrame(HWND parent) : parent(parent)
//  {
//    //FUNKNOWN_CTOR
//  }
//
//  Steinberg::tresult PLUGIN_API resizeView(Steinberg::IPlugView* view, Steinberg::ViewRect* newSize) override
//  {
//    return Steinberg::kResultOk;
//  }
//}

void Vst3::openWindow(i32 index, i32 instance)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];
  assert(vst3Plugin.plugView == nullptr);
  assert(vst3Plugin.editController);

  vst3Plugin.plugView = vst3Plugin.editController->createView(Steinberg::Vst::ViewType::kEditor);

  assert(vst3Plugin.plugView != nullptr);

  {
    Steinberg::ViewRect defaultSize;
    const Steinberg::tresult result = vst3Plugin.plugView->getSize(&defaultSize);
    assert(result == Steinberg::kResultOk);
  }

  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(Global::window, &wmInfo);

  //PlugFrame* plugFrame = new PlugFrame(wmInfo.info.win.window);
  //vst3Plugin.plugView->setFrame(plugFrame);

  {
    const Steinberg::tresult result = vst3Plugin.plugView->attached(wmInfo.info.win.window, Steinberg::kPlatformTypeHWND);
    assert(result == Steinberg::kResultOk);
  }

  vst3Plugin.hwnd = GetWindow(wmInfo.info.win.window, GW_CHILD);
}

Rect Vst3::getWindowRect(i32 index)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];
  assert(vst3Plugin.plugView != nullptr);

  Steinberg::ViewRect size;
  const Steinberg::tresult result = vst3Plugin.plugView->getSize(&size);
  assert(result == Steinberg::kResultOk);

  const Rect r = {
    .top = i16(size.top),
    .left = i16(size.left),
    .bottom = i16(size.bottom),
    .right = i16(size.right)
  };

  return r;
}

void Vst3::moveWindow(i32 index, i32 x, i32 y)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];

  Steinberg::ViewRect size;
  const Steinberg::tresult result = vst3Plugin.plugView->getSize(&size);
  assert(result == Steinberg::kResultOk);

  assert(size.left == 0);
  assert(size.top == 0);

  MoveWindow(vst3Plugin.hwnd, x, y, size.right, size.bottom, TRUE);
}

void Vst3::closeWindow(i32 index)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];

  DestroyWindow(vst3Plugin.hwnd);

  vst3Plugin.plugView->removed();
  vst3Plugin.plugView = nullptr;;
}


u64 Vst3::processBlock(i32 index, i32 instance, f32** inBlock, f32** outBlock, i32 blockLen)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];

  using namespace Steinberg;
  if (auto audioProcessor = FUnknownPtr<Vst::IAudioProcessor>(vst3Plugin.effectComponent))
  {
    Vst::ProcessData data;
    data.processMode = vst3Plugin.processSetup.processMode;
    data.symbolicSampleSize = vst3Plugin.processSetup.symbolicSampleSize;
    data.numSamples = blockLen;
    data.numInputs = vst3Plugin.effectComponent->getBusCount(Vst::kAudio, Vst::kInput);
    data.numOutputs = vst3Plugin.effectComponent->getBusCount(Vst::kAudio, Vst::kOutput);

    if (data.numInputs > 0)
    {
      int inputBlocksOffset{ 0 };

      data.inputs = static_cast<Vst::AudioBusBuffers*>(alloca(sizeof(Vst::AudioBusBuffers) * data.numInputs));

      for (int busIndex = 0; busIndex < data.numInputs; ++busIndex)
      {
        Vst::BusInfo busInfo{ };
        if (vst3Plugin.effectComponent->getBusInfo(Vst::kAudio, Vst::kInput, busIndex, busInfo) != kResultOk)
        {
          return 0;
        }
        if (busInfo.busType == Vst::kMain)
        {
          data.inputs[busIndex].numChannels = busInfo.channelCount;
          data.inputs[busIndex].channelBuffers32 = const_cast<f32**>(inBlock + inputBlocksOffset);
          inputBlocksOffset += busInfo.channelCount;
        }
        else
        {
          //aux is not yet supported
          data.inputs[busIndex].numChannels = 0;
          data.inputs[busIndex].channelBuffers32 = nullptr;
        }
        data.inputs[busIndex].silenceFlags = 0UL;
      }
    }
    if (data.numOutputs > 0)
    {
      int outputBlocksOffset{ 0 };

      data.outputs = static_cast<Vst::AudioBusBuffers*>(alloca(sizeof(Vst::AudioBusBuffers) * data.numOutputs));
      for (int busIndex = 0; busIndex < data.numOutputs; ++busIndex)
      {
        Vst::BusInfo busInfo{ };
        if (vst3Plugin.effectComponent->getBusInfo(Vst::kAudio, Vst::kOutput, busIndex, busInfo) != kResultOk)
        {
          return 0;
        }
        if (busInfo.busType == Vst::kMain)
        {
          data.outputs[busIndex].numChannels = busInfo.channelCount;
          data.outputs[busIndex].channelBuffers32 = const_cast<float**>(outBlock + outputBlocksOffset);
          outputBlocksOffset += busInfo.channelCount;
        }
        else
        {
          //aux is not yet supported
          data.outputs[busIndex].numChannels = 0;
          data.outputs[busIndex].channelBuffers32 = nullptr;
        }
        data.outputs[busIndex].silenceFlags = 0UL;
      }
    }

    const auto processResult = audioProcessor->process(data);

    return processResult == kResultOk ?
      data.numSamples : 0;
  }
  return blockLen;
}

class PresetsBufferStream : public Steinberg::Vst::BufferStream
{
public:
  Steinberg::Buffer& get()
  {
    return mBuffer;
  }
};

std::string Vst3::saveParameters(i32 index, i32 instance)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];

  PresetsBufferStream bufferStream;
  const Steinberg::tresult result = vst3Plugin.effectComponent->getState(&bufferStream);
  assert(result == Steinberg::kResultOk);
  assert(bufferStream.get().getFillSize() > 0);

  return Base64::encode(reinterpret_cast<u8*>(bufferStream.get().pass()), bufferStream.get().getFillSize());
}

void Vst3::loadParameter(i32 index, i32 instance, const std::string& base64)
{
  assert(index >= 0);
  assert(index < vst3Plugins.size());

  Vst3Plugin& vst3Plugin = vst3Plugins[index];

  const u64 estimatedSize = (base64.size() / 4 * 3);

  PresetsBufferStream bufferStream;
  bufferStream.get().setSize(estimatedSize);
  const i64 len = Base64::decode(base64, reinterpret_cast<u8*>(bufferStream.get().pass()));

  assert(len <= estimatedSize);
  assert(len > estimatedSize - 3);

  const Steinberg::tresult result = vst3Plugin.effectComponent->setState(&bufferStream);
  assert(result == Steinberg::kResultOk);
}

#endif // SUPPORT_VST