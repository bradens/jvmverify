/* PrintByteCode.c */

#include <stdio.h>
#include <stdlib.h>
#include "ClassFileFormat.h"
#include "PrintByteCode.h"

/*  The following table contains information used for disassembling
    bytecode to generate a formatted output.  As well as the names of
    the opcodes, the table contains a string of format codes to describe
    the operands that appear explicitly in the bytecode.  The codes are:
        v == one byte index to a local variable
        T == type (class/array/interface) ref,
             as a two byte index into the constant pool
        C == class ref, as a two byte index into the constant pool
        u == one byte unsigned immediate integer
        s == one byte signed immediate integer
        F == field ref, as a two byte index into the constant pool
        I == interface ref, as a two byte index into the constant pool
        b == two byte branch destination
        B == four byte branch destination
        M == method ref, as a two byte index into the constant pool
        k == simple constant in constant pool, one byte index
        K == simple constant in constant pool, two byte index
        t == one byte primitive type
        S == two byte signed immediate integer
        w == wide (special case)
        * == lookupswitch (special case)
        & == tableswitch (special case)
        0 == a zero byte
*/

struct { char *name, *opnds_format; }
    OPCodeFormat[256] =
{
    { /*0X00*/ "nop", NULL },
    { /*0X01*/ "aconst_null", NULL },
    { /*0X02*/ "iconst_m1", NULL },
    { /*0X03*/ "iconst_0", NULL },
    { /*0X04*/ "iconst_1", NULL },
    { /*0X05*/ "iconst_2", NULL },
    { /*0X06*/ "iconst_3", NULL },
    { /*0X07*/ "iconst_4", NULL },
    { /*0X08*/ "iconst_5", NULL },
    { /*0X09*/ "lconst_0", NULL },
    { /*0X0a*/ "lconst_1", NULL },
    { /*0X0b*/ "fconst_0", NULL },
    { /*0X0c*/ "fconst_1", NULL },
    { /*0X0d*/ "fconst_2", NULL },
    { /*0X0e*/ "dconst_0", NULL },
    { /*0X0f*/ "dconst_1", NULL },
    { /*0X10*/ "bipush", "s" },
    { /*0X11*/ "sipush", "S" },
    { /*0X12*/ "ldc", "k" },
    { /*0X13*/ "ldc_w", "K" },
    { /*0X14*/ "ldc2_w", "K" },
    { /*0X15*/ "iload", "v" },
    { /*0X16*/ "lload", "v" },
    { /*0X17*/ "fload", "v" },
    { /*0X18*/ "dload", "v" },
    { /*0X19*/ "aload", "v" },
    { /*0X1a*/ "iload_0", NULL },
    { /*0X1b*/ "iload_1", NULL },
    { /*0X1c*/ "iload_2", NULL },
    { /*0X1d*/ "iload_3", NULL },
    { /*0X1e*/ "lload_0", NULL },
    { /*0X1f*/ "lload_1", NULL },
    { /*0X20*/ "lload_2", NULL },
    { /*0X21*/ "lload_3", NULL },
    { /*0X22*/ "fload_0", NULL },
    { /*0X23*/ "fload_1", NULL },
    { /*0X24*/ "fload_2", NULL },
    { /*0X25*/ "fload_3", NULL },
    { /*0X26*/ "dload_0", NULL },
    { /*0X27*/ "dload_1", NULL },
    { /*0X28*/ "dload_2", NULL },
    { /*0X29*/ "dload_3", NULL },
    { /*0X2a*/ "aload_0", NULL },
    { /*0X2b*/ "aload_1", NULL },
    { /*0X2c*/ "aload_2", NULL },
    { /*0X2d*/ "aload_3", NULL },
    { /*0X2e*/ "iaload", NULL },
    { /*0X2f*/ "laload", NULL },
    { /*0X30*/ "faload", NULL },
    { /*0X31*/ "daload", NULL },
    { /*0X32*/ "aaload", NULL },
    { /*0X33*/ "baload", NULL },
    { /*0X34*/ "caload", NULL },
    { /*0X35*/ "saload", NULL },
    { /*0X36*/ "istore", "v" },
    { /*0X37*/ "lstore", "v" },
    { /*0X38*/ "fstore", "v" },
    { /*0X39*/ "dstore", "v" },
    { /*0X3a*/ "astore", "v" },
    { /*0X3b*/ "istore_0", NULL },
    { /*0X3c*/ "istore_1", NULL },
    { /*0X3d*/ "istore_2", NULL },
    { /*0X3e*/ "istore_3", NULL },
    { /*0X3f*/ "lstore_0", NULL },
    { /*0X40*/ "lstore_1", NULL },
    { /*0X41*/ "lstore_2", NULL },
    { /*0X42*/ "lstore_3", NULL },
    { /*0X43*/ "fstore_0", NULL },
    { /*0X44*/ "fstore_1", NULL },
    { /*0X45*/ "fstore_2", NULL },
    { /*0X46*/ "fstore_3", NULL },
    { /*0X47*/ "dstore_0", NULL },
    { /*0X48*/ "dstore_1", NULL },
    { /*0X49*/ "dstore_2", NULL },
    { /*0X4a*/ "dstore_3", NULL },
    { /*0X4b*/ "astore_0", NULL },
    { /*0X4c*/ "astore_1", NULL },
    { /*0X4d*/ "astore_2", NULL },
    { /*0X4e*/ "astore_3", NULL },
    { /*0X4f*/ "iastore", NULL },
    { /*0X50*/ "lastore", NULL },
    { /*0X51*/ "fastore", NULL },
    { /*0X52*/ "dastore", NULL },
    { /*0X53*/ "aastore", NULL },
    { /*0X54*/ "bastore", NULL },
    { /*0X55*/ "castore", NULL },
    { /*0X56*/ "sastore", NULL },
    { /*0X57*/ "pop", NULL },
    { /*0X58*/ "pop2", NULL },
    { /*0X59*/ "dup", NULL },
    { /*0X5a*/ "dup_x1", NULL },
    { /*0X5b*/ "dup_x2", NULL },
    { /*0X5c*/ "dup2", NULL },
    { /*0X5d*/ "dup2_x1", NULL },
    { /*0X5e*/ "dup2_x2", NULL },
    { /*0X5f*/ "swap", NULL },
    { /*0X60*/ "iadd", NULL },
    { /*0X61*/ "ladd", NULL },
    { /*0X62*/ "fadd", NULL },
    { /*0X63*/ "dadd", NULL },
    { /*0X64*/ "isub", NULL },
    { /*0X65*/ "lsub", NULL },
    { /*0X66*/ "fsub", NULL },
    { /*0X67*/ "dsub", NULL },
    { /*0X68*/ "imul", NULL },
    { /*0X69*/ "lmul", NULL },
    { /*0X6a*/ "fmul", NULL },
    { /*0X6b*/ "dmul", NULL },
    { /*0X6c*/ "idiv", NULL },
    { /*0X6d*/ "ldiv", NULL },
    { /*0X6e*/ "fdiv", NULL },
    { /*0X6f*/ "ddiv", NULL },
    { /*0X70*/ "irem", NULL },
    { /*0X71*/ "lrem", NULL },
    { /*0X72*/ "frem", NULL },
    { /*0X73*/ "drem", NULL },
    { /*0X74*/ "ineg", NULL },
    { /*0X75*/ "lneg", NULL },
    { /*0X76*/ "fneg", NULL },
    { /*0X77*/ "dneg", NULL },
    { /*0X78*/ "ishl", NULL },
    { /*0X79*/ "lshl", NULL },
    { /*0X7a*/ "ishr", NULL },
    { /*0X7b*/ "lshr", NULL },
    { /*0X7c*/ "iushr", NULL },
    { /*0X7d*/ "lushr", NULL },
    { /*0X7e*/ "iand", NULL },
    { /*0X7f*/ "land", NULL },
    { /*0X80*/ "ior", NULL },
    { /*0X81*/ "lor", NULL },
    { /*0X82*/ "ixor", NULL },
    { /*0X83*/ "lxor", NULL },
    { /*0X84*/ "iinc", "vs" },
    { /*0X85*/ "i2l", NULL },
    { /*0X86*/ "i2f", NULL },
    { /*0X87*/ "i2d", NULL },
    { /*0X88*/ "l2i", NULL },
    { /*0X89*/ "l2f", NULL },
    { /*0X8a*/ "l2d", NULL },
    { /*0X8b*/ "f2i", NULL },
    { /*0X8c*/ "f2l", NULL },
    { /*0X8d*/ "f2d", NULL },
    { /*0X8e*/ "d2i", NULL },
    { /*0X8f*/ "d2l", NULL },
    { /*0X90*/ "d2f", NULL },
    { /*0X91*/ "i2b", NULL },
    { /*0X92*/ "i2c", NULL },
    { /*0X93*/ "i2s", NULL },
    { /*0X94*/ "lcmp", NULL },
    { /*0X95*/ "fcmpl", NULL },
    { /*0X96*/ "fcmpg", NULL },
    { /*0X97*/ "dcmpl", NULL },
    { /*0X98*/ "dcmpg", NULL },
    { /*0X99*/ "ifeq", "b" },
    { /*0X9a*/ "ifne", "b" },
    { /*0X9b*/ "iflt", "b" },
    { /*0X9c*/ "ifge", "b" },
    { /*0X9d*/ "ifgt", "b" },
    { /*0X9e*/ "ifle", "b" },
    { /*0X9f*/ "if_icmpeq", "b" },
    { /*0Xa0*/ "if_icmpne", "b" },
    { /*0Xa1*/ "if_icmplt", "b" },
    { /*0Xa2*/ "if_icmpge", "b" },
    { /*0Xa3*/ "if_icmpgt", "b" },
    { /*0Xa4*/ "if_icmple", "b" },
    { /*0Xa5*/ "if_acmpeq", "b" },
    { /*0Xa6*/ "if_acmpne", "b" },
    { /*0Xa7*/ "goto", "b" },
    { /*0Xa8*/ "jsr", "b" },
    { /*0Xa9*/ "ret", "v" },
    { /*0Xaa*/ "tableswitch", "&" },
    { /*0Xab*/ "lookupswitch", "*" },
    { /*0Xac*/ "ireturn", NULL },
    { /*0Xad*/ "lreturn", NULL },
    { /*0Xae*/ "freturn", NULL },
    { /*0Xaf*/ "dreturn", NULL },
    { /*0Xb0*/ "areturn", NULL },
    { /*0Xb1*/ "return", NULL },
    { /*0Xb2*/ "getstatic", "F" },
    { /*0Xb3*/ "putstatic", "F" },
    { /*0Xb4*/ "getfield", "F" },
    { /*0Xb5*/ "putfield", "F" },
    { /*0Xb6*/ "invokevirtual", "M" },
    { /*0Xb7*/ "invokespecial", "M" },
    { /*0Xb8*/ "invokestatic", "M" },
    { /*0Xb9*/ "invokeinterface", "Iu0" },
    { /*0Xba*/ "", NULL },
    { /*0Xbb*/ "new", "C" },
    { /*0Xbc*/ "newarray", "a" },
    { /*0Xbd*/ "anewarray", "T" },
    { /*0Xbe*/ "arraylength", NULL },
    { /*0Xbf*/ "athrow", NULL },
    { /*0Xc0*/ "checkcast", "T" },
    { /*0Xc1*/ "instanceof", "T" },
    { /*0Xc2*/ "monitorenter", NULL },
    { /*0Xc3*/ "monitorexit", NULL },
    { /*0Xc4*/ "wide", "w" },
    { /*0Xc5*/ "multianewarray", "Cu" },
    { /*0Xc6*/ "ifnull", "b" },
    { /*0Xc7*/ "ifnonnull", "b" },
    { /*0Xc8*/ "goto_w", "B" },
    { /*0Xc9*/ "jsr_w", "B" },
    { /*0Xca*/ "breakpoint", NULL },
    { /*0Xcb*/ "", NULL },
    { /*0Xcc*/ "", NULL },
    { /*0Xcd*/ "", NULL },
    { /*0Xce*/ "", NULL },
    { /*0Xcf*/ "", NULL },
    { /*0Xd0*/ "", NULL },
    { /*0Xd1*/ "", NULL },
    { /*0Xd2*/ "", NULL },
    { /*0Xd3*/ "", NULL },
    { /*0Xd4*/ "", NULL },
    { /*0Xd5*/ "", NULL },
    { /*0Xd6*/ "", NULL },
    { /*0Xd7*/ "", NULL },
    { /*0Xd8*/ "", NULL },
    { /*0Xd9*/ "", NULL },
    { /*0Xda*/ "", NULL },
    { /*0Xdb*/ "", NULL },
    { /*0Xdc*/ "", NULL },
    { /*0Xdd*/ "", NULL },
    { /*0Xde*/ "", NULL },
    { /*0Xdf*/ "", NULL },
    { /*0Xe0*/ "", NULL },
    { /*0Xe1*/ "", NULL },
    { /*0Xe2*/ "", NULL },
    { /*0Xe3*/ "", NULL },
    { /*0Xe4*/ "", NULL },
    { /*0Xe5*/ "", NULL },
    { /*0Xe6*/ "", NULL },
    { /*0Xe7*/ "", NULL },
    { /*0Xe8*/ "", NULL },
    { /*0Xe9*/ "", NULL },
    { /*0Xea*/ "", NULL },
    { /*0Xeb*/ "", NULL },
    { /*0Xec*/ "", NULL },
    { /*0Xed*/ "", NULL },
    { /*0Xee*/ "", NULL },
    { /*0Xef*/ "", NULL },
    { /*0Xf0*/ "", NULL },
    { /*0Xf1*/ "", NULL },
    { /*0Xf2*/ "", NULL },
    { /*0Xf3*/ "", NULL },
    { /*0Xf4*/ "", NULL },
    { /*0Xf5*/ "", NULL },
    { /*0Xf6*/ "", NULL },
    { /*0Xf7*/ "", NULL },
    { /*0Xf8*/ "", NULL },
    { /*0Xf9*/ "", NULL },
    { /*0Xfa*/ "", NULL },
    { /*0Xfb*/ "", NULL },
    { /*0Xfc*/ "", NULL },
    { /*0Xfd*/ "", NULL },
    { /*0Xfe*/ "impdep1", NULL },
    { /*0Xff*/ "impdep2", NULL }
};

