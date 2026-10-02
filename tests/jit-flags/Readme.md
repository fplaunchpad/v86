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

## Inline indirect-target lookup experiment

The perf/inline-indirect-lookup branch builds on the flag patch. It emits
the live TLB code-pointer, CPU-mode, module-index, and entry-state checks
directly in Wasm, avoiding a Rust helper call on every indirect jump. It
uses compiler-derived field offsets and retains the missing-target exit.

Eight warm Dune builds averaged 4.067 s with flags alone and 4.046 s with
inline lookup, which does not establish a Dune speedup. Matching native-only runs averaged 0.790 s for flags alone and 0.698 s
for inline lookup on the indirect-call fixture (about 13% faster). Integer
and memory means were unchanged. These are microbenchmark results and
require broader validation.

The final candidate passed Firefox flag, paging, multipage self-modifying
code, and precise-fault checks; two Rust unit tests with warnings denied;
and the full course workflow using current upstream JavaScript and the
existing course snapshot. Coverage completed at 14/19 (73.68%).

Build memory-check.c with gcc -O2 and run it in the 32-bit x86 guest. It
checks successful unaligned/cross-page reads and writes, protection faults,
and preservation of arithmetic flags at a fault. Expected output:
memory-check passed.

A separate short-circuit memory-check prototype regressed eight warm Dune
builds (4.610 s versus 4.067 s) and was rejected.

## Register-passing JIT experiment

The experiment/register-abi branch passes eight guest registers and the
accumulated instruction budget as Wasm function arguments between compiled
modules. Cached targets use bounded tail calls. Uncached code, faults,
self-modifying-code exits, halt, and budget exhaustion retain CPU-loop
writeback. This requires matching JavaScript and Wasm engines, including
the shared table import, plus browser support for Wasm tail calls.

The cross-page-benchmark.c fixture calls eight separate executable pages
20 million times. Its expected checksum is 213456789. Three Chromium runs
averaged 0.571 s for inline lookup alone and 0.355 s for register passing
(about 1.6x faster). This specifically measures cross-module calls.

The first eight warm Dune observations averaged 3.770 s and 3.611 s,
respectively. The apparent improvement is small and needs repeated paired
measurements. Integer, memory, and same-page indirect-call means were
essentially unchanged. This does not achieve the 10x VM or build goal.

The final candidate passed Chromium and Firefox flag, executable-page
aliasing, multipage self-modification, precise-fault, and cross-page-call
checks; two Rust unit tests with warnings denied; and the full course
workflow with the existing snapshot, ending at Coverage: 14/19 (73.68%).
The paired Node course runs completed in 158.1 s for the control and
152.1 s for register passing. An earlier attempt timed out and a retry
completed; the cause of that timeout is not established. Full emulator
conformance and wider browser/workload validation remain outstanding.

A repeated eight-build Chromium comparison did not reproduce that small
Dune gain: control mean 8.907 s, register-ABI mean 9.186 s. Both runs
were substantially slower than the earlier pair, so launch/environment
variability remains unresolved. No Dune improvement is established.
The cross-page fixture gain must not be generalized to build workloads.
