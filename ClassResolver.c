/* ClassResolver.c */

/*
   This module provides functions associated with resolving class
   references.
   
   The functions are in three groups:

   * InvokeMethod  -- begins execution of a method's bytecode once
                      the class has been resolved and method found
   * InvokeStaticMethod  -- implements JVM op invokestatic
   * InvokeSpecialMethod -- implements JVM op invokespecial
   * InvokeVirtualMethod -- implements JVM op invokevirtual

   * LoadClass  -- attempts to load a class from a disk file

   * GetStatic  -- implements JVM op getstatic
   * PutStatic  -- implements JVM op getstatic
   * GetField   -- implements JVM op getfield
   * PutField   -- implements JVM op putfield
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "ClassFileFormat.h"
#include "ReadClassFile.h"
#include "jvm.h"
#include "InterpretLoop.h"
#include "NativeClasses.h"
#include "TraceOptions.h"
#include "MyAlloc.h"
#include "ClassResolver.h"

ClassType *FirstLoadedClass = NULL;  /* list of loaded classes or array types in use */


/* For a class identified by cf, this returns the number of static (class) variables
   and the number of instance variables */
static void getNumClassVars( ClassFile *cf, int *numClassVars, int *numInstVars ) {
    int n = cf->fields_count;
    int result[2];
    field_info *fp = cf->fields;
    ConstantPoolItem *cpi;
    char c;
    int dix;

    result[0] = result[1] = 0;
    while(n-- > 0) {
        field_info *cfp = fp++;
        dix = cfp->descriptor_index;
        assert(cf->cp_tag[dix] == CP_UTF8);
        cpi = &cf->cp_item[dix];
        c = cpi->sval[2];  /* first char of type descriptor */
        result[(cfp->access_flags & ACC_STATIC)?0:1] += (c == 'D' || c == 'J')? 2 : 1;
    }
    *numClassVars = result[0];
    *numInstVars  = result[1];
}


/* Invoke a method whose class has been resolved and the
   method implementation identified */
void InvokeMethod( ClassType *ct, method_info *m, int isStatic ) {
    int i;
    int rw;
    uint32_t result1, result2;

    if (ct == NULL) {
        if (!isStatic) (void)JVM_Pop();
        return;
    }
    DataItem *locals = JVM_Top + 1 - m->nArgs;  /* points locals at first arg */

    for( i = m->nArgs;  i < m->max_locals;  i++ )
        JVM_Push(0);
    rw = InterpretMethod(ct, m, locals);
    /* pop the method result (if any) off the stack */
    if (rw > 0) {
        result1 = JVM_Pop();
        if (rw > 1)
            result2 = JVM_Pop();
    }
    /* pop local variables off the stack */
    while(JVM_Top >= locals)  // Bug fix: 02/06/10
        (void)JVM_Pop();
    /* pop the instance pointer (this) of the stack, if not static */
    // if (!isStatic)
    //    (void)JVM_Pop();
    /* push the method result back on the stack */
    if (rw > 0) {
        if (rw > 1)
            JVM_Push(result2);
        JVM_Push(result1);
    } 
}


/* Given a method name and signature, we search class cf for that method */
method_info *SearchClassForMethodByName( ClassFile *cf, char *name, char *signature ) {
    int ix;
    for( ix = cf->methods_count - 1;  ix >= 0;  ix-- ) {
        method_info *m = &(cf->methods[ix]);
        char *s = GetCPItemAsString(cf, m->name_index);
        if (strcmp(s,name) == 0) {
            SafeFree(s);
            s = GetCPItemAsString(cf, m->descriptor_index);
            if (strcmp(s,signature) == 0) {
                SafeFree(s);
                return m;
            }
        }
        SafeFree(s);
    }
    return NULL;
}


/* Given a class and a method invocation (specified by index ix in the
   class's constant pool, this function looks up the class name, method
   name, method signature *and* returns the resolved class reference. */
