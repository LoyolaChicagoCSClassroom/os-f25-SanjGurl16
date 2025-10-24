#include <stdint.h>
#include "rprintf.h" // Provides esp_printf()
#include "page.h" // Page allocator

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

struct termbuf {
   char ascii;
   char color;
};

static int row = 0;
static int col = 0;
static const int COLS = 80;
static const int ROWS = 25;

//putc function writes a single char, handles newline and scrolling
void putc(int data) {
    struct termbuf *vram = (struct termbuf*)0xB8000;

    if (data == '\n') {
		col = 0;
		row++;
    } else {
		int index = row * COLS + col;
		vram[index].ascii = (char)data;
		vram[index].color = 9; //blue text
		col++;
		if (col >= COLS) {
	    	col = 0;
	    	row++;
		}
    }

    //Scrolling logic
    if (row >= ROWS) {
		//Scroll: Copy rows 1-24 up to rows 0-23
		for (int r = 1; r < ROWS; r++) {
	    	for (int c = 0; c < COLS; c++) {
			//Move row 'r' content to row 'r-1'
				vram[(r-1)*COLS + c] = vram[r*COLS + c];
	    	}
		}
		//Clear last row (row 24)
		for (int c = 0; c < COLS; c++) {
	    	vram[(ROWS-1)*COLS + c].ascii = ' ';
	    	vram[(ROWS-1)*COLS + c].color = 7; //gray text
		}

		//Reset cursor to the start of the last line
		row = ROWS - 1;
		col = 0;
    }
}

//Get current CPL
unsigned int get_cpl() {
    unsigned int cs;
    __asm__ __volatile__("mov %%cs, %0" : "=r"(cs));
    return cs & 0x3;
}

// ---- GLOBALS ----
// Ensure these match the bitfield definitions in your page.h
struct page_directory_entry page_directory[1024] __attribute__((aligned(4096)));
struct page page_table[1024] __attribute__((aligned(4096)));

// ---- FUNCTION DECLARATIONS ----
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd);

void loadPageDirectory(struct page_directory_entry *pd) {
    asm volatile("mov %0, %%cr3" : : "r"(pd) : "memory");
}

void enablePaging(void) {
    asm volatile(
	"mov %%cr0, %%eax\n"
	"or $0x80000001, %%eax\n"
	"mov %%eax, %%cr0\n"
	:
	:
	: "eax", "memory"
    );
}

// ---- IMPLEMENTATIONS ----
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd) {
    struct ppage *curr = pglist;
    uint32_t curr_vaddr = (uint32_t)vaddr;

    while (curr != NULL) {
    	uint32_t dir_index = (curr_vaddr >> 22) & 0x3FF;
		uint32_t tbl_index = (curr_vaddr >> 12) & 0x3FF;

		// If the page directory entry isn't present yet, initialize it
		if (!pd[dir_index].present) {
	    	pd[dir_index].frame = ((uint32_t)page_table) >> 12;
	    	pd[dir_index].present = 1;
	    	pd[dir_index].rw = 1; 
	    	pd[dir_index].user = 0;
	
	    	// Clear all page table entries
	    	for (int i = 0; i < 1024; i++) {
				page_table[i].present = 0;
				page_table[i].rw = 0;
				page_table[i].user = 0;
				page_table[i].accessed = 0;
				page_table[i].dirty = 0;
				page_table[i].unused = 0;
				page_table[i].frame = 0;
	    	}
	 	}

	 	struct page *pt = (struct page *)(((uint32_t)pd[dir_index].frame) << 12);

	 	// Map this page
	 	pt[tbl_index].frame = ((uint32_t)curr->physical_addr) >> 12;
	 	pt[tbl_index].present = 1;
	 	pt[tbl_index].rw = 1;
	 	pt[tbl_index].user = 0;
	 
	 	curr_vaddr += 0x1000;
	 	curr = curr->next;
   }

   return vaddr;
}

