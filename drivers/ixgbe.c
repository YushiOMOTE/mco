#define __DEBUG_PRINT
#include <common.h>
#include <init.h>
#include <lock.h>
#include <drivers/pci.h>
#include <mmio.h>

/******** IXGBE configuration ********/
struct ixgbe_config {
	int bufsize;
	int num_tdesc;
	int num_rdesc;

	int use_flowcontrol;
	int use_jumboframe;
	int jumboframe_size;
};

static const struct ixgbe_config ixgbe_default_config = {
	.bufsize = 2048,
	.num_tdesc = 256,
	.num_rdesc = 256,
	.use_flowcontrol = 0,
	.use_jumboframe = 0,
	.jumboframe_size = 9000,
};

/******** Transmit descriptor ********/
struct ixgbe_tdesc {
	u64 bufaddr;		   /* buffer address */

	/* 0 */
	u32 len      : 16; /* length per segment */
	/* 16 */
	u32 cso      : 8; /* checksum offset */
	/* 24 */
	u32 cmd_eop  : 1; /* command: end of packet */
	u32 cmd_ifcs : 1; /* command: insert FCS */
	u32 cmd_ic   : 1; /* command: insert checksum */
	u32 cmd_rs   : 1; /* command: report status */
	u32 cmd_rsv0 : 1; /* command: reserved */
	u32 cmd_dext : 1; /* command: use advanced descriptor */
	u32 cmd_vle  : 1; /* command: VLAN packet enable */
	u32 cmd_rsv1 : 1; /* command: reserved */
	/* 32 */
	u32 sta_dd   : 1; /* status field: descriptor done */
	u32 sta_rsv  : 3;
	/* 36 */
	u32 rsv      : 4;  /* reserved */
	/* 40 */
	u32 css      : 8;  /* checksum start */
	/* 48 */
	u32 vlan     : 16; /* VLAN */
} __attribute__ ((packed));

/******** Receive descriptor ********/
struct ixgbe_rdesc {
	u64 bufaddr;		/* buffer address */
	/* 0 */
	u32 len : 16;		/* length */
	/* 16 */
	u32 checksum : 16;	/* packet checksum */
	/* 32 */
	u32 status_dd : 1;	/* status field: descriptor done */
	u32 status_eop : 1;	/* status field: end of packet */
	u32 status_rsv : 1;	/* status field: reserved */
	u32 status_vp : 1;	/* status field: packet is 802.1Q */
	u32 status_udpcs : 1;	/* status field: UDP checksum
					 * calculated */
	u32 status_tcpcs : 1;	/* status field: TCP checksum
					 * calculated */
	u32 status_ipcs : 1;	/* status field: IPv4 checksum
					 * calculated */
	u32 status_pif : 1;	/* status field: non unicast address */
	/* 40 */
	u32 err_rxe : 1;	/* errors field: mac error */
	u32 err_rsv : 5;	/* errors field: reserved */
	u32 err_tcpe : 1;	/* errors field: tcp/udp
					 * checksum error */
	u32 err_ipe : 1;	/* errors field: ip check sum error */
	/* 48 */
	u32 vlantag : 16;	/* VLAN Tag */
} __attribute__ ((packed));

#define IXGBE_NUM_QUEUE_PAIRS 64

struct ixgbe {
	u8 macaddr[6];

	spinlock_t lock;

	struct ixgbe_config config;

	bool xmit_enabled;
	bool recv_enabled;
	struct ixgbe_queue_pair {
		/* transmission */
		int num_tdesc;
		struct ixgbe_tdesc *tdesc_ring;
		void **tdesc_buf;
		/* reception */
		int num_rdesc;
		struct ixgbe_rdesc *rdesc_ring;
		void **rdesc_buf;
	} *queue_pair;

	/* mmio */
	u64 mmio_base;
};

static void
ixgbe_usleep (u32 usec)
{
	usleep (usec);
}

/* register offsets */
enum {
	IXGBE_REG_CTRL = 0x0,
	IXGBE_REG_STATUS = 0x8,
	IXGBE_REG_EIMC = 0x888,

	IXGBE_REG_RAL = 0xA200,
	IXGBE_REG_RAH = 0xA204,
	IXGBE_REG_MPSAR = 0xA600,

