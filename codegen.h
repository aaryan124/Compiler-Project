#ifndef CODEGEN_H
#define CODEGEN_H

#include "tac.h"

/*
 * codegen.h  —  Phase 6: x86-32 Code Generator
 *
 * Takes the optimised TAC list and emits NASM (Intel syntax) x86-32 assembly
 * to the given output file.
 *
 * Assemble & link (Linux):
 *   nasm -f elf32 output.asm -o output.o
 *   gcc  -m32 output.o -o program
 *
 * Assemble & link (Windows / MinGW):
 *   nasm -f win32 output.asm -o output.obj
 *   gcc  -m32 output.obj -o program.exe
 */

void generate_x86(TAC* head, const char* output_file);

#endif
