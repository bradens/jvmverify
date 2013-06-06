/* ClassFileFormat.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

#include "MyAlloc.h"
#include "ClassFileFormat.h"


/* Returns a UTF8 string from position ix of the constant pool
   of classfile cf.
   The referenced constant must have the UTF8 tag.  */
char *GetUTF8( ClassFile *cf, int ix ) {
    uint8_t *r;
    if (ix <= 0 || ix > cf->constant_pool_count)
        return NULL;
    if (cf->cp_tag[ix] != CP_UTF8)
        return NULL;
    r = (cf->cp_item[ix]).sval;
    return (char *)(r+2);
}


/* Returns a string representation of constant number ix in the
   constant pool of classfile cf.
   The string is returned as new storage allocated on the heap.
   The caller must eventually free this storage, or else there
   will be a memory leak. */
char *GetCPItemAsString( ClassFile *cf, int ix ) {
    uint8_t *r;
    char temp[32], *s1, *s2, *s3;
    static int depth=0;
    union { double d; int64_t ll; uint32_t uval[2]; } pair;

    if (ix <= 0 || ix > cf->constant_pool_count)
        return NULL;
    assert(depth <= 3);
    ConstantPoolItem *cpi = &cf->cp_item[ix];
    switch(cf->cp_tag[ix]) {
        case CP_UTF8:
            r = cpi->sval;
            return SafeStrdup((char *)(r+2));
        case CP_Integer:  // PRId32 is defined in inttypes.h
            sprintf(temp, "%" PRIi32, cpi->ival);
            break;
        case CP_Float:
            sprintf(temp, "%f", cpi->fval);
            break;
        case CP_Long:  // PRId64 is defined in inttypes.h
            pair.uval[0] = cpi->ival;
            pair.uval[1] = (cpi+1)->ival;
            sprintf(temp, "%" PRId64, pair.ll);
            break;
        case CP_Double:
            pair.uval[0] = cpi->ival;
            pair.uval[1] = (cpi+1)->ival;
            sprintf(temp, "%lf", pair.d);
            break;
        case CP_Class:
        case CP_String:
            depth++;
            s1 = GetCPItemAsString(cf,cpi->ival);
            depth--;
            return s1;
        case CP_Field:
        case CP_Method:
        case CP_Interface:
            depth++;
            s1 = GetCPItemAsString(cf,cpi->ss.sval1);
            s2 = GetCPItemAsString(cf,cpi->ss.sval2);
            s3 = SafeMalloc(strlen(s1)+strlen(s2)+2);
            sprintf(s3,"%s.%s",s1,s2);
            SafeFree(s1); SafeFree(s2);
            depth--;
            return s3;
        case CP_NameAndType:
            depth++;
            s1 = GetCPItemAsString(cf,cpi->ss.sval1);
            s2 = GetCPItemAsString(cf,cpi->ss.sval2);
            s3 = SafeMalloc(strlen(s1)+strlen(s2)+2);
            sprintf(s3,"%s:%s",s1,s2);
            SafeFree(s1); SafeFree(s2);
            depth--;
            return s3;
        default:
            sprintf(temp,"*unknown CP tag (%d)*", cf->cp_tag[ix]);
            break;
    }
    return SafeStrdup(temp);
}

