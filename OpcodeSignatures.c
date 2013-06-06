/* OpcodeSignatures.c */

/*
    The opcodes table provides the following information about JVM opcodes.
    
    1. The op field is an integer in the range 0 to 0xC9 and is the op's
       representation in the bytecode array.
    2. The opcodeName is the human-readable name for the opcode.
    3. The inlineOperands field describes what information, if any, immediately
       follows the opcode in the bytecode array.
       A lower case letter (b, i, c, v) indicates that there is a single byte
       (and bb indicates 2 bytes, etc.). The letter gives a clue as the usage
       of the byte.
       A 'i' indicates an 8-bit integer index and ii indicates a 16-bit integer
       index into the constant pool, or the array of local variables, or ...
       A 'b' or 'bb' or 'bbbb' indicates an 8, 16 or 32 bit branch offset.
       A 'v' or 'vv' indicates an 8 or 16 bit value used by the instruction.
       A 'c' indicates an 8-bit count.
       A '0' indicates a byte which must hold 0.
       The exceptional cases are for ops where the number of inline operand
       bytes is variable. These ops need special handling. The entries are
       shown as "n+" where n is a decimal digit greater than 0. The value of n
       indicates the minimum number of inline operand bytes.
    4. The signature field shows the type signature of the op. Every entry
       has the format "{lhs}>{rhs}" where {lhs} and {rhs} represent sequences
       of 0 or more code letters. The codes used in the table are as follows.
       I:  int, 32-bit          (1 word on stack)
       Ll: long, 64-bit         (2 words on stack)
       F:  float, 32-bit        (1 word on stack)
       Dd: double, 64-bit       (2 words on stack)
       A:  address, 32-bit heap reference (1 word on stack)
       R:  return address pushed by jsr (1 word on stack)
       N:  null pointer
       Because we use two code letters for long and double values, there will
       always be one code letter per stack slot.
       There are some ops which work on more than one type. For example
       ldc pushes a String or an int or a float. Its signature is shown as
       ">X". The letters W, X, Y, Z represent such wildcard types and all
       need some special handling.
       Finally, the symbol "*" represents a sequence of 0 or more values
       on the stack, where the number and the types of these values are not
       specified in the opcode. For example, invokevirtual requires an object
       reference (type A) on the stack plus 0 or more operands for the
       virtual method being called.

   Each entry in the table is followed by a comment taken almost verbatim
   from the Wikipedia page:
        http://en.wikipedia.org/wiki/Java_bytecode_instruction_listings
        
   Note: the op field in each table entry is redundant but serves as a good
   consistency check for the table. See the CheckOpcodeTable() function.
*/

#include <assert.h>
#include "OpcodeSignatures.h"

