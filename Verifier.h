/* Verify.h */

#ifndef VERIFYH
#define VERIFYH

#include "ClassFileFormat.h"  // for ClassFile

extern void Verify( ClassFile *cf );
extern void InitVerifier(void);

#endif
