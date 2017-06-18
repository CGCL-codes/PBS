/*
 * Perfctr support for Xen/Dom
 * Copyright (C) 2010 Ruslan Nikolaev
 */

#ifndef __ASM_PMUSTATE_H__
#define __ASM_PMUSTATE_H__

#include <xen/sched.h>
#include <asm/processor.h>

#ifdef CONFIG_PMUSTATE_VIRT

#define PERFCTROP_RESUME   0
#define PERFCTROP_ISUSPEND 1

extern int __read_mostly opt_perfctr_enabled;

void pmu_init(void);
void pmu_do_interrupt(struct cpu_user_regs *regs);
void pmu_save_regs(struct vcpu *v);
void pmu_restore_regs(struct vcpu *v);
int pmu_init_vcpu(struct vcpu *v);

#else /* !CONFIG_PMUSTATE_VIRT */

#define opt_perfctr_enabled 0

static inline void pmu_init(void) {}
static inline void pmu_do_interrupt(struct cpu_user_regs *regs) {}
static inline void pmu_save_regs(struct vcpu *v) {}
static inline void pmu_restore_regs(struct vcpu *v) {}
static inline int pmu_init_vcpu(struct vcpu *v) { return 0; }

#endif /* CONFIG_PMUSTATE_VIRT */

#endif /* !__ASM_PMUSTATE_H__ */
