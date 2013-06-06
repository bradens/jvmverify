/* StringBuilder.c */

/*
    Handles some of the more frequently used methods of the
    java/lang/StringBuilder class.
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

char *StringBuilderName = "java/lang/StringBuilder";

// forward declaration
static void sbAppend( char *sval );


// Allocate a new instance of StringBuilder on the heap
ClassInstance *NewStringBuilderInstance() {
    StringBuilderInstance *sbi = MyHeapAlloc(sizeof(StringBuilderInstance));
    sbi->kind = CODE_SBLD;
    return (ClassInstance*)sbi;
}


// Handle the instance methods
void StringBuilderClass( char *methodName, char *methodDescr ) {
    HeapPointer hp;
    char buffer[32];

    if (strcmp(methodName,"<init>") == 0) {
        if (strcmp(methodDescr,"()V") == 0) {
            hp = JVM_Pop();
            if (hp == NULL_HEAP_REFERENCE)
                throwExceptionExternal("NullPointerException", methodName, StringBuilderName);
            StringBuilderInstance *sb = REAL_HEAP_POINTER(hp);
            sb->capacity = 64;  // could be any number!
            sb->buffer = SafeMalloc(sb->capacity);
            sb->len = 0;
            return;
        }
        // could add support for more constructors here
    }
    if (strcmp(methodName,"append") == 0) {
        if (strcmp(methodDescr,"(Ljava/lang/String;)Ljava/lang/StringBuilder;") == 0) {
            hp = JVM_Pop();
            if (hp == NULL_HEAP_REFERENCE)
                throwExceptionExternal("NullPointerException", methodName, StringBuilderName);
            StringInstance *sp = REAL_HEAP_POINTER(hp);
            sbAppend(sp->sval);
            return;
        }
        if (strcmp(methodDescr,"(I)Ljava/lang/StringBuilder;") == 0) {
            int32_t ival = JVM_Pop();
            sprintf(buffer, "%d", ival);
            sbAppend(buffer);
            return;
        }
        if (strcmp(methodDescr,"(F)Ljava/lang/StringBuilder;") == 0) {
            float fval = JVM_PopFloat();
            sprintf(buffer, "%f", fval);
            sbAppend(buffer);
            return;
        }
        if (strcmp(methodDescr,"(D)Ljava/lang/StringBuilder;") == 0) {
            union { struct { uint32_t v0; uint32_t v1; } ss; double d; } pair;
            pair.ss.v1 = JVM_Pop();
            pair.ss.v0 = JVM_Pop();
            sprintf(buffer, "%lf", pair.d);
            sbAppend(buffer);
            return;
        }
        if (strcmp(methodDescr,"(C)Ljava/lang/StringBuilder") == 0) {
            int32_t ival = JVM_Pop();
            buffer[0] = (char)ival;  buffer[1] = 0;
            sbAppend(buffer);
            return;
        }
        // could add support for appending more datatypes here
    }
    if (strcmp(methodName,"toString") == 0 && strcmp(methodDescr,"()Ljava/lang/String;") == 0) {
        StringBuilderInstance *sbi;
        HeapPointer hp = JVM_Pop();
        if (hp == NULL_HEAP_REFERENCE)
            throwExceptionExternal("NullPointerException", methodName, StringBuilderName);
        sbi = REAL_HEAP_POINTER(hp);
        StringInstance *sp = MyHeapAlloc(sizeof(StringInstance));
        sp->kind = CODE_STRG;
        sp->sval = SafeMalloc(sbi->len+1);
        memcpy(sp->sval, sbi->buffer, sbi->len+1);
        hp = MAKE_HEAP_REFERENCE(sp);
        JVM_Push(hp);
        return;
    }
    fprintf(stderr, "%s.%s with signature %s is unsupported\n",
        StringBuilderName, methodName, methodDescr);
    exit(1);
}


// apopends the sval string onto the current StringBuilder contents.
static void sbAppend( char *sval ) {
    StringBuilderInstance *sbi;
    HeapPointer hp = JVM_Pop();
    if (hp == NULL_HEAP_REFERENCE)
        throwExceptionExternal("NullPointerException", "append", StringBuilderName);
    sbi = REAL_HEAP_POINTER(hp);
    int slen = sval==NULL? 0 : strlen(sval);
    if (slen > 0) {
        if (sbi->len + slen >= sbi->capacity) {
            // need to expand the buffer
            sbi->capacity = sbi->len + slen + 30;
            char *newBuffer = SafeMalloc(sbi->capacity);
            memcpy(newBuffer,sbi->buffer,sbi->len);
            SafeFree(sbi->buffer);
            sbi->buffer = newBuffer;
        }
        memcpy(sbi->buffer+sbi->len, sval, slen);
        sbi->len += slen;
        sbi->buffer[sbi->len] = 0;  // make sure there's a string terminator
    }
    JVM_Push(hp);
}
