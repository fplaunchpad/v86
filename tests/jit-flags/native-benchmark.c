#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
static uint32_t a[1 << 20];
__attribute__((noinline)) static uint32_t mix(uint32_t x) {
    x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x;
}
int main(int argc, char **argv) {
    unsigned kind = argc > 1 ? atoi(argv[1]) : 0;
    uint32_t x = 123456789;
    if(kind == 0) {
        for(unsigned i = 0; i < 500000000; i++) {
            x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        }
    } else if(kind == 1) {
        for(unsigned i = 0; i < (1 << 20); i++) a[i] = i * 2654435761u;
        for(unsigned i = 0; i < 100000000; i++) {
            unsigned j = x & ((1 << 20) - 1);
            x = (x >> 3) ^ a[j]; a[j] += x;
        }
    } else {
        uint32_t (*volatile f)(uint32_t) = mix;
        for(unsigned i = 0; i < 100000000; i++) x = f(x);
    }
    printf("checksum=%u\n", x);
    return 0;
}