static ClassType *lookupClassAndMethod( ClassType *ct, int ix,
        char **cnamep, char **mnamep, char **cdescrp ) {
    ClassFile *cf;
    ClassType *result;
    ConstantPoolItem *cpi, *cpm;
    int classIndex, classNameIx, methodNTIx;

    assert(ct != NULL);
    cf = ct->cf;
    cpi = &cf->cp_item[ix];
    assert(cf->cp_tag[ix] == CP_Method);
    classIndex = cpi->ss.sval1;  /* reference to the class */
    assert(cf->cp_tag[classIndex] == CP_Class);
    result = ResolveClassReference(ct,classIndex);
    classNameIx = cf->cp_item[classIndex].ival;
    assert(cf->cp_tag[classNameIx] == CP_UTF8);
    *cnamep = (char*)(cf->cp_item[classNameIx].sval+2);
    methodNTIx = cpi->ss.sval2;  /* reference to Name&Type of the method */
    assert(cf->cp_tag[methodNTIx] == CP_NameAndType);
    cpm = &cf->cp_item[methodNTIx];
    *mnamep  = GetCPItemAsString(cf, cpm->ss.sval1);
    *cdescrp = GetCPItemAsString(cf, cpm->ss.sval2);
    return result;
}


typedef void (*MissingMethodHandler)(char *, char *, char *);

static void GeneralInvoke( ClassType *ct, int ix, int isStatic,
        int isVirtual, MissingMethodHandler missingFnHandler ) {
    ClassType *ct1;
    char *className, *methodName, *methodDescr;
    method_info *m;

    ct1 = lookupClassAndMethod(ct, ix, &className, &methodName, &methodDescr);
    if (ct1 == NULL) {
        if (missingFnHandler != NULL)
            missingFnHandler(className, methodName, methodDescr);
        SafeFree(methodName);
        SafeFree(methodDescr);
        return;
    }
    if (isVirtual) {
        int argSize = CountParameters((unsigned char *)methodDescr);
        DataItem *objRef = JVM_Top - argSize;
        ClassInstance *theObj = REAL_HEAP_POINTER(objRef->pval);
        assert(theObj->kind == CODE_INST);
        ct1 = theObj->thisClass;  // ct1 is the dynamic type of theObj
    }
    while(ct1 != NULL) {
        /* now we have to find the matching method in the ct1 class */
        m = SearchClassForMethodByName(ct1->cf, methodName, methodDescr);
        if (m != NULL)  /* found the method? */
            break;
        /* if not, repeat the search with the parent class */
        ct1 = ct1->parent;
    }

    if (ct1 == NULL || m == NULL) {
        fprintf(stderr, "Unable to resolve reference to method %s"
            "\nwith signature %s while"
            "\nexecuting invokevirtual/invokespecial/invokestatic\n",
            methodName, methodDescr);
        exit(1);
    }
    
    InvokeMethod(ct1, m, isStatic);
    
    SafeFree(methodName);
    SafeFree(methodDescr);
}


/* Invoke a static method whose class may not have been resolved;
   the method is identified by the index of a MethodRef entry in
   the constant pool of the class identified by ct. */
void InvokeStaticMethod( ClassType *ct, int ix ) {
    GeneralInvoke(ct, ix, 1, 0, &MissingClassStaticMethod);
}


/* Invoke an instance method whose class may not have been resolved;
   the method is identified by the index of a MethodRef entry in
   the constant pool of the class identified by ct.
   This function implements the InvokeSpecial JVM opcode.
*/
void InvokeSpecialMethod( ClassType *ct, int ix ) {
    GeneralInvoke(ct, ix, 0, 0, &MissingClassVirtualMethod);
}


/* Invoke a virtual method whose class may not have been resolved;
   the method is identified by the index of a MethodRef entry in
   the constant pool of the class identified by ct.
   If the class cannot be found, we call MissingClassVirtualMethod
   in case it is a class/method implemented as a native method.  */
void InvokeVirtualMethod( ClassType *ct, int ix ) {
    GeneralInvoke(ct, ix, 0, 1, &MissingClassVirtualMethod);
}


