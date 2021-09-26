# Kernel Patches

Take care, patches only works with 5.13.3 version of kernel!

## 0001-compiler-march-kernel-5.8+.patch
[graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

## 0002-clang_cfi.patch

This series adds support for Clang's Control-Flow Integrity (CFI)
checking to x86_64. With CFI, the compiler injects a runtime
check before each indirect function call to ensure the target is
a valid function with the correct static type. This restricts
possible call targets and makes it more difficult for an attacker
to exploit bugs that allow the modification of stored function
pointers. For more details, see:

  https://clang.llvm.org/docs/ControlFlowIntegrity.html

Version 2 depends on Clang >=14, where we fixed the issue with
referencing static functions from inline assembly. Based on the
feedback for v1, this version also changes the declaration of
functions that are not callable from C to use an opaque type,
which stops the compiler from replacing references to them. This
avoids the need to sprinkle function_nocfi() macros in the kernel
code.

The first two patches contain objtool support for CFI, the
remaining patches change function declarations to use opaque
types, fix type mismatch issues that confuse the compiler, and
disable CFI where it can't be used.

You can also pull this series from

  https://github.com/samitolvanen/linux.git x86-cfi-v2

---
Changes in v2:

- Dropped the first objtool patch as the warnings were fixed in separate patches.

- Changed fix_cfi_relocs() in objtool to not rely on jump table symbols, and to return an error if it can't find a relocation.

- Fixed a build issue with ASM_STACK_FRAME_NON_STANDARD().

- Dropped workarounds for inline assembly references to address-taken static functions with CFI as this was fixed in the compiler.

- Changed the C declarations of non-callable functions to use opaque types and dropped the function_nocfi() patches.

- Changed ARCH_SUPPORTS_CFI_CLANG to depend on Clang >=14 for the compiler fixes.

## 0003-pgo.patch

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

## 0012-amd-acpi-fix-suspend-resume.patch

These add ACPI support to the PCI VMD driver, improve suspend-to-idle
support for AMD platforms and update documentation.

Specifics:

- Add ACPI support to the PCI VMD driver (Rafael Wysocki).

- Rearrange suspend-to-idle support code to reflect the platform
firmware expectations on some AMD platforms (Mario Limonciello).

- Make SSDT overlays documentation follow the code documented by it
more closely (Andy Shevchenko).

## 0013-KASLR.patch

This is a massive rework and a respin of Kristen Accardi's marvellous
FG-KASLR series (v5).

The major differences since v5 [0]:
- You can now tune the number of functions per each section to
  achieve the preferable vmlinux size or protection level. Default
  is still as one section per each function.
  This can be handy for storage-constrained systems. 4-8 fps are
  still strong, but reduce the size of the final vmlinu{x,z}
  significantly;
- I don't use orphan sections anymore. It's not reliable at all /
  may differ from linker to linker, and also conflicts with
  CONFIG_LD_ORPHAN_WARN which is great for catching random bugs ->
- All the .text.- sections are now being described explicitly in the
  linker script. A Perl script is used to take the original LDS, the
  original object file, read a list of input sections from it and
  generate the resulting LDS.
  This costs a bit of linking time as LD tends to think hard when
  processing scripts > 1 Mb. It adds about 40-60 seconds to the
  whole linking process (BTF step, 2-3 kallsyms steps and the final
  step), but "better safe than sorry".
  In addition, that approach allows to reserve some space at the end
  and add some link assertions ->
- Input .text section now must be empty, otherwise the linkage will
  be stopped. This is implemented by the size assertion in the
  resulting LD script and is designed to plug the potentional layout
  leakage. This also means that ->
- "Regular" ASM functions are now being placed into unique separate
  functions the same way compiler does this for C functions. This is
  achieved by introducing and using several new macros which take
  the symbol name as a base for its new section name.
  This gives a better opportunity to both DCE and FG-KASLR, as ASM
  code now can also be randomized or garbage-collected;
- It's now fully compatible with ClangLTO, ClangCFI,
  CONFIG_LD_ORPHAN_WARN and some more stuff landed since the last
  revision was published;
- Includes several fixes: relocations inside .altinstr_replacement
  code and minor issues found and/or suggested by LKP robot.

The series was compile-time and runtime tested on the following
setups with no issues:
- x86_64, GCC 11, Binutils 2.35;
- x86_64, Clang/LLVM 12, ClangLTO + ClangCFI (from Sami's tree).

The first 4 patches are from the linux-kbuild tree and included
to avoid merge conflicts and non-intuitive resolving of them.

The series is also available here: [1]

[0] https://lore.kernel.org/kernel-hardening/20200923173905.11219-1-kristen@linux.intel.com
[1] https://github.com/alobakin/linux/pull/3

The original v5 cover letter:

Function Granular Kernel Address Space Layout Randomization (fgkaslr)
---------------------------------------------------------------------

This patch set is an implementation of finer grained kernel address space
randomization. It rearranges your kernel code at load time 
on a per-function level granularity, with only around a second added to
boot time.

Changes in v5:
--------------

- fixed a bug in the code which increases boot heap size for
  CONFIG_FG_KASLR which prevented the boot heap from being increased
  for CONFIG_FG_KASLR when using bzip2 compression. Thanks to Andy Lavr
  for finding the problem and identifying the solution.
- changed the adjustment of the orc_unwind_ip table at boot time to
  disregard relocs associated with this table, and instead inspect the
  entries separately. Relocs are not able to be used since they are
  no longer correct once the table is resorted at buildtime.
- changed how orc_unwind_ip addresses in randomized sections are identified
  to include the byte immediately after the end of the section.
- updated module code to use kvmalloc/kvfree based on suggestions from
  Evgenii Shatokhin <eshatokhin@virtuozzo.com>.
- changed kernel commandline to disable fgkaslr to simply "nofgkaslr" to
  match the nokaslr option. fgkaslr="X" can be added at a later date
  if it is needed.
- Added a patch to force livepatch to require symbols to be unique if
  using while fgkaslr either for core or modules.

Changes in v4:
-------------

- dropped the patch to split out change to STATIC definition in
  x86/boot/compressed/misc.c and replaced with a patch authored
  by Kees Cook to avoid the duplicate malloc definitions
- Added a section to Documentation/admin-guide/kernel-parameters.txt
  to document the fgkaslr boot option.
- redesigned the patch to hide the new layout when reading
  /proc/kallsyms. The previous implementation utilized a dynamically
  allocated linked list to display the kernel and module symbols
  in alphabetical order. The new implementation uses a randomly
  shuffled index array to display the kernel and module symbols
  in a random order.

Changes in v3:
-------------

- Makefile changes to accommodate CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
- removal of extraneous ALIGN_PAGE from _etext changes
- changed variable names in x86/tools/relocs to be less confusing
- split out change to STATIC definition in x86/boot/compressed/misc.c
- Updates to Documentation to make it more clear what is preserved in .text
- much more detailed commit message for function granular KASLR patch
- minor tweaks and changes that make for more readable code
- this cover letter updated slightly to add additional details

Changes in v2:
--------------

- Fix to address i386 build failure
- Allow module reordering patch to be configured separately so that
  arm (or other non-x86_64 arches) can take advantage of module function
  reordering. This support has not be tested by me, but smoke tested by
  Ard Biesheuvel <ardb@kernel.org> on arm.
- Fix build issue when building on arm as reported by
  Ard Biesheuvel <ardb@kernel.org> 

Patches to objtool are included because they are dependencies for this
patchset, however they have been submitted by their maintainer separately.

Background
----------
KASLR was merged into the kernel with the objective of increasing the
difficulty of code reuse attacks. Code reuse attacks reused existing code
snippets to get around existing memory protections. They exploit software bugs
which expose addresses of useful code snippets to control the flow of
execution for their own nefarious purposes. KASLR moves the entire kernel
code text as a unit at boot time in order to make addresses less predictable.
The order of the code within the segment is unchanged - only the base address
is shifted. There are a few shortcomings to this algorithm.

1. Low Entropy - there are only so many locations the kernel can fit in. This
   means an attacker could guess without too much trouble.
2. Knowledge of a single address can reveal the offset of the base address,
   exposing all other locations for a published/known kernel image.
3. Info leaks abound.

Finer grained ASLR has been proposed as a way to make ASLR more resistant
to info leaks. It is not a new concept at all, and there are many variations
possible. Function reordering is an implementation of finer grained ASLR
which randomizes the layout of an address space on a function level
granularity. We use the term "fgkaslr" in this document to refer to the
technique of function reordering when used with KASLR, as well as finer grained
KASLR in general.

Proposed Improvement
--------------------
This patch set proposes adding function reordering on top of the existing
KASLR base address randomization. The over-arching objective is incremental
improvement over what we already have. It is designed to work in combination
with the existing solution. The implementation is really pretty simple, and
there are 2 main area where changes occur:

- Build time

GCC has had an option to place functions into individual .text sections for
many years now. This option can be used to implement function reordering at
load time. The final compiled vmlinux retains all the section headers, which
can be used to help find the address ranges of each function. Using this
information and an expanded table of relocation addresses, individual text
sections can be suffled immediately after decompression. Some data tables
inside the kernel that have assumptions about order require re-sorting
after being updated when applying relocations. In order to modify these tables,
a few key symbols are excluded from the objcopy symbol stripping process for
use after shuffling the text segments.

Some highlights from the build time changes to look for:

The top level kernel Makefile was modified to add the gcc flag if it
is supported. Currently, I am applying this flag to everything it is
possible to randomize. Anything that is written in C and not present in a
special input section is randomized. The final binary segment 0 retains a
consolidated .text section, as well as all the individual .text.- sections.
Future work could turn off this flags for selected files or even entire
subsystems, although obviously at the cost of security.

The relocs tool is updated to add relative relocations. This information
previously wasn't included because it wasn't necessary when moving the
entire .text segment as a unit. 

A new file was created to contain a list of symbols that objcopy should
keep. We use those symbols at load time as described below.

- Load time

The boot kernel was modified to parse the vmlinux elf file after
decompression to check for our interesting symbols that we kept, and to
look for any .text.- sections to randomize. The consolidated .text section
is skipped and not moved. The sections are shuffled randomly, and copied
into memory following the .text section in a new random order. The existing
code which updated relocation addresses was modified to account for
not just a fixed delta from the load address, but the offset that the function
section was moved to. This requires inspection of each address to see if
it was impacted by a randomization. We use a bsearch to make this less
horrible on performance. Any tables that need to be modified with new
addresses or resorted are updated using the symbol addresses parsed from the
elf symbol table.

In order to hide our new layout, symbols reported through /proc/kallsyms
will be displayed in a random order.

Security Considerations
-----------------------
The objective of this patch set is to improve a technology that is already
merged into the kernel (KASLR). This code will not prevent all attacks,
but should instead be considered as one of several tools that can be used.
In particular, this code is meant to make KASLR more effective in the presence
of info leaks.

How much entropy we are adding to the existing entropy of standard KASLR will
depend on a few variables. Firstly and most obviously, the number of functions
that are randomized matters. This implementation keeps the existing .text
section for code that cannot be randomized - for example, because it was
assembly code. The less sections to randomize, the less entropy. In addition,
due to alignment (16 bytes for x86_64), the number of bits in a address that
the attacker needs to guess is reduced, as the lower bits are identical.

Performance Impact
------------------
There are two areas where function reordering can impact performance: boot
time latency, and run time performance.

- Boot time latency
This implementation of finer grained KASLR impacts the boot time of the kernel
in several places. It requires additional parsing of the kernel ELF file to
obtain the section headers of the sections to be randomized. It calls the
random number generator for each section to be randomized to determine that
section's new memory location. It copies the decompressed kernel into a new
area of memory to avoid corruption when laying out the newly randomized
sections. It increases the number of relocations the kernel has to perform at
boot time vs. standard KASLR, and it also requires a lookup on each address
that needs to be relocated to see if it was in a randomized section and needs
to be adjusted by a new offset. Finally, it re-sorts a few data tables that
are required to be sorted by address.

Booting a test VM on a modern, well appointed system showed an increase in
latency of approximately 1 second.

- Run time
The performance impact at run-time of function reordering varies by workload.
Using kcbench, a kernel compilation benchmark, the performance of a kernel
build with finer grained KASLR was about 1% slower than a kernel with standard
KASLR. Analysis with perf showed a slightly higher percentage of 
L1-icache-load-misses. Other workloads were examined as well, with varied
results. Some workloads performed significantly worse under FGKASLR, while
others stayed the same or were mysteriously better. In general, it will
depend on the code flow whether or not finer grained KASLR will impact
your workload, and how the underlying code was designed. Because the layout
changes per boot, each time a system is rebooted the performance of a workload
may change.

Future work could identify hot areas that may not be randomized and either
leave them in the .text section or group them together into a single section
that may be randomized. If grouping things together helps, one other thing to
consider is that if we could identify text blobs that should be grouped together
to benefit a particular code flow, it could be interesting to explore
whether this security feature could be also be used as a performance
feature if you are interested in optimizing your kernel layout for a
particular workload at boot time. Optimizing function layout for a particular
workload has been researched and proven effective - for more information
read the Facebook paper "Optimizing Function Placement for Large-Scale
Data-Center Applications" (see references section below).

Image Size
----------
Adding additional section headers as a result of compiling with
-ffunction-sections will increase the size of the vmlinux ELF file.
With a standard distro config, the resulting vmlinux was increased by
about 3%. The compressed image is also increased due to the header files,
as well as the extra relocations that must be added. You can expect fgkaslr
to increase the size of the compressed image by about 15%.

Memory Usage
------------
fgkaslr increases the amount of heap that is required at boot time,
although this extra memory is released when the kernel has finished
decompression. As a result, it may not be appropriate to use this feature on
systems without much memory.

Building
--------
To enable fine grained KASLR, you need to have the following config options
set (including all the ones you would use to build normal KASLR)

CONFIG_FG_KASLR=y

In addition, fgkaslr is only supported for the X86_64 architecture.

Modules
-------
Modules are randomized similarly to the rest of the kernel by shuffling
the sections at load time prior to moving them into memory. The module must
also have been build with the -ffunction-sections compiler option.

Although fgkaslr for the kernel is only supported for the X86_64 architecture,
it is possible to use fgkaslr with modules on other architectures. To enable
this feature, select

CONFIG_MODULE_FG_KASLR=y

This option is selected automatically for X86_64 when CONFIG_FG_KASLR is set.

Disabling
---------
Disabling normal KASLR using the nokaslr command line option also disables
fgkaslr. It is also possible to disable fgkaslr separately by booting with
nofgkaslr on the commandline.