char *GetOpcodeName( int op ) {
    if (op < 0 || op > 255) return "???";
    return OPCodeFormat[op].name;
}

static int uget2(uint8_t *code, int pc) {
    return ((code[pc]<<8) + code[pc+1]) & 0xFFFF;
}


static int iget2(uint8_t *code, int pc) {
    int16_t r = ((code[pc]<<8) + code[pc+1]) & 0xFFFF;
    return (int)r;
}

static unsigned int iget4(uint8_t *code, int pc) {
    return ((code[pc]<<24) + (code[pc+1]<<16) + (code[pc+2]<<8) + code[pc+3]) & 0xFFFFFFFF;
}


static char *basicType[12] = {
    "???",  "???", "???", "???",
    "boolean", "char", "float", "double",
    "byte", "short", "int", "long"
};


void PrintByteCode( ClassFile *cf, uint8_t *code, int len ) {
    int ix = 0;
    while(ix < len) {
        int opcodeAddr = ix;
        uint8_t op = code[ix++];
        char *opname = OPCodeFormat[op].name;
        char *fmt = OPCodeFormat[op].opnds_format;
        printf("    %3d: %s", opcodeAddr, opname);
        if (fmt != NULL) {
            int opndCnt = 0;
            int dest, low, high, npairs;
            int cpref = 0;
            char *cptype = NULL;
            while(*fmt != '\0') {
                char f = *fmt++;
                if (f != '0') {
                    if (opndCnt++ > 0) putchar(',');
                    putchar(' ');
                }
                switch(f) {
                case 'v': /* one byte index to a local variable */
                    printf(" %d", code[ix++]);
                    break;
                case 'T': /* type (class/array/interface) ref,
                            as a two byte index into the constant pool */
                    cpref = uget2(code,ix);
                    cptype = "Type";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'C': /* class ref, as a two byte index into the constant pool */
                    cpref = uget2(code,ix);
                    cptype = "Class";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'u': /* one byte unsigned immediate integer */
                    printf(" %d", code[ix]);
                    ix++;
                    break;
                case 's': /* one byte signed immediate integer */
                    printf(" %d", (signed char)code[ix]);
                    ix++;
                    break;
                case 'F': /* field ref, as a two byte index into the constant pool */
                    cpref = uget2(code,ix);
                    cptype = "Field";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'I': /* interface ref, as a two byte index into the constant pool */
                    cpref = uget2(code,ix);
                    cptype = "Interface";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'b': /* two byte branch destination */
                    dest = opcodeAddr + iget2(code,ix);
                    printf(" %d", dest);
                    ix += 2;
                    break;
                case 'B': /* four byte branch destination */
                    printf(" %d", (int)(opcodeAddr + iget4(code,ix)));
                    ix += 4;
                    break;
                case 'M': /* method ref, as a two byte index into the constant pool */
                    cpref = uget2(code,ix);
                    cptype = "Method";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'k': /* simple constant in constant pool, one byte index */
                    cpref = code[ix];
                    cptype = " ";
                    printf(" #%d", cpref);
                    ix++;
                    break;
                case 'K': /* simple constant in constant pool, two byte index */
                    cpref = uget2(code,ix);
                    cptype = " ";
                    printf(" #%d", cpref);
                    ix += 2;
                    break;
                case 'a': /* one byte primitive type */
                    dest = code[ix] & 0xff;
                    if (dest >= 12) dest = 0;
                    printf(" %s", basicType[dest]);
                    ix++;
                    break;
                case 'S': /* two byte signed immediate integer */
                    printf(" %d", iget2(code,ix));
                    ix += 2;
                    break;
                case '*': /* lookupswitch */
                    ix = (ix + 3) & 0xFFFFFFFC;  /* skip over padding bytes */
                    printf("\n          default: %d\n",
                        (int)(opcodeAddr + iget4(code,ix)));
                    ix += 4;
                    npairs = iget4(code,ix);  ix += 4;
                    while(npairs-- > 0) {
                        printf("          case %d:", iget4(code,ix));  ix += 4;
                        printf(" %d\n", opcodeAddr + iget4(code,ix));  ix += 4;
                    }
                    break;
                case '&': /* tableswitch */
                    ix = (ix + 3) & 0xFFFFFFFC;  /* skip over padding bytes */
                    printf("\n          default: %d\n",
                        opcodeAddr + iget4(code,ix));
                    ix += 4;
                    low  = iget4(code,ix);  ix += 4;
                    high = iget4(code,ix);  ix += 4;
                    while(low <= high) {
                        printf("          case %d:", low++);
                        printf(" %d\n", opcodeAddr + iget4(code,ix));  ix += 4;
                    }
                    break;
                case 'w':  /* wide */
                    op = code[ix++];  /* the op being modified by wide */
                    printf("-%s %d", OPCodeFormat[op].name, uget2(code,ix));
                    ix += 2;
                    if (op == 0x84) { /* iinc -- Format 2 */
                        printf(", %d", iget2(code,ix));
                        ix += 2;
                    }
                    break;
                case '0': /* a zero byte */
                    ix++;
                    break;
                default:
                    fprintf(stderr, "bad operand format code; op=%s, code=%c\n",
                        opname, fmt[-1]);
                    printf(" ???\n");
                    return;
                }
            }
            if (cptype != NULL)
                printf(";  //%s %s", cptype, GetCPItemAsString(cf, cpref));
        }
        putchar('\n');
    }
}

