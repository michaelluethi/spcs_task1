/*
 * vm_fib_obf.c
 *
 * Task 2 (e) - Interpreter mit zusaetzlich obfuskierten Handlern.
 *   - 0x05 (add): MBA (x^y)+2*(x&y) + laufzeitabhaengiges Opaque Predicate
 *   - 0x03 (cmp): echter Vergleich XOR laufzeitabhaengiges Opaque Predicate
 * Beide ueberleben GCC -O3 dank volatile-gebundenem Laufzeitwert.
 *
 * Build:  gcc -O3 -Wall -o vm_fib_obf vm_fib_obf.c
 */
#include <stdio.h>
#include <stdlib.h>

unsigned int vm_execute(unsigned char *bc, unsigned int param0) {
    unsigned int vip = 0;
    unsigned int regs[256] = {0};
    int flag_eq = 0;
    int flag_ge = 0;

    while (1) {
        unsigned char opcode = bc[vip];

        switch (opcode) {

            case 0x00:  /* mov reg, const */
                regs[bc[vip + 1]] = bc[vip + 2];
                vip += 3;
                break;

            case 0x01:  /* mov reg, param0 */
                regs[bc[vip + 1]] = param0;
                vip += 2;
                break;

            case 0x02:  /* mov dst, src */
                regs[bc[vip + 1]] = regs[bc[vip + 2]];
                vip += 3;
                break;

            case 0x03:  /* cmp reg, const  (opaque-predicate-obfuskiert) */
                {
                    volatile unsigned int k = vip;
                    unsigned int opaque = (k * k - k) & 1;   /* immer 0 */
                    flag_eq = (regs[bc[vip + 1]] == bc[vip + 2]) ^ opaque;
                    vip += 3;
                }
                break;

            case 0x04:  /* cmp_ge reg, reg */
                flag_ge = (regs[bc[vip + 1]] >= regs[bc[vip + 2]]);
                vip += 3;
                break;

            case 0x05:  /* add reg, reg  (MBA + opaque predicate) */
                {
                    unsigned int x = regs[bc[vip + 1]];
                    unsigned int y = regs[bc[vip + 2]];
                    volatile unsigned int k = vip;
                    unsigned int opaque = (k * k - k) & 1;   /* immer 0 */
                    regs[bc[vip + 1]] = (x ^ y) + 2 * (x & y) + opaque;
                    vip += 3;
                }
                break;

            case 0x06:  /* je addr */
                if (flag_eq) { vip = bc[vip + 1]; } else { vip += 2; }
                break;

            case 0x07:  /* jae addr */
                if (flag_ge) { vip = bc[vip + 1]; } else { vip += 2; }
                break;

            case 0x08:  /* jmp addr */
                vip = bc[vip + 1];
                break;

            case 0x09:  /* vmexit */
                return regs[0];

            default:
                return 0;
        }
    }
}

unsigned char bytecode[] = {
    0x00,0x00,0x00, 0x00,0x01,0x01, 0x00,0x05,0x01, 0x02,0x02,0x01, 0x01,0x03,
    0x03,0x03,0x00, 0x06,0x15, 0x08,0x19, 0x00,0x00,0x00, 0x09,
    0x00,0x04,0x02, 0x08,0x1E, 0x04,0x03,0x04, 0x07,0x25, 0x08,0x36,
    0x02,0x02,0x00, 0x05,0x02,0x01, 0x02,0x00,0x01, 0x02,0x01,0x02,
    0x05,0x04,0x05, 0x08,0x1E, 0x02,0x00,0x02, 0x09,
};

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <n>\n", argv[0]); return -1; }
    unsigned int n = (unsigned int)atoi(argv[1]);
    printf("result: %u\n", vm_execute(bytecode, n));
    return 0;
}
