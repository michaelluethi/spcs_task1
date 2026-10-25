# Task 2 — Virtualization-based Obfuscation: Design Decisions

## Files

| File | Task part | Purpose |
|------|-----------|---------|
| 'vm_fib.c' | (d) | Clean interpreter + plaintext bytecode |
| 'vm_fib_obf.c' | (e) | Interpreter with two obfuscated handlers |
| 'vm_fib_blinded.c' | (e)+(f) | Obfuscated handlers + encrypted bytecode |
| 'disassembler.py' | (c) | Disassembler for the bytecode |
| 'encrypt_bytecode.py' | (f) | Build helper that encrypts the bytecode |

Build: 'gcc -O3 -Wall -o vm_fib_blinded vm_fib_blinded.c' for the hardened one.

## (a) VM architecture and ISA

### Overall design choices

- **Register-based** (not stack-based). Chosen because it stays close to
  the structure of the original 'fib()' (a handful of local variables map
  directly onto registers) and is easier to reason about than a stack
  machine. 256 registers are available; only r0–r5 are used.
- **Variable instruction length.** Each instruction consists of a 1-byte
  opcode followed by 0–2 operand bytes. Mirrors the reference example from the lecture.
- **Two separate comparison flags** ('flag_eq', 'flag_ge') instead of one
  context-dependent flag. This keeps the mapping clear: each compare
  instruction sets exactly its own flag, and each conditional jump reads
  exactly one flag. It trades a little economy for clarity.
- **r0 is the return register.** 'vmexit' always returns r0.
- **Elementary instructions only.** No single instruction performs a
  large part of the Fibonacci computation; each one does one primitive
  step (load, copy, add, compare, jump), as required by the task.

### Register allocation

| Register | Meaning |
|----------|---------|
| r0 | 'a' / return value |
| r1 | 'b' |
| r2 | 's' |
| r3 | 'n' (parameter) |
| r4 | 'i' (loop counter) |
| r5 | constant 1 (helper for 'i++') |

### Instruction set

| Opcode | Mnemonic | Operands | Bytes | Semantics |
|--------|----------|----------|-------|-----------|
| 0x00 | 'mov reg, const' | reg, const | 3 | 'reg = const' |
| 0x01 | 'mov reg, param0' | reg | 2 | 'reg = param0' |
| 0x02 | 'mov dst, src' | dst, src | 3 | 'dst = src' (register copy) |
| 0x03 | 'cmp reg, const' | reg, const | 3 | 'flag_eq = (reg == const)' |
| 0x04 | 'cmp_ge reg, reg' | reg0, reg1 | 3 | 'flag_ge = (reg0 >= reg1)' |
| 0x05 | 'add reg, reg' | reg0, reg1 | 3 | 'reg0 = reg0 + reg1' |
| 0x06 | 'je addr' | addr | 2 | 'if flag_eq: vip = addr; else vip += 2' |
| 0x07 | 'jae addr' | addr | 2 | 'if flag_ge: vip = addr; else vip += 2' |
| 0x08 | 'jmp addr' | addr | 2 | 'vip = addr' |
| 0x09 | 'vmexit' | – | 1 | 'return r0' |

### Notable encoding decision — comparison direction

The loop condition in the source is 'i <= n'. The ISA only provides a
"greater-or-equal" comparison ('cmp_ge'). Rather than add a dedicated
"less-or-equal" instruction, the operands are swapped: 'cmp_ge r3, r4'
computes 'n >= i', which is equivalent to 'i <= n'. The loop body is
entered on the taken branch ('jae'), and the fall-through path leads to the
exit.

## (b) Bytecode generation

'fib()' was translated by hand into VM assembly, split into the same basic
blocks as the control-flow-flattening task (ST_ENTRY, ST_RET0,
ST_LOOP_INIT, ST_LOOP_CHECK, ST_LOOP_BODY, ST_RET_RESULT). Jump addresses
were resolved with a two-pass approach: first the byte offset of every
block was computed from the instruction lengths, then each symbolic jump
target was replaced by the concrete offset. The resulting bytecode is 58
bytes.

