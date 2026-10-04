#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

int main(void) {
    uint8_t *code = mmap(0, 8*4096, PROT_READ | PROT_WRITE | PROT_EXEC,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if(code == MAP_FAILED) return 1;
    uint32_t (*volatile functions[8])(uint32_t);
    for(unsigned i=0; i<8; i++) {
        /* mov eax,[esp+4]; add eax,imm32; ret */
        uint8_t body[] = {0x8b,0x44,0x24,0x04,0x05,0,0,0,0,0xc3};
        uint32_t increment = i+1;
        memcpy(body+5, &increment, sizeof increment);
        memcpy(code+i*4096, body, sizeof body);
        functions[i] = (uint32_t (*)(uint32_t))(code+i*4096);
    }
    uint32_t x=123456789;
    for(unsigned i=0; i<20000000; i++) x=functions[i&7](x);
    printf("cross-page-checksum=%u\n",x);
    return x == 213456789 ? 0 : 1;
}
