/*******************************************************************************
 * Copyright (c) 2010 Linaro Limited
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *     Peter Maydell (Linaro) - initial implementation
 ******************************************************************************/

#ifndef RISU_H
#define RISU_H

#include <inttypes.h>
#include <stdint.h>
#include <ucontext.h>
#include <stdio.h>
#include <getopt.h>

/* Extra option processing for architectures */
extern const struct option * const arch_long_opts;
extern const char * const arch_extra_help;
void process_arch_opt(int opt, const char *arg);
#define FIRST_ARCH_OPT   0x100

/* GCC computed include to pull in the correct risu_reginfo_*.h for
 * the architecture.
 */
#define REGINFO_HEADER2(X) #X
#define REGINFO_HEADER1(ARCHNAME) REGINFO_HEADER2(risu_reginfo_ ## ARCHNAME.h)
#define REGINFO_HEADER(ARCH) REGINFO_HEADER1(ARCH)

#include REGINFO_HEADER(ARCH)

/* Socket related routines */
int master_connect(int port);
int apprentice_connect(const char *hostname, int port);
int send_data_pkt(int sock, void *pkt, int pktlen);
int recv_data_pkt(int sock, void *pkt, int pktlen);
void send_response_byte(int sock, int resp);

extern uintptr_t image_start_address;
extern void *memblock;

/* Ops code under test can request from risu: */
#define OP_COMPARE 0
#define OP_TESTEND 1
#define OP_SETMEMBLOCK 2
#define OP_GETMEMBLOCK 3
#define OP_COMPAREMEM 4

/* The memory block should be this long */
#define MEMBLOCKLEN 8192

/* This is the data structure we pass over the socket for OP_COMPARE
 * and OP_TESTEND. It is a simplified and reduced subset of what can
 * be obtained with a ucontext_t*, and is architecture specific
 * (defined in risu_reginfo_*.h).
 */
struct reginfo;

typedef struct {
   uintptr_t pc;
   uint32_t risu_op;
} trace_header_t;

/* Functions operating on reginfo */

/* Function prototypes for read/write helper functions.
 *
 * We pass the helper function to send_register_info and
 * recv_and_compare_register_info which can either be backed by the
 * traditional network socket or a trace file.
 */
typedef int (*write_fn) (void *ptr, size_t bytes);
typedef int (*read_fn) (void *ptr, size_t bytes);
typedef void (*respond_fn) (int response);

/* Send the register information from the struct ucontext down the socket.
 * Return the response code from the master.
 * NB: called from a signal handler.
 */
int send_register_info(write_fn write_fn, void *uc);

/* Read register info from the socket and compare it with that from the
 * ucontext. Return 0 for match, 1 for end-of-test, 2 for mismatch.
 * NB: called from a signal handler.
 */
int recv_and_compare_register_info(read_fn read_fn,
                                   respond_fn respond, void *uc);

/* Print a useful report on the status of the last comparison
 * done in recv_and_compare_register_info(). This is called on
 * exit, so need not restrict itself to signal-safe functions.
 * Should return 0 if it was a good match (ie end of test)
 * and 1 for a mismatch.
 */
int report_match_status(int trace);

/* Interface provided by CPU-specific code: */

/* Move the PC past this faulting insn by adjusting ucontext
 */
void advance_pc(void *uc);

/* Set the parameter register in a ucontext_t to the specified value.
 * (32-bit targets can ignore high 32 bits.)
 * vuc is a ucontext_t* cast to void*.
 */
void set_ucontext_paramreg(void *vuc, uint64_t value);

/* Return the value of the parameter register from a reginfo. */
uint64_t get_reginfo_paramreg(struct reginfo *ri);

/* Return the risu operation number we have been asked to do,
 * or -1 if this was a SIGILL for a non-risuop insn.
 */
int get_risuop(struct reginfo *ri);

/* Return the PC from a reginfo */
uintptr_t get_pc(struct reginfo *ri);

/* initialize structure from a ucontext */
void reginfo_init(struct reginfo *ri, ucontext_t *uc);

/* return 1 if structs are equal, 0 otherwise. */
int reginfo_is_eq(struct reginfo *r1, struct reginfo *r2);

/* print reginfo state to a stream, returns 1 on success, 0 on failure */
int reginfo_dump(struct reginfo *ri, FILE * f);

/* reginfo_dump_mismatch: print mismatch details to a stream, ret nonzero=ok */
int reginfo_dump_mismatch(struct reginfo *m, struct reginfo *a, FILE *f);

/* return size of reginfo */
const int reginfo_size(void);

#endif /* RISU_H */
