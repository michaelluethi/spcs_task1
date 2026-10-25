/*
 * vm_fib_blinded.c
 *
 * Task 2 (e)+(f) - Interpreter with obfuscated handlers AND an
 * encrypted ("blinded") bytecode.
 *
 * (f) Bytecode blinding:
 *   The bytecode is stored encrypted in the binary. Each byte at
 *   position p was XOR-encrypted with (p & 0xFF) (see encrypt_bytecode.py).
 *   At runtime, each byte is decrypted only when it is read, via fetch() -
 *   i.e. gradually, with a position-dependent key that changes per
 *   position. The full plaintext bytecode therefore never exists in
 *   memory at the same time.
 *
 * (e) Additionally obfuscated handlers:
 *   - 0x05 (add): MBA (x^y)+2*(x&y) plus a runtime-dependent opaque predicate
 *   - 0x03 (cmp): real comparison XORed with a runtime-dependent opaque predicate
 *
 * Build:  gcc -O3 -Wall -o vm_fib_blinded vm_fib_blinded.c
 * Run:    ./vm_fib_blinded <n>
 */
#include <stdio.h>
#include <stdlib.h>

/* Decrypts the byte at position p (key = p & 0xFF). XOR is its own
 * inverse, so this is the same formula used for encryption. Only the
 * single byte currently needed is ever decrypted (gradual decoding). */
static unsigned char fetch(unsigned char *bc, unsigned int p) {
    return bc[p] ^ (unsigned char)(p & 0xFF);
}

unsigned int vm_execute(unsigned char *bc, unsigned int param0) {
    unsigned int vip = 0;
    unsigned int regs[256] = {0};
    int flag_eq = 0;
    int flag_ge = 0;

    while (1) {
        unsigned char opcode = fetch(bc, vip);

        switch (opcode) {

            case 0x00:  /* mov reg, const */
                regs[fetch(bc, vip + 1)] = fetch(bc, vip + 2);
                vip += 3;
                break;

            case 0x01:  /* mov reg, param0 */
                regs[fetch(bc, vip + 1)] = param0;
                vip += 2;
                break;

            case 0x02:  /* mov dst, src */
                regs[fetch(bc, vip + 1)] = regs[fetch(bc, vip + 2)];
                vip += 3;
                break;

            case 0x03:  /* cmp reg, const  (obfuscated with an opaque predicate) */
                {
                    volatile unsigned int k = vip;
                    unsigned int opaque = (k * k - k) & 1;   /* always 0 */
                    flag_eq = (regs[fetch(bc, vip + 1)] == fetch(bc, vip + 2)) ^ opaque;
                    vip += 3;
                }
                break;

            case 0x04:  /* cmp_ge reg, reg */
                flag_ge = (regs[fetch(bc, vip + 1)] >= regs[fetch(bc, vip + 2)]);
                vip += 3;
                break;

            case 0x05:  /* add reg, reg  (MBA + opaque predicate) */
                {
                    unsigned int x = regs[fetch(bc, vip + 1)];
                    unsigned int y = regs[fetch(bc, vip + 2)];
                    volatile unsigned int k = vip;
                    unsigned int opaque = (k * k - k) & 1;   /* always 0 */
                    regs[fetch(bc, vip + 1)] = (x ^ y) + 2 * (x & y) + opaque;
                    vip += 3;
                }
                break;

            case 0x06:  /* je addr */
                if (flag_eq) { vip = fetch(bc, vip + 1); } else { vip += 2; }
                break;

            case 0x07:  /* jae addr */
                if (flag_ge) { vip = fetch(bc, vip + 1); } else { vip += 2; }
                break;

            case 0x08:  /* jmp addr */
                vip = fetch(bc, vip + 1);
                break;

            case 0x09:  /* vmexit */
                return regs[0];

            default:
                return 0;
        }
    }
}

/* Encrypted bytecode (produced by encrypt_bytecode.py). */
unsigned char bytecode[] = {
    0x00, 0x01, 0x02, 0x03, 0x05, 0x04, 0x06, 0x02, 0x09, 0x0B, 0x08, 0x0A,
    0x0D, 0x0E, 0x0D, 0x0C, 0x10, 0x17, 0x07, 0x1B, 0x0D, 0x15, 0x16, 0x17,
    0x11, 0x19, 0x1E, 0x19, 0x14, 0x03, 0x1A, 0x1C, 0x24, 0x26, 0x07, 0x2B,
    0x12, 0x27, 0x24, 0x27, 0x2D, 0x2B, 0x2B, 0x29, 0x2C, 0x2C, 0x2C, 0x2E,
    0x32, 0x34, 0x36, 0x36, 0x3C, 0x2B, 0x34, 0x37, 0x3A, 0x30,
};

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <n>\n", argv[0]); return -1; }
    unsigned int n = (unsigned int)atoi(argv[1]);
    printf("result: %u\n", vm_execute(bytecode, n));
    return 0;
}