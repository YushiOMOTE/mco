#include <common.h>
#include <boot.h>
#include <init.h>
#include <asm.h>

#define PAGE_POOL_BASE BOOT_HEAP

static __attribute__((used)) char *e820type_msg[] = {
	"",
	"Usable",
	"Reserved",
	"ACPI reclaim",
	"ACPI NVS",
	"Bad",
};

enum e820type {
	E820_USABLE = 1,
	E820_RESERVED,
	E820_ACPI_REC,
	E820_ACPI_NVS,
	E820_BAD,
};

struct e820entry {
	u32 entry_size;
	u64 addr;
	u64 size;
	u32 type;
} __attribute__((packed));

static u64 page_heap_base;
static void *page_heap_offset;

/* FIXME: Implement Buddy System. */
struct freelist {
	u64 addr;
	u64 size;
} page_freelist[32];
static int page_freelist_size;
static int page_freelist_index;

static void 
page_layout (void)
{
	struct e820entry *e;
	void *p, *q;
	int size;

	size = *(u32 *) BOOT_E820_SIZE;
	p = (void *) BOOT_E820_BASE;
	q = p + size;
	printi ("Memory: E820 Layout %p - %p {\n",
		p, p + size - 1);
	while (p < q) {
		e = p;
		if (e->type == E820_USABLE) {
			page_freelist[page_freelist_size].addr = e->addr;
			page_freelist[page_freelist_size].size = e->size;
			page_freelist_size ++;
		}
		if (e->type) {
			printi ("  %016llx - %016llx"
				" (%s)\n",
				e->addr, 
				e->addr + e->size - 1, 
				e->type > 5 ? "Unknown" : e820type_msg[e->type]);
		}
		p += e->entry_size + 4;
	}
	printi ("}\n");

	int i;
	printi ("Page: Free List {\n");
	for (i = 0; i < page_freelist_size; i++) {
		printi ("  %016llx - %016llx\n", 
			page_freelist[i].addr,
			page_freelist[i].addr + 
			page_freelist[i].size);
	}
	printi ("}\n");

	while ((u64)(page_heap_offset + page_heap_base)
	       >= 
	       page_freelist[page_freelist_index].addr +
	       page_freelist[page_freelist_index].size) {
		page_freelist_index++;
	}
	printd ("Page: Allocation. (HeapTop: %llx, In: %llx - %llx)\n",
		((u64)page_heap_offset + page_heap_base),
		page_freelist[page_freelist_index].addr,
		page_freelist[page_freelist_index].addr +
		page_freelist[page_freelist_index].size);
	if ((u64)(page_heap_offset + page_heap_base) <
	    page_freelist[page_freelist_index].addr) {
		printf ("Page: Wrong allocation. (HeapTop: %llx, Out: %llx - %llx)\n",
			((u64)page_heap_offset + page_heap_base),
			page_freelist[page_freelist_index].addr,
			page_freelist[page_freelist_index].addr +
			page_freelist[page_freelist_index].size);
	}
}

void *
page_alloc (int pages)
{
	void *p;

	p = page_heap_offset + page_heap_base;
	if (page_freelist[page_freelist_index].addr + 
	    page_freelist[page_freelist_index].size
	    < (u64) page_heap_offset + page_heap_base + 4096) {
		page_freelist_index ++;
		printd ("Page: Allocation. (HeapTop: %llx, In: %llx - %llx)\n",
			((u64)page_heap_offset + page_heap_base),
			page_freelist[page_freelist_index].addr,
			page_freelist[page_freelist_index].addr +
			page_freelist[page_freelist_index].size);
		if (page_freelist_index >= page_freelist_size) {
			panic ("Page: Out of free pages.\n");
		}
		page_heap_offset = (void *)
			page_freelist[page_freelist_index].addr
			- page_heap_base;
	}
	page_heap_offset += pages * 0x1000;

	return p;
}

int 
page_free (void *p, int pages)
{
	return 0;
}

static void 
page_alloc_init (void)
{
	page_heap_base = BOOT_BSP_HEAP_BASE;
	page_heap_offset = 0;
}

static void 
page_init (void)
{
	printd ("Memory: %s().\n", __func__);
	page_alloc_init ();
	page_layout ();
}

static void 
page_deinit (void)
{
	printd ("Memory: %s().\n", __func__);
}

register_init ("bsp0", page_init);
register_deinit ("bsp0", page_deinit);
