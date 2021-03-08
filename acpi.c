#include <common.h>
#include <init.h>
#include <string.h>
#include <acpi.h>
#include <asm.h>

#define RSDT_SIG "RSD PTR"
#define FADT_SIG "FACP"
#define MADT_SIG "APIC"
#define HPET_SIG "HPET"
#define SIG_LEN  4
#define ADDRESS_SPACE_ID_MEM	0
#define ADDRESS_SPACE_ID_IO	1

struct rsdp {
	u8 signature[8];
	u8 checksum1;
	u8 oemid[6];
	u8 revision;
	u32 rsdt_address;
	u32 length;
	u64 xsdt_address;
	u8 checksum2;
	u8 reserved[3];
} __attribute__ ((packed));

struct description_header {
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	u8 oemid[6];
	u8 oem_table_id[8];
	u8 oem_revision[4];
	u8 creator_id[4];
	u8 creator_revision[4];
} __attribute__ ((packed));

struct rsdt {
	struct description_header header;
	u32 entry[];
} __attribute__ ((packed));

struct xsdt {
	struct description_header header;
	u64 entry[];
} __attribute__ ((packed));

struct gas {
	u8 address_space_id;
	u8 register_bit_width;
	u8 register_bit_offset;
	u8 access_size;
	u64 address;
} __attribute__ ((packed));

struct fadt {
	struct description_header header;
	u32 firmware_ctrl;
	u32 dsdt;
	u8 reserved1;
	u8 preferred_pm_profile;
	u16 sci_int;
	u32 smi_cmd;
	u8 acpi_enable;
	u8 acpi_disable;
	u8 s4bios_req;
	u8 pstate_cnt;
	u32 pm1a_evt_blk;
	u32 pm1b_evt_blk;
	u32 pm1a_cnt_blk;
	u32 pm1b_cnt_blk;
	u32 pm2_cnt_blk;
	u32 pm_tmr_blk;
	u32 gpe0_blk;
	u32 gpe1_blk;
	u8 pm1_evt_len;
	u8 pm1_cnt_len;
	u8 pm2_cnt_len;
	u8 pm_tmr_len;
	u8 gpe0_blk_len;
	u8 gpe1_blk_len;
	u8 gpe1_base;
	u8 cst_cnt;
	u16 p_lvl2_lat;
	u16 p_lvl3_lat;
	u16 flush_size;
	u16 flush_stride;
	u8 duty_offset;
	u8 duty_width;
	u8 day_alrm;
	u8 mon_alrm;
	u8 century;
	u16 iapc_boot_arch;
	u8 reserved2;
	u32 flags;
	struct gas reset_reg;
	u8 reset_value;
	u8 reserved3[3];
	u64 x_firmware_ctrl;
	u64 x_dsdt;
	struct gas x_pm1a_evt_blk;
	struct gas x_pm1b_evt_blk;
	struct gas x_pm1a_cnt_blk;
	struct gas x_pm1b_cnt_blk;
	struct gas x_pm2_cnt_blk;
	struct gas x_pm_tmr_blk;
	struct gas x_gpe0_blk;
	struct gas x_gpe1_blk;
} __attribute__ ((packed));

struct madt {
	struct description_header header;
        u32 local_ctrl_addr;
        u32 flags;
} __attribute__ ((packed));

struct madt_entry {
        u8 entry_type;
        u8 length;
        union {
                struct {
                        u8 proc_id;
                        u8 apic_id;
                        u32 flags;
                } __attribute__((packed)) lapic;
                struct {
                        u8 ioapic_id;
                        u8 reserved;
                        u32 ioapic_addr;
                        u32 glob_sys_intr_base;
                } __attribute__((packed)) ioapic;
                struct {
                        u8 bus_src;
                        u8 irq_src;
                        u32 glob_sys_intr;
                        u16 flags;
                } __attribute__((packed)) intr;
        };
} __attribute__ ((packed));

struct hpet {
	struct description_header header;
	u32 block_id;
	struct gas hpet;
	u8 hpet_num;
	u16 counter_min;
	u8 oem_attr;
} __attribute__ ((packed));

