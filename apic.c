#include <common.h>
#include <acpi.h>
#include <asm.h>
#include <init.h>
#include <boot.h>
#include <msr.h>

struct stacklist {
	u64 apic_id;
	u64 stack_base;
} __attribute__((packed)) *stacklist;

static u64 boot_apic_id;

static void 
apic_init (void)
{
	struct cpulist *cpulist;
	u8 apic_id;
	int i;
	u64 base;

	printd ("APIC: %s().\n", __func__);

	asm_rdmsr (MSR_IA32_APIC_BASE, &base);
	asm_wrmsr (MSR_IA32_APIC_BASE, 
		   base | (1<<10) | (1<<11));
	asm_rdmsr (MSR_X2APIC_ID, &boot_apic_id);

	cpulist = acpi_get_cpulist ();
	stacklist = malloc (cpulist->num * sizeof *stacklist);
	for (i = 0; i < cpulist->num; i++) {
		apic_id = cpulist->info[i].apic_id;
		if (apic_id == boot_apic_id) {
			stacklist[i].apic_id = apic_id;
			stacklist[i].stack_base = BOOT_BSP_STACK_BASE;
			continue;
		}
		printd ("APIC: Send IPI to local APIC %02x\n",
			apic_id);
		stacklist[i].apic_id = apic_id;
		stacklist[i].stack_base = (u64)malloc (BOOT_STACK_SIZE);
		asm_wrmsr (MSR_X2APIC_ICR, 0x4500 | ((u64) apic_id << 32));
		asm_wrmsr (MSR_X2APIC_ICR, 0x4600 | ((u64) apic_id << 32) | 8);
		asm_wrmsr (MSR_X2APIC_ICR, 0x4600 | ((u64) apic_id << 32) | 8);
	}
}

static void 
apic_deinit (void)
{
	struct cpulist *cpulist;
	int i;
	u8 apic_id;

	printd ("APIC: %s().\n", __func__);

	cpulist = acpi_get_cpulist ();
	for (i = 0; i < cpulist->num; i++) {
		apic_id = cpulist->info[i].apic_id;
		if (apic_id == boot_apic_id) {
			continue;
		}
		asm_wrmsr (MSR_X2APIC_ICR, 0x4500 | ((u64) apic_id << 32));
		printf ("APIC: Processor %02x stopped.\n",
			apic_id);
	}
}

register_init ("ap0", apic_init);
register_deinit ("ap0", apic_deinit);
