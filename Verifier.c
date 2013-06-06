/* Verifier.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "ClassFileFormat.h"
#include "OpcodeSignatures.h"
#include "TraceOptions.h"
#include "MyAlloc.h"
#include "VerifierUtils.h"
#include "Verifier.h"
#include "jvm.h"

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

static bool safe_push(method_state *ms, method_info *mi, char* val) {
    ms->stack_height++;
    if(ms->stack_height > mi->max_stack)
        return false;
    ms->typecode_list[mi->max_locals+ms->stack_height-1] = val;
    return true;
}

static bool safe_pop(method_state *ms, method_info *mi, char* val) {
    ms->stack_height--;
    if(ms->stack_height < 0)
        return false;
    char* pop = ms->typecode_list[mi->max_locals+ms->stack_height];
    if(strcmp(val, pop) == 0)
        return true;
    return false;
}

static node *init_dict(method_state *ms);
static method_state *create_method_state(uint32_t bytecode_position, uint8_t change_bit, uint16_t stack_height, char **typecode_list);
static method_state *find_set_change_bit(node *root);


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

    node *D = init_dict(create_method_state(0,1,0,initState));
    method_state *curr_ms;

    while ((curr_ms = find_set_change_bit(D)) != NULL) {
        curr_ms->change_bit = 0;
        uint32_t p = curr_ms->bytecode_position;
        uint32_t h = curr_ms->stack_height;
        char** t   = curr_ms->typecode_list;
        uint8_t opcode = m->code[p++];

        OpcodeDescription op = opcodes[opcode]; 
        ParseOpSignature(op, curr_ms, m);
        // switch(op) {
        // case OP_fload:
        //     safe_push(curr_ms, m, "F");
        // case OP_dload:
        //     safe_push(curr_ms, m, "D");
        //     safe_push(curr_ms, m, "d");
        // case OP_lload:
        //     safe_push(curr_ms, m, "L");
        //     safe_push(curr_ms, m, "l");
        // case OP_iload:
        //     safe_push(curr_ms, m, "I");
        // case OP_aload:
        //     safe_push(curr_ms, m, "A");
        // }
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

static void ParseOpSignature(OpcodeDescription op, method_state* ms, method_info* mi) {
    char* sig = op.signature;
    bool isPopping = true;
    printf("parsing %s", sig);
    int i;
    for (i = 0;i < strlen(sig);i++) {
        if (isPopping){
            switch(sig[i]) {
                case '>': 
                    isPopping = false;
                    
                    break;
                case '-': 
                    printf("FAILURE: Popped an empty stack slot.\n");
                    exit(0);
                case 'U': 
                    printf("FAILURE: Popped an uninitialized value.\n");
                    exit(0);
                default:
                    pop_die(ms, mi, &(sig[i]));
                    break;
            }
        }
        else {
            switch(sig[i]) {
                case 'A': 
                    push_die(ms, mi, sig);
                    break;
                default:
                    push_die(ms, mi, &(sig[i]));
                    break;

            }
        }
    }
}
void push_die(method_state* ms, method_info* mi, char* val) {
    printf("pushing %s\n", val);
    if (!safe_push(ms, mi, val)) {
        printf("Stack push expected %s", val);
        exit(0);
    }
}

void pop_die(method_state* ms, method_info* mi, char* val) {
    printf("popping %s\n", val);
    if (!safe_pop(ms, mi, val)) {
        printf("Stack pop expected %s", val);
        exit(0);
    }
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

static method_state *find_set_change_bit(node *root) {
  node *np;
  for(np = root; np != NULL; np = np->next) {
    if(np->ms->change_bit == 1)
        return np->ms;
  }
  return NULL;
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
