#include <stdint.h>
#include <stdio.h>
#define TEST(name, code) \
__attribute__((noinline)) static uint32_t name(uint32_t a, uint32_t b, uint32_t c, uint32_t d) { \
 uint32_t flags; \
 __asm__ volatile(code "\n\tpushfl\n\tpopl %2" : "+a"(a), "+b"(b), "=S"(flags) : "c"(c), "d"(d) : "cc"); \
 return a ^ (b * 2654435761u) ^ (flags & 0x8d5); \
}
TEST(add_xor, "addl %%ecx, %%eax; xorl %%edx, %%ebx")
TEST(sub_add, "subl %%ecx, %%eax; addl %%edx, %%ebx")
TEST(and_or, "andl %%ecx, %%eax; orl %%edx, %%ebx")
TEST(xor_cmp, "xorl %%ecx, %%eax; cmpl %%edx, %%ebx")
TEST(shl_test, "shll $13, %%eax; testl %%edx, %%ebx")
TEST(shr_add, "shrl %%cl, %%eax; addl %%edx, %%ebx")
TEST(sar_xor, "sarl $17, %%eax; xorl %%edx, %%ebx")
TEST(carry_read, "addl %%ecx, %%eax; adcl %%edx, %%ebx")
TEST(carry_preserve, "addl %%ecx, %%eax; incl %%ebx")
TEST(flags_observed, "addl %%ecx, %%eax; movl %%eax, %%ebx")
TEST(prefix16, "addl %%ecx, %%eax; xorw %%dx, %%bx")
TEST(move_add, "addl %%ecx, %%eax; movl %%eax, %%ebx; xorl %%edx, %%ebx")
TEST(lea_add, "addl %%ecx, %%eax; leal 7(%%eax,%%ecx,4), %%ebx; xorl %%edx, %%ebx")
TEST(move_carry, "addl %%ecx, %%eax; movl %%eax, %%ebx; adcl %%edx, %%ebx")
TEST(movzx_add, "addl %%ecx, %%eax; movzbl %%al, %%ebx; xorl %%edx, %%ebx")
TEST(immediate, "addl $42, %%eax; subl $17, %%ebx")
static uint32_t random32(uint32_t x) { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; }
int main(void) {
 uint32_t (*fs[])(uint32_t,uint32_t,uint32_t,uint32_t) = {add_xor, sub_add, and_or, xor_cmp, shl_test, shr_add, sar_xor, carry_read, carry_preserve, flags_observed, prefix16, immediate, move_add, lea_add, move_carry, movzx_add};
 uint32_t sum=0, x=123456789;
 for(unsigned j=0;j<sizeof(fs)/sizeof(fs[0]);j++) for(unsigned i=0;i<50000;i++) {
  uint32_t a=x; x=random32(x); uint32_t b=x; x=random32(x); uint32_t c=x; x=random32(x);
  sum = random32(sum) ^ fs[j](a,b,c,x);
 }
 printf("flags-checksum=%u\n",sum);
 return 0;
}