	IXGBE_REG_FCTTV = 0x3200,
	IXGBE_REG_FCRTL = 0x3220,
	IXGBE_REG_FCRTH = 0x3260,
	IXGBE_REG_FCRTV = 0x32A0,
	IXGBE_REG_FCCFG = 0x3D00,

	IXGBE_REG_MSCA = 0x425C,
	IXGBE_REG_LINKS = 0x42A4,
	IXGBE_REG_MSRWD = 0x4260,

	IXGBE_REG_EEC = 0x10010,
	IXGBE_REG_EEMNGCTL = 0x10110,

	IXGBE_REG_FCTRL = 0x5080,

	IXGBE_REG_DMATXCTL = 0x4A80,
	IXGBE_REG_TDBAL = 0x6000,
	IXGBE_REG_TDBAH = 0x6004,
	IXGBE_REG_TDLEN = 0x6008,
	IXGBE_REG_TDH = 0x6010,
	IXGBE_REG_TDT = 0x6018,
	IXGBE_REG_TXDCTL = 0x6028,

	IXGBE_REG_RDBAL = 0x1000,
	IXGBE_REG_RDBAH = 0x1004,
	IXGBE_REG_RDLEN = 0x1008,
	IXGBE_REG_RDH = 0x1010,
	IXGBE_REG_RDT = 0x1018,
	IXGBE_REG_RXDCTL = 0x1028,
	IXGBE_REG_RXCTRL = 0x3000,

	IXGBE_REG_HLREG0 = 0x4240,
	IXGBE_REG_MAXFRS = 0x4268,

	IXGBE_REG_MRQC = 0xEC80,
};

enum {
	IXGBE_RET_OK = 0,
	IXGBE_RET_ERR = -1,

	IXGBE_RET_PHYOK = 0,
	IXGBE_RET_PHYTIMEOUT = -1,

	IXGBE_RET_XMTDIS = 1,
	IXGBE_RET_XMTFULL = 2,
	IXGBE_RET_XMTBIG = 4,

	IXGBE_RET_RCVDIS = 1,
	IXGBE_RET_RCVEOP = 2,
	IXGBE_RET_RCVIPE = 4,
	IXGBE_RET_RCVTCPE = 8,
	IXGBE_RET_RCVRXE = 16,
};

/******** MMIO read/write/check functions ********/
static inline u32
ixgbe_read32 (struct ixgbe *ixgbe, u32 offset)
{
	u32 value = 0;
	if (ixgbe->mmio_base)
		mmio_read32 (ixgbe->mmio_base + offset, &value);
	return value;
}

static inline void
ixgbe_write32 (struct ixgbe *ixgbe, u32 offset, u32 value)
{
	if (ixgbe->mmio_base)
		mmio_write32 (ixgbe->mmio_base + offset, value);
}

static inline int
ixgbe_check32 (struct ixgbe *ixgbe, u32 offset, u32 value)
{
	/* check if the bits in value is set */
	return ((ixgbe_read32 (ixgbe, offset) & value) == value);
}

/******** Transmission functions ********/
static int
ixgbe_send_frames (struct ixgbe *ixgbe, int num_frames, void **frames,
		   int *frame_sizes, int *error, int q)
{
	int i, errstate = 0;
	u32 head, tail, nt;
	struct ixgbe_tdesc *tdesc;
	struct ixgbe_queue_pair *qp;

	spinlock_lock (&ixgbe->lock);
	if (!ixgbe->xmit_enabled) {
		errstate |= IXGBE_RET_XMTDIS;
		goto end;
	}

	qp = &ixgbe->queue_pair[q];

	head = ixgbe_read32 (ixgbe, IXGBE_REG_TDH + 0x40 * q);
	tail = ixgbe_read32 (ixgbe, IXGBE_REG_TDT + 0x40 * q);

	if (head == 0xFFFFFFFF || tail == 0xFFFFFFFF) {
		errstate |= IXGBE_RET_XMTDIS;
		goto end;
	}

	for (i = 0; i < num_frames; i++) {
		nt = tail + 1;

		if (nt >= ixgbe->config.num_tdesc)
			nt = 0;

		if (head == nt) {
			errstate |= IXGBE_RET_XMTFULL;
			ixgbe_write32 (ixgbe, 
				       IXGBE_REG_TDT + 0x40 * q, 
				       tail);
			goto end;
		}

		if (frame_sizes[i] >= ixgbe->config.bufsize) {
			errstate |= IXGBE_RET_XMTBIG;
			continue;
		}

		memcpy (qp->tdesc_buf[tail], frames[i], frame_sizes[i]);
		tdesc = &qp->tdesc_ring[tail];
		tdesc->len = frame_sizes[i];
		tdesc->cmd_eop = 1;
		tdesc->cmd_ifcs = 1;

		tail = nt;
	}
	ixgbe_write32 (ixgbe, IXGBE_REG_TDT + 0x40 * q, tail);

end:
	spinlock_unlock (&ixgbe->lock);

	if (errstate) {
		if (error)
			*error = errstate;
		return IXGBE_RET_ERR;
	}

	return IXGBE_RET_OK;
}

