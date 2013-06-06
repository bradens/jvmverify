/* NativeClasses.c */

/* A few classes from the java.lang and java.io packages are implemented
   directly in C.
   Only a few of the important fields and methods for these classes
   are supported.

   The classes and field/methods are:
      java/lang/System
          out   -- static field, type PrintStream
          gc()  -- static method
      java/lang/Integer
          parseInt()  -- static method
      java/lang/Double
          parseDouble() -- static method
      java/lang/Float
          parseFloat() -- static method
      java/io/PrintStream
          print(String)
          print(int)
          print(float)
          print(double)
          println ... same argument types as for print
      java/lang/String
          charAt(int)
          length()
          
      StringBuilder methods are passed on to StringBuilder.c for handling

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "ClassFileFormat.h"
#include "jvm.h"
#include "InterpretLoop.h"
#include "MyAlloc.h"
#include "TraceOptions.h"
#include "StringBuilder.h"
#include "NativeClasses.h"


void MissingClassVirtualMethod( char *className, char *methodName, char *methodDescr ) {
    if (strcmp(className,"java/io/PrintStream") == 0) {  /* fake the method invocation */
        if (strcmp(methodName,"println") == 0 || strcmp(methodName,"print") == 0) {
            if (strcmp(methodDescr,"(Ljava/lang/String;)V") == 0) {
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *arg = REAL_HEAP_POINTER(hp);
                printf("%s",arg->sval);
            } else if (strcmp(methodDescr,"(I)V") == 0) {
                int i = JVM_Pop();
                printf("%d",i);
            } else if (strcmp(methodDescr,"(F)V") == 0) {
                float f = JVM_PopFloat();
                printf("%f",f);
            } else if (strcmp(methodDescr,"(D)V") == 0) {
                union { struct { uint32_t v0; uint32_t v1; } ss; double d; } pair;
                pair.ss.v1 = JVM_Pop();
                pair.ss.v0 = JVM_Pop();
                printf("%lf", pair.d);
           } else if (strcmp(methodDescr, "()V") == 0 && strcmp(methodName,"println") == 0) {
                // nothing -- the newline will be output below    
           } else {
                printf("%s with signature %s not implemented\n",
                    methodName, methodDescr);
                JVM_Pop();
            }
            if (strcmp(methodName,"println") == 0)
                putchar('\n');
        } else {
            fprintf(stderr, "Method %s.%s with signature %s is unsupported\n",
                className, methodName, methodDescr);
            exit(1);
        }
        JVM_Pop();  /* there was an object ref on the stack */
        return;
    }
    if (strcmp(className, "java/lang/String") == 0) {
        if (strcmp(methodName,"charAt") == 0) {
            if (strcmp(methodDescr,"(I)C") == 0) {
                int ix = JVM_Pop();
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *sp = REAL_HEAP_POINTER(hp);
                char *s = sp->sval;
                if (ix < 0 || ix >= strlen(s))
                    throwExceptionExternal("StringIndexOutOfBoundsException",
                        methodName, className);
                JVM_Push(s[ix]);
                return;
            }
        }
        if (strcmp(methodName,"length") == 0) {
            if (strcmp(methodDescr,"()I") == 0) {
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *sp = REAL_HEAP_POINTER(hp);
                JVM_Push(strlen(sp->sval));
                return;
            }
        }
    }
    if (strcmp(className,"java/lang/Object") == 0) {
        if (strcmp(methodName,"<init>") == 0) {
            // no initialization to perform!
            JVM_Pop();  /* there was an object ref on the stack */
            return;
        }
        fprintf(stderr, "Method %s.%s with signature %s is unsupported\n",
            className, methodName, methodDescr);
        exit(1);
    }
    if (strcmp(className, StringBuilderName) == 0) {
        StringBuilderClass(methodName, methodDescr);
        return;
    }
    fprintf(stderr, "Class %s is missing or unsupported (invoked method = %s)\n",
        className, methodName);
    exit(1);
}


void MissingClassStaticMethod( char *className, char *methodName, char *methodDescr ) {
    union { int64_t lval;  double dval;  int32_t ival[2];  uint32_t uval[2]; } pair;

    if (strcmp(className,"java/lang/System") == 0) {
        if (strcmp(methodName,"gc") == 0) {
            if (strcmp(methodDescr,"()V") == 0) {
                gc();
                return;
            }
        } else {
            fprintf(stderr, "Static method %s.%s with signature %s is unsupported\n",
                className, methodName, methodDescr);
            exit(1);
        }
    }
    if (strcmp(className,"java/lang/Integer") == 0) {
        if (strcmp(methodName,"parseInt") == 0) {
            if (strcmp(methodDescr,"(Ljava/lang/String;)I") == 0) {
                int ival = 0;
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *arg = REAL_HEAP_POINTER(hp);
                if (sscanf(arg->sval, "%d", &ival) < 1)
                    throwExceptionExternal( "NumberFormatException", methodName, className );
                JVM_Push( (uint32_t)ival );
                return;
            }
        }
    }
    if (strcmp(className,"java/lang/Double") == 0) {
        if (strcmp(methodName,"parseDouble") == 0) {
            if (strcmp(methodDescr,"(Ljava/lang/String;)D") == 0) {
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *arg = REAL_HEAP_POINTER(hp);
                if (sscanf(arg->sval, "%lg", &pair.dval) < 1)
                    throwExceptionExternal( "NumberFormatException", methodName, className );
                JVM_Push(pair.uval[0]);
                JVM_Push(pair.uval[1]);
                return;
            }
        }
    }
    if (strcmp(className,"java/lang/Float") == 0) {
        if (strcmp(methodName,"parseFloat") == 0) {
            if (strcmp(methodDescr,"(Ljava/lang/String;)F") == 0) {
                float fval = 0.0;
                HeapPointer hp = JVM_Pop();
                if (hp == NULL_HEAP_REFERENCE)
                    throwExceptionExternal("NullPointerException", methodName, className);
                StringInstance *arg = REAL_HEAP_POINTER(hp);
                if (sscanf(arg->sval, "%f", &fval) < 1)
                    throwExceptionExternal( "NumberFormatException", methodName, className );
                JVM_PushFloat(fval);
                return;
            }
        }
    }
    fprintf(stderr, "Static method %s.%s with signature %s is missing or unsupported\n",
        className, methodName, methodDescr);
    exit(1);
}

