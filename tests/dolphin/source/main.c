#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fat.h>
#include <gccore.h>

extern u32 dolphin_test_blr_fn(void);
extern u32 dolphin_test_bctr_fn(void);
extern void dolphin_test_lmw_fn(const u32* src, u32* out);
extern void dolphin_test_stmw_fn(u32* out);

asm(
    ".text\n"
    ".globl dolphin_test_blr_fn\n"
    "dolphin_test_blr_fn:\n"
    "    li 3,0x55\n"
    "    blr\n"
    ".globl dolphin_test_bctr_fn\n"
    "dolphin_test_bctr_fn:\n"
    "    lis 4,dolphin_test_bctr_target@ha\n"
    "    addi 4,4,dolphin_test_bctr_target@l\n"
    "    mtctr 4\n"
    "    bctr\n"
    "    li 3,0\n"
    "dolphin_test_bctr_target:\n"
    "    li 3,0x66\n"
    "    blr\n"
    ".globl dolphin_test_lmw_fn\n"
    "dolphin_test_lmw_fn:\n"
    "    stwu 1,-64(1)\n"
    "    stmw 20,8(1)\n"
    "    lmw 20,0(3)\n"
    "    stw 20,0(4)\n"
    "    stw 31,4(4)\n"
    "    lmw 20,8(1)\n"
    "    addi 1,1,64\n"
    "    blr\n"
    ".globl dolphin_test_stmw_fn\n"
    "dolphin_test_stmw_fn:\n"
    "    stwu 1,-64(1)\n"
    "    stmw 20,8(1)\n"
    "    lis 20,0xB000\n"
    "    ori 20,20,0x0014\n"
    "    lis 31,0xB000\n"
    "    ori 31,31,0x001F\n"
    "    stmw 20,0(3)\n"
    "    lmw 20,8(1)\n"
    "    addi 1,1,64\n"
    "    blr\n"
);

static int pass_count = 0;
static int fail_count = 0;
static FILE* reference_file = NULL;
static int reference_gecko_channel = -1;

static void reference_probe_gecko(void) {
    if (reference_gecko_channel >= 0)
        return;

    if (usb_isgeckoalive(1)) {
        reference_gecko_channel = 1;
    } else if (usb_isgeckoalive(0)) {
        reference_gecko_channel = 0;
    } else if (usb_isgeckoalive(2)) {
        reference_gecko_channel = 2;
    }
}

static void reference_printf(const char* fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    va_start(args, fmt);
    char line[256];
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    kprintf("%s", line);
    if (reference_file) {
        fputs(line, reference_file);
        fflush(reference_file);
    }

    reference_probe_gecko();
    if (reference_gecko_channel >= 0)
        usb_sendbuffer_safe(reference_gecko_channel, line, strlen(line));
}

static void check(int cond, const char* name) {
    reference_printf("CPUREF_BOOL,%s,%u,%s\n", name, cond ? 1 : 0, cond ? "PASS" : "FAIL");

    if (cond) {
        pass_count++;
        reference_printf("  PASS: %s\n", name);
    } else {
        fail_count++;
        reference_printf("  FAIL: %s\n", name);
    }
}

static void check_eq(u32 got, u32 want, const char* name) {
    reference_printf("CPUREF,%s,0x%08X,0x%08X,%s\n",
                  name, got, want, got == want ? "PASS" : "FAIL");

    if (got == want) {
        pass_count++;
        reference_printf("  PASS: %s (0x%08X)\n", name, got);
    } else {
        fail_count++;
        reference_printf("  FAIL: %s - got 0x%08X, want 0x%08X\n", name, got, want);
    }
}

#define DEFINE_CR_LOGIC_TEST(fn_name, mnemonic) \
static u32 fn_name(u32 seed) { \
    u32 cr; \
    asm volatile( \
        "mtcrf 0xff,%1\n" \
        #mnemonic " 2,3,4\n" \
        "mfcr %0\n" \
        : "=r"(cr) \
        : "r"(seed) \
        : "cr0" \
    ); \
    return (cr >> 29) & 1u; \
}

DEFINE_CR_LOGIC_TEST(dolphin_crand, crand)
DEFINE_CR_LOGIC_TEST(dolphin_crandc, crandc)
DEFINE_CR_LOGIC_TEST(dolphin_creqv, creqv)
DEFINE_CR_LOGIC_TEST(dolphin_crnand, crnand)
DEFINE_CR_LOGIC_TEST(dolphin_crnor, crnor)
DEFINE_CR_LOGIC_TEST(dolphin_crorc, crorc)
DEFINE_CR_LOGIC_TEST(dolphin_crxor, crxor)

