/* StringBuilder.h */

#ifndef STRINGBUILDERH

#define STRINGBUILDERH

#include "jvm.h"  /* to define ClassInstance type */

extern char *StringBuilderName;

extern void StringBuilderClass( char *methodName, char *methodDescr );
extern ClassInstance *NewStringBuilderInstance();

#endif
