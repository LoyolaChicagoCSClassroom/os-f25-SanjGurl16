
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

int x = 0;
int y = 0;
void print_char(char c) {
   struct termbuf *vram = (struct termbuf*)0xB8000;
   vram[x].ascii = c;
   vram[x].color = 7;
   x++;
}

void main() {
   print_char('s');
   print_char('a');
   print_char('n');
   print_char('j');
   print_char('a');
   print_char('n');
   print_char('a');
   while(1);
}
