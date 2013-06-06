/* VerifierUtils.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ClassFileFormat.h"
#include "ClassResolver.h"
#include "OpcodeSignatures.h"
#include "TraceOptions.h"
#include "MyAlloc.h"
#include "VerifierUtils.h"



// Frees the storage for a type descriptor, but only for a
// heap datatype (letter A code).
void FreeTypeDescriptor( char *s ) {
    if (s == NULL) return;
    if (s[0] == 'A') // we allocated storage for this descriptor
        SafeFree(s);
    // else we used string constants for these other descriptors
    // so don't attempt to free their storage
}


// Input: a string for a type description in the JVM classfile
//   format, and a reference to a char* variable to receive
//   the typecode description in the verifier's format.
// Results:
// A. The variable *resultp contains a type description.
//   1.  For simple types, the description is one of the strings
//         I" "F" "Ll" "Dd" for int, float, long and double
//   2. For a class type, the description takes the form
//        "RL<classname>;
//   3. For an array type, the description takes the form
//        "R[<elementType>;
//   where <classname> is a fully qualified class name,
//   such as java/io/PrintStream, and <elementType> is one
//   of the 3 possibilities listed above (yes, this is a
//   recursive definition!)
//   4. For a void method, the result type is coded as
//        "-"
// B. The returned result is a pointer to the character
//   which immediately follows the last character of the
//   JVM type description which was used.
char *ExtractOneType( char **resultp, char *jvmType ) {
    int len;
    char *newString;
    char *temp;
    switch(*jvmType++) {
    case 'B':   // byte
    case 'C':   // char
    case 'Z':   // boolean
    case 'S':   // short
    case 'I':   // int
        *resultp = "I";
        break;
    case 'J':   // long
        *resultp = "Ll";
        break;
    case 'F':   // float
        *resultp = "F";
        break;
    case 'D':   // double
        *resultp = "Dd";
        break;
    case 'L':   // class
        temp = jvmType;
        jvmType = strchr(jvmType, ';')+1;
        assert(jvmType != NULL);
        len = jvmType-temp;
        newString = SafeMalloc(len+2);
        newString[0] = 'A';
        strncpy(newString+1, temp-1, len);
        *resultp = newString;
        break;
    case '[':   // array
        jvmType = ExtractOneType(&temp, jvmType);  // recurse!
        *resultp = SafeMalloc(strlen(temp)+3);
        strcat(strcpy(*resultp, "A["), temp);
        FreeTypeDescriptor(temp);
        break;
    case 'V':   // void type (only used as method result)
        *resultp = "-";
        break;
    default:
        assert(0);  // force a fatal error
    }
    return jvmType;
}


// Input arguments
//    sig: a method signature sig,
//    argsp: an array which provides one element for each formal parameter
//           of the method,
//    retTypep: a pointer to a string variable
// Result:
//    argsp[i] is assigned a description of the type of the i-th formal
//        parameter as represented on the JVM stack, for each i.
//    *retTypep is assigned a description of the result type of the method
//    And the function result is the number of formal parameters obtained
//    from the signature.  (Note: instance methods have an implicit extra
//    parameter in first position which is the 'this' pointer.)
// See ExtractOneType function for a description of how types are
// represented by strings.
int ExtractTypesFromSignature( char **argsp, char **retTypep, char *sig ) {
    int cnt = 0;
    assert(*sig == '(');
    sig++;
    while(*sig != ')') {
        sig = ExtractOneType(argsp++, sig);
        cnt++;
    }
    ExtractOneType(retTypep, sig+1);
    return cnt;
}


// Given a method, this function returns an array of strings which
// represent the initial contents of the local variables and stack.
// See the ExtractOneType function for a description of the string
// format.
char **MapSigToInitState( ClassFile *cf, method_info *m, char **retTypep ) {
    int i, size;
    char **result, **rp, *sig;

    size = m->max_locals + m->max_stack;

    result = SafeCalloc(size, sizeof(char*));
    for( i = 0;  i < m->max_locals;  i++ )
        result[i] = "U";
    while( i < size)
        result[i++] = "-";

    if ((m->access_flags & ACC_STATIC) == 0) {
        // fill in result[0] with type of 'this'
        int len = strlen(cf->cname);
        result[0] = SafeMalloc(len+3);
        strcat(strcpy(result[0], "AL"), cf->cname);
        rp = result+1;
    } else {
        rp = result;
    }

    sig = GetCPItemAsString(cf, m->descriptor_index);
    ExtractTypesFromSignature(rp, retTypep, sig);

    SafeFree(sig);
    return result;
}


// Given the immediate operand of an invoke instruction (an index into the
// constant pool), this function returns a list of typecode descriptions for
// the method's formal parameters and the method result.
//
// Input parameters
//    cf: a reference to the class file which contains the invoke instruction
//    ix: an index into the constant pool of the class file
//    isStatic: 1 if the opcode is invokestatic and 0 otherwise
//    retTypep: a pointer to a string variable which will receive the typecode
//         description of the method's result type
//    cntp: a reference to an int variable which will receive the number of
//         formal parameters (this count includes the implicit extra
//         parameter to pass a class instance pointer)
//  Result
//     *retTypep is assigned a description of the method return type
//     *cntp is assigned a count of the number of formal parameters
//        including an implicit instance pointer (if the method is not static)
//     A reference to an array of strings, where the i-th element is a
//     description of the i-th formal paramter, is the function result.
//     The number of elements in the array is *cntp.
//
// IMPORTANT NOTE: the function result is a pointer to statically allocated
// memory. So ...
// 1. The next call to this function will overwrite an earlier result.
// 2. Do not attempt to free the storage that holds the result, that would
//    cause a crash.
//
char **AnalyzeInvoke( ClassFile *cf, int ix, int isStatic, char **retTypep, int *cntp ) {
    static char *result[256]; // max number of parameters is 256
    char *dummy;
    int i, cnt;
    char *sig = GetCPItemAsString(cf, ix);
    
    // clear the array
    for( i = 0;  i < 256; i++)
        result[0] = NULL;

    if (!isStatic) {
        // local #0 is an implicit 'this' argument
        cnt = ExtractTypesFromSignature(result+1, result, sig);
        cnt++;
    } else {
        cnt = ExtractTypesFromSignature(result, &dummy, sig);
    }
    SafeFree(sig);
    *cntp = cnt;
    return result;
}


static char *getClassName( ClassType *ctp ) {
    if (ctp == NULL)
        return "java/lang/Object";
    if (ctp->isArrayType) return NULL;
    ClassFile *cfp = ctp->cf;
    return cfp->cname;
}



static ClassType **ancestorTypesC( ClassType *ctp ) {
    static ClassType *parents[32];
    int cnt = 0;
    while(ctp != NULL) {
        parents[cnt++] = ctp;
        ctp = ctp->parent;
    }
    parents[cnt++] = NULL;  // represents java.lang.object
    return parents;
}


// Given a type descriptor in the verifier's format, this function creates
// a list of all its ancestor types starting with the current type.
// The function result is a pointer to an array of strings, where element
// 0 is the initial type descriptor, element 1 is the parent of element 0,
// ... element i+1 is the parent of element i.
// The last element is always the string "ALjava/lang/Object".
// The int variable referenced by *cntp is set to the number of
// strings and number of elements in the array.
// Note: the result array and all strings in it have their storage
// dynamically allocated.  The caller should deallocate the storage
// via a call to FreeTypeDescriptorArray.
char **AncestorTypes( char *typedescr, int *cntp ) {
    static char *parents[32];     // we limit number of ancestors to 32
    char *endPos, *s, **result;
    ClassFile *cfp;
    ClassType *ct1;
    int cnt = 0;
    int len, i;

    if (typedescr[1] == 'L') {
        parents[cnt++] = typedescr;
        typedescr += 2;
        for( ct1 = FirstLoadedClass;  ct1 != NULL;  ct1 = ct1->nextClass ) {
            if (ct1->isArrayType) continue;
            endPos = strchr(typedescr,';');
            assert(endPos != NULL);
            len = endPos - typedescr;
            cfp = ct1->cf;
            if (strncmp(cfp->cname, typedescr, len) == 0) {
                // found it!
                ClassType **cparents = ancestorTypesC(ct1);
                for( cnt = 1;  ;  cnt++ ) {
                    ClassType *ct2 = cparents[cnt];
                    if (ct2 == NULL) break;
                    s = getClassName(ct2);
                    len = strlen(s);
                    parents[cnt] = strcat(strcpy(SafeMalloc(len+3), "AL"), s);
                }
                break;
            }       
        }
        parents[cnt++] = SafeStrdup("ALjava/lang/Object");
        *cntp = cnt;
        result = SafeCalloc(cnt, sizeof(char *));
        for( i = 0; i < cnt; i++ )
            result[i] = parents[i];
        return result;
    }
    if (typedescr[1] == '[') {
        AncestorTypes(typedescr+2, &cnt);
        parents[0] = typedescr;
        for( i = 1;  i < cnt;  i++ ) {
            s = parents[i];
            len = strlen(s);
            parents[i] = strcat(strcpy(SafeMalloc(len+3), "A["), s);
            FreeTypeDescriptor(s);
        }
        assert(cnt<256);
        parents[cnt++] = SafeStrdup("ALjava/lang/Object");
        *cntp = cnt;
        return parents;
    }
    *cntp = 0;
    return NULL;
}

// Given two type descriptors for reference types (i.e. the initial code
// letter is 'A'), return the type descriptor for their lub in the
// lattice of types.
// Note: the result is a string whose storage is allocated on the heap.
// The caller should eventually deallocate it using SafeFree.
char *LUB( char *type1, char *type2 ) {
    int cnt1, cnt2, i, j;
    char **path1, **path2, *result;
    if (type1[0] != 'A' || type2[1] != 'A')
        return "X";
    path1 = AncestorTypes(type1, &cnt1);
    path2 = AncestorTypes(type2, &cnt2);
    result = "ALjava/lang/Object";  // default result
    i = cnt1;  j = cnt2;
    while( i-- > 0 && j-- > 0 ) {
        if (strcmp(path1[i], path2[j]) != 0) break;
        result = path1[i];
    }
    result = SafeStrdup(result);
    FreeTypeDescriptorArray(path1, cnt1);
    FreeTypeDescriptorArray(path2, cnt2);
    return result;
}


// Given the immediate operand of a getfield or putfield instruction (which
// is an index into the constant pool), this function returns a string which
// represents the datatype of that field.
char *FieldTypeCode( ClassFile *cf, int ix ) {
    char *result = "";
    char *s = GetCPItemAsString(cf, ix);
    char *rp = strrchr(s, ':'); // find last occurrence of ':'
    assert(rp != NULL);
    ExtractOneType(&result, rp+1);
    SafeFree(s);
    return result;
}


// Given an array of type descriptors as used by the verifier, this
// function frees all the storage for the array and the constituent
// strings (but only strings describing heap datatypes).
void FreeTypeDescriptorArray( char **ap, int numSlots ) {
    while( --numSlots >= 0 ) {
        FreeTypeDescriptor(ap[numSlots]);
    }
    SafeFree(ap);
}
