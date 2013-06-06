/* jvm.h */

/* This header file defines the run-time datatypes used by
   our implementation of the JVM. */

#ifndef JVMH

#define JVMH

#include <stdint.h>
#include "MyAlloc.h"  /* to define HeapPointer */

#define UNINIT_PATTERN  0xDEADBEEF  /* a funny bit pattern */

extern uint8_t *HeapStart;

/* used as a NULL reference for a heap pointer */
#define NULL_HEAP_REFERENCE  ((HeapPointer)0)

/* converts a HeapReference x into a real pointer */
#define REAL_HEAP_POINTER(x)    ((void*)(HeapStart + (x)))

/* converts a real pointer into a HeapReference, an offset in the heap */
#define MAKE_HEAP_REFERENCE(p)  ((HeapPointer)((uint8_t*)(p) - HeapStart))


/* This structure is used to access either a local variable of
   a method or an item on the stack or a field of a class.
   It must be exactly 4 bytes in size to match the requirements
   of the JVM.
   Pairs of these structures are used for double values and
   long integer values.
*/
typedef union {
    int32_t     ival;
    uint32_t    uval;
    float       fval;
    HeapPointer pval;
} DataItem;


/* The fields precede the elements of an array on a heap. */
/* This version is used for an array of refs to objects.  */
typedef struct {
    uint32_t kind;             /* holds the chars 'ARRA' */
    int32_t  size;             /* number of elements */
    HeapPointer classRef;      /* datatype of the elements */
    HeapPointer elements[1];   /* storage is allocated for more than 1 element */
} ArrayOfRef;


/* These fields precede the elements of an array on a heap. */
/* This version is used for an array of simple values.      */
/* See newarray opcode for list of typecode values.         */
typedef struct {
    uint32_t kind;             /* holds the chars 'ARRS' */
    int32_t size;              /* number of elements */
    int16_t typecode;          /* type of the elements */
    int16_t elemSize;          /* # bytes for each element */
    union {
        uint8_t   bval[8];  /* typecode == 4  */
        char      cval[8];  /* typecode == 5  */
        int16_t   hval[4];  /* typecode == 9  */
        float     fval[2];  /* typecode == 6  */
        double    dval[1];  /* typecode == 7  */
        int32_t   ival[2];  /* typecode == 10 */
        int64_t   lval[1];  /* typecode == 11 */
    } u;
} ArrayOfSimple;


/* a cheat implementation of strings on the heap */
typedef struct {
    uint32_t kind;          /* holds the chars 'STRG' */
    char *sval;
} StringInstance;

/* a cheat implementation of StringBuilder instances on the heap */
typedef struct {
    uint32_t kind;          /* holds the chars 'SBLD' */
    uint32_t capacity;
    uint32_t len;
    char *buffer;
} StringBuilderInstance;

/* One instance of this struct is allocated on the heap for each
   reference type (a class or an array) that is loaded/created by the JVM.
   Some fields are used only if the type is a class, other fields only if
   the type is an array.  (Ideally a union type would reduce storage
   requirements, but that would be error prone.)

   If it's a class type, the classField array provides storage for only
   this class's static fields, and not for its parent class(es). */
typedef struct ClassType {
    uint32_t kind;                    /* holds the chars 'CLAS' */
    uint8_t  isArrayType;             /* true => it's an array type */
    char *typeDescriptor;             /* string version of the type name */
    struct ClassType *nextClass;      /* maintain list of all ClassType structs */

    /* the following field is only used if isArrayType is true */
    struct ClassType *elementType;    /* datatype of the elements */

    /* the following fields are used only if isArrayType is false */
    ClassFile *cf;                    /* the source file info */
    struct ClassType *parent;         /* super class */
    int numInstanceFields;            /* count of instance fields */
    DataItem classField[1];           /* storage for static fields */
} ClassType;


/* One instance of this struct is allocated on the heap for each
   instance of a normal Java class.
   The instField array contains storage for the instance fields. */