static struct rsdp *acpi_rsdp;
static struct rsdt *acpi_rsdt;
static struct xsdt *acpi_xsdt;
static struct madt *acpi_madt;
static struct fadt *acpi_fadt;
static struct hpet *acpi_hpet;
static struct cpulist *acpi_cpulist;

static u16 acpi_pmt_iobase;
static u64 acpi_hpet_mmiobase;

struct rsdp *
acpi_get_rsdp (void)
{
	long i, p;
	u32 begin, end;
	bool found;

	p = 0;
	found = false;
        begin = (u32) *((u16 *) 0x40e) << 2;
	end = begin + 0x3ff;
	for (i = begin; i < end; i++) {
		if (!memcmp ((void *) i, RSDT_SIG, strlen(RSDT_SIG))) {
			found = true;
			break;
		}
	}
	if (!found) {
		for (i = 0xe0000; i < 0xfffff; i++) {
			if (!memcmp ((void *)i, RSDT_SIG, strlen(RSDT_SIG))) {
				found = true;
				p = i;
				break;
			}
		}
	}
	return (void *) p;
}

static struct rsdt *
acpi_get_rsdt (struct rsdp *rsdp)
{
	return (void *) (long) rsdp->rsdt_address;
}

static struct xsdt *
acpi_get_xsdt (struct rsdp *rsdp)
{
	return (void *) rsdp->xsdt_address;
}

static void *
acpi_get_xsdt_entry (struct xsdt *xsdt, char *sig)
{
	int i, len;
	void *p;
	
	p = NULL;
	len = (xsdt->header.length - sizeof xsdt->header) / 4;
	for (i = 0; i < len; i++) {
		if (!memcmp ((void *) (long) xsdt->entry[i], sig, SIG_LEN)) {
			p = (void *) (long) xsdt->entry[i];
			break;
		}
	}
	return p;
}

static void *
acpi_get_rsdt_entry (struct rsdt *rsdt, char *sig)
{
	int i, len;
	void *p;
	
	p = NULL;
	len = (rsdt->header.length - sizeof rsdt->header) / 4;
	for (i = 0; i < len; i++) {
		if (!memcmp ((void *) (long) rsdt->entry[i], sig, SIG_LEN)) {
			p = (void *) (long) rsdt->entry[i];
			break;
		}
	}
	return p;
}

static void *
acpi_get_entry (struct rsdt *rsdt, struct xsdt *xsdt, char *sig)
{
	void *p;

	p = NULL;
	if (xsdt)
		p = acpi_get_xsdt_entry (xsdt, sig);
	if (!p && rsdt)
		p = acpi_get_rsdt_entry (rsdt, sig);
	return p;
}

static struct cpulist *
acpi_create_cpulist (struct madt *madt)
{
	struct cpulist *cpulist;
	struct cpuinfo *cpuinfo;
	struct madt_entry *entry;
	void *end;
	int cpunum, i;

	cpunum = 0;
	entry = (struct madt_entry *)(madt + 1);
	end = (void *) madt + madt->header.length;
	while (entry != end) {
		if (entry->entry_type == 0
		    && (entry->lapic.flags & 1)) {
			cpunum ++;
		}
		entry = (void *) 
			entry
			+ entry->length;
	}
	i = 0;
	cpulist = malloc (sizeof *cpulist);
	cpuinfo = malloc (cpunum * 
			  sizeof *cpuinfo);
	cpulist->num = cpunum;
	cpulist->info = cpuinfo;
	entry = (struct madt_entry *)(madt + 1);
	while (entry != end) {
		if (entry->entry_type == 0) {
			cpulist->info[i].proc_id =
				entry->lapic.proc_id;
			cpulist->info[i].apic_id =
				entry->lapic.apic_id;
			i++;
		}
		entry = (void *) 
			entry
			+ entry->length;
	}

	return cpulist;
}

#define offsetof(st, m) __builtin_offsetof(st, m)

u16
acpi_get_pmt_timer_iobase (void)
{
	return acpi_pmt_iobase;
}

