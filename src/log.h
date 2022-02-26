#ifndef LOG_H
#define LOG_H

#include "typedefs.h"

namespace Log {
    void fatalErrorMsg(const char *errorMsg, ...);

    void errorMsg(const char *errorMsg, ...);

    void infoMsg(const char *infoMsg, ...);

    void printCommandLineArguments(int argc, char *argv[]);

    struct RaiiTimer {
        struct RaiiTimerInstanceData *i = nullptr;

        RaiiTimer(const char *name = {});

        ~RaiiTimer();
    };
}

#endif // LOG_H
