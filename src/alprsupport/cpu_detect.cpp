/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "cpu_detect.h"

#ifdef _WIN32
// Import windows only stuff
	#include <windows.h>
	#include <sys/timeb.h>

	#define timespec timeval

#else
#include <sys/time.h>
#endif


#ifdef _MSC_VER
/* MSVC < 2010 does not have stdint.h */
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h>
#endif

// Support for OS X
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
#include <Windows.h>
#else
#include <chrono>
#endif  // _MSC_VER

#include <fstream>
/*
 * Using the cpuid instruction to get CPU information
 *
 * See: https://en.wikipedia.org/wiki/CPUID
 */

#include <string.h>


namespace alprsupport {

    
    
    
#if !defined(_MSC_VER)
/*
 * We're asking whether the OS supports AVX.
 *
 * Quoting from: https://en.wikipedia.org/wiki/Advanced_Vector_Extensions
 * "AVX adds new register-state through the 256-bit wide YMM register file, so
 * explicit operating system support is required to properly save and restore
 * AVX's expanded registers between context switches."
 *
 * In order to save the registers:
 * - the CPU needs to support XSAVE, XRESTOR, XSETBV, XGETBV instructions
 *   (checked elsewhere with _cpuid(1) call, checking bit 26 of ECX);
 * - the XSAVE instruction should be "enabled by OS" (cpuid(1), bit 27 of
 *   ECX, checked elsewhere);
 * - A check whether the expanded YMM registers are saved / restored on
 *   context switch.
 *
 * The routines here check do this last check.
 *
 * If we can use assembly, we can use the XGETBV x86 instruction to tell us
 * whether the registers are preserved on context switch:
 * https://software.intel.com/en-us/blogs/2011/04/14/is-avx-enabled/
 *
 * On MSVC we have a problem, because the MSVC 2010 SP1 is the first version
 * of MSVC to have a compiler intrinsic for XGETBV, and MSVC does not support
 * inline ``__asm`` in 64-bit:
 * http://forums.codeguru.com/showthread.php?551499-xgetbv
 * https://msdn.microsoft.com/en-us/library/hb5z4sxd.aspx
 *
 * Therefore, for MSVC, we have to use the GetEnabledXStateFeatures function
 * to check if the feature is enabled:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/hh134235(v=vs.85).aspx
 * https://msdn.microsoft.com/en-us/library/windows/desktop/hh134240(v=vs.85).aspx
 *
 * When we can drop support for MSVC < 2010 SP1, then we can use the
 * ``_XGETBV`` intrinsic instead of this workaround:
 * https://msdn.microsoft.com/en-us/library/hh977023.aspx
 */

#include <stdint.h>

void _xgetbv(uint32_t op, uint32_t* eax_var, uint32_t* edx_var)
{
    /* Use binary code for xgetbv
     *
     * We should not reach this check unless cpuid says the CPU has xgetbv
     * instructions via cpuid(1) and ECX bit 26 (xsave):
     * https://en.wikipedia.org/wiki/CPUID
     */
    __asm__ __volatile__
        (".byte 0x0f, 0x01, 0xd0": "=a" (*eax_var), "=d" (*edx_var) : "c" (op) : "cc");
}

int os_restores_ymm(void)
{
    uint32_t eax_val, edx_val;
    // XFEATURE_ENABLED_MASK/XCR0 register number = 0
    _xgetbv(0, &eax_val, &edx_val);
    // XFEATURE_ENABLED_MASK register is in edx:eax
    return (eax_val & 6) == 6;
}

#else
/*
 * This is the workaround where we cannot use the XGETBV instruction for the
 * check.
 *
 * Much of this code fragment copied from:
 * https://msdn.microsoft.com/en-us/library/windows/desktop/hh134240(v=vs.85).aspx
 * I (MB) assume this code is public domain given its apparent intended
 * purpose as an exmaple of code usage.
*/
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <winerror.h>

// Windows 7 SP1 is the first version of Windows to support the AVX API.

// Since the AVX API is not declared in the Windows 7 SDK headers and
// since we don't have the proper libs to work with, we will declare
// the API as function pointers and get them with GetProcAddress calls
// from kernel32.dll.  We also need to set some #defines.

#define XSTATE_AVX                          (XSTATE_GSSE)
#define XSTATE_MASK_AVX                     (XSTATE_MASK_GSSE)

typedef DWORD64 (WINAPI *PGETENABLEDXSTATEFEATURES)();
PGETENABLEDXSTATEFEATURES pfnGetEnabledXStateFeatures = NULL;

typedef BOOL (WINAPI *PINITIALIZECONTEXT)(PVOID Buffer, DWORD ContextFlags, PCONTEXT* Context, PDWORD ContextLength);
PINITIALIZECONTEXT pfnInitializeContext = NULL;

typedef BOOL (WINAPI *PGETXSTATEFEATURESMASK)(PCONTEXT Context, PDWORD64 FeatureMask);
PGETXSTATEFEATURESMASK pfnGetXStateFeaturesMask = NULL;

typedef PVOID (WINAPI *LOCATEXSTATEFEATURE)(PCONTEXT Context, DWORD FeatureId, PDWORD Length);
LOCATEXSTATEFEATURE pfnLocateXStateFeature = NULL;

typedef BOOL (WINAPI *SETXSTATEFEATURESMASK)(PCONTEXT Context, DWORD64 FeatureMask);
SETXSTATEFEATURESMASK pfnSetXStateFeaturesMask = NULL;

int xstate_has_avx(void)
{
    DWORD64 FeatureMask;

    // If this function was called before and we were not running on
    // at least Windws 7 SP1, then bail.
    if (pfnGetEnabledXStateFeatures == (PGETENABLEDXSTATEFEATURES)-1)
    {
        return -1;
    }

    // Get the addresses of the AVX XState functions.
    if (pfnGetEnabledXStateFeatures == NULL)
    {
        HMODULE hm = GetModuleHandle(_T("kernel32.dll"));
        if (hm == NULL)
        {
            pfnGetEnabledXStateFeatures = (PGETENABLEDXSTATEFEATURES)-1;
            return -2;
        }

        pfnGetEnabledXStateFeatures = (PGETENABLEDXSTATEFEATURES)GetProcAddress(hm, "GetEnabledXStateFeatures");
        pfnInitializeContext = (PINITIALIZECONTEXT)GetProcAddress(hm, "InitializeContext");
        pfnGetXStateFeaturesMask = (PGETXSTATEFEATURESMASK)GetProcAddress(hm, "GetXStateFeaturesMask");
        pfnLocateXStateFeature = (LOCATEXSTATEFEATURE)GetProcAddress(hm, "LocateXStateFeature");
        pfnSetXStateFeaturesMask = (SETXSTATEFEATURESMASK)GetProcAddress(hm, "SetXStateFeaturesMask");

        if (pfnGetEnabledXStateFeatures == NULL
            || pfnInitializeContext == NULL
            || pfnGetXStateFeaturesMask == NULL
            || pfnLocateXStateFeature == NULL
            || pfnSetXStateFeaturesMask == NULL)
        {
            pfnGetEnabledXStateFeatures = (PGETENABLEDXSTATEFEATURES)-1;
            return -3;
        }
    }

    FeatureMask = pfnGetEnabledXStateFeatures();
    return ((FeatureMask & XSTATE_MASK_AVX) != 0);
}


int os_restores_ymm(void)
{
    return xstate_has_avx() > 0;
}
#endif

    
    
    
    
    
int has_cpuid(void)
{
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
    /* All 64-bit capable chips have cpuid, according to
     * https://software.intel.com/en-us/articles/using-cpuid-to-detect-the-presence-of-sse-41-and-sse-42-instruction-sets/
     */
    return 1;
#else
    /*
     * cpuid instruction present if it is possible to set the ID bit in EFLAGS.
     * ID bit is 0x200000 (21st bit).
     *
     * https://software.intel.com/en-us/articles/using-cpuid-to-detect-the-presence-of-sse-41-and-sse-42-instruction-sets/
     * http://wiki.osdev.org/CPUID
     */
    int tf = 0;
#if defined (_MSC_VER)
    /*
     * See also comments in for gcc asm below.
     * Notes on MSVX asm and registers at
     * https://msdn.microsoft.com/en-us/library/k1a8ss06.aspx
     */
    __asm {
        push ecx;  just in case (__fastcall needs ecx preserved)
        pushfd;  original eflags
        pushfd;  original eflags again
        pop eax;  copy of eflags into eax
        mov ecx, eax;  store original eflags in ecx
        xor eax, 200000h;  flip bit 21
        push eax;  set eflags from eax
        popfd;  this call will unflip bit 21 if we lack cpuid
        pushfd;  copy of new eflags into eax
        pop eax
        xor eax, ecx;  check whether copy of eflags still has flipped bit
        shr eax, 21;   1 if bit still flipped, 0 otherwise
        mov tf, eax;  put eax result into return variable
        popfd  ; restore original eflags
        pop ecx ; restore original ecx
    }
#else
    __asm__ __volatile__(
        "pushfl; pop %%eax;"  /* get current eflags into eax */
        "mov %%eax, %%ecx;"  /* store copy of input eflags in ecx */
        "xorl $0x200000, %%eax;"  /* flip bit 21 */
        "push %%eax; popfl;"  /* try to set eflags with this bit flipped */
        "pushfl; pop %%eax;"  /* get resulting eflags back into eax */
        "xorl %%ecx, %%eax;"  /* is bit still flipped cf original? */
        "shrl $21, %%eax;"   /* if so, we have cpuid */
        : "=a" (tf)  /* outputs */
        :            /* inputs */
        : "cc", "%ecx");     /* eflags and ecx are clobbered */
#endif
    return tf;
#endif
}

/* shift a by b bits to the right, then mask with c */
#define SHIFT_MASK(a, b, c) ((((a) >> (b)) & (c)))

/*
 * https://software.intel.com/en-us/articles/using-cpuid-to-detect-the-presence-of-sse-41-and-sse-42-instruction-sets/
 * https://software.intel.com/en-us/articles/how-to-detect-new-instruction-support-in-the-4th-generation-intel-core-processor-family
 * https://en.wikipedia.org/wiki/CPUID#CPUID_usage_from_high-level_languages
 */
void read_cpuid(uint32_t op, uint32_t sub_op, e_registers_t* reg)
{
#if defined(_MSC_VER)
    int cpu_info[4] = {-1};
    __cpuidex(cpu_info, (int)op, (int)sub_op);
    reg->eax = cpu_info[0];
    reg->ebx = cpu_info[1];
    reg->ecx = cpu_info[2];
    reg->edx = cpu_info[3];
#else
    __asm__ __volatile__(
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
        "pushq %%rbx;" /* save %rbx on 64-bit */
#else
        "pushl %%ebx;" /* save %ebx on 32-bit */
#endif
        "cpuid;"
        /* write EBX value somewhere we can find it */
        "movl %%ebx, %[ebx_store];"
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
        "popq %%rbx;"
#else
        "popl %%ebx;"
#endif
        : "=a" (reg->eax), [ebx_store] "=rm" (reg->ebx), "=c" (reg->ecx), "=d" (reg->edx)
        : "a" (op), "c" (sub_op)
        : "cc");
#endif
}

void read_vendor_string(e_registers_t cpuid_1, char* vendor)
{
    /* Read vendor string from ebx, edx, ecx
     * Registers in `cpuid_1` resulted from call to read_cpuid(1, 0, &cpuid_1)
     */
    uint32_t* char_as_int=(uint32_t*)vendor;
    *(char_as_int++) = cpuid_1.ebx;
    *(char_as_int++) = cpuid_1.edx;
    *(char_as_int) = cpuid_1.ecx;
    vendor[12] = '\0';
}

void read_brand_string(char* brand)
{
    /*
     * https://en.wikipedia.org/wiki/CPUID#EAX.3D80000002h.2C80000003h.2C80000004h:_Processor_Brand_String
     */
    uint32_t* char_as_int=(uint32_t*)brand;
    int op;
    e_registers_t registers;
    /* does this cpuid support extended calls up to 0x80000004? */
    read_cpuid(0x80000000, 0, &registers);
    if (registers.eax < 0x80000004)
    {
        brand[0] = '\0';
        return;
    }
    for (op = 0x80000002; op < 0x80000005; op++)
    {
        read_cpuid(op, 0, &registers);
        *(char_as_int++) = registers.eax;
        *(char_as_int++) = registers.ebx;
        *(char_as_int++) = registers.ecx;
        *(char_as_int++) = registers.edx;
    }
}

void read_classifiers(e_registers_t cpuid_1, cpu_classifiers_t* cpu_params)
{
    /*
    * 3:0 – Stepping
    * 7:4 – Model
    * 11:8 – Family
    * 13:12 – Processor Type
    * 19:16 – Extended Model
    * 27:20 – Extended Family
    * See:
    * https://en.wikipedia.org/wiki/CPUID#EAX.3D1:_Processor_Info_and_Feature_Bits
    * Page 3-191 of
    * http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html
    */
    uint32_t eax = cpuid_1.eax;
    cpu_params->stepping = SHIFT_MASK(eax, 0, 0x0f);
    cpu_params->model = SHIFT_MASK(eax,  4, 0x0f);
    cpu_params->family = SHIFT_MASK(eax, 8, 0x0f);
    cpu_params->processor_type = SHIFT_MASK(eax, 12, 0x03);
    cpu_params->extended_model = SHIFT_MASK(eax, 16, 0x0f);
    cpu_params->extended_family = SHIFT_MASK(eax, 20, 0xff);
}

int os_supports_avx(e_registers_t cpuid_1)
{
    /*
     * The cpuid(1) ECX register tells us if the CPU supports AVX.
     *
     * For the OS to support AVX, it needs to preserve the AVX YMM registers
     * when doing a context switch.  In order to do this, the needs to support
     * the relevant instructions, and the OS needs to know to preserve these
     * registers.
     *
     * See:
     * https://en.wikipedia.org/wiki/CPUID
     * https://en.wikipedia.org/wiki/Advanced_Vector_Extensions
     * https://software.intel.com/en-us/blogs/2011/04/14/is-avx-enabled/
     */
    uint32_t mask = BIT_MASK(26, 28);
    return ((cpuid_1.ecx & mask) == mask) ? os_restores_ymm() : 0;
}


// Returns CPU info.  You must free() the response to avoid a memory leak
cpu_info_t * cpu_detect(void)
{
    cpu_info_t * info = (cpu_info_t *)malloc(sizeof(cpu_info_t));
    if (!info) {
        exit(1);
    }
    int res = collect_info(info);
    if (!res) {
        std::cerr << "Failed to read cpuid!" << std::endl;
        exit(1);
    }
    return info;
}

int has_sse(cpu_info_t * info) {
    return info->has_sse4_2;
}

int has_avx(cpu_info_t * info) {
    return info->supports_avx;
}
int collect_info(cpu_info_t *info)
{
    cpu_classifiers_t cpu_classifiers;
    uint32_t edx1, ecx1;

    if (! has_cpuid() )
    {
        return 0;
    }
    read_cpuid(0, 0, &(info->reg0));
    info->max_basic_arg = info->reg0.eax;
    read_cpuid(1, 0, &(info->reg1));
    info->has_reg7 = info->max_basic_arg >= 7;
    if (info->has_reg7)
    {
        read_cpuid(7, 0, &(info->reg7));
    }
    read_vendor_string(info->reg0, info->vendor);
    read_brand_string(info->brand);
    read_classifiers(info->reg1, &cpu_classifiers);
    info->stepping = cpu_classifiers.stepping;
    info->model = cpu_classifiers.model;
    info->family = cpu_classifiers.family;
    info->processor_type = cpu_classifiers.processor_type;
    info->extended_model = cpu_classifiers.extended_model;
    info->extended_family = cpu_classifiers.extended_family;
    /* Implement algorithm in
     http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html
     */
    if ((info->family == 6) | (info->family == 15))
    {
        info->model_display = (info->extended_model << 4) + info->model;
    } else {
        info->model_display = info->model;
    }
    if (info->family == 15)
    {
        info->family_display = (info->extended_family << 4) + info->family;
    } else {
        info->family_display = info->family;
    }
    info->signature = info->reg1.eax;
    edx1 = info->reg1.edx;
    ecx1 = info->reg1.ecx;
    info->has_mmx = HAS_BIT(edx1, 23);
    info->has_sse = HAS_BIT(edx1, 25);
    info->has_sse2 = HAS_BIT(edx1, 26);
    info->has_3dnow = HAS_BIT(edx1, 26);
    info->has_sse3 = HAS_BIT(ecx1, 0);
    info->has_ssse3 = HAS_BIT(ecx1, 9);
    info->has_sse4_1 = HAS_BIT(ecx1, 19);
    info->has_sse4_2 = HAS_BIT(ecx1, 20);
    /* supports_avx
     * True if CPU and OS support AVX instructions
     *
     * The cpuid(1) ECX register tells us if the CPU supports AVX.
     *
     * For the OS to support AVX, it needs to preserve the AVX YMM registers
     * when doing a context switch.  In order to do this, the needs to support
     * the relevant instructions, and the OS needs to know to preserve these
     * registers.
     * See:
     * https://en.wikipedia.org/wiki/cpuid
     * https://en.wikipedia.org/wiki/Advanced_Vector_Extensions
     * https://software.intel.com/en-us/blogs/2011/04/14/is-avx-enabled/
     */
    info->supports_avx = os_supports_avx(info->reg1);
    info->supports_avx2 = (
            info->supports_avx &
            info->has_reg7 &
            HAS_BIT(info->reg7.ebx, 5));
    return 1;
}


};
