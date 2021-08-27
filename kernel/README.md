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

## 0004-folio-mm.patch

For Kernel 5.14+

Memory folios is the work that can allow for better system performance like ~7% faster kernel builds as one beneficial metric. For those missing out on the earlier articles on Linux's memory folios, Wilcox sums it up as:
Managing memory in 4KiB pages is a serious overhead. Many benchmarks benefit from a larger "page size". As an example, an earlier iteration of this idea which used compound pages (and wasn't particularly tuned) got a 7% performance boost when compiling the kernel.

Using compound pages or THPs exposes a weakness of our type system. Functions are often unprepared for compound pages to be passed to them, and may only act on PAGE_SIZE chunks. Even functions which are aware of compound pages may expect a head page, and do the wrong thing if passed a tail page.

We also waste a lot of instructions ensuring that we're not looking at a tail page. Almost every call to PageFoo() contains one or more hidden calls to compound_head(). This also happens for get_page(), put_page() and many more functions.

This patch series uses a new type, the struct folio, to manage memory. It converts enough of the page cache, iomap and XFS to use folios instead of pages, and then adds support for multi-page folios. It passes xfstests (running on XFS) with no regressions compared to v5.14-rc1.

## 0005-bbr2.patch

For Kernel 5.14+
Add tcp_congestion_control - bbr2 <https://github.com/google/bbr> <https://groups.google.com/g/bbr-dev>
Fixed bbr1 bugs and for me works more stable than bbr1.
<https://datatracker.ietf.org/meeting/104/materials/slides-104-iccrg-an-update-on-bbr-00>

Qdisc can be fq, fq_codel, cake.

```sysctl
net.core.default_qdisc=fq_codel
net.ipv4.tcp_congestion_control=bbr2
```

```bash
tc qdisc add dev your_network_device root fq_codel
```

## 0006-compat.patch

For Kernel 5.14+
Simplify and remove uneeded compatable code.

## 0007-string.patch

For Kernel 5.14+
Optimize performance of kernel code for working with strings.

## 0008-remove-LightNVM.patch

For Kernel 5.14+

LightNVM was focused on features around predictable latency, I/O isolation, and better memory management. However, LightNVM has been effectively superseded by the Zoned Namespace (ZNS) command set with NVMe.

There hasn't been much happening upstream with LightNVM in the past two years due to the zoned storage support in NVMe succeeding it and needless to say not any real kernel activity as a result. Thus this relatively short-lived LightNVM code is now expected to be removed next kernel merge window with the subsystem now removed in the block subsystem's "for-next" branch.

## 0009-compiler-remove-stale-cc-option-checks.patch

cc-option, cc-option-yn, and cc-disable-warning all invoke the compiler
during build time, and can slow down the build when these checks become
stale for our supported compilers, whose minimally supported versions
increases over time.  See Documentation/process/changes.rst for the
current supported minimal versions (GCC 4.9+, clang 10.0.1+). Compiler
version support for these flags may be verified on godbolt.org.

The following flags are GCC only and supported since at least GCC 4.9.
Remove cc-option and cc-disable-warning tests.

- **-fno-tree-loop-im**
- **-Wno-maybe-uninitialized**
- **-fno-reorder-blocks**
- **-fno-ipa-cp-clone**
- **-fno-partial-inlining**
- **-femit-struct-debug-baseonly**
- **-fno-inline-functions-called-once**
- **-fconserve-stack**

The following flags are supported by all supported versions of GCC and
Clang. Remove their cc-option, cc-option-yn, and cc-disable-warning tests.

- **-fno-delete-null-pointer-checks**
- **-fno-var-tracking**
- **-mfentry**
- **-Wno-array-bounds**

The following configs are made dependent on GCC, since they use GCC
specific flags.
**READABLE_ASM**
**DEBUG_SECTION_MISMATCH**

**--param=allow-store-data-races=0** was renamed to **--allow-store-data-races**
in the GCC 10 release.

Also, base **RETPOLINE_CFLAGS** and **RETPOLINE_VDSO_CFLAGS** on **CONFIC_CC_IS_\***
then remove cc-option tests for Clang.

## 0010-AMD-CPU-C3-cache.patch

AMD CPU which support C3 shares cache. Its not necessary to flush the
caches in software before entering C3. This will cause performance drop
for the cores which share some caches. ARB_DIS is not used with current
AMD C state implementation. So set related flags correctly.

## 0011-pipe-FASYNC.patch

pipe: do FASYNC notifications for every pipe IO, not just state changes

It turns out that the SIGIO/FASYNC situation is almost exactly the same
as the EPOLLET case was: user space really wants to be notified after
every operation.

Now, in a perfect world it should be sufficient to only notify user
space on "state transitions" when the IO state changes (ie when a pipe
goes from unreadable to readable, or from unwritable to writable).  User
space should then do as much as possible - fully emptying the buffer or
what not - and we'll notify it again the next time the state changes.

But as with EPOLLET, we have at least one case (stress-ng) where the
kernel sent SIGIO due to the pipe being marked for asynchronous
notification, but the user space signal handler then didn't actually
necessarily read it all before returning (it read more than what was
written, but since there could be multiple writes, it could leave data
pending).

The user space code then expected to get another SIGIO for subsequent
writes - even though the pipe had been readable the whole time - and
would only then read more.

This is arguably a user space bug - and Colin King already fixed the
stress-ng code in question - but the kernel regression rules are clear:
it doesn't matter if kernel people think that user space did something
silly and wrong.  What matters is that it used to work.

So if user space depends on specific historical kernel behavior, it's a
regression when that behavior changes.  It's on us: we were silly to
have that non-optimal historical behavior, and our old kernel behavior
was what user space was tested against.

Because of how the FASYNC notification was tied to wakeup behavior, this
was first broken by commits f467a6a66419 and 1b6b26ae7053 ("pipe: fix
and clarify pipe read/write wakeup logic"), but at the time it seems
nobody noticed.  Probably because the stress-ng problem case ends up
being timing-dependent too.

It was then unwittingly fixed by commit 3a34b13a88ca ("pipe: make pipe
writes always wake up readers") only to be broken again when by commit
3b844826b6c6 ("pipe: avoid unnecessary EPOLLET wakeups under normal
loads").

And at that point the kernel test robot noticed the performance
refression in the stress-ng.sigio.ops_per_sec case.  So the "Fixes" tag
below is somewhat ad hoc, but it matches when the issue was noticed.

Fix it for good (knock wood) by simply making the kill_fasync() case
separate from the wakeup case.  FASYNC is quite rare, and we clearly
shouldn't even try to use the "avoid unnecessary wakeups" logic for it.

## 0012-drm-fbdev.patch

This patch series splits the fbdev core support in two different Kconfig
symbols: FB and FB_CORE. The motivation for this is to allow CONFIG_FB to
be disabled, while still using fbcon with the DRM fbdev emulation layer.

The reason for doing this is that now with simpledrm we could just boot
with simpledrm -> real DRM driver, without needing any legacy fbdev driver
(e.g: efifb or simplefb) even for the early console.

We want to do that in the Fedora kernel, but currently need to keep option
CONFIG_FB enabled and all fbdev drivers explicitly disabled, which makes
the configuration harder to maintain.

It is a RFC because I'm not that familiar with the fbdev core, but I have
tested and works with CONFIG_DRM_FBDEV_EMULATION=y and CONFIG_FB disabled.
This config automatically disables all the fbdev drivers that is our goal.

Patch 1/4 is just a clean up, patch 2/4 moves a couple of functions out of
fbsysfs.o, that are not related to sysfs attributes creation and finally
patch 3/4 makes the fbdev split that is mentioned above.

Patch 4/4 makes the DRM fbdev emulation depend on the new FB_CORE symbol
instead of FB. This could be done as a follow-up but for completeness is
also included in this series.
