#include <stdint.h>

uint8_t inb(uint16_t port) {
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outb(uint16_t _port, uint8_t val) {
   __asm__ __volatile__("outb %0, %1" : : "a" (val), "dN" (_port));
}
