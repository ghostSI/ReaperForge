#include "getopt.h"

#include <string.h>

const char *optarg;        // global argument pointer
int optind = 0;    // global argv index

int getopt(int argc, char *argv[], const char *optstring) {
    static const char *next = nullptr;
    if (optind == 0)
        next = nullptr;

    optarg = nullptr;

    if (next == nullptr || *next == '\0') {
        if (optind == 0)
            optind++;

        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
            optarg = nullptr;
            if (optind < argc)
                optarg = argv[optind];
            return -1;
        }

        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            optarg = nullptr;
            if (optind < argc)
                optarg = argv[optind];
            return -1;
        }

        next = argv[optind];
        next++;        // skip past -
        optind++;
    }

    char c = *next++;
    const char *cp = strchr(optstring, c);

    if (cp == nullptr || c == ':')
        return '?';

    cp++;
    if (*cp == ':') {
        if (*next != '\0') {
            optarg = next;
            next = nullptr;
        } else if (optind < argc) {
            optarg = argv[optind];
            optind++;
        } else {
            return '?';
        }
    }

    return c;
}
