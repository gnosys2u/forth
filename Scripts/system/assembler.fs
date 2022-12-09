// basic ops not included in the kernel

FORTH64 [if]

requires asm_amd64

[else]

ARCH_X86 [if]

requires asm_pentium

[else]

ARCH_ARM [if]

requires asm_arm

[endif]
[endif]
[endif]