typedef u32 (*cr_logic_test_fn)(u32 seed);

static void check_cr_logic(const char* name, cr_logic_test_fn fn,
                           const u8 expected[4]) {
    static const u32 bit3 = 0x10000000u;
    static const u32 bit4 = 0x08000000u;

    for (u32 i = 0; i < 4; i++) {
        u32 a = (i >> 1) & 1u;
        u32 b = i & 1u;
        u32 seed = (a ? bit3 : 0u) | (b ? bit4 : 0u);
        char label[32];

        snprintf(label, sizeof(label), "%s %u%u", name, a, b);
        check_eq(fn(seed), expected[i], label);
    }
}

static void test_immediate_arithmetic(void) {
    kprintf("--- immediate arithmetic ---\n");
    printf("--- immediate arithmetic ---\n");

    u32 result;
    u32 xer;
    u32 saved_xer;

    asm volatile("li %0,42" : "=r"(result));
    check_eq(result, 42, "addi/li literal");

    u32 base = 100;
    asm volatile("addi %0,%1,-10" : "=r"(result) : "r"(base));
    check_eq(result, 90, "addi negative SIMM");

    asm volatile("lis %0,0x1234" : "=r"(result));
    check_eq(result, 0x12340000, "addis/lis shifted immediate");

    base = 0x00020000;
    asm volatile("addis %0,%1,-1" : "=r"(result) : "r"(base));
    check_eq(result, 0xFFFF0000u + base, "addis negative shifted immediate");

    base = 0xFFFFFFFEu;
    asm volatile("mulli %0,%1,-7" : "=r"(result) : "r"(base));
    check_eq(result, 14, "mulli negative times negative");

    base = 0x80000000u;
    asm volatile("mulli %0,%1,2" : "=r"(result) : "r"(base));
    check_eq(result, 0, "mulli keeps low 32 bits");

    asm volatile("mfxer %0" : "=r"(saved_xer));

    base = 5;
    asm volatile(
        "subfic %0,%2,1\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(base)
    );
    check_eq(result, 0xFFFFFFFCu, "subfic immediate minus register");
    check_eq(xer & 0x20000000u, 0, "subfic borrow clears CA");

    base = 1;
    asm volatile(
        "subfic %0,%2,1\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(base)
    );
    check_eq(result, 0, "subfic equal result");
    check_eq(xer & 0x20000000u, 0x20000000u, "subfic no borrow sets CA");

    base = 0;
    asm volatile(
        "addic %0,%2,-1\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(base)
    );
    check_eq(result, 0xFFFFFFFF, "addic 0 + -1 result");
    check_eq(xer & 0x20000000u, 0, "addic 0 + -1 clears CA");

    base = 1;
    asm volatile(
        "addic %0,%2,-1\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(base)
    );
    check_eq(result, 0, "addic 1 + -1 result");
    check_eq(xer & 0x20000000u, 0x20000000u, "addic 1 + -1 sets CA");

    u32 cr;
    u32 zero_xer = 0;
    base = 1;
    asm volatile(
        "mtxer %3\n"
        "addic. %0,%2,-1\n"
        "mfxer %1\n"
        "mfcr %4\n"
        : "=&r"(result), "=r"(xer), "+r"(base), "+r"(zero_xer), "=r"(cr)
        :
        : "cr0"
    );
    check_eq(result, 0, "addic. result");
    check_eq((cr >> 28) & 0xFu, 0x2, "addic. records EQ with SO clear");

    asm volatile("mtxer %0" : : "r"(saved_xer));
}

