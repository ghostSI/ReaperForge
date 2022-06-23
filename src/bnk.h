#ifndef BNK_H
#define BNK_H

#include "configuration.h"

#ifdef SUPPORT_BNK

#include "typedefs.h"

namespace Bnk
{
  enum struct Result : i32
  {
    NotImplemented = 0,	///< This feature is not implemented.
    Success = 1,	///< The operation was successful.
    Fail = 2,	///< The operation failed.
    PartialSuccess = 3,	///< The operation succeeded partially.
    NotCompatible = 4,	///< Incompatible formats
    AlreadyConnected = 5,	///< The stream is already connected to another node.
    InvalidFile = 7,	///< The provided file is the wrong format or unexpected values causes the file to be invalid.
    AudioFileHeaderTooLarge = 8,	///< The file header is too large.
    MaxReached = 9,	///< The maximum was reached.
    InvalidID = 14,	///< The ID is invalid.
    IDNotFound = 15,	///< The ID was not found.
    InvalidInstanceID = 16,	///< The InstanceID is invalid.
    NoMoreData = 17,	///< No more data is available from the source.
    InvalidStateGroup = 20,	///< The StateGroup is not a valid channel.
    ChildAlreadyHasAParent = 21,	///< The child already has a parent.
    InvalidLanguage = 22,	///< The language is invalid (applies to the Low-Level I/O).
    CannotAddItseflAsAChild = 23,	///< It is not possible to add itself as its own child.
    InvalidParameter = 31,	///< Something is not within bounds, check the documentation of the function returning this code.
    ElementAlreadyInList = 35,	///< The item could not be added because it was already in the list.
    PathNotFound = 36,	///< This path is not known.
    PathNoVertices = 37,	///< Stuff in vertices before trying to start it
    PathNotRunning = 38,	///< Only a running path can be paused.
    PathNotPaused = 39,	///< Only a paused path can be resumed.
    PathNodeAlreadyInList = 40,	///< This path is already there.
    PathNodeNotInList = 41,	///< This path is not there.
    DataNeeded = 43,	///< The consumer needs more.
    NoDataNeeded = 44,	///< The consumer does not need more.
    DataReady = 45,	///< The provider has available data.
    NoDataReady = 46,	///< The provider does not have available data.
    InsufficientMemory = 52,	///< Memory error.
    Cancelled = 53,	///< The requested action was cancelled (not an error).
    UnknownBankID = 54,	///< Trying to load a bank using an ID which is not defined.
    BankReadError = 56,	///< Error while reading a bank.
    InvalidSwitchType = 57,	///< Invalid switch type (used with the switch container)
    FormatNotReady = 63,   ///< Source format not known yet.
    WrongBankVersion = 64,	///< The bank version is not compatible with the current bank reader.
    FileNotFound = 66,   ///< File not found.
    DeviceNotReady = 67,   ///< Specified ID doesn't match a valid hardware device: either the device doesn't exist or is disabled.
    BankAlreadyLoaded = 69,	///< The bank load failed because the bank is already loaded.
    RenderedFX = 71,	///< The effect on the node is rendered.
    ProcessNeeded = 72,	///< A routine needs to be executed on some CPU.
    ProcessDone = 73,	///< The executed routine has finished its execution.
    MemManagerNotInitialized = 74,	///< The memory manager should have been initialized at this point.
    StreamMgrNotInitialized = 75,	///< The stream manager should have been initialized at this point.
    SSEInstructionsNotSupported = 76,///< The machine does not support SSE instructions (required on PC).
    Busy = 77,	///< The system is busy and could not process the request.
    UnsupportedChannelConfig = 78,	///< Channel configuration is not supported in the current execution context.
    PluginMediaNotAvailable = 79,	///< Plugin media is not available for effect.
    MustBeVirtualized = 80,	///< Sound was Not Allowed to play.
    CommandTooLarge = 81,	///< SDK command is too large to fit in the command queue.
    RejectedByFilter = 82,	///< A play request was rejected due to the MIDI filter parameters.
    InvalidCustomPlatformName = 83,	///< Detecting incompatibility between Custom platform of banks and custom platform of connected application
    DLLCannotLoad = 84,	///< Plugin DLL could not be loaded, either because it is not found or one dependency is missing.
    DLLPathNotFound = 85,	///< Plugin DLL search path could not be found.
    NoJavaVM = 86,	///< No Java VM provided in AkInitSettings.
    OpenSLError = 87,	///< OpenSL returned an error.  Check error log for more details.
    PluginNotRegistered = 88,	///< Plugin is not registered.  Make sure to implement a AK::PluginRegistration class for it and use AK_STATIC_LINK_PLUGIN in the game binary.
    DataAlignmentError = 89,	///< A pointer to audio data was not aligned to the platform's required alignment (check AkTypes.h in the platform-specific folder)
    DeviceNotCompatible = 90,	///< Incompatible Audio device.
    DuplicateUniqueID = 91,	///< Two Wwise objects share the same ID.
    InitBankNotLoaded = 92,	///< The Init bank was not loaded yet, the sound engine isn't completely ready yet.
    DeviceNotFound = 93,	///< The specified device ID does not match with any of the output devices that the sound engine is currently using.
    PlayingIDNotFound = 94,	///< Calling a function with a playing ID that is not known.
    InvalidFloatValue = 95,	///< One parameter has a invalid float value such as NaN, INF or FLT_MAX.
    FileFormatMismatch = 96,   ///< Media file format unexpected
    NoDistinctListener = 97,	///< No distinct listener provided for AddOutput
    ACP_Error = 98,	///< Generic XMA decoder error.
    ResourceInUse = 99,	///< Resource is in use and cannot be released.
    InvalidBankType = 100,	///< Invalid bank type. The bank type was either supplied through a function call (e.g. LoadBank) or obtained from a bank loaded from memory.
    AlreadyInitialized = 101,	///< Init() was called but that element was already initialized.
    NotInitialized = 102,	///< The component being used is not initialized. Most likely AK::SoundEngine::Init() was not called yet, or AK::SoundEngine::Term was called too early.
    FilePermissionError = 103,	///< The file access permissions prevent opening a file.
    UnknownFileError = 104,	///< Rare file error occured, as opposed to AK_FileNotFound or AK_FilePermissionError. This lumps all unrecognized OS file system errors.
  };