static void
ixgbe_alloc_tdesc (struct ixgbe *ixgbe, int qnum)
{
	int i, q;
	int num_tdesc = ixgbe->config.num_tdesc;
	int bufsize = ixgbe->config.bufsize;
	int num_tdesc_ring_pages =
		(num_tdesc * sizeof (struct ixgbe_tdesc) - 1) / PAGESIZE + 1;
	int num_tdesc_buf_pages = (bufsize - 1) / PAGESIZE + 1;
	int tdesc_ring_size = num_tdesc * sizeof (struct ixgbe_tdesc);
	void *vaddr;
	struct ixgbe_queue_pair *qp;

	for (q = 0; q < qnum; q++) {
		printd ("Make xmit descriptor[%d] "
			"(num_tdesc: %d, "
			"tdesc_size: %ld, "
			"tdesc_bufsize: %d (%d pages), "
			"tdesc_ring_size: %d (%d pages))\n",
			q, num_tdesc,
			sizeof (struct ixgbe_tdesc),
			bufsize, num_tdesc_buf_pages,
			tdesc_ring_size, num_tdesc_ring_pages);
		qp = &ixgbe->queue_pair[q];
		qp->tdesc_ring = malloc (num_tdesc_ring_pages * PAGESIZE);
		memset (qp->tdesc_ring, 0, tdesc_ring_size);
		qp->tdesc_buf = malloc (num_tdesc * sizeof (void *));
		for (i = 0; i < num_tdesc; i++) {
			vaddr = malloc (num_tdesc_buf_pages * PAGESIZE);
			qp->tdesc_buf[i] = vaddr;
			qp->tdesc_ring[i].bufaddr = (u64)vaddr;
		}
	}
}

static void
ixgbe_write_tdesc (struct ixgbe *ixgbe, int qnum)
{
	int q;
	int num_tdesc = ixgbe->config.num_tdesc;
	int tdesc_ring_size = num_tdesc * sizeof (struct ixgbe_tdesc);
	struct ixgbe_queue_pair *qp;

	for (q = 0; q < qnum; q++) {
		qp = &ixgbe->queue_pair[q];

		printd ("Writing xmit descriptor ring[%d]... "
			"(size: %d, header: 0, tail: 0, phys: %llx)\n",
			q, tdesc_ring_size, (u64)qp->tdesc_ring);
		
		/* write base address */
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_TDBAL + 0x40 * q, 
			       (u32)((u64)qp->tdesc_ring & 0xffffffff));
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_TDBAH + 0x40 * q, 
			       (u32)((u64)qp->tdesc_ring >> 32));
		
		/* write header/tail */
		ixgbe_write32 (ixgbe, IXGBE_REG_TDT + 0x40 * q, 0);
		ixgbe_write32 (ixgbe, IXGBE_REG_TDH + 0x40 * q, 0);
		
		/* write size of descriptor ring */
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_TDLEN + 0x40 * q, 
			       tdesc_ring_size);
	}
}

