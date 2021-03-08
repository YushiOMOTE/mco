#ifndef __ACPI_H
#define __ACPI_H

struct cpuinfo {
	u8 proc_id;
	u8 apic_id;
} __attribute__((packed));

struct cpulist {
	u16 num;
	struct cpuinfo *info;
};

struct cpulist *acpi_get_cpulist (void);
u16 acpi_get_pmt_timer_iobase (void);
u64 acpi_get_hpet_mmiobase (void);

#endif	/* __ACPI_H */
