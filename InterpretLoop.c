/* InterpretLoop.c */

/*
   The interpreter for JVM bytecode instructions.

   The bytecode is in exactly the same format as in the class file on disk.
   That is, no preprocessing of the bytecode has been performed.  The
   implication is that interpretation is much slower than it would be
   in a production Java interpreter. 

   The list of JVM opcodes and their descriptions were copied in 2010 from
      http://en.wikipedia.org/wiki/Java_bytecode_instruction_listings
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>

#include "ClassFileFormat.h"
#include "jvm.h"
#include "PrintByteCode.h"
#include "TraceOptions.h"
#include "ClassResolver.h"
#include "StringBuilder.h"
#include "MyAlloc.h"
#include "InterpretLoop.h"


/* Exception handling is unimplemented, so we halt the program */
void throwException( char *kind, uint8_t *pc, method_info *meth, ClassType *ct ) {
    int pcOffset = pc - 1 - meth->code;
    fprintf(stderr, "Exception %s thrown at offset %d in method %s of class %s\n",
    	kind, pcOffset,
    	GetCPItemAsString(ct->cf, meth->name_index),
    	GetCPItemAsString(ct->cf, ct->cf->this_class));
    exit(1);
}


void throwExceptionExternal( char *kind, char *methodname, char *className ) {
    fprintf(stderr, "Exception %s thrown in method %s of class %s\n",
        kind, methodname, className);
    exit(1);
}


/* get a 4 byte signed integer from the bytecode array
   pcp is a reference to the pc variable of the caller.  */
static int32_t iget4( uint8_t **pcp ) {
    uint8_t *pc = *pcp;
    int32_t result = (pc[0]<<24) + (pc[1]<<16) + (pc[2]<<8) + pc[3];
    *pcp = pc + 4;
    return result;
}

/* get a 2 byte signed integer from the bytecode array
   pcp is a reference to the pc variable of the caller.  */
static int16_t iget2( uint8_t **pcp ) {
    uint8_t *pc = *pcp;
    int16_t result = (pc[0]<<8) + pc[1];
    *pcp = pc + 2;
    return result;
}

/* get a 2 byte unsigned integer from the bytecode array
   pcp is a reference to the pc variable of the caller.  */
static uint16_t uget2( uint8_t **pcp ) {
    uint8_t *pc = *pcp;
    uint16_t result = (pc[0]<<8) + pc[1];
    *pcp = pc + 2;
    return result;
}


/* implements the JVM ldc instruction (but also may be used during
   initialization of a class's static fields).
   i is the index in the constant pool belonging to class ct
   of the constant to push onto the stack.
*/
void  PushConstant( ClassType *thisClass, int i ) {
    StringInstance *p;
    char *s;
    ConstantPoolItem *cpi = &thisClass->cf->cp_item[i];

    switch(thisClass->cf->cp_tag[i]) {
    case CP_Integer:
        JVM_Push(cpi->ival);
        break;
    case CP_Float:
        JVM_PushFloat(cpi->fval);
        break;
    case CP_String:
        s = GetCPItemAsString(thisClass->cf,i);
        p = MyHeapAlloc(sizeof(StringInstance));
        p->kind = CODE_STRG;
        p->sval = s;
        JVM_PushReference(MAKE_HEAP_REFERENCE(p));
        break;
    default:
        fprintf(stderr, "invalid index into constant pool for PushConstant\n");
        exit(1);
    }
}

/* Execute the bytecode for method, which belongs to the class referenced
   by thisClass.
   The localVariable parameter references variable #0 of the method on the
   stack (which would be the first argument if the method has arguments).
   The function returns a result which specifies how many stack slots
   are needed for the method's returned value, i.e. 0 for a void method,
   2 for a method which returns a double or long, and otherwise 1 */
