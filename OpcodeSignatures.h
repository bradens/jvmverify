/* OpcodeSignatures.h */


#ifndef OPCODESIGSH
#define OPCODESIGSH

typedef struct {
        int     op;
        char    *opcodeName;
        char    *inlineOperands;
        char    *signature;
} OpcodeDescription;

#define LASTOPCODE  0Xc9        /* the number of the last op in the table */


extern OpcodeDescription opcodes[];

extern void CheckOpcodeTable(void);

#endif