u64
acpi_get_hpet_mmiobase (void)
{
	return acpi_hpet_mmiobase;
}

static void 
acpi_pmt_init (struct fadt *fadt)
{
	if (fadt->header.length > 
	    offsetof (struct fadt, x_pm_tmr_blk) + sizeof (fadt->x_pm_tmr_blk) &&
	    fadt->x_pm_tmr_blk.address_space_id 
	    == ADDRESS_SPACE_ID_IO && 
	    fadt->x_pm_tmr_blk.address <= 0xfff) {
		acpi_pmt_iobase = fadt->x_pm_tmr_blk.address;
	} else if (fadt->header.length > 
		   offsetof (struct fadt, pm1a_cnt_blk) + sizeof (fadt->pm1a_cnt_blk)) {
		acpi_pmt_iobase = fadt->pm1a_cnt_blk;
	} else {
		acpi_pmt_iobase = 0;
	}
	if (!acpi_pmt_iobase) {
		printf ("ACPI: ACPI Timer not found.\n");
		return;
	}
	printf ("ACPI: ACPI Timer found: %04x.\n", 
		acpi_pmt_iobase);
}

static void 
acpi_hpet_init (struct hpet *hpet)
{
	acpi_hpet_mmiobase = hpet->hpet.address;
	if (!acpi_hpet_mmiobase) {
		printf ("ACPI: HPET not found.\n");
		return;
	}
	printf ("ACPI: HPET found: %016llx\n",
		acpi_hpet_mmiobase);
}

static void 
acpi_init (void)
{
	int i;

	printd ("ACPI: %s().\n", __func__);

	acpi_rsdp = acpi_get_rsdp ();
	if (!acpi_rsdp) {
		printf ("ACPI: RSDP not found.\n");
		return;
	}
	acpi_rsdt = acpi_get_rsdt (acpi_rsdp);
	acpi_xsdt = acpi_get_xsdt (acpi_rsdp);
	if (!acpi_rsdt && !acpi_xsdt) {
		printf ("ACPI: Neither RSDT nor XSDT not found.\n");
		return;
	}
	acpi_fadt = acpi_get_entry (acpi_rsdt, acpi_xsdt, FADT_SIG);
	if (!acpi_fadt) {
		printf ("ACPI: FADT not found.\n");
		return;
	}
	acpi_madt = acpi_get_entry (acpi_rsdt, acpi_xsdt, MADT_SIG);
	if (!acpi_madt) {
		printf ("ACPI: MADT not found.\n");
		return;
	}
	acpi_hpet = acpi_get_entry (acpi_rsdt, acpi_xsdt, HPET_SIG);
	if (!acpi_hpet) {
		printf ("ACPI: HPET not found.\n");
		return;
	}

	printi ("ACPI: Table {\n");
	printi ("  RSDP %p\n", acpi_rsdp);
	printi ("  RSDT %p\n", acpi_rsdt);
	printi ("  XSDT %p\n", acpi_xsdt);
	printi ("  FADT %p\n", acpi_fadt);
	printi ("  MADT %p\n", acpi_madt);
	printi ("  HPET %p\n", acpi_hpet);
	printi ("}\n");

	acpi_cpulist = acpi_create_cpulist (acpi_madt);
	printi ("CPU: %d processors detected {\n", acpi_cpulist->num);
	for (i = 0; i < acpi_cpulist->num; i++) {
		printi ("  CPU%d: ProcessorID %02x, APICID %02x\n",
			i, 
			acpi_cpulist->info[i].proc_id,
			acpi_cpulist->info[i].apic_id);
	}
	printi ("}\n");

	printf ("ACPI: %d processors detected.\n",
		acpi_cpulist->num);

	acpi_pmt_init (acpi_fadt);
	acpi_hpet_init (acpi_hpet);
}

struct cpulist *
acpi_get_cpulist (void)
{
	return acpi_cpulist;
}

static void 
acpi_deinit (void)
{
	printd ("ACPI: %s().\n", __func__);
}

register_init ("bsp1", acpi_init);
register_deinit ("bsp1", acpi_deinit);
