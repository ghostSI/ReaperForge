// main.cpp
// Copyright (C) 2010 Audiokinetic Inc
/// \file 
/// Contains the entry point for the application.

/////////////////////////
//  INCLUDES
/////////////////////////


#define DEMO_DEFAULT_POOL_SIZE 400ULL*1024*1024
#define DEMO_LENGINE_DEFAULT_POOL_SIZE 400ULL*1024*1024
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

  //AK::SoundEngine::LoadBank("DoorInit.bnk", AK_DEFAULT_POOL_ID, bankID);
  //AK::SoundEngine::LoadBank("Door.bnk", AK_DEFAULT_POOL_ID, bankID);

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

static std::vector<unsigned int> init_rtpc =
{
3484274UL,
5205459UL,
6423288UL,
7376998UL,
8951515UL,
14247475UL,
17194343UL,
18180713UL,
20203794UL,
21468188UL,
27313854UL,
35592391UL,
36606540UL,
38836821UL,
38836822UL,
43306129UL,
43966678UL,
45477574UL,
47790742UL,
49101491UL,
52503222UL,
55289225UL,
58860418UL,
62191340UL,
62628515UL,
62758263UL,
68342841UL,
73042095UL,
74326962UL,
75679700UL,
78338109UL,
79140768UL,
79733756UL,
80677073UL,
81934675UL,
82056199UL,
82541283UL,
90231495UL,
93151782UL,
94806316UL,
95953315UL,
99581292UL,
102718170UL,
103283384UL,
103381181UL,
104332312UL,
109289464UL,
114290557UL,
116745936UL,
116866841UL,
118008747UL,
118300554UL,
119368049UL,
128858254UL,
129923793UL,
133051341UL,
133201238UL,
137329742UL,
152236940UL,
153006184UL,
153559593UL,
154755801UL,
155571457UL,
157149294UL,
160379990UL,
160737214UL,
166184252UL,
167188772UL,
167653415UL,
169627249UL,
182575977UL,
185254347UL,
190258981UL,
193364999UL,
193717650UL,
196508852UL,
199054804UL,
200104187UL,
202722404UL,
202889079UL,
205218678UL,
205290587UL,
207270705UL,
207733941UL,
210438092UL,
212034260UL,
212562701UL,
212969625UL,
220603429UL,
222108924UL,
224970440UL,
226290440UL,
226753571UL,
232303844UL,
233155866UL,
234067079UL,
234571050UL,
237406553UL,
237577199UL,
238106111UL,
238837112UL,
239819088UL,
239950674UL,
243368495UL,
247095865UL,
248559592UL,
250987342UL,
250991261UL,
254114520UL,
254670027UL,
254913495UL,
254974812UL,
255268488UL,
255455549UL,
261403729UL,
264766445UL,
266726275UL,
266747165UL,
267298320UL,
267394936UL,
268498298UL,
271132867UL,
271633888UL,
272140610UL,
272646552UL,
272944453UL,
273489416UL,
273583412UL,
274665146UL,
274973469UL,
278486660UL,
284321262UL,
285876925UL,
291852530UL,
292704195UL,
296875683UL,
297665368UL,
298650422UL,
299248380UL,
304964514UL,
307305719UL,
307532787UL,
307964705UL,
308152919UL,
310323091UL,
312790456UL,
314343421UL,
317612390UL,
318823734UL,
320397298UL,
322006184UL,
328215681UL,
328640020UL,
331597428UL,
346062600UL,
347134790UL,
348545887UL,
359321995UL,
361328725UL,
362608117UL,
364598297UL,
368634899UL,
370692516UL,
370799579UL,
372095136UL,
372337218UL,
381442090UL,
383806850UL,
383997207UL,
388206364UL,
400092499UL,
400656358UL,
401293277UL,
402617697UL,
404483416UL,
411276710UL,
414238161UL,
417216722UL,
420360138UL,
420429564UL,
422575470UL,
424693580UL,
424899595UL,
427070867UL,
431342190UL,
431357069UL,
434173290UL,
434646548UL,
438720324UL,
443755030UL,
444530400UL,
446392702UL,
447663682UL,
450937841UL,
462855528UL,
463137750UL,
464882577UL,
468399761UL,
472176242UL,
473241435UL,
473442999UL,
473484722UL,
478083332UL,
487263779UL,
487925696UL,
488167903UL,
489111825UL,
492378325UL,
494532720UL,
494604836UL,
495335072UL,
502360511UL,
502743421UL,
503593818UL,
503809133UL,
507250203UL,
507732380UL,
512433537UL,
513882271UL,
515972554UL,
516878344UL,
519212481UL,
520127569UL,
521123450UL,
521124233UL,
522680904UL,
524441498UL,
530049869UL,
530769464UL,
533979366UL,
536705904UL,
538309955UL,
538811794UL,
542763715UL,
543900758UL,
544661226UL,
549734879UL,
551397418UL,
553829463UL,
554424063UL,
554932592UL,
555601262UL,
555886196UL,
562676150UL,
565349928UL,
567522134UL,
572163171UL,
572782444UL,
581530010UL,
582182788UL,
588848119UL,
589126135UL,
589406117UL,
594029185UL,
594479613UL,
595383395UL,
595855283UL,
598843433UL,
599202648UL,
599568367UL,
604302674UL,
605807305UL,
607968698UL,
608473840UL,
609470567UL,
610167237UL,
612740271UL,
614546713UL,
614832937UL,
617160644UL,
618744169UL,
618998215UL,
619335518UL,
620682058UL,
623410477UL,
626494531UL,
629097418UL,
634529240UL,
634905802UL,
636784036UL,
639256107UL,
639981501UL,
644704001UL,
651809471UL,
653270530UL,
655737159UL,
659451250UL,
662246982UL,
663396175UL,
663686223UL,
664284349UL,
667012747UL,
667733453UL,
669504653UL,
669994635UL,
674242249UL,
681963246UL,
688212277UL,
690024846UL,
691453930UL,
696631193UL,
698077794UL,
705960759UL,
711838141UL,
712382155UL,
713459137UL,
716633267UL,
723308545UL,
725491885UL,
727118352UL,
727637789UL,
729056483UL,
729334956UL,
730757797UL,
734889032UL,
736468798UL,
737758004UL,
743260409UL,
743335749UL,
746245647UL,
749705520UL,
751180696UL,
751720651UL,
752572745UL,
760171401UL,
762201973UL,
762681246UL,
762777615UL,
762868631UL,
763081829UL,
763928177UL,
766710537UL,
768386809UL,
772996677UL,
774909675UL,
776505693UL,
781862802UL,
782685743UL,
782912786UL,
783370183UL,
786328713UL,
786828784UL,
788877895UL,
793670852UL,
794137100UL,
799572209UL,
800723052UL,
802151045UL,
802374705UL,
805637814UL,
806785048UL,
807933265UL,
808645185UL,
811803647UL,
815184361UL,
823098980UL,
824666088UL,
825791460UL,
827002759UL,
829535033UL,
831295053UL,
831739438UL,
834312749UL,
846817762UL,
846937557UL,
850163212UL,
850979469UL,
851493385UL,
852063878UL,
853218435UL,
855844032UL,
857296019UL,
865542154UL,
870672129UL,
875351818UL,
877206018UL,
885190127UL,
886851708UL,
887907898UL,
888240026UL,
892507755UL,
895698032UL,
897671039UL,
898480243UL,
899044116UL,
904382244UL,
909579349UL,
910884441UL,
911817472UL,
912054247UL,
912200127UL,
914911479UL,
915452587UL,
918528143UL,
919760612UL,
920376032UL,
927063274UL,
929829444UL,
931017155UL,
932252924UL,
933611160UL,
935579359UL,
935887182UL,
936665304UL,
942522890UL,
943299653UL,
947535107UL,
948508018UL,
948701058UL,
948704733UL,
951400726UL,
953830361UL,
959695869UL,
959975961UL,
964723547UL,
965003271UL,
965630463UL,
966488619UL,
966496870UL,
969501231UL,
973020991UL,
973931528UL,
974737350UL,
975955650UL,
986801681UL,
988995448UL,
990138982UL,
990484080UL,
991071112UL,
993633557UL,
994698082UL,
994955269UL,
1003857334UL,
1004702071UL,
1006518138UL,
1012027270UL,
1012430498UL,
1015484348UL,
1015564463UL,
1021392082UL,
1025981706UL,
1027840700UL,
1028162590UL,
1031619552UL,
1032548501UL,
1036473100UL,
1039291656UL,
1041545650UL,
1042542021UL,
1043350084UL,
1043506096UL,
1048597520UL,
1048790181UL,
1050113495UL,
1053554062UL,
1054880722UL,
1059468674UL,
1061087025UL,
1063644010UL,
1065772599UL,
1067751824UL,
1068593328UL,
1076220369UL,
1076611338UL,
1079249785UL,
1081672925UL,
1091884991UL,
1092439895UL,
1093634991UL,
1093937226UL,
1097445659UL,
1099238462UL,
1102561613UL,
1102805919UL,
1104329803UL,
1105555488UL,
1106543786UL,
1108059278UL,
1108105366UL,
1109423977UL,
1111219968UL,
1111601390UL,
1111660617UL,
1112485078UL,
1114143038UL,
1115019996UL,
1115916954UL,
1120036061UL,
1123020715UL,
1123659049UL,
1124599490UL,
1127295313UL,
1128047852UL,
1128197571UL,
1129664662UL,
1130248499UL,
1130811845UL,
1131163621UL,
1134194709UL,
1141094592UL,
1142699949UL,
1146673605UL,
1149761609UL,
1151249540UL,
1157239097UL,
1158304044UL,
1159231017UL,
1162222544UL,
1164148009UL,
1165487481UL,
1168209372UL,
1178104669UL,
1180224274UL,
1183268560UL,
1185800221UL,
1185885207UL,
1188649753UL,
1189097760UL,
1194705823UL,
1194987332UL,
1198097623UL,
1200261372UL,
1201200675UL,
1202688579UL,
1203180794UL,
1203666727UL,
1208567230UL,
1209231991UL,
1210428560UL,
1218147606UL,
1224323471UL,
1227759428UL,
1229718757UL,
1231302222UL,
1236895320UL,
1238950972UL,
1239854468UL,
1253937933UL,
1255580968UL,
1256735438UL,
1256893681UL,
1266254659UL,
1269662948UL,
1279235283UL,
1280625485UL,
1281575175UL,
1284141570UL,
1294504501UL,
1295996070UL,
1298014945UL,
1301976759UL,
1304866153UL,
1306784941UL,
1311282697UL,
1313892095UL,
1315392664UL,
1319257813UL,
1319525927UL,
1321140277UL,
1323888317UL,
1323888318UL,
1325687280UL,
1326389799UL,
1329830955UL,
1330424172UL,
1330907323UL,
1331606754UL,
1334396440UL,
1335060026UL,
1335932847UL,
1338772434UL,
1350877829UL,
1359977170UL,
1361602734UL,
1363628743UL,
1364274198UL,
1365624920UL,
1366824842UL,
1372529946UL,
1375080374UL,
1375770644UL,
1379554122UL,
1380761585UL,
1388266035UL,
1390098500UL,
1390114008UL,
1394751688UL,
1395025135UL,
1401468995UL,
1401760619UL,
1404526342UL,
1413069362UL,
1415246708UL,
1417033562UL,
1417787719UL,
1417875334UL,
1424879375UL,
1425358290UL,
1431990489UL,
1432748873UL,
1432773380UL,
1437135315UL,
1437685950UL,
1441797622UL,
1443092704UL,
1444266639UL,
1446172764UL,
1446814362UL,
1452786595UL,
1455118025UL,
1455514332UL,
1455798980UL,
1457916023UL,
1458342649UL,
1460411373UL,
1461191168UL,
1462410968UL,
1464139971UL,
1464637105UL,
1466009822UL,
1466496531UL,
1468467623UL,
1473343627UL,
1475550280UL,
1475877144UL,
1477340332UL,
1482271461UL,
1484796522UL,
1492332646UL,
1494408559UL,
1495009922UL,
1500184607UL,
1500262364UL,
1500342617UL,
1501908165UL,
1506876165UL,
1508986866UL,
1509089318UL,
1509610451UL,
1511116785UL,
1512664389UL,
1512686619UL,
1513272192UL,
1517402117UL,
1521823950UL,
1522795093UL,
1523037525UL,
1525914488UL,
1528628040UL,
1529943327UL,
1530867353UL,
1533275751UL,
1533383491UL,
1535660027UL,
1537289754UL,
1542742393UL,
1544453168UL,
1545753749UL,
1546117312UL,
1546755879UL,
1548357966UL,
1548422812UL,
1551168185UL,
1554647516UL,
1556511182UL,
1556684576UL,
1557533553UL,
1558707950UL,
1558934008UL,
1559049634UL,
1561315593UL,
1567679741UL,
1569163791UL,
1572227515UL,
1574896383UL,
1578695502UL,
1585071954UL,
1585112111UL,
1585570176UL,
1587780800UL,
1590171938UL,
1594586421UL,
1594817409UL,
1598078131UL,
1603057607UL,
1607384651UL,
1611454155UL,
1619755892UL,
1625699835UL,
1629983818UL,
1635619681UL,
1637328094UL,
1638538020UL,
1641005847UL,
1641390889UL,
1641651616UL,
1641803270UL,
1644739213UL,
1646609596UL,
1647915329UL,
1659149703UL,
1659845586UL,
1660467970UL,
1660790025UL,
1663232926UL,
1664009691UL,
1665194724UL,
1666101643UL,
1667562634UL,
1669050674UL,
1669054307UL,
1671541560UL,
1676870479UL,
1679569200UL,
1679673831UL,
1679958449UL,
1684063083UL,
1686897072UL,
1692982090UL,
1695275110UL,
1702438051UL,
1704960355UL,
1729961972UL,
1734506988UL,
1736593302UL,
1737351645UL,
1739122818UL,
1746918792UL,
1748033986UL,
1755881460UL,
1756491483UL,
1765323135UL,
1777153479UL,
1777545957UL,
1778966434UL,
1785683566UL,
1792700717UL,
1793901478UL,
1794561032UL,
1797548037UL,
1798531360UL,
1804680536UL,
1807640239UL,
1808102267UL,
1811264704UL,
1811621965UL,
1816960362UL,
1817404072UL,
1822628340UL,
1825969072UL,
1828245625UL,
1828498476UL,
1835087893UL,
1837096676UL,
1843799673UL,
1844089860UL,
1846375797UL,
1850662180UL,
1850815482UL,
1851885607UL,
1856839795UL,
1859047061UL,
1860719044UL,
1864088704UL,
1865190603UL,
1865869996UL,
1868581621UL,
1870289504UL,
1878819984UL,
1879909121UL,
1883224128UL,
1883819830UL,
1886466227UL,
1886896260UL,
1887249003UL,
1895022692UL,
1896369455UL,
1898835876UL,
1898835879UL,
1899904433UL,
1901263556UL,
1902915303UL,
1905333325UL,
1907356400UL,
1908219416UL,
1911156370UL,
1911345750UL,
1913940847UL,
1913980812UL,
1914991626UL,
1916706871UL,
1916850154UL,
1922140386UL,
1927293754UL,
1927586664UL,
1927811180UL,
1929852337UL,
1937708410UL,
1939704371UL,
1947526489UL,
1950371680UL,
1951044096UL,
1951906047UL,
1958592221UL,
1960484997UL,
1975693509UL,
1977201850UL,
1980736042UL,
1985764260UL,
1989219750UL,
1990589009UL,
1992983835UL,
1993857690UL,
1996486973UL,
1996568854UL,
1998846839UL,
1999848593UL,
2002102891UL,
2008083072UL,
2008083075UL,
2008568968UL,
2011453540UL,
2018165600UL,
2026908455UL,
2028012547UL,
2029395179UL,
2030229327UL,
2032063743UL,
2035121853UL,
2035845296UL,
2036374531UL,
2037715364UL,
2038094497UL,
2043053989UL,
2048569118UL,
2048625045UL,
2051544608UL,
2052957785UL,
2053173708UL,
2053397880UL,
2054731054UL,
2056181210UL,
2058131381UL,
2060067919UL,
2063391619UL,
2064120177UL,
2064597667UL,
2064978726UL,
2066290617UL,
2066298376UL,
2066438972UL,
2066957487UL,
2068257940UL,
2069764388UL,
2077281031UL,
2078149838UL,
2089871325UL,
2089941427UL,
2095301642UL,
2099739690UL,
2102215638UL,
2102284467UL,
2104491571UL,
2104778493UL,
2104778494UL,
2104864369UL,
2105489793UL,
2105546042UL,
2107235076UL,
2118314874UL,
2121300716UL,
2127297127UL,
2129514209UL,
2130520713UL,
2130938184UL,
2132507524UL,
2134200747UL,
2134821049UL,
2138578979UL,
2138870814UL,
2139088950UL,
2139997353UL,
2142094165UL,
2146817983UL,
2147236512UL,
2151177666UL,
2158142509UL,
2158206865UL,
2161003586UL,
2161660485UL,
2162668492UL,
2167658747UL,
2170567761UL,
2172168093UL,
2172651221UL,
2173806707UL,
2177331507UL,
2177376620UL,
2179946799UL,
2182739907UL,
2185639322UL,
2187992846UL,
2192441602UL,
2194407485UL,
2194935341UL,
2205836085UL,
2205936370UL,
2208707988UL,
2214850332UL,
2215700931UL,
2216346804UL,
2216393970UL,
2216776264UL,
2223057360UL,
2223269238UL,
2223764034UL,
2226406005UL,
2235308406UL,
2238020307UL,
2239710535UL,
2241816287UL,
2242826633UL,
2243069495UL,
2252570799UL,
2254713813UL,
2257058725UL,
2257951835UL,
2258372329UL,
2260586384UL,
2263482316UL,
2264475672UL,
2265262272UL,
2268461215UL,
2270493950UL,
2270835285UL,
2271271209UL,
2272455812UL,
2273261451UL,
2280580771UL,
2281547611UL,
2282528183UL,
2282940243UL,
2284693731UL,
2288197533UL,
2291350023UL,
2293332473UL,
2293394461UL,
2293636359UL,
2293889411UL,
2294779980UL,
2298002273UL,
2299525551UL,
2300933251UL,
2304951980UL,
2305406967UL,
2308071439UL,
2314244570UL,
2315636921UL,
2315813106UL,
2319612508UL,
2320153963UL,
2321725317UL,
2323181215UL,
2325961246UL,
2330808575UL,
2332040423UL,
2334086363UL,
2334776832UL,
2340368670UL,
2341989593UL,
2342075217UL,
2343837818UL,
2344706474UL,
2345779361UL,
2346183791UL,
2349287669UL,
2352107727UL,
2352649248UL,
2353008739UL,
2353485104UL,
2356360099UL,
2357497462UL,
2358529219UL,
2358744415UL,
2364361080UL,
2370344009UL,
2371892787UL,
2374866037UL,
2375043192UL,
2380292404UL,
2386067096UL,
2389816463UL,
2391821046UL,
2392493420UL,
2393443342UL,
2393623286UL,
2393772614UL,
2397414532UL,
2397485123UL,
2397922996UL,
2398296965UL,
2400469174UL,
2408969723UL,
2409030233UL,
2412231888UL,
2414446920UL,
2414983063UL,
2420449658UL,
2420667393UL,
2422378030UL,
2428858924UL,
2430377040UL,
2430758309UL,
2432510226UL,
2433532208UL,
2441295179UL,
2442653967UL,
2451944302UL,
2451998811UL,
2454892344UL,
2455646737UL,
2456550655UL,
2458116344UL,
2458301995UL,
2464304396UL,
2464343736UL,
2465835437UL,
2466200568UL,
2471592839UL,
2472767748UL,
2477552205UL,
2482332983UL,
2483903730UL,
2487145464UL,
2488485540UL,
2488673007UL,
2490300116UL,
2490470901UL,
2490962110UL,
2495927348UL,
2499650228UL,
2510346182UL,
2514269172UL,
2514737378UL,
2519633400UL,
2522262508UL,
2524973125UL,
2528638468UL,
2528638490UL,
2529375242UL,
2529483969UL,
2533198032UL,
2536464950UL,
2537846798UL,
2540986148UL,
2541972287UL,
2546004780UL,
2547064280UL,
2547754506UL,
2548172056UL,
2548302812UL,
2549667617UL,
2554981950UL,
2560272295UL,
2568269530UL,
2569815388UL,
2582988125UL,
2585670420UL,
2587820873UL,
2588421717UL,
2590215635UL,
2591851042UL,
2595336850UL,
2596322534UL,
2603964412UL,
2604406539UL,
2608304013UL,
2614078791UL,
2614347315UL,
2618057294UL,
2622527342UL,
2625036312UL,
2626620329UL,
2627644768UL,
2628575865UL,
2631322941UL,
2635678217UL,
2636344712UL,
2639103975UL,
2639331766UL,
2639793303UL,
2644682316UL,
2650319704UL,
2654897657UL,
2660101685UL,
2661475068UL,
2662328617UL,
2663633707UL,
2666405982UL,
2674132656UL,
2674196909UL,
2675675577UL,
2683557405UL,
2685719560UL,
2685862743UL,
2688391606UL,
2689543723UL,
2694766405UL,
2696156906UL,
2701420256UL,
2702835062UL,
2703592150UL,
2704416600UL,
2709946475UL,
2710100163UL,
2710987175UL,
2714724332UL,
2719411738UL,
2721999860UL,
2724236939UL,
2725944673UL,
2727298849UL,
2727996231UL,
2728556384UL,
2744399322UL,
2745023664UL,
2746148137UL,
2748098071UL,
2757391178UL,
2757939845UL,
2761706158UL,
2763632772UL,
2764102469UL,
2765911376UL,
2768303693UL,
2769055664UL,
2770858338UL,
2772730095UL,
2773703988UL,
2776075392UL,
2777626814UL,
2778948834UL,
2779663775UL,
2780292158UL,
2780714691UL,
2782342843UL,
2784057694UL,
2786160423UL,
2792019714UL,
2792840127UL,
2795012673UL,
2796940095UL,
2797910534UL,
2803438817UL,
2804192455UL,
2806708647UL,
2813156533UL,
2816723163UL,
2818288697UL,
2819007498UL,
2819862605UL,
2820620907UL,
2823278594UL,
2825896954UL,
2829270772UL,
2829778997UL,
2833255051UL,
2833444668UL,
2833853545UL,
2836298693UL,
2841216982UL,
2841902424UL,
2842762598UL,
2847099765UL,
2847535852UL,
2847898387UL,
2849434997UL,
2858899367UL,
2863079543UL,
2868328233UL,
2869070114UL,
2869226513UL,
2871009380UL,
2871810822UL,
2876788815UL,
2876816233UL,
2880077624UL,
2884482440UL,
2884800460UL,
2888919787UL,
2889809107UL,
2892137356UL,
2893235566UL,
2897648218UL,
2900768883UL,
2903658059UL,
2906248349UL,
2909320001UL,
2916130978UL,
2918975465UL,
2923200035UL,
2926666910UL,
2927515658UL,
2929453081UL,
2929895745UL,
2936132555UL,
2941362695UL,
2943843512UL,
2944381098UL,
2944416696UL,
2947061964UL,
2947420738UL,
2949799888UL,
2953412493UL,
2956497708UL,
2958303996UL,
2958683177UL,
2959082724UL,
2959376429UL,
2962494213UL,
2962597747UL,
2963901570UL,
2964376334UL,
2964476263UL,
2965746109UL,
2965969735UL,
2967694161UL,
2967743267UL,
2968892941UL,
2971731223UL,
2978953019UL,
2979015354UL,
2979112781UL,
2982756377UL,
2986239846UL,
2991609908UL,
2992111257UL,
2998328393UL,
3006590422UL,
3007208089UL,
3010621613UL,
3012079102UL,
3012398608UL,
3015010017UL,
3016965623UL,
3020865078UL,
3025617259UL,
3026838994UL,
3030624199UL,
3032203815UL,
3037111044UL,
3037504004UL,
3038477307UL,
3039427935UL,
3041960943UL,
3043247283UL,
3045041653UL,
3046919091UL,
3051514001UL,
3053233081UL,
3054349059UL,
3055827762UL,
3056633497UL,
3056823364UL,
3061061880UL,
3062119853UL,
3065973518UL,
3067095846UL,
3068404581UL,
3068451985UL,
3073786799UL,
3076885600UL,
3077858753UL,
3082823252UL,
3084806269UL,
3086612887UL,
3086978686UL,
3088036524UL,
3091406857UL,
3094116722UL,
3095989206UL,
3099276498UL,
3102171820UL,
3103891649UL,
3104588663UL,
3107340961UL,
3108622379UL,
3108860961UL,
3112910930UL,
3113767455UL,
3114642578UL,
3117299905UL,
3119312615UL,
3119440621UL,
3122862177UL,
3135461853UL,
3137156168UL,
3137390602UL,
3137873708UL,
3144303462UL,
3145536891UL,
3146994431UL,
3147470995UL,
3150531504UL,
3152977744UL,
3154358421UL,
3154525606UL,
3155447863UL,
3159384985UL,
3160055836UL,
3161322554UL,
3167362688UL,
3172062681UL,
3175518292UL,
3182858400UL,
3182903712UL,
3183642086UL,
3185228175UL,
3187526325UL,
3189287571UL,
3190986146UL,
3197346116UL,
3198700497UL,
3199709302UL,
3200676340UL,
3205987041UL,
3208596291UL,
3210812366UL,
3213192119UL,
3219981107UL,
3225733427UL,
3231290723UL,
3231855813UL,
3235711487UL,
3240210207UL,
3242237877UL,
3247486291UL,
3250748633UL,
3251603213UL,
3252123622UL,
3255843419UL,
3259098665UL,
3264608485UL,
3269192861UL,
3269601101UL,
3269931358UL,
3272170941UL,
3273597876UL,
3274675359UL,
3278736728UL,
3281074265UL,
3281917114UL,
3283307377UL,
3285892920UL,
3292293910UL,
3292601715UL,
3299711755UL,
3302438317UL,
3303117517UL,
3303500508UL,
3305694324UL,
3307579120UL,
3309634203UL,
3313169098UL,
3313300925UL,
3315195320UL,
3316540885UL,
3322935631UL,
3323157739UL,
3324870427UL,
3332752308UL,
3334090397UL,
3340634148UL,
3342273438UL,
3346195652UL,
3349068474UL,
3349993603UL,
3351089532UL,
3353063157UL,
3355857946UL,
3357076305UL,
3358548111UL,
3359385886UL,
3359927478UL,
3364937717UL,
3366921411UL,
3367072945UL,
3369959983UL,
3370380605UL,
3370395734UL,
3371033601UL,
3373787042UL,
3374613544UL,
3375584354UL,
3378261638UL,
3378956826UL,
3384728271UL,
3385083669UL,
3386970172UL,
3388155153UL,
3388374334UL,
3388388643UL,
3391330979UL,
3394385093UL,
3395928758UL,
3398692103UL,
3404953265UL,
3405205361UL,
3406890780UL,
3411696116UL,
3413798025UL,
3418550615UL,
3419002701UL,
3422336970UL,
3425140178UL,
3428175115UL,
3428833092UL,
3429823897UL,
3433347746UL,
3437506284UL,
3440337975UL,
3445204612UL,
3446907930UL,
3449111877UL,
3450805178UL,
3452652925UL,
3453107261UL,
3453486258UL,
3453708752UL,
3463421325UL,
3463473114UL,
3466218620UL,
3467052378UL,
3467075887UL,
3467421578UL,
3468715478UL,
3469920019UL,
3472706788UL,
3477063501UL,
3478229888UL,
3481485175UL,
3485785250UL,
3488607036UL,
3494890583UL,
3494926454UL,
3495019082UL,
3495050839UL,
3497907846UL,
3502120726UL,
3502931387UL,
3503822232UL,
3503822489UL,
3505007324UL,
3509481368UL,
3515615774UL,
3516062094UL,
3517317305UL,
3520578955UL,
3522213370UL,
3523080940UL,
3524839374UL,
3526346390UL,
3527411383UL,
3537626039UL,
3538263040UL,
3542656013UL,
3544502610UL,
3545118945UL,
3545286963UL,
3545362760UL,
3550811661UL,
3552655983UL,
3558988441UL,
3560744038UL,
3561801179UL,
3563243112UL,
3563878394UL,
3563993105UL,
3564830587UL,
3565289859UL,
3565681337UL,
3580299887UL,
3584629962UL,
3585473648UL,
3587404099UL,
3590327820UL,
3591136458UL,
3592120922UL,
3597033151UL,
3600917323UL,
3601250027UL,
3604511355UL,
3604546738UL,
3610434623UL,
3612835625UL,
3613084166UL,
3614131077UL,
3615029898UL,
3616972630UL,
3617242721UL,
3617683136UL,
3619351885UL,
3619586146UL,
3619884325UL,
3624298370UL,
3630667165UL,
3632937784UL,
3636618796UL,
3638073461UL,
3645290691UL,
3648271647UL,
3649064923UL,
3659944626UL,
3664296540UL,
3665404711UL,
3670544011UL,
3671090828UL,
3675347226UL,
3680242053UL,
3680242054UL,
3682830693UL,
3688049823UL,
3688351515UL,
3691754481UL,
3694052312UL,
3695364924UL,
3695995572UL,
3703945192UL,
3704026693UL,
3704370378UL,
3705122455UL,
3709757482UL,
3713898202UL,
3727734592UL,
3731942382UL,
3731958811UL,
3732218154UL,
3736514962UL,
3737024708UL,
3741060478UL,
3745751005UL,
3746504041UL,
3747775545UL,
3749142160UL,
3749675851UL,
3752950562UL,
3755645230UL,
3755751727UL,
3760021166UL,
3760184785UL,
3761491022UL,
3762194849UL,
3764975715UL,
3766343397UL,
3766607227UL,
3769011566UL,
3777084010UL,
3780266271UL,
3784043520UL,
3785081316UL,
3789307418UL,
3790849430UL,
3792573041UL,
3801193941UL,
3805354358UL,
3808839530UL,
3809245861UL,
3811577267UL,
3817772486UL,
3829638671UL,
3831022976UL,
3831833145UL,
3839079557UL,
3839927321UL,
3840883331UL,
3842472721UL,
3847562856UL,
3848586457UL,
3849244279UL,
3851041481UL,
3852580888UL,
3855522243UL,
3857073024UL,
3861964125UL,
3867612393UL,
3868351552UL,
3871070174UL,
3873245129UL,
3877390049UL,
3877391093UL,
3877680540UL,
3881897006UL,
3883090450UL,
3894192615UL,
3898520867UL,
3901644985UL,
3903324928UL,
3903497963UL,
3907962790UL,
3910036177UL,
3910501967UL,
3913961507UL,
3922004524UL,
3928067937UL,
3929545186UL,
3934054333UL,
3940103162UL,
3943954739UL,
3945481792UL,
3946808571UL,
3947132970UL,
3949112832UL,
3952746303UL,
3952775206UL,
3954728860UL,
3960245135UL,
3960422418UL,
3966241026UL,
3975702363UL,
3981067707UL,
3981673014UL,
3983732179UL,
3987351542UL,
3993793762UL,
3996861010UL,
3998709917UL,
4004060567UL,
4004109939UL,
4004944163UL,
4008133425UL,
4010444213UL,
4012681477UL,
4017582172UL,
4019050991UL,
4024211785UL,
4029214626UL,
4032125239UL,
4035315789UL,
4035594361UL,
4045084347UL,
4046947436UL,
4047254468UL,
4048312703UL,
4049090770UL,
4049493677UL,
4049493678UL,
4052279150UL,
4053124604UL,
4053230961UL,
4064596188UL,
4065269513UL,
4067879299UL,
4069481512UL,
4069481862UL,
4071381028UL,
4072450508UL,
4074582241UL,
4082507315UL,
4084828698UL,
4085617538UL,
4086833273UL,
4088127009UL,
4094766340UL,
4095764255UL,
4099910234UL,
4101214571UL,
4104009085UL,
4106772614UL,
4106908219UL,
4110238413UL,
4111097366UL,
4113622641UL,
4113959718UL,
4114337765UL,
4114701575UL,
4114824682UL,
4114873206UL,
4114950893UL,
4117804330UL,
4118297931UL,
4122431097UL,
4123151252UL,
4123877061UL,
4125012840UL,
4129165899UL,
4130829911UL,
4132128737UL,
4132482931UL,
4132859614UL,
4134368071UL,
4134667896UL,
4135497641UL,
4138665869UL,
4139286815UL,
4146488299UL,
4151284832UL,
4152590671UL,
4155185133UL,
4156978930UL,
4157121899UL,
4157567815UL,
4162860660UL,
4165851644UL,
4166218761UL,
4167245955UL,
4167309142UL,
4168810389UL,
4169468875UL,
4171592864UL,
4171888188UL,
4173271781UL,
4173316509UL,
4179668880UL,
4180172514UL,
4180527114UL,
4191445161UL,
4194157408UL,
4196136373UL,
4196697640UL,
4199432401UL,
4201958683UL,
4203136157UL,
4203674650UL,
4204505438UL,
4208803307UL,
4210140251UL,
4214378173UL,
4216233813UL,
4221973191UL,
4222584931UL,
4226432799UL,
4226766059UL,
4234134193UL,
4235088187UL,
4235485149UL,
4237195614UL,
4238565651UL,
4238842866UL,
4242015419UL,
4244623852UL,
4247708043UL,
4248086958UL,
4251269056UL,
4253222473UL,
4253289435UL,
4257445136UL,
4258555600UL,
4259658120UL,
4264306831UL,
4270368850UL,
4272242651UL,
4273951086UL,
4274655655UL,
4275772888UL,
4276582568UL,
4280372840UL,
4280653102UL,
4282268691UL,
4288096336UL,
4292043952UL,
4294394764UL,
4294717941UL,
4294955400UL
};

