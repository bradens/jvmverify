/* PrintClassFile.c */

/*
    Prints all the information for a class in a format
    similar to that produced by the command
       javap -c -verbose foo.class
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "ClassFileFormat.h"
#include "MyAlloc.h"
#include "PrintClassFile.h"
#include "PrintByteCode.h"



char *CP_Tagname[] = {
    "??", "UTF8", "??", "Integer", "Float", "Long", "Double", "Class",
    "String", "Field", "Method", "Interface", "NameAndType"
};


void PrintUTF8( uint8_t *s ) {
    int len;
    if (s == NULL) {
        printf("*NULL*");
        return;
    }
    len = (s[0]<<8) + s[1];
    s += 2;
    putchar('\"');
    while(len-- > 0) {
        int c = (*s++) & 0xFF;
        switch(c) {
        case '\"':
        case '\\':
        case '\'':
            putchar('\\');  break;
        case '\n':
            putchar('\\');  c = 'n';  break;
        case '\r':
            putchar('\\');  c = 'r';  break;
        case '\t':
            putchar('\\');  c = 't';  break;
        case '\b':
            putchar('\\');  c = 'b';  break;
        case '\f':
            putchar('\\');  c = 'f';  break;
        default:
            if (!isprint(c)) {
                printf("\\x%2x", c);
                continue;
            }
            break;
        }
        putchar(c);
    }
    putchar('\"');
}


void PrintConstantPool( ClassFile *cf ) {
    int ix;
    printf("\n    Constant Pool\n    =============\n\n");
    for( ix=1;  ix<cf->constant_pool_count;  ix++ ) {
        ConstantPoolTag t = cf->cp_tag[ix];
        ConstantPoolItem *cpi = &cf->cp_item[ix];
        char *s = GetCPItemAsString(cf,ix);
        if (t > 12) t = 0;
        printf("    #%d:   %s ", ix, CP_Tagname[t]);
        switch(t) {
        case CP_Field:
        case CP_Method:
        case CP_Interface:
            printf("#%d.#%d  %s\n", cpi->ss.sval1, cpi->ss.sval2, s);
            break;
        case CP_NameAndType:
            printf("#%d:#%d  %s\n", cpi->ss.sval1, cpi->ss.sval2, s);
            break;
        case CP_UTF8:
            PrintUTF8(cpi->sval);
            putchar('\n');
            break;
        case CP_Double:
        case CP_Long:
            ix++;
            printf("%s\n", s);
            break;
        default:
            printf("%s\n", s);
            break;
        }
        SafeFree(s);
    }
}

void PrintMethod( ClassFile *cf, int ix ) {
    method_info *m = &(cf->methods[ix]);
    char *s = GetCPItemAsString(cf, m->name_index);
    printf("\n   Method %s\n", s);
    SafeFree(s);
    s = GetCPItemAsString(cf, m->descriptor_index);
    printf("      signature = %s\n", s);
    SafeFree(s);
    printf("      args_size=%d, max_stack=%d, max_locals=%d\n",
        (int)m->nArgs, (int)m->max_stack, (int)m->max_locals);
    printf("   Code:\n");
    PrintByteCode(cf, m->code, m->code_length);
}

void PrintInterfaces( ClassFile *cf, int ifcnt, uint16_t *ip ) {
    if (ifcnt == 0) return;
    printf("\n interfaces implemented:\n");
    while(ifcnt-- > 0) {
        uint16_t ix = *ip++;
        // ConstantPoolTag t = cf->cp_tag[ix];
        char *s = GetCPItemAsString(cf, ix);
        printf(" %s\n", s);
        SafeFree(s);
    }
    printf("\n");
}

void PrintClassFile( ClassFile *cf ) {
    char *s;
    int i;
    
    if (cf == NULL) {
        printf("Null\n");
        return;
    }
    printf("Class %s:\n", cf->cname);
    s = GetCPItemAsString(cf, cf->super_class);
    printf("    parent = %s\n", s);
    SafeFree(s);
    PrintConstantPool(cf);
    PrintInterfaces(cf, cf->interfaces_count, cf->interfaces);
    for( i=0;  i<cf->methods_count;  i++ ) {
        PrintMethod(cf, i);
    }
}
