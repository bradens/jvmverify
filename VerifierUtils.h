/* VerifierUtils.h */

#ifndef VERIFIERUTILS_H
#define VERIFIERUTILS_H

#include "ClassFileFormat.h"  // for ClassFile and method_info type definitions

extern void FreeTypeDescriptor( char *s );
extern char *ExtractOneType( char **resultp, char *jvmType );
extern int ExtractTypesFromSignature( char **argsp, char **retTypep, char *sig );
extern char **MapSigToInitState( ClassFile *cf, method_info *m, char **retTypep );
extern char **AnalyzeInvoke( ClassFile *cf, int ix, int isStatic, char **retTypep, int *cntp );
extern char *FieldTypeCode( ClassFile *cf, int ix );
extern char **AncestorTypes( char *typedescr, int *cntp );
extern char *LUB( char *type1, char *type2 );
extern void FreeTypeDescriptorArray( char **ap, int numSlots );

#endif
