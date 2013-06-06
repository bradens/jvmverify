/* InterpretLoop.h */

#ifndef INTERPRETLOOPH

#define INTERPRETLOOPH

extern void throwException( char *kind, uint8_t *pc, method_info *meth, ClassType *ct );
extern void throwExceptionExternal( char *kind, char *methodname, char *className );

extern void  PushConstant( ClassType *ct, int i );
extern int InterpretMethod( ClassType *ct, method_info *meth, DataItem *localVariable );

#endif
