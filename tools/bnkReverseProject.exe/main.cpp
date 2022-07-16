// main.cpp
// Copyright (C) 2010 Audiokinetic Inc
/// \file 
/// Contains the entry point for the application.

/////////////////////////
//  INCLUDES
/////////////////////////


#define DEMO_DEFAULT_POOL_SIZE 4000ULL*1024*1024
#define DEMO_LENGINE_DEFAULT_POOL_SIZE 4000ULL*1024*1024
#define SOUND_BANK_PATH L"../../bnk/"

#include <AK/MusicEngine/Common/AkMusicEngine.h>    // Sound engine
#include <AK/Plugin/AllPluginsFactories.h>
#include <AK/SoundEngine/Common/AkCallback.h>    // Callback
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>		// Memory Manager
#include <AK/SoundEngine/Common/AkModule.h>			// Default memory and stream managers
#include <AK/SoundEngine/Common/AkSoundEngine.h>    // Sound engine
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>	// AkStreamMgrModule
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>		// Streaming Manager
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkFNVHash.h>
#include <AK/Tools/Common/AkListBare.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Win32/AkPlatformFuncs.h>

// Needs to be included after AkSoundEngine.h
#include <AK/Plugin/AllPluginsRegistrationHelpers.h>	// Plug-ins. 

#include <queue>

struct AkAudioFormat;
class AkAudioBuffer;

class SoundInputBase
{
public:

  virtual bool InputOn(unsigned int in_DevNumber = 0) = 0;		// Start input recording
  virtual bool InputOff() = 0;									// Stop input recording

  // For callback purpose.
  virtual void Execute(AkAudioBuffer* io_pBufferOut) = 0;
  virtual void GetFormatCallback(AkAudioFormat& io_AudioFormat) = 0;

  void SetPlayingID(AkPlayingID in_playingID);
  AkPlayingID GetPlayingID();

  SoundInputBase();
  virtual ~SoundInputBase();

private:
  AkPlayingID m_playingID;
};

#define MAX_NUM_SOUNDINPUT		4

#define INVALID_DEVICE_ID		0

typedef AkUInt32 SoundInputDevID;

// Forward definitions.
class SoundInput;
struct AkAudioFormat;
class AkAudioBuffer;


class SoundInputMgrBase
{
public:
  SoundInputMgrBase() { m_MicCount = 0; }

  struct DeviceToInputAssoc
  {
    SoundInputBase* pInput;
    SoundInputDevID idDevice;
  };

  virtual bool Initialize();
  virtual void Term();

  virtual AkUInt32 GetMicCount() { return m_MicCount; }
  virtual SoundInputDevID GetMicDeviceID(AkUInt32 in_index) { return m_DeviceInputAssoc[in_index].idDevice; }

  bool RegisterInput(SoundInputBase* in_pSoundInput, SoundInputDevID in_idDevice);
  void UnregisterInput(SoundInputBase* in_pSoundInput, SoundInputDevID in_idDevice);

protected:
  virtual ~SoundInputMgrBase() {}

  // Register detected devices (via platform API)
  void ClearDevices();
  bool RegisterDevice(SoundInputDevID in_idDevice);
  void UnregisterDevice(SoundInputDevID in_idDevice);

  // Callbacks that will be passed to Wwise.
  static void GetFormatCallback(
    AkPlayingID		/*in_playingID*/,   // Playing ID
    AkAudioFormat&  /*io_AudioFormat*/  // Already filled format, modify it if required.
  );

  static void ExecuteCallback(
    AkPlayingID		in_playingID,  // Playing ID
    AkAudioBuffer* io_pBufferOut  // Buffer to fill
  );

  void Reset();
  SoundInputBase* GetInput(AkPlayingID in_playingID);

  DeviceToInputAssoc m_DeviceInputAssoc[MAX_NUM_SOUNDINPUT];

  AkUInt32 m_MicCount;
};

// SoundInputMgr definition.
class SoundInputMgr : public SoundInputMgrBase
{
public:
  // Get the static instance.
  static SoundInputMgrBase& Instance();

  virtual bool Initialize();
  virtual void Term();

  // Hidden constructor set to prevent multiple instances of SoundInputMgr.
private:
  SoundInputMgr() {}								// Private empty constructor
  virtual ~SoundInputMgr() {}
  SoundInputMgr(const SoundInputMgr&);			// Prevent copy-construction
  SoundInputMgr& operator=(const SoundInputMgr&);	// Prevent assignment
};


// The window size of the sample input

#ifdef AK_LOW_LATENCY
#define AUDIO_INPUT_NUM_BUFFERS 8
#define AUDIO_INPUT_BUFFER_SIZE 256
#define NUM_TARGET_FREE_BUFFERS 5
#else
#define AUDIO_INPUT_NUM_BUFFERS 8
#define AUDIO_INPUT_BUFFER_SIZE 1024
#define NUM_TARGET_FREE_BUFFERS (AUDIO_INPUT_NUM_BUFFERS/2)
#endif

class SoundInput : public SoundInputBase
{
public:
  SoundInput();
  virtual ~SoundInput();

  static SoundInput& Instance()
  {
    return ms_Instance;
  }

  virtual bool InputOn(unsigned int in_DevNumber = 0);		// Start input recording
  virtual bool InputOff();

  virtual void Execute(AkAudioBuffer* io_pBufferOut);
  virtual void GetFormatCallback(AkAudioFormat& io_AudioFormat);

private:

  BOOL SetInputDevice(unsigned int inDev);	// Mic. or Line

  void SendBufferRequest(DWORD in_bufferIndex);
  void ExecuteBufferRequest(DWORD in_bufferIndex);

  // Refil thread
  static AK_DECLARE_THREAD_ROUTINE(RefillThreadFunc);
  AKRESULT StartRefillThread();
  void StopRefillThread();

  static SoundInput ms_Instance;

  AkThread	m_RefillThread;
  AkEvent		m_eventRefill;
  bool		m_bStopRefillThread;
  CAkLock		m_refillLock;

  std::queue<DWORD> m_refillQueue;

  DWORD m_CurrentInputBufferIndex; // Current Transfer buffer index.
  AkUInt16 m_RemainingSize; // Offset where to read in the currently being read index buffer.
  void MoveCurrentIndex();
  AkUInt16 GetFullBufferSize();

  HWAVEIN			m_WaveInHandle;
  WAVEHDR			m_WaveHeader[AUDIO_INPUT_NUM_BUFFERS];
  WAVEFORMATEX	m_WaveFormat;

  // WG-20421
  // SoundInput will sleep in destructor only when successfully initialized.
  // This should be fixed in a better way in future.
  bool			m_bSuccessfullyInitOnceAndMustSleepOnExit;

  AkUInt32		m_idDevice;
};






// AK file packager definitions.
#define AKPK_CURRENT_VERSION		(1)

#define AKPK_HEADER_CHUNK_DEF_SIZE	(8)	// The header chunk definition is 8 bytes wide.

#define AKPK_FILE_FORMAT_TAG	\
		AkmmioFOURCC('A','K','P','K')

//-----------------------------------------------------------------------------
// Name: class CAkFilePackageLUT.
// Desc: Keeps pointers to various parts of the header. Offers look-up services
// for file look-up and soundbank ID retrieval.
//-----------------------------------------------------------------------------
class CAkFilePackageLUT
{
public:

  static const AkUInt16 AK_INVALID_LANGUAGE_ID = 0;

  // Ensure no padding is done because this structure is mapped to file content
#pragma pack(push, 4)
  template <class T_FILEID>
  struct AkFileEntry
  {
    T_FILEID	fileID;		// File identifier. 
    AkUInt32	uBlockSize;	// Size of one block, required alignment (in bytes).
    AkUInt32	uFileSize;  // File size in bytes. 
    AkUInt32	uStartBlock;// Start block, expressed in terms of uBlockSize. 
    AkUInt32	uLanguageID;// Language ID. AK_INVALID_LANGUAGE_ID if not language-specific. 
  };
#pragma pack(pop)

  CAkFilePackageLUT();
  virtual ~CAkFilePackageLUT();

  // Create a new LUT from a packaged file header.
  // The LUT sets pointers to appropriate location inside header data (in_pData).
  AKRESULT Setup(
    AkUInt8* in_pData,			// Header data.
    AkUInt32			in_uHeaderSize		// Size of file package header.
  );

  // Find a file entry by ID.
  const AkFileEntry<AkFileID>* LookupFile(
    AkFileID			in_uID,				// File ID.
    AkFileSystemFlags* in_pFlags			// Special flags. Do not pass NULL.
  );

  // Find a file entry by ID with 64 bit ID.
  const AkFileEntry<AkUInt64>* LookupFile(
    AkUInt64			in_uID,				// File ID.
    AkFileSystemFlags* in_pFlags			// Special flags. Do not pass NULL.
  );

  // Set current language.
  // Returns AK_InvalidLanguage if a package is loaded but the language string cannot be found.
  // Returns AK_Success otherwise.
  AKRESULT SetCurLanguage(
    const AkOSChar* in_pszLanguage		// Language string.
  );

  // Find a soundbank ID by its name (by hashing its name)
  AkFileID GetSoundBankID(
    const AkOSChar* in_pszBankName		// Soundbank name.
  );

  // Return the id of an external file (by hashing its name in 64 bits)
  AkUInt64 GetExternalID(
    const AkOSChar* in_pszExternalName		// External Source name.
  );

protected:
  static void RemoveFileExtension(AkOSChar* in_pstring);
  static void _MakeLower(AkOSChar* in_pString);
  static void _MakeLowerA(char* in_pString, size_t in_strlen);
  static AkUInt64 _MakeLowerAndHash64(char* in_pszString);
  static AkUInt64 GetID64FromString(const char* in_pszString);
  static AkUInt64 GetID64FromString(const wchar_t* in_pszString);

  //
  // File LUTs.
  // 
  template <class T_FILEID>
  class FileLUT
  {
  public:
    const AkFileEntry<T_FILEID>* FileEntries() const { return (AkFileEntry<T_FILEID>*)((AkUInt32*)this + 1); }
    bool HasFiles() const { return (m_uNumFiles > 0); }
    AkUInt32 NumFiles() const { return m_uNumFiles; }
  private:
    FileLUT();	// Do not create this object, just cast raw data.
    AkUInt32		m_uNumFiles;
  };

  // Helper: Find a file entry by ID.
  template <class T_FILEID>
  const AkFileEntry<T_FILEID>* LookupFile(
    T_FILEID					in_uID,					// File ID.
    const FileLUT<T_FILEID>* in_pLut,				// LUT to search.
    bool						in_bIsLanguageSpecific	// True: match language ID.
  );

private:

  AkUInt16			m_curLangID;	// Current language.


  //
  // Maps format.
  //
  class StringMap
  {
  public:
    // Returns AK_INVALID_UNIQUE_ID if ID is not found.
    AkUInt32 GetID(const AkOSChar* in_pszString);
    inline AkUInt32 GetNumStrings() { return m_uNumStrings; }
  private:
    struct StringEntry
    {
      AkUInt32	uOffset;	// Byte offset of the string in the packaged strings section, 
                  // from beginning of the string map.
      AkUInt32	uID;		// ID.
    };
    StringMap();	// Do not create this object, just cast raw data to use GetID().
    AkUInt32	m_uNumStrings;
  };

  // Languages map.
  StringMap* m_pLangMap;

  // SoundBanks LUT.
  FileLUT<AkFileID>* m_pSoundBanks;

  // StreamedFiles LUT.
  FileLUT<AkFileID>* m_pStmFiles;

  // External Sources LUT.
  FileLUT<AkUInt64>* m_pExternals;
};

// Helper: Find a file entry by ID.
template <class T_FILEID>
const CAkFilePackageLUT::AkFileEntry<T_FILEID>* CAkFilePackageLUT::LookupFile(
  T_FILEID					in_uID,					// File ID.
  const FileLUT<T_FILEID>* in_pLut,				// LUT to search.
  bool						in_bIsLanguageSpecific	// True: match language ID.
)
{
  const AkFileEntry<T_FILEID>* pTable = in_pLut->FileEntries();

  AKASSERT(pTable && in_pLut->HasFiles());
  AkUInt16 uLangID = in_bIsLanguageSpecific ? m_curLangID : AK_INVALID_LANGUAGE_ID;

  // Binary search. LUT items should be sorted by fileID, then by language ID.
  AkInt32 uTop = 0, uBottom = in_pLut->NumFiles() - 1;
  do
  {
    AkInt32 uThis = (uBottom - uTop) / 2 + uTop;
    if (pTable[uThis].fileID > in_uID)
      uBottom = uThis - 1;
    else if (pTable[uThis].fileID < in_uID)
      uTop = uThis + 1;
    else
    {
      // Correct ID. Check language.
      if (pTable[uThis].uLanguageID > uLangID)
        uBottom = uThis - 1;
      else if (pTable[uThis].uLanguageID < uLangID)
        uTop = uThis + 1;
      else
        return pTable + uThis;
    }
  } while (uTop <= uBottom);

  return NULL;
}




//-----------------------------------------------------------------------------
// Name: Base class for items that can be chained in AkListBareLight lists.
//-----------------------------------------------------------------------------
template<class T>
class CAkListAware
{
public:
  CAkListAware()
    : pNextItem(NULL) {}

  struct AkListNextItem
  {
    static AkForceInline T*& Get(T* in_pItem)
    {
      return in_pItem->pNextItem;
    }
  };

  T* pNextItem;
};

//-----------------------------------------------------------------------------
// Name: CAkFilePackage 
// Desc: Base class representing a file package (incomplete implementation). 
// It holds a look-up table (CAkFilePackageLUT) and manages memory for the LUT and
// for itself. 
//-----------------------------------------------------------------------------
class CAkFilePackage : public CAkListAware<CAkFilePackage>
{
public:
  // Package factory.
  // Creates a memory pool to contain the header of the file package and this object. 
  // Returns its address.
  template<class T_PACKAGE>
  static T_PACKAGE* Create(
    const AkOSChar* in_pszPackageName,	// Name of the file package (for memory monitoring and ID generation).
    AkMemPoolId			in_memPoolID,		// Memory pool in which the package is created with its lookup table.
    AkUInt32 			in_uHeaderSize,		// File package header size, including the size of the header chunk AKPK_HEADER_CHUNK_DEF_SIZE.
    AkUInt32			in_uBlockAlign,		// Alignment of memory block.
    AkUInt32& out_uReservedHeaderSize, // Size reserved for header, taking mem align into account.
    AkUInt8*& out_pHeaderBuffer	// Returned address of memory for header.
  )
  {
    AKASSERT(in_uHeaderSize > 0);

    out_pHeaderBuffer = NULL;

    // Create memory pool and copy header.
    // The pool must be big enough to hold both the buffer for the LUT's header
    // and a CAkFilePackage object.
    bool bIsInternalPool;
    AkUInt8* pToRelease = NULL;
    out_uReservedHeaderSize = ((in_uHeaderSize + in_uBlockAlign - 1) / in_uBlockAlign) * in_uBlockAlign;
    AkUInt32 uMemSize = out_uReservedHeaderSize + sizeof(T_PACKAGE);
    if (in_memPoolID == AK_DEFAULT_POOL_ID)
    {
      in_memPoolID = AK::MemoryMgr::CreatePool(NULL, uMemSize, uMemSize, AkMalloc | AkFixedSizeBlocksMode, in_uBlockAlign);
      if (in_memPoolID == AK_INVALID_POOL_ID)
        return NULL;
      AK_SETPOOLNAME(in_memPoolID, in_pszPackageName);
      bIsInternalPool = true;
      pToRelease = (AkUInt8*)AK::MemoryMgr::GetBlock(in_memPoolID);
      AKASSERT(pToRelease);
    }
    else
    {
      // Shared pool.
      bIsInternalPool = false;
      AKRESULT eResult = AK::MemoryMgr::CheckPoolId(in_memPoolID);
      if (eResult == AK_Success)
      {
        if (AK::MemoryMgr::GetPoolAttributes(in_memPoolID) & AkBlockMgmtMask)
        {
          if (AK::MemoryMgr::GetBlockSize(in_memPoolID) >= uMemSize)
            pToRelease = (AkUInt8*)AK::MemoryMgr::GetBlock(in_memPoolID);
        }
        else
          pToRelease = (AkUInt8*)AkAlloc(in_memPoolID, uMemSize);
      }
    }

    if (!pToRelease)
      return NULL;

    // Generate an ID.
    AkUInt32 uPackageID = AK::SoundEngine::GetIDFromString(in_pszPackageName);

    // Construct a CAkFilePackage at the end of this memory region.
    T_PACKAGE* pFilePackage = AkPlacementNew(pToRelease + out_uReservedHeaderSize) T_PACKAGE(uPackageID, in_uHeaderSize, in_memPoolID, pToRelease, bIsInternalPool);
    AKASSERT(pFilePackage);	// Must succeed.

    out_pHeaderBuffer = pToRelease;

    return pFilePackage;
  }

