/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Miscellaneous utility functions.
 */
#ifndef _DALVIK_MISC
#define _DALVIK_MISC

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "Inlines.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Used to shut up the compiler when a parameter isn't used.
 */
#define UNUSED_PARAMETER(p)     (void)(p)

/*
 * Floating point conversion functions.  These are necessary to avoid
 * strict-aliasing problems ("dereferencing type-punned pointer will break
 * strict-aliasing rules").  According to the gcc info page, this usage
 * is allowed, even with "-fstrict-aliasing".
 *
 * The code generated by gcc-4.1.1 appears to be much better than a
 * type cast dereference ("int foo = *(int*)&myfloat") when the conversion
 * function is inlined.  It also allows us to take advantage of the
 * optimizations that strict aliasing rules allow.
 */
INLINE float dvmU4ToFloat(u4 val) {
    union { u4 in; float out; } conv;
    conv.in = val;
    return conv.out;
}
INLINE u4 dvmFloatToU4(float val) {
    union { float in; u4 out; } conv;
    conv.in = val;
    return conv.out;
}

/*
 * Print a hex dump to the log file.
 *
 * "local" mode prints a hex dump starting from offset 0 (roughly equivalent
 * to "xxd -g1").
 *
 * "mem" mode shows the actual memory address, and will offset the start
 * so that the low nibble of the address is always zero.
 *
 * If "tag" is NULL the default tag ("dalvikvm") will be used.
 */
typedef enum { kHexDumpLocal, kHexDumpMem } HexDumpMode;
void dvmPrintHexDumpEx(int priority, const char* tag, const void* vaddr,
    size_t length, HexDumpMode mode);

/*
 * Print a hex dump, at INFO level.
 */
INLINE void dvmPrintHexDump(const void* vaddr, size_t length) {
    dvmPrintHexDumpEx(ANDROID_LOG_INFO, LOG_TAG,
        vaddr, length, kHexDumpLocal);
}

/*
 * Print a hex dump at VERBOSE level. This does nothing in non-debug builds.
 */
INLINE void dvmPrintHexDumpDbg(const void* vaddr, size_t length,const char* tag)
{
#if !LOG_NDEBUG
    dvmPrintHexDumpEx(ANDROID_LOG_VERBOSE, (tag != NULL) ? tag : LOG_TAG,
        vaddr, length, kHexDumpLocal);
#endif
}

typedef enum {
    kDebugTargetUnknown = 0,
    kDebugTargetLog,
    kDebugTargetFile,
} DebugTargetKind;

/*
 * We pass one of these around when we want code to be able to write debug
 * info to either the log or to a file (or stdout/stderr).
 */
typedef struct DebugOutputTarget {
    /* where to? */
    DebugTargetKind which;

    /* additional bits */
    union {
        struct {
            int priority;
            const char* tag;
        } log;
        struct {
            FILE* fp;
        } file;
    } data;
} DebugOutputTarget;

/*
 * Fill in a DebugOutputTarget struct.
 */
void dvmCreateLogOutputTarget(DebugOutputTarget* target, int priority,
    const char* tag);
void dvmCreateFileOutputTarget(DebugOutputTarget* target, FILE* fp);

/*
 * Print a debug message.
 */
void dvmPrintDebugMessage(const DebugOutputTarget* target, const char* format,
    ...)
#if defined(__GNUC__)
    __attribute__ ((format(printf, 2, 3)))
#endif
    ;

/*
 * Return a newly-allocated string in which all occurrences of '.' have
 * been changed to '/'.  If we find a '/' in the original string, NULL
 * is returned to avoid ambiguity.
 */
char* dvmDotToSlash(const char* str);

/*
 * Return a newly-allocated string containing a human-readable equivalent
 * of 'descriptor'. So "I" would be "int", "[[I" would be "int[][]",
 * "[Ljava/lang/String;" would be "java.lang.String[]", and so forth.
 */
char* dvmHumanReadableDescriptor(const char* descriptor);

/*
 * Return a newly-allocated string for the "dot version" of the class
 * name for the given type descriptor. That is, The initial "L" and
 * final ";" (if any) have been removed and all occurrences of '/'
 * have been changed to '.'.
 *
 * "Dot version" names are used in the class loading machinery.
 * See also dvmHumanReadableDescriptor.
 */
char* dvmDescriptorToDot(const char* str);

/*
 * Return a newly-allocated string for the type descriptor
 * corresponding to the "dot version" of the given class name. That
 * is, non-array names are surrounded by "L" and ";", and all
 * occurrences of '.' have been changed to '/'.
 *
 * "Dot version" names are used in the class loading machinery.
 */
char* dvmDotToDescriptor(const char* str);

/*
 * Return a newly-allocated string for the internal-form class name for
 * the given type descriptor. That is, the initial "L" and final ";" (if
 * any) have been removed.
 */
char* dvmDescriptorToName(const char* str);