static int
ixgbe_setup_xmit (struct ixgbe *ixgbe, int qnum)
{
	int q;
	int timeout;
	u32 regvalue;

	printd ("Setting up transmission...\n");
	regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_HLREG0);
	ixgbe_write32 (ixgbe, IXGBE_REG_HLREG0, regvalue & ~0x4);

	/* setup descriptors */
	ixgbe_alloc_tdesc (ixgbe, qnum);
	ixgbe_write_tdesc (ixgbe, qnum);

	/* transmit enable */
	regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_DMATXCTL);
	ixgbe_write32 (ixgbe, IXGBE_REG_DMATXCTL, regvalue | 0x1);
	for (q = 0; q < qnum; q++) {
		regvalue = ixgbe_read32 (ixgbe, 
					 IXGBE_REG_TXDCTL + 0x40 * q);
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_TXDCTL + 0x40 * q, 
			       regvalue | 1 << 25);
		timeout = 3 * 1000;
		while (1) {
			ixgbe_usleep (1000);
			if (timeout-- < 0) {
				printf ("failed to enable transmission\n");
				return IXGBE_RET_ERR;
			}
			if (ixgbe_check32 (ixgbe, 
					   IXGBE_REG_TXDCTL + 0x40 * q, 
					   1 << 25))
				break;
		}
	}
	printd ("Transmission enabled.\n");

	ixgbe->xmit_enabled = true;

	return IXGBE_RET_OK;
}

/******** Receive functions ********/
static int
ixgbe_recv_frames (struct ixgbe *ixgbe, int *error, int q)
{
	u32 head, tail, nt;
	void *frames[16];
	int frame_sizes[16];
	int i = 0, num = 16, errstate = 0;
	struct ixgbe_rdesc *rdesc;
	struct ixgbe_queue_pair *qp;

	spinlock_lock (&ixgbe->lock);
	if (!ixgbe->recv_enabled) {
		errstate |= IXGBE_RET_RCVDIS;
		goto end;
	}

	head = ixgbe_read32 (ixgbe, IXGBE_REG_RDH + 0x40 * q);
	tail = ixgbe_read32 (ixgbe, IXGBE_REG_RDT + 0x40 * q);
	if (head < 0 || head > ixgbe->config.num_rdesc ||
	    tail < 0 || tail > ixgbe->config.num_rdesc) {
		errstate |= IXGBE_RET_RCVDIS;
		goto end;
	}

	qp = &ixgbe->queue_pair[q];
	while (1) {
		nt = tail + 1;
		if (nt >= ixgbe->config.num_rdesc)
			nt = 0;
		if (head == nt || i == num) {
			printf ("IXGBE: Received %d packets. "
				"(%p, %p)\n",
				i, frames, frame_sizes);
			if (head == nt)
				break;
			i = 0;
		}
		tail = nt;
		rdesc = &qp->rdesc_ring[tail];
		frames[i] = qp->rdesc_buf[tail];
		frame_sizes[i] = rdesc->len;

		if (!rdesc->status_eop) {
			printf ("Status is not EOP\n");
			errstate |= IXGBE_RET_RCVEOP;
			continue;
		}
		if (rdesc->err_ipe) {
			printf ("Reception IP error\n");
			errstate |= IXGBE_RET_RCVIPE;
			continue;
		}
		if (rdesc->err_tcpe) {
			printf ("Reception TCP/UDP error\n");
			errstate |= IXGBE_RET_RCVTCPE;
			continue;
		}
		if (rdesc->err_rxe) {
			printf ("Reception RX data error\n");
			errstate |= IXGBE_RET_RCVRXE;
			continue;
		}
		i++;
	}
	ixgbe_write32 (ixgbe, IXGBE_REG_RDT + 0x40 * q, tail);

end:
	spinlock_unlock (&ixgbe->lock);

	if (errstate) {
		if (error)
			*error = errstate;
		return IXGBE_RET_ERR;
	}

	return IXGBE_RET_OK;
}

