/* $Id: x86_setup.c,v 1.47.2.9 2009/01/23 17:21:20 mikpe Exp $
 * Performance-monitoring counters driver.
 * x86/x86_64-specific kernel-resident code.
 *
 * Copyright (C) 1999-2007, 2009  Mikael Pettersson
 */
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
#include <linux/config.h>
#endif
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <asm/processor.h>
#include <asm/perfctr.h>
#include <asm/fixmap.h>
#include <asm/apic.h>
#include "x86_compat.h"
#include "compat.h"

#ifdef CONFIG_PERFCTR_XEN
# include <xen/interface/xen.h>
# include <xen/events.h>
# include "virq.h"
#endif

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

#ifdef CONFIG_PERFCTR_XEN
static int xen_perfctr_irq[NR_CPUS];

/* VIRQ handler for Xen version */
static irqreturn_t xen_perfctr_interrupt(int irq, void *dev_id)
{
	if (!interrupts_masked[smp_processor_id()])
		(*perfctr_ihandler)();
	return IRQ_HANDLED;
}

/* VIRQ unbinding for Xen version */
void xen_perfctr_virq_unbind(void)
{
    int i;

    for_each_online_cpu(i) {
        if (xen_perfctr_irq[i] >= 0) {
            unbind_from_irqhandler(xen_perfctr_irq[i], NULL);
            xen_perfctr_irq[i] = -1;
        }
    }
}

/* VIRQ binding for Xen version */
int xen_perfctr_virq_bind(void)
{
    int irq, i;

    for (i = 0; i < NR_CPUS; i++)
        xen_perfctr_irq[i] = -1;

    for_each_online_cpu(i) {
        irq = bind_virq_to_irqhandler(VIRQ_PERFCTR, i, xen_perfctr_interrupt,
            IRQF_DISABLED|IRQF_PERCPU|IRQF_NOBALANCING, "perfctr_irq", NULL);

        if (irq < 0) {
            xen_perfctr_virq_unbind();
            return irq;
        }

        xen_perfctr_irq[i] = irq;
    }
    return 0;
}

/* keep a stub for now */
asmlinkage void smp_perfctr_interrupt(struct pt_regs *regs)
{
	ack_APIC_irq();
}
#else
asmlinkage void smp_perfctr_interrupt(struct pt_regs *regs)
{
	/* PREEMPT note: invoked via an interrupt gate, which
	   masks interrupts. We're still on the originating CPU. */
	/* XXX: recursive interrupts? delay the ACK, mask LVTPC, or queue? */
	ack_APIC_irq();
	if (interrupts_masked[smp_processor_id()])
		return;
	irq_enter();
	(*perfctr_ihandler)();
	irq_exit();
}
#endif

void perfctr_cpu_set_ihandler(perfctr_ihandler_t ihandler)
{
	perfctr_ihandler = ihandler ? ihandler : perfctr_default_ihandler;
}
#endif

#if defined(__x86_64__) || LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
extern unsigned int cpu_khz;
#else
extern unsigned long cpu_khz;
#endif

unsigned int perfctr_cpu_khz(void)
{
	return cpu_khz;
}

#ifdef CONFIG_PERFCTR_MODULE
EXPORT_SYMBOL_mmu_cr4_features;
EXPORT_SYMBOL(perfctr_cpu_khz);

#ifdef CONFIG_X86_LOCAL_APIC

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
#include <asm/nmi.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
EXPORT_SYMBOL(disable_lapic_nmi_watchdog);
EXPORT_SYMBOL(enable_lapic_nmi_watchdog);
#else
EXPORT_SYMBOL(setup_apic_nmi_watchdog);
EXPORT_SYMBOL(stop_apic_nmi_watchdog);
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,6)
EXPORT_SYMBOL(nmi_perfctr_msr);
#endif

EXPORT_SYMBOL(__perfctr_cpu_mask_interrupts);
EXPORT_SYMBOL(__perfctr_cpu_unmask_interrupts);
EXPORT_SYMBOL(perfctr_cpu_set_ihandler);

#ifdef CONFIG_PERFCTR_XEN
EXPORT_SYMBOL(xen_perfctr_virq_bind);
EXPORT_SYMBOL(xen_perfctr_virq_unbind);
#endif
#endif /* CONFIG_X86_LOCAL_APIC */

#endif /* MODULE */
