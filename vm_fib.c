/*
 * vm_fib.c
 *
 * Task 2 (d) - Virtualization-based interpreter for fib().
 * Lecture: Software Protection, HSLU, Autumn Term 2026.
 *
 * The function fib() has been translated into bytecode for a custom,
 * register-based VM (see the ISA description in DESIGN.md). vm_execute()
 * interprets this embedded bytecode. fib_orig() is kept only for
 * comparison/testing.
 *
 * Build:  gcc -O0 -Wall -o vm_fib vm_fib.c
 * Run:    ./vm_fib <n>
 */
#include <stdio.h>
#include <stdlib.h>


/* Original (kept only for comparison during testing) */
unsigned int fib_orig(unsigned int n) {
  unsigned int a=0;
  unsigned int b=1;
  unsigned int s = b;
  unsigned int i;

  if (n == 0) {
    return 0;
  }

  for (i=2; i<=n; i++) {
    s=a+b;
    a=b;
    b=s;
  }
  return s;
}


/*
 * VM interpreter (register-based, variable instruction length).
 * - vip  : virtual instruction pointer (byte index into the bytecode)
 * - regs : 256 registers; r0 is the return register (read by vmexit)
 * - flag_eq / flag_ge : two separate comparison flags
 */
unsigned int vm_execute(unsigned char *bc, unsigned int param0) {
    unsigned int vip = 0;
    unsigned int regs[256] = {0};
    int flag_eq = 0;
    int flag_ge = 0;

    while (1) {
        unsigned char opcode = bc[vip];

        switch (opcode) {

            case 0x00:  /* mov reg, const  ->  regs[reg] = const */
                regs[bc[vip + 1]] = bc[vip + 2];
                vip += 3;
                break;

            case 0x01:  /* mov reg, param0 ->  regs[reg] = param0 */
                regs[bc[vip + 1]] = param0;
                vip += 2;
                break;

            case 0x02:  /* mov dst, src    ->  regs[dst] = regs[src] */
                regs[bc[vip + 1]] = regs[bc[vip + 2]];
                vip += 3;
                break;

            case 0x03:  /* cmp reg, const  ->  flag_eq = (regs[reg] == const) */
                flag_eq = (regs[bc[vip + 1]] == bc[vip + 2]);
                vip += 3;
                break;

            case 0x04:  /* cmp_ge reg, reg ->  flag_ge = (regs[r0] >= regs[r1]) */
                flag_ge = (regs[bc[vip + 1]] >= regs[bc[vip + 2]]);
                vip += 3;
                break;

            case 0x05:  /* add reg, reg    ->  regs[r0] = regs[r0] + regs[r1] */
                regs[bc[vip + 1]] = regs[bc[vip + 1]] + regs[bc[vip + 2]];
                vip += 3;
                break;

            case 0x06:  /* je addr         ->  if flag_eq: vip = addr else vip += 2 */
                if (flag_eq) {
                    vip = bc[vip + 1];
                } else {
                    vip += 2;
                }
                break;

            case 0x07:  /* jae addr        ->  if flag_ge: vip = addr else vip += 2 */
                if (flag_ge) {
                    vip = bc[vip + 1];
                } else {
                    vip += 2;
                }
                break;

            case 0x08:  /* jmp addr        ->  vip = addr */
                vip = bc[vip + 1];
                break;

            case 0x09:  /* vmexit          ->  return r0 */
                return regs[0];

            default:    /* invalid opcode */
                return 0;
        }
    }
}


/* Embedded bytecode of the virtualized fib(). Registers:
 * r0=a/return, r1=b, r2=s, r3=n, r4=i, r5=constant 1. */
unsigned char bytecode[] = {
    /* ST_ENTRY @0x00 */
    0x00, 0x00, 0x00,   /* mov r0, 0        a = 0 */
    0x00, 0x01, 0x01,   /* mov r1, 1        b = 1 */
    0x00, 0x05, 0x01,   /* mov r5, 1        one-register (for i++) */
    0x02, 0x02, 0x01,   /* mov r2, r1       s = b */
    0x01, 0x03,         /* mov r3, param0   load n */
    0x03, 0x03, 0x00,   /* cmp r3, 0        flag_eq = (n == 0) */
    0x06, 0x15,         /* je  0x15         -> ST_RET0 */
    0x08, 0x19,         /* jmp 0x19         -> ST_LOOP_INIT */
    /* ST_RET0 @0x15 */
    0x00, 0x00, 0x00,   /* mov r0, 0        return 0 */
    0x09,               /* vmexit */
    /* ST_LOOP_INIT @0x19 */
    0x00, 0x04, 0x02,   /* mov r4, 2        i = 2 */
    0x08, 0x1E,         /* jmp 0x1E         -> ST_LOOP_CHECK */
    /* ST_LOOP_CHECK @0x1E */
    0x04, 0x03, 0x04,   /* cmp_ge r3, r4    flag_ge = (n >= i) */
    0x07, 0x25,         /* jae 0x25         -> ST_LOOP_BODY */
    0x08, 0x36,         /* jmp 0x36         -> ST_RET_RESULT */
    /* ST_LOOP_BODY @0x25 */
    0x02, 0x02, 0x00,   /* mov r2, r0       s = a */
    0x05, 0x02, 0x01,   /* add r2, r1       s = s + b */
    0x02, 0x00, 0x01,   /* mov r0, r1       a = b */
    0x02, 0x01, 0x02,   /* mov r1, r2       b = s */
    0x05, 0x04, 0x05,   /* add r4, r5       i++ */
    0x08, 0x1E,         /* jmp 0x1E         -> ST_LOOP_CHECK */
    /* ST_RET_RESULT @0x36 */
    0x02, 0x00, 0x02,   /* mov r0, r2       return s */
    0x09,               /* vmexit */
};


int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <n>\n", argv[0]);
        return -1;
    }

    unsigned int n = (unsigned int)atoi(argv[1]);
    printf("result: %u\n", vm_execute(bytecode, n));

    return 0;
}