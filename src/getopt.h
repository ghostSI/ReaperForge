#ifndef GETOPT_H
#define GETOPT_H

extern int optind;
extern const char *optarg;

int getopt(int argc, char *argv[], const char *optstring);

#endif // GETOPT_H