  using UniqueID = u32;
  using BankID = u32;
  using RtpcID = u32;
  using RtpcValue = f32;
  using PlayID = u32;

  namespace EVENTS
  {
    static const UniqueID DISABLE_MICROPHONE_DELAY = 6251382U;
    static const UniqueID ENABLE_MICROPHONE_DELAY = 3533161767U;
    static const UniqueID ENTER_AREA_1 = 2507024135U;
    static const UniqueID ENTER_AREA_2 = 2507024132U;
    static const UniqueID IM_1_ONE_ENEMY_WANTS_TO_FIGHT = 2221704914U;
    static const UniqueID IM_2_TWO_ENEMIES_WANT_TO_FIGHT = 3753105098U;
    static const UniqueID IM_3_SURRONDED_BY_ENEMIES = 1350929071U;
    static const UniqueID IM_4_DEATH_IS_COMING = 3175089270U;
    static const UniqueID IM_COMMUNICATION_BEGIN = 2160840676U;
    static const UniqueID IM_EXPLORE = 3280047539U;
    static const UniqueID IM_GAMEOVER = 3455955770U;
    static const UniqueID IM_START = 3952084898U;
    static const UniqueID IM_THEYAREHOSTILE = 2841817544U;
    static const UniqueID IM_WINTHEFIGHT = 1133905385U;
    static const UniqueID METRONOME_POSTMIDI = 2710399919U;
    static const UniqueID PAUSE_ALL = 3864097025U;
    static const UniqueID PAUSE_ALL_GLOBAL = 3493516265U;
    static const UniqueID PLAY_3D_AUDIO_DEMO = 4057320454U;
    static const UniqueID PLAY_3DBUS_DEMO = 834165051U;
    static const UniqueID PLAY_AMBIENCE_QUAD = 146788224U;
    static const UniqueID PLAY_CHIRP = 3187155090U;
    static const UniqueID PLAY_CLUSTER = 2148126352U;
    static const UniqueID PLAY_ENGINE = 639345804U;
    static const UniqueID PLAY_FOOTSTEP = 1602358412U;
    static const UniqueID PLAY_FOOTSTEPS = 3854155799U;
    static const UniqueID PLAY_HELLO = 2952797154U;
    static const UniqueID PLAY_HELLO_REVERB = 3795249249U;
    static const UniqueID PLAY_MARKERS_TEST = 3368417626U;
    static const UniqueID PLAY_MICROPHONE = 1324678662U;
    static const UniqueID PLAY_NONRECORDABLEMUSIC = 3873244457U;
    static const UniqueID PLAY_POSITIONING_DEMO = 1237313597U;
    static const UniqueID PLAY_RECORDABLEMUSIC = 2567011622U;
    static const UniqueID PLAY_ROOM_EMITTER = 2172342284U;
    static const UniqueID PLAY_SANDSTEP = 2266299534U;
    static const UniqueID PLAY_THREE_NUMBERS_IN_A_ROW = 4142087708U;
    static const UniqueID PLAYMUSICDEMO1 = 519773714U;
    static const UniqueID PLAYMUSICDEMO2 = 519773713U;
    static const UniqueID PLAYMUSICDEMO3 = 519773712U;
    static const UniqueID RESUME_ALL = 3679762312U;
    static const UniqueID RESUME_ALL_GLOBAL = 1327221850U;
    static const UniqueID STOP_3DBUS_DEMO = 246841725U;
    static const UniqueID STOP_ALL = 452547817U;
    static const UniqueID STOP_CLUSTER = 2775363470U;
    static const UniqueID STOP_ENGINE = 37214798U;
    static const UniqueID STOP_MICROPHONE = 3629954576U;
  } // namespace EVENTS