/* Given a type descriptor for a class type or an array type, this
   function finds or creates, if necessary, an instance of the
   ClassType struct which describes the datatype. */
ClassType *ResolveClassReferenceByName( char *cname ) {
    ClassType *ct1;

    if (strcmp(cname,"java/lang/Object") == 0)
        return NULL;
    if (cname[0] == '[') {
        ClassType *cta = ResolveClassReferenceByName(cname+1);
        for( ct1 = FirstLoadedClass;  ct1 != NULL;  ct1 = ct1->nextClass ) {
            if (!ct1->isArrayType) continue;
            if (cta == ct1->elementType) {  /* already created */
                return ct1;
            }
        }
        ct1 = MyHeapAlloc(sizeof(ClassType));
        ct1->kind = CODE_CLAS;
        ct1->typeDescriptor = SafeStrdup(cname);
        ct1->isArrayType = 1;
        ct1->elementType = cta;
        ct1->nextClass = FirstLoadedClass;
        FirstLoadedClass = ct1;
        return ct1;
    }
    // it's a class type
    for( ct1 = FirstLoadedClass;  ct1 != NULL;  ct1 = ct1->nextClass ) {
        if (ct1->isArrayType) continue;
        if (strcmp(cname,ct1->cf->cname) == 0) {  /* already loaded */
            return ct1;
        }
    }
    ct1 = LoadClass(cname);
    return ct1;
}

/* i must be the index of a Class item or an array type in the
   constant pool of the class identified by ct.
   If it is a Class item and the class has not been loaded, we
   try to find the class file on disk and load it.
   Any class initialization is performed.
   The result is a reference to a ClassType struct.
*/
ClassType *ResolveClassReference( ClassType *ct, int ix ) {
    char *cname;
    ClassType *ct1;
    ClassFile *cf = ct->cf;
    ConstantPoolItem *cpi = &cf->cp_item[ix];

    assert(cf->cp_tag[ix] == CP_Class);
    cname = GetCPItemAsString(cf,cpi->ival);
    ct1 = ResolveClassReferenceByName(cname);
    SafeFree(cname);
    return ct1;
}    


/* Given the name of a class, we attempt to read it into memory
   from the current directory on the disk.
   The result is a ClassType instance for this class, or
   NULL if the class cannot be found.

   Note: jar files are not supported.  This function cannot search
   for a class inside a jar file.
*/
ClassType *LoadClass( char *cname ) {
    ClassType *ct1;
    ClassType *pct;
    ClassFile *cf;
    method_info *m;
    char *parent;
    int numClassVars, numInstVars, i;

    // We don't support reading classes from any jar files ... so we don't
    // even try with anything in the java class library.
    if (strncmp(cname, "java/", 5) == 0)
        return NULL;
    cf = ReadClassFile(cname);
    if (cf == NULL)
        return NULL;

    /* make sure the parent class is loaded too */
    parent = GetCPItemAsString(cf,cf->super_class);
    pct = LoadClass(parent);
    SafeFree(parent);

    if (tracingExecution & TRACE_CLASS_LOADS)
        printf("loading class %s\n", cname);

    /* At this point, the bytecode needs to be verified */
    Verify(cf);
    
    getNumClassVars(cf, &numClassVars, &numInstVars);
    // The class itself would be allocated in the Method Area of a real JVM.
    ct1 = SafeMalloc(sizeof(ClassType)+(numClassVars-1)*sizeof(DataItem));
    ct1->kind = CODE_CLAS;
    ct1->typeDescriptor = SafeStrdup(cname);
    ct1->cf = cf;
    ct1->parent = pct;
    ct1->numInstanceFields = numInstVars;
    if (pct != NULL)
        ct1->numInstanceFields += pct->numInstanceFields;
    ct1->nextClass = FirstLoadedClass;
    FirstLoadedClass = ct1;

    /* we must check the class fields to see if any of them have
     the ConstantValue attribute; if so we initialize them. */
    for( i = 0;  i < cf->fields_count;  i++ ) {
        field_info *fi = &cf->fields[i];
        if ((fi->access_flags & ACC_STATIC) == 0) continue;
        int k = fi->constantValue_index;  // index of constant in constant pool
        if (k == 0) continue;
        PushConstant(ct1, k);  // get value onto the stack
        PutStatic(ct1, fi->name_index); // now store it into the field
    }

    /* Finally, we execute the <clinit> static method */
    m = SearchClassForMethodByName(cf, "<clinit>", "()V");
    if (m != NULL)  // initialize class variables via call to clinit
        InvokeMethod(ct1,m,1);

    return ct1;
}


