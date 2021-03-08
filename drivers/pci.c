#define __DEBUG_PRINT
#include <common.h>
#include <string.h>
#include <asm.h>
#include <init.h>
#include <drivers/pci.h>

#define PCI_ADDR_PORT 0xcf8
#define PCI_DATA_PORT 0xcfc

#define PCI_BUS_MAX 256
#define PCI_DEV_MAX 32
#define PCI_FUNC_MAX 8

static struct pci_list {
	struct pci *pci;
	struct pci_list *next;
} *pci_list_head;

static struct pci_driver_list {
	struct pci_driver *driver;
	struct pci_driver_list *next;
} *pci_driver_list_head;

static inline u32
pci_addr (struct pci *pci, u32 reg)
{
	return (reg << 2 |
		pci->func << 8 |
		pci->dev << 11 |
		pci->bus << 16 |
		1 << 31);
}

static u32
pci_config_read (struct pci *pci, u32 reg)
{
	u32 value;

	asm_out32 (PCI_ADDR_PORT, pci_addr (pci, reg >> 2));
	asm_in32 (PCI_DATA_PORT, &value);

	value = (value >> ((reg & 3) * 8));

	return value;
}

u8
pci_config_read8 (struct pci *pci, u32 reg)
{
	return ((u8) pci_config_read (pci, reg) & 0xff);
}

u16
pci_config_read16 (struct pci *pci, u32 reg)
{
	return ((u16) pci_config_read (pci, reg) & 0xffff);
}

u32
pci_config_read32 (struct pci *pci, u32 reg)
{
	return ((u32) pci_config_read (pci, reg));
}

static void 
pci_register (struct pci *pci)
{
	struct pci_list *p;

	p = malloc (sizeof *p);
	p->pci = pci;
	p->next = pci_list_head;
	pci_list_head = p;
}

void 
pci_driver_register (struct pci_driver *pci_driver)
{
	struct pci_driver_list *p;

	p = malloc (sizeof *p);
	p->driver = pci_driver;
	p->next = pci_driver_list_head;
	pci_driver_list_head = p;
}

static bool 
pci_match (struct pci *pci, struct pci_driver *driver)
{
	int i;
	u32 id;

	id = (u32) pci->vendorid << 16 | pci->deviceid;

	i = 0;
	while (driver->id[i] != PCIID_END) {
		if (id == driver->id[i]) {
			return true;
		}
		i++;
	}
	return false;
}

static void 
pci_load_driver (struct pci *pci)
{
	struct pci_driver_list *p;

	p = pci_driver_list_head;
	while (p) {
		if (pci_match (pci, p->driver)) {
			p->driver->new (pci);
		}
		p = p->next;
	}
}

static struct pci *
pci_new (u32 bus, u32 dev, u32 func)
{
	static struct pci pci, *p;
	enum bartype type;
	u32 id, i, j, bar;
	u64 addr;
	
	pci.bus = bus;
	pci.dev = dev;
	pci.func = func;
	id = pci_config_read32 (&pci, 0);
	if (id == 0xffffffff)
		return NULL;
	p = malloc (sizeof *p);
	memset (p, 0, sizeof *p);
	p->bus = bus;
	p->dev = dev;
	p->func = func;
	p->deviceid = (id >> 16) & 0xffff;
	p->vendorid = id & 0xffff;
	p->revid = pci_config_read8  (p, 8);
	p->classcode = pci_config_read32 (p, 9);
	for (i = 0x10, j = 0; i <= 0x24; i += 4, j ++) {
		bar = pci_config_read32 (p, i);
		type = (bar & 1) ? 
			bartype_io :
			bartype_mem;
		if (!bar) {
			p->bar[j].e = false;
			continue;
		}
		if (type == bartype_io) {
			addr = bar & 0xfffc;
		} else {
			addr = bar & 0xfffffff0;
		}
		if ((bar & 6) == 4) {
			bar = pci_config_read32 (p, i + 4);
			addr |= (u64) bar << 32;
			p->bar[j].e = true;
			p->bar[j].type = type;
			p->bar[j].addr = addr;
			i += 4;
			p->bar[j].e = false;
		} else {
			p->bar[j].e = true;
			p->bar[j].type = type;
			p->bar[j].addr = addr;
		}
	}

	return p;
}

static void 
pci_probe (void)
{
	int i;
	u32 bus, dev, func;
	struct pci *pci;

	i = 0;
	printi ("PCI: Probe {\n");
	for (bus = 0; bus < PCI_BUS_MAX; bus++) {
	for (dev = 0; dev < PCI_DEV_MAX; dev++) {
	for (func = 0; func < PCI_FUNC_MAX; func++) {
		pci = pci_new (bus, 
			       dev, 
			       func);
		if (!pci)
			continue;
		pci_register (pci);
		pci_load_driver (pci);

		i++;
		
		printi ("  %02x:%02x.%x "
			"%04x:%04x "
			"[%02x,%06x]\n", 
			pci->bus, 
			pci->dev, 
			pci->func, 
			pci->vendorid, 
			pci->deviceid,
			pci->revid, 
			pci->classcode);
	}
	}
	}
	printi ("}\n");

	printf ("PCI: %d devices detected.\n", i);
}

static void
pci_init (void)
{
	printd ("PCI: %s().\n", __func__);
	pci_probe ();
}

static void 
pci_deinit (void)
{
	printd ("PCI: %s().\n", __func__);
}

register_init ("driver99", pci_init);
register_deinit ("driver99", pci_deinit);