typedef struct {
    uint32_t kind;              /* holds the chars 'INST' */
    ClassType *thisClass;
    DataItem instField[1];
} ClassInstance;


#define CODE_ARRA (0x41525241)   /* the 4 characters 'ARRA' */
#define CODE_ARRS (0x41525253)   /* the 4 characters 'ARRS' */
#define CODE_CLAS (0x434C4153)   /* the 4 characters 'CLAS' */
#define CODE_INST (0x494E5354)   /* the 4 characters 'INST' */
#define CODE_STRG (0x53545247)   /* the 4 characters 'STRG' */
#define CODE_SBLD (0x53424C44)   /* the 4 characters 'SBLD' */


/* These are all the JVM opcodes in alphabetic order*/
typedef enum { OP_aaload=0X32, OP_aastore=0X53, OP_aconst_null=0X01, OP_aload=0X19,
    OP_aload_0=0X2a, OP_aload_1=0X2b, OP_aload_2=0X2c, OP_aload_3=0X2d,
    OP_anewarray=0Xbd, OP_areturn=0Xb0, OP_arraylength=0Xbe, OP_astore=0X3a,
    OP_astore_0=0X4b, OP_astore_1=0X4c, OP_astore_2=0X4d, OP_astore_3=0X4e,
    OP_athrow=0Xbf, OP_baload=0X33, OP_bastore=0X54, OP_bipush=0X10,
    OP_caload=0X34, OP_castore=0X55, OP_checkcast=0Xc0, OP_d2f=0X90, OP_d2i=0X8e,
    OP_d2l=0X8f, OP_dadd=0X63, OP_daload=0X31, OP_dastore=0X52, OP_dcmpg=0X98,
    OP_dcmpl=0X97, OP_dconst_0=0X0e, OP_dconst_1=0X0f, OP_ddiv=0X6f, OP_dload=0X18,
    OP_dload_0=0X26, OP_dload_1=0X27, OP_dload_2=0X28, OP_dload_3=0X29,
    OP_dmul=0X6b, OP_dneg=0X77, OP_drem=0X73, OP_dreturn=0Xaf, OP_dstore=0X39,
    OP_dstore_0=0X47, OP_dstore_1=0X48, OP_dstore_2=0X49, OP_dstore_3=0X4a,
    OP_dsub=0X67, OP_dup=0X59, OP_dup_x1=0X5a, OP_dup_x2=0X5b, OP_dup2=0X5c,
    OP_dup2_x1=0X5d, OP_dup2_x2=0X5e, OP_f2d=0X8d, OP_f2i=0X8b, OP_f2l=0X8c,
    OP_fadd=0X62, OP_faload=0X30, OP_fastore=0X51, OP_fcmpg=0X96, OP_fcmpl=0X95,
    OP_fconst_0=0X0b, OP_fconst_1=0X0c, OP_fconst_2=0X0d, OP_fdiv=0X6e,
    OP_fload=0X17, OP_fload_0=0X22, OP_fload_1=0X23, OP_fload_2=0X24, OP_fload_3=0X25,
    OP_fmul=0X6a, OP_fneg=0X76, OP_frem=0X72, OP_freturn=0Xae, OP_fstore=0X38,
    OP_fstore_0=0X43, OP_fstore_1=0X44, OP_fstore_2=0X45, OP_fstore_3=0X46,
    OP_fsub=0X66, OP_getfield=0Xb4, OP_getstatic=0Xb2, OP_goto=0Xa7, OP_goto_w=0Xc8,
    OP_i2b=0X91, OP_i2c=0X92, OP_i2d=0X87, OP_i2f=0X86, OP_i2l=0X85, OP_i2s=0X93,
    OP_iadd=0X60, OP_iaload=0X2e, OP_iand=0X7e, OP_iastore=0X4f, OP_iconst_m1=0X02,
    OP_iconst_0=0X03, OP_iconst_1=0X04, OP_iconst_2=0X05, OP_iconst_3=0X06,
    OP_iconst_4=0X07, OP_iconst_5=0X08, OP_idiv=0X6c, OP_if_acmpeq=0Xa5,
    OP_if_acmpne=0Xa6, OP_if_icmpeq=0X9f, OP_if_icmpne=0Xa0, OP_if_icmplt=0Xa1,
    OP_if_icmpge=0Xa2, OP_if_icmpgt=0Xa3, OP_if_icmple=0Xa4, OP_ifeq=0X99,
    OP_ifne=0X9a, OP_iflt=0X9b, OP_ifge=0X9c, OP_ifgt=0X9d, OP_ifle=0X9e,
    OP_ifnonnull=0Xc7, OP_ifnull=0Xc6, OP_iinc=0X84, OP_iload =0X15,
    OP_iload_0=0X1a, OP_iload_1=0X1b, OP_iload_2=0X1c, OP_iload_3=0X1d,
    OP_imul=0X68, OP_ineg=0X74, OP_instanceof=0Xc1, OP_invokeinterface=0Xb9,
    OP_invokespecial=0Xb7, OP_invokestatic=0Xb8, OP_invokevirtual=0Xb6,
    OP_ior=0X80, OP_irem=0X70, OP_ireturn=0Xac, OP_ishl=0X78, OP_ishr=0X7a,
    OP_istore=0X36, OP_istore_0=0X3b, OP_istore_1=0X3c, OP_istore_2=0X3d,
    OP_istore_3=0X3e, OP_isub=0X64, OP_iushr=0X7c, OP_ixor=0X82, OP_jsr=0Xa8,
    OP_jsr_w=0Xc9, OP_l2d=0X8a, OP_l2f=0X89, OP_l2i=0X88, OP_ladd=0X61,
    OP_laload=0X2f, OP_land=0X7f, OP_lastore=0X50, OP_lcmp=0X94, OP_lconst_0=0X09,
    OP_lconst_1=0X0a, OP_ldc=0X12, OP_ldc_w=0X13, OP_ldc2_w=0X14, OP_ldiv=0X6d,
    OP_lload=0X16, OP_lload_0=0X1e, OP_lload_1=0X1f, OP_lload_2=0X20,
    OP_lload_3=0X21, OP_lmul=0X69, OP_lneg=0X75, OP_lookupswitch=0Xab,
    OP_lor=0X81, OP_lrem=0X71, OP_lreturn=0Xad, OP_lshl=0X79, OP_lshr=0X7b, OP_lstore=0X37,
    OP_lstore_0=0X3f, OP_lstore_1=0X40, OP_lstore_2=0X41, OP_lstore_3=0X42,
    OP_lsub=0X65, OP_lushr=0X7d, OP_lxor=0X83, OP_monitorenter=0Xc2, OP_monitorexit=0Xc3,
    OP_multianewarray=0Xc5, OP_new=0Xbb, OP_newarray=0Xbc, OP_nop=0X00, OP_pop=0X57,
    OP_pop2=0X58, OP_putfield=0Xb5, OP_putstatic=0Xb3, OP_ret=0Xa9, OP_return=0Xb1,
    OP_saload=0X35, OP_sastore=0X56, OP_sipush=0X11, OP_swap=0X5f, OP_tableswitch=0Xaa,
    OP_wide=0Xc4, OP_breakpoint=0Xca, OP_impdep1=0Xfe, OP_impdep2=0Xff
} JVM_Opcode;


extern DataItem *JVM_Top;
extern void *HeapReferencePointer;
extern void *Fake_System_Out;

extern void JVM_Init( int stackSize );
extern void JVM_Push( uint32_t x );
extern void JVM_PushFloat( float x );
extern void JVM_PushReference( HeapPointer x );

extern uint32_t JVM_Pop();
extern float JVM_PopFloat();
extern HeapPointer JVM_PopReference();

#endif

