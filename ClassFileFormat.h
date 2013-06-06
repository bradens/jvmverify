/* ClassFileFormat.h */

#ifndef CLASSFILEFORMATH

#define CLASSFILEFORMATH

/* The layouts of data objects found in a Java class file are defined
   here.  They are copied almost verbatim from chapter 4 of the
   Lindholm and Yellin book, The Java Virtual Machine Specification. */
#include <stdint.h>

// Every class file must begin with these 4 bytes
#define MagicNumber 0xCAFEBABE

/* u1, u2, u4 are names used in the JVM specifications for
   1, 2 and 4 byte integer types respectively */
typedef uint32_t u4;
typedef uint16_t u2;
typedef uint8_t  u1;

typedef enum {                 /* Access flags used by ... */
    ACC_NONE         = 0X0000,
    ACC_PUBLIC       = 0X0001, /*  Class, Interface, any field, any method */
    ACC_PRIVATE      = 0X0002, /*  Class field, Class/instance method */
    ACC_PROTECTED    = 0X0004, /*  Class field, Class/instance method */
    ACC_STATIC       = 0X0008, /*  ny field, Class/instance method */
    ACC_FINAL        = 0X0010, /*  Class, any field, Class/instance method */
    ACC_SYNCHRONIZED = 0X0020, /*  Class/instance method */
    ACC_SUPER        = 0X0020, /*  Class, Interface */
    ACC_VOLATILE     = 0X0040, /*  Class field */
    ACC_TRANSIENT    = 0X0080, /*  Class field */
    ACC_NATIVE       = 0X0100, /*  Class/instance method */
    ACC_INTERFACE    = 0X0200, /*  Interface */
    ACC_ABSTRACT     = 0X0400  /*  Class, Interface, Class/instance method */
} AccessFlag;

typedef enum {
    CP_Unknown=0, CP_UTF8=1, CP_Integer=3, CP_Float=4, CP_Long=5,
    CP_Double=6, CP_Class=7, CP_String=8, CP_Field=9, CP_Method=10,
    CP_Interface=11, CP_NameAndType=12
} ConstantPoolTag;

/* Our in-memory copy of a class's constant pool uses this
   union datatype for each constant.  A long or a double
   constant uses two consecutive entries in the table. */
typedef union {
    int32_t    ival;
    float      fval;
    u1        *sval;
    u4         uval;
    struct { u2 sval1;  u2 sval2; } ss;
} ConstantPoolItem;

typedef struct {
    u2  attribute_name_index;
    u4  attribute_length;
    u1  *info;
} attribute_info;

typedef struct {
    u2  attribute_name_index;
    u4  attribute_length;
    u2  constantValue_index;
} ConstantValue_attribute;

typedef struct {
    u2  access_flags;
    u2  name_index;
    u2  descriptor_index;
    u2  constantValue_index;
} field_info;

typedef struct {
    u2  access_flags;
    u2  name_index;
    u2  descriptor_index;
    u2  code_length;
    u1  *code;
    u2  max_stack, max_locals;
    u2  exception_table_length;
    u1  *exception_table;
    u2  attributes_count;
    u1  *attributes;
    u4   nArgs;  /* # arguments (including 'this' for an instance method) */
} method_info;

typedef struct {
    char  *cname;
    u2     constant_pool_count;
    u1    *cp_tag;        /* array of tags for const pool entries */
    ConstantPoolItem *cp_item;  /* array of constant pool values */
    u2     access_flags;
    u2     this_class;
    u2     super_class;
    u2     interfaces_count;
    u2    *interfaces;
    u2     fields_count;
    field_info  *fields;
    u2     methods_count;
    method_info *methods;
} ClassFile;

/* access functions */

extern char *GetUTF8( ClassFile *cf, int ix );
extern char *GetCPItemAsString( ClassFile *cf, int ix );

#endif