  namespace DIALOGUE_EVENTS
  {
    static const UniqueID OBJECTIVE_STATUS = 3970659059U;
    static const UniqueID UNIT_UNDER_ATTACK = 3585983975U;
    static const UniqueID WALKIETALKIE = 4110439188U;
  } // namespace DIALOGUE_EVENTS

  namespace STATES
  {
    namespace HOSTILE
    {
      static const UniqueID GROUP = 3712907969U;

      namespace STATE
      {
        static const UniqueID BUM = 714721627U;
        static const UniqueID GANG = 685704824U;
        static const UniqueID NONE = 748895195U;
      } // namespace STATE
    } // namespace HOSTILE

    namespace LOCATION
    {
      static const UniqueID GROUP = 1176052424U;

      namespace STATE
      {
        static const UniqueID ALLEY = 672587556U;
        static const UniqueID HANGAR = 2192450996U;
        static const UniqueID NONE = 748895195U;
        static const UniqueID STREET = 4142189312U;
      } // namespace STATE
    } // namespace LOCATION

    namespace MUSIC
    {
      static const UniqueID GROUP = 3991942870U;

      namespace STATE
      {
        static const UniqueID EXPLORING = 1823678183U;
        static const UniqueID FIGHT = 514064485U;
        static const UniqueID FIGHT_DAMAGED = 886139701U;
        static const UniqueID FIGHT_DYING = 4222988787U;
        static const UniqueID FIGHT_LOWHEALTH = 1420167880U;
        static const UniqueID GAMEOVER = 4158285989U;
        static const UniqueID NONE = 748895195U;
        static const UniqueID PLAYING = 1852808225U;
        static const UniqueID WINNING_THEFIGHT = 1323211483U;
      } // namespace STATE
    } // namespace MUSIC

