#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#ifdef DEBUG
#define RUN_TEST // build the test instead of bulding the game
#endif // DEBUG

//#define XBLOCK_FULL // read all data from xblock files

#define ARRANGEMENT_XML // read arrangement.xml file

//#define FORCE_OPENGL_ES

#define XML_IGNORE_ERROR

#define OPENGL_ERROR_CHECK

#define COLLECTION_WORKER_THREAD

#ifdef _WIN32
#define SUPPORT_BNK
#define SUPPORT_PLUGIN
#define SUPPORT_MIDI // bind notes to switch Tone Assignments
#endif // _WIN32

#ifdef SUPPORT_PLUGIN
#define SUPPORT_VST
#define SUPPORT_VST3
#endif // SUPPORT_PLUGIN

#endif // CONFIGURATION_H
