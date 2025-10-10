#include "io.h"
#include "rprintf.h"

void putc(char c);

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
	    esp_printf((func_ptr)putc, "Scancode: %x\n", scancode);
	}
    }
}
