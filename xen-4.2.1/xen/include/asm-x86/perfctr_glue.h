/*
 * Perfctr support for Xen/Dom
 * Copyright (C) 2010 Ruslan Nikolaev
 */

#ifndef __ASM_PERFCTR_GLUE_H__
#define __ASM_PERFCTR_GLUE_H__

#include <xen/spinlock.h>
#include <xen/errno.h>
#include <xen/cpumask.h>
#include <xen/cache.h>
#include <xen/time.h>
#include <xen/preempt.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/cpufeature.h>
#include <asm/smp.h>

/* Cache line alignment */
#ifndef ____cacheline_aligned
# define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))
#endif

/* Compatibility CPU mask macros */
# define cpu_set(x,y)	cpumask_set_cpu((x), &(y))
# define cpu_isset(x,y)	cpumask_test_cpu((x), &(y))
# define cpus_clear(x)	cpumask_clear(&(x))
# define cpus_empty(x)	cpumask_empty(&(x))

/* Compilation constants */
#define CONFIG_PERFCTR 1
#define CONFIG_PERFCTR_INTERRUPT_SUPPORT 1

/* Some staff for Xen */
#define LOCAL_PERFCTR_VECTOR PMU_APIC_VECTOR
extern unsigned int nmi_perfctr_msr;
extern void disable_lapic_nmi_watchdog(void);

/* missing from <asm-i386/cpufeature.h> */
#define cpu_has_msr     boot_cpu_has(X86_FEATURE_MSR)

#define perfctr_cpu_khz()   ((unsigned int)cpu_khz)

/* From version.h */
#define VERSION "2.6.41"

/* From linux/perfctr.h */
#ifdef CONFIG_PERFCTR_DEBUG
#define VERSION_DEBUG " DEBUG"
#else
#define VERSION_DEBUG
#endif
struct perfctr_info {
        unsigned int abi_version;
        char driver_version[32];
        unsigned int cpu_type;
        unsigned int cpu_features;
        unsigned int cpu_khz;
        unsigned int tsc_to_cpu_mult;
        unsigned int _reserved2;
        unsigned int _reserved3;
        unsigned int _reserved4;
};
/* abi_version values: Lower 16 bits contain the CPU data version, upper
   16 bits contain the API version. Each half has a major version in its
   upper 8 bits, and a minor version in its lower 8 bits. */
#define PERFCTR_API_VERSION     0x0502  /* 5.2 */
#define PERFCTR_ABI_VERSION     ((PERFCTR_API_VERSION<<16)|PERFCTR_CPU_VERSION)
/* cpu_features flag bits */
#define PERFCTR_FEATURE_RDPMC   0x01
#define PERFCTR_FEATURE_RDTSC   0x02
#define PERFCTR_FEATURE_PCINT   0x04
extern struct perfctr_info perfctr_info;

/* From x86_tests.h */
enum perfctr_x86_tests_type {
        PTT_UNKNOWN,
        PTT_GENERIC,
        PTT_P5,
        PTT_P6,
        PTT_CORE2,
        PTT_P4,
        PTT_AMD,
        PTT_WINCHIP,
        PTT_VC3,
};

/* From compat.h */
static inline struct cpuinfo_x86 *perfctr_cpu_data(int cpu)
{
        return &cpu_data[cpu];
}
#undef cpu_data
#define cpu_data(cpu)   (*perfctr_cpu_data((cpu)))

/* From cpumask.h */
/* CPUs in `perfctr_cpus_forbidden_mask' must not use the
   performance-monitoring counters. TSC use is unrestricted.
   This is needed to prevent resource conflicts on hyper-threaded P4s. */
extern cpumask_t perfctr_cpus_forbidden_mask;
#define perfctr_cpu_is_forbidden(cpu)   cpu_isset((cpu), perfctr_cpus_forbidden_mask)

/* Make perfctr code think that it's Linux */
#define LINUX_VERSION_CODE 132640
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#define __KERNEL__ 1

/* We don't need any of those */
#define THIS_MODULE 0
#define __module_get(x)
#define module_put(x)
#define __SI_FAULT 0
#define sysdev_class_register(x) 0
#define sysdev_class_unregister(x)
#define sysdev_register(x)
#define sysdev_unregister(x)
#define perfctr_set_tests_type(x)

/* The code is borrowed from arch/x86/cpu/mtrr/main.c
   No blocking mutexes in Xen. Spin instead. */
#define DEFINE_MUTEX(_m) DEFINE_SPINLOCK(_m)
#define mutex_lock(_m) spin_lock(_m)
#define mutex_unlock(_m) spin_unlock(_m)
#define dump_stack() ((void)0)
#define get_cpu()       smp_processor_id()
#define put_cpu()       do {} while(0)

#ifndef CONFIG_SMP
# define CONFIG_SMP	1
#endif

#endif /* __ASM_PERFCTR_GLUE_H__ */
