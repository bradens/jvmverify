/*
 * Verifier.h
 * Braden Simpson (V00685500)
 * Jordan Ell (V00660306)
 * University of Victoria, CSC586A
 * Virtual Machines
 */

#ifndef VERIFYH
#define VERIFYH

#include "ClassFileFormat.h"  // for ClassFile
#include "OpcodeSignatures.h"
#include <stdbool.h> 

typedef struct {
  uint32_t  bytecode_position;
  uint8_t   change_bit;
  int16_t  stack_height;
  char      **typecode_list;
} method_state;

typedef struct node{
	method_state *ms;
	struct node  *next;
} node;
char* safe_load_local(method_state*, method_info*, uint8_t);
bool safe_store_local(method_state*, method_info*, char*, uint8_t);
void push_die(method_state*, method_info*, char*);
void pop_die(method_state*, method_info*, char*);

extern void Verify( ClassFile *cf );
extern void InitVerifier(void);

#endif