/*
 * Return a newly-allocated string for the type descriptor for the given
 * internal-form class name. That is, a non-array class name will get
 * surrounded by "L" and ";", while array names are left as-is.
 */
char* dvmNameToDescriptor(const char* str);

/*
 * Get the current time, in nanoseconds.  This is "relative" time, meaning
 * it could be wall-clock time or a monotonic counter, and is only suitable
 * for computing time deltas.
 */
u8 dvmGetRelativeTimeNsec(void);

/*
 * Get the current time, in microseconds.  This is "relative" time, meaning
 * it could be wall-clock time or a monotonic counter, and is only suitable
 * for computing time deltas.
 */
INLINE u8 dvmGetRelativeTimeUsec(void) {
    return dvmGetRelativeTimeNsec() / 1000;
}

/*
 * Get the current time, in milliseconds.  This is "relative" time,
 * meaning it could be wall-clock time or a monotonic counter, and is
 * only suitable for computing time deltas.  The value returned from
 * this function is a u4 and should only be used for debugging
 * messages.  TODO: make this value relative to the start-up time of
 * the VM.
 */
INLINE u4 dvmGetRelativeTimeMsec(void) {
    return (u4)(dvmGetRelativeTimeUsec() / 1000);
}

/*
 * Get the current per-thread CPU time.  This clock increases monotonically
 * when the thread is running, but not when it's sleeping or blocked on a
 * synchronization object.
 *
 * The absolute value of the clock may not be useful, so this should only
 * be used for time deltas.
 *
 * If the thread CPU clock is not available, this always returns (u8)-1.
 */
u8 dvmGetThreadCpuTimeNsec(void);

/*
 * Per-thread CPU time, in micros.
 */
INLINE u8 dvmGetThreadCpuTimeUsec(void) {
    return dvmGetThreadCpuTimeNsec() / 1000;
}

/*
 * Like dvmGetThreadCpuTimeNsec, but for a different thread.
 */
u8 dvmGetOtherThreadCpuTimeNsec(pthread_t thread);
INLINE u8 dvmGetOtherThreadCpuTimeUsec(pthread_t thread) {
    return dvmGetOtherThreadCpuTimeNsec(thread) / 1000;
}

/*
 * Sleep for increasingly longer periods, until "maxTotalSleep" microseconds
 * have elapsed.  Pass in the start time, which must be a value returned by
 * dvmGetRelativeTimeUsec().
 *
 * Returns "false" if we were unable to sleep because our time is up.
 */
bool dvmIterativeSleep(int iteration, int maxTotalSleep, u8 relStartTime);

/*
 * Set the "close on exec" flag on a file descriptor.
 */
bool dvmSetCloseOnExec(int fd);

/*
 * Unconditionally abort the entire VM.  Try not to use this.
 *
 * NOTE: if this is marked ((noreturn)), gcc will merge multiple dvmAbort()
 * calls in a single function together.  This is good, in that it reduces
 * code size slightly, but also bad, because the native stack trace we
 * get from the abort may point at the wrong call site.  Best to leave
 * it undecorated.
 */
void dvmAbort(void);
void dvmPrintNativeBackTrace(void);

#if (!HAVE_STRLCPY)
/* Implementation of strlcpy() for platforms that don't already have it. */
size_t strlcpy(char *dst, const char *src, size_t size);
#endif

/*
 *  Allocates a memory region using ashmem and mmap, initialized to
 *  zero.  Actual allocation rounded up to page multiple.  Returns
 *  NULL on failure.
 */
void *dvmAllocRegion(size_t size, int prot, const char *name);

/*
 * Get some per-thread stats from /proc/self/task/N/stat.
 */
typedef struct {
    unsigned long utime;    /* number of jiffies scheduled in user mode */
    unsigned long stime;    /* number of jiffies scheduled in kernel mode */
    int processor;          /* number of CPU that last executed thread */
} ProcStatData;
bool dvmGetThreadStats(ProcStatData* pData, pid_t tid);

/*
 * Returns the pointer to the "absolute path" part of the given path
 * string, treating first (if any) instance of "/./" as a sentinel
 * indicating the start of the absolute path. If the path isn't absolute
 * in the usual way (i.e., starts with "/") and doesn't have the sentinel,
 * then this returns NULL.
 *
 * For example:
 *     "/foo/bar/baz" returns "/foo/bar/baz"
 *     "foo/./bar/baz" returns "/bar/baz"
 *     "foo/bar/baz" returns NULL
 *
 * The sentinel is used specifically to aid in cross-optimization, where
 * a host is processing dex files in a build tree, and where we don't want
 * the build tree's directory structure to be baked into the output (such
 * as, for example, in the dependency paths of optimized dex files).
 */
const char* dvmPathToAbsolutePortion(const char* path);

#ifdef __cplusplus
}
#endif

#endif /*_DALVIK_MISC*/
