# Kernel Patches

`0001-compiler-march-kernel-5.8+.patch` - [graysky2/kernel_compiler_patch](https://github.com/graysky2/kernel_compiler_patch)

Allow choose processor optimization. **Processor type and features**  --->
  **Processor family** ---> **your processor**.

`0002-scheduler-nohz.patch` - [git.kernel.org - timers/nohz](https://git.kernel.org/pub/scm/linux/kernel/git/tip/tip.git/commit/?id=09fe880ed7a160ebbffb84a0a9096a075e314d2f)

Updates to the tick/nohz code in this cycle:

- Micro-optimize tick_nohz_full_cpu()

- Optimize idle exit tick restarts to be less eager

- Optimize tick_nohz_dep_set_task() to only wake up
a single CPU. This reduces IPIs and interruptions
on nohz_full CPUs.

- Optimize tick_nohz_dep_set_signal() in a similar
fashion.

- Skip IPIs in tick_nohz_kick_task() when trying
to kick a non-running task.

- Micro-optimize tick_nohz_task_switch() IRQ flags
handling to reduce context switching costs.

- Misc cleanups and fixes.
