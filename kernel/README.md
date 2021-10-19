# Kernel Patches

Take care, patches only works with 5.13.3 version of kernel!

## 0001-compiler-march-kernel-5.8+.patch
[graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

## 0002-pgo.patch

Add PGO optimizations with clang.

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

## 0006-amd-cppc.patch

We would like to introduce a new AMD CPU frequency control mechanism as the
"amd-pstate" driver for modern AMD Zen based CPU series in Linux Kernel.
The new mechanism is based on Collaborative processor performance control
(CPPC) which is finer grain frequency management than legacy ACPI hardware
P-States. Current AMD CPU platforms are using the ACPI P-states driver to
manage CPU frequency and clocks with switching only in 3 P-states. AMD
P-States is to replace the ACPI P-states controls, allows a flexible,
low-latency interface for the Linux kernel to directly communicate the
performance hints to hardware.

"amd-pstate" leverages the Linux kernel governors such as *schedutil*,
*ondemand*, etc. to manage the performance hints which are provided by CPPC
hardware functionality. The first version for amd-pstate is to support one
of the Zen3 processors, and we will support more in future after we verify
the hardware and SBIOS functionalities.

There are two types of hardware implementations for amd-pstate: one is full
MSR support and another is shared memory support. It can use
X86_FEATURE_AMD_CPPC_EXT feature flag to distinguish the different types. 

Using the new AMD P-States method + kernel governors (*schedutil*,
*ondemand*, ...) to manage the frequency update is the most appropriate
bridge between AMD Zen based hardware processor and Linux kernel, the
processor is able to ajust to the most efficiency frequency according to
the kernel scheduler loading.

According to above average data, we can see this solution has shown better
performance per watt scaling on mobile CPU benchmarks in most of cases.

These patch series depends on a "hotplug capable" CPU fix below (Only few
of CPU parts with "un-hotplug" core will encounter the issue and Mario is
working on the fix):
https://lore.kernel.org/linux-pm/20210813161842.222414-1-mario.limonciello@amd.com/

And we can see patch series in below git repo:
V1: https://git.kernel.org/pub/scm/linux/kernel/git/rui/linux.git/log/?h=amd-pstate-dev-v1
V2: https://git.kernel.org/pub/scm/linux/kernel/git/rui/linux.git/log/?h=amd-pstate-dev-v2

For details introduction, please see the patch 19.

Changes from V1 -> V2:

cpufreq:
- Add detailed description in the commit log.
- Clean up the "extension" postfix in the x86 feature flag.
- Revise cppc_set_enable helper.
- Add a fix to check online cpus in cppc_acpi.
- Use static calls to avoid retpolines.
- Revise the comment style.
- Remove amd_pstate_boost_supported() function.
- Revise the return value in syfs attribute functions.

cpupower:
- Refine the commit log for cpupower patches.
- Expose a function to get the sysfs value from specific table.
- Move amd-pstate sysfs definitions and functions into amd helper file.
- Move the boost init function into amd helper file and explain the details in the commit log.
- Remove the amd_pstate_get_data in the lib/cpufreq.c to keep the lib as common operations.
- Move print_speed function into misc helper file.
- Add amd_pstate_show_perf_and_freq() function in amd helper for cpufreq-info print.

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

**--param=allow-store-data-races=0*- was renamed to **--allow-store-data-races**
in the GCC 10 release.

Also, base **RETPOLINE_CFLAGS*- and **RETPOLINE_VDSO_CFLAGS*- on **CONFIC_CC_IS_\***
then remove cc-option tests for Clang.

## 0010-AMD-CPU-C3-cache.patch

AMD CPU which support C3 shares cache. Its not necessary to flush the
caches in software before entering C3. This will cause performance drop
for the cores which share some caches. ARB_DIS is not used with current
AMD C state implementation. So set related flags correctly.

## 0011-amd-ptdma.patch

Add support for AMD PTDMA controller. It performs high-bandwidth
memory to memory and IO copy operation. Device commands are managed
via a circular queue of 'descriptors', each of which specifies source
and destination addresses for copying a single buffer of data.

**Device Drivers** ---> **DMA Engine support** ---> **<*> AMD PassThru DMA Engine**

## 0012-zstd.patch

Update to zstd-1.4.10

- The first commit adds a new kernel-style wrapper around zstd. This wrapper API
  is functionally equivalent to the subset of the current zstd API that is
  currently used. The wrapper API changes to be kernel style so that the symbols
  don't collide with zstd's symbols. The update to zstd-1.4.10 maintains the same
  API and preserves the semantics, so that none of the callers need to be
  updated. All callers are updated in the commit, because there are zero
  functional changes.
- The second commit adds an indirection for `lib/decompress_unzstd.c` so it
  doesn't depend on the layout of `lib/zstd/` to include every source file.
  This allows the next patch to be automatically generated.
- The third commit is automatically generated, and imports the zstd-1.4.10 source
  code. This commit is completely generated by automation.
- The fourth commit adds me (terrelln@fb.com) as the maintainer of `lib/zstd`.

The discussion around this patchset has been pretty long, so I've included a
FAQ-style summary of the history of the patchset, and why we are taking this
approach.

Why do we need to update?
-------------------------

The zstd version in the kernel is based off of zstd-1.3.1, which is was released
August 20, 2017. Since then zstd has seen many bug fixes and performance
improvements. And, importantly, upstream zstd is continuously fuzzed by OSS-Fuzz,
and bug fixes aren't backported to older versions. So the only way to sanely get
these fixes is to keep up to date with upstream zstd. There are no known security
issues that affect the kernel, but we need to be able to update in case there
are. And while there are no known security issues, there are relevant bug fixes.
For example the problem with large kernel decompression has been fixed upstream
for over 2 years https://lkml.org/lkml/2020/9/29/27.

Additionally the performance improvements for kernel use cases are significant.
Measured for x86_64 on my Intel i9-9900k @ 3.6 GHz:

- BtrFS zstd compression at levels 1 and 3 is 5% faster
- BtrFS zstd decompression+read is 15% faster
- SquashFS zstd decompression+read is 15% faster
- F2FS zstd compression+write at level 3 is 8% faster
- F2FS zstd decompression+read is 20% faster
- ZRAM decompression+read is 30% faster
- Kernel zstd decompression is 35% faster
- Initramfs zstd decompression+build is 5% faster

On top of this, there are significant performance improvements coming down the
line in the next zstd release, and the new automated update patch generation
will allow us to pull them easily.

How is the update patch generated?
----------------------------------

The first two patches are preparation for updating the zstd version. Then the
3rd patch in the series imports upstream zstd into the kernel. This patch is
automatically generated from upstream. A script makes the necessary changes and
imports it into the kernel. The changes are:

- Replace all libc dependencies with kernel replacements and rewrite includes.
- Remove unncessary portability macros like: #if defined(_MSC_VER).
- Use the kernel xxhash instead of bundling it.

This automation gets tested every commit by upstream's continuous integration.
When we cut a new zstd release, we will submit a patch to the kernel to update
the zstd version in the kernel.

The automated process makes it easy to keep the kernel version of zstd up to
date. The current zstd in the kernel shares the guts of the code, but has a lot
of API and minor changes to work in the kernel. This is because at the time
upstream zstd was not ready to be used in the kernel envrionment as-is. But,
since then upstream zstd has evolved to support being used in the kernel as-is.

Why are we updating in one big patch?
-------------------------------------

The 3rd patch in the series is very large. This is because it is restructuring
the code, so it both deletes the existing zstd, and re-adds the new structure.
Future updates will be directly proportional to the changes in upstream zstd
since the last import. They will admittidly be large, as zstd is an actively
developed project, and has hundreds of commits between every release. However,
there is no other great alternative.

One option ruled out is to replay every upstream zstd commit. This is not feasible for several reasons:

- There are over 3500 upstream commits since the zstd version in the kernel.
- The automation to automatically generate the kernel update was only added recently,
  so older commits cannot easily be imported.
- Not every upstream zstd commit builds.
- Only zstd releases are "supported", and individual commits may have bugs that were
  fixed before a release.

Another option to reduce the patch size would be to first reorganize to the new
file structure, and then apply the patch. However, the current kernel zstd is formatted
with clang-format to be more "kernel-like". But, the new method imports zstd as-is,
without additional formatting, to allow for closer correlation with upstream, and
easier debugging. So the patch wouldn't be any smaller.

It also doesn't make sense to import upstream zstd commit by commit going
forward. Upstream zstd doesn't support production use cases running of the
development branch. We have a lot of post-commit fuzzing that catches many bugs,
so indiviudal commits may be buggy, but fixed before a release. So going forward,
I intend to import every (important) zstd release into the Kernel.

So, while it isn't ideal, updating in one big patch is the only patch I see forward.

Who is responsible for this code?
---------------------------------

I am. This patchset adds me as the maintainer for zstd. Previously, there was no tree
for zstd patches. Because of that, there were several patches that either got ignored,
or took a long time to merge, since it wasn't clear which tree should pick them up.
I'm officially stepping up as maintainer, and setting up my tree as the path through
which zstd patches get merged. I'll make sure that patches to the kernel zstd get
ported upstream, so they aren't erased when the next version update happens.

How is this code tested?
------------------------

I tested every caller of zstd on x86_64 (BtrFS, ZRAM, SquashFS, F2FS, Kernel,
InitRAMFS). I also tested Kernel & InitRAMFS on i386 and aarch64. I checked both
performance and correctness.

Also, thanks to many people in the community who have tested these patches locally.
If you have tested the patches, please reply with a Tested-By so I can collect them
for the PR I will send to Linus.

Lastly, this code will bake in linux-next before being merged into v5.16.

Why update to zstd-1.4.10 when zstd-1.5.0 has been released?
------------------------------------------------------------

This patchset has been outstanding since 2020, and zstd-1.4.10 was the latest
release when it was created. Since the update patch is automatically generated
from upstream, I could generate it from zstd-1.5.0. However, there were some
large stack usage regressions in zstd-1.5.0, and are only fixed in the latest
development branch. And the latest development branch contains some new code that
needs to bake in the fuzzer before I would feel comfortable releasing to the
kernel.

Once this patchset has been merged, and we've released zstd-1.5.1, we can update
the kernel to zstd-1.5.1, and exercise the update process.

You may notice that zstd-1.4.10 doesn't exist upstream. This release is an
artifical release based off of zstd-1.4.9, with some fixes for the kernel
backported from the development branch. I will tag the zstd-1.4.10 release after
this patchset is merged, so the Linux Kernel is running a known version of zstd
that can be debugged upstream.

Why was a wrapper API added?
----------------------------

The first versions of this patchset migrated the kernel to the upstream zstd
API. It first added a shim API that supported the new upstream API with the old
code, then updated callers to use the new shim API, then transitioned to the
new code and deleted the shim API. However, Cristoph Hellwig suggested that we
transition to a kernel style API, and hide zstd's upstream API behind that.
This is because zstd's upstream API is supports many other use cases, and does
not follow the kernel style guide, while the kernel API is focused on the
kernel's use cases, and follows the kernel style guide.
