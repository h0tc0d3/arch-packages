# Kernel Patches

Take care, patches only works with 5.13.3 version of kernel!

## 0001-compiler-march-kernel-5.8+.patch
[graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

## 0002-x86-fpu-2021-07-07.patch

Fixes and improvements for FPU handling on x86:

- Prevent sigaltstack out of bounds writes. The kernel unconditionally
writes the FPU state to the alternate stack without checking whether
the stack is large enough to accomodate it.

Check the alternate stack size before doing so and in case it's too
small force a SIGSEGV instead of silently corrupting user space data.

- MINSIGSTKZ and SIGSTKSZ are constants in signal.h and have never been
updated despite the fact that the FPU state which is stored on the
signal stack has grown over time which causes trouble in the field
when AVX512 is available on a CPU. The kernel does not expose the
minimum requirements for the alternate stack size depending on the
available and enabled CPU features.

ARM already added an aux vector AT_MINSIGSTKSZ for the same reason.
Add it to x86 as well

- A major cleanup of the x86 FPU code. The recent discoveries of XSTATE
related issues unearthed quite some inconsistencies, duplicated code
and other issues like lack of robustness.

The fine granular overhaul addresses this, makes the code more robust
and maintainable, which allows to integrate upcoming XSTATE related
features in sane ways.

This PR comes late so the changes could soak for a while in -next. The
changes have been extensively tested by various teams at Intel and the
marginal fallout has been addressed. Some test cases have been added
already, but there is a larger set of Intel internal tests coming up soon
which will allow to catch similar issues in the future.

## 0003-clang.patch

Add future optimizations for clang.

## 0004-pgo.patch

Build kernel with LLVM PGO optimization.

```bash
su
mount -t debugfs none /sys/kernel/debug
cp -a /sys/kernel/debug/pgo/profraw vmlinux.profraw
chown user:user vmlinux.profraw
exit
llvm-profdata merge --output=vmlinux.profdata vmlinux.profraw
make LLVM=1 KCFLAGS=-fprofile-use=vmlinux.profdata
```
