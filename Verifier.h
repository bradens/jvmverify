/* Verify.h */

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

extern void Verify( ClassFile *cf );
extern void InitVerifier(void);

#endif
