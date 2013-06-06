/* PrintByteCode.h */

#ifndef PRINTBYTECODEH

#define PRINTBYTECODEH

#include "ClassFileFormat.h"  /* to define ClassFile type */

extern char *GetOpcodeName( int op );
extern void PrintByteCode( ClassFile *cf, uint8_t *code, int len );

#endif