static void test_compare_and_bc(void) {
    kprintf("--- compare / bc ---\n");
    printf("--- compare / bc ---\n");

    u32 flag = 0;
    u32 value = 7;
    asm volatile(
        "cmpwi %1,7\n"
        "bne 1f\n"
        "li %0,1\n"
        "1:\n"
        : "+r"(flag)
        : "r"(value)
        : "cr0"
    );
    check_eq(flag, 1, "cmpwi equal then bne not taken");

    flag = 0;
    value = 0xFFFFFFFFu;
    asm volatile(
        "cmplwi %1,0x8000\n"
        "bgt 1f\n"
        "b 2f\n"
        "1:\n"
        "li %0,1\n"
        "2:\n"
        : "+r"(flag)
        : "r"(value)
        : "cr0"
    );
    check_eq(flag, 1, "cmplwi unsigned greater then bc taken");

    u32 cr;
    u32 a = 0xFFFFFFFFu;
    u32 b = 0;
    u32 zero_xer = 0;
    u32 xer_after_clear;
    asm volatile(
        "mtxer %4\n"
        "mfxer %1\n"
        "cmpw cr1,%2,%3\n"
        "mfcr %0\n"
        : "=&r"(cr), "=&r"(xer_after_clear)
        : "r"(a), "r"(b), "r"(zero_xer)
        : "cr1"
    );
    check_eq(xer_after_clear & 0x80000000u, 0, "mtxer zero clears SO before cmpw");
    check_eq((cr >> 24) & 0xFu, 0x8, "cmpw signed less in CR1");

    asm volatile(
        "cmplw cr2,%1,%2\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(a), "r"(b)
        : "cr2"
    );
    check_eq((cr >> 20) & 0xFu, 0x4, "cmplw unsigned greater in CR2");

    u32 ctr_before = 1;
    u32 ctr_after;
    flag = 0;
    asm volatile(
        "mtctr %2\n"
        "bc 0,0,1f\n"
        "li %0,1\n"
        "1:\n"
        "mfctr %1\n"
        : "+r"(flag), "=r"(ctr_after)
        : "r"(ctr_before)
        : "ctr"
    );
    check_eq(ctr_after, 0, "bc decrements CTR when BO says so");
    check_eq(flag, 1, "bc not taken when CTR condition false");

    static const u8 crand_expected[4] = {0, 0, 0, 1};
    static const u8 crandc_expected[4] = {0, 0, 1, 0};
    static const u8 creqv_expected[4] = {1, 0, 0, 1};
    static const u8 crnand_expected[4] = {1, 1, 1, 0};
    static const u8 crnor_expected[4] = {1, 0, 0, 0};
    static const u8 crorc_expected[4] = {1, 0, 1, 1};
    static const u8 crxor_expected[4] = {0, 1, 1, 0};
    check_cr_logic("crand", dolphin_crand, crand_expected);
    check_cr_logic("crandc", dolphin_crandc, crandc_expected);
    check_cr_logic("creqv", dolphin_creqv, creqv_expected);
    check_cr_logic("crnand", dolphin_crnand, crnand_expected);
    check_cr_logic("crnor", dolphin_crnor, crnor_expected);
    check_cr_logic("crorc", dolphin_crorc, crorc_expected);
    check_cr_logic("crxor", dolphin_crxor, crxor_expected);

    u32 cr_seed = 0x10000000u;
    asm volatile(
        "mtcrf 0xff,%1\n"
        "cror 2,3,4\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed)
        : "cr0"
    );
    check_eq((cr >> 29) & 1u, 1, "cror copies true source");

    cr_seed = 0x12345678u;
    asm volatile(
        "mtcrf 0xff,%1\n"
        "mcrf cr2,cr3\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed)
        : "cr0"
    );
    check_eq((cr >> 20) & 0xFu, 0x4, "mcrf copies source field");
    check_eq((cr >> 16) & 0xFu, 0x4, "mcrf leaves source field");

    cr_seed = 0xA5A50000u;
    asm volatile(
        "mtcrf 0xff,%1\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed)
        : "cr0"
    );
    check_eq(cr, 0xA5A50000u, "mfcr reads CR");

    cr_seed = 0x12345678u;
    asm volatile(
        "mtcrf 0xff,%1\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed)
        : "cr0"
    );
    check_eq(cr, 0x12345678u, "mtcrf full mask writes CR");

    u32 partial_seed = 0x89ABCDEFu;
    u32 old_cr = 0x11111111u;
    asm volatile(
        "mtcrf 0xff,%2\n"
        "mtcrf 0x90,%1\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(partial_seed), "r"(old_cr)
        : "cr0"
    );
    check_eq(cr, 0x811B1111u, "mtcrf partial mask writes selected fields");

    cr_seed = 0xFFFFFFFFu;
    old_cr = 0x2468ACE0u;
    asm volatile(
        "mtcrf 0xff,%2\n"
        "mtcrf 0x00,%1\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed), "r"(old_cr)
        : "cr0"
    );
    check_eq(cr, 0x2468ACE0u, "mtcrf zero mask leaves CR");

    cr_seed = 0x0000000Fu;
    old_cr = 0x12345670u;
    asm volatile(
        "mtcrf 0xff,%2\n"
        "mtcrf 0x01,%1\n"
        "mfcr %0\n"
        : "=r"(cr)
        : "r"(cr_seed), "r"(old_cr)
        : "cr0"
    );
    check_eq(cr, 0x1234567Fu, "mtcrf low mask writes CR7");
}

