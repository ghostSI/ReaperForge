#include "log.h"

#include <chrono>
#include <stdarg.h>
#include <time.h>
#include <string>

static constexpr char reset[] = "\033[0m"; /* reset to default Color */
static constexpr char black[] = "\033[30m";
static constexpr char red[] = "\033[31m";
static constexpr char green[] = "\033[32m";
static constexpr char yellow[] = "\033[33m";
static constexpr char blue[] = "\033[34m";
static constexpr char magenta[] = "\033[35m";
static constexpr char cyan[] = "\033[36m";
static constexpr char white[] = "\033[37m";
static constexpr char boldBlack[] = "\033[1m\033[30m";
static constexpr char boldRed[] = "\033[1m\033[31m";
static constexpr char boldGreen[] = "\033[1m\033[32m";
static constexpr char boldYellow[] = "\033[1m\033[33m";
static constexpr char boldBlue[] = "\033[1m\033[34m";
static constexpr char boldMagenta[] = "\033[1m\033[35m";
static constexpr char boldCyan[] = "\033[1m\033[36m";
static constexpr char boldWhite[] = "\033[1m\033[37m";

static void windowsEnableColors()
{
#ifdef _WIN32
  static bool isEnabled;
  if (isEnabled)
    return;

  isEnabled = true;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
}

static char* getTimeString()
{
  static char timeString[20];

  const time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  strftime(&timeString[0], 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
  return timeString;
}

void Log::fatalErrorMsg(const char* errorMsg, ...)
{
  windowsEnableColors();

  fprintf(stderr, "%s %sFatal Error:%s ", getTimeString(), boldRed, reset);

  va_list args;
  va_start(args, errorMsg);
  vfprintf(stderr, errorMsg, args);
  va_end(args);

  fputc('\n', stderr);

  fflush(stderr);

  //SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, errorCode_string.c_str(), errorMsg.c_str(), nullptr);
  //SDL_Quit();
}

void Log::errorMsg(const char* errorMsg, ...)
{
  windowsEnableColors();

  fprintf(stderr, "%s %sError:%s ", getTimeString(), red, reset);

  va_list args;
  va_start(args, errorMsg);
  vfprintf(stderr, errorMsg, args);
  va_end(args);

  fputc('\n', stderr);

  fflush(stderr);
}

void Log::infoMsg(const char* infoMsg, ...)
{
  windowsEnableColors();

  fprintf(stderr, "%s %sInfo:%s ", getTimeString(), yellow, reset);

  va_list args;
  va_start(args, infoMsg);
  vfprintf(stderr, infoMsg, args);
  va_end(args);

  fputc('\n', stderr);

  fflush(stderr);
}

void Log::printCommandLineArguments(int argc, char* argv[])
{
  windowsEnableColors();

  // Append Command Line input to Debug Console
  std::string arguments;
  arguments.append("Command Line Args:\n");
  for (int i = 0; i < argc; ++i)
  {
    arguments.append(" [");
    arguments.append(std::to_string(i));
    arguments.append("]: ");
    arguments.append((argv[i]));
    arguments.append("\n");
  }
  infoMsg(arguments.c_str());
}

struct Log::RaiiTimerInstanceData
{
  std::chrono::high_resolution_clock::time_point begin = std::chrono::high_resolution_clock::now();
  const char* name;
};

Log::RaiiTimer::RaiiTimer(const char* name) : i(new RaiiTimerInstanceData)
{
  i->name = name;
}

Log::RaiiTimer::~RaiiTimer()
{
  const f32 diff = std::chrono::duration<f32, std::milli>(std::chrono::high_resolution_clock::now() - i->begin).count();
  printf("%s %s: %f\n", getTimeString(), i->name, diff);
}

