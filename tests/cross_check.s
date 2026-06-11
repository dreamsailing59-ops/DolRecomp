# test instructions for cross-checking against our decoder
# assemble with: powerpc-eabi-as -mregnames -o cross_check.o cross_check.s

.section .text
.globl _start
_start:

# --- D-form arithmetic / compare ---
mulli r3, r4, -7
subfic r4, r5, 1
addi r3, r1, 16
addic r4, r4, -1
addic. r5, r5, -1
lis r5, 0x1234
cmpwi r3, -1
cmplwi r3, 0x8000

# --- immediate logical ---
ori r4, r3, 0xFF00
oris r5, r4, 0x1234
xori r6, r5, 0xFFFF
xoris r7, r6, 0x8000
andi. r8, r7, 0x00FF
andis. r9, r7, 0x00FF

# --- D-form loads/stores ---
lwz r3, 0(r1)
lwzu r4, 4(r1)
lbz r5, 8(r1)
lbzu r6, 12(r1)
lhz r7, 16(r1)
lhzu r8, 20(r1)
lha r9, -4(r1)
lhau r10, 24(r1)
stw r3, 28(r1)
stwu r4, 32(r1)
stb r5, 36(r1)
stbu r6, 40(r1)
sth r7, 44(r1)
sthu r8, 48(r1)
lmw r20, 52(r1)
stmw r20, 100(r1)

# --- branches / CR / SPR ---
b branch_target
bc 12, 2, branch_target
blr
bctr
cror 2, 3, 4
mflr r10
mtlr r10

# --- register compare / arithmetic ---
cmpw cr1, r3, r4
cmplw cr2, r3, r4
add r10, r11, r12
addc r11, r12, r13
adde r12, r13, r14
addze r13, r14
subf r14, r15, r16
subfc r15, r16, r17
subfe r16, r17, r18
subfze r17, r18
neg r18, r19

# --- register logical / shifts ---
and r19, r20, r21
andc r20, r21, r22
or r21, r22, r23
orc r22, r23, r24
xor r23, r24, r25
nand r24, r25, r26
nor r25, r26, r27
eqv r26, r27, r28
cntlzw r27, r28
extsb r28, r29
extsh r29, r30
slw r30, r31, r3
srw r31, r3, r4
sraw r3, r4, r5
srawi r4, r5, 7

# --- rotate/mask ---
rlwinm r5, r6, 5, 8, 23
rlwnm r6, r7, r8, 4, 27
rlwimi r7, r8, 8, 8, 15

# --- indexed loads/stores ---
lwzx r3, r4, r5
lwzux r6, r4, r5
lbzx r7, r4, r5
lbzux r8, r4, r5
lhzx r9, r4, r5
lhzux r10, r4, r5
lhax r11, r4, r5
lhaux r12, r4, r5
stwx r3, r4, r5
stwux r6, r4, r5
stbx r7, r4, r5
stbux r8, r4, r5
sthx r9, r4, r5
sthux r10, r4, r5

branch_target:
nop
