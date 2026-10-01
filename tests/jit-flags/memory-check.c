#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <setjmp.h>
#include <ucontext.h>

static sigjmp_buf escape;
static volatile unsigned faults, fault_flags;
static void fault(int sig, siginfo_t *info, void *context) {
    (void)sig; (void)info;
    ucontext_t *uc = context;
    fault_flags = uc->uc_mcontext.gregs[REG_EFL] & 0x8d5;
    faults++;
    siglongjmp(escape, 1);
}
static void require(int ok) {
    if(!ok) { puts("memory-check FAILED"); exit(1); }
}
int main(void) {
    uint8_t *p = mmap(0, 8192, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    require(p != MAP_FAILED);
    /* Include every alignment and successful cross-page access. */
    for(unsigned repeat = 0; repeat < 1000; repeat++) {
        for(unsigned offset = 4080; offset < 4112; offset++) {
            uint32_t value = repeat * 2654435761u + offset;
            __asm__ volatile("movl %1, (%0)" :: "r"(p+offset), "r"(value) : "memory");
            uint32_t got;
            __asm__ volatile("movl (%1), %0" : "=r"(got) : "r"(p+offset) : "memory");
            require(got == value);
            *(volatile uint16_t *)(p+offset) = value;
            require(*(volatile uint16_t *)(p+offset) == (uint16_t)value);
            *(volatile uint8_t *)(p+offset) = value;
            require(*(volatile uint8_t *)(p+offset) == (uint8_t)value);
        }
    }
    struct sigaction sa = { .sa_sigaction = fault, .sa_flags = SA_SIGINFO };
    sigemptyset(&sa.sa_mask);
    require(sigaction(SIGSEGV, &sa, 0) == 0);
    require(mprotect(p+4096, 4096, PROT_NONE) == 0);
    /* A fault must retain ADD's flags despite the later XOR. */
    for(unsigned i = 0; i < 1000; i++) {
        if(!sigsetjmp(escape, 1)) {
            __asm__ volatile("movl $-1, %%eax; addl $1, %%eax; movl (%%ecx), %%edx; xorl %%eax, %%eax"
                             :: "c"(p+4095) : "eax", "edx", "cc", "memory");
            require(0);
        }
        require(fault_flags == 0x55);
        if(!sigsetjmp(escape, 1)) {
            __asm__ volatile("movl $-1, %%eax; addl $1, %%eax; movl %%edx, (%%ecx); xorl %%eax, %%eax"
                             :: "c"(p+4095), "d"(42) : "eax", "cc", "memory");
            require(0);
        }
        require(fault_flags == 0x55);
    }
    require(faults == 2000);
    require(mprotect(p+4096, 4096, PROT_READ) == 0);
    if(!sigsetjmp(escape, 1)) {
        *(volatile uint32_t *)(p+4096) = 42;
        require(0);
    }
    require(faults == 2001);
    puts("memory-check passed");
    return 0;
}
