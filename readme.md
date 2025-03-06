# Koshka
This is a very WIP "recompiling" emulator for ARMv8-A. This means it'll try to
translate an entire binary to native (x86) code (as much as possible), rather
than being a simulator that interprets the ARM instructions as bytecode. There
are a few caveats that make it near-impossible to translate an entire binary
ahead of time (e.g. to a native executable), but I'm trying to get as close to
that as possible. If you're familiar with the CPU emulators in Cemu or RPCS3,
that design is my end goal.

## Design
My design has a few distinct layers/stages, each dependent on the last. This is
roughly based on my high-level knowledge of compilers and Cemu's own PowerPC
recompiler. Here are the rough steps handled by (or planned for) each stage:
IRGen (IR being "Intermediate Representation):
  - Convert ARM code to a tree of instructions, attached to their memory/register operands
  - Replace constant branches with references to the target instruction node

Shatter:
  - Split IR into a tree of functions, which we can treat like small
    independent programs.

RegAlloc:
  - In each function, replace ARM register operands with x86 ones, or stack
    operations when we run out.
  - Store metadata with the function about which registers map to what. If we
    think of the register mappings in each function like a coordinate system,
    this is like a transformation matrix to take us between coordinate systems.
    See the codegen section for details.

Codegen:
    - Emit x86 code for each operation of each function.
    - Across function calls the register allocation will change, so we need to
    generate code to adjust for it.

    In short:
    - Emit code to shuffle registers around and match the callee's mapping
    - Make function call
    - Emit code to shuffle registers around and match our mapping

    A simple mapping translation might look like, in terms of the instructions
    generated on the way in and out of each function:

    func1_mapping:
    r18 -> rax
    r3 -> rcx
    r8 -> rdx

    func2_mapping:
    r8 -> rax
    r3 -> rcx

    Before calling func2, we have to apply its mapping so it gets values in the
    expected places:
    - Push rax to stack, because it's in use but needed by the new mapping
    - Load rdx -> rax to get r8 where func2 expects it

    - Do nothing to rcx, the mapping matches

    Our mappings now match, call func2. Once it returns, we have to undo the
    mapping:

    - mov rax -> rdx to get r8 where we expect it
    - pop rax from stack
    - Our mapping is now back to normal, continue

    Some notes:

    - This might mess up offsets for arguments passed on the stack... so we
    might need to have some register storage at the base of the stack like Cemu.

    - When we hit a branch-to-register, use Cemu's strategy of emitting a call
    back to the translator. It'll look up the address and translate it. Then it
    overwrites the calling instruction (known via the return address) with a
    branch to the translated code. The emitted callback should be the same size
    every time (probably an absolute call) so we know where to return to.

## Useful Links
NRO & Assembly test files:
[test_files.zip](https://github.com/Torphedo/koshka/files/15326609/test_files.zip)

PDFs:
[Cortex A57 Technical Reference Manual.pdf](https://github.com/Torphedo/koshka/files/15326624/cortex_a57_mpcore_trm.pdf)     
[Tegra X1 Errata](https://github.com/Torphedo/koshka/files/15326629/TegraX1_Si_Errata_DA07134002v08.pdf)     
[Intel x86_64 Full Manual](https://github.com/Torphedo/koshka/files/15326632/x86_64_full.pdf)     
[Tegra X1 TRM Technical Reference Manual](https://cs.rit.edu/~aeh6923/files/Tegra_X1_TRM_DP07225001_v1.3p.pdf)     
[ARMv8-A Version A.k (Matches Switch)](https://cs.rit.edu/~aeh6923/files/ARMv8-A_A.k.pdf)     

[HOS memory map](https://switchbrew.org/wiki/Memory_layout)     
[ARMv8-M emulator](https://github.com/hlandau/memu/blob/master/emu2.cc)     
