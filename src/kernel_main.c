
#include <stdint.h>

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

void putc(int data) {
	struct termbuf *vram = (struct termbuf*)0xB8000;

	if (data == '\n') {
		col = 0;
		row++;
	} else {
		int index = row * COLS + col;
		vram[index].ascii = (char)data;
		vram[index].color = 9;
		col++;
		if (col >= COLS) {
			col = 0;
			row++;
		}
	}

	if (row >= ROWS) {
		for (int r = 1; r < ROWS; r++) {
			for (int c = 0; c < COLS; c++) {
				vram[(r-1)*COLS + c] = vram[r*COLS + c];
			}
		}
		for (int c = 0; c < COLS; c++) {
			vram[(ROWS-1)*COLS + c].ascii = ' ';
			vram[(ROWS-1)*COLS + c].color = 7;
		}
		row = ROWS - 1;
		col = 0;
	}
}

void main() {
   putc('s');
   putc('a');
   putc('n');
   putc('j');
   putc('a');
   putc('n');
   putc('a');
   putc('\n');
   putc('!');
   while(1);
}