    namespace OBJECTIVE
    {
      static const UniqueID GROUP = 6899006U;

      namespace STATE
      {
        static const UniqueID DEFUSEBOMB = 3261872615U;
        static const UniqueID NEUTRALIZEHOSTILE = 141419130U;
        static const UniqueID NONE = 748895195U;
        static const UniqueID RESCUEHOSTAGE = 3841112373U;
      } // namespace STATE
    } // namespace OBJECTIVE

    namespace OBJECTIVESTATUS
    {
      static const UniqueID GROUP = 3299963692U;

      namespace STATE
      {
        static const UniqueID COMPLETED = 94054856U;
        static const UniqueID FAILED = 1655200910U;
        static const UniqueID NONE = 748895195U;
      } // namespace STATE
    } // namespace OBJECTIVESTATUS

    namespace PLAYERHEALTH
    {
      static const UniqueID GROUP = 151362964U;

      namespace STATE
      {
        static const UniqueID BLASTED = 868398962U;
        static const UniqueID NONE = 748895195U;
        static const UniqueID NORMAL = 1160234136U;
      } // namespace STATE
    } // namespace PLAYERHEALTH

    namespace UNIT
    {
      static const UniqueID GROUP = 1304109583U;

      namespace STATE
      {
        static const UniqueID NONE = 748895195U;
        static const UniqueID UNIT_A = 3004848135U;
        static const UniqueID UNIT_B = 3004848132U;
      } // namespace STATE
    } // namespace UNIT

    namespace WALKIETALKIE
    {
      static const UniqueID GROUP = 4110439188U;

      namespace STATE
      {
        static const UniqueID COMM_IN = 1856010785U;
        static const UniqueID COMM_OUT = 1553720736U;
        static const UniqueID NONE = 748895195U;
      } // namespace STATE
    } // namespace WALKIETALKIE

  } // namespace STATES

  namespace SWITCHES
  {
    namespace FOOTSTEP_GAIT
    {
      static const UniqueID GROUP = 4202554577U;

      namespace SWITCH
      {
        static const UniqueID RUN = 712161704U;
        static const UniqueID WALK = 2108779966U;
      } // namespace SWITCH
    } // namespace FOOTSTEP_GAIT

    namespace FOOTSTEP_WEIGHT
    {
      static const UniqueID GROUP = 246300162U;

      namespace SWITCH
      {
        static const UniqueID HEAVY = 2732489590U;
        static const UniqueID LIGHT = 1935470627U;
      } // namespace SWITCH
    } // namespace FOOTSTEP_WEIGHT

    namespace SURFACE
    {
      static const UniqueID GROUP = 1834394558U;

      namespace SWITCH
      {
        static const UniqueID DIRT = 2195636714U;
        static const UniqueID GRAVEL = 2185786256U;
        static const UniqueID METAL = 2473969246U;
        static const UniqueID WOOD = 2058049674U;
      } // namespace SWITCH
    } // namespace SURFACE

  } // namespace SWITCHES

  namespace GAME_PARAMETERS
  {
    static const UniqueID ENABLE_EFFECT = 2451442924U;
    static const UniqueID FOOTSTEP_SPEED = 3182548923U;
    static const UniqueID FOOTSTEP_WEIGHT = 246300162U;
    static const UniqueID RPM = 796049864U;
  } // namespace GAME_PARAMETERS