/* Implements both the getstatic and putstatic JVM ops.
   The doAGet flag is 0 for putstatic, and nonzero for getstatic.
   The static field is identified by item ix in the constant pool
   of the class identified by ct.
   The JVM stack is modified and the class variable is accessed
   or overwritten as required for the JVM op.
   The result is 0 if the operation fails (field not found).  */
static int getOrPutStatic( ClassType *ct, int ix, int doAGet ) {
    ClassType *ct1;
    ClassFile *cf;
    int ntix, fnameIx, ftypeIx, itsTwoWords;
    char c;
    char *fname;  /* the field name */

    cf = ct->cf;
    assert(cf->cp_tag[ix] == CP_Field);
    ntix = cf->cp_item[ix].ss.sval2;
    assert(cf->cp_tag[ntix] == CP_NameAndType);
    fnameIx = cf->cp_item[ntix].ss.sval1;
    ftypeIx = cf->cp_item[ntix].ss.sval2;
    c = cf->cp_item[ftypeIx].sval[2];  // c = first char of type descriptor
    itsTwoWords = (c == 'D' || c == 'J');
    fname = (char *)(cf->cp_item[fnameIx].sval+2);
    if (tracingExecution & TRACE_FIELDS)
        fprintf(stdout,"%s access to static field %s\n",
            doAGet? "get" : "put", fname);

    if (doAGet && strcmp(fname,"out") == 0) {
        int cix = cf->cp_item[ix].ss.sval1;
        int cnix;
        assert(cf->cp_tag[cix] == CP_Class);
        cnix = cf->cp_item[cix].ival;
        char *cname = (char *)(cf->cp_item[cnix].sval+2);
        if (strcmp(cname,"java/lang/System") == 0) {
            JVM_Push(MAKE_HEAP_REFERENCE(Fake_System_Out));
            if (tracingExecution & TRACE_FIELDS)
                fprintf(stdout,"reference to fake System.out value pushed\n");
            return 1;
        }
    }

    /* now search for a static field named fname in its owning class */
    ct1 = ResolveClassReference(ct, cf->cp_item[ix].ss.sval1);
    while(ct1 != NULL) {
        int fieldCount = 0;
        ClassFile *cf1 = ct1->cf;
        int n = cf1->fields_count;
        field_info *fp = cf1->fields;
    
        while(n-- > 0) {
            field_info *cfp = fp++;
            int fnix = cfp->name_index;
            ConstantPoolItem *cpi = &cf1->cp_item[fnix];
            char *s = (char *)(cpi->sval+2);

            assert(cf1->cp_tag[fnix] == CP_UTF8);
            if (strcmp(s,fname) == 0) {  /* the same name */
                if (doAGet) {
                    JVM_Push(ct1->classField[fieldCount].uval);
                    if (itsTwoWords)
                        JVM_Push(ct1->classField[fieldCount+1].uval);
                } else {
                    if (itsTwoWords)
                        ct1->classField[fieldCount+1].uval = JVM_Pop();
                    ct1->classField[fieldCount].uval = JVM_Pop();
                }
                return 1;
            }
            if (cfp->access_flags & ACC_STATIC) {
                fieldCount++;
                fnix = cfp->descriptor_index;
                c = cf1->cp_item[fnix].sval[2];
                if (c == 'D' || c == 'J') fieldCount++;
            }
        }
        /* not found in current class, try the parent */
        ct1 = ct1->parent;
    }
    return 0;  /* field was not found */
}


