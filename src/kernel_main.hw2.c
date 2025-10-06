#include <stdint.h>
#include "rprintf.h"

// Inline assembly helpers
uint8_t inb(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outb(uint16_t _port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a" (val), "dN" (_port));
}

// Ports
#define PS2_DATA 0x60
#define PS2_STATUS 0x64

// Check PS/2 status register
int ps2_has_data() {
    return inb(PS2_STATUS) & 0x01; // check OS bit
}

// Polling loop
void poll_keyboard() {
    while (1) {
	if (ps2_has_data()) {
	    uint8_t scancode = inb(PS2_DATA);
	    esp_printf(putc, "Scancode: %x\n", scancode);
	}
    }
}
