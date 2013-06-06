/* TraceOptions.h */

#ifndef TRACEOPTIONSH

#define TRACEOPTIONSH

enum {
    TRACE_NONE=0, TRACE_OPS=0x00000001, TRACE_CLASS_LOADS=0x00000002,
    TRACE_INVOKES=0x00000004, TRACE_FIELDS=0x00000008, TRACE_STACK=0x00000010,
    TRACE_HEAP=0x00000020, TRACE_VERIFY=0x00000040, TRACE_ALL=0xFFFFFFFF
} TRACE_FLAG;

extern int tracingExecution;  /* setting to non-zero enables JVM trace output */
extern int showWarnings;      /* setting to 0 suppresses some warning messages */

#endif