  // Destroy file package and free memory / destroy pool.
  virtual void Destroy();

  // Getters.
  inline AkUInt32 ID() { return m_uPackageID; }
  inline AkUInt32 HeaderSize() { return m_uHeaderSize; }
  inline AkUInt32 ExternalPool() { return (!m_bIsInternalPool) ? m_poolID : AK_DEFAULT_POOL_ID; }

  // Members.
  // ------------------------------
  CAkFilePackageLUT	lut;		// Package look-up table.

protected:
  AkUInt32			m_uPackageID;
  AkUInt32			m_uHeaderSize;
  // ------------------------------

protected:
  // Private constructors: users should use Create().
  CAkFilePackage();
  CAkFilePackage(CAkFilePackage&);
  CAkFilePackage(AkUInt32 in_uPackageID, AkUInt32 in_uHeaderSize, AkMemPoolId in_poolID, void* in_pToRelease, bool in_bIsInternalPool)
    : m_uPackageID(in_uPackageID)
    , m_uHeaderSize(in_uHeaderSize)
    , m_poolID(in_poolID)
    , m_pToRelease(in_pToRelease)
    , m_bIsInternalPool(in_bIsInternalPool)
  {
  }
  virtual ~CAkFilePackage() {}

  // Helper.
  static void ClearMemory(
    AkMemPoolId in_poolID,			// Pool to destroy.
    void* in_pMemToRelease,	// Memory block to free before destroying pool.
    bool		in_bIsInternalPool	// Pool was created internally (and needs to be destroyed).
  );

protected:
  // Memory management.
  AkMemPoolId			m_poolID;		// Memory pool for LUT.
  void* m_pToRelease;	// LUT storage (only keep this pointer to release memory).
  bool				m_bIsInternalPool;	// True if pool was created by package, false if shared.
};

//-----------------------------------------------------------------------------
// Name: ListFilePackages
// Desc: AkListBare of CAkFilePackage items.
//-----------------------------------------------------------------------------
typedef AkListBare<CAkFilePackage, CAkListAware<CAkFilePackage>::AkListNextItem, AkCountPolicyWithCount> ListFilePackages;


struct AkFileSystemFlags;

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

class CAkFileLocationBase
{
public:
  CAkFileLocationBase();
  virtual ~CAkFileLocationBase();

  //
  // Global path functions.
  // ------------------------------------------------------

  // Base path is prepended to all file names.
  // Audio source path is appended to base path whenever uCompanyID is AK and uCodecID specifies an audio source.
  // Bank path is appended to base path whenever uCompanyID is AK and uCodecID specifies a sound bank.
  // Language specific dir name is appended to path whenever "bIsLanguageSpecific" is true.
  AKRESULT SetBasePath(
    const AkOSChar* in_pszBasePath
  );
  AKRESULT SetBankPath(
    const AkOSChar* in_pszBankPath
  );
  AKRESULT SetAudioSrcPath(
    const AkOSChar* in_pszAudioSrcPath
  );
  // Note: SetLangSpecificDirName() does not exist anymore. See release note WG-19397 (Wwise 2011.2).

  //
  // Path resolving services.
  // ------------------------------------------------------

  // String overload.
  // Returns AK_Success if input flags are supported and the resulting path is not too long.
  // Returns AK_Fail otherwise.
  AKRESULT GetFullFilePath(
    const AkOSChar* in_pszFileName,		// File name.
    AkFileSystemFlags* in_pFlags,			// Special flags. Can be NULL.
    AkOpenMode			in_eOpenMode,		// File open mode (read, write, ...).
    AkOSChar* out_pszFullFilePath // Full file path.
  );

  // ID overload. 
  // The name of the file will be formatted as ID.ext. This is meant to be used with options
  // "Use SoundBank Names" unchecked, and/or "Copy Streamed Files" in the SoundBank Settings.
  // For more details, refer to the SoundBank Settings in Wwise Help, and to section "Identifying Banks" inside
  // "Sound Engine Integration Walkthrough > Integrate Wwise Elements into Your Game > Integrating Banks > 
  // Integration Details - Banks > General Information" of the SDK documentation.
  // Returns AK_Success if input flags are supported and the resulting path is not too long.
  // Returns AK_Fail otherwise.
  AKRESULT GetFullFilePath(
    AkFileID			in_fileID,			// File ID.
    AkFileSystemFlags* in_pFlags,			// Special flags. 
    AkOpenMode			in_eOpenMode,		// File open mode (read, write, ...).
    AkOSChar* out_pszFullFilePath	// Full file path.
  );

protected:

  // Internal user paths.
  AkOSChar			m_szBasePath[AK_MAX_PATH];
  AkOSChar			m_szBankPath[AK_MAX_PATH];
  AkOSChar			m_szAudioSrcPath[AK_MAX_PATH];

};




//-----------------------------------------------------------------------------
// Name: AkFilePackageReader 
// Desc: This class wraps an AK::IAkStdStream to read a file package.
//-----------------------------------------------------------------------------
class AkFilePackageReader
{
public:
  AkFilePackageReader()
    : m_pStream(NULL), m_uBlockSize(0) {}
  ~AkFilePackageReader()
  {
    // IMPORTANT: Do not close file. This object can be copied.
  }

  AKRESULT Open(
    const AkOSChar* in_pszFilePackageName,	// File package name. Location is resolved using base class' Open().
    bool in_bReadFromSFXOnlyDir		// If true, the file package is opened from the language agnostic directory only. Otherwise, it tries to open it 
                    // from the current language-specific directory first, and then from the common directory if it fails, similarly to the soundbanks loader of the Sound Engine (Default).
  )
  {
    AkFileSystemFlags flags;
    flags.uCompanyID = AKCOMPANYID_AUDIOKINETIC;
    flags.uCodecID = AKCODECID_FILE_PACKAGE;
    flags.uCustomParamSize = 0;
    flags.pCustomParam = NULL;
    flags.bIsLanguageSpecific = !in_bReadFromSFXOnlyDir;
    flags.bIsFromRSX = false;

    AKRESULT eResult = AK::IAkStreamMgr::Get()->CreateStd(
      in_pszFilePackageName,
      &flags,
      AK_OpenModeRead,
      m_pStream,
      true);

    if (eResult != AK_Success
      && !in_bReadFromSFXOnlyDir)
    {
      // Try again, in SFX-only directory.
      flags.bIsLanguageSpecific = false;
      eResult = AK::IAkStreamMgr::Get()->CreateStd(
        in_pszFilePackageName,
        &flags,
        AK_OpenModeRead,
        m_pStream,
        true);
    }

    return eResult;
  }

  AKRESULT Read(
    void* in_pBuffer,			// Buffer. Must be aligned with value returned by GetBlockSize().
    AkUInt32		in_uSizeToRead,		// Size to read. Must be a multiple of value returned by GetBlockSize().
    AkUInt32& out_uSizeRead,		// Returned size read.
    AkPriority		in_priority = AK_DEFAULT_PRIORITY,	// Priority heuristic.
    AkReal32		in_fThroughput = 0	// Throughput heuristic. 0 means "not set", and results in "immediate".
  )
  {
    AKASSERT(m_pStream);
    AkReal32 fDeadline = (in_fThroughput > 0) ? in_uSizeToRead / in_fThroughput : 0;
    return m_pStream->Read(
      in_pBuffer,
      in_uSizeToRead,
      true,
      in_priority,
      fDeadline,
      out_uSizeRead);
  }

  AKRESULT Seek(
    AkUInt32		in_uPosition,
    AkUInt32& out_uRealOffset
  )
  {
    AkInt64 iRealOffset;
    AKRESULT eResult = m_pStream->SetPosition(in_uPosition, AK_MoveBegin, &iRealOffset);
    AKASSERT(eResult == AK_Success || !"Failed changing stream position");
    out_uRealOffset = (AkUInt32)iRealOffset;
    return eResult;
  }

  void Close()
  {
    if (m_pStream)
      m_pStream->Destroy();
    m_pStream = NULL;
  }

  void SetName(
    const AkOSChar*
#ifndef AK_OPTIMIZED
    in_pszName
#endif
  )
  {
#ifndef AK_OPTIMIZED
    AKASSERT(m_pStream);
    m_pStream->SetStreamName(in_pszName);
#endif
  }

  AkUInt64 GetSize()
  {
    AKASSERT(m_pStream);
    AkStreamInfo info;
    m_pStream->GetInfo(info);
    return info.uSize;
  }

  AkUInt32 GetBlockSize()
  {
    AKASSERT(m_pStream);
    // AK::IAkStdStream::GetBlockSize() is costly. Cache block size.
    if (!m_uBlockSize)
      m_uBlockSize = m_pStream->GetBlockSize();
    return m_uBlockSize;
  }

  AkFileHandle GetHandle()
  {
    AKASSERT(m_pStream);
    AkFileDesc* pFileDesc = (AkFileDesc*)m_pStream->GetFileDescriptor();
    AKASSERT(pFileDesc);
    return pFileDesc->hFile;
  }

  AkFileDesc* GetFileDesc()
  {
    return (AkFileDesc*)m_pStream->GetFileDescriptor();
  }

private:
  AK::IAkStdStream* m_pStream;
  AkUInt32			m_uBlockSize;
};

//-----------------------------------------------------------------------------
// Name: CAkDiskPackage 
// Desc: This class extends the CAkFilePackage class by providing system handle
// management.
// It keeps a copy of the file package reader that was used to read the file package
// header from disk, and uses it to query and cache its low-level system handle
// (AkFileDesc::hFile). This handle is kept open and used directly to read portions
// of the package from disk, corresponding to read requests for the files it 
// contains. The streaming object / package handle is closed when the package
// is destroyed.
//-----------------------------------------------------------------------------
class CAkDiskPackage : public CAkFilePackage
{
public:
  // Factory for disk package.
  // Instantiates a file package object, queries its file handle once and keep in package.
  // Also keeps a copy of its reader object, which is used to close the file handle on destruction.
  static CAkDiskPackage* Create(
    AkFilePackageReader& in_reader,		// File package reader.
    const AkOSChar* in_pszPackageName,	// Name of the file package (for memory monitoring).
    AkMemPoolId			in_memPoolID,		// Memory pool in which the package is created with its lookup table.
    AkUInt32 			in_uHeaderSize,		// File package header size, including the size of the header chunk AKPK_HEADER_CHUNK_DEF_SIZE.
    AkUInt32& out_uReservedHeaderSize, // Size reserved for header, taking mem align into account.
    AkUInt8*& out_pHeaderBuffer	// Returned address of memory for header.
  )
  {
    CAkDiskPackage* pPackage = CAkFilePackage::Create<CAkDiskPackage>(
      in_pszPackageName,
      in_memPoolID,
      in_uHeaderSize,
      in_reader.GetBlockSize(),
      out_uReservedHeaderSize,
      out_pHeaderBuffer);
    if (pPackage)
    {
      pPackage->m_reader = in_reader;				// Copy reader.
      pPackage->m_hFile = in_reader.GetHandle();	// Cache handle.
    }
    return pPackage;
  }

  CAkDiskPackage(AkUInt32 in_uPackageID, AkUInt32 in_uHeaderSize, AkMemPoolId in_poolID, void* in_pToRelease, bool in_bIsInternalPool)
    : CAkFilePackage(in_uPackageID, in_uHeaderSize, in_poolID, in_pToRelease, in_bIsInternalPool)
  { }

  // Override Destroy(): Close 
  virtual void Destroy()
  {
    m_reader.Close();
    CAkFilePackage::Destroy();
  }

  // Fills an AkFileHandle with a value that will be useable by the low-level I/O hook.
  // Disk packages return the package's system handle: the hook reads from the package file itself, with
  // proper offset, to get the data it needs.
  inline void GetHandleForFileDesc(AkFileHandle& out_hFile) { out_hFile = m_hFile; }

  AkFileDesc* GetFileDesc() { return m_reader.GetFileDesc(); }

protected:

  AkFilePackageReader	m_reader;	// READER object. Holds the stream used to read the package. Closed only upon package destruction.
  AkFileHandle		m_hFile;	// Platform-independent file handle (cached from READER).
};

//-----------------------------------------------------------------------------
// Name: class CAkFilePackageLowLevelIO.
// Desc: Extends default Low-level IO implementation with packaged file support.
//		 Base class must implement one of the low-level I/O hooks 
//		 (AK::StreamMgr::IAkIOHookBlocking or AK::StreamMgr::IAkIOHookDeferred)
//		 _and_ the AK::StreamMgr::IAkFileLocationResolver interface.
//		 It must also define the following methods:
//			- void Term()
// Note: This class uses AkFileDesc::uCustomParamSize to store the block size 
//		 of files opened from a package, and relies on the fact that it is 0 
//		 when they are not part of the package.
//-----------------------------------------------------------------------------
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE = CAkDiskPackage>
class CAkFilePackageLowLevelIO : public T_LLIOHOOK_FILELOC
{
public:

  CAkFilePackageLowLevelIO();
  virtual ~CAkFilePackageLowLevelIO();

  // File package loading:
    // Opens a package file, parses its header, fills LUT.
    // Overrides of Open() will search files in loaded LUTs first, then use default Low-Level I/O 
  // services if they cannot be found.
  // Any number of packages can be loaded at a time. Each LUT is searched until a match is found.
  // Returns AK_Success if successful, AK_InvalidLanguage if the current language 
  // does not exist in the LUT (not necessarily an error), AK_Fail for any other reason.
  // Also returns a package ID which can be used to unload it (see UnloadFilePackage()).
  // WARNING: This method is not thread safe. Ensure there are no I/O occurring on this device
  // when loading a file package.
  virtual AKRESULT LoadFilePackage(
    const AkOSChar* in_pszFilePackageName,	// File package name. Location is resolved using base class' Open().
    AkUInt32& out_uPackageID,			// Returned package ID.
    AkMemPoolId		in_memPoolID = AK_DEFAULT_POOL_ID	// Memory pool in which the LUT is written. Passing AK_DEFAULT_POOL_ID will create a new pool automatically. 
                // Note that the bulk of the package's data is stored in VRAM, which is allocated through the VRAM allocation hook.
  );

  // Unload a file package.
  // Returns AK_Success if in_uPackageID exists, AK_Fail otherwise.
  // WARNING: This method is not thread safe. Ensure there are no I/O occurring on this device
  // when unloading a file package.
  virtual AKRESULT UnloadFilePackage(
    AkUInt32	in_uPackageID			// Returned package ID.
  );

