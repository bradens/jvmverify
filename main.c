/* main.c -- tests the class reader */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ClassFileFormat.h"
#include "ReadClassFile.h"
#include "PrintClassFile.h"
#include "jvm.h"
#include "InterpretLoop.h"
#include "ClassResolver.h"
#include "Verifier.h"
#include "TraceOptions.h"
#include "MyAlloc.h"

static char *pgmName = NULL;

static char *usageText[] = {
    "Usage:",
    "\t%s [options] classname [arguments for main method]",
    "where the options are:",
    "\t-D\tprint the disassembled classfile",
    "\t-X\tsuppress execution of the classfile",
    "\t-W\tsuppress runtime warning messages",
    "\t-T\ttrace everything",
    "\t-To\ttrace execution of the bytecode ops",
    "\t-Tc\ttrace class loads",
    "\t-Ti\ttrace method invocations",
    "\t-Tf\ttrace field accesses",
    "\t-Ts\ttrace most stack pushes/pops",
    "\t-Th\ttrace heap usage and gc",
    "\t-Tv\ttrace bytecode verificaton",
    "\t-Snnn\tset max stack size to nnn entries",
    "\t-Hnnn\tset heap size to nnn bytes",
    NULL
};

static void usage() {
    char **s;
    for( s=usageText;  *s!=NULL;  s++ ) {
        fprintf(stderr, *s, pgmName);
        fputc('\n',stderr);
    }
    exit(1);
}

static void callMain( ClassType *ct, int stackSize, int heapSize,
        char *jArgs[], int jArgCnt ) {
    static char *mainSignature = "([Ljava/lang/String;)V";
    ClassFile *cf = ct->cf;
    int i;
    method_info *m = NULL;
    ArrayOfRef *arr;

    m = SearchClassForMethodByName(cf, "main", mainSignature);
    if (m == NULL)
        m = SearchClassForMethodByName(cf, "Main", mainSignature);
    if (m == NULL) {
        fprintf(stderr,"%s does not contain a suitable Main method\n",
            GetCPItemAsString(cf, cf->this_class));
        exit(1);
    }
    if ((m->access_flags & ACC_STATIC) == 0) {
        fprintf(stderr,"The Main method of %s is not static\n",
            GetCPItemAsString(cf, cf->this_class));
        exit(1);
    }
    printf("Execution begins ...\n\n");


    // allocate an array for the command line arguments
    arr = MyHeapAlloc(sizeof(ArrayOfRef)+(jArgCnt-1)*4);
    arr->kind = CODE_ARRA;
    arr->size = jArgCnt;
    ClassType *cta = ResolveClassReferenceByName( "java/lang/String" );
    arr->classRef =  cta==NULL? NULL_HEAP_REFERENCE : MAKE_HEAP_REFERENCE(cta);
    JVM_PushReference(MAKE_HEAP_REFERENCE(arr));
    for( i = 0;  i < jArgCnt;  i++ ) {
        StringInstance *p;
        p = MyHeapAlloc(sizeof(StringInstance));
        p->kind = CODE_STRG;
        p->sval = jArgs[i];
        arr->elements[i] = MAKE_HEAP_REFERENCE(p);
    }

    InvokeMethod(ct,m,1);

    if (tracingExecution & TRACE_HEAP)
        PrintHeapUsageStatistics();
    if (tracingExecution & TRACE_CLASS_LOADS)
        PrintFilesRead();
}


int main( int argc, char *argv[] ) {
    int argNum;
    char *classname = NULL;
    int stackSize = 1024;
    int heapSize = 10240;
    ClassType *ct;
    uint8_t DFlag = 0, XFlag = 0;

    pgmName = argv[0];
    for( argNum=1; argNum<argc; argNum++ ) {
        char *cp = argv[argNum];
        if (*cp == '-') {
            switch(*++cp) {
            case 'D':   DFlag = 1;  break;
            case 'W':   showWarnings = 0;  break;
            case 'X':   XFlag = 1;  break;
            case 'T':   if (*++cp == '\0') {
                            tracingExecution = TRACE_ALL;
                        } else while(*cp != '\0') {
                            char c = *cp++;
                            if (c == 'o')
                                tracingExecution |= TRACE_OPS;
                            else if (c == 'c')
                                tracingExecution |= TRACE_CLASS_LOADS;
                            else if (c == 'f')
                                tracingExecution |= TRACE_FIELDS;
                            else if (c == 'i')
                                tracingExecution |= TRACE_INVOKES;
                            else if (c == 's')
                                tracingExecution |= TRACE_STACK;
                            else if (c == 'h')
                                tracingExecution |= TRACE_HEAP;
                            else if (c == 'v')
                                tracingExecution |= TRACE_VERIFY;
                        }
                        break;
            case 'S':   stackSize = atoi(cp+1);  break;
            case 'H':   heapSize = atoi(cp+1);  break;
            default:    usage();
            }
        } else {
            classname = cp;
            break;
        }
    }
    // argNum+1 is the index of the first arg, if any, to pass to the
    // main method of the Java program

    if (classname == NULL) usage();

    InitMyAlloc(heapSize);
    JVM_Init(stackSize);
    InitVerifier();

    printf("Reading class %s ...\n", classname);
    ct = LoadClass(classname);
    if (ct != NULL) {
        if (DFlag)
            PrintClassFile(ct->cf);
        if (!XFlag) {
            int numArgs = argc - argNum - 1;
            callMain(ct, stackSize, heapSize, argv+argNum+1, numArgs);
        }
    } else {
        fprintf(stderr, "Unable to read/parse classfile %s.class\n",
            classname);
        return 1;
    }
    return 0;
}
