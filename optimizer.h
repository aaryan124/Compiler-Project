#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "tac.h"

/* ============================================================
 *  OPTIMIZER  —  Phase 5 of the compiler
 *
 *  Passes (applied in order):
 *    1. Constant Folding      :  t0 = 3 + 4   →  t0 = 7
 *    2. Constant Propagation  :  x = 7; t1 = x + 2  →  t1 = 7 + 2
 *    3. Copy Propagation      :  x = y; t2 = x + 1  →  t2 = y + 1
 *    4. Dead Code Elimination :  remove assignments whose result
 *                                is never subsequently used
 *
 *  All passes work on the TAC linked list in-place (or return a
 *  new trimmed list) so that no changes to tac.h / tac.c are needed.
 * ============================================================ */

/* Run all optimisation passes and return the (possibly shorter) list.
 * Call this with the raw TAC from generate_tac() and pass the result
 * to print_tac(). */
TAC* optimize(TAC* head);

/* Individual passes – exposed so they can be called independently. */
TAC* pass_constant_folding(TAC* head);
TAC* pass_constant_propagation(TAC* head);
TAC* pass_copy_propagation(TAC* head);
TAC* pass_dead_code_elimination(TAC* head);

/* Pretty-printer (same format as print_tac but with a different header). */
void print_optimized_tac(TAC* head);

#endif /* OPTIMIZER_H */