OpcodeDescription opcodes[] = {
{ 0X00, "nop",          "", ">" },      /* perform no operation */
{ 0X01, "aconst_null",  "", ">N" },     /* push a null reference onto the stack */
{ 0X02, "iconst_m1",    "", ">I"},      /* load the int value -1 onto the stack */
{ 0X03, "iconst_0",     "", ">I"},      /* load the int value 0 onto the stack */
{ 0X04, "iconst_1",     "", ">I"},      /* load the int value 1 onto the stack */
{ 0X05, "iconst_2",     "", ">I"},      /* load the int value 2 onto the stack */
{ 0X06, "iconst_3",     "", ">I"},      /* load the int value 3 onto the stack */
{ 0X07, "iconst_4",     "", ">I"},      /* load the int value 4 onto the stack */
{ 0X08, "iconst_5",     "", ">I"},      /* load the int value 5 onto the stack */
{ 0X09, "lconst_0",     "", ">Ll"},     /* push the long 0 onto the stack */
{ 0X0a, "lconst_1",     "", ">Ll"},     /* push the long 1 onto the stack */
{ 0X0b, "fconst_0",     "", ">F"},      /* push 0.0f on the stack */
{ 0X0c, "fconst_1",     "", ">F"},      /* push 1.0f on the stack */
{ 0X0d, "fconst_2",     "", ">F"},      /* push 2.0f on the stack */
{ 0X0e, "dconst_0",     "", ">Dd"},     /* push the constant 0.0 onto the stack */
{ 0X0f, "dconst_1",     "", ">Dd"},     /* push the constant 1.0 onto the stack */
{ 0X10, "bipush",       "v", ">I"},     /* push a byte onto the stack as an integer value */
{ 0X11, "sipush",       "vv", ">I" },   /* push a short onto the stack */
{ 0X12, "ldc",          "i", ">X" },    /* push a constant #index from a constant pool (String, int or float) onto the stack */
{ 0X13, "ldc_w",        "ii", ">X" },   /* push a constant #index from a constant pool (String, int or float) onto the stack (wide index is constructed as i1 << 8 + i2) */
{ 0X14, "ldc2_w",       "ii", ">X" },   /* push a constant #index from a constant pool (double or long) onto the stack (wide index is constructed as i1 << 8 + i2) */
{ 0X15, "iload",        "i", ">I"},     /* load an int value from a local variable #index */
{ 0X16, "lload",        "i", ">Ll"},    /* load a long value from a local variable #index */
{ 0X17, "fload",        "i", ">F"},     /* load a float value from a local variable #index */
{ 0X18, "dload",        "i", ">Dd"},    /* load a double value from a local variable #index */
{ 0X19, "aload",        "i", ">A" },    /* load a reference onto the stack from a local variable #index */
{ 0X1a, "iload_0",      "", ">I"},      /* load an int value from local variable 0 */
{ 0X1b, "iload_1",      "", ">I"},      /* load an int value from local variable 1 */
{ 0X1c, "iload_2",      "", ">I"},      /* load an int value from local variable 2 */
{ 0X1d, "iload_3",      "", ">I"},      /* load an int value from local variable 3 */
{ 0X1e, "lload_0",      "", ">Ll"},     /* load a long value from a local variable 0 */
{ 0X1f, "lload_1",      "", ">Ll"},     /* load a long value from a local variable 1 */
{ 0X20, "lload_2",      "", ">Ll"},     /* load a long value from a local variable 2 */
{ 0X21, "lload_3",      "", ">Ll"},     /* load a long value from a local variable 3 */
{ 0X22, "fload_0",      "", ">F"},      /* load a float value from local variable 0 */
{ 0X23, "fload_1",      "", ">F"},      /* load a float value from local variable 1 */
{ 0X24, "fload_2",      "", ">F"},      /* load a float value from local variable 2 */
{ 0X25, "fload_3",      "", ">F"},      /* load a float value from local variable 3 */
{ 0X26, "dload_0",      "", ">Dd"},     /* load a double from local variable 0 */
{ 0X27, "dload_1",      "", ">Dd"},     /* load a double from local variable 1 */
{ 0X28, "dload_2",      "", ">Dd"},     /* load a double from local variable 2 */
{ 0X29, "dload_3",      "", ">Dd"},     /* load a double from local variable 3 */
{ 0X2a, "aload_0",      "", ">A" },     /* load a reference onto the stack from local variable 0 */
{ 0X2b, "aload_1",      "", ">A" },     /* load a reference onto the stack from local variable 1 */
{ 0X2c, "aload_2",      "", ">A" },     /* load a reference onto the stack from local variable 2 */
{ 0X2d, "aload_3",      "", ">A" },     /* load a reference onto the stack from local variable 3 */
{ 0X2e, "iaload",       "", "AI->I" },  /* load an int from an array */
{ 0X2f, "laload",       "", "AI->Ll" }, /* load a long from an array */
{ 0X30, "faload",       "", "AI->F" },  /* load a float from an array */
{ 0X31, "daload",       "", "AI->Dd" }, /* load a double from an array */
{ 0X32, "aaload",       "", "AI->A" },  /* load onto the stack a reference from an array */
{ 0X33, "baload",       "", "AI->I" },  /* load a byte or Boolean value from an array */
{ 0X34, "caload",       "", "AI->I" },  /* load a char from an array */
{ 0X35, "saload",       "", "AI->I" },  /* load short from array */
{ 0X36, "istore",       "i", "I>" },    /* store int value into variable #index */
{ 0X37, "lstore",       "i", "Ll>" },   /* store a long value in a local variable #index */
{ 0X38, "fstore",       "i", "F>" },    /* store a float value into a local variable #index */
{ 0X39, "dstore",       "i", "Dd>" },   /* store a double value into a local variable #index */
{ 0X3a, "astore",       "i", "A>" },    /* store a reference into a local variable #index */
{ 0X3b, "istore_0",     "", "I>" },     /* store int value into variable 0 */
{ 0X3c, "istore_1",     "", "I>" },     /* store int value into variable 1 */
{ 0X3d, "istore_2",     "", "I>" },     /* store int value into variable 2 */
{ 0X3e, "istore_3",     "", "I>" },     /* store int value into variable 3 */
{ 0X3f, "lstore_0",     "", "Ll>" },    /* store a long value in a local variable 0 */
{ 0X40, "lstore_1",     "", "Ll>" },    /* store a long value in a local variable 1 */
{ 0X41, "lstore_2",     "", "Ll>" },    /* store a long value in a local variable 2 */
{ 0X42, "lstore_3",     "", "Ll>" },    /* store a long value in a local variable 3 */
{ 0X43, "fstore_0",     "", "F>" },     /* store a float value into local variable 0 */
{ 0X44, "fstore_1",     "", "F>" },     /* store a float value into local variable 1 */
{ 0X45, "fstore_2",     "", "F>" },     /* store a float value into local variable 2 */
{ 0X46, "fstore_3",     "", "F>" },     /* store a float value into local variable 3 */
{ 0X47, "dstore_0",     "", "Dd>" },    /* store a double into local variable 0 */
{ 0X48, "dstore_1",     "", "Dd>" },    /* store a double into local variable 1 */
{ 0X49, "dstore_2",     "", "Dd>" },    /* store a double into local variable 2 */
{ 0X4a, "dstore_3",     "", "Dd>" },    /* store a double into local variable 3 */
{ 0X4b, "astore_0",     "", "A>" },     /* store a reference into local variable 0 */
{ 0X4c, "astore_1",     "", "A>" },     /* store a reference into local variable 1 */
{ 0X4d, "astore_2",     "", "A>" },     /* store a reference into local variable 2 */
{ 0X4e, "astore_3",     "", "A>" },     /* store a reference into local variable 3 */
{ 0X4f, "iastore",      "", "AII>" },   /* store an int into an array */
{ 0X50, "lastore",      "", "AILl>" },  /* store a long to an array */
{ 0X51, "fastore",      "", "AIF>" },   /* store a float in an array */
{ 0X52, "dastore",      "", "AIDd>" },  /* store a double into an array */
{ 0X53, "aastore",      "", "AIA>" },   /* store into a reference in an array */
{ 0X54, "bastore",      "", "AII>" },   /* store a byte or Boolean value into an array */
{ 0X55, "castore",      "", "AII>" },   /* store a char into an array */
{ 0X56, "sastore",      "", "AII>" },   /* store short to array */
{ 0X57, "pop",          "", "X>" },     /* discard the top value on the stack */
{ 0X58, "pop2",         "", "XX>" },    /* discard the top two values on the stack (or one value, if it is a double or long) */
{ 0X59, "dup",          "", "X>XX" },   /* duplicate the value on top of the stack */
{ 0X5a, "dup_x1",       "", "YX>XYX" }, /* insert a copy of the top value into the stack two values from the top. X and Y must not be of the type double or long. */
{ 0X5b, "dup_x2",       "", "ZYX>XZYX" }, /* insert a copy of X into the stack two (if Y is double or long it takes up the entry of Z, too) or three values (if Y is neither double nor long) from the top */
{ 0X5c, "dup2",         "", "XY>XYXY" }, /* duplicate top two stack words (two values, if Y is not double nor long; a single value, if Y is double or long) */
{ 0X5d, "dup2_x1",      "", "ZYX>YXYXZYX" }, /* duplicate two words and insert beneath third word (see explanation above) */
{ 0X5e, "dup2_x2",      "", "ZYXW>YXWZYX" }, /* duplicate two words and insert beneath fourth word */
{ 0X5f, "swap",         "", "XY>YX" },  /* swaps two top words on the stack (note that X and Y must not be double or long) */
{ 0X60, "iadd",         "", "II>I" },   /* add two ints */
{ 0X61, "ladd",         "", "LlLl>Ll" }, /* add two longs */
{ 0X62, "fadd",         "", "FF>F" },   /* add two floats */
{ 0X63, "dadd",         "", "DdDd>Dd" }, /* add two doubles */
{ 0X64, "isub",         "", "II>I" },   /* int subtract */
{ 0X65, "lsub",         "", "LlLl>Ll" }, /* subtract two longs */
{ 0X66, "fsub",         "", "FF>F" },   /* subtract two floats */
{ 0X67, "dsub",         "", "DdDd>Dd" }, /* subtract a double from another */
{ 0X68, "imul",         "", "II>I" },   /* multiply two integers */
{ 0X69, "lmul",         "", "LlLl>Ll" }, /* multiply two longs */
{ 0X6a, "fmul",         "", "FF>F" },   /* multiply two floats */
{ 0X6b, "dmul",         "", "DdDd>Dd" }, /* multiply two doubles */
{ 0X6c, "idiv",         "", "II>I" },   /* divide two integers */
{ 0X6d, "ldiv",         "", "LlLl>Ll" }, /* divide two longs */
{ 0X6e, "fdiv",         "", "FF>F" },   /* divide two floats */
{ 0X6f, "ddiv",         "", "DdDd>Dd" }, /* divide two doubles */
{ 0X70, "irem",         "", "II>I" },   /* logical int remainder */
{ 0X71, "lrem",         "", "LlLl>Ll" }, /* remainder of division of two longs */
{ 0X72, "frem",         "", "FF>F" },   /* get the remainder from a division between two floats */
{ 0X73, "drem",         "", "DdDd>Dd" }, /* get the remainder from a division between two doubles */
{ 0X74, "ineg",         "", "I>I" },    /* negate int */
{ 0X75, "lneg",         "", "Ll>Ll" },   /* negate a long */
{ 0X76, "fneg",         "", "F>F" },    /* negate a float */
{ 0X77, "dneg",         "", "Dd>Dd" },   /* negate a double */
{ 0X78, "ishl",         "", "II>I" },   /* int shift left */
{ 0X79, "lshl",         "", "LlI>Ll" }, /* bitwise shift left of a long value1 by value2 positions */
{ 0X7a, "ishr",         "", "II>I" },   /* int arithmetic shift right */
{ 0X7b, "lshr",         "", "LlI>Ll" }, /* bitwise shift right of a long value1 by value2 positions */
{ 0X7c, "iushr",        "", "II>I" },   /* int logical shift right */
{ 0X7d, "lushr",        "", "LlI>Ll" }, /* bitwise shift right of a long value1 by value2 positions, unsigned */
{ 0X7e, "iand",         "", "II>I" },   /* perform a bitwise and on two integers */
{ 0X7f, "land",         "", "LlLl>Ll" }, /* bitwise and of two longs */
{ 0X80, "ior",          "", "II>I" },   /* bitwise int or */
{ 0X81, "lor",          "", "LlLl>Ll" }, /* bitwise or of two longs */
{ 0X82, "ixor",         "", "II>I" },   /* int xor */
{ 0X83, "lxor",         "", "LlLl>Ll" }, /* bitwise exclusive or of two longs */
{ 0X84, "iinc",         "iiv", "" },    /* increment local variable #index by signed byte const */
{ 0X85, "i2l",          "", "I>Ll" },   /* convert an int into a long */
{ 0X86, "i2f",          "", "I>Dd" },   /* convert an int into a float */
{ 0X87, "i2d",          "", "Ll>I" },   /* convert an int into a double */
{ 0X88, "l2i",          "", "Ll>I" },   /* convert a long to a int */
{ 0X89, "l2f",          "", "Ll>F" },   /* convert a long to a float */
{ 0X8a, "l2d",          "", "Ll>Dd" },  /* convert a long to a double */
{ 0X8b, "f2i",          "", "F>I" },    /* convert a float to an int */
{ 0X8c, "f2l",          "", "F>Ll" },   /* convert a float to a long */
{ 0X8d, "f2d",          "", "F>Dd" },   /* convert a float to a double */
{ 0X8e, "d2i",          "", "Dd>I" },   /* convert a double to an int */
{ 0X8f, "d2l",          "", "Dd>Ll" },  /* convert a double to a long */
{ 0X90, "d2f",          "", "Dd>F" },   /* convert a double to a float */
{ 0X91, "i2b",          "", "I>I" },    /* convert an int into a byte */
{ 0X92, "i2c",          "", "I>I" },    /* convert an int into a character */
{ 0X93, "i2s",          "", "I>I" },    /* convert an int into a short */
{ 0X94, "lcmp",         "", "LlLl>I" }, /* compare two longs values */
{ 0X95, "fcmpl",        "", "FF>I" },   /* compare two floats */
{ 0X96, "fcmpg",        "", "FF>I" },   /* compare two floats */
{ 0X97, "dcmpl",        "", "DdDd>I" }, /* compare two doubles */
{ 0X98, "dcmpg",        "", "DdDd>I" }, /* compare two doubles */
{ 0X99, "ifeq",         "bb", "I>" },   /* if value is 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9a, "ifne",         "bb", "I>" },   /* if value is not 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9b, "iflt",         "bb", "I>" },   /* if value is less than 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9c, "ifge",         "bb", "I>" },   /* if value is greater than or equal to 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9d, "ifgt",         "bb", "I>" },   /* if value is greater than 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9e, "ifle",         "bb", "I>" },   /* if value is less than or equal to 0, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0X9f, "if_icmpeq",    "bb", "II>" },  /* if ints are equal, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa0, "if_icmpne",    "bb", "II>" },  /* if ints are not equal, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa1, "if_icmplt",    "bb", "II>" },  /* if value1 is less than value2, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa2, "if_icmpge",    "bb", "II>" },  /* if value1 is greater than or equal to value2, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa3, "if_icmpgt",    "bb", "II>" },  /* if value1 is greater than value2, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa4, "if_icmple",    "bb", "II>" },  /* if value1 is less than or equal to value2, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa5, "if_acmpeq",    "bb", "AA>" },  /* if references are equal, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa6, "if_acmpne",    "bb", "AA>" },  /* if references are not equal, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa7, "goto",         "bb", "" },     /* go to another instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xa8, "jsr",          "bb", ">R" },   /* jump to subroutine at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) and place the return address on the stack */
{ 0Xa9, "ret",          "i", "" },      /* continue execution from address taken from a local variable #index (the asymmetry with jsr is intentional) */
{ 0Xaa, "tableswitch",  "4+", "I>" },   /* continue execution from an address in the table at offset index */
{ 0Xab, "lookupswitch", "4+", "I>" },   /* a target address is looked up from a table using a key and execution continues from the instruction at that address */
{ 0Xac, "ireturn",      "", "I>" },     /* return an integer from a method */
{ 0Xad, "lreturn",      "", "Ll>" },    /* return a long value */
{ 0Xae, "freturn",      "",  "F>" },    /* return a float */
{ 0Xaf, "dreturn",      "", "Dd>" },    /* return a double from a method */
{ 0Xb0, "areturn",      "",  "A>" },    /* return a reference from a method */
{ 0Xb1, "return",       "", "" },       /* return void from method */
{ 0Xb2, "getstatic",    "ii", ">X" },   /* get a static field value of a class, where the field is identified by field reference in the constant pool index (index1 << 8 + index2) */
{ 0Xb3, "putstatic",    "ii", "X>" },   /* set static field to value in a class, where the field is identified by a field reference index in constant pool (i1 << 8 + i2) */
{ 0Xb4, "getfield",     "ii", "A>X" },  /* get a field value of an object objectref, where the field is identified by field reference in the constant pool index (index1 << 8 + index2) */
{ 0Xb5, "putfield",     "ii", "AX>" },  /* set field to value in an object objectref, where the field is identified by a field reference index in constant pool (i1 << 8 + i2) */
{ 0Xb6, "invokevirtual", "ii", "A*>*" }, /* invoke virtual method on object objectref, where the method is identified by method reference index in constant pool (i1 << 8 + i2) */
{ 0Xb7, "invokespecial", "ii", "A*>*" }, /* invoke instance method on object objectref, where the method is identified by method reference index in constant pool (i1 << 8 + i2) */
{ 0Xb8, "invokestatic", "ii", "*>*" },   /* invoke a static method, where the method is identified by method reference index in constant pool (i1 << 8 + i2) */
{ 0Xb9, "invokeinterface", "iic0", "A*>*" }, /* invoke an interface method on object objectref, where the interface method is identified by method reference index in constant pool (i1 << 8 + i2) */
{ 0Xba, "invokedynamic", "ii00", "*>*" }, /* invoke a dynamic method identified by method reference index in constant pool (i1 << 8 + i2) */
{ 0Xbb, "new",          "ii", ">A" },   /* create new object of type identified by class reference in constant pool index (i1 << 8 + i2) */
{ 0Xbc, "newarray",     "i", "I>A" },   /* create new array with count elements of primitive type identified by atype */
{ 0Xbd, "anewarray",    "ii", "I>A" },  /* create a new array of references of length count and component type identified by the class reference index (i1 << 8 + i2) in the constant pool */
{ 0Xbe, "arraylength",  "", "A>I" },    /* get the length of an array */
{ 0Xbf, "athrow",       "", "A>" },     /* throw an error or exception (notice that the rest of the stack is cleared, leaving only a reference to the Throwable) */
{ 0Xc0, "checkcast",    "ii", "A>A" },  /* check whether an objectref is of a certain type, the class reference of which is in the constant pool at index (i1 << 8 + i2) */
{ 0Xc1, "instanceof",   "ii", "A>I" },  /* determine if an object objectref is of a given type, identified by class reference index in constant pool (i1 << 8 + i2) */
{ 0Xc2, "monitorenter", "", "A>" },     /* enter monitor for object ("grab the lock" - start of synchronized() section) */
{ 0Xc3, "monitorexit",  "", "A>" },     /* exit monitor for object ("release the lock" - end of synchronized() section) */
{ 0Xc4, "wide",         "3+", "" },     /* execute opcode, where opcode is either iload, fload, aload, lload, dload, istore, fstore, astore, lstore, dstore, or ret, but assume the index is 16 bit; or execute iinc, where the index is 16 bits and the constant to increment by is a signed 16 bit short */
{ 0Xc5, "multianewarray", "iii", "I*>A" }, /* create a new array of i3 dimensions with elements of type identified by class reference in constant pool index (i1 << 8 + i2); the sizes of each dimension is identified by count1, [count2, etc.] */
{ 0Xc6, "ifnull",       "bb", "A>" },   /* if value is null, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xc7, "ifnonnull",    "bb","A>" },    /* if value is not null, branch to instruction at branchoffset (signed short constructed from unsigned bytes b1 << 8 + b2) */
{ 0Xc8, "goto_w",       "bbbb", "" },   /* go to another instruction at branchoffset (signed int constructed from unsigned bytes b1 << 24 + b2 << 16 + b3 << 8 + b4) */
{ 0Xc9, "jsr_w",        "bbbb", ">R" }, /* jump to subroutine at branchoffset (signed int constructed from unsigned bytes b1 << 24 + b2 << 16 + b3 << 8 + b4) and place the return address on the stack */
#if 0
   // The following lines are provided for documentation purposes only
{ 0Xca, "breakpoint",   "", "" },       /* reserved for breakpoints in debuggers; should not appear in any class file */
{ 0Xcb-fd, "(no name)", "",  ""}, /* these values are currently unassigned for opcodes and are reserved for future use */
{ 0Xfe, "impdep1", "", "" },    /* reserved for implementation-dependent operations within debuggers; should not appear in any class file */
{ 0Xff, "impdep2", "", "" }    /* reserved for implementation-dependent operations within debuggers; should not appear in any class file
 */
 #endif
};  // end of opcode table


/* This is just a sanity check that an entry in the table has
   not been accidentally deleted or exchanged with another.
   If the check succeeds, then it is safe to use
       opcode[i] to look up info about the op numbered i
*/
void CheckOpcodeTable() {
    int i;
    for( i=0; i<=LASTOPCODE; i++ ) {
        assert(i == opcodes[i].op);
    }
}