int InterpretMethod( ClassType *thisClass, method_info *method, DataItem *localVariable ) {
    uint8_t *opcodeAddr, *code, *pc;
    int i, j, dflt, offset, anIntValue, npairs, low, high;
    ClassType *aClassType;
    ClassInstance *aClassInstance;
    ConstantPoolItem *aConstPoolItem;
    ArrayOfRef *arr;
    ArrayOfSimple *arrSimple;
    HeapPointer aHeapReference, anotherHeapRef;
    double doubleVal;
    float floatVal;
    int16_t shortVal;
    int8_t signedByteVal;
    int64_t longVal;
    uint32_t  u;
    union { int64_t lval;  double dval;  int32_t ival[2];  uint32_t uval[2]; } pair;

	pc = code = method->code;
    for( ; ; ) {
        uint8_t op = *pc++;
        if (tracingExecution & TRACE_OPS)
            fprintf(stdout, "%d: %s\n", (int)(pc-1-code), GetOpcodeName(op));
        switch(op) {
        case OP_aaload:
            /*  arrayref, index --> value
                loads onto the stack a reference from an array */
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE) /* NullPointerException */
                throwException("NullPointerException",pc,method,thisClass);
            arr = REAL_HEAP_POINTER(aHeapReference);
            if (i<0 || i>=arr->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arr = REAL_HEAP_POINTER(JVM_Top->pval);
            JVM_Top->pval = arr->elements[i];
            break;
        case OP_aastore:
            /*  arrayref, index, value -->
                stores a reference into an array */
            anotherHeapRef = JVM_PopReference();
            i = JVM_Pop();
            aHeapReference = JVM_PopReference();
            if (aHeapReference == NULL_HEAP_REFERENCE) /* NullPointerException */
                throwException("NullPointerException",pc,method,thisClass);
            arr = REAL_HEAP_POINTER(aHeapReference);
            if (i<0 || i>=arr->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arr->elements[i] = anotherHeapRef;
            break;
        case OP_aconst_null:
            /*  --> null 	pushes a null reference onto the stack */
            JVM_PushReference(NULL_HEAP_REFERENCE);
            break;
        case OP_aload:  /* index */
            /*  --> objectref
                loads a reference onto the stack from a local variable #index */
            i = *pc++;
            JVM_PushReference(localVariable[i].pval);
            break;
        case OP_aload_0:
        case OP_aload_1:
        case OP_aload_2:
        case OP_aload_3:
            /*  --> objectref
                loads a reference onto the stack from local variable 0/1/2/3 */
            JVM_PushReference(localVariable[op-OP_aload_0].pval);
            break;
        case OP_anewarray:  /*  indexbyte1, indexbyte2  */
            /*  count --> arrayref
            	creates a new array of references of length count
                and component type identified by the class reference index
                (indexbyte1 << 8 + indexbyte2) in the constant pool */
            i = uget2(&pc);
            aClassType = ResolveClassReference(thisClass,i);
            i = JVM_Top->ival;
            if (i < 0)
                throwException("NegativeArraySizeException",pc,method,thisClass);
            arr = MyHeapAlloc(sizeof(ArrayOfRef)+(i-1)*4);
            arr->kind = CODE_ARRA;
            arr->size = i;
            // handle built-in types (eg String) where aClassType is NULL
            arr->classRef = (aClassType==NULL)? NULL_HEAP_REFERENCE : MAKE_HEAP_REFERENCE(aClassType);
            JVM_Top->pval = MAKE_HEAP_REFERENCE(arr);
            break;
        case OP_areturn:
            /*  objectref --> [empty] 	returns a reference from a method */
            localVariable[0].pval = JVM_PopReference();
            return 1;
        case OP_arraylength:
            /*  arrayref --> length 	gets the length of an array */
            arr = REAL_HEAP_POINTER(JVM_Top->pval);
            JVM_Top->ival = arr->size;
            break;
        case OP_astore:  /* index */
            /*  objectref --> 	stores a reference into a local variable #index */
            i = *pc++;
            localVariable[i].pval = JVM_PopReference();
            break;
        case OP_astore_0:
        case OP_astore_1:
        case OP_astore_2:
        case OP_astore_3:
            /*  objectref --> 	stores a reference into local variable 0/1/2/3 */
            localVariable[op-OP_astore_0].pval = JVM_PopReference();
            break;
        case OP_athrow:
            /*  objectref --> [empty], objectref
                throws an error or exception (notice that the rest of the
                stack is cleared, leaving only a reference to the Throwable) */
            throwException("???",pc,method,thisClass);
            break;
        case OP_baload:
        case OP_caload:
            /*  arrayref, index --> value
                loads a byte or Boolean value from an array
                loads a char from an array */
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            JVM_Top->ival = arrSimple->u.bval[i];
            break;
        case OP_bastore:
        case OP_castore:
            /*  arrayref, index, value -->
                stores a byte / Boolean / char value into an array */
            anIntValue = JVM_Pop();
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.bval[i] = anIntValue;
            break;
        case OP_bipush:  /*  byte  */
            /*  --> value 	pushes a byte onto the stack as an integer value */
            i = (signed char)(*pc++);
            JVM_Push(i);
            break;
        case OP_checkcast:  /*  indexbyte1, indexbyte2  */
            /*  objectref --> objectref 
                should check whether an objectref is of a certain type,
                the class reference of which is in the constant pool at index
                (indexbyte1 << 8 + indexbyte2) */
            i = iget2(&pc);
            aClassType = ResolveClassReference(thisClass, i);
            /* however the check is unimplemented and ignored -- we assume the
               check succeeds
            */
            break;
        case OP_d2f:
            /*  value --> result 	converts a double to a float */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            floatVal = pair.dval;
            JVM_Push((uint32_t)floatVal);            
            break;
        case OP_d2i:
            /*  value --> result 	converts a double to an int */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Top->uval;
            i = pair.dval;
            JVM_Top->ival = i;
            break;
        case OP_d2l:
            /*  value --> result 	converts a double to a long */
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval = pair.dval;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_dadd:
            /*  value1, value2 --> result 	adds two doubles */
            /*  value1, value2 --> result 	divides two doubles */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            doubleVal = pair.dval;  /* this is value 2 */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.dval = pair.dval + doubleVal;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_daload:
            /*  arrayref, index --> value 	loads a double from an array */
            i = JVM_Top->ival;
            aHeapReference = (JVM_Top-1)->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            pair.dval = arrSimple->u.dval[i];
            (JVM_Top-1)->uval = pair.uval[0];
            JVM_Top->uval     = pair.uval[1];
            break;
        case OP_dastore:
            /*  arrayref, index, value --> 	stores a double into an array */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            i = JVM_Top->ival;
            aHeapReference = (JVM_Top-1)->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.dval[i] = pair.dval;
            break;
        case OP_dcmpg:
        case OP_dcmpl:
            /*  value1, value2 --> result 	compares two doubles */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            doubleVal = pair.dval;  // doubleVal is value 2
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();  // pair.dval is value 1
            if (isnan(doubleVal) || isnan(pair.dval))
                u = (op == OP_dcmpg)? 1 : -1;
            else
                u = (pair.dval == doubleVal)? 0 : (pair.dval > doubleVal)? 1 : -1;  // bugfix: 5/6/10
            JVM_Push(u);
            break;
        case OP_dconst_0:
            /*  --> 0.0 	pushes the constant 0.0 onto the stack */
            pair.dval = 0.0;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_dconst_1:
            /*  --> 1.0 	pushes the constant 1.0 onto the stack */
            pair.dval = 1.0;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_ddiv:
            /*  value1, value2 --> result 	divides two doubles */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            doubleVal = pair.dval;  /* this is value 2 */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.dval = pair.dval / doubleVal;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_dload :  /* index */
            /*  --> value 	loads a double value from a local variable #index */
            i = *pc++;
            JVM_Push(localVariable[i].uval);
            JVM_Push(localVariable[i+1].uval);            
            break;
        case OP_dload_0:
        case OP_dload_1:
        case OP_dload_2:
        case OP_dload_3 :
            /*  --> value 	loads a double from local variable 0/1/2/3 */
            JVM_Push(localVariable[op-OP_dload_0].uval);
            JVM_Push(localVariable[op-OP_dload_0+1].uval);     
            break;
        case OP_dmul:
            /*  value1, value2 --> result 	multiplies two doubles */
            /*  value1, value2 --> result 	divides two doubles */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            doubleVal = pair.dval;  /* this is value 2 */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.dval = pair.dval * doubleVal;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_dneg:
            /*  value --> result 	negates a double */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.dval = -pair.dval;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_drem:
            /*  value1, value2 --> result
                gets the remainder from a division between two doubles */
            fprintf(stderr, "unimplemented op: drem");
            JVM_Pop();
            break;
        case OP_dreturn:
            /*  value --> [empty] 	returns a double from a method */
            // the return value is left on the stack
            return 2;
        case OP_dstore:  /*  index  */
            /*  value -->
                stores a double value into a local variable #index */
            i = *pc++;
            localVariable[i+1].uval = JVM_Pop();
            localVariable[i].uval   = JVM_Pop();
            break;
        case OP_dstore_0:
        case OP_dstore_1:
        case OP_dstore_2:
        case OP_dstore_3:
            /*  value --> 	stores a double into local variable 0/1/2/3 */
            localVariable[op-OP_dstore_0+1].uval = JVM_Pop();
            localVariable[op-OP_dstore_0].uval = JVM_Pop();
            break;
        case OP_dsub:
            /*  value1, value2 --> result 	subtracts a double from another */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            doubleVal = pair.dval;  /* this is value 2 */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.dval = pair.dval - doubleVal;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_dup:
            /*  value --> value, value
                duplicates the value on top of the stack */
            JVM_Push(JVM_Top->uval);
            break;
        case OP_dup_x1:
            /*  value2, value1 --> value1, value2, value1  */
            JVM_Push(JVM_Top->uval);
            (JVM_Top-1)->uval = (JVM_Top-2)->uval;
            (JVM_Top-2)->uval = JVM_Top->uval;
            break;
        case OP_dup_x2:
            /*  value3, value2, value1 --> value1, value3, value2, value1  */
            JVM_Push(JVM_Top->uval);
            (JVM_Top-1)->uval = (JVM_Top-2)->uval;
            (JVM_Top-2)->uval = (JVM_Top-3)->uval;
            (JVM_Top-3)->uval = JVM_Top->uval;
            break;
        case OP_dup2:
            /*  {value2, value1} --> {value2, value1}, {value2, value1}  */
            JVM_Push((JVM_Top-1)->uval);
            JVM_Push((JVM_Top-1)->uval);
            break;
        case OP_dup2_x1:
            /*  value3, {value2, value1} --> {value2, value1}, value3, {value2, value1}   */
            JVM_Push((JVM_Top-1)->uval);
            JVM_Push((JVM_Top-1)->uval);
            (JVM_Top-2)->uval = (JVM_Top-4)->uval;
            (JVM_Top-3)->uval = JVM_Top->uval;
            (JVM_Top-4)->uval = (JVM_Top-1)->uval;
            break;
        case OP_dup2_x2:
            /*  {value4, value3}, {value2, value1} --> {value2, value1},
                                      {value4, value3}, {value2, value1}  */
            JVM_Push((JVM_Top-1)->uval);
            JVM_Push((JVM_Top-1)->uval);
            (JVM_Top-2)->uval = (JVM_Top-4)->uval;
            (JVM_Top-3)->uval = (JVM_Top-5)->uval;
            (JVM_Top-4)->uval = JVM_Top->uval;
            (JVM_Top-5)->uval = (JVM_Top-1)->uval;
            break;
        case OP_f2d:
            /*  value --> result 	converts a float to a double */
            pair.dval = (double)JVM_Top->fval;
            JVM_Top->uval = pair.uval[0];
            JVM_Push(pair.uval[1]);
            break;
        case OP_f2i:
            /*  value --> result 	converts a float to an int */
            JVM_Top->ival = JVM_Top->fval;
            break;
        case OP_f2l:
            /*  value --> result 	converts a float to a long */
            pair.lval = JVM_Top->fval;
            JVM_Top->uval = pair.uval[0];
            JVM_Push(pair.uval[1]);
            break;
        case OP_fadd:
            /*  value1, value2 --> result 	adds two floats */
            floatVal = JVM_PopFloat();
            JVM_Top->fval += floatVal;
            break;
        case OP_faload:
            /*  arrayref, index --> value 	loads a float from an array */
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            JVM_Top->fval = arrSimple->u.fval[i];
            break;
        case OP_fastore:
            /*  arreyref, index, value --> 	stores a float in an array */
            floatVal = JVM_PopFloat();
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.fval[i] = floatVal;
            break;
        case OP_fcmpg:
        case OP_fcmpl:
            /*  value1, value2 --> result 	compares two floats */
            floatVal = JVM_PopFloat();  // floatVal is value 2
            if (isnan(floatVal) || isnan(JVM_Top->fval))
                JVM_Top->uval = (op == OP_fcmpg)? 1 : -1;
            else
                JVM_Top->uval = (JVM_Top->fval == floatVal)? 0 : (JVM_Top->fval > floatVal)? 1 : -1;  // bugfix: 5/6/10
            break;
        case OP_fconst_0 :
        case OP_fconst_1:
        case OP_fconst_2:
            /*  --> value 	pushes 0.0 / 1.0 / 2.0 on the stack */
            JVM_PushFloat((op-OP_fconst_0)*1.0);
            break;
        case OP_fdiv:
            /*  value1, value2 --> result 	divides two floats */
            floatVal = JVM_PopFloat();
            JVM_Top->fval /= floatVal;
            break;
        case OP_fload:  /*  index  */
            /*  index 	--> value 	loads a float value from a local variable #index */
            i = *pc++;
            JVM_PushFloat(localVariable[i].fval);
            break;
        case OP_fload_0:
        case OP_fload_1:
        case OP_fload_2:
        case OP_fload_3:
            /*  --> value 	loads a float value from local variable 0/1/2/3 */
            JVM_PushFloat(localVariable[op-OP_fload_0].fval);
            break;
        case OP_fmul:
            /*  value1, value2 --> result 	multiplies two floats */
            floatVal = JVM_PopFloat();
            JVM_Top->fval *= floatVal;
            break;
        case OP_fneg :
            /*  value --> result 	negates a float */
            JVM_Top->fval = - JVM_Top->fval;
            break;
        case OP_frem:
            /*  value1, value2 --> result
                gets the remainder from a division between two floats */
            fprintf(stderr,"unimplemented op: frem\n");
            JVM_Pop();
            break;
        case OP_freturn:
            /*  value --> [empty] 	returns a float */
            // the return value is left on the stack
            return 1;
        case OP_fstore:  /* index */
            /*  value -->
                stores a float value into a local variable #index */
            i = *pc++;
            localVariable[i].fval = JVM_PopFloat();
            break;
        case OP_fstore_0:
        case OP_fstore_1:
        case OP_fstore_2:
        case OP_fstore_3:
            /*  value --> 	stores a float value into local variable 0/1/2/3 */
            localVariable[op-OP_fstore_0].fval = JVM_PopFloat();
            break;
        case OP_fsub:
            /*  value1, value2 --> result 	subtracts two floats */
            floatVal = JVM_PopFloat();
            JVM_Top->fval -= floatVal;
            break;
        case OP_getfield:
            /*  index1, index2 	objectref --> value
                gets a field value of an object objectref, where the field
                is identified by field reference index in the constant pool */
            i = uget2(&pc);
            if (!GetField(thisClass,i))
                throwException("IllegalAccessError",pc-2,method,thisClass);
            break;
        case OP_getstatic:
            /*  index1, index2 	--> value 
                gets a static field value of a class, where the field is
                identified by field reference in the constant pool index */
            i = uget2(&pc);
            if (!GetStatic(thisClass,i))
                throwException("IllegalAccessError",pc-2,method,thisClass);
            break;
        case OP_goto:
            /*  branchbyte1, branchbyte2 	[no change]
                goes to another instruction at branchoffset */
            shortVal = iget2(&pc);
            pc = (pc - 3) + shortVal;
            break;
        case OP_goto_w :
            /*  branchbyte1, branchbyte2, branchbyte3, branchbyte4 	[no change]
                goes to another instruction at branchoffset */
            offset = iget4(&pc);
            pc = (pc-5) + offset;
            break;
        case OP_i2b:
            /*  value --> result 	converts an int into a byte */
            JVM_Top->ival = (JVM_Top->ival << 24) >> 24;
            break;
        case OP_i2c:
            /*  value --> result 	converts an int into a character */
            JVM_Top->ival = (JVM_Top->ival) & 0xff;
            break;
        case OP_i2d:
            /*  value --> result 	converts an int into a double */
            pair.dval = JVM_Top->ival * 1.0;
            JVM_Top->uval = pair.uval[0];
            JVM_Push(pair.uval[1]);
            break;
        case OP_i2f:
            /*  value --> result 	converts an int into a float */
            JVM_Top->fval = JVM_Top->ival * 1.0;
            break;
        case OP_i2l:
            /*  value --> result 	converts an int into a long */
            pair.lval = JVM_Top->ival * 1L;
            JVM_Top->uval = pair.uval[0];
            JVM_Push(pair.uval[1]);
            break;
        case OP_i2s:
            /*  value --> result 	converts an int into a short */
            JVM_Top->ival = (JVM_Top->ival << 16) >> 16;
            break;
        case OP_iadd:
            /*  value1, value2 --> result 	adds two ints together */
            i = JVM_Pop();
            JVM_Top->ival += i;
            break;
        case OP_iaload:
            /*  arrayref, index --> value 	loads an int from an array */
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            JVM_Top->ival = arrSimple->u.ival[i];
            break;
        case OP_iand:
            /*  value1, value2 --> result 	performs a logical and on two integers */
            i = JVM_Pop();
            JVM_Top->ival &= i;
            break;
        case OP_iastore:
            /*  arrayref, index, value --> 	stores an int into an array */
            anIntValue = JVM_Pop();
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.ival[i] = anIntValue;
            break;
        case OP_iconst_m1:
        case OP_iconst_0:
        case OP_iconst_1:
        case OP_iconst_2:
        case OP_iconst_3:
        case OP_iconst_4:
        case OP_iconst_5:
            /*  --> -1/0/1/2/3/4/5 	loads the int value -1...5 onto the stack */
            JVM_Push(op-OP_iconst_0);
            break;
        case OP_idiv:
            /*  value1, value2 --> result 	divides two integers */
            i = JVM_Pop();
            JVM_Top->ival /= i;
            break;
        case OP_if_acmpeq:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 --> 
                if references are equal, branch to instruction at branchoffset */
            offset = uget2(&pc);
            u = JVM_Pop();
            if (JVM_Pop() == u)
                pc = (pc-3) + offset;
            break;
        case OP_if_acmpne:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if references are not equal, branch to instruction at branchoffset */
            offset = uget2(&pc);
            u = JVM_Pop();
            if (JVM_Pop() != u)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmpeq:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 --> 
                if value1 == value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();
            if (j == i)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmpne:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if value1 != value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();
            if (j != i)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmplt:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if value1 < value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();;
            if (j < i)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmpge:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if value1 >= value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();
            if (j >= i)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmpgt:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if value1 > value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();
            if (j > i)
                pc = (pc-3) + offset;
            break;
        case OP_if_icmple:  /*  branchbyte1, branchbyte2 */
            /*  value1, value2 -->
                if value1 <= value2, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            j = (int)JVM_Pop();
            if (j <= i)
                pc = (pc-3) + offset;
            break;
        case OP_ifeq:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value == 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            if (JVM_Pop() == 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifne:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value != 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            if (JVM_Pop() != 0)
                pc = (pc-3) + offset;
            break;
        case OP_iflt:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value < 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            if (i < 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifge:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value is >= 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            if (i >= 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifgt:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value > 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            if (i > 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifle:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value <= 0, branch to instruction at branchoffset */
            offset = uget2(&pc);
            i = (int)JVM_Pop();
            if (i <= 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifnonnull:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value is not null, branch to instruction at branchoffset */
            offset = uget2(&pc);
            if (JVM_Pop() != 0)
                pc = (pc-3) + offset;
            break;
        case OP_ifnull:  /*  branchbyte1, branchbyte2 */
            /*  value -->
                if value is null, branch to instruction at branchoffset */
            offset = uget2(&pc);
            if (JVM_Pop() == 0)
                pc = (pc-3) + offset;
            break;
        case OP_iinc:  /*  index, const  */
            /*  [No change]
                increment local variable #index by signed byte const */
            i = *pc++;
            signedByteVal = (int8_t)(*pc++);
            localVariable[i].ival += signedByteVal;
            break;
        case OP_iload:
            /*  index 	--> value
                loads an int value from a variable #index */
            i = *pc++;
            JVM_Push(localVariable[i].ival);
            break;
        case OP_iload_0:
        case OP_iload_1:
        case OP_iload_2:
        case OP_iload_3:
            /*  --> value 	loads an int value from variable 0/1/2/3 */
            JVM_Push(localVariable[op-OP_iload_0].ival);
            break;
        case OP_imul:
            /*  value1, value2 --> result 	multiply two integers */
            i = JVM_Pop();
            JVM_Top->ival *= i;
            break;
        case OP_ineg:
            /*  value --> result 	negate int */
            JVM_Top->ival = - JVM_Top->ival;
            break;
        case OP_instanceof:
            /*  indexbyte1, indexbyte2
            		objectref --> result
                determines if an object objectref is of a given type,
                identified by class reference index in constant pool */
            fprintf(stderr,"unimplemented op: instanceof\n");
            break;
        case OP_invokeinterface:
            /*  indexbyte1, indexbyte2, count, 0
            		objectref, [arg1, arg2, ...] -->
                invokes an interface method on object objectref, where the
                interface method is identified by method reference index in
                constant pool */
            fprintf(stderr,"unimplemented op: invokeinterface\n");
            break;
        case OP_invokespecial:  /*  indexbyte1, indexbyte2  */
            /*  objectref, [arg1, arg2, ...] -->
                invoke instance method on object objectref, where the method
                is identified by method reference index in constant pool */
            i = uget2(&pc);
            if (tracingExecution & TRACE_INVOKES) {
                char *s = GetCPItemAsString(thisClass->cf,i);
                fprintf(stdout, "    Invoking special method %s...\n", s);
                free(s);
            }
            InvokeSpecialMethod(thisClass,i);
            break;
        case OP_invokestatic:  /*  indexbyte1, indexbyte2  */
            /*  [arg1, arg2, ...] -->
                invoke a static method, where the method is identified by
                method reference index in constant pool */
            i = uget2(&pc);
            if (tracingExecution & TRACE_INVOKES) {
                char *s = GetCPItemAsString(thisClass->cf,i);
                fprintf(stdout, "    Invoking static method %s...\n", s);
                free(s);
            }
            InvokeStaticMethod(thisClass,i);
            break;
        case OP_invokevirtual:  /*  indexbyte1, indexbyte2 	*/
            /*  objectref, [arg1, arg2, ...] -->
                invoke virtual method on object objectref, where the method
                is identified by method reference index in constant pool */
            i = uget2(&pc);
            if (tracingExecution & TRACE_INVOKES) {
                char *s = GetCPItemAsString(thisClass->cf,i);
                fprintf(stdout, "    Invoking virtual method %s...\n", s);
                free(s);
            }
            InvokeVirtualMethod(thisClass,i);
            break;
        case OP_ior:
            /*  value1, value2 --> result 	logical int or */
            i = JVM_Pop();
            JVM_Top->ival |= i;
            break;
        case OP_irem:
            /*  value1, value2 --> result 	logical int remainder */
            i = JVM_Pop();
            JVM_Top->ival %= i;
            break;
        case OP_ireturn:
            /*  value --> [empty] 	returns an integer from a method */
            // the return value is left on the stack
            return 1;
        case OP_ishl:
            /*  value1, value2 --> result 	int shift left */
            i = JVM_Pop();
            JVM_Top->ival <<= i;
            break;
        case OP_ishr:
            /*  value1, value2 --> result 	int shift right */
            i = JVM_Pop();
            JVM_Top->ival >>= i;
            break;
        case OP_istore:  /*  index  */
            /*  value --> 	store int value into variable #index */
            i = *pc++;
            localVariable[i].ival = JVM_Pop();
            break;
        case OP_istore_0:
        case OP_istore_1:
        case OP_istore_2:
        case OP_istore_3:
            /*  value --> 	store int value into variable 0/1/2/3 */
            i = JVM_Pop();
            localVariable[op-OP_istore_0].ival = i;
            break;
        case OP_isub:
            /*  value1, value2 --> result 	int subtract */
            i = JVM_Pop();
            JVM_Top->ival -= i;
            break;
        case OP_iushr:
            /*  value1, value2 --> result 	int shift right */
            i = JVM_Pop();
            JVM_Top->ival >>= i;
            break;
        case OP_ixor:
            /*  value1, value2 --> result 	int xor */
            i = JVM_Pop();
            JVM_Top->ival ^= i;
            break;
        case OP_jsr:  /*  branchbyte1, branchbyte2  */
            /*  --> address
                jump to subroutine at branchoffset
                and place the return address on the stack */
            offset = iget2(&pc);
            JVM_Push(pc-code);
            pc = (pc-3) + offset;
            break;
        case OP_jsr_w:  /*  branchbyte1, branchbyte2, ... branchbyte4 */
            /* 	--> address 
                jump to subroutine at branchoffset
                and place the return address on the stack */
            offset = iget4(&pc);
            JVM_Push(pc-code);
            pc = (pc-3) + offset;
            break;
        case OP_l2d:
            /*  value --> result 	converts a long to a double */
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.dval = pair.lval;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_l2f:
            /*  value --> result 	converts a long to a float */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Top->uval;
            JVM_Top->fval = pair.lval * 1.0;
            break;
        case OP_l2i:
            /*  value --> result 	converts a long to an int */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Top->uval;
            JVM_Top->ival = pair.lval;
            break;
        case OP_ladd:
            /*  value1, value2 --> result 	add two longs */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval += longVal;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_laload:
            /*  arrayref, index --> value 	load a long from an array */
            i = JVM_Top->ival;
            aHeapReference = (JVM_Top-1)->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            pair.lval = arrSimple->u.lval[i];
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_land:
            /*  value1, value2 --> result 	bitwise and of two longs */
            i = JVM_Pop();
            (JVM_Top-1)->ival &= i;
            i = JVM_Pop();
            (JVM_Top-1)->ival &= i;
            break;
        case OP_lastore:
            /*  arrayref, index, value --> 	   store a long to an array */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            i = JVM_Top->ival;
            aHeapReference = (JVM_Top-1)->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.lval[i] = pair.lval;
            break;
        case OP_lcmp:
            /*  value1, value2 --> result 	compares two long values */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Top->uval;
            JVM_Top->uval = (pair.lval < longVal)? -1 : (pair.lval == longVal)? 0 : 1;
            break;
        case OP_lconst_0:
        case OP_lconst_1:
            /*  --> 0L 	pushes the long 0 / 1 onto the stack */
            pair.lval = (op-OP_lconst_0);
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_ldc:  /*  index  */
            /*  --> value 
                pushes a constant #index from a constant pool
                (String, int, float or class type) onto the stack */
            i = *pc++;
            PushConstant(thisClass,i);
            break;
        case OP_ldc_w:  /*  indexbyte1, indexbyte2  */
            /*  --> value 
                pushes a constant #index from a constant pool
                (String, int, float or class type) onto the stack */
            i = uget2(&pc);
            PushConstant(thisClass,i);
            break;
        case OP_ldc2_w:  /*  indexbyte1, indexbyte2  */
            /*  --> value
                pushes a constant #index from a constant pool
                (double or long) onto the stack */
            i = uget2(&pc);
            aConstPoolItem = &thisClass->cf->cp_item[i];
            JVM_Push(aConstPoolItem->uval);
            JVM_Push((aConstPoolItem+1)->uval);
            break;
        case OP_ldiv:
            /*  value1, value2 --> result 	divide two longs */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval /= longVal;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lload:
            /*  index 	--> value 	load a long value from a local variable #index */
            i = *pc++;
            JVM_Push(localVariable[i].uval);
            JVM_Push(localVariable[i+1].uval);
            break;
        case OP_lload_0:
        case OP_lload_1:
        case OP_lload_2:
        case OP_lload_3:
            /*  --> value 	load a long value from a local variable 0/1/2/3 */
            JVM_Push(localVariable[op-OP_lload_0].uval);
            JVM_Push(localVariable[op-OP_lload_0+1].uval);
            break;
        case OP_lmul:
            /*  value1, value2 --> result 	multiplies two longs */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval *= longVal;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lneg:
            /*  value --> result 	negates a long */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            pair.lval = -pair.lval;
            JVM_Push(pair.uval[0]);
            JVM_Push(pair.uval[1]);
            break;
        case OP_lookupswitch:
            /*  <0-3 bytes padding>, defaultbyte1, defaultbyte2, defaultbyte3,
                defaultbyte4, npairs1, npairs2, npairs3, npairs4, match-offset pairs...
                key -->
             	a target address is looked up from a table using a key
                and execution continues from the instruction at that address */
            opcodeAddr = pc-1;
            i = JVM_Pop();
            pc = code + ((pc - code + 3) & 0xFFFFFFFC);  /* account for padding bytes */
            dflt = iget4(&pc);
            npairs = iget4(&pc);
            while(npairs-- > 0) {
                int match = iget4(&pc);
                offset = iget4(&pc);
                if (match == i) break;
            }
            if (npairs < 0) offset = dflt;
            pc = opcodeAddr + offset;
            break;
        case OP_lor:
            /*  value1, value2 --> result 	bitwise or of two longs */
            /*  value1, value2 --> result 	bitwise and of two longs */
            i = JVM_Pop();
            (JVM_Top-1)->ival |= i;
            i = JVM_Pop();
            (JVM_Top-1)->ival |= i;
            break;
        case OP_lrem:
            /*  value1, value2 --> result 	remainder of division of two longs */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval %= longVal;
            JVM_Top->uval     = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lreturn:
            /*  value --> [empty] 	returns a long value */
            localVariable[1].uval = JVM_Pop();
            localVariable[0].uval = JVM_Pop();
            return 2;
        case OP_lshl:
            /*  value1, value2 --> result
                bitwise shift left of a long value1 by value2 positions */
            i = JVM_Pop();
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval <<= i;
            JVM_Top->uval     = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lshr:
            /*  value1, value2 --> result
                bitwise shift right of a long value1 by value2 positions */
            i = JVM_Pop();
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval >>= i;
            JVM_Top->uval     = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lstore:  /*  index  */
            /*  value -->
                store a long value in a local variable #index */
            i = *pc++;
            localVariable[i+1].uval = JVM_Pop();
            localVariable[i].uval   = JVM_Pop();
            break;
        case OP_lstore_0:
        case OP_lstore_1:
        case OP_lstore_2:
        case OP_lstore_3:
            /*  value --> 	store a long value in a local variable 0/1/2/3 */
            localVariable[op-OP_lstore_0+1].uval = JVM_Pop();
            localVariable[op-OP_lstore_0].uval = JVM_Pop();
            break;
        case OP_lsub:
            /*  value1, value2 --> result 	subtract two longs */
            pair.uval[1] = JVM_Pop();
            pair.uval[0] = JVM_Pop();
            longVal = pair.lval;
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval -= longVal;
            JVM_Top->uval = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lushr:
            /*  value1, value2 --> result 
                bitwise shift right of a long value1 by value2 positions, unsigned */
            i = JVM_Pop();
            pair.uval[1] = JVM_Top->uval;
            pair.uval[0] = (JVM_Top-1)->uval;
            pair.lval >>= i;
            JVM_Top->uval     = pair.uval[1];
            (JVM_Top-1)->uval = pair.uval[0];
            break;
        case OP_lxor:
            /*  value1, value2 --> result
                bitwise exclusive or of two longs */
            i = JVM_Pop();
            (JVM_Top-1)->ival ^= i;
            i = JVM_Pop();
            (JVM_Top-1)->ival ^= i;
            break;
        case OP_monitorenter:
            /*  objectref --> 
                enter monitor for object ("grab the lock"
                - start of synchronized() section) */
            fprintf(stderr,"unimplemented op: monitorenter\n");
            break;
        case OP_monitorexit:
            /*  objectref --> 	exit monitor for object
                ("release the lock" - end of synchronized() section) */
            fprintf(stderr,"unimplemented op: monitorexit\n");
            break;
        case OP_multianewarray:  /*  indexbyte1, indexbyte2, dimensions */
            /*  count1, [count2,...] --> arrayref
                create a new array of dimensions dimensions with elements
                of type identified by class reference in constant pool
                index; the size of each dimension is identified by
                count1, [count2, etc] */
            i = uget2(&pc);
            anIntValue = *pc++;
            fprintf(stderr,"unimplemented op: multianewarray\n");
            break;
        case OP_new:  /*  indexbyte1, indexbyte2  */
            /*  --> objectref 
                creates new object of type identified by class reference in
                constant pool at given index */
            i = uget2(&pc);
            aClassType = ResolveClassReference(thisClass,i);
            if (aClassType == NULL) {
                char *cn = GetCPItemAsString(thisClass->cf,i);
                if (strcmp(cn, "java/lang/StringBuilder") == 0)
                    aClassInstance = NewStringBuilderInstance();
                else {    
                    fprintf(stderr, "Cannot resolve reference to class %s "
                        "(while executing new op)\n", cn);
                    free(cn);
                    exit(1);
                }
				free(cn);
            } else {
                aClassInstance = MyHeapAlloc(sizeof(ClassInstance)+
                        (aClassType->numInstanceFields-1)*sizeof(DataItem));
                aClassInstance->kind = CODE_INST;
                aClassInstance->thisClass = aClassType;
            }
            JVM_PushReference(MAKE_HEAP_REFERENCE(aClassInstance));
            break;
        case OP_newarray:  /*  atype  */
            /*  count --> arrayref 
                creates new array with count elements of primitive type
                identified by atype */
            i = *pc++;
            anIntValue = JVM_Pop();
            if (anIntValue < 0)
                throwException("NegativeArraySizeException",pc,method,thisClass);
            switch(i) {
            case 4:   /* boolean elements */
            case 5:   /* char elements */
            case 8:   /* byte elements */
                shortVal = 1;
                break;
            case 9:   /* short elements */
                shortVal = 2;
                break;
            case 6:   /* float elements */
            case 10:  /* int elements */
                shortVal = 4;
                break;
            case 7:   /* double elements */
            case 11:  /* long elements */
                shortVal = 8;
                break;
            }
            arrSimple = MyHeapAlloc(sizeof(ArrayOfSimple)+anIntValue*shortVal-8);
            arrSimple->kind = CODE_ARRS;
            arrSimple->size = anIntValue;
            arrSimple->typecode = i;
            arrSimple->elemSize = shortVal;
            aHeapReference = MAKE_HEAP_REFERENCE(arrSimple);
            JVM_PushReference(aHeapReference);
            break;
        case OP_nop:
            /*  [No change] 	performs no operation */
            break;
        case OP_pop:
            /*  value --> 	discards the top value on the stack */
            (void)JVM_Pop();
            break;
        case OP_pop2:
            /*  {value2, value1} -->
                discards the top two values on the stack (or one value,
                if it is a double or long) */
            (void)JVM_Pop();
            (void)JVM_Pop();
            break;
        case OP_putfield:  /*  indexbyte1, indexbyte2  */
            /*  objectref, value -->
                set field to value in an object objectref, where the field is
                identified by a field reference index in constant pool */
            i = uget2(&pc);
            if (!PutField(thisClass,i))
                throwException("IllegalAccessError",pc-2,method,thisClass);
            break;
        case OP_putstatic:  /*  indexbyte1, indexbyte2  */
            /*  value -->
                set static field to value in a class, where the field is
                identified by a field reference index in constant pool  */
            i = uget2(&pc);
            if (!PutStatic(thisClass,i))
                throwException("IllegalAccessError",pc-2,method,thisClass);
            break;
        case OP_ret:  /*  index  */
            /*  [No change]
                continue execution from address taken from a local variable
                #index (the asymmetry with jsr is intentional) */
            i = pc[0];
            pc = code + localVariable[i].ival;
            break;
        case OP_return:
            /*  --> [empty] 	return void from method */
            return 0;
        case OP_saload:
            /*  arrayref, index --> value 	load short from array */
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            JVM_Top->ival = arrSimple->u.hval[i];
            break;
        case OP_sastore:
            /*  arrayref, index, value --> 	store short to array */
            anIntValue = JVM_Pop();
            i = JVM_Pop();
            aHeapReference = JVM_Top->pval;
            if (aHeapReference == NULL_HEAP_REFERENCE)
                throwException("NullPointerException",pc,method,thisClass);
            arrSimple = REAL_HEAP_POINTER(aHeapReference);
            if (i < 0 || i >= arrSimple->size)
                throwException("ArrayIndexOutOfBoundsException",pc,method,thisClass);
            arrSimple->u.hval[i] = anIntValue;
            break;
        case OP_sipush:
            /*  byte1, byte2 	--> value 
                pushes a 16-bit signed integer onto the stack */
            shortVal = iget2(&pc);
            JVM_Push(shortVal);
            break;
        case OP_swap:
            /*  value2, value1 --> value1, value2
                swaps two top words on the stack (note that value1 and
                value2 must not be double or long) */
            u = JVM_Top->uval;
            JVM_Top->uval = (JVM_Top-1)->uval;
            (JVM_Top-1)->uval = u;
            break;
        case OP_tableswitch:
            /*  [0-3 bytes padding], defaultbyte1, defaultbyte2, defaultbyte3,
                defaultbyte4, lowbyte1, lowbyte2, lowbyte3, lowbyte4, highbyte1,
                highbyte2, highbyte3, highbyte4, jump offsets... 	index -->
                continue execution from an address in the table at offset index */
            opcodeAddr = pc-1;
            i = JVM_Pop();
            pc = code + ((pc - code + 3) & 0xFFFFFFFC);  /* account for padding bytes */
            offset = iget4(&pc);
            low = iget4(&pc);
            high = iget4(&pc);
            if (i >= low && i <= high) {
                pc += 4 * (i - low);
                offset = iget4(&pc);
            }
            pc = opcodeAddr + offset;
            break;
        case OP_wide:
            /*  opcode, indexbyte1, indexbyte2 or
                iinc, indexbyte1, indexbyte2, countbyte1, countbyte2
                [same as for corresponding instructions]
                execute opcode, where opcode is either iload, fload, aload,
                lload, dload, istore, fstore, astore, lstore, dstore, or ret,
                but assume the index is 16 bit; or execute iinc,
                where the index is 16 bits and the constant to increment
                by is a signed 16 bit short */
            fprintf(stderr,"unimplemented op: wide\n");
            break;
        case OP_breakpoint:
            /*  reserved for breakpoints in Java debuggers; should not
                appear in any class file */
            fprintf(stderr,"unimplemented op: breakpoint\n");
            break;
        case OP_impdep1:
            /*  reserved for implementation-dependent operations within
                debuggers; should not appear in any class file */
            fprintf(stderr,"unimplemented op: impdep1\n");
            break;
        case OP_impdep2:
            /*  reserved for implementation-dependent operations within
                debuggers; should not appear in any class file */
            fprintf(stderr,"unimplemented op: impdep2\n");
            break;
        default:
            fprintf(stderr,"unimplemented op with code %d\n", op);
        }
    }
}
