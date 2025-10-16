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
