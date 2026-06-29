#include <stdio.h>
#include <stdint.h>
#include <string.h>

void copy_input(const char *src, int len) {
    char buf[64];
    if (len > 64) {
        return;
    }
    memcpy(buf, src, len);
    buf[63] = '\0';
}

uint8_t square_mod256(uint8_t x) {
    return x * x;
}

int main(void) {

    printf("Loesungen von x^2 = 33 (mod 256), unsigned 8-bit:\n");
    for (int x = 0; x < 256; x++) {
        if (square_mod256((uint8_t)x) == 33) {
            printf("  x = %d\n", x);
        }
    }
    return 0;
}