static void
ixgbe_alloc_rdesc (struct ixgbe *ixgbe, int qnum)
{
	int i, q;
	int num_rdesc = ixgbe->config.num_rdesc;
	int bufsize = ixgbe->config.bufsize;
	int num_rdesc_ring_pages =
		(num_rdesc * sizeof (struct ixgbe_rdesc) - 1) / PAGESIZE + 1;
	int num_rdesc_buf_pages = (bufsize - 1) / PAGESIZE + 1;
	int rdesc_ring_size = num_rdesc * sizeof (struct ixgbe_rdesc);
	void *vaddr;
	struct ixgbe_queue_pair *qp;

	for (q = 0; q < qnum; q++) {
		qp = &ixgbe->queue_pair[q];

		printd ("Make recv descriptor[%d] "
			"(num_rdesc: %d, "
			"rdesc_size: %ld, "
			"rdesc_bufsize: %d (%d pages), "
			"rdesc_ring_size: %d (%d pages))\n",
			q, num_rdesc,
			sizeof (struct ixgbe_rdesc),
			bufsize, num_rdesc_buf_pages,
			rdesc_ring_size, num_rdesc_ring_pages);

		qp->rdesc_ring = malloc (num_rdesc_ring_pages * PAGESIZE);
		memset (qp->rdesc_ring, 0, rdesc_ring_size);

		qp->rdesc_buf = malloc (num_rdesc * sizeof (void *));
		for (i = 0; i < num_rdesc; i++) {
			vaddr = malloc (num_rdesc_buf_pages * PAGESIZE);
			qp->rdesc_buf[i] = vaddr;
			qp->rdesc_ring[i].bufaddr = (u64)vaddr;
		}
	}
}

static void
ixgbe_write_rdesc (struct ixgbe *ixgbe, int qnum)
{
	int q;
	struct ixgbe_queue_pair *qp;
	int num_rdesc = ixgbe->config.num_rdesc;
	int rdesc_ring_size = num_rdesc * sizeof (struct ixgbe_rdesc);

	for (q = 0; q < qnum; q++) {
		qp = &ixgbe->queue_pair[q];

		printd ("Writing recv descriptor ring[%d]... "
			"(size: %d, header: 0, tail: 0, phys: %llx)\n",
			q, rdesc_ring_size, (u64)qp->rdesc_ring);

		/* write base address */
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_RDBAL + 0x40 * q, 
			       (u32)((u64)qp->rdesc_ring & 0xffffffff));
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_RDBAH + 0x40 * q, 
			       (u32)((u64)qp->rdesc_ring >> 32));
		
		/* write header/tail */
		ixgbe_write32 (ixgbe, IXGBE_REG_RDT + 0x40 * q, 0);
		ixgbe_write32 (ixgbe, IXGBE_REG_RDH + 0x40 * q, 0);
		
		/* write size of descriptor ring */
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_RDLEN + 0x40 * q, 
			       rdesc_ring_size);
	}
}

static int
ixgbe_setup_recv (struct ixgbe *ixgbe, int qnum)
{
	int q;
	int timeout;
	u32 regvalue;

	printd ("Setting up reception...\n");

	/* enable/disable jumboframe */
	if (ixgbe->config.use_jumboframe) {
		ixgbe_write32 (ixgbe, IXGBE_REG_MAXFRS,
			      ixgbe->config.jumboframe_size << 16);
		regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_HLREG0);
		ixgbe_write32 (ixgbe, IXGBE_REG_HLREG0, regvalue | 0x4);
	} else {
		regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_HLREG0);
		ixgbe_write32 (ixgbe, IXGBE_REG_HLREG0, regvalue & ~0x4);
	}

	/* setup descriptors */
	ixgbe_alloc_rdesc (ixgbe, qnum);
	ixgbe_write_rdesc (ixgbe, qnum);

	/* enable the reception queue */
	for (q = 0; q < qnum; q++) {
		regvalue = ixgbe_read32 (ixgbe, 
					 IXGBE_REG_RXDCTL + 0x40 * q);
		ixgbe_write32 (ixgbe, IXGBE_REG_RXDCTL + 0x40 * q, 
			       regvalue | 1 << 25);
		timeout = 3 * 1000;
		while (1) {
			ixgbe_usleep (1000);
			if (timeout-- < 0) {
				printf ("failed to enable reception\n");
				return IXGBE_RET_ERR;
			}
			if (ixgbe_check32 (ixgbe, 
					   IXGBE_REG_RXDCTL + 0x40 * q, 
					   1 << 25))
				break;
		}
		/* bump up the value of receive descriptor tail */
		ixgbe_write32 (ixgbe, 
			       IXGBE_REG_RDT + 0x40 * q, 
			       ixgbe->config.num_rdesc - 1);
	}

	/* enable L2 filters: accept broadcast */
	ixgbe_write32 (ixgbe, IXGBE_REG_FCTRL, 1<<10);

	/* start reception */
	regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_RXCTRL);
	ixgbe_write32 (ixgbe, IXGBE_REG_RXCTRL, regvalue | 1);

	printd ("Reception enabled.\n");

	ixgbe->recv_enabled = true;

	return IXGBE_RET_OK;
}