/* Implements both the getfield and putfield JVM ops.
   The doAGet flag is 0 for putfield, and nozero for getfield.
   The instance field is identified by item ix in the constant pool
   of the class identified by ct.
   The JVM stack is modified and the class variable is accessed
   or overwritten as required for the JVM op.
   The result is 0 if the operation fails (field not found).    */
static int getOrPutField( ClassType *ct, int ix, int doAGet ) {
    ClassType *ct1;
    ClassFile *cf;
    int ntix, fnameIx, ftypeIx, itsTwoWords;
    char c;
    char *fname;  /* the field name */

    cf = ct->cf;
    assert(cf->cp_tag[ix] == CP_Field);
    ntix = cf->cp_item[ix].ss.sval2;
    assert(cf->cp_tag[ntix] == CP_NameAndType);
    fnameIx = cf->cp_item[ntix].ss.sval1;
    ftypeIx = cf->cp_item[ntix].ss.sval2;
    c = cf->cp_item[ftypeIx].sval[2]; /* c = first char of type descriptor */
    itsTwoWords = (c == 'D' || c == 'J');
    fname = (char *)(cf->cp_item[fnameIx].sval+2);
    if (tracingExecution & TRACE_FIELDS)
        fprintf(stdout,"%s access to instance field %s\n",
            doAGet? "get" : "put", fname);

    /* now search for an instance field named fname in its owning class */
    ct1 = ResolveClassReference(ct, cf->cp_item[ix].ss.sval1);
    while(ct1 != NULL) {
        int fieldCount = 0;
        ClassFile *cf1 = ct1->cf;
        int n = cf1->fields_count;
        field_info *fp = cf1->fields;
    
        while(n-- > 0) {
            field_info *cfp = fp++;
            int fnix = cfp->name_index;
            ConstantPoolItem *cpi = &cf1->cp_item[fnix];
            char *s = (char *)(cpi->sval+2);
            ClassInstance *objRef;

            assert(cf1->cp_tag[fnix] == CP_UTF8);
            if (strcmp(s,fname) == 0) {  /* the same name */
                if (ct1->parent != 0)
                    fieldCount += ct1->parent->numInstanceFields;
                if (doAGet) {
                    objRef = REAL_HEAP_POINTER(JVM_PopReference());
                    JVM_Push(objRef->instField[fieldCount].uval);
                    if (itsTwoWords)
                        JVM_Push(objRef->instField[fieldCount+1].uval);
                } else if (itsTwoWords) {
                    uint32_t v1 = JVM_Pop();
                    uint32_t v2 = JVM_Pop();
                    objRef = REAL_HEAP_POINTER(JVM_PopReference());
                    objRef->instField[fieldCount+1].uval = v1;
                    objRef->instField[fieldCount].uval = v2;
                } else {
                    uint32_t v1 = JVM_Pop();
                    objRef = REAL_HEAP_POINTER(JVM_PopReference());
                    objRef->instField[fieldCount].uval = v1;
                }
                return 1;
            }
            if ((cfp->access_flags & ACC_STATIC)==0) {  /* it's not static */
                fieldCount++;
                fnix = cfp->descriptor_index;
                c = cf1->cp_item[fnix].sval[2];
                if (c == 'D' || c == 'J') fieldCount++;
            }
        }
        /* not found in current class, try the parent */
        ct1 = ct1->parent;
    }
    return 0;  /* field was not found */
}


/* these four functions implement the JVM ops of the same name */

int GetStatic( ClassType *ct, int ix ) {
    return getOrPutStatic(ct,ix,1);
}

int PutStatic( ClassType *ct, int ix ) {
    return getOrPutStatic(ct,ix,0);
}

int GetField(ClassType *ct, int ix) {
    return getOrPutField(ct,ix,1);
}

int PutField(ClassType *ct, int ix) {
    return getOrPutField(ct,ix,0);
}