AK::IAkPlugin* LAB_01fc44d7(AK::IAkPluginMemAlloc* param_1)
{
  /*undefined4* puVar1;
  int iVar2;

  puVar1 = (undefined4*)(**(code**)(*param_1 + 4))(0x150);
  if (puVar1 != (undefined4*)0x0) {
    *puVar1 = &PTR_LAB_01121c24;
    iVar2 = 2;
    do {
      FUN_01fc45b7();
      iVar2 = iVar2 + -1;
    } while (-1 < iVar2);
    puVar1[0x4e] = 0;
    puVar1[0x4f] = 0;
    puVar1[0x50] = 0;
    puVar1[0x51] = 0;
    puVar1[0x52] = 0;
    puVar1[0x53] = 0;
    puVar1[0x4c] = 0;
    puVar1[0x4d] = 0;
    puVar1[1] = 0;
    memset(puVar1 + 0x22, 0, 0x3c);
    memset(puVar1 + 0x31, 0, 0x6c);
    puVar1[9] = 0x3f800000;
    puVar1[0x20] = 0;
    puVar1[0x21] = 0;
    puVar1[0x1f] = 0;
    puVar1[3] = 0;
    puVar1[4] = 0;
    puVar1[5] = 0;
    puVar1[6] = 0;
    puVar1[7] = 0;
    puVar1[10] = 0;
    puVar1[0xc] = 0;
    puVar1[0xb] = 0;
    puVar1[0xd] = 0;
    puVar1[0x13] = 0;
    *(undefined2*)(puVar1 + 2) = 1;
    *(undefined*)(puVar1 + 0xe) = 1;
    puVar1[0x14] = 0;
    puVar1[0x15] = 0;
    puVar1[0x16] = 0;
    puVar1[0x17] = 0;
    *(undefined2*)(puVar1 + 0x18) = 0;
    puVar1[0x19] = 0;
    puVar1[0x1a] = 0;
    puVar1[0x1b] = 0;
    puVar1[0x1c] = 0;
    puVar1[0x1d] = 0;
    puVar1[0x1e] = 0;
    return puVar1;
  }*/
  return nullptr;
}

