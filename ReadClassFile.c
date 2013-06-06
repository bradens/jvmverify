/* ReadClassFile.c */

/*
   Reads a class from a file, building a representation in memory as an
   instance of the ClassFile struct.

   Attributes other than those explicitly needed by the MyJVM program
   are ignored.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "ClassFileFormat.h"
#include "ReadClassFile.h"
#include "MyAlloc.h"


typedef struct FileNameListItem {
        char *filename;
        int cnt;  // number of tries
        struct FileNameListItem *next;
    } *FileNameList;

static FileNameList filesRead = NULL;  // list of class files we tried to read


void PrintFilesRead() {
    if (filesRead == NULL) {
        printf("\nNo class files read\n");
        return;
    }
    FileNameList fnp;
    printf("\nRequests to Read Class Files...\n");
    for( fnp = filesRead;  fnp != NULL;  fnp = fnp->next ) {
        printf("    (%d times): %s\n", fnp->cnt, fnp->filename);
    }
}


static uint32_t ReadU4(FILE *f) {
    uint32_t r = 0;
    r = fgetc(f) & 0xff;
    r = (r << 8) | (fgetc(f) & 0xff);
    r = (r << 8) | (fgetc(f) & 0xff);
    r = (r << 8) | (fgetc(f) & 0xff);
    return r;
}


static uint16_t ReadU2(FILE *f) {
    uint16_t r = 0;
    r = fgetc(f) & 0xff;
    r = (r << 8) | (fgetc(f) & 0xff);
    return r;
}


static void ReadConstantPool(FILE *f, ClassFile *cf) {\
    uint16_t cnt;
    int i;
    ConstantPoolTag t;
    uint8_t *s;
    int len;

    cf->constant_pool_count = cnt = ReadU2(f);
    cf->cp_tag = SafeCalloc(cnt,sizeof(uint8_t));
    cf->cp_item = SafeCalloc(cnt,sizeof(ConstantPoolItem));
    for( i=1; i<cnt; i++ ) {
        t = (ConstantPoolTag)fgetc(f);
        cf->cp_tag[i] = (uint8_t)t;
        switch(t) {
        case CP_UTF8:
            len = ReadU2(f);
            // We allocate an extra null byte at end of the string.
            // This allows most UTF8 strings to be treated as regular
            // ASCII strings in C.
            cf->cp_item[i].sval = s = SafeMalloc(len+3);
            *s++ = (len >> 8);
            *s++ = len & 0xff;
            while(len-- > 0)
                *s++ = fgetc(f);
            *s = 0;
            break;
        case CP_Integer:
        case CP_Float:
            cf->cp_item[i].ival = ReadU4(f);
            break;
        case CP_Long:
        case CP_Double:
            cf->cp_item[i+1].ival = ReadU4(f);
            cf->cp_item[i].ival   = ReadU4(f);
            cf->cp_tag[++i] = (uint8_t)t;
            break;
        case CP_Class:
        case CP_String:
            cf->cp_item[i].ival = ReadU2(f);
            break;
        case CP_Field:
        case CP_Method:
        case CP_Interface:
        case CP_NameAndType:
            cf->cp_item[i].ss.sval1 = ReadU2(f);
            cf->cp_item[i].ss.sval2 = ReadU2(f);
            break;
        default:
            cf->cp_tag[i] = CP_Unknown;
            cf->cp_item[i].ival = 0;
            break;
        }
    }
}


static void ReadInterfaces(FILE *f, ClassFile *cf) {
    int cnt;
    uint16_t *ip;
    cf->interfaces_count = cnt = ReadU2(f);
    cf->interfaces = ip = SafeCalloc(cnt,2);
    while(cnt-- > 0) 
        *ip++ = ReadU2(f);
}


// Reads up to 3 different attributes
static void ReadAttributes(FILE *f, ClassFile *cf, ... ) {
    int acnt;
    va_list argp;
    char *name[3];
    uint32_t *length[3];
    uint8_t **where[3];
    int num = 0;

    acnt = ReadU2(f);
    if (acnt == 0) {
        return;
    }
    va_start(argp, cf);
    for( ; ; ) {
        assert(num < 3);
        name[num] = va_arg(argp, char*);
        if (name[num] == NULL) break;
        length[num] = va_arg(argp, uint32_t*);
        where[num] = va_arg(argp, uint8_t**);
        *where[num] = NULL;
        num++;
    }
    va_end(argp);
    while(acnt-- > 0) {
        uint16_t ix = ReadU2(f);
        uint8_t *ap = NULL;
        int len = ReadU4(f);
        int i;
        for( i=0; i<num; i++ ) {
            // look up the attribute name in the constant pool
            char *s = GetUTF8(cf,ix);
            if (strcmp(s,name[i]) == 0) {
                /* this is an attribute we want */
                *length[i] = len;
                *where[i] = ap = SafeMalloc(len);
                fread(ap, 1, len, f);
                break;
            }
        }
        if (ap == NULL && len > 0)
            /* it's an attribute we ignore */
            fseek(f,len,SEEK_CUR);
    }
}


