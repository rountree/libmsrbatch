#ifndef batchlib_h__
#define batchlib_h__

#include "msr_safe.h"
#include <inttypes.h>
#include <stdbool.h>

/* Welcome to batchlib, soon to be relocated in Variorum.
 *
 * This is not at all thread safe.
 *
 * Authors:  Matthew Bobbitt (bobbitt5@llnl.gov) and Barry Rountree (rountree@llnl.gov)
 */

/* The msr-batch interface expects a struct msr_batch_array which contains the length of an
 * array of msr_batch_ops and and a point to the beginnig of that array.  Both add_readops()
 * and add_writeops() expect a pointer to an (initially) empty msr_batch_array with
 * the msr_ops pointer set to NULL and the count set to zero, e.g.,
 *
 * struct msr_batch_array mybatch{ .numops=0, .ops=NULL };
 *
 * Subsequent calls to add_readops() and add_writeops() rely on realloc() to manage the
 * memory, so adding tens of thousands of operations to a single batch will incur substantial
 * overhead.
 *
 * The user is responsible for freeing the ops pointer.
 */

extern int add_readops(struct msr_batch_array *batch, __u16 firstcpu, __u16 lastcpu, __u32 msr);
extern int add_writeops(struct msr_batch_array *batch, __u16 first_cpu,__u16 last_cpu, __u32 msr, __u64 writemask);

/* Calling run_batch can fail for several different reasons.
 *
 * 1. If the msr-safe kernel module  has not been loaded.
 * 2. If the user does not have sufficient permissions to open the file.
 * 3. A read is attempted on an MSR not in the approved list.
 * 4. A read is attempted on an MSR that is not in the approved list, but doesn't exist on the architecure.
 * 5. A write is attempted on an MSR that is not in the approved list.
 * 6. A write is attempted on an MSR that is in the approved list, but read-only bits would be written to.
 * 7. A write is attempted on an MSR that is in the approved list with an appropriate write mask, but the underlying MSR is read-only.
 * 8. A write is attempted on an MSR that is in the approved list with an appropriate write maks, but the underlying MSR does not exist.
 * 9. The numops field is greater than the number of ops provided, leading to the kernel attempting interpret random garbage as ops (or segfault).
 * 10. The pointer passed into the ioctl() does not actually point to a msr_batch_array struct.
 * 11. The wrong constant is passed to the ioctl().
 * 12. The CPU requested does not exist or is not available.
 * 13. The kernel module is unloaded after the fopen() call but before the ioctl() call.
 * 14. The batch has zero numops.
 *
 * The call to run_batch() should guarantee that numbers 9-11 will not happen if add_readops() and add_writeops() have been
 * used with a correctly-initialized msr_batch_array object.  run_batch does not current distinguish among failures 1-8 and 12-14.
 * Failures 3, 5 and 6 can be avoided by calling msr_read_check() and msr_write_check(), below.
 */
extern int run_batch(struct msr_batch_array *batch);

/* The functions msr_read_check(), msr_write_check() and print_approved_list() rely on having a parse copy of the
 * approved list found in /dev/cpu/msr_approved_list.  Calls to those functions will automatically call parse_msr_approved_list()
 * if it has not been called before.  If the approve list changes during the execution of the program, the user can
 * force a call to parse_msr_approved_list() to reparse the approved list.  This function also relies on realloc(), but as the
 * number of possible MSRs is well under 10k, there should be little overhead involved.
 */
extern void parse_msr_approved_list();

/* msr_read_check() and msr_write_check() will confirm, using the most recently parsed msr_approved_list,
 * whether or not the proposed read or write op will succeed.
 */
extern bool msr_read_check(uint32_t address);
extern bool msr_write_check (uint32_t address, uint64_t value);

/* print_approved_list() dumps out the approved list to stdout.
 */
extern void print_approved_list();

/* print_batch() and print_op() prints individual batches and operations to stdout.  To use these functions,
 * recompile batchlib with -DDEBUG.
 */
#if DEBUG
extern int print_batch( struct msr_batch_array *batch);
extern int print_op(struct msr_batch_op *op);
#endif

#endif

