/* Verifier.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ClassFileFormat.h"
#include "OpcodeSignatures.h"
#include "TraceOptions.h"
#include "MyAlloc.h"
#include "VerifierUtils.h"
#include "Verifier.h"

// Output an array of the verifier's type descriptors
static void printTypeCodesArray( char **vstate, method_info *m, char *name ) {
    int i;
    if (name != NULL)
        fprintf(stdout, "\nMethod %s:\n", name);
    else
        fputc('\n', stdout);
    for( i = 0;  i < m->max_locals; i++ ) {
        fprintf(stdout, "  V%d:  %s\n", i, *vstate++);
    }
    for( i = 0;  i < m->max_stack; i++ )
        fprintf(stdout, "  S%d:  %s\n", i, *vstate++);
}

static node *init_dict(method_state *ms);


// Verify the bytecode of one method m from class file cf
static void verifyMethod( ClassFile *cf, method_info *m ) {
    char *name = GetCPItemAsString(cf, m->name_index);
    char *retType;
    int numSlots = m->max_locals + m->max_stack;

    // initState is an array of strings, it has numSlots elements
    // retType describes the result type of this method
    char **initState = MapSigToInitState(cf, m, &retType);

    if (tracingExecution & TRACE_VERIFY)
    	printTypeCodesArray(initState, m, name);

    node *D = init_dict();
    node *first = D; 

    while (D->next != NULL) {
        if ((D->method_state->change_bit) == 0)
            continue;
        
        D->method_state->change_bit = 0;
        uint32_t bytecode_pos = D->method_state->bytecode_pos;
        uint32_t stack_height = D->method_state->stack_height;
        char** typecode_list = D->method_state->typecode_list;
        uint8_t op = m->code[p]
        
        // get the inline operands from the opcode
        char* operands = 
    }

    /* Verification rules that need to be implemented:
     *   1. No matter what execution path is followed to reach a point P in the bytecode
     *   the height of the stack will be the same at P for all these paths.
     *   2. The height of the stack will never exceed the number determined by the Java compiler
     *   Note: see the max_stack field in the method_info struct for a method
     *   3. The stack will never underflow 
     *   4. When the value of a local variable is used at point P in the bytecode, 
     *   that local variable should have been assigned a value on all paths which reach P.
     *   5. When a value is stored into a field F of a class, the value must be compatible with
     *   the type of F
     *   6. When an opcode OP at point P in the bytecode is executed, any operands for OP
     *   on the stack must have types which are compatible with OP
     */

     // Implement verification here.

    FreeTypeDescriptorArray(initState, numSlots);
    SafeFree(name);
}

static node *init_dict(method_state *ms) {
  node *root = malloc(sizeof(node));
  root->next = 0;
  root->ms = ms;
  return root;
}

static method_state *get_method_state(node *root, uint32_t position) {
  node *np;
  for(np = root; np != NULL; np = np->next) {
    if(np->ms != NULL && np->ms->bytecode_position == position)
        return np->ms;
  }
  return NULL;
}

static void insert_method_state(node *root, method_state *ms) {
  node *np;
  for(np = root; np != NULL; np = np->next) {
    if(np->next == NULL) {
        node *new = malloc(sizeof(node));
        new->next = 0;
        new->ms = ms;
        np->next = new;
        break;
    }
  }
}

static method_state *create_method_state(uint32_t bytecode_position, uint8_t change_bit, uint16_t stack_height, char **typecode_list) {
  method_state *ms = malloc(sizeof(method_state));
  ms->bytecode_position = bytecode_position;
  ms->change_bit = change_bit;
  ms->stack_height = stack_height;
  ms->typecode_list = typecode_list;
  return ms;
}

// Verify the bytecode of all methods in class file cf
void Verify( ClassFile *cf ) {
    int i;

    for( i = 0;  i < cf->methods_count;  i++ ) {
        method_info *m = &(cf->methods[i]);
	    verifyMethod(cf, m);
    }
    if (tracingExecution & TRACE_VERIFY)
    	fprintf(stdout, "Verification of class %s completed\n\n", cf->cname);
}


// Initialize this module
void InitVerifier(void) {
#ifndef NDEBUG
    // perform integrity check on the opcode table
    CheckOpcodeTable();
#endif
    // any initialization of local data structures can go here
}