  namespace BANKS
  {
    static const UniqueID INIT = 1355168291U;
    static const UniqueID BGM = 412724365U;
    static const UniqueID BUS3D_DEMO = 3682547786U;
    static const UniqueID CAR = 983016381U;
    static const UniqueID DIRT = 2195636714U;
    static const UniqueID DLLDEMO = 2517646102U;
    static const UniqueID DYNAMICDIALOGUE = 1028808198U;
    static const UniqueID EXTERNALSOURCES = 480966290U;
    static const UniqueID GRAVEL = 2185786256U;
    static const UniqueID HUMAN = 3887404748U;
    static const UniqueID INTERACTIVEMUSIC = 2279279248U;
    static const UniqueID MARKERTEST = 2309453583U;
    static const UniqueID METAL = 2473969246U;
    static const UniqueID METRONOME = 3537469747U;
    static const UniqueID MICROPHONE = 2872041301U;
    static const UniqueID MUSICCALLBACKS = 4146461094U;
    static const UniqueID PAUSERESUME = 3699003020U;
    static const UniqueID POSITIONING_DEMO = 418215934U;
    static const UniqueID PREPAREDEMO = 3353080015U;
    static const UniqueID THREED_AUDIO_DEMO = 1265494800U;
    static const UniqueID WOOD = 2058049674U;
  } // namespace BANKS

  namespace BUSSES
  {
    static const UniqueID _3D_SUBMIX_BUS = 1101487118U;
    static const UniqueID _3D_AUDIO_DEMO = 3742896575U;
    static const UniqueID _3D_BUS_DEMO = 4083517055U;
    static const UniqueID BGM = 412724365U;
    static const UniqueID DRY_PATH = 1673180298U;
    static const UniqueID ENVIRONMENTAL_BUS = 3600197733U;
    static const UniqueID ENVIRONMENTS = 3761286811U;
    static const UniqueID GAME_PAD_BUS = 3596053402U;
    static const UniqueID MASTER_AUDIO_BUS = 3803692087U;
    static const UniqueID MUSIC = 3991942870U;
    static const UniqueID MUTED_FOR_USER_MUSIC = 1949198961U;
    static const UniqueID NON_RECORDABLE_BUS = 461496087U;
    static const UniqueID NON_WORLD = 838047381U;
    static const UniqueID PORTALS = 2017263062U;
    static const UniqueID SOUNDS = 1492361653U;
    static const UniqueID VOICES = 3313685232U;
    static const UniqueID VOICES_RADIO = 197057172U;
    static const UniqueID WET_PATH_3D = 2281484271U;
    static const UniqueID WET_PATH_OMNI = 1410202225U;
    static const UniqueID WORLD = 2609808943U;
  } // namespace BUSSES

  namespace AUX_BUSSES
  {
    static const UniqueID HANGAR_ENV = 2112490296U;
    static const UniqueID LISTENERENV = 924456902U;
    static const UniqueID OUTSIDE = 438105790U;
    static const UniqueID ROOM = 2077253480U;
    static const UniqueID ROOM1 = 1359360137U;
    static const UniqueID ROOM2 = 1359360138U;
  } // namespace AUX_BUSSES

  namespace AUDIO_DEVICES
  {
    static const UniqueID COMMUNICATION_OUTPUT = 3884583641U;
    static const UniqueID CONTROLLER_HEADPHONES = 2868300805U;
    static const UniqueID DVR_BYPASS = 1535232814U;
    static const UniqueID NO_OUTPUT = 2317455096U;
    static const UniqueID PAD_OUTPUT = 666305828U;
    static const UniqueID SYSTEM = 3859886410U;
  } // namespace AUDIO_DEVICES

  namespace EXTERNAL_SOURCES
  {
    static const UniqueID EXTERN_2ND_NUMBER = 293435250U;
    static const UniqueID EXTERN_3RD_NUMBER = 978954801U;
    static const UniqueID EXTERN_1ST_NUMBER = 4004957102U;
  } // namespace EXTERNAL_SOURCES

  void init();
  void tick();

  Bnk::Result loadBank(const char* name, Bnk::BankID& bankID);
  Bnk::PlayID postEvent(Bnk::UniqueID eventID);
  Bnk::Result setRTPCValue(Bnk::RtpcID rtpcId, Bnk::RtpcValue value);
  void inputOn(Bnk::PlayID playID);
}

#endif // SUPPORT_BNK

#endif // BNK_H
