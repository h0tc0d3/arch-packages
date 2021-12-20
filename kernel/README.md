# Kernel Patches

Take care, patches only works with 5.14 version of kernel!

## 0001-compiler-march-kernel-5.8+.patch 0001-compiler-march-kernel-5.15+.patch
[graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

## 0002-pgo-5.14.patch and 0002-pgo-5.15+.patch

Add PGO optimizations with clang.

## 0003-amd-pstate-and-kernel-next.patch

For Kernel 5.15+
Add AMD P-States and next kernels AMD optimizations.

## 0003-amd-perf.patch

Add deep branch sampling for perf on AMD Fam19h.

## 0004-clang-next.patch

Add clang patches from kernel next.

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

## 0006-ELF.patch

Fix overflow in total mapping size calculation

Kernel assumes that ELF program headers are ordered by mapping address,
but doesn't enforce it.  It is possible to make mapping size extremely
huge by simply shuffling first and last PT_LOAD segments.

As long as PT_LOAD segments do not overlap, it is silly to require sorting
by v_addr anyway because mmap() doesn't care.

Don't assume PT_LOAD segments are sorted and calculate min and max
addresses correctly.

## 0007-string.patch

For Kernel 5.14+
Optimize performance of kernel code for working with strings.