  // Unload all file packages.
  // Returns AK_Success;
  // WARNING: This method is not thread safe. Ensure there are no I/O occurring on this device
  // when unloading a file package.
  virtual AKRESULT UnloadAllFilePackages();


  //
  // Overriden base class policies.
  // ---------------------------------------------------------------

    // Clean up.
  void Term();

protected:

  //
  // IAkFileLocationResolver interface overriden methods.
  // ---------------------------------------------------------------

  // Override Open (string): Search file in each LUT first. If it cannot be found, use base class services.
  // If the file is found in the LUTs, open is always synchronous.
  // Applies to AK soundbanks only.
  virtual AKRESULT Open(
    const AkOSChar* in_pszFileName,     // File name.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
    bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
    AkFileDesc& out_fileDesc        // Returned file descriptor.
  );

  // Override Open (ID): Search file in each LUT first. If it cannot be found, use base class services.
  // If the file is found in the LUTs, open is always synchronous.
  virtual AKRESULT Open(
    AkFileID        in_fileID,          // File ID.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
    bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
    AkFileDesc& out_fileDesc        // Returned file descriptor.
  );

  //
// IAkLowLevelIOHook interface overriden methods.
// ---------------------------------------------------------------

// Override Close: Do not close handle if file descriptor is part of the current packaged file.
  virtual AKRESULT Close(
    AkFileDesc& in_fileDesc			// File descriptor.
  );

  // Override GetBlockSize: Get the block size of the LUT if a file package is loaded.
  virtual AkUInt32 GetBlockSize(
    AkFileDesc& in_fileDesc			// File descriptor.
  );

protected:

  // Language change handling.
    // ------------------------------------------

  // Handler for global language change.
  static AK_FUNC(void, LanguageChangeHandler)(
    const AkOSChar* const in_pLanguageName,// New language name.
    void* in_pCookie						// Cookie that was passed to AddLanguageChangeObserver().
    )
  {
    ((CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>*)in_pCookie)->OnLanguageChange(in_pLanguageName);
  }

  // Updates language of all loaded packages. Packages keep a language ID to help them find 
  // language-specific assets quickly.
  void OnLanguageChange(
    const AkOSChar* const in_pLanguageName	// New language name.
  );


  // File package handling methods.
    // ------------------------------------------

  // Loads a file package, with a given file package reader.
  AKRESULT _LoadFilePackage(
    const AkOSChar* in_pszFilePackageName,	// File package name. Location is resolved using base class' Open().
    AkFilePackageReader& in_reader,				// File package reader.
    AkPriority				in_readerPriority,		// File package reader priority heuristic.
    AkMemPoolId				in_memPoolID,			// Memory pool in which the LUT is written. Passing AK_DEFAULT_POOL_ID will create a new pool automatically. 
    T_PACKAGE*& out_pPackage			// Returned package
  );

  // Searches the LUT to find the file data associated with the FileID.
  // Returns AK_Success if the file is found.
  template <class T_FILEID>
  AKRESULT FindPackagedFile(
    T_PACKAGE* in_pPackage,	// Package to search into.
    T_FILEID			in_fileID,		// File ID.
    AkFileSystemFlags* in_pFlags,		// Special flags. Can pass NULL.
    AkFileDesc& out_fileDesc	// Returned file descriptor.
  );

  virtual void InitFileDesc(T_PACKAGE* /*in_pPackage*/, AkFileDesc& /*io_fileDesc*/) {};

  // Returns true if file described by in_fileDesc is in a package.
  inline bool IsInPackage(
    const AkFileDesc& in_fileDesc		// File descriptor.
  )
  {
    return in_fileDesc.uCustomParamSize > 0;
  }

protected:
  // List of loaded packages.
  ListFilePackages	m_packages;
  bool				m_bRegisteredToLangChg;	// True after registering to language change notifications.
};



template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::CAkFilePackageLowLevelIO()
  : m_bRegisteredToLangChg(false)
{
}

template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::~CAkFilePackageLowLevelIO()
{
}

// Initialize/terminate.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
void CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::Term()
{
  UnloadAllFilePackages();
  m_packages.Term();
  if (m_bRegisteredToLangChg)
    AK::StreamMgr::RemoveLanguageChangeObserver(this);
  T_LLIOHOOK_FILELOC::Term();
}

// Override Open (string): Search file in each LUT first. If it cannot be found, use base class services.
// If the file is found in the LUTs, open is always synchronous.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::Open(
  const AkOSChar* in_pszFileName,     // File name.
  AkOpenMode      in_eOpenMode,       // Open mode.
  AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
  bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
  AkFileDesc& out_fileDesc        // Returned file descriptor.
)
{
  // If the file is an AK sound bank, try to find the identifier in the lookup table first.
  if (in_eOpenMode == AK_OpenModeRead
    && in_pFlags)
  {
    if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC
      && in_pFlags->uCodecID == AKCODECID_BANK)
    {
      // Search file in each package.
      ListFilePackages::Iterator it = m_packages.Begin();
      while (it != m_packages.End())
      {
        AkFileID fileID = (*it)->lut.GetSoundBankID(in_pszFileName);

        if (FindPackagedFile((T_PACKAGE*)(*it), fileID, in_pFlags, out_fileDesc) == AK_Success)
        {
          // Found the ID in the lut. 
          io_bSyncOpen = true;	// File is opened, now.
          return AK_Success;
        }
        ++it;
      }
    }
    else if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL)
    {
      // Search file in each package.
      ListFilePackages::Iterator it = m_packages.Begin();
      while (it != m_packages.End())
      {
        AkUInt64 fileID = (*it)->lut.GetExternalID(in_pszFileName);

        if (FindPackagedFile((T_PACKAGE*)(*it), fileID, in_pFlags, out_fileDesc) == AK_Success)
        {
          // Found the ID in the lut. 
          io_bSyncOpen = true;	// File is opened, now.
          return AK_Success;
        }

        ++it;
      }
    }
  }

  // It is not a soundbank, or it is not in the file package LUT. Use default implementation.
  return T_LLIOHOOK_FILELOC::Open(
    in_pszFileName,
    in_eOpenMode,
    in_pFlags,
    io_bSyncOpen,
    out_fileDesc);
}

// Override Open (ID): Search file in each LUT first. If it cannot be found, use base class services.
// If the file is found in the LUTs, open is always synchronous.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::Open(
  AkFileID        in_fileID,          // File ID.
  AkOpenMode      in_eOpenMode,       // Open mode.
  AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
  bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
  AkFileDesc& out_fileDesc        // Returned file descriptor.
)
{
  // Try to find the identifier in the lookup table first.
  if (in_eOpenMode == AK_OpenModeRead
    && in_pFlags
    && in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC)
  {
    // Search file in each package.
    ListFilePackages::Iterator it = m_packages.Begin();
    while (it != m_packages.End())
    {
      if (FindPackagedFile((T_PACKAGE*)(*it), in_fileID, in_pFlags, out_fileDesc) == AK_Success)
      {
        // File found. Return now.
        io_bSyncOpen = true;	// File is opened, now.
        return AK_Success;
      }
      ++it;
    }
  }
  else if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL)
  {
    // Search file in each package.
    ListFilePackages::Iterator it = m_packages.Begin();
    while (it != m_packages.End())
    {
      AkOSChar szFileName[20];
      AK_OSPRINTF(szFileName, 20, AKTEXT("%u.wem"), (unsigned int)in_fileID);
      AkUInt64 fileID = (*it)->lut.GetExternalID(szFileName);

      if (FindPackagedFile((T_PACKAGE*)(*it), fileID, in_pFlags, out_fileDesc) == AK_Success)
      {
        // Found the ID in the lut. 
        io_bSyncOpen = true;	// File is opened, now.
        return AK_Success;
      }

      ++it;
    }
  }

  // If it the fileID is not in the LUT, perform standard path concatenation logic.
  return T_LLIOHOOK_FILELOC::Open(
    in_fileID,
    in_eOpenMode,
    in_pFlags,
    io_bSyncOpen,
    out_fileDesc);
}

// Override Close: Do not close handle if file descriptor is part of the current packaged file.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::Close(
  AkFileDesc& in_fileDesc      // File descriptor.
)
{
  // Do not close handle if it is that of the file package (closed only in UnloadFilePackage()).
  if (!IsInPackage(in_fileDesc))
    return T_LLIOHOOK_FILELOC::Close(in_fileDesc);

  return AK_Success;
}

// Override GetBlockSize: Get the block size of the LUT if a file package is loaded.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AkUInt32 CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::GetBlockSize(
  AkFileDesc& in_fileDesc     // File descriptor.
)
{
  if (IsInPackage(in_fileDesc))
  {
    // This file is part of a package. At Open(), we used the 
    // AkFileDesc.uCustomParamSize field to store the block size.
    return in_fileDesc.uCustomParamSize;
  }
  return T_LLIOHOOK_FILELOC::GetBlockSize(in_fileDesc);
}

// Updates language of all loaded packages. Packages keep a language ID to help them find 
// language-specific assets quickly.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
void CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::OnLanguageChange(
  const AkOSChar* const in_pLanguageName	// New language name.
)
{
  // Set language on all loaded packages.
  ListFilePackages::Iterator it = m_packages.Begin();
  while (it != m_packages.End())
  {
    (*it)->lut.SetCurLanguage(in_pLanguageName);
    ++it;
  }
}

// Searches the LUT to find the file data associated with the FileID.
// Returns AK_Success if the file is found.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
template <class T_FILEID>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::FindPackagedFile(
  T_PACKAGE* in_pPackage,	// Package to search into.
  T_FILEID			in_fileID,		// File ID.
  AkFileSystemFlags* in_pFlags,		// Special flags. Can pass NULL.
  AkFileDesc& out_fileDesc	// Returned file descriptor.
)
{
  AKASSERT(in_pPackage && in_pFlags);
  const CAkFilePackageLUT::AkFileEntry<T_FILEID>* pEntry = in_pPackage->lut.LookupFile(in_fileID, in_pFlags);

  if (pEntry)
  {
    // Fill file descriptor.
    out_fileDesc.deviceID = T_LLIOHOOK_FILELOC::m_deviceID;
    in_pPackage->GetHandleForFileDesc(out_fileDesc.hFile);
    out_fileDesc.iFileSize = pEntry->uFileSize;
    out_fileDesc.uSector = pEntry->uStartBlock;
    out_fileDesc.pCustomParam = NULL;
    // NOTE: We use the uCustomParamSize to store the block size.
    // We will determine whether this file was opened from a package by comparing 
    // uCustomParamSize with 0 (see IsInPackage()).
    out_fileDesc.uCustomParamSize = pEntry->uBlockSize;

    // Deal with custom parameters in derived classes.
    InitFileDesc(in_pPackage, out_fileDesc);
    return AK_Success;
  }
  return AK_FileNotFound;
}

// File package loading:
// Opens a package file, parses its header, fills LUT.
// Overrides of Open() will search files in loaded LUTs first, then use default Low-Level I/O 
// services if they cannot be found.
// Any number of packages can be loaded at a time. Each LUT is searched until a match is found.
// Returns AK_Success if successful, AK_InvalidLanguage if the current language 
// does not exist in the LUT (not necessarily an error), AK_Fail for any other reason.
// Also returns a package ID which can be used to unload it (see UnloadFilePackage()).
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::LoadFilePackage(
  const AkOSChar* in_pszFilePackageName,	// File package name. 
  AkUInt32& out_uPackageID,			// Returned package ID.
  AkMemPoolId			in_memPoolID /*= AK_DEFAULT_POOL_ID	*/ // Memory pool in which the LUT is written. Passing AK_DEFAULT_POOL_ID will create a new pool automatically. 
)
{
  // Open package file.
  AkFilePackageReader filePackageReader;
  AKRESULT eRes = filePackageReader.Open(in_pszFilePackageName, true);	// Open from SFX-only directory.
  if (eRes != AK_Success)
    return eRes;

  filePackageReader.SetName(in_pszFilePackageName);

  T_PACKAGE* pPackage;
  eRes = _LoadFilePackage(in_pszFilePackageName, filePackageReader, AK_DEFAULT_PRIORITY, in_memPoolID, pPackage);
  if (eRes == AK_Success
    || eRes == AK_InvalidLanguage)
  {
    AKASSERT(pPackage);
    // Add to packages list.
    m_packages.AddFirst(pPackage);

    out_uPackageID = pPackage->ID();
  }
  return eRes;
}

// Loads a file package, with a given file package reader.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::_LoadFilePackage(
  const AkOSChar* in_pszFilePackageName,	// File package name. 
  AkFilePackageReader& in_reader,				// File package reader.
  AkPriority				in_readerPriority,		// File package reader priority heuristic.
  AkMemPoolId				in_memPoolID,			// Memory pool in which the LUT is written. Passing AK_DEFAULT_POOL_ID will create a new pool automatically. 
  T_PACKAGE*& out_pPackage			// Returned package
)
{
  // Read header chunk definition.
  struct AkFilePackageHeader
  {
    AkUInt32 uFileFormatTag;
    AkUInt32 uHeaderSize;
  };

  AkUInt32 uReadBufferSize = AkMax(2 * in_reader.GetBlockSize(), sizeof(AkFilePackageHeader));
  AkUInt8* pBufferForHeader = (AkUInt8*)AkAlloca(uReadBufferSize);
  AkUInt32 uSizeToRead;
  bool bAligned = (sizeof(AkFilePackageHeader) % in_reader.GetBlockSize()) > 0;
  if (bAligned)
  {
    // Header size is not a multiple of the required block size. Allocate an aligned buffer on the stack.
    pBufferForHeader += (in_reader.GetBlockSize() - (AkUIntPtr)pBufferForHeader % in_reader.GetBlockSize());
    uSizeToRead = in_reader.GetBlockSize();
  }
  else
  {
    // Header size is a multiple of the required block size. 
    uSizeToRead = sizeof(AkFilePackageHeader);
  }

  AkUInt32 uSizeRead;
  AKRESULT eRes = in_reader.Read(pBufferForHeader, uSizeToRead, uSizeRead, in_readerPriority);
  if (eRes != AK_Success
    || uSizeRead < sizeof(AkFilePackageHeader))
  {
    AKASSERT(!"Could not read package, or package is invalid");
    in_reader.Close();
    return AK_Fail;
  }

  const AkFilePackageHeader& uFileHeader = *(AkFilePackageHeader*)pBufferForHeader;

  if (uFileHeader.uFileFormatTag != AKPK_FILE_FORMAT_TAG
    || 0 == uFileHeader.uHeaderSize)
  {
    AKASSERT(!"Invalid file package header");
    in_reader.Close();
    return AK_Fail;
  }

  // Create file package.
  AkUInt32 uReservedHeaderSize;
  AkUInt8* pFilePackageHeader;
  out_pPackage = T_PACKAGE::Create(
    in_reader,
    in_pszFilePackageName,
    in_memPoolID,
    uFileHeader.uHeaderSize + AKPK_HEADER_CHUNK_DEF_SIZE,	// NOTE: The header size written in the file package excludes the AKPK_HEADER_CHUNK_DEF_SIZE.
    uReservedHeaderSize,
    pFilePackageHeader);
  if (!out_pPackage)
  {
    AKASSERT(!"Could not create file package");
    in_reader.Close();
    return AK_Fail;
  }

  AkUInt32 uHeaderSize = uFileHeader.uHeaderSize;
  AkUInt32 uHeaderReadOffset = AKPK_HEADER_CHUNK_DEF_SIZE;

  // If we had already read more than sizeof(AkFilePackageHeader), copy the rest now.
  if (uSizeRead > sizeof(AkFilePackageHeader))
  {
    pBufferForHeader += sizeof(AkFilePackageHeader);
    AkUInt32 uSizeToCopy = uSizeRead - sizeof(AkFilePackageHeader);
    AKPLATFORM::AkMemCpy(pFilePackageHeader + AKPK_HEADER_CHUNK_DEF_SIZE, pBufferForHeader, uSizeToCopy);
    // Adjust header size and read offset.
    if (uSizeToCopy > uHeaderSize)
      uSizeToCopy = uHeaderSize;
    uHeaderSize -= uSizeToCopy;
    uHeaderReadOffset += uSizeToCopy;
    // Round it up to required block size. It should be equal to the size that was reserved (minus what was already read).
    uHeaderSize = ((uHeaderSize + in_reader.GetBlockSize() - 1) / in_reader.GetBlockSize()) * in_reader.GetBlockSize();
    AKASSERT(uHeaderSize == uReservedHeaderSize - uSizeRead);
  }

  // Stream in remaining of the header.
  if (uHeaderSize > 0)
  {
    AKASSERT(uHeaderReadOffset % in_reader.GetBlockSize() == 0);
    if (in_reader.Read(pFilePackageHeader + uHeaderReadOffset, uHeaderSize, uSizeRead, in_readerPriority) != AK_Success
      || uSizeRead < uHeaderSize)
    {
      AKASSERT(!"Could not read file package");
      out_pPackage->Destroy();
      return AK_Fail;
    }
  }

  // Parse LUT.
  eRes = out_pPackage->lut.Setup(pFilePackageHeader, uFileHeader.uHeaderSize + AKPK_HEADER_CHUNK_DEF_SIZE);
  if (eRes != AK_Success)
  {
    out_pPackage->Destroy();
    return eRes;
  }

  // Register to language change notifications if it wasn't already done
  if (!m_bRegisteredToLangChg)
  {
    if (AK::StreamMgr::AddLanguageChangeObserver(LanguageChangeHandler, this) != AK_Success)
    {
      out_pPackage->Destroy();
      return AK_Fail;
    }
    m_bRegisteredToLangChg = true;
  }

  // Use the current language path (if defined) to set the language ID, 
    // for language specific file mapping.
  return out_pPackage->lut.SetCurLanguage(AK::StreamMgr::GetCurrentLanguage());
}