AK::IAkPluginParam* LAB_01fc3941(AK::IAkPluginMemAlloc* param_1)
{
  //undefined4* puVar1;

  //puVar1 = (undefined4*)(**(code**)(*param_1 + 4))(0x4c);
  //if (puVar1 != (undefined4*)0x0) {
  //  *puVar1 = &PTR_LAB_01121bf8;
  //  return puVar1;
  //}
  return nullptr;
}

AK::IAkPlugin* LAB_01f55909(AK::IAkPluginMemAlloc* param_1)
{
  return nullptr;
}

AK::IAkPluginParam* LAB_01f5552d(AK::IAkPluginMemAlloc* param_1)
{
  return nullptr;
}

AK::IAkPlugin* LAB_01f552ed(AK::IAkPluginMemAlloc* param_1)
{
  return nullptr;
}

AK::IAkPluginParam* LAB_01f552cd(AK::IAkPluginMemAlloc* param_1)
{
  return nullptr;
}

static void registerAllPlugins()
{
  AKRESULT res;
  res = AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeSource, 0, 0x66, LAB_01fc44d7, LAB_01fc3941);
  assert(res == AKRESULT::AK_Success);
  res = AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeSource, 0, 100, LAB_01f55909, LAB_01f5552d);
  assert(res == AKRESULT::AK_Success);
  res = AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeSource, 0, 0x65, LAB_01f552ed, LAB_01f552cd);
  assert(res == AKRESULT::AK_Success);
  res = AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeSource, 0, 200, CreateAudioInputSource, CreateAudioInputSourceParams);
  assert(res == AKRESULT::AK_Success);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x73, (FuncDef5*)&LAB_01f32348, (FuncDef6*)&LAB_01f2727d);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x76, (FuncDef5*)&LAB_01f516ef, (FuncDef6*)&LAB_01f4d8ff);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x69, (FuncDef5*)&LAB_01f49cd6, (FuncDef6*)&LAB_01f48f37);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x6a, (FuncDef5*)&LAB_01f184fa, (FuncDef6*)&LAB_01f182f6);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x87, (FuncDef5*)&LAB_01fba060, (FuncDef6*)&LAB_01fb9a21);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x88, (FuncDef5*)&LAB_01f20d74, (FuncDef6*)&LAB_01f2052c);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x6c, (FuncDef5*)&LAB_01f125a4, (FuncDef6*)&LAB_01f117e5);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x6d, (FuncDef5*)&LAB_01f19c61, (FuncDef6*)&LAB_01f18e6a);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x6e, (FuncDef5*)&LAB_01f4bb16, (FuncDef6*)&LAB_01f4a006);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x7d, (FuncDef5*)&LAB_01f1ad5e, (FuncDef6*)&LAB_01f1a11d);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x7e, (FuncDef5*)&LAB_01f1d8fe, (FuncDef6*)&LAB_01f1d154);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x7f, (FuncDef5*)&LAB_01f13d1f, (FuncDef6*)&LAB_01f12ad8);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x83, (FuncDef5*)&LAB_01fc6bc4, (FuncDef6*)&LAB_01fc66c2);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x82, (FuncDef5*)&LAB_01fc35b1, (FuncDef6*)&LAB_01fc2ce3);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x84, (FuncDef5*)&LAB_01f4c583, (FuncDef6*)&LAB_01f4bef8);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x81, (FuncDef5*)&LAB_01f33e23, (FuncDef6*)&LAB_01f334fd);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0x100, 0x6e, (FuncDef5*)&LAB_01febc77, (FuncDef6*)&LAB_01feae79);
  //AK::SoundEngine::RegisterPlugin(AkPluginType::AkPluginTypeEffect, 0, 0x8a, (FuncDef5*)&LAB_01f232f2, (FuncDef6*)&LAB_01f22929);
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

  AkPlatformInitSettings platformInitSettings;
  AK::SoundEngine::GetDefaultPlatformInitSettings(platformInitSettings);
  platformInitSettings.uLEngineDefaultPoolSize = DEMO_LENGINE_DEFAULT_POOL_SIZE;

  AkMusicSettings initMusicSettings;
  initMusicSettings.fStreamingLookAheadRatio = 1.0f;
  AK::MusicEngine::GetDefaultInitSettings(initMusicSettings);

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


  //registerAllPlugins();
  res = AK::SoundEngine::RegisterAllPlugins();
  if (res != AK_Success)
    abort();

  res = AK::SoundEngine::RegisterAllCodecPlugins();
  if (res != AK_Success)
    abort();

  m_pLowLevelIO.SetBasePath(SOUND_BANK_PATH);

  if (AK::StreamMgr::SetCurrentLanguage(AKTEXT("English(US)")) != AK_Success)
    abort();

  if (!SoundInputMgr::Instance().Initialize())
    abort();

