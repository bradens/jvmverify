/* ReadClassFile.h */

#ifndef READCLASSFILEH

#define READCLASSFILEH

#include <stdint.h>  /* to define uint8_t */
#include "ClassFileFormat.h"  /* to define ClassFile type */

extern void PrintFilesRead();
extern int CountParameters( uint8_t *s );
extern ClassFile *ReadClassFile( char *filename );

#endif
