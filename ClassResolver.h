/* ClassResolver.h */

#ifndef CLASSFILERESOLVER
#define CLASSFILERESOLVER

#include "ClassFileFormat.h"  // for definition of ClassFile
#include "jvm.h"              // for definition of ClassType

extern ClassType *FirstLoadedClass;

extern void InvokeMethod( ClassType *ct, method_info *m, int isStatic );

extern void InvokeStaticMethod( ClassType *ct, int ix );
extern void InvokeSpecialMethod( ClassType *ct, int ix );
extern void InvokeVirtualMethod( ClassType *ct, int ix );

extern method_info *SearchClassForMethodByName(
        ClassFile *cf, char *name, char *signature );
extern ClassType *ResolveClassReference( ClassType *ct, int ix );
extern ClassType *ResolveClassReferenceByName( char *name );

extern ClassType *LoadClass( char *cname );

extern int GetStatic(ClassType *ct, int ix);
extern int GetField(ClassType *ct, int ix);
extern int PutStatic(ClassType *ct, int ix);
extern int PutField(ClassType *ct, int ix);

#endif