#define GAME_OBJECT 1234
  if (AK::SoundEngine::RegisterGameObj(GAME_OBJECT, "GAME_OBJECT") != AK_Success)
    abort();

  AkBankID bankID;
  if (AK::SoundEngine::LoadBank("init.bnk", AK_DEFAULT_POOL_ID, bankID) != AK_Success)
    abort();

  loadAllBnkFiles();


  AK::SoundEngine::SetRTPCValue("Master_Volume", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("MusicRamping", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Crowd_RRVolumeControl", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_SFX", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_Music", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_Player1", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_Player2", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_VO", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Mixer_Mic", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Player_Success", 100, GAME_OBJECT);
  AK::SoundEngine::SetRTPCValue("Portal_Size", 100, GAME_OBJECT);

  AK::SoundEngine::SetRTPCValue("BusVolume", 100, GAME_OBJECT);

  //for (unsigned int i : init_rtpc)
  //{
  //  AK::SoundEngine::SetRTPCValue(i, 1000.0f, GAME_OBJECT);
  //}

  {
    AK::SoundEngine::PostEvent("Play_TitleScreen", GAME_OBJECT);
    AK::SoundEngine::PostEvent("TitleScreen", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Dialog_TransitionIn", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Dialog_TransitionOut", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Play_RocksmithStart_NewVO", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Enter_Bib", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Nav_Down", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Nav_Select", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Enter_Dialog_WarnTapeMachine", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Play_ProfileLoaded", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Exit_Dialog_WarnTapeMachine", GAME_OBJECT);
    AK::SoundEngine::PostEvent("GearChangeComplete", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Enter_MainMenu", GAME_OBJECT);
    AK::SoundEngine::PostEvent("Nav_Mouse_Focus", GAME_OBJECT);

    AK::SoundEngine::SetGameObjectOutputBusVolume(GAME_OBJECT, 100.0f);


    //AK::MotionEngine::SetPlayerVolume(0, 1.0f);
    //AK::MotionEngine::SetPlayerVolume(GAME_OBJECT, 1.0f);

  }

  AK::SoundEngine::PostEvent(1984420030, GAME_OBJECT);

  for (;;) {
    if (AK::SoundEngine::IsInitialized())
      AK::SoundEngine::RenderAudio();
  }

  return 0;
}