## (c) Disassembler

'disassembler.py' mirrors the interpreter's fetch-decode loop but prints
instead of executing: an index walks the array, and for each opcode the
correct number of operand bytes is consumed and printed, prefixed with the
byte offset. Its output matches the hand-written assembly exactly, which
confirms the bytecode layout and jump addresses from part (b).

## (d) Interpreter

A standard fetch-decode-execute loop in C: 'vip' indexes into the bytecode,
a 'switch' over the opcode dispatches to one handler per instruction, and
each handler advances 'vip' by that instruction's length (or sets it
directly for jumps). Registers are a 256-entry 'unsigned int' array;
'unsigned int' arithmetic wraps modulo 2^32, which matches the original
'fib()' behaviour on overflow.

## (e) Additional obfuscation of two handlers

Two handlers were hardened, and — crucially — the hardening was verified to
**survive GCC '-O3'** by disassembling the optimized binary with 'objdump'.

### Key finding: plain MBA is not enough against -O3

Three variants of the 'add' handler were tried:

1. **Textbook MBA** '(x ^ y) + 2*(x & y)' — this is exactly the identity
   shown in the lecture slides. Under '-O3', GCC recognized it and folded
   it back into a plain addition ('lea (%rax,%rbx,2)' etc.). The
   obfuscation was gone.
2. **Hidden zero** '(x|y) - (x^y) - (x&y)' (always 0, because
   'OR = XOR + AND' bit by bit) added to 'x + y' — also recognized as 0 and
   removed.
3. **MBA + runtime-dependent opaque predicate** — survives.

### The technique used

Both hardened handlers use an **opaque predicate bound to a 'volatile'
runtime value**:

'''c
volatile unsigned int k = vip;        // volatile forces a real load
unsigned int opaque = (k * k - k) & 1; // k*(k-1) is always even -> &1 == 0
'''

'k*(k-1)' is the product of two consecutive integers, one of which is
always even, so the product is even and its lowest bit is 0 — the predicate
is always 0. Because 'k' is 'volatile', the compiler must actually read it
and cannot prove the result is 0, so the whole computation stays in the
binary (confirmed: an 'imul' instruction is present per handler at '-O3').

- **'0x05' (add):** 'regs[dst] = (x ^ y) + 2*(x & y) + opaque;' — the MBA
  plus the opaque zero. The 'xor'/'and' of the MBA now survive alongside
  the 'imul' of the predicate.
- **'0x03' (cmp):** 'flag_eq = (reg == const) ^ opaque;' — XORing the real
  comparison result with an always-zero value leaves it unchanged
  ('x ^ 0 == x'). This works cleanly because '==' in C yields exactly 0 or 1.

## (f) Bytecode blinding (encrypted bytecode)

### Scheme: gradual, position-dependent XOR

Each byte at position 'p' is stored XOR-encrypted with 'p & 0xFF'
('encrypt_bytecode.py' produces the array). At runtime every byte is
decrypted only when read, through a one-line helper:

'''c
static unsigned char fetch(unsigned char *bc, unsigned int p) {
    return bc[p] ^ (unsigned char)(p & 0xFF);
}
'''

Every 'bc[...]' access in the interpreter was replaced by 'fetch(bc, ...)'.
This satisfies the task requirements:

- **Gradual decoding:** only the single byte currently needed is ever
  decrypted. The complete plaintext bytecode is never in memory at once, so
  a memory dump at any instant does not reveal the whole program.
- **Several keys / control-flow sensitive:** the key 'p & 0xFF' changes per
  position, so each byte offset effectively has its own key — not one
  global key.

Verification: in the '-O3' binary the '.data' section contains the
encrypted sequence ('00 01 02 03 05 04 06 02 09 0b …'), not the plaintext
opcodes, and the program still produces correct results.

## AI assistance

An AI assistant was used for boilerplate and as a tutor (explaining
concepts and pointing out syntax errors. The ISA design, the
hand-translation of 'fib()' to bytecode and the jump-address calculation,
the handler logic and the blinding scheme were the group's own work. A little assistance was used in finding the rigfht obfuscation techniques.