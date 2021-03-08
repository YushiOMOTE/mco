#ifndef __PCI_H
#define __PCI_H

#define PCIID(vendor,device) ((vendor)<<16|(device))
#define PCIID_END (0)

struct pci {
	u32 bus;
	u32 dev;
	u32 func;

	u16 vendorid;
	u16 deviceid;
	u8 revid;
	u32 classcode;
	struct bar {
		bool e;
		enum bartype {
			bartype_io = 0,
			bartype_mem,
		} type;
		u64 addr;
	} bar[6];
	void *handle;
};

struct pci_driver {
	u32 *id;
	void (*new) (struct pci *pci);
	void (*control) (struct pci *pci, int cmd, 
			 int arglen, void *arg);
};

u8 pci_config_read8 (struct pci *pci, u32 reg);
u16 pci_config_read16 (struct pci *pci, u32 reg);
u32 pci_config_read32 (struct pci *pci, u32 reg);
void pci_driver_register (struct pci_driver *driver);

#endif	/* __PCI_H */