void main() {
    // Initial test prints
    esp_printf((func_ptr)putc, "Hello from kernel!\n");
    esp_printf((func_ptr)putc, "Current CPL = %d\n", get_cpl());

    // Initialize page allocator
    init_pfa_list();
    esp_printf((func_ptr)putc, "Initialized page frame allocator.\n");

    // Print total free pages before allocation
    int free_count = 0;
    struct ppage *curr = free_page_list;
    while (curr) {
       free_count++;
       curr = curr->next;
    }
    esp_printf((func_ptr)putc, "Free pages before allocation: %d\n", free_count);

    // Zero page directory entries first (clear any garbage)
    for (int i = 0; i < 1024; i++) {
		page_directory[i].present = 0;
		page_directory[i].rw = 0;
		page_directory[i].user = 0;
		page_directory[i].writethru = 0;
		page_directory[i].cachedisabled = 0;
		page_directory[i].accessed = 0;
		page_directory[i].pagesize = 0;
		page_directory[i].ignored = 0;
		page_directory[i].os_specific = 0;
		page_directory[i].frame = 0;
    }

    esp_printf((func_ptr)putc, "Preparing identity map...\n");

    // 1) Identity map kernel
    extern uint8_t _end_kernel; // linker symbol (end of kernel)
    uint32_t kernel_start = 0x100000;
    uint32_t kernel_end = (uint32_t)&_end_kernel;
    if (kernel_end < kernel_start) kernel_end = kernel_start; // safety

    uint32_t kernel_pages = (kernel_end - kernel_start + 0xFFF) / 0x1000;
    for (uint32_t i = 0; i < kernel_pages; i++) {
		struct ppage tmp;
		tmp.next = NULL;
		tmp.physical_addr = (void*)(kernel_start + i * 0x1000);
		map_pages((void*)(kernel_start + i * 0x1000), &tmp, page_directory);
    }
    esp_printf((func_ptr)putc, "Identity-mapped kernel: 0x%x - 0x%x (%d pages)\n",
	       kernel_start, kernel_end, (int)kernel_pages);

    // 2) Identity map stack: map a small number of pages around ESP
    uint32_t esp_val;
    asm("mov %%esp, %0" : "=r"(esp_val));
    uint32_t stack_top = esp_val & ~0xFFF; // align to page
    const int STACK_PAGES = 8; // change if you need more
    for (int p = 0; p < STACK_PAGES; p++) {
		uint32_t addr = stack_top - p * 0x1000;
		struct ppage tmp;
		tmp.next = NULL;
		tmp.physical_addr = (void*)addr;
		map_pages((void*)addr, &tmp, page_directory);
    }
    esp_printf((func_ptr)putc, "Identity-mapped %d stack pages around 0x%x\n", STACK_PAGES, stack_top);

    // 3) Identity map video buffer at 0xB8000
    struct ppage vtmp;
    vtmp.next = NULL;
    vtmp.physical_addr = (void*)0xB8000;
    map_pages((void*)0xB8000, &vtmp, page_directory);
    esp_printf((func_ptr)putc, "Identity-mapped video buffer at 0xB8000\n");

    // 4) Load CR3 and enable paging
    esp_printf((func_ptr)putc, "Loading page directory @ 0x%x\n", (unsigned int)page_directory);
    loadPageDirectory(page_directory);

    // Memory barrier: ensure CR3 write has taken effect before setting CR0
    asm volatile("mfence" ::: "memory");

    esp_printf((func_ptr)putc, "Enabling paging now...\n");
    enablePaging();
    esp_printf((func_ptr)putc, "Paging enabled.\n");

    // Allocate 3 pages
    struct ppage *pages = allocate_physical_pages(3);
    if (pages == 0) {
       esp_printf((func_ptr)putc, "Page allocation failed!\n");
    } else {
       curr = pages;
       int i = 1;
       while (curr) {
	  	  esp_printf((func_ptr)putc, "Allocated page %d at 0x%x\n", 
			  		  i, (unsigned int) (uintptr_t) curr->physical_addr);
	  	  curr = curr->next;
	  	  i++;
       }

       // Free pages
       free_physical_pages(pages);
       esp_printf((func_ptr)putc, "Freed 3 pages back to free list.\n");

       // Print total free pages after freeing
       free_count = 0;
       curr = free_page_list;
       while (curr) {
	      free_count++;
	      curr = curr->next;
       }
       esp_printf((func_ptr)putc, "Total free pages now: %d\n", free_count);
    }

    // Test scrolling
    /*
    for (int i = 0; i < 30; i++) {
	esp_printf((func_ptr)putc, "Line %d\n", i + 1);
    }
    */

    poll_keyboard();

    while(1); //Keep kernel running
}
