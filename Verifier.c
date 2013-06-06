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

static bool is_simple_type(char* type) {
    if(strcmp(type,"I") == 0 || strcmp(type,"D") == 0 || strcmp(type,"d") == 0 || strcmp(type,"L") == 0 || 
        strcmp(type,"l") == 0 || strcmp(type,"F") == 0 )
        return true;
    return false;
}

static bool is_reference(char* type) {
    size_t lenpre = strlen("A"),
           lenstr = strlen(type);
    return lenstr < lenpre ? false : strncmp("A", type, lenpre) == 0;
}

static bool merge(method_state *ms, int numSlots, uint32_t h, char** t) {
    int index = 0;
    if(ms->stack_height != h)
        return false;
    for(; index < numSlots; index++) {
        if(strcmp(ms->typecode_list[index], t[index]) == 0)
            continue;
        else if(strcmp(ms->typecode_list[index], "X") == 0 || strcmp(t[index], "X") == 0) {
            return false;
        }
        else if(strcmp(ms->typecode_list[index], "U") == 0 || strcmp(t[index], "U") == 0) {
            if(strcmp(ms->typecode_list[index], "U") != 0)
                ms->change_bit = 1;
            ms->typecode_list[index] = "U";
        }
        else if(is_simple_type(ms->typecode_list[index]) && is_simple_type(t[index]) &&
            strcmp(ms->typecode_list[index], t[index]) != 0) {
            return false;
        }
        else if((is_reference(ms->typecode_list[index]) && is_simple_type(t[index])) || 
            (is_simple_type(ms->typecode_list[index]) && is_reference(t[index])) ) {
            return false;
        }
        else if((is_reference(ms->typecode_list[index]) && (strcmp(t[index],"N") == 0)) || 
            (is_reference(t[index]) && (strcmp(ms->typecode_list[index],"N") == 0))) {
            if(strcmp(ms->typecode_list[index],"N") == 0) {
                ms->change_bit = 1;
                ms->typecode_list[index] = t[index];
            }
        }
        else if(is_reference(ms->typecode_list[index]) && is_reference(t[index])) {
            char* lub = LUB(ms->typecode_list[index], t[index]);
            if(strcmp(ms->typecode_list[index],lub) != 0) {
                ms->change_bit = 1;
                ms->typecode_list[index] = lub;
            }
        }
    }
    return true;
}

static int next_op_offset(OpcodeDescription op) {
    return strlen(op.inlineOperands)+1;
}

static short branch_offset(method_info *mi, OpcodeDescription op, uint32_t p) {
    if(strcmp(op.inlineOperands,"bb") == 0) {
        uint8_t b1 = mi->code[p+1];
        uint8_t b2 = mi->code[p+2];
        return (b1 << 8 + b2);
    }
    else if(strcmp(op.inlineOperands,"bbbb") == 0) {
        uint8_t b1 = mi->code[p+1];
        uint8_t b2 = mi->code[p+2];
        uint8_t b3 = mi->code[p+3];
        uint8_t b4 = mi->code[p+4];
        return (b1 << 24 + b2 << 16 + b3 << 8 + b4);
    }
    return NULL;
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
        uint8_t opcode = m->code[p];

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

    for (int i = 0;i < strlen(sig);i++) {
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
                case 'X': 
                    
                case 'I':
                case 'L':
                case 'l':
                case 'D':
                case 'd':
                case 'F':
                case 'N': 
                    pop_die(ms, mi, &(sig[i]));
                    break;
            }
        }
        else {
            switch(sig[i]) {
                case 'I': 
                    push_die(ms, mi, &(sig[i]));

            }
            // isPushing
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