static void ReadFields(FILE *f, ClassFile *cf) {
    int cnt;
    field_info *ip;
    uint32_t attr_len;
    uint8_t *attr;

    cf->fields_count = cnt = ReadU2(f);
    cf->fields = ip = SafeCalloc(cnt, sizeof(field_info));
    while(cnt-- > 0) {
        ip->access_flags = ReadU2(f);
        ip->name_index = ReadU2(f);
        ip->descriptor_index = ReadU2(f);
        ip->constantValue_index = 0;
        attr = (uint8_t*)(&ip->constantValue_index);
        ReadAttributes(f, cf, "ConstantValue", &attr_len, &attr, NULL);
        ip++;
    }
}


int CountParameters( uint8_t *s ) {
    int result = 0;

    assert(*s == '(');
    s++;
    while(*s != '\0') {
        switch(*s++) {
        case 'B':
        case 'C':
        case 'F':
        case 'I':
        case 'S':
        case 'Z':
            result++;
            break;
        case 'D':
        case 'J':
            result += 2;  // these types take 2 slots
            break;
        case 'L':
            result++;
            while(*s++ != ';')
                ;
            break;
        case '[':
            while(*s == '[')
                s++;
            if (*s == 'L') {
                while(*s++ != ';')
                    ;
            } else if (*s != '\0')
                s++;
            result++;
            break;
        case ')':
            return result;
        }   
    }
    /* should not be reached */
    assert(*s == ')');
    return result;
}


static void ReadMethods(FILE *f, ClassFile *cf) {
    int cnt;
    method_info *ip;
    uint32_t attr_len;
    uint8_t *attr;

    cf->methods_count = cnt = ReadU2(f);
    cf->methods = ip = SafeCalloc(cnt, sizeof(method_info));
    while(cnt-- > 0) {
        uint8_t *ptr;
        int ix, dix;
        ConstantPoolItem *cpi;
    
        ip->access_flags = ReadU2(f);
        ip->name_index = ReadU2(f);
        ip->descriptor_index = ReadU2(f);
        attr = NULL;
        attr_len = 0;
        ReadAttributes(f, cf, "Code", &attr_len, &attr, NULL);
        if (attr != NULL && attr_len > 0) {
            ix = 0;
            ip->max_stack = (attr[ix]<<8)+attr[ix+1];
            ix += 2;
            ip->max_locals = (attr[ix]<<8)+attr[ix+1];
            ix += 2;
            ip->code_length = (attr[ix]<<24)+(attr[ix+1]<<16)+
                 (attr[ix+2]<<8)+attr[ix+3];
            ix += 4;
            if (ip->code_length > 0) {
                ip->code = ptr = SafeMalloc(ip->code_length);
                memcpy(ptr, attr+ix, ip->code_length);
            } else
                ip->code = NULL;
            ix += ip->code_length;
            ip->exception_table_length = (attr[ix]<<8) + attr[ix+1];
            ix += 2;
            if (ip->exception_table_length > 0) {
                ip->exception_table = ptr = SafeMalloc(ip->exception_table_length);
                memcpy(ptr, attr+ix, ip->exception_table_length);
            } else
                ip->exception_table = NULL;
            ix += ip->exception_table_length;
            ip->attributes_count = (attr[ix]<<8) + attr[ix+1];
            ix += 2;
            if (ip->attributes_count > 0) {
                ip->attributes = ptr = SafeMalloc(ip->attributes_count);
                memcpy(ptr, attr+ix, ip->attributes_count);
            } else
                ip->attributes = NULL;
            SafeFree(attr);
        }
        /* extra analysis needed for run-time */
        dix = ip->descriptor_index;
        cpi = &cf->cp_item[dix];
        ip->nArgs = CountParameters(cpi->sval+2);
        if (!(ip->access_flags & ACC_STATIC))
            ip->nArgs += 1;
        ip++;
    }
}


ClassFile *ReadClassFile( char *classname ) {
    FILE *f;
    ClassFile *result;
    uint16_t t1;
    char *filename;
    FileNameList fnp;

    filename = SafeMalloc(strlen(classname)+7);
    strcpy(filename,classname);
    strcat(filename,".class");

    // Check if we have already tried to read this file
    for( fnp = filesRead;  fnp != NULL;  fnp = fnp->next ) {
        if (strcmp(fnp->filename,filename) == 0) {
            SafeFree(filename);
            fnp->cnt++;
            return NULL;  // we have tried to read it before
        }
    }
    fnp = SafeMalloc(sizeof(*fnp));
    fnp->filename = filename;
    fnp->cnt = 1;
    fnp->next = filesRead;
    filesRead = fnp;

    f = fopen(filename, "rb");
    if (f == NULL) {
        // Our interpreter simply does not support loading of built-in
        // classes, so suppress the error message in this case
        if (strncmp(filename, "java/", 5) != 0)
            fprintf(stderr, "Unable to read file %s\n", filename);
        return NULL;
    }
    if (ReadU4(f) != MagicNumber) {
        fprintf(stderr, "File %s does not begin with magic number\n", filename);
        exit(1);
    }
    result = SafeMalloc(sizeof(ClassFile));
    t1 = ReadU2(f);  // minor version
    t1 = ReadU2(f);  // major version
    ReadConstantPool(f,result);
    result->access_flags = ReadU2(f);
    result->this_class = ReadU2(f);
    result->super_class = ReadU2(f);
    ReadInterfaces(f,result);
    ReadFields(f,result);
    ReadMethods(f,result);
    ReadAttributes(f, result, NULL);
    result->cname = GetCPItemAsString(result,result->this_class);
    fclose(f);
    return result;
}