// Unload a file package.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::UnloadFilePackage(
  AkUInt32	in_uPackageID			// Package ID.
)
{
  ListFilePackages::IteratorEx it = m_packages.BeginEx();
  while (it != m_packages.End())
  {
    if ((*it)->ID() == in_uPackageID)
    {
      CAkFilePackage* pPackage = (*it);
      it = m_packages.Erase(it);

      // Destroy package.
      pPackage->Destroy();

      return AK_Success;
    }
    else
      ++it;
  }

  AKASSERT(!"Invalid package ID");
  return AK_Fail;
}

// Unload all file packages.
template <class T_LLIOHOOK_FILELOC, class T_PACKAGE>
AKRESULT CAkFilePackageLowLevelIO<T_LLIOHOOK_FILELOC, T_PACKAGE>::UnloadAllFilePackages()
{
  ListFilePackages::IteratorEx it = m_packages.BeginEx();
  while (it != m_packages.End())
  {
    CAkFilePackage* pPackage = (*it);
    it = m_packages.Erase(it);

    // Destroy package.
    pPackage->Destroy();
  }

  return AK_Success;
}



//-----------------------------------------------------------------------------
// Name: class CAkDefaultIOHookBlocking.
// Desc: Implements IAkIOHookBlocking low-level I/O hook, and 
//		 IAkFileLocationResolver. Can be used as a standalone Low-Level I/O
//		 system, or as part of a system with multiple devices.
//		 File location is resolved using simple path concatenation logic
//		 (implemented in CAkFileLocationBase).
//-----------------------------------------------------------------------------
class CAkDefaultIOHookBlocking : public AK::StreamMgr::IAkFileLocationResolver
  , public AK::StreamMgr::IAkIOHookBlocking
  , public CAkFileLocationBase
{
public:

  CAkDefaultIOHookBlocking();
  virtual ~CAkDefaultIOHookBlocking();

  // Initialization/termination. Init() registers this object as the one and 
  // only File Location Resolver if none were registered before. Then 
  // it creates a streaming device with scheduler type AK_SCHEDULER_BLOCKING.
  AKRESULT Init(
    const AkDeviceSettings& in_deviceSettings,	// Device settings.
    bool						in_bAsyncOpen = false	// If true, files are opened asynchronously when possible.
  );
  void Term();


  //
  // IAkFileLocationAware interface.
  //-----------------------------------------------------------------------------

  // Returns a file descriptor for a given file name (string).
  virtual AKRESULT Open(
    const AkOSChar* in_pszFileName,		// File name.
    AkOpenMode				in_eOpenMode,		// Open mode.
    AkFileSystemFlags* in_pFlags,			// Special flags. Can pass NULL.
    bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
    AkFileDesc& out_fileDesc        // Returned file descriptor.
  );

  // Returns a file descriptor for a given file ID.
  virtual AKRESULT Open(
    AkFileID				in_fileID,          // File ID.
    AkOpenMode				in_eOpenMode,       // Open mode.
    AkFileSystemFlags* in_pFlags,			// Special flags. Can pass NULL.
    bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
    AkFileDesc& out_fileDesc        // Returned file descriptor.
  );


  //
  // IAkIOHookBlocking interface.
  //-----------------------------------------------------------------------------

  // Reads data from a file (synchronous). 
  virtual AKRESULT Read(
    AkFileDesc& in_fileDesc,        // File descriptor.
    const AkIoHeuristics& in_heuristics,		// Heuristics for this data transfer.
    void* out_pBuffer,        // Buffer to be filled with data.
    AkIOTransferInfo& io_transferInfo		// Synchronous data transfer info. 
  );

  // Writes data to a file (synchronous). 
  virtual AKRESULT Write(
    AkFileDesc& in_fileDesc,        // File descriptor.
    const AkIoHeuristics& in_heuristics,		// Heuristics for this data transfer.
    void* in_pData,           // Data to be written.
    AkIOTransferInfo& io_transferInfo		// Synchronous data transfer info. 
  );

  // Cleans up a file.
  virtual AKRESULT Close(
    AkFileDesc& in_fileDesc			// File descriptor.
  );

  // Returns the block size for the file or its storage device. 
  virtual AkUInt32 GetBlockSize(
    AkFileDesc& in_fileDesc			// File descriptor.
  );

  // Returns a description for the streaming device above this low-level hook.
  virtual void GetDeviceDesc(
    AkDeviceDesc& out_deviceDesc      // Device description.
  );

  // Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
  virtual AkUInt32 GetDeviceData();

protected:
  AkDeviceID	m_deviceID;
  bool		m_bAsyncOpen;	// If true, opens files asynchronously when it can.
};

class CAkFilePackageLowLevelIOBlocking
  : public CAkFilePackageLowLevelIO<CAkDefaultIOHookBlocking>
{
public:
  CAkFilePackageLowLevelIOBlocking() {}
  virtual ~CAkFilePackageLowLevelIOBlocking() {}
};


class CAkFileHelpers
{
public:

  // Wrapper for Win32 CreateFile().
  static AKRESULT OpenFile(
    const AkOSChar* in_pszFilename,     // File name.
    AkOpenMode      in_eOpenMode,       // Open mode.
    bool            in_bOverlappedIO,	// Use FILE_FLAG_OVERLAPPED flag.
    bool            in_bUnbufferedIO,   // Use FILE_FLAG_NO_BUFFERING flag.
    AkFileHandle& out_hFile           // Returned file identifier/handle.
  )
  {
    // Check parameters.
    if (!in_pszFilename)
    {
      AKASSERT(!"NULL file name");
      return AK_InvalidParameter;
    }

    // Open mode
    DWORD dwShareMode;
    DWORD dwAccessMode;
    DWORD dwCreationDisposition;
    switch (in_eOpenMode)
    {
    case AK_OpenModeRead:
      dwShareMode = FILE_SHARE_READ;
      dwAccessMode = GENERIC_READ;
      dwCreationDisposition = OPEN_EXISTING;
      break;
    case AK_OpenModeWrite:
      dwShareMode = FILE_SHARE_WRITE;
      dwAccessMode = GENERIC_WRITE;
      dwCreationDisposition = OPEN_ALWAYS;
      break;
    case AK_OpenModeWriteOvrwr:
      dwShareMode = FILE_SHARE_WRITE;
      dwAccessMode = GENERIC_WRITE;
      dwCreationDisposition = CREATE_ALWAYS;
      break;
    case AK_OpenModeReadWrite:
      dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
      dwAccessMode = GENERIC_READ | GENERIC_WRITE;
      dwCreationDisposition = OPEN_ALWAYS;
      break;
    default:
      AKASSERT(!"Invalid open mode");
      out_hFile = NULL;
      return AK_InvalidParameter;
      break;
    }

    // Flags
    DWORD dwFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    if (in_bUnbufferedIO && in_eOpenMode == AK_OpenModeRead)
      dwFlags |= FILE_FLAG_NO_BUFFERING;
    if (in_bOverlappedIO)
      dwFlags |= FILE_FLAG_OVERLAPPED;

    // Create the file handle.
#if defined(AK_USE_METRO_API) && !defined(AK_XBOXONE) // Xbox One can use "normal" IO
    out_hFile = ::CreateFile2(
      in_pszFilename,
      dwAccessMode,
      dwShareMode,
      dwCreationDisposition,
      NULL);
#else
    out_hFile = ::CreateFileW(
      in_pszFilename,
      dwAccessMode,
      dwShareMode,
      NULL,
      dwCreationDisposition,
      dwFlags,
      NULL);
#endif
    if (out_hFile == INVALID_HANDLE_VALUE)
    {
      DWORD dwAllocError = ::GetLastError();
      if (ERROR_FILE_NOT_FOUND == dwAllocError ||
        ERROR_PATH_NOT_FOUND == dwAllocError)
        return AK_FileNotFound;

      return AK_Fail;
    }

    return AK_Success;
  }

  // Wrapper for system file handle closing.
  static AKRESULT CloseFile(AkFileHandle in_hFile)
  {
    if (::CloseHandle(in_hFile))
      return AK_Success;

    AKASSERT(!"Failed to close file handle");
    return AK_Fail;
  }

  //
  // Simple platform-independent API to open and read files using AkFileHandles, 
  // with blocking calls and minimal constraints.
  // ---------------------------------------------------------------------------

  // Open file to use with ReadBlocking().
  static AKRESULT OpenBlocking(
    const AkOSChar* in_pszFilename,     // File name.
    AkFileHandle& out_hFile           // Returned file handle.
  )
  {
    return OpenFile(
      in_pszFilename,
      AK_OpenModeRead,
      false,
      false,
      out_hFile);
  }

  // Required block size for reads (used by ReadBlocking() below).
  static const AkUInt32 s_uRequiredBlockSize = 1;

  // Simple blocking read method.
  static AKRESULT ReadBlocking(
    AkFileHandle& in_hFile,			// Returned file identifier/handle.
    void* in_pBuffer,			// Buffer. Must be aligned on CAkFileHelpers::s_uRequiredBlockSize boundary.
    AkUInt32		in_uPosition,		// Position from which to start reading.
    AkUInt32		in_uSizeToRead,		// Size to read. Must be a multiple of CAkFileHelpers::s_uRequiredBlockSize.
    AkUInt32& out_uSizeRead		// Returned size read.        
  )
  {
    AKASSERT(in_uSizeToRead % s_uRequiredBlockSize == 0
      && in_uPosition % s_uRequiredBlockSize == 0);

#ifdef AK_USE_METRO_API
    LARGE_INTEGER uPosition;
    uPosition.QuadPart = in_uPosition;
    if (SetFilePointerEx(in_hFile, uPosition, NULL, FILE_BEGIN) == FALSE)
      return AK_Fail;
#else
    if (SetFilePointer(in_hFile, in_uPosition, NULL, FILE_BEGIN) != in_uPosition)
      return AK_Fail;
#endif
    if (::ReadFile(in_hFile, in_pBuffer, in_uSizeToRead, &out_uSizeRead, NULL))
      return AK_Success;
    return AK_Fail;
  }
};










//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// SoundInputBase implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
SoundInputBase::SoundInputBase()
  :m_playingID(AK_INVALID_PLAYING_ID)
{
}

SoundInputBase::~SoundInputBase()
{
}

void SoundInputBase::SetPlayingID(AkPlayingID in_playingID)
{
  m_playingID = in_playingID;
}

AkPlayingID SoundInputBase::GetPlayingID()
{
  return m_playingID;
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// SoundInputMgrBase implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

bool SoundInputMgrBase::Initialize()
{
  // Reset members.
  Reset();
  // Set Wwise required Callbacks
#ifndef AK_3DS
  SetAudioInputCallbacks(ExecuteCallback, GetFormatCallback);
#endif

  // Cannot fail for now, return true.
  return true;
}

void SoundInputMgrBase::Term()
{
}

void SoundInputMgrBase::GetFormatCallback(
  AkPlayingID		in_playingID,   // Playing ID
  AkAudioFormat& io_AudioFormat  // Already filled format, modify it if required.
)
{
  SoundInputBase* pInput = SoundInputMgr::Instance().GetInput(in_playingID);
  if (pInput)
  {
    pInput->GetFormatCallback(io_AudioFormat);
  }
}

void SoundInputMgrBase::ExecuteCallback(
  AkPlayingID		in_playingID,  // Playing ID
  AkAudioBuffer* io_pBufferOut  // Buffer to fill
)
{
  io_pBufferOut->uValidFrames = 0;

  SoundInputBase* pInput = SoundInputMgr::Instance().GetInput(in_playingID);
  if (pInput == NULL)
  {
    // No Input... fake starvation, we are waiting the user to connect in the microphone.
    io_pBufferOut->eState = AK_NoDataReady;
    io_pBufferOut->uValidFrames = 0;
    return;
  }

  // Execute will fill io_pBufferOut->uValidFrames with the correct value.
  pInput->Execute(io_pBufferOut);

  if (io_pBufferOut->uValidFrames > 0)
    io_pBufferOut->eState = AK_DataReady;
  else
    io_pBufferOut->eState = AK_NoDataReady;
}

SoundInputBase* SoundInputMgrBase::GetInput(AkPlayingID in_playingID)
{
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].pInput && m_DeviceInputAssoc[i].pInput->GetPlayingID() == in_playingID)
    {
      return m_DeviceInputAssoc[i].pInput;
    }
  }
#ifdef AK_WIN
  // Default to find one, any...
  // A real game should not adopt this behavior, this should only be done for playback in Wwise.
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].pInput)
    {
      return m_DeviceInputAssoc[i].pInput;
    }
  }
#endif //WIN32
  return NULL;
}

void SoundInputMgrBase::Reset()
{
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    m_DeviceInputAssoc[i].idDevice = INVALID_DEVICE_ID;
    m_DeviceInputAssoc[i].pInput = NULL;
  }

  m_MicCount = 0;
}

void SoundInputMgrBase::ClearDevices()
{
  Reset();
}

bool SoundInputMgrBase::RegisterDevice(SoundInputDevID in_idDevice)
{
  int idxFree = ~0;

  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].idDevice == INVALID_DEVICE_ID && idxFree == ~0)
      idxFree = i;
    else if (m_DeviceInputAssoc[i].idDevice == in_idDevice)
      return false;
  }

  if (idxFree != ~0)
  {
    AKASSERT(m_DeviceInputAssoc[idxFree].pInput == NULL);
    m_DeviceInputAssoc[idxFree].idDevice = in_idDevice;
    m_DeviceInputAssoc[idxFree].pInput = NULL;
    m_MicCount++;
    return true;
  }

  //printf("Cannot register more than MAX_NUM_SOUNDINPUT microphone devices simultaneously\n");
  return false;
}

