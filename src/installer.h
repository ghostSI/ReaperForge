#ifndef INSTALLER_H
#define INSTALLER_H

namespace Installer {
    bool isInstalled(const char *installPath);

    void install(const char* installPath);
}


#endif // INSTALLER_H
