# Kernel Patches

## 0001-compiler-march-kernel-5.8+.patch 
[graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

## 0002-sched-core-2021-06-28.patch

Scheduler udpates:

- Changes to core scheduling facilities:

  - Add "Core Scheduling" via CONFIG_SCHED_CORE=y, which enables
    coordinated scheduling across SMT siblings. This is a much
    requested feature for cloud computing platforms, to allow
    the flexible utilization of SMT siblings, without exposing
    untrusted domains to information leaks & side channels, plus
    to ensure more deterministic computing performance on SMT
    systems used by heterogenous workloads.

    There's new prctls to set core scheduling groups, which
    allows more flexible management of workloads that can share
    siblings.

  - Fix task->state access anti-patterns that may result in missed
    wakeups and rename it to ->__state in the process to catch new
    abuses.

- Load-balancing changes:

  - Tweak newidle_balance for fair-sched, to improve
   'memcache'-like workloads.

  - "Age" (decay) average idle time, to better track & improve workloads
   such as 'tbench'.

  - Fix & improve energy-aware (EAS) balancing logic & metrics.

  - Fix & improve the uclamp metrics.

  - Fix task migration (taskset) corner case on !CONFIG_CPUSET.

  - Fix RT and deadline utilization tracking across policy changes

  - Introduce a "burstable" CFS controller via cgroups, which allows
   bursty CPU-bound workloads to borrow a bit against their future
   quota to improve overall latencies & batching. Can be tweaked
   via /sys/fs/cgroup/cpu/<X>/cpu.cfs_burst_us.

  - Rework assymetric topology/capacity detection & handling.

- Scheduler statistics & tooling:

  - Disable delayacct by default, but add a sysctl to enable
   it at runtime if tooling needs it. Use static keys and
   other optimizations to make it more palatable.

  - Use sched_clock() in delayacct, instead of ktime_get_ns().

- Misc cleanups and fixes.

## 0003-x86-fpu-2021-07-07.patch

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