/******** Status functions ********/
static void 
ixgbe_associate_macaddr (struct ixgbe *ixgbe, 
			 bool enable, u64 assoc_table, int q)
{
	u32 rah = ixgbe_read32 (ixgbe, IXGBE_REG_RAH + q * 8);
	if (enable) {
		rah |= 1<<31;
	} else {
		rah &= ~(1<<31);
	}
	ixgbe_write32 (ixgbe, IXGBE_REG_RAH + q * 8, rah);
	ixgbe_write32 (ixgbe, IXGBE_REG_MPSAR + (2 * q) * 4, 
		       (u32)(assoc_table & 0xffffffff));
	ixgbe_write32 (ixgbe, IXGBE_REG_MPSAR + (2 * q + 1) * 4,
		       (u32)(assoc_table >> 32));
}

static void
ixgbe_get_macaddr (struct ixgbe *ixgbe, u8 *macaddr, int q)
{
	u32 ral = ixgbe_read32 (ixgbe, IXGBE_REG_RAL + q * 8);
	u32 rah = ixgbe_read32 (ixgbe, IXGBE_REG_RAH + q * 8);

	*(u32 *)macaddr = ral;
	*(u16 *)(macaddr + 4) = rah;
}

/********* Linkup functions *********/
static void 
ixgbe_setup_vmdq (struct ixgbe *ixgbe)
{
	u32 mrqc = ixgbe_read32 (ixgbe, IXGBE_REG_MRQC);

	mrqc &= ~0xf;
	mrqc |= 0x8;
}

static int
ixgbe_linkup (struct ixgbe *ixgbe)
{
	int qnum;
	int timeout;
	u32 regvalue;

	/* clear internal state values */
	ixgbe->xmit_enabled = false;
	ixgbe->recv_enabled = false;

	/* disable interrupt */
	ixgbe_write32 (ixgbe, IXGBE_REG_EIMC, 0xffffffff);

	/* wait for linkup */
	ixgbe_write32 (ixgbe, IXGBE_REG_CTRL, 0x00000004);
	timeout = 3 * 1000;
	while (1) {
		ixgbe_usleep (1000);
		if (timeout-- < 0) {
			printf ("Master cannot be disabled.\n");
			return IXGBE_RET_ERR;
		}
		if (!ixgbe_check32 (ixgbe, IXGBE_REG_STATUS, 0x00080000))
			break;
	}

	/* software&link reset */
	regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_CTRL);
	ixgbe_write32 (ixgbe, IXGBE_REG_CTRL, 0x04000008 | regvalue);
	timeout = 3 * 1000;
	while (1) {
		ixgbe_usleep (1000);
		if (timeout-- < 0) {
			printf ("Cannot complete software/link reset.\n");
			return IXGBE_RET_ERR;
		}
		/* wait until CTRL.RST is cleared and STATUS.MASTERE is set */
		if (!ixgbe_check32 (ixgbe, IXGBE_REG_CTRL, 0x04000000) &&
		    ixgbe_check32 (ixgbe, IXGBE_REG_STATUS, 0x00080000))
			break;
	}
	/* as mentioned in ixgbe datasheet,
	   we wait for 10msec after the completion of software reset */
	ixgbe_usleep (10 * 1000);

	/* disable interrupt, again */
	ixgbe_write32 (ixgbe, IXGBE_REG_EIMC, 0xffffffff);

	/* waiting for link up */
	printf ("IXGBE: Waiting for linkup...");
	timeout = 10 * 1000;
	while (1) {
		ixgbe_usleep (1000);

		if (timeout-- < 0) {
			printf ("timed-out.\n");
			return IXGBE_RET_ERR;
		} else if (timeout % 1000 == 0) {
			printf (".");
		}

		if (ixgbe_check32 (ixgbe, IXGBE_REG_LINKS, 1 << 7 | 1 << 30)) {
			printf ("linked-up! (LinkSpeed: ");

			regvalue = ixgbe_read32 (ixgbe, IXGBE_REG_LINKS);
			switch (regvalue >> 28 & 3) {
			case 1:
				printf ("100Mbps");
				break;
			case 2:
				printf ("1Gbps");
				break;
			case 3:
				printf ("10Gbps");
				break;
			default:
				printf ("(Unkown)");
				break;
			}
			printf (")\n");

			break;
		}
	}

	/* disable all flow control */
	ixgbe_write32 (ixgbe, IXGBE_REG_FCTTV, 0);
	ixgbe_write32 (ixgbe, IXGBE_REG_FCRTL, 0);
	ixgbe_write32 (ixgbe, IXGBE_REG_FCRTH, 0);
	ixgbe_write32 (ixgbe, IXGBE_REG_FCRTV, 0);
	ixgbe_write32 (ixgbe, IXGBE_REG_FCCFG, 0);

	/* vmdq setup */
	ixgbe_setup_vmdq (ixgbe);

	qnum = IXGBE_NUM_QUEUE_PAIRS;

	/* transmission setup */
	if (ixgbe_setup_xmit (ixgbe, qnum) != IXGBE_RET_OK) {
		printf ("Failed to setup transmission.\n");
		return IXGBE_RET_ERR;
	}

	/* reception setup */
	if (ixgbe_setup_recv (ixgbe, qnum) != IXGBE_RET_OK) {
		printf ("Failed to setup reception.\n");
		return IXGBE_RET_ERR;
	}

	return IXGBE_RET_OK;
}