static void test_immediate_logical(void) {
    kprintf("--- immediate logical ---\n");
    printf("--- immediate logical ---\n");

    u32 result;
    u32 cr;
    u32 src = 0x12340000;

    asm volatile("ori %0,%1,0x5678" : "=r"(result) : "r"(src));
    check_eq(result, 0x12345678, "ori low half");

    src = 0x00005678;
    asm volatile("oris %0,%1,0x1234" : "=r"(result) : "r"(src));
    check_eq(result, 0x12345678, "oris high half");

    src = 0xAAAA0000;
    asm volatile("xori %0,%1,0xFFFF" : "=r"(result) : "r"(src));
    check_eq(result, 0xAAAAFFFF, "xori low half");

    src = 0x2AAA5555;
    asm volatile("xoris %0,%1,0x8000" : "=r"(result) : "r"(src));
    check_eq(result, 0xAAAA5555, "xoris high half");

    src = 0x123400F0;
    asm volatile(
        "andi. %0,%2,0x00FF\n"
        "mfcr %1\n"
        : "=&r"(result), "=r"(cr)
        : "r"(src)
        : "cr0"
    );
    check_eq(result, 0xF0, "andi. result");
    check_eq((cr >> 28) & 0xFu, 0x4, "andi. updates CR0 nonzero");

    src = 0x12FF0000;
    asm volatile(
        "andis. %0,%2,0x00FF\n"
        "mfcr %1\n"
        : "=&r"(result), "=r"(cr)
        : "r"(src)
        : "cr0"
    );
    check_eq(result, 0x00FF0000, "andis. result");
    check_eq((cr >> 28) & 0xFu, 0x4, "andis. updates CR0 nonzero");
}

