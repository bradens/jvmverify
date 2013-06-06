/* printClassfile.h */

#ifndef PRINTCLASSFILEH

#define PRINTCLASSFILEH

#include "ClassFileFormat.h"  /* to define ClassFile type */

extern void PrintConstantPool( ClassFile *cf );
extern void PrintMethod( ClassFile *cf, int ix );
extern void PrintClassFile( ClassFile *cf );

#endif
