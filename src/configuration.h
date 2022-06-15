#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#ifdef DEBUG
#define RUN_TEST // build the test instead of bulding the game
#endif // DEBUG

//#define XBLOCK_FULL // read all data from xblock files

#define ARRANGEMENT_XML // read arrangement.xml file

//#define FORCE_OPENGL_ES

#define XML_IGNORE_ERROR

#ifdef _WIN32
#define SUPPORT_MIDI // bind effects 
#define SUPPORT_VST
#endif // _WIN32

#endif // CONFIGURATION_H