static void test_register_arithmetic(void) {
    kprintf("--- register arithmetic ---\n");
    printf("--- register arithmetic ---\n");

    u32 result;
    u32 xer;
    u32 saved_xer;
    u32 a = 10;
    u32 b = 20;

    asm volatile("add %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 30, "add");

    asm volatile("mfxer %0" : "=r"(saved_xer));

    a = 0xFFFFFFFFu;
    b = 1;
    asm volatile(
        "addc %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b)
    );
    check_eq(result, 0, "addc wraps");
    check_eq(xer & 0x20000000u, 0x20000000u, "addc sets CA on carry");

    a = 0xFFFFFFFFu;
    b = 0;
    u32 ca = 0x20000000u;
    asm volatile(
        "mtxer %4\n"
        "adde %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b), "r"(ca)
    );
    check_eq(result, 0, "adde includes CA");
    check_eq(xer & 0x20000000u, 0x20000000u, "adde keeps carry");

    a = 0xFFFFFFFFu;
    asm volatile(
        "mtxer %3\n"
        "addze %0,%2\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(ca)
    );
    check_eq(result, 0, "addze includes CA");
    check_eq(xer & 0x20000000u, 0x20000000u, "addze sets carry");

    a = 5;
    b = 9;
    asm volatile("subf %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 4, "subf rB-rA");

    a = 5;
    b = 5;
    asm volatile(
        "subfc %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b)
    );
    check_eq(result, 0, "subfc equal");
    check_eq(xer & 0x20000000u, 0x20000000u, "subfc equal sets CA");

    a = 7;
    b = 5;
    asm volatile(
        "mtxer %4\n"
        "subfe %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b), "r"(ca)
    );
    check_eq(result, 0xFFFFFFFEu, "subfe with borrow result");
    check_eq(xer & 0x20000000u, 0, "subfe borrow clears CA");

    a = 0;
    asm volatile(
        "mtxer %3\n"
        "subfze %0,%2\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(ca)
    );
    check_eq(result, 0, "subfze zero with CA");
    check_eq(xer & 0x20000000u, 0x20000000u, "subfze zero sets CA");

    a = 5;
    asm volatile("neg %0,%1" : "=r"(result) : "r"(a));
    check_eq(result, 0xFFFFFFFBu, "neg");

    u32 cr;
    a = 0xFFFFFFFFu;
    b = 1;
    asm volatile(
        "add. %0,%2,%3\n"
        "mfcr %1\n"
        : "=&r"(result), "=r"(cr)
        : "r"(a), "r"(b)
        : "cr0"
    );
    check_eq(result, 0, "add. result");
    check_eq((cr >> 28) & 0xFu, 0x2, "add. records EQ");

    asm volatile("mtxer %0" : : "r"(saved_xer));
}

static void test_logical_shift_rotate(void) {
    kprintf("--- logical / shift / rotate ---\n");
    printf("--- logical / shift / rotate ---\n");

    u32 result;
    u32 xer;
    u32 a = 0xF0F0F0F0u;
    u32 b = 0x0FF00FF0u;

    asm volatile("and %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x00F000F0u, "and");

    a = 0x0FF00FF0u;
    b = 0x00FF00FFu;
    asm volatile("andc %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x0F000F00u, "andc");

    a = 0x12340000u;
    b = 0x00005678u;
    asm volatile("or %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x12345678u, "or");

    a = 0;
    b = 0xFFFF0000u;
    asm volatile("orc %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x0000FFFFu, "orc");

    a = 0xAAAA5555u;
    b = 0xFFFF0000u;
    asm volatile("xor %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x55555555u, "xor");

    a = 0xFFFFFFFFu;
    b = 0x0F0F0F0Fu;
    asm volatile("nand %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0xF0F0F0F0u, "nand");

    a = 0xF0000000u;
    b = 0x0F000000u;
    asm volatile("nor %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x00FFFFFFu, "nor");

    a = 0xFFFF0000u;
    b = 0xFF00FF00u;
    asm volatile("eqv %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0xFF0000FFu, "eqv");

    a = 0;
    asm volatile("cntlzw %0,%1" : "=r"(result) : "r"(a));
    check_eq(result, 32, "cntlzw zero");

    a = 0x00F00000u;
    asm volatile("cntlzw %0,%1" : "=r"(result) : "r"(a));
    check_eq(result, 8, "cntlzw leading zeros");

    a = 0x00000080u;
    asm volatile("extsb %0,%1" : "=r"(result) : "r"(a));
    check_eq(result, 0xFFFFFF80u, "extsb sign");

    a = 0x00008001u;
    asm volatile("extsh %0,%1" : "=r"(result) : "r"(a));
    check_eq(result, 0xFFFF8001u, "extsh sign");

    a = 1;
    b = 31;
    asm volatile("slw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x80000000u, "slw shift 31");

    b = 32;
    asm volatile("slw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0, "slw shift 32 clears");

    a = 0x80000000u;
    b = 31;
    asm volatile("srw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 1, "srw shift 31");

    b = 32;
    asm volatile("srw %0,%1,%2" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0, "srw shift 32 clears");

    a = 0x80000001u;
    b = 1;
    asm volatile(
        "sraw %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b)
    );
    check_eq(result, 0xC0000000u, "sraw arithmetic shift");
    check_eq(xer & 0x20000000u, 0x20000000u, "sraw sets CA when shifting out ones");

    b = 32;
    asm volatile(
        "sraw %0,%2,%3\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a), "r"(b)
    );
    check_eq(result, 0xFFFFFFFFu, "sraw shift 32 sign fills");
    check_eq(xer & 0x20000000u, 0x20000000u, "sraw shift 32 sets CA for negative");

    a = 0x00000080u;
    asm volatile(
        "srawi %0,%2,7\n"
        "mfxer %1\n"
        : "=&r"(result), "=r"(xer)
        : "r"(a)
    );
    check_eq(result, 1, "srawi positive");
    check_eq(xer & 0x20000000u, 0, "srawi positive clears CA");

    a = 0x12345678u;
    asm volatile("rlwinm %0,%1,5,8,23" : "=r"(result) : "r"(a));
    check_eq(result, 0x008ACF00u, "rlwinm mask");

    a = 0x89ABCDEFu;
    b = 36;
    asm volatile("rlwnm %0,%1,%2,4,27" : "=r"(result) : "r"(a), "r"(b));
    check_eq(result, 0x0ABCDEF0u, "rlwnm masks variable rotate");

    result = 0xAA00AA00u;
    a = 0x12345678u;
    asm volatile("rlwimi %0,%1,8,8,15" : "+r"(result) : "r"(a) : "cr0");
    check_eq(result, 0xAA56AA00u, "rlwimi inserts masked rotate");
}

static void test_loads(void) {
    kprintf("--- loads ---\n");
    printf("--- loads ---\n");

    u32 result;
    volatile u32 words[8];
    words[0] = 0xDEADBEEF;
    words[1] = 0xCAFEBABE;
    words[2] = 0x12345678;

    asm volatile("lwz %0,0(%1)" : "=r"(result) : "r"(words) : "memory");
    check_eq(result, 0xDEADBEEF, "lwz offset 0");

    volatile u32* wp = &words[0];
    asm volatile("lwzu %0,4(%1)" : "=r"(result), "+r"(wp) : : "memory");
    check_eq(result, 0xCAFEBABE, "lwzu loads updated address");
    check((u32)wp == (u32)&words[1], "lwzu updates rA");

    volatile u8 bytes[8];
    bytes[0] = 0x11;
    bytes[1] = 0xA5;
    bytes[2] = 0x7F;

    asm volatile("lbz %0,1(%1)" : "=r"(result) : "r"(bytes) : "memory");
    check_eq(result, 0xA5, "lbz zero-extends byte");

    volatile u8* bp = &bytes[0];
    asm volatile("lbzu %0,2(%1)" : "=r"(result), "+r"(bp) : : "memory");
    check_eq(result, 0x7F, "lbzu loads updated address");
    check((u32)bp == (u32)&bytes[2], "lbzu updates rA");

    volatile u16 halves[4];
    halves[0] = 0x1234;
    halves[1] = 0x8001;
    halves[2] = 0x7FFF;

    asm volatile("lhz %0,0(%1)" : "=r"(result) : "r"(halves) : "memory");
    check_eq(result, 0x1234, "lhz zero-extends halfword");

    volatile u16* hp = &halves[0];
    asm volatile("lhzu %0,4(%1)" : "=r"(result), "+r"(hp) : : "memory");
    check_eq(result, 0x7FFF, "lhzu loads updated address");
    check((u32)hp == (u32)&halves[2], "lhzu updates rA");

    asm volatile("lha %0,2(%1)" : "=r"(result) : "r"(halves) : "memory");
    check_eq(result, 0xFFFF8001, "lha sign-extends halfword");

    volatile u16* hp2 = &halves[0];
    asm volatile("lhau %0,2(%1)" : "=r"(result), "+r"(hp2) : : "memory");
    check_eq(result, 0xFFFF8001, "lhau sign-extends halfword");
    check((u32)hp2 == (u32)&halves[1], "lhau updates rA");

    volatile u32 many[12];
    volatile u32 lmw_out[2];
    for (u32 i = 0; i < 12; i++)
        many[i] = 0xA0000014u + i;
    dolphin_test_lmw_fn((const u32*)many, (u32*)lmw_out);
    check_eq(lmw_out[0], 0xA0000014u, "lmw first register");
    check_eq(lmw_out[1], 0xA000001Fu, "lmw last register");
}

static void test_stores(void) {
    kprintf("--- stores ---\n");
    printf("--- stores ---\n");

    volatile u32 words[8];
    memset((void*)words, 0, sizeof(words));

    u32 value = 0xAABBCCDD;
    asm volatile("stw %0,4(%1)" : : "r"(value), "r"(words) : "memory");
    check_eq(words[1], 0xAABBCCDD, "stw offset 4");

    volatile u32* wp = &words[1];
    value = 0x11223344;
    asm volatile("stwu %1,4(%0)" : "+r"(wp) : "r"(value) : "memory");
    check_eq(words[2], 0x11223344, "stwu stores updated address");
    check((u32)wp == (u32)&words[2], "stwu updates rA");

    volatile u8 bytes[8];
    memset((void*)bytes, 0, sizeof(bytes));

    value = 0x123456A5;
    asm volatile("stb %0,1(%1)" : : "r"(value), "r"(bytes) : "memory");
    check_eq(bytes[1], 0xA5, "stb stores low byte");

    volatile u8* bp = &bytes[1];
    value = 0x0000007E;
    asm volatile("stbu %1,2(%0)" : "+r"(bp) : "r"(value) : "memory");
    check_eq(bytes[3], 0x7E, "stbu stores updated address");
    check((u32)bp == (u32)&bytes[3], "stbu updates rA");

    volatile u16 halves[8];
    memset((void*)halves, 0, sizeof(halves));

    value = 0xFFFFABCD;
    asm volatile("sth %0,2(%1)" : : "r"(value), "r"(halves) : "memory");
    check_eq(halves[1], 0xABCD, "sth stores low halfword");

    volatile u16* hp = &halves[1];
    value = 0x00001234;
    asm volatile("sthu %1,4(%0)" : "+r"(hp) : "r"(value) : "memory");
    check_eq(halves[3], 0x1234, "sthu stores updated address");
    check((u32)hp == (u32)&halves[3], "sthu updates rA");

    volatile u32 stmw_out[12];
    memset((void*)stmw_out, 0, sizeof(stmw_out));
    dolphin_test_stmw_fn((u32*)stmw_out);
    check_eq(stmw_out[0], 0xB0000014u, "stmw first register");
    check_eq(stmw_out[11], 0xB000001Fu, "stmw last register");
}

static void test_indexed_memory(void) {
    kprintf("--- indexed memory ---\n");
    printf("--- indexed memory ---\n");

    u32 result;
    u32 off;

    volatile u32 words[16];
    memset((void*)words, 0, sizeof(words));
    words[8] = 0x01020304;

    off = 8 * sizeof(words[0]);
    asm volatile("lwzx %0,%1,%2" : "=r"(result) : "r"(words), "r"(off) : "memory");
    check_eq(result, 0x01020304, "lwzx");

    volatile u32* wp = &words[0];
    asm volatile("lwzux %0,%1,%2" : "=r"(result), "+r"(wp) : "r"(off) : "memory");
    check_eq(result, 0x01020304, "lwzux");
    check((u32)wp == (u32)&words[8], "lwzux updates rA");

    volatile u8 bytes[64];
    memset((void*)bytes, 0, sizeof(bytes));
    bytes[0x24] = 0xA5;

    off = 0x24;
    asm volatile("lbzx %0,%1,%2" : "=r"(result) : "r"(bytes), "r"(off) : "memory");
    check_eq(result, 0xA5, "lbzx");

    volatile u8* bp = &bytes[0];
    asm volatile("lbzux %0,%1,%2" : "=r"(result), "+r"(bp) : "r"(off) : "memory");
    check_eq(result, 0xA5, "lbzux");
    check((u32)bp == (u32)&bytes[0x24], "lbzux updates rA");

    volatile u16 halves[32];
    memset((void*)halves, 0, sizeof(halves));
    halves[0x14] = 0x1234;
    halves[0x16] = 0x8003;

    off = 0x14 * sizeof(halves[0]);
    asm volatile("lhzx %0,%1,%2" : "=r"(result) : "r"(halves), "r"(off) : "memory");
    check_eq(result, 0x1234, "lhzx");

    volatile u16* hp = &halves[0];
    asm volatile("lhzux %0,%1,%2" : "=r"(result), "+r"(hp) : "r"(off) : "memory");
    check_eq(result, 0x1234, "lhzux");
    check((u32)hp == (u32)&halves[0x14], "lhzux updates rA");

    off = 0x16 * sizeof(halves[0]);
    asm volatile("lhax %0,%1,%2" : "=r"(result) : "r"(halves), "r"(off) : "memory");
    check_eq(result, 0xFFFF8003, "lhax");

    hp = &halves[0];
    asm volatile("lhaux %0,%1,%2" : "=r"(result), "+r"(hp) : "r"(off) : "memory");
    check_eq(result, 0xFFFF8003, "lhaux");
    check((u32)hp == (u32)&halves[0x16], "lhaux updates rA");

    words[8] = 0;
    u32 value = 0xAABBCCDD;
    off = 8 * sizeof(words[0]);
    asm volatile("stwx %0,%1,%2" : : "r"(value), "r"(words), "r"(off) : "memory");
    check_eq(words[8], 0xAABBCCDD, "stwx");

    wp = &words[0];
    value = 0x11223344;
    asm volatile("stwux %1,%0,%2" : "+r"(wp) : "r"(value), "r"(off) : "memory");
    check_eq(words[8], 0x11223344, "stwux");
    check((u32)wp == (u32)&words[8], "stwux updates rA");

    bytes[0x28] = 0;
    value = 0x0000005A;
    off = 0x28;
    asm volatile("stbx %0,%1,%2" : : "r"(value), "r"(bytes), "r"(off) : "memory");
    check_eq(bytes[0x28], 0x5A, "stbx");

    bp = &bytes[0];
    value = 0x0000006B;
    asm volatile("stbux %1,%0,%2" : "+r"(bp) : "r"(value), "r"(off) : "memory");
    check_eq(bytes[0x28], 0x6B, "stbux");
    check((u32)bp == (u32)&bytes[0x28], "stbux updates rA");

    halves[0x18] = 0;
    value = 0x0000CAFE;
    off = 0x18 * sizeof(halves[0]);
    asm volatile("sthx %0,%1,%2" : : "r"(value), "r"(halves), "r"(off) : "memory");
    check_eq(halves[0x18], 0xCAFE, "sthx");

    hp = &halves[0];
    value = 0x0000FACE;
    asm volatile("sthux %1,%0,%2" : "+r"(hp) : "r"(value), "r"(off) : "memory");
    check_eq(halves[0x18], 0xFACE, "sthux");
    check((u32)hp == (u32)&halves[0x18], "sthux updates rA");

    words[8] = 0x01020304u;
    off = 8 * sizeof(words[0]);
    asm volatile("lwbrx %0,%1,%2" : "=r"(result) : "r"(words), "r"(off) : "memory");
    check_eq(result, 0x04030201u, "lwbrx byte-reverses word");

    words[12] = 0x11223344u;
    asm volatile("lwbrx %0,0,%1" : "=r"(result) : "r"(&words[12]) : "memory");
    check_eq(result, 0x44332211u, "lwbrx uses zero base when rA is zero");

    halves[0x1A] = 0x1234u;
    off = 0x1A * sizeof(halves[0]);
    asm volatile("lhbrx %0,%1,%2" : "=r"(result) : "r"(halves), "r"(off) : "memory");
    check_eq(result, 0x3412u, "lhbrx byte-reverses halfword");

    words[9] = 0;
    value = 0xA1B2C3D4u;
    off = 9 * sizeof(words[0]);
    asm volatile("stwbrx %0,%1,%2" : : "r"(value), "r"(words), "r"(off) : "memory");
    check_eq(words[9], 0xD4C3B2A1u, "stwbrx byte-reverses word");

    halves[0x1B] = 0;
    value = 0x00001234u;
    off = 0x1B * sizeof(halves[0]);
    asm volatile("sthbrx %0,%1,%2" : : "r"(value), "r"(halves), "r"(off) : "memory");
    check_eq(halves[0x1B], 0x3412u, "sthbrx byte-reverses halfword");

    static volatile u32 dcbz_words[16] __attribute__((aligned(32)));
    dcbz_words[7] = 0x11111111u;
    for (u32 i = 8; i < 16; i++)
        dcbz_words[i] = 0xFFFFFFFFu;
    asm volatile(
        "dcbz %0,%1\n"
        "sync\n"
        :
        : "r"(&dcbz_words[8]), "r"(0x13u)
        : "memory"
    );
    check_eq(dcbz_words[8], 0, "dcbz clears first word of cache block");
    check_eq(dcbz_words[15], 0, "dcbz clears last word of cache block");
    check_eq(dcbz_words[7], 0x11111111u, "dcbz leaves previous block");

    for (u32 i = 0; i < 8; i++)
        dcbz_words[i] = 0x77777777u;
    asm volatile(
        "dcbz 0,%0\n"
        "sync\n"
        :
        : "r"((u8*)&dcbz_words[0] + 0x17)
        : "memory"
    );
    check_eq(dcbz_words[0], 0, "dcbz uses zero base when rA is zero");
}

static void test_branches_and_spr(void) {
    kprintf("--- branches / SPR ---\n");
    printf("--- branches / SPR ---\n");

    u32 flag = 0;
    asm volatile(
        "b 1f\n"
        "li %0,1\n"
        "1:\n"
        : "+r"(flag)
    );
    check_eq(flag, 0, "b skips instruction");

    u32 saved_lr;
    u32 new_lr;
    asm volatile(
        "mflr %0\n"
        "bl 1f\n"
        "1:\n"
        "mflr %1\n"
        "mtlr %0\n"
        : "=&r"(saved_lr), "=r"(new_lr)
        :
        : "lr"
    );
    check(new_lr != 0, "bl sets LR nonzero");
    check(new_lr != saved_lr, "bl changes LR");

    check_eq(dolphin_test_blr_fn(), 0x55, "bclr/blr returns from asm function");
    check_eq(dolphin_test_bctr_fn(), 0x66, "bcctr/bctr jumps through CTR");

    u32 ctr_value = 5;
    u32 ctr_readback;
    asm volatile(
        "mtctr %1\n"
        "mfctr %0\n"
        : "=r"(ctr_readback)
        : "r"(ctr_value)
        : "ctr"
    );
    check_eq(ctr_readback, 5, "mtspr/mfspr CTR roundtrip");

    u32 lr_readback;
    asm volatile(
        "mflr %0\n"
        "mtlr %0\n"
        "mflr %1\n"
        : "=&r"(saved_lr), "=r"(lr_readback)
        :
        : "lr"
    );
    check_eq(lr_readback, saved_lr, "mtspr/mfspr LR roundtrip");
}

int main(void) {
    VIDEO_Init();

    GXRModeObj* rmode = VIDEO_GetPreferredMode(NULL);
    void* xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    CON_Init(xfb, 20, 20, rmode->fbWidth, rmode->xfbHeight,
             rmode->fbWidth * VI_DISPLAY_PIX_SZ);

    if (fatInitDefault()) {
        reference_file = fopen("sd:/dolrecomp_cpu_reference.txt", "w");
        if (!reference_file)
            reference_file = fopen("dolrecomp_cpu_reference.txt", "w");
    }

    reference_printf("\n=== DolRecomp opcode test (real PPC) ===\n\n");
    reference_printf("CPUREF_BEGIN,version,1\n");

    test_immediate_arithmetic();
    test_compare_and_bc();
    test_register_arithmetic();
    test_immediate_logical();
    test_logical_shift_rotate();
    test_loads();
    test_stores();
    test_indexed_memory();
    test_branches_and_spr();

    reference_printf("\n%d passed, %d failed\n", pass_count, fail_count);

    if (fail_count == 0) {
        reference_printf("\nALL TESTS PASSED\n");
    } else {
        reference_printf("\nSOME TESTS FAILED\n");
    }

    reference_printf("CPUREF_END\n");
    if (reference_file)
        fclose(reference_file);

    while (1) {
        VIDEO_WaitVSync();
    }

    return 0;
}
