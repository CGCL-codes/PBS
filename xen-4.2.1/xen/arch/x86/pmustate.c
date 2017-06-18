/*
 * Perfctr support for Xen/Dom
 * Copyright (C) 2010 Ruslan Nikolaev
 */

#include <xen/errno.h>
#include <xen/init.h>
#include <xen/guest_access.h>
#include <xen/cache.h>
#include <xen/event.h>
#include <asm/pmustate.h>
#include <asm/hardirq.h>

#ifdef CONFIG_PMUSTATE_VIRT

/* We need to have access to some static members;
   that's why we include the entire file. */
#include "perfctr.c"

/* From init.c */
char *perfctr_cpu_name __initdata;
struct perfctr_info perfctr_info = {
    .abi_version = PERFCTR_ABI_VERSION,
    .driver_version = VERSION VERSION_DEBUG,
};

int __read_mostly opt_perfctr_enabled;
boolean_param("perfctr", opt_perfctr_enabled);

#include <asm/hvm/hvm.h>

#ifdef CONFIG_X86_LOCAL_APIC

static void perfctr_default_ihandler(void)
{
}

static perfctr_ihandler_t perfctr_ihandler = perfctr_default_ihandler;
static unsigned int interrupts_masked[NR_CPUS] __cacheline_aligned;

void __perfctr_cpu_mask_interrupts(void)
{
    interrupts_masked[smp_processor_id()] = 1;
}

void __perfctr_cpu_unmask_interrupts(void)
{
    interrupts_masked[smp_processor_id()] = 0;
}

void perfctr_cpu_set_ihandler(perfctr_ihandler_t ihandler)
{
    perfctr_ihandler = ihandler ? ihandler : perfctr_default_ihandler;
}

void pmu_do_interrupt(struct cpu_user_regs *regs)
{
    if (interrupts_masked[smp_processor_id()])
        return;
    irq_enter();
    (*perfctr_ihandler)();
    irq_exit();
}
#endif

#ifdef CONFIG_PERFCTR_INTERRUPT_SUPPORT

static void pmu_ihandler(void)
{
    struct vcpu *v = current;

    if (lvtpc_reinit_needed)
        apic_write(APIC_LVTPC, LOCAL_PERFCTR_VECTOR);

    send_guest_vcpu_virq(v, VIRQ_PERFCTR);
}

static inline void pmu_set_ihandler(void)
{
    perfctr_cpu_set_ihandler(pmu_ihandler);
}

#else
static inline void pmu_set_ihandler(void) { }
#endif

void pmu_save_regs(struct vcpu *v)
{
    struct domain *d = v->domain;
    struct perfctr_cpu_state *state;
    unsigned long flags;	

    /* We calculate difference for HVM in a tricky way: we first add
       the current TSC value here but in pmu_restore_regs() we will substruct
       the following TSC value. In this way, we do not need an additional
       variable for the current value. */
    if (!opt_perfctr_enabled)
    {
        if (is_hvm_vcpu(v))
            rdtscll(v->arch.hvm_vcpu.tsc_last);
    }
    else if (d->shared_info)
    {
        state = (struct perfctr_cpu_state *)((char *)d->shared_info + PAGE_SIZE) + v->vcpu_id;
        spin_lock_irqsave(&v->arch.perfctr_lock, flags);
		//modified by zhao
		//perfctr_cpu_suspend(state);
		perfctr_cpu_vsuspend(state, v);
        spin_unlock_irqrestore(&v->arch.perfctr_lock, flags);
    }
}

void pmu_restore_regs(struct vcpu *v)
{
    struct domain *d = v->domain;
    struct perfctr_cpu_state *state;
    uint64_t tsc_now;
    unsigned long flags;

    /* Update an HVM value */
    if (!opt_perfctr_enabled)
    {
        if (is_hvm_vcpu(v)) {
            rdtscll(tsc_now);
            v->arch.hvm_vcpu.cache_tsc_offset += v->arch.hvm_vcpu.tsc_last - tsc_now;
        }
    }
    else if (d->shared_info)
    {
        state = (struct perfctr_cpu_state *)((char *)d->shared_info + PAGE_SIZE) + v->vcpu_id;
        spin_lock_irqsave(&v->arch.perfctr_lock, flags);
        perfctr_cpu_resume(state);
        spin_unlock_irqrestore(&v->arch.perfctr_lock, flags);
    }
}

/* Set TSC value */
int pmu_init_vcpu(struct vcpu *v)
{
    struct domain *d = v->domain;
    struct perfctr_cpu_state *state;

    /* Allow only when shared_info is present */
    if (opt_perfctr_enabled && d->shared_info)
    {
        state = (struct perfctr_cpu_state *)((char *)d->shared_info + PAGE_SIZE) + v->vcpu_id;
        memset(state, 0, sizeof(*state));
    }
    return 0;
}

void pmu_init(void)
{
    static const char this_service[] = __FILE__;

    if (opt_perfctr_enabled) {
        perfctr_cpu_init();
        perfctr_cpu_reserve(this_service);
        pmu_set_ihandler();
    }
}

static int pmu_perfctr_resume(XEN_GUEST_HANDLE(void) arg)
{
    struct vcpu *v = current;
    struct perfctr_cpu_state *state;
    unsigned int i, nrctrs;
    unsigned long flags;

    if (!opt_perfctr_enabled)
        return -ENODEV;
    state = (struct perfctr_cpu_state *)((char *)v->domain->shared_info + PAGE_SIZE) + v->vcpu_id;
    spin_lock_irqsave(&v->arch.perfctr_lock, flags);
    copy_from_guest(state, arg, 1);
    perfctr_cpu_resume(state);
    nrctrs = perfctr_cstatus_nractrs(state->cstatus);
    for (i = 0; i < nrctrs; i++)
        state->pmc[i].sum = 0;
    state->tsc_sum = 0;
    spin_unlock_irqrestore(&v->arch.perfctr_lock, flags);
    return 0;
}

static int pmu_perfctr_isuspend(void)
{
    struct vcpu *v = current;
    struct perfctr_cpu_state *state;
    unsigned long flags;

    if (!opt_perfctr_enabled)
        return -ENODEV;
    state = (struct perfctr_cpu_state *)((char *)v->domain->shared_info + PAGE_SIZE) + v->vcpu_id;
    spin_lock_irqsave(&v->arch.perfctr_lock, flags);
    perfctr_cpu_isuspend(state);
    spin_unlock_irqrestore(&v->arch.perfctr_lock, flags);
    return 0;
}

int do_perfctr_op(int cmd, XEN_GUEST_HANDLE(void) arg)
{
    int res;

    switch (cmd)
    {
        case PERFCTROP_RESUME:
            res = pmu_perfctr_resume(arg);
            break;
        case PERFCTROP_ISUSPEND:
            res = pmu_perfctr_isuspend();
            break;
        default:
            res = -EINVAL;
    }
    return res;
}

#else /* !CONFIG_PMUSTATE_VIRT */

int do_perfctr_op(int cmd, XEN_GUEST_HANDLE(void) arg)
{
    return -EINVAL;
}

#endif
