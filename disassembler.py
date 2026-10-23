#!/usr/bin/env python3

# Eigene Kopie des Bytecodes (aus dem C-File rueberkopiert)
bytecode = [
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x05, 0x01, 0x02, 0x02, 0x01,
    0x01, 0x03, 0x03, 0x03, 0x00, 0x06, 0x15, 0x08, 0x19, 0x00, 0x00, 0x00,
    0x09, 0x00, 0x04, 0x02, 0x08, 0x1E, 0x04, 0x03, 0x04, 0x07, 0x25, 0x08,
    0x36, 0x02, 0x02, 0x00, 0x05, 0x02, 0x01, 0x02, 0x00, 0x01, 0x02, 0x01,
    0x02, 0x05, 0x04, 0x05, 0x08, 0x1E, 0x02, 0x00, 0x02, 0x09,
]


def vm_disassembler(bc):
    i = 0
    while i < len(bc):
        opcode = bc[i]

        if opcode == 0x00:    # mov reg, const
            print(f"{hex(i)}: mov r{bc[i+1]}, {hex(bc[i+2])}")
            i += 3
        elif opcode == 0x01:  # mov reg, param0
            print(f"{hex(i)}: mov r{bc[i+1]}, param0")
            i += 2
        elif opcode == 0x02:  # mov dst, src
            print(f"{hex(i)}: mov r{bc[i+1]}, r{bc[i+2]}")
            i += 3
        elif opcode == 0x03:  # cmp reg, const
            print(f"{hex(i)}: cmp r{bc[i+1]}, {hex(bc[i+2])}")
            i += 3
        elif opcode == 0x04:  # cmp_ge reg, reg
            print(f"{hex(i)}: cmp_ge r{bc[i+1]}, r{bc[i+2]}")
            i += 3
        elif opcode == 0x05:  # add reg, reg
            print(f"{hex(i)}: add r{bc[i+1]}, r{bc[i+2]}")
            i += 3
        elif opcode == 0x06:  # je addr
            print(f"{hex(i)}: je {hex(bc[i+1])}")
            i += 2
        elif opcode == 0x07:  # jae addr
            print(f"{hex(i)}: jae {hex(bc[i+1])}")
            i += 2
        elif opcode == 0x08:  # jmp addr
            print(f"{hex(i)}: jmp {hex(bc[i+1])}")
            i += 2
        elif opcode == 0x09:  # vmexit
            print(f"{hex(i)}: vmexit")
            i += 1
        else:
            print(f"{hex(i)}: INVALID {hex(opcode)}")
            i += 1


vm_disassembler(bytecode)
