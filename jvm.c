/* jvm.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   /* for the definition of sbrk */
#include <assert.h>
#include <inttypes.h>
#include "ClassFileFormat.h"
#include "ReadClassFile.h"
#include "TraceOptions.h"
#include "MyAlloc.h"
#include "jvm.h"

DataItem *JVM_Stack;          /* ptr to base of stack */
DataItem *JVM_Top;            /* ptr to current top element on stack */
DataItem *JVM_StackLimit;     /* ptr to end of storage for stack */
int JVM_StackSize;            /* size of stack area, as # of elements */
void *Fake_System_Out;        /* pretends to be the java/lang/System.out value */


void JVM_Init( int stackSize ) {
    ClassInstance *x;
    JVM_StackSize = stackSize;
    JVM_Stack = SafeCalloc(JVM_StackSize, sizeof(DataItem));
    JVM_StackLimit = JVM_Stack + JVM_StackSize - 1;
    JVM_Top = JVM_Stack;
    JVM_Top->uval = UNINIT_PATTERN;  /* fake item on bottom of stack */
    x = MyHeapAlloc(sizeof(ClassInstance));
    Fake_System_Out = x;
    x->kind = CODE_INST;
    x->thisClass = NULL;
    x->instField[0].uval = 0;
}

void JVM_Push( uint32_t x ) {
    if (JVM_Top >= JVM_StackLimit) {
        fprintf(stderr, "stack overflow, execution must end\n");
        exit(1);
    }
    if (tracingExecution & TRACE_STACK)  // PRIX32 is defined in inttypes.h
        printf("push 0x%" PRIX32 " onto stack; new height = %d\n",
            (int)x, (int)(JVM_Top-JVM_Stack+1));
    (++JVM_Top)->uval = x;
}

void JVM_PushFloat( float x ) {
    if (JVM_Top >= JVM_StackLimit) {
        fprintf(stderr, "stack overflow, execution must end\n");
        exit(1);
    }
    if (tracingExecution & TRACE_STACK)
        printf("push %f onto stack; new height = %d\n",
            x, (int)(JVM_Top-JVM_Stack+1));
    (++JVM_Top)->fval = x;
}

void JVM_PushReference( HeapPointer x ) {
    if (JVM_Top >= JVM_StackLimit) {
        fprintf(stderr, "stack overflow, execution must end\n");
        exit(1);
    }
    assert(x >= 0 && x < MaxHeapPtr);
    if (tracingExecution & TRACE_STACK)  // PRIX32 is defined in inttypes.h
        printf("push heap reference 0x%" PRIX32 " onto stack; new height = %d\n",
            x, (int)(JVM_Top-JVM_Stack+1));
    (++JVM_Top)->pval = x;
}

uint32_t JVM_Pop() {
    if (JVM_Top <= JVM_Stack) {
        fprintf(stderr, "stack underflow, execution terminated\n");
        exit(1);
    }
    if (tracingExecution & TRACE_STACK)  // PRIX32 is defined in inttypes.h
        printf("pop 0x%" PRIX32 " from stack; new height = %d\n",
            JVM_Top->uval, (int)(JVM_Top-JVM_Stack-1));
    return (JVM_Top--)->uval;
}

float JVM_PopFloat() {
    if (JVM_Top <= JVM_Stack) {
        fprintf(stderr, "stack underflow, execution terminated\n");
        exit(1);
    }
    if (tracingExecution & TRACE_STACK)
        printf("pop %f from stack; new height = %d\n",
            JVM_Top->fval, (int)(JVM_Top-JVM_Stack-1));
    return (JVM_Top--)->fval;
}

HeapPointer JVM_PopReference() {
    if (JVM_Top <= JVM_Stack) {
        fprintf(stderr, "stack underflow, execution terminated\n");
        exit(1);
    }
    if (tracingExecution & TRACE_STACK)  // PRIX32 is defined in inttypes.h
        printf("pop heap reference 0x%" PRIX32 " from stack; new height = %d\n",
            JVM_Top->pval, (int)(JVM_Top-JVM_Stack-1));
    HeapPointer result = (JVM_Top--)->pval;
    assert( result >= 0 && result < MaxHeapPtr);
    return result;
}


