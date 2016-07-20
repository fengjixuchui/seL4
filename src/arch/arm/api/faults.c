/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <config.h>
#include <types.h>
#include <object.h>
#include <kernel/vspace.h>
#include <api/faults.h>
#include <api/syscall.h>

bool_t
Arch_handleFaultReply(tcb_t *receiver, tcb_t *sender, word_t faultType)
{
    switch (faultType) {
    case seL4_Fault_VMFault:
        return true;

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
    case fault_vgic_maintenance:
        return true;
    case fault_vcpu_fault:
        return true;
#endif
    default:
        fail("Invalid fault");
    }
}

word_t
Arch_setMRs_fault(tcb_t *sender, tcb_t* receiver, word_t *receiveIPCBuffer, word_t faultType)
{
    switch (faultType) {
    case seL4_Fault_VMFault: {
        if (config_set(CONFIG_ARM_HYPERVISOR_SUPPORT)) {
            word_t ipa, va;
            va = getRestartPC(sender);
            ipa = (addressTranslateS1CPR(va) & ~MASK(PAGE_BITS)) | (va & MASK(PAGE_BITS));
            setMR(receiver, receiveIPCBuffer, seL4_VMFault_IP, ipa);
        } else {
            setMR(receiver, receiveIPCBuffer, seL4_VMFault_IP, getRestartPC(sender));
        }
        setMR(receiver, receiveIPCBuffer, seL4_VMFault_Addr,
              seL4_Fault_VMFault_get_address(sender->tcbFault));
        setMR(receiver, receiveIPCBuffer, seL4_VMFault_PrefetchFault,
              seL4_Fault_VMFault_get_instructionFault(sender->tcbFault));
        return setMR(receiver, receiveIPCBuffer, seL4_VMFault_FSR,
                     seL4_Fault_VMFault_get_FSR(sender->tcbFault));
    }

#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
    case fault_vgic_maintenance:
        if (fault_vgic_maintenance_get_idxValid(sender->tcbFault)) {
            return setMR(receiver, receiveIPCBuffer, seL4_VGICMaintenance_IDX,
                         fault_vgic_maintenance_get_idx(sender->tcbFault));
        } else {
            return setMR(receiver, receiveIPCBuffer, seL4_VGICMaintenance_IDX, -1);
        }
    case fault_vcpu_fault:
        return setMR(receiver, receiveIPCBuffer, seL4_VCPUFault_HSR, fault_vcpu_fault_get_hsr(sender->tcbFault));
#endif

    default:
        fail("Invalid fault");
    }
}
