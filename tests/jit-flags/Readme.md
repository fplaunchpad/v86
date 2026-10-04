# Experimental dead arithmetic flags elimination

This fork elides flag bookkeeping for selected 32-bit register arithmetic
when a bounded scan proves that the flags are overwritten in the same
basic block. The scan stops at memory operations, prefixes, unknown
instructions, and block boundaries. Carry consumers and partial flag
writers retain their existing behavior.

The implementation is experimental. It has not passed the complete v86
conformance suite, and it is not a demonstrated general VM speedup.

## Reproduce the guest checks

Build and run in a 32-bit x86 Linux guest, or use gcc -m32 with the required
32-bit libraries on an x86 Linux host:

```sh
gcc -O2 test-flags.c -o test-flags
./test-flags
gcc -O2 native-benchmark.c -o native-benchmark
./native-benchmark 0
./native-benchmark 1
./native-benchmark 2
```

Expected flag checksum: 1955412202. The 16 cases cover 800,000 input/case
combinations, including observed flags, carry consumers, variable shifts,
16-bit prefixes, register moves, LEA, and MOVZX. Native benchmark checksums
are 189529361, 3011903488, and 611465325 respectively.

## Measurements

On Chromium 148.0.7778.96 using the offline OCaml course VM, upstream 7693c56d and
this patch were built with the same Rust 1.90.0 and LLVM 20 toolchains.
Three native repetitions were run sequentially for each engine. Unchanged
Dune builds compile both OCaml and the native executable launcher.

| Workload | Upstream | Candidate |
| --- | ---: | ---: |
| Integer loop, mean | 3.301 s | 0.800 s |
| Indirect-call loop, mean | 0.985 s | 0.862 s |
| Random memory loop, mean | 0.498 s | 0.479 s |
| Warm clean Dune build, two-run mean | 4.653 s | 4.539 s |

The arithmetic gain is about 4.1x. Dune results vary between runs and do
not establish a speedup. These numbers describe these workloads only.

The current-upstream candidate passed the flag and native checksums,
upstream executable-page aliasing/self-modification checks, and two Rust
unit tests with warnings denied. The earlier course-pinned implementation
also passed Firefox checks and the complete course build/test workflow.
That earlier validation does not replace testing current upstream.
