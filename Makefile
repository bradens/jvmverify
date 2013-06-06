## This is a Makefile for the MyJVM project
##
## To build a production version:
## 1. Exchange the definition of CFLAGS with the commented definition, below.
## 2. Then execute the two commands:
##      make clean; make


CSRCS =	ClassFileFormat.c ReadClassFile.c PrintClassFile.c PrintByteCode.c \
	InterpretLoop.c jvm.c ClassResolver.c NativeClasses.c StringBuilder.c \
	MyAlloc.c TraceOptions.c Verifier.c VerifierUtils.c OpcodeSignatures.c main.c

HDRS =	ClassFileFormat.h ReadClassFile.h PrintClassFile.h PrintByteCode.h \
	InterpretLoop.h jvm.h ClassResolver.h NativeClasses.h StringBuilder.h \
	MyAlloc.h TraceOptions.h Verifier.h VerifierUtils.h OpcodeSignatures.h

OBJS =	ClassFileFormat.o ReadClassFile.o PrintClassFile.o PrintByteCode.o \
	InterpretLoop.o jvm.o ClassResolver.o NativeClasses.o StringBuilder.o \
	MyAlloc.o TraceOptions.o Verifier.o VerifierUtils.o OpcodeSignatures.o main.o

CFLAGS = -g -Wall               # definition for debugging
#CFLAGS = -Wall -O2 -DNDEBUG    # definition for production version

MyJVM: $(OBJS)
	gcc $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f $(OBJS)

myjvm.tar.gz: $(CSRCS) $(HDRS) Makefile
	tar cvf myjvm.tar $(CSRCS) $(HDRS) Makefile
	rm -f myjvm.tar.gz
	gzip myjvm.tar

## Dependencies

ClassFileFormat.o: MyAlloc.h ClassFileFormat.h ClassFileFormat.c

ReadClassFile.o: ClassFileFormat.h ReadClassFile.h MyAlloc.c ReadClassFile.c

PrintClassFile.o: ClassFileFormat.h MyAlloc.h PrintByteCode.h \
		PrintClassFile.h PrintClassFile.c

PrintByteCode.o: ClassFileFormat.h PrintByteCode.h PrintByteCode.c

InterpretLoop.o: ClassFileFormat.h jvm.h PrintByteCode.h TraceOptions.h \
		ClassResolver.h StringBuilder.h MyAlloc.h InterpretLoop.h \
		InterpretLoop.c

jvm.o: ClassFileFormat.h ReadClassFile.h  TraceOptions.h MyAlloc.h \
		jvm.h jvm.c

ClassResolver.o: ClassFileFormat.h ReadClassFile.h jvm.h TraceOptions.h \
                 Verifier.h MyAlloc.h ClassResolver.h ClassResolver.c

NativeClasses.o: ClassFileFormat.h jvm.h InterpretLoop.h MyAlloc.h \
                 StringBuilder.h TraceOptions.h NativeClasses.h NativeClasses.c

StringBuilder.o: ClassFileFormat.h jvm.h InterpretLoop.h MyAlloc.h \
                 StringBuilder.h TraceOptions.h StringBuilder.c

MyAlloc.o: ClassFileFormat.h TraceOptions.h MyAlloc.h MyAlloc.c

TraceOptions.o: TraceOptions.h TraceOptions.c

Verifier.o: ClassFileFormat.h OpcodeSignatures.h TraceOptions.h MyAlloc.h \
		Verifier.h VerifierUtils.h Verifier.c

VerifierUtils.o: ClassFileFormat.h ClassResolver.h OpcodeSignatures.h \
		TraceOptions.h MyAlloc.h VerifierUtils.c

OpcodeSignatures.o: OpcodeSignatures.h OpcodeSignatures.c

main.o: ClassFileFormat.h ReadClassFile.h PrintClassFile.h jvm.h \
		InterpretLoop.h ClassResolver.h TraceOptions.h \
		MyAlloc.h main.c