void SoundInputMgrBase::UnregisterDevice(SoundInputDevID in_idDevice)
{
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].idDevice == in_idDevice)
    {
      //AKASSERT( m_DeviceInputAssoc[i].pInput == NULL );
      m_DeviceInputAssoc[i].idDevice = INVALID_DEVICE_ID;
      m_DeviceInputAssoc[i].pInput = NULL;
      m_MicCount = (m_MicCount > 0) ? m_MicCount - 1 : 0;
      break;
    }
  }
}

bool SoundInputMgrBase::RegisterInput(SoundInputBase* in_pSoundInput, SoundInputDevID in_idDevice)
{
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].idDevice == in_idDevice)
    {
      if (m_DeviceInputAssoc[i].pInput)
        return false;

      m_DeviceInputAssoc[i].pInput = in_pSoundInput;
      return true;
    }
  }

  //printf("Cannot register more than MAX_NUM_SOUNDINPUT microphone devices simultaneously\n");
  return false;
}

void SoundInputMgrBase::UnregisterInput(SoundInputBase* in_pSoundInput, SoundInputDevID in_idDevice)
{
  for (int i = 0; i < MAX_NUM_SOUNDINPUT; ++i)
  {
    if (m_DeviceInputAssoc[i].pInput == in_pSoundInput)
    {
      AKASSERT(m_DeviceInputAssoc[i].idDevice == in_idDevice);
      m_DeviceInputAssoc[i].pInput = NULL;
      break;
    }
  }
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// SoundInputMgr implementation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
SoundInputMgrBase& SoundInputMgr::Instance()
{
  static SoundInputMgr Singleton;
  return Singleton;
}

bool SoundInputMgr::Initialize()
{
  return SoundInputMgrBase::Initialize();
}

void SoundInputMgr::Term()
{
  SoundInputMgrBase::Term();
}





// The sample rate
#define AUDIO_INPUT_SAMPLE_RATE 48000

#define BUFFER_STATUS_NOT_READY 0
#define BUFFER_STATUS_READY 1

SoundInput SoundInput::ms_Instance;

// Writes data from output into given buffer
void CALLBACK AudioCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

//---------------------------------------------------------------------------
SoundInput::SoundInput()
  : m_CurrentInputBufferIndex(0)
  , m_RemainingSize(0)
  , m_bStopRefillThread(false)
  , m_bSuccessfullyInitOnceAndMustSleepOnExit(false)
{
  AKPLATFORM::AkClearEvent(m_eventRefill);
  AKPLATFORM::AkClearThread(&m_RefillThread);
  m_WaveHeader[0].lpData = NULL;
  m_WaveInHandle = NULL;
}

//---------------------------------------------------------------------------
SoundInput::~SoundInput()
{
  InputOff();

  // Avoid deadlock in waveIn
  if (m_bSuccessfullyInitOnceAndMustSleepOnExit)
    Sleep(100);
}

//---------------------------------------------------------------------------
bool SoundInput::InputOn(unsigned int in_DevNumber)
{
  //Reset some members.
  m_WaveInHandle = NULL;

  (void)in_DevNumber;// Remove warning while not supporting multiple microphones.

  m_idDevice = 0;

  // In this sample we use MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE to read
  // audio data from the microphone and feed it to the Audio Input plug-in.
  // Other MIXERLINE_COMPONENTTYPE_SRC_* component types exist for other
  // types of input -- See MIXERLINE documentation in MSDN for more information.
  if (SetInputDevice(MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE) == FALSE)
  {
    return false;
  }

  MMRESULT err;

  // Clear out both of our WAVEHDRs. At the very least, waveInPrepareHeader() expects the dwFlags field to be cleared
  ZeroMemory(&m_WaveHeader, sizeof(m_WaveHeader));

  // Initialize the WAVEFORMATEX for 16-bit, 48KHz, mono. That's what we want to record 
  m_WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
  m_WaveFormat.nChannels = 1;
  m_WaveFormat.nSamplesPerSec = AUDIO_INPUT_SAMPLE_RATE;
  m_WaveFormat.wBitsPerSample = 16;
  m_WaveFormat.nBlockAlign = m_WaveFormat.nChannels * (m_WaveFormat.wBitsPerSample / 8);
  m_WaveFormat.nAvgBytesPerSec = m_WaveFormat.nSamplesPerSec * m_WaveFormat.nBlockAlign;
  m_WaveFormat.cbSize = 0;

  // Open the default WAVE In Device, specifying my callback. Note that if this device doesn't
  // support 16-bit, 48KHz, mono recording, then Windows will attempt to open another device
  // that does. So don't make any assumptions about the name of the device that opens. After
  // waveInOpen returns the handle, use waveInGetID to fetch its ID, and then waveInGetDevCaps
  // to retrieve the actual name	                         
  if ((err = waveInOpen(&m_WaveInHandle,
    WAVE_MAPPER,
    &m_WaveFormat,
    (DWORD_PTR)AudioCallback,
    (DWORD_PTR)this,
    CALLBACK_FUNCTION)) != 0)
  {
    goto err_cond;
  }

  // Initialize the remaining size.
  m_RemainingSize = GetFullBufferSize();

  // Allocate, prepare, and queue two buffers that the driver can use to record blocks of audio data.
  // (ie, We're using double-buffering. You can use more buffers if you'd like, and you should do that
  // if you suspect that you may lag the driver when you're writing a buffer to disk and are too slow
  // to requeue it with waveInAddBuffer. With more buffers, you can take your time requeueing
  // each).
  //
  // Just to make it easy, I'll use 1 call to VirtualAlloc to allocate both buffers, but you can
  // use 2 separate calls since the buffers do NOT need to be contiguous. You should do the latter if
  // using many, large buffers
  m_WaveHeader[0].dwBufferLength = GetFullBufferSize();
  if (m_WaveHeader[0].lpData == NULL)
  {
    m_WaveHeader[0].lpData = (char*)VirtualAlloc(0, m_WaveHeader[0].dwBufferLength * AUDIO_INPUT_NUM_BUFFERS, MEM_COMMIT, PAGE_READWRITE);
    if (!m_WaveHeader[0].lpData)
    {
      goto err_cond;
    }
  }

  // Prepare the WAVEHDR's 
  for (int i = 0; i < AUDIO_INPUT_NUM_BUFFERS; ++i)
  {
    if (i > 0)
    {
      m_WaveHeader[i].dwBufferLength = GetFullBufferSize();
      m_WaveHeader[i].lpData = m_WaveHeader[i - 1].lpData + GetFullBufferSize();
    }
    m_WaveHeader[i].dwUser = BUFFER_STATUS_NOT_READY;

    err = waveInPrepareHeader(m_WaveInHandle, &m_WaveHeader[i], sizeof(WAVEHDR));
    if (err)
      goto err_cond;

    err = waveInAddBuffer(m_WaveInHandle, &m_WaveHeader[i], sizeof(WAVEHDR));
    if (err)
      goto err_cond;
  }

  if (m_WaveHeader[0].lpData)
  {
    // clear write buffer and set read pointer half-way into buffer (to allow for read
    // pointer to catch up)
    memset(m_WaveHeader[0].lpData, 0, GetFullBufferSize() * AUDIO_INPUT_NUM_BUFFERS);
  }

  if (StartRefillThread() != AK_Success)
  {
    goto err_cond;
  }

  // start getting input
  if ((waveInStart(m_WaveInHandle)))
  {
    goto err_cond;
  }

  if (!SoundInputMgr::Instance().RegisterInput(this, m_idDevice))
  {
    goto err_cond;
  }

  m_bSuccessfullyInitOnceAndMustSleepOnExit = true;
  return true;

err_cond:
  InputOff();
  return false;
}

//---------------------------------------------------------------------------
bool SoundInput::InputOff()
{
  StopRefillThread();
  SoundInputMgr::Instance().UnregisterInput(this, m_idDevice);

  // stop getting input
  waveInStop(m_WaveInHandle);
  waveInClose(m_WaveInHandle);
  m_WaveInHandle = NULL;

  m_CurrentInputBufferIndex = 0;
  m_RemainingSize = 0;

  // Free the recording buffers
  if (m_WaveHeader[0].lpData)
  {
    VirtualFree(m_WaveHeader[0].lpData, (m_WaveHeader[0].dwBufferLength << 1), MEM_RELEASE);
    m_WaveHeader[0].lpData = NULL;
  }

  return true;
}

//---------------------------------------------------------------------------
BOOL SoundInput::SetInputDevice(unsigned int inDev)
{
  HMIXER hMixer = NULL;
  int    inDevIdx = -1;

  if (mixerOpen(&hMixer, 0, 0, NULL, MIXER_OBJECTF_MIXER) != MMSYSERR_NOERROR)
  {
    return FALSE;
  }

  // get dwLineID
  MIXERLINE mxl;
  mxl.cbStruct = sizeof(MIXERLINE);
  mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

  if (mixerGetLineInfo((HMIXEROBJ)hMixer, &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
  {
    mixerClose(hMixer);
    return TRUE; // In vista, mixerGetLineInfo doesn't work but the rest still might.
  }

  // get dwControlID
  MIXERCONTROL      mxc;
  MIXERLINECONTROLS mxlc;
  DWORD             dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;

  mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
  mxlc.dwLineID = mxl.dwLineID;
  mxlc.dwControlType = dwControlType;
  mxlc.cControls = 0;
  mxlc.cbmxctrl = sizeof(MIXERCONTROL);
  mxlc.pamxctrl = &mxc;

  if (mixerGetLineControls((HMIXEROBJ)hMixer, &mxlc, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
  {
    // no mixer, try MUX
    dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
    mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlc.dwLineID = mxl.dwLineID;
    mxlc.dwControlType = dwControlType;
    mxlc.cControls = 0;
    mxlc.cbmxctrl = sizeof(MIXERCONTROL);
    mxlc.pamxctrl = &mxc;
    if (mixerGetLineControls((HMIXEROBJ)hMixer, &mxlc, MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
    {
      mixerClose(hMixer);
      return FALSE;
    }
  }

  if (mxc.cMultipleItems <= 0)
  {
    mixerClose(hMixer);
    return FALSE;
  }

  // get the index of the inDevice from available controls
  MIXERCONTROLDETAILS_LISTTEXT* pmxcdSelectText = new MIXERCONTROLDETAILS_LISTTEXT[mxc.cMultipleItems];

  if (pmxcdSelectText != NULL)
  {
    MIXERCONTROLDETAILS mxcd;

    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = mxc.cMultipleItems;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
    mxcd.paDetails = pmxcdSelectText;

    if (mixerGetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_LISTTEXT) == MMSYSERR_NOERROR)
    {
      // determine which controls the inputDevice source line
      DWORD dwi;
      for (dwi = 0; dwi < mxc.cMultipleItems; dwi++)
      {
        // get the line information
        MIXERLINE mxl;
        mxl.cbStruct = sizeof(MIXERLINE);
        mxl.dwLineID = pmxcdSelectText[dwi].dwParam1;
        if (mixerGetLineInfo((HMIXEROBJ)hMixer, &mxl, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_LINEID) == MMSYSERR_NOERROR && mxl.dwComponentType == inDev)
        {
          // found, dwi is the index.
          inDevIdx = dwi;
          // break;
        }
      }
    }

    delete[]pmxcdSelectText;
  }

  if (inDevIdx < 0)
  {
    mixerClose(hMixer);
    return FALSE;
  }

  // get all the values first
  MIXERCONTROLDETAILS_BOOLEAN* pmxcdSelectValue = new MIXERCONTROLDETAILS_BOOLEAN[mxc.cMultipleItems];

  if (pmxcdSelectValue != NULL)
  {
    MIXERCONTROLDETAILS mxcd;
    mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
    mxcd.dwControlID = mxc.dwControlID;
    mxcd.cChannels = 1;
    mxcd.cMultipleItems = mxc.cMultipleItems;
    mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mxcd.paDetails = pmxcdSelectValue;
    if (mixerGetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
    {
      // ASSERT(m_dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER || m_dwControlType == MIXERCONTROL_CONTROLTYPE_MUX);

      // MUX restricts the line selection to one source line at a time.
      if (dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
      {
        ZeroMemory(pmxcdSelectValue, mxc.cMultipleItems * sizeof(MIXERCONTROLDETAILS_BOOLEAN));
      }

      // Turn on this input device
      pmxcdSelectValue[inDevIdx].fValue = 0x1;

      mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
      mxcd.dwControlID = mxc.dwControlID;
      mxcd.cChannels = 1;
      mxcd.cMultipleItems = mxc.cMultipleItems;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
      mxcd.paDetails = pmxcdSelectValue;
      if (mixerSetControlDetails((HMIXEROBJ)hMixer, &mxcd, MIXER_OBJECTF_HMIXER | MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
      {
        delete[]pmxcdSelectValue;
        mixerClose(hMixer);
        return FALSE;
      }
    }

    delete[]pmxcdSelectValue;
  }

  mixerClose(hMixer);
  return TRUE;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void CALLBACK AudioCallback(HWAVEIN /*hwi*/, UINT uMsg, DWORD_PTR, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/)
{
  switch (uMsg)
  {
  case WIM_DATA:
  {
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;
    hdr->dwUser = BUFFER_STATUS_READY;
  }
  break;

  case WIM_CLOSE:
    // Notification meaning that the WaveInput was closed.
    // Could be 
    // - A disconnected device
    // - An error in the driver 
    // - Normal closing.
    //
    // No use for this at the moment.
    // If you add something here, make sure you don’t call anything on the system directly from the callback.
    break;

  default:
    break;
  }
}

void SoundInput::GetFormatCallback(
  AkAudioFormat& io_AudioFormat  // Already filled format, modify it if required.
)
{
  io_AudioFormat.SetAll(
    48000,// Sample rate
    AK_SPEAKER_SETUP_MONO,
    16,						// Bits per samples
    2,						// 2 bytes per samples
    AK_INT,					// feeding integers(signed)
    AK_INTERLEAVED
  );
}

void SoundInput::SendBufferRequest(DWORD in_bufferIndex)
{
  m_WaveHeader[in_bufferIndex].dwUser = BUFFER_STATUS_NOT_READY;
  m_WaveHeader[in_bufferIndex].dwBufferLength = GetFullBufferSize();

  // Option 1: Send the request right away.
  /*
  ExecuteBufferRequest( in_bufferIndex );
  */

  // Option 2: Send the request to be executed by another thread, allowing the blocking function to not be called in this thread.
  m_refillLock.Lock();
  m_refillQueue.push(in_bufferIndex);
  m_refillLock.Unlock();
  AKPLATFORM::AkSignalEvent(m_eventRefill);
}

void SoundInput::ExecuteBufferRequest(DWORD in_bufferIndex)
{
  MMRESULT result = waveInPrepareHeader(m_WaveInHandle, &(m_WaveHeader[in_bufferIndex]), sizeof(WAVEHDR));
  if (MMSYSERR_NOERROR == result)
  {
    result = waveInAddBuffer(m_WaveInHandle, &(m_WaveHeader[in_bufferIndex]), sizeof(WAVEHDR));
  }
}

//---------------------------------------------------------------------------
AkUInt16 SoundInput::GetFullBufferSize()
{
  return AUDIO_INPUT_BUFFER_SIZE * sizeof(short);
}

void SoundInput::MoveCurrentIndex()
{
  SendBufferRequest(m_CurrentInputBufferIndex);
  ++m_CurrentInputBufferIndex;
  m_CurrentInputBufferIndex = m_CurrentInputBufferIndex % AUDIO_INPUT_NUM_BUFFERS;
  m_RemainingSize = GetFullBufferSize();
}

void SoundInput::Execute(
  AkAudioBuffer* io_pBufferOut  // Buffer to fill
)
{
  if (m_WaveHeader[0].lpData == NULL)
  {
    io_pBufferOut->eState = AK_Fail;
    io_pBufferOut->uValidFrames = 0;

    return;
  }

  AkUInt32 uMaxFrames = io_pBufferOut->MaxFrames();

  AkInt8* AK_RESTRICT pBufOut = (AkInt8*)(io_pBufferOut->GetChannel(0));

  /////////////////////////////////////////
  DWORD previousBufferIndex = (m_CurrentInputBufferIndex + AUDIO_INPUT_NUM_BUFFERS - 1) % AUDIO_INPUT_NUM_BUFFERS;
#pragma warning( push )
#pragma warning( disable : 4127 )
  if (AUDIO_INPUT_NUM_BUFFERS > 2 && m_WaveHeader[previousBufferIndex].dwUser == BUFFER_STATUS_READY)
#pragma warning( pop )
  {
    // The microphone recording stopped because we did not play in time.
    // It will glitch randomly because we will be on the edge of being late.
    // Reset synchronisation by skipping some buffers.


    for (DWORD i = 0; i < NUM_TARGET_FREE_BUFFERS; ++i)
    {
      MoveCurrentIndex();
    }
  }
  /////////////////////////////////////////

  while (io_pBufferOut->uValidFrames != uMaxFrames)
  {
    WAVEHDR& rCurrentWavHdr = m_WaveHeader[m_CurrentInputBufferIndex];
    if (rCurrentWavHdr.dwUser == BUFFER_STATUS_READY)
    {
      AkUInt16 l_transferSize = min(
        m_RemainingSize,
        (AkUInt16)((uMaxFrames - io_pBufferOut->uValidFrames) * sizeof(AkInt16))
      );

      memcpy(pBufOut,
        rCurrentWavHdr.lpData + (GetFullBufferSize() - m_RemainingSize),
        l_transferSize);

      pBufOut += l_transferSize;

      m_RemainingSize -= l_transferSize;
      io_pBufferOut->uValidFrames += (l_transferSize / sizeof(AkInt16));

      if (m_RemainingSize == 0)
      {
        MoveCurrentIndex();
      }
    }
    else
    {
      break;
    }
  }
}

AK_DECLARE_THREAD_ROUTINE(SoundInput::RefillThreadFunc)
{
  SoundInput& rThis = *reinterpret_cast<SoundInput*>(AK_THREAD_ROUTINE_PARAMETER);
#pragma warning( push )
#pragma warning( disable : 4127 )
  while (true)
#pragma warning( pop )
  {
    AKPLATFORM::AkWaitForEvent(rThis.m_eventRefill);

    if (rThis.m_bStopRefillThread)
      break;

    rThis.m_refillLock.Lock();
    while (!rThis.m_refillQueue.empty())
    {
      DWORD index = rThis.m_refillQueue.front();
      rThis.m_refillQueue.pop();
      rThis.m_refillLock.Unlock();
      rThis.ExecuteBufferRequest(index);
      rThis.m_refillLock.Lock();
    }
    rThis.m_refillLock.Unlock();
  }

  AkExitThread(AK_RETURN_THREAD_OK);
}

//====================================================================================================
//====================================================================================================
AKRESULT SoundInput::StartRefillThread()
{
  if (AKPLATFORM::AkIsValidThread(&m_RefillThread))
  {
    return AK_Fail;
  }

  m_bStopRefillThread = false;

  if (AKPLATFORM::AkCreateEvent(m_eventRefill) != AK_Success)
  {
    return AK_Fail;
  }

  AkThreadProperties threadProps;
  AKPLATFORM::AkGetDefaultThreadProperties(threadProps);
  threadProps.nPriority = AK_THREAD_PRIORITY_ABOVE_NORMAL;

  AKPLATFORM::AkCreateThread(RefillThreadFunc,	// Start address
    this,		 // Parameter
    threadProps, // Thread properties 
    &m_RefillThread,
    "AK::SoundInput");	// Name
  if (!AKPLATFORM::AkIsValidThread(&m_RefillThread))
  {
    return AK_Fail;
  }
  return AK_Success;
}
//====================================================================================================
//====================================================================================================
void SoundInput::StopRefillThread()
{
  m_bStopRefillThread = true;
  if (AKPLATFORM::AkIsValidThread(&m_RefillThread))
  {
    // stop the eventMgr thread
    AKPLATFORM::AkSignalEvent(m_eventRefill);
    AKPLATFORM::AkWaitForSingleThread(&m_RefillThread);

    AKPLATFORM::AkCloseThread(&m_RefillThread);
  }
  AKPLATFORM::AkDestroyEvent(m_eventRefill);
}








//////////////////////////////////////////////////////////////////////
// 
// AkFilePackageLUT.cpp
//
// This class parses the header of file packages that were created with the 
// AkFilePackager utility app (located in ($WWISESDK)/samples/FilePackager/),
// and looks-up files at run-time.
// 
// The header of these file packages contains look-up tables that describe the 
// internal offset of each file it references, their block size (required alignment), 
// and their language. Each combination of AkFileID and Language ID is unique.
//
// The language was created dynamically when the package was created. The header 
// also contains a map of language names (strings) to their ID, so that the proper 
// language-specific version of files can be resolved. The language name that is stored
// matches the name of the directory that is created by the Wwise Bank Manager,
// except for the trailing slash.
//
// Copyright (c) 2007-2009 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
template<bool> struct AkCompileTimeAssert;
template<> struct AkCompileTimeAssert<true> { };
#define AK_STATIC_ASSERT(e) (AkCompileTimeAssert<(e) != 0>())
#else
#define AK_STATIC_ASSERT(e)
#endif

#define AK_MAX_EXTERNAL_NAME_SIZE		260

CAkFilePackageLUT::CAkFilePackageLUT()
  :m_curLangID(AK_INVALID_LANGUAGE_ID)
  , m_pLangMap(NULL)
  , m_pSoundBanks(NULL)
  , m_pStmFiles(NULL)
  , m_pExternals(NULL)
{
  AK_STATIC_ASSERT(sizeof(AkFileEntry<AkFileID>) == 20);
  AK_STATIC_ASSERT(sizeof(AkFileEntry<AkUInt64>) == 24);
}

CAkFilePackageLUT::~CAkFilePackageLUT()
{
}

// Create a new LUT from a packaged file header.
// The LUT sets pointers to appropriate location inside header data (in_pData).
AKRESULT CAkFilePackageLUT::Setup(
  AkUInt8* in_pData,			// Header data.
  AkUInt32			in_uHeaderSize		// Size of file package header.
)
{
  struct FileHeaderFormat
  {
    char				headerDefinition[AKPK_HEADER_CHUNK_DEF_SIZE];
    AkUInt32			uVersion;
    AkUInt32			uLanguageMapSize;
    AkUInt32			uSoundBanksLUTSize;
    AkUInt32			uStmFilesLUTSize;
    AkUInt32			uExternalsLUTSize;
  };
  FileHeaderFormat* pHeader = (FileHeaderFormat*)in_pData;

  // Check header size,
  if (in_uHeaderSize < sizeof(FileHeaderFormat)
    + pHeader->uLanguageMapSize
    + pHeader->uSoundBanksLUTSize
    + pHeader->uStmFilesLUTSize
    + pHeader->uExternalsLUTSize)
  {
    return AK_Fail;
  }

  // Check version.
  if (pHeader->uVersion < AKPK_CURRENT_VERSION)
    return AK_Fail;

  // Get address of maps and LUTs.
  in_pData += sizeof(FileHeaderFormat);

  m_pLangMap = (StringMap*)in_pData;
  in_pData += pHeader->uLanguageMapSize;

  m_pSoundBanks = (FileLUT<AkFileID>*)in_pData;
  in_pData += pHeader->uSoundBanksLUTSize;

  m_pStmFiles = (FileLUT<AkFileID>*)in_pData;
  in_pData += pHeader->uStmFilesLUTSize;

  m_pExternals = (FileLUT<AkUInt64>*)in_pData;

  return AK_Success;
}

// Find a file entry by ID.
const CAkFilePackageLUT::AkFileEntry<AkFileID>* CAkFilePackageLUT::LookupFile(
  AkFileID			in_uID,			// File ID.
  AkFileSystemFlags* in_pFlags		// Special flags. Do not pass NULL.
)
{
  AKASSERT(in_pFlags && in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC);

  if (in_pFlags->uCodecID == AKCODECID_BANK
    && m_pSoundBanks
    && m_pSoundBanks->HasFiles())
  {
    return LookupFile<AkFileID>(in_uID, m_pSoundBanks, in_pFlags->bIsLanguageSpecific);
  }
  else if (m_pStmFiles && m_pStmFiles->HasFiles())
  {
    // We assume that the file is a streamed audio file.
    return LookupFile<AkFileID>(in_uID, m_pStmFiles, in_pFlags->bIsLanguageSpecific);
  }
  // No table loaded.
  return NULL;
}

// Find a file entry by ID.
const CAkFilePackageLUT::AkFileEntry<AkUInt64>* CAkFilePackageLUT::LookupFile(
  AkUInt64			in_uID,			// File ID.
  AkFileSystemFlags* in_pFlags		// Special flags. Do not pass NULL.
)
{
  AKASSERT(in_pFlags);

  if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL
    && m_pExternals
    && m_pExternals->HasFiles())
  {
    return LookupFile<AkUInt64>(in_uID, m_pExternals, in_pFlags->bIsLanguageSpecific);
  }

  // No table loaded.
  return NULL;
}

// Set current language. 
// Returns AK_InvalidLanguage if a package is loaded but the language string cannot be found.
// Returns AK_Success otherwise.
AKRESULT CAkFilePackageLUT::SetCurLanguage(
  const AkOSChar* in_pszLanguage	// Language string.
)
{
  m_curLangID = AK_INVALID_LANGUAGE_ID;
  if (m_pLangMap && in_pszLanguage)
  {
    AkUInt16 uLangID = (AkUInt16)m_pLangMap->GetID(in_pszLanguage);
    if (uLangID == AK_INVALID_UNIQUE_ID
      && m_pLangMap->GetNumStrings() > 1)	// Do not return AK_InvalidLanguage if package contains only SFX data.
    {
      return AK_InvalidLanguage;
    }
    m_curLangID = uLangID;
  }

  return AK_Success;
}

void CAkFilePackageLUT::RemoveFileExtension(AkOSChar* in_pstring)
{
  while (*in_pstring != 0)
  {
    if (*in_pstring == AKTEXT('.'))
    {
      *in_pstring = 0;
      return;
    }
    ++in_pstring;
  }
}

// Find a soundbank ID by its name. 
// Returns AK_INVALID_FILE_ID if no soundbank LUT is loaded.
AkFileID CAkFilePackageLUT::GetSoundBankID(
  const AkOSChar* in_pszBankName	// Soundbank name.
)
{
  // Remove the file extension if it was used.
  AkUInt32 stringSize = (AkUInt32)AKPLATFORM::OsStrLen(in_pszBankName) + 1;
  AkOSChar* pStringWithoutExtension = (AkOSChar*)AkAlloca((stringSize) * sizeof(AkOSChar));
  AKPLATFORM::SafeStrCpy(pStringWithoutExtension, in_pszBankName, stringSize);
  RemoveFileExtension(pStringWithoutExtension);

  // Hash
  return AK::SoundEngine::GetIDFromString(pStringWithoutExtension);
}

AkUInt64 CAkFilePackageLUT::GetExternalID(
  const AkOSChar* in_pszExternalName		// External Source name.
)
{
  char* szString;
  CONVERT_OSCHAR_TO_CHAR(in_pszExternalName, szString);

  size_t stringSize = strlen(szString);

  // 1- Make lower case.
  _MakeLowerA(szString, stringSize);

  AK::FNVHash64 MainHash;
  return MainHash.Compute((const unsigned char*)szString, (unsigned int)stringSize);
}

void CAkFilePackageLUT::_MakeLowerA(char* in_pString, size_t in_strlen)
{
  for (size_t i = 0; i < in_strlen; ++i)
  {
    if (in_pString[i] >= 'A' && in_pString[i] <= 'Z')
    {
      in_pString[i] += 0x20;
    }
  }
}

void CAkFilePackageLUT::_MakeLower(AkOSChar* in_pString)
{
  size_t uStrlen = AKPLATFORM::OsStrLen(in_pString);
  const AkOSChar CaseDiff = AKTEXT('a') - AKTEXT('A');
  for (size_t i = 0; i < uStrlen; ++i)
  {
    if (in_pString[i] >= AKTEXT('A') && in_pString[i] <= AKTEXT('Z'))
    {
      in_pString[i] += CaseDiff;
    }
  }
}

AkUInt32 CAkFilePackageLUT::StringMap::GetID(const AkOSChar* in_pszString)
{
  // Make string lower case.
  size_t uStrLen = AKPLATFORM::OsStrLen(in_pszString) + 1;
  AkOSChar* pszLowerCaseString = (AkOSChar*)AkAlloca(uStrLen * sizeof(AkOSChar));
  AKASSERT(pszLowerCaseString);
  AKPLATFORM::SafeStrCpy(pszLowerCaseString, in_pszString, uStrLen);
  _MakeLower(pszLowerCaseString);

  // 'this' is m_uNumStrings. +1 points to the beginning of the StringEntry array.
  StringEntry* pTable = (StringEntry*)((AkUInt32*)this + 1);

  // Binary search: strings are sorted (case sensitive).
  AkInt32 uTop = 0, uBottom = m_uNumStrings - 1;
  do
  {
    AkInt32 uThis = (uBottom - uTop) / 2 + uTop;
    AkOSChar* pString = (AkOSChar*)((AkUInt8*)this + pTable[uThis].uOffset);
    int iCmp = AKPLATFORM::OsStrCmp(pString, pszLowerCaseString);
    if (0 == iCmp)
      return pTable[uThis].uID;
    else if (iCmp > 0)	//in_pTable[ uThis ].pString > pszLowerCaseString 
      uBottom = uThis - 1;
    else					//in_pTable[ uThis ].pString < pszLowerCaseString 
      uTop = uThis + 1;
  } while (uTop <= uBottom);

  // ID not found.
  return AK_INVALID_UNIQUE_ID;
}






//////////////////////////////////////////////////////////////////////
//
// AkFileLocationBase.cpp
//
// Basic file location resolving: Uses simple path concatenation logic.
// Exposes basic path functions for convenience.
// For more details on resolving file location, refer to section "File Location" inside
// "Going Further > Overriding Managers > Streaming / Stream Manager > Low-Level I/O"
// of the SDK documentation. 
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    AKTEXT("%u.bnk")
#define ID_TO_STRING_FORMAT_WEM     AKTEXT("%u.wem")
#define MAX_EXTENSION_SIZE          (4)     // .xxx
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE+MAX_EXTENSION_SIZE+1)   // null-terminated

template< class TYPE > inline
const TYPE& AkTemplMax(const TYPE& in_left, const TYPE& in_right)
{
  return (in_left < in_right) ? in_right : in_left;
}


CAkFileLocationBase::CAkFileLocationBase()
{
  m_szBasePath[0] = '\0';
  m_szBankPath[0] = '\0';
  m_szAudioSrcPath[0] = '\0';
}

CAkFileLocationBase::~CAkFileLocationBase()
{
}

// String overload.
// Returns AK_Success if input flags are supported and the resulting path is not too long.
// Returns AK_Fail otherwise.
AKRESULT CAkFileLocationBase::GetFullFilePath(
  const AkOSChar* in_pszFileName,		// File name.
  AkFileSystemFlags* in_pFlags,			// Special flags. Can be NULL.
  AkOpenMode			in_eOpenMode,		// File open mode (read, write, ...).
  AkOSChar* out_pszFullFilePath // Full file path.
)
{
  if (!in_pszFileName)
  {
    AKASSERT(!"Invalid file name");
    return AK_InvalidParameter;
  }

  // Prepend string path (basic file system logic).

    // Compute file name with file system paths.
  size_t uiPathSize = AKPLATFORM::OsStrLen(in_pszFileName);

  if (uiPathSize >= AK_MAX_PATH)
  {
    AKASSERT(!"Input string too large");
    return AK_InvalidParameter;
  }

#ifdef AK_WIN
  // MP3 files using the MP3 sample code, usually being provided by the gamer will 
  // not be located in the game path, for these sounds, we are using the Full path
  // to access them.
  if (in_pFlags != NULL &&
    in_pFlags->uCodecID == AKSOURCEID_MP3 &&
    in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC)
  {
    out_pszFullFilePath[0] = 0;
  }
  else
#endif
  {
    AKPLATFORM::SafeStrCpy(out_pszFullFilePath, m_szBasePath, AK_MAX_PATH);
  }

  if (in_pFlags
    && in_eOpenMode == AK_OpenModeRead)
  {
    // Add bank path if file is an AK sound bank.
    if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC &&
      in_pFlags->uCodecID == AKCODECID_BANK)
    {
      uiPathSize += AKPLATFORM::OsStrLen(m_szBankPath);
      if (uiPathSize >= AK_MAX_PATH)
      {
        AKASSERT(!"Path is too large");
        return AK_Fail;
      }
      AKPLATFORM::SafeStrCat(out_pszFullFilePath, m_szBankPath, AK_MAX_PATH);
    }

    // Note: Standard streaming files do not use this overload. On the other hand, streaming external 
    // sources use it if you use AkExternalSourceInfo::szFile instead of AkExternalSourceInfo::idFile.		

    // Externally supplied source (see External Sources in SDK doc)
    // In this sample, we will assume that the external source file name in_pszFileName
    // must be used as is (e.g. "myExternalSourceFile.wem").  If you use the External Source feature
    // you should modify this section to handle your FileIDs properly.
    /*if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL)
    {

    }*/

    // Add language directory name if needed.
    if (in_pFlags->bIsLanguageSpecific)
    {
      size_t uLanguageStrLen = AKPLATFORM::OsStrLen(AK::StreamMgr::GetCurrentLanguage());
      if (uLanguageStrLen > 0)
      {
        uiPathSize += (uLanguageStrLen + 1);
        if (uiPathSize >= AK_MAX_PATH)
        {
          AKASSERT(!"Path is too large");
          return AK_Fail;
        }
        AKPLATFORM::SafeStrCat(out_pszFullFilePath, AK::StreamMgr::GetCurrentLanguage(), AK_MAX_PATH);
        AKPLATFORM::SafeStrCat(out_pszFullFilePath, AK_PATH_SEPARATOR, AK_MAX_PATH);
      }
    }
  }

  // Append file title.
  uiPathSize += AKPLATFORM::OsStrLen(out_pszFullFilePath);
  if (uiPathSize >= AK_MAX_PATH)
  {
    AKASSERT(!"File name string too large");
    return AK_Fail;
  }
  AKPLATFORM::SafeStrCat(out_pszFullFilePath, in_pszFileName, AK_MAX_PATH);
  return AK_Success;
}

// ID overload. 
// The name of the file will be formatted as ID.ext. This is meant to be used with options
// "Use SoundBank Names" unchecked, and/or "Copy Streamed Files" in the SoundBank Settings.
// For more details, refer to the SoundBank Settings in Wwise Help, and to section "Identifying Banks" inside
// "Sound Engine Integration Walkthrough > Integrate Wwise Elements into Your Game > Integrating Banks > 
// Integration Details - Banks > General Information" of the SDK documentation.
// Returns AK_Success if input flags are supported and the resulting path is not too long.
// Returns AK_Fail otherwise.
AKRESULT CAkFileLocationBase::GetFullFilePath(
  AkFileID			in_fileID,			// File ID.
  AkFileSystemFlags* in_pFlags,			// Special flags. 
  AkOpenMode			/* in_eOpenMode*/,	// File open mode (read, write, ...).
  AkOSChar* out_pszFullFilePath	// Full file path.
)
{
  // If the file descriptor could not be found, or if the script-based FS does not exist,
  // map file ID to file descriptor (string based) for Audiokinetic IDs.

  if (!in_pFlags ||
    !(in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC || in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL))
  {
    AKASSERT(!"Unhandled file type");
    return AK_Fail;
  }

  // Compute file name with file system paths.
  size_t uiPathSize = AKPLATFORM::OsStrLen(m_szBasePath);

  // Copy base path. 
  AKPLATFORM::SafeStrCpy(out_pszFullFilePath, m_szBasePath, AK_MAX_PATH);
  // Concatenate path for AK banks or streamed audio files (everything except banks).
  if (in_pFlags->uCodecID == AKCODECID_BANK)
  {
    uiPathSize += AKPLATFORM::OsStrLen(m_szBankPath);
    if (uiPathSize >= AK_MAX_PATH)
    {
      AKASSERT(!"Path is too large");
      return AK_Fail;
    }
    AKPLATFORM::SafeStrCat(out_pszFullFilePath, m_szBankPath, AK_MAX_PATH);
  }
  else
  {
    uiPathSize += AKPLATFORM::OsStrLen(m_szAudioSrcPath);
    if (uiPathSize >= AK_MAX_PATH)
    {
      AKASSERT(!"Path is too large");
      return AK_Fail;
    }
    AKPLATFORM::SafeStrCat(out_pszFullFilePath, m_szAudioSrcPath, AK_MAX_PATH);
  }

  // Externally supplied source (see External Sources in SDK doc)
  // In this sample, we will assume that the file to load when receiving an external FileID is 
  // simply the FileID.wem (e.g. "12345.wem").  If you use the External Source feature
  // you should modify this section to handle your FileIDs properly.
  /*if (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL)
  {

  }*/

  // Add language directory name if needed.
  if (in_pFlags->bIsLanguageSpecific)
  {
    size_t uLanguageStrLen = AKPLATFORM::OsStrLen(AK::StreamMgr::GetCurrentLanguage());
    if (uLanguageStrLen > 0)
    {
      uiPathSize += (uLanguageStrLen + 1);
      if (uiPathSize >= AK_MAX_PATH)
      {
        AKASSERT(!"Path is too large");
        return AK_Fail;
      }
      AKPLATFORM::SafeStrCat(out_pszFullFilePath, AK::StreamMgr::GetCurrentLanguage(), AK_MAX_PATH);
      AKPLATFORM::SafeStrCat(out_pszFullFilePath, AK_PATH_SEPARATOR, AK_MAX_PATH);
    }
  }

  // Append file title.
  if ((uiPathSize + MAX_FILETITLE_SIZE) <= AK_MAX_PATH)
  {
    AkOSChar* pszTitle = out_pszFullFilePath + uiPathSize;
    if (in_pFlags->uCodecID == AKCODECID_BANK)
      AK_OSPRINTF(pszTitle, MAX_FILETITLE_SIZE, ID_TO_STRING_FORMAT_BANK, (unsigned int)in_fileID);
    else
      AK_OSPRINTF(pszTitle, MAX_FILETITLE_SIZE, ID_TO_STRING_FORMAT_WEM, (unsigned int)in_fileID);
  }
  else
  {
    AKASSERT(!"String buffer too small");
    return AK_Fail;
  }

  return AK_Success;
}

AKRESULT CAkFileLocationBase::SetBasePath(
  const AkOSChar* in_pszBasePath
)
{
  if (AKPLATFORM::OsStrLen(in_pszBasePath) + AkTemplMax(AKPLATFORM::OsStrLen(m_szBankPath), AKPLATFORM::OsStrLen(m_szAudioSrcPath)) + AKPLATFORM::OsStrLen(AK::StreamMgr::GetCurrentLanguage()) + 1 >= AK_MAX_PATH)
  {
    return AK_InvalidParameter;
  }
  AKPLATFORM::SafeStrCpy(m_szBasePath, in_pszBasePath, AK_MAX_PATH);
  return AK_Success;
}

AKRESULT CAkFileLocationBase::SetBankPath(
  const AkOSChar* in_pszBankPath
)
{
  if (AKPLATFORM::OsStrLen(m_szBasePath) + AkTemplMax(AKPLATFORM::OsStrLen(in_pszBankPath), AKPLATFORM::OsStrLen(m_szAudioSrcPath)) + AKPLATFORM::OsStrLen(AK::StreamMgr::GetCurrentLanguage()) + 1 >= AK_MAX_PATH)
  {
    return AK_InvalidParameter;
  }
  AKPLATFORM::SafeStrCpy(m_szBankPath, in_pszBankPath, AK_MAX_PATH);
  return AK_Success;
}

AKRESULT CAkFileLocationBase::SetAudioSrcPath(
  const AkOSChar* in_pszAudioSrcPath
)
{
  if (AKPLATFORM::OsStrLen(m_szBasePath) + AkTemplMax(AKPLATFORM::OsStrLen(m_szBankPath), AKPLATFORM::OsStrLen(in_pszAudioSrcPath)) + AKPLATFORM::OsStrLen(AK::StreamMgr::GetCurrentLanguage()) + 1 >= AK_MAX_PATH)
  {
    return AK_InvalidParameter;
  }
  AKPLATFORM::SafeStrCpy(m_szAudioSrcPath, in_pszAudioSrcPath, AK_MAX_PATH);
  return AK_Success;
}


//////////////////////////////////////////////////////////////////////
// 
// AkFilePackage.h
//
// This class represents a file package that was created with the 
// AkFilePackager utility app (located in ($WWISESDK)/samples/FilePackager/). 
// It holds a system file handle and a look-up table (CAkFilePackageLUT).
//
// CAkFilePackage objects can be chained together using the ListFilePackages
// typedef defined below.
// 
// Copyright (c) 2007-2009 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// Destroy file package and free memory / destroy pool.
void CAkFilePackage::Destroy()
{
  // Cache memory pointer and pool ID because memory pool is destroyed _after_ deleting this.
  AkMemPoolId	poolID = m_poolID;
  void* pToRelease = m_pToRelease;
  bool bIsInternalPool = m_bIsInternalPool;

  // Call destructor.
  this->~CAkFilePackage();

  // Free memory.
  ClearMemory(poolID, pToRelease, bIsInternalPool);
}

void CAkFilePackage::ClearMemory(
  AkMemPoolId in_poolID,			// Pool to destroy.
  void* in_pMemToRelease,	// Memory block to free before destroying pool.
  bool		in_bIsInternalPool	// Pool was created internally (and needs to be destroyed).
)
{
  if (in_poolID != AK_INVALID_POOL_ID)
  {
    if (in_pMemToRelease)
    {
      if (in_bIsInternalPool)
      {
        AK::MemoryMgr::ReleaseBlock(in_poolID, in_pMemToRelease);
        // Destroy pool
        AKVERIFY(AK::MemoryMgr::DestroyPool(in_poolID) == AK_Success);
      }
      else
      {
        if (AK::MemoryMgr::GetPoolAttributes(in_poolID) & AkBlockMgmtMask)
          AK::MemoryMgr::ReleaseBlock(in_poolID, in_pMemToRelease);
        else
          AkFree(in_poolID, in_pMemToRelease);
      }
    }
    else
      AKASSERT(!in_bIsInternalPool);	// Internal pools allocation is guaranteed.
  }
}





#define WIN32_BLOCKING_DEVICE_NAME		(AKTEXT("Win32 Blocking"))	// Default blocking device name.

CAkDefaultIOHookBlocking::CAkDefaultIOHookBlocking()
  : m_deviceID(AK_INVALID_DEVICE_ID)
  , m_bAsyncOpen(false)
{
}

CAkDefaultIOHookBlocking::~CAkDefaultIOHookBlocking()
{
}

// Initialization/termination. Init() registers this object as the one and 
// only File Location Resolver if none were registered before. Then 
// it creates a streaming device with scheduler type AK_SCHEDULER_BLOCKING.
AKRESULT CAkDefaultIOHookBlocking::Init(
  const AkDeviceSettings& in_deviceSettings,		// Device settings.
  bool						in_bAsyncOpen/*=false*/	// If true, files are opened asynchronously when possible.
)
{
  if (in_deviceSettings.uSchedulerTypeFlags != AK_SCHEDULER_BLOCKING)
  {
    AKASSERT(!"CAkDefaultIOHookBlocking I/O hook only works with AK_SCHEDULER_BLOCKING devices");
    return AK_Fail;
  }

  m_bAsyncOpen = in_bAsyncOpen;

  // If the Stream Manager's File Location Resolver was not set yet, set this object as the 
  // File Location Resolver (this I/O hook is also able to resolve file location).
  if (!AK::StreamMgr::GetFileLocationResolver())
    AK::StreamMgr::SetFileLocationResolver(this);

  // Create a device in the Stream Manager, specifying this as the hook.
  m_deviceID = AK::StreamMgr::CreateDevice(in_deviceSettings, this);
  if (m_deviceID != AK_INVALID_DEVICE_ID)
    return AK_Success;

  return AK_Fail;
}

void CAkDefaultIOHookBlocking::Term()
{
  if (AK::StreamMgr::GetFileLocationResolver() == this)
    AK::StreamMgr::SetFileLocationResolver(NULL);
  AK::StreamMgr::DestroyDevice(m_deviceID);
}

//
// IAkFileLocationAware interface.
//-----------------------------------------------------------------------------

// Returns a file descriptor for a given file name (string).
AKRESULT CAkDefaultIOHookBlocking::Open(
  const AkOSChar* in_pszFileName,     // File name.
  AkOpenMode      in_eOpenMode,       // Open mode.
  AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
  bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
  AkFileDesc& out_fileDesc        // Returned file descriptor.
)
{
  // We normally consider that calls to ::CreateFile() on a hard drive are fast enough to execute in the
  // client thread. If you want files to be opened asynchronously when it is possible, this device should 
  // be initialized with the flag in_bAsyncOpen set to true.
  if (io_bSyncOpen || !m_bAsyncOpen)
  {
    io_bSyncOpen = true;

    // Get the full file path, using path concatenation logic.
    AkOSChar szFullFilePath[AK_MAX_PATH];
    if (GetFullFilePath(in_pszFileName, in_pFlags, in_eOpenMode, szFullFilePath) == AK_Success)
    {
      // Open the file without FILE_FLAG_OVERLAPPED and FILE_FLAG_NO_BUFFERING flags.
      AKRESULT eResult = CAkFileHelpers::OpenFile(
        szFullFilePath,
        in_eOpenMode,
        false,
        false,
        out_fileDesc.hFile);
      if (eResult == AK_Success)
      {
#ifdef AK_USE_METRO_API
        FILE_STANDARD_INFO info;
        ::GetFileInformationByHandleEx(out_fileDesc.hFile, FileStandardInfo, &info, sizeof(info));
        out_fileDesc.iFileSize = info.EndOfFile.QuadPart;
#else
        ULARGE_INTEGER Temp;
        Temp.LowPart = ::GetFileSize(out_fileDesc.hFile, (LPDWORD)&Temp.HighPart);
        out_fileDesc.iFileSize = Temp.QuadPart;
#endif
        out_fileDesc.uSector = 0;
        out_fileDesc.deviceID = m_deviceID;
        out_fileDesc.pCustomParam = NULL;
        out_fileDesc.uCustomParamSize = 0;
      }
      return eResult;
    }

    return AK_Fail;
  }
  else
  {
    // The client allows us to perform asynchronous opening.
    // We only need to specify the deviceID, and leave the boolean to false.
    out_fileDesc.iFileSize = 0;
    out_fileDesc.uSector = 0;
    out_fileDesc.deviceID = m_deviceID;
    out_fileDesc.pCustomParam = NULL;
    out_fileDesc.uCustomParamSize = 0;
    return AK_Success;
  }
}

// Returns a file descriptor for a given file ID.
AKRESULT CAkDefaultIOHookBlocking::Open(
  AkFileID        in_fileID,          // File ID.
  AkOpenMode      in_eOpenMode,       // Open mode.
  AkFileSystemFlags* in_pFlags,      // Special flags. Can pass NULL.
  bool& io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
  AkFileDesc& out_fileDesc        // Returned file descriptor.
)
{
  // We normally consider that calls to ::CreateFile() on a hard drive are fast enough to execute in the
  // client thread. If you want files to be opened asynchronously when it is possible, this device should 
  // be initialized with the flag in_bAsyncOpen set to true.
  if (io_bSyncOpen || !m_bAsyncOpen)
  {
    io_bSyncOpen = true;

    // Get the full file path, using path concatenation logic.
    AkOSChar szFullFilePath[AK_MAX_PATH];
    if (GetFullFilePath(in_fileID, in_pFlags, in_eOpenMode, szFullFilePath) == AK_Success)
    {
      // Open the file without FILE_FLAG_OVERLAPPED and FILE_FLAG_NO_BUFFERING flags.
      AKRESULT eResult = CAkFileHelpers::OpenFile(
        szFullFilePath,
        in_eOpenMode,
        false,
        false,
        out_fileDesc.hFile);
      if (eResult == AK_Success)
      {
#ifdef AK_USE_METRO_API
        FILE_STANDARD_INFO info;
        ::GetFileInformationByHandleEx(out_fileDesc.hFile, FileStandardInfo, &info, sizeof(info));
        out_fileDesc.iFileSize = info.EndOfFile.QuadPart;
#else
        ULARGE_INTEGER Temp;
        Temp.LowPart = ::GetFileSize(out_fileDesc.hFile, (LPDWORD)&Temp.HighPart);
        out_fileDesc.iFileSize = Temp.QuadPart;
#endif
        out_fileDesc.uSector = 0;
        out_fileDesc.deviceID = m_deviceID;
        out_fileDesc.pCustomParam = NULL;
        out_fileDesc.uCustomParamSize = 0;
      }
      return eResult;
    }

    return AK_Fail;
  }
  else
  {
    // The client allows us to perform asynchronous opening.
    // We only need to specify the deviceID, and leave the boolean to false.
    out_fileDesc.iFileSize = 0;
    out_fileDesc.uSector = 0;
    out_fileDesc.deviceID = m_deviceID;
    out_fileDesc.pCustomParam = NULL;
    out_fileDesc.uCustomParamSize = 0;
    return AK_Success;
  }
}

//
// IAkIOHookBlocking implementation.
//-----------------------------------------------------------------------------

// Reads data from a file (synchronous). 
AKRESULT CAkDefaultIOHookBlocking::Read(
  AkFileDesc& in_fileDesc,        // File descriptor.
  const AkIoHeuristics& /*in_heuristics*/,	// Heuristics for this data transfer (not used in this implementation).
  void* out_pBuffer,        // Buffer to be filled with data.
  AkIOTransferInfo& io_transferInfo		// Synchronous data transfer info. 
)
{
  AKASSERT(out_pBuffer &&
    in_fileDesc.hFile != INVALID_HANDLE_VALUE);

  OVERLAPPED overlapped;
  overlapped.Offset = (DWORD)(io_transferInfo.uFilePosition & 0xFFFFFFFF);
  overlapped.OffsetHigh = (DWORD)((io_transferInfo.uFilePosition >> 32) & 0xFFFFFFFF);
  overlapped.hEvent = NULL;

  AkUInt32 uSizeTransferred;

  if (::ReadFile(
    in_fileDesc.hFile,
    out_pBuffer,
    io_transferInfo.uRequestedSize,
    &uSizeTransferred,
    &overlapped))
  {
    AKASSERT(uSizeTransferred == io_transferInfo.uRequestedSize);
    return AK_Success;
  }
  return AK_Fail;
}

// Writes data to a file (synchronous). 
AKRESULT CAkDefaultIOHookBlocking::Write(
  AkFileDesc& in_fileDesc,        // File descriptor.
  const AkIoHeuristics& /*in_heuristics*/,	// Heuristics for this data transfer (not used in this implementation).
  void* in_pData,           // Data to be written.
  AkIOTransferInfo& io_transferInfo		// Synchronous data transfer info. 
)
{
  AKASSERT(in_pData &&
    in_fileDesc.hFile != INVALID_HANDLE_VALUE);

  OVERLAPPED overlapped;
  overlapped.Offset = (DWORD)(io_transferInfo.uFilePosition & 0xFFFFFFFF);
  overlapped.OffsetHigh = (DWORD)((io_transferInfo.uFilePosition >> 32) & 0xFFFFFFFF);
  overlapped.hEvent = NULL;

  AkUInt32 uSizeTransferred;

  if (::WriteFile(
    in_fileDesc.hFile,
    in_pData,
    io_transferInfo.uRequestedSize,
    &uSizeTransferred,
    &overlapped))
  {
    AKASSERT(uSizeTransferred == io_transferInfo.uRequestedSize);
    return AK_Success;
  }
  return AK_Fail;
}

// Cleans up a file.
AKRESULT CAkDefaultIOHookBlocking::Close(
  AkFileDesc& in_fileDesc      // File descriptor.
)
{
  return CAkFileHelpers::CloseFile(in_fileDesc.hFile);
}

// Returns the block size for the file or its storage device. 
AkUInt32 CAkDefaultIOHookBlocking::GetBlockSize(
  AkFileDesc&  /*in_fileDesc*/     // File descriptor.
)
{
  // No constraint on block size (file seeking).
  return 1;
}


// Returns a description for the streaming device above this low-level hook.
void CAkDefaultIOHookBlocking::GetDeviceDesc(
  AkDeviceDesc&
#ifndef AK_OPTIMIZED
  out_deviceDesc      // Description of associated low-level I/O device.
#endif
)
{
#ifndef AK_OPTIMIZED
  AKASSERT(m_deviceID != AK_INVALID_DEVICE_ID || !"Low-Level device was not initialized");
  out_deviceDesc.deviceID = m_deviceID;
  out_deviceDesc.bCanRead = true;
  out_deviceDesc.bCanWrite = true;
  AKPLATFORM::SafeStrCpy(out_deviceDesc.szDeviceName, WIN32_BLOCKING_DEVICE_NAME, AK_MONITOR_DEVICENAME_MAXLENGTH);
  out_deviceDesc.uStringSize = (AkUInt32)wcslen(out_deviceDesc.szDeviceName) + 1;
#endif
}

// Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
AkUInt32 CAkDefaultIOHookBlocking::GetDeviceData()
{
  return (m_bAsyncOpen) ? 1 : 0;
}



/////////////////////////////////////////////////////////////////////////////////
//                              MEMORY HOOKS SETUP
//
//                             ##### IMPORTANT #####
//
// These custom alloc/free functions are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
/////////////////////////////////////////////////////////////////////////////////

namespace AK
{
  void* AllocHook(size_t in_size)
  {
    return malloc(in_size);
  }
  void FreeHook(void* in_ptr)
  {
    free(in_ptr);
  }
  void* VirtualAllocHook(
    void* in_pMemAddress,
    size_t in_size,
    DWORD in_dwAllocationType,
    DWORD in_dwProtect
  )
  {
    return VirtualAlloc(in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect);
  }
  void VirtualFreeHook(
    void* in_pMemAddress,
    size_t in_size,
    DWORD in_dwFreeType
  )
  {
    VirtualFree(in_pMemAddress, in_size, in_dwFreeType);
  }
}


/////////////////////////
//  MAIN
/////////////////////////


static void AkAssertHookA(const char* in_pszExpression, const char* in_pszFileName, int in_lineNumber)
{
  abort();
}

static void MusicCallback(AkCallbackType in_eType, AkCallbackInfo* in_pCallbackInfo)
{
  if (in_eType == AK_MusicSyncBar)
  {
  }
  else if (in_eType == AK_MusicSyncBeat)
  {
  }
  else if (in_eType == AK_EndOfEvent)
  {
  }
}


static AK::IAkPlugin* createPluginCallback(AK::IAkPluginMemAlloc* in_pAllocator)
{
  return nullptr;
}
static AK::IAkPluginParam* createParamCallback(AK::IAkPluginMemAlloc* in_pAllocator)
{
  return nullptr;
}

static void loadAllBnkFiles()
{
  AkBankID bankID;

  AK::SoundEngine::LoadBank("DoorInit.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("Door.bnk", AK_DEFAULT_POOL_ID, bankID);

  AK::SoundEngine::LoadBank("amp_at120.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_at20.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_bt15.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_bt30.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_bt45.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_ca100.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_ca38.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_ca85.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_cs100.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_cs120.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_cs90.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_en30.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_en50.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_epiphoneelectarcentury.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_epiphoneelectarm.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_epiphoneelectrichawaiian.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_epiphonezephyr.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gb100.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gb38.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gb50.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gibsonga79.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gibsonga8.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_gibsonga88.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_hg100.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_hg180.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_hg500.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshall1962bluesbreaker.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshalldsl100h.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshalldsl15h.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshalljcm800.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshalljtm45.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshalljvm410h.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_marshallplexi.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangead50.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangejimmybean.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangeor100.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangeor50h.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangerockerverb.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_orangetinyterror.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_tw22.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_tw26.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("amp_tw40.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_bt600b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_bt880b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_bt975b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ch300b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ch350b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ch600b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_cs240b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_cs300b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_cs75b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_edene300.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_edenwt550.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_edenwt800.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ht100b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ht300b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_ht400b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_lt25b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_lt85b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_amp_orangead200b.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_at1150bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_at810bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_bt115bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_bt212bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_bt410bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_ca1510bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_ch210bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_ch310bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_ch410bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_cs112bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_cs15bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_cs410bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_edend115xlt.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_edend212xlt.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_edend410xst.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_edend610xst.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_gb415bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_orangeobc115.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_orangeobc810.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_cab_tw215bc.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassautofilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_basschorus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassdistortion.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassenbig.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_basseq8.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassfilterdelay.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassfilterecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassfuzz.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassoverdrive.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_bassphase.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_basssuboctave.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_basswah.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_edenwtdi.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("bass_pedal_mbcomp.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("boot.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_at0112c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_at1121c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_audiophile.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_boombox.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_bt1120c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_bt1121c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_bt410c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_ca112c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_ca215c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_ca412c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_cabinetradio.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_cs1120c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_cs1515c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_cs212c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_en212c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_en4120c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_gb412cmki.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_gb412cmkii.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_gb412cmkiii.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_gramophone.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_hg2120c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_hg212c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_hg215c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_jukebox.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_marshall1936.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_marshall1960a.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_marshall1960ax.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_marshall1960tv.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_orangejimmybean.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_orangeppc212ob.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_orangeppc412.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_pa1152c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_pa600c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_pa999c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_tw110c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_tw112c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_tw410c.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("cab_vintagehifi.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("core.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("crowd_venuesize_large.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("crowd_venuesize_medium.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("crowd_venuesize_small.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("di_amp_bassdriver.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("di_amp_mixerpre.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("di_amp_tubepre.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("init.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_80sflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_acousticemulator.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_ampeq.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_amptrem.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_ampvibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_analoguedelay.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_autofilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_autovibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_bakedrotatoe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_bassemulator.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_bitcruncher.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_bobfilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_buzzone.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_buzztoo.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_captfuzzle.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_chorus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_chorus20.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_classicflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_compression.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_cosmicecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_customdrive.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_digitalchorus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_digitalverb.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_distortion.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_enbiggenator.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_eq5.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_eq8.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_fuzzwashe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_germaniumdrive.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_limiter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_linedrive.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_lofifilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_marshallguvnorplus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_marshallsupervibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_metaldistortion.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_moddelay.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_modernflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_modernwah.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_multipitch.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_multitrem.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_multivibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_nofiecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_noisegate.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_npndelay.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_octaveup.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_octavius.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_oilcanecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_omnimod.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_phaser.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_planephase.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_plateverb.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_rangebooster.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_ringmod.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_sendintheclones.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_shaverphaser.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_shredzone.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_springreverb.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_superdrive.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_swole.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_tremole.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_tremolo.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_tubespring.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_ukwah.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_uswah.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_valveecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_vintagechorus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_vintagedistortion.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("pedal_vintageflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_rotavibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_stereoanalogvibe.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_stereophaser.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_stereotubetrem.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiochamber.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiochorus.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiocompressor.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiodelay.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studioeq.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studioflanger.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiographiceq.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiopitch.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studioplate.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studioverb.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_studiowahfilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_synthfilter.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("rack_tapeecho.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("tonedesigner.bnk", AK_DEFAULT_POOL_ID, bankID);
  AK::SoundEngine::LoadBank("videobank.bnk", AK_DEFAULT_POOL_ID, bankID);
}

int main()
{
  AkStreamMgrSettings stmSettings;
  AK::StreamMgr::GetDefaultSettings(stmSettings);

  AkDeviceSettings deviceSettings;
  AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);

  AkInitSettings initSettings;
  AK::SoundEngine::GetDefaultInitSettings(initSettings);
  initSettings.uDefaultPoolSize = DEMO_DEFAULT_POOL_SIZE;
  initSettings.pfnAssertHook = AkAssertHookA;

  AkMusicSettings initMusicSettings;
  initMusicSettings.fStreamingLookAheadRatio = 1.0f;
  AK::MusicEngine::GetDefaultInitSettings(initMusicSettings);

  AkPlatformInitSettings platformInitSettings;
  AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
  platformInitSettings.uLEngineDefaultPoolSize = DEMO_LENGINE_DEFAULT_POOL_SIZE;

  if (AK::MemoryMgr::IsInitialized())
    abort();

  AkMemSettings memSettings;
  memSettings.uMaxNumPools = 20;
  AKRESULT res = AK::MemoryMgr::Init(&memSettings);
  if (res != AK_Success)
    abort();

  if (!AK::MemoryMgr::IsInitialized())
    abort();

  if (!AK::StreamMgr::Create(stmSettings))
    abort();

  CAkFilePackageLowLevelIOBlocking m_pLowLevelIO;
  res = m_pLowLevelIO.Init(deviceSettings);
  if (res != AK_Success)
    abort();

  res = AK::SoundEngine::Init(&initSettings, &platformInitSettings);
  if (res != AK_Success)
    abort();

  res = AK::MusicEngine::Init(&initMusicSettings);
  if (res != AK_Success)
    abort();

  res = AK::SoundEngine::RegisterAllPlugins();
  if (res != AK_Success)
    abort();

  m_pLowLevelIO.SetBasePath(SOUND_BANK_PATH);

  if (AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)")) != AK_Success)
    abort();

  AkBankID bankID;

  if (AK::SoundEngine::LoadBank("init.bnk", AK_DEFAULT_POOL_ID, bankID) != AK_Success)
    abort();

  if (!SoundInputMgr::Instance().Initialize())
    abort();
#define GAME_OBJECT 1234
  if (AK::SoundEngine::RegisterGameObj(GAME_OBJECT, "GAME_OBJECT") != AK_Success)
    abort();

  loadAllBnkFiles();

  {
    AK::SoundEngine::PostEvent("Play_TitleScreen", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Dialog_TransitionIn", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Dialog_TransitionOut", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Play_RocksmithStart_NewVO", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Enter_Bib", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Nav_Down", GAME_OBJECT);
  }

  AK::SoundEngine::PostEvent("openTheDoor_justDoor", GAME_OBJECT);

  for (;;) {
    if (AK::SoundEngine::IsInitialized())
      AK::SoundEngine::RenderAudio();
  }

  return 0;
}