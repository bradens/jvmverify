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

typedef struct {
  uint32_t  bytecode_position;
  uint8_t   change_bit;
  uint16_t  stack_height;
  char      **typecode_list;
} method_state;

typedef struct {
	method_state *ms;
	struct node  *next;
} node;

void push_die(method_state*, method_info*, char*);
void pop_die(method_state*, method_info*, char*);
static void ParseOpSignature(OpcodeDescription, method_state*, method_info*);

extern void Verify( ClassFile *cf );
extern void InitVerifier(void);

#endif