/******** NIC functions ********/
static void 
ixgbe_getinfo_physnic (void *handle, void *info)
{
}

static void
ixgbe_send_physnic (void *handle, unsigned int num_packets, void **packets, 
		    unsigned int *packet_sizes)
{
	struct ixgbe *ixgbe = handle;

	ixgbe_send_frames (ixgbe, (int) num_packets, 
			   packets, (int *)packet_sizes, NULL, 0);
}

static void
ixgbe_poll_physnic (void *handle)
{
	struct ixgbe *ixgbe = handle;
	int error;

	ixgbe_recv_frames (ixgbe, &error, 0);
	if (error)
		;		/* TODO: Error check */
}

static void
ixgbe_new (struct pci *pci)
{
	int i;
	struct ixgbe *ixgbe;

	printf ("IXGBE: Initializing...\n");

	ixgbe = malloc (sizeof *ixgbe);
	if (!ixgbe) {
		printf ("failed to allocate ixgbe\n");
		return;
	}
	memset (ixgbe, 0, sizeof *ixgbe);
	ixgbe->queue_pair = malloc (IXGBE_NUM_QUEUE_PAIRS *
				    sizeof *ixgbe->queue_pair);
	ixgbe->config = ixgbe_default_config;
	ixgbe->mmio_base = pci->bar[0].addr;
	spinlock_init (&ixgbe->lock);

	pci->handle = ixgbe;

	ixgbe_get_macaddr (ixgbe, ixgbe->macaddr, 0);
	ixgbe_associate_macaddr (ixgbe, true, 
				 0xffffffffffffffffULL,
				 0);
	ixgbe_linkup (ixgbe);

	printf ("IXGBE: MAC Address[%d]: ", 0);
	for (i = 0; i < 6; i++) {
		printf ("%02X", ixgbe->macaddr[i]);
		if (i != 5)
			printf (":");
		else
			printf ("\n");
	}
}

static void
ixgbe_control (struct pci *pci, int cmd,
	       int arglen, void *arg)
{
	if (cmd == 0) {
		ixgbe_poll_physnic (pci->handle);
	} else if (cmd == 1) {
		ixgbe_send_physnic (pci->handle, 0, NULL, NULL);
	} else if (cmd == 2) {
		ixgbe_getinfo_physnic (pci->handle, NULL);
	} else {

	}
}

static u32 ixgbe_idlist[] = {
	PCIID(0x8086, 0x1528),
	PCIID_END,
};

static struct pci_driver ixgbe_driver = {
	.id = ixgbe_idlist,
	.new = ixgbe_new,
	.control = ixgbe_control,
};

static void 
ixgbe_init (void)
{
	printd ("IXGBE: %s().\n", __func__);

	pci_driver_register (&ixgbe_driver);
}

static void 
ixgbe_deinit (void)
{
	printd ("IXGBE: %s().\n", __func__);
}

register_init ("driver1", ixgbe_init);
register_deinit ("driver1", ixgbe_deinit);
