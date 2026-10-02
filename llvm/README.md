# LLVM build script for Arch Linux

Install dependencies:

```bash
pacman -Syu base-devel llvm clang lld vim git \
  ninja cmake libffi libedit ncurses libxml2 \
  ocaml ocaml-ctypes ocaml-findlib python-setuptools \
  python-psutil python-sphinx python-recommonmark
```

Edit yours **/etc/makepkg.conf** and set num thread for build in **MAKEFLAGS** and **NINJAFLAGS**. It can speed up builds.
$(nproc) automatic detect maximum available threads.

```text
MAKEFLAGS="-j$(nproc)"
NINJAFLAGS="-j$(nproc)"
```

How to use this build script:
`git clone https://github.com/h0tc0d3/llvm-git.git && cd llvm-git && makepkg -cfi`

Build script ask you few questions:

- Build with clang and llvm toolchain? — Y or Enter if yes, or N if not. I recommend build with LLVM and Clang. For this need installed: `pacman -Syu base-devel llvm clang lld vim`
- Skip build tests? Answer Y or Enter. git version fail some tests and tests needs to skip. It will speed up the assembly.
- Skip build documentation? Y or Enter if yes, or N if not. If you not need llvm documentation then you are can skip. It will speed up the assembly a little.
- Skip build OCaml and Go bindings? Answer Y or Enter if yes, or N if not. Many people don't need them. You can skip it, it will speed up the assembly a little.

## Using LLVM and Clang as default system compiler

For build system packages with clang and llvm edit yours **/etc/makepkg.conf** and remove debug flags **-fvar-tracking-assignments**,
add strings:

```bash
export CC=clang
export CXX=clang++
export CC_LD=lld
export CXX_LD=lld
export AR=llvm-ar
export NM=llvm-nm
export STRIP=llvm-strip
export OBJCOPY=llvm-objcopy
export OBJDUMP=llvm-objdump
export READELF=llvm-readelf
export RANLIB=llvm-ranlib
export HOSTCC=clang
export HOSTCXX=clang++
export HOSTAR=llvm-ar
```

Yours  **/etc/makepkg.conf** can be like this, -march=native can be replaced with [your cpu](https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html):

```bash
CARCH="x86_64"
CHOST="x86_64-pc-linux-gnu"

export CC=clang
export CXX=clang++
export CC_LD=lld
export CXX_LD=lld
export AR=llvm-ar
export NM=llvm-nm
export STRIP=llvm-strip
export OBJCOPY=llvm-objcopy
export OBJDUMP=llvm-objdump
export READELF=llvm-readelf
export RANLIB=llvm-ranlib
export HOSTCC=clang
export HOSTCXX=clang++
export HOSTAR=llvm-ar

CPPFLAGS="-D_FORTIFY_SOURCE=2"
CFLAGS="-fdiagnostics-color=always -pipe -O2 -march=native -fstack-protector-strong --param ssp-buffer-size=4 -fstack-clash-protection"
CXXFLAGS="-fdiagnostics-color=always -pipe -O2 -march=native -fstack-protector-strong --param ssp-buffer-size=4 -fstack-clash-protection"
LDFLAGS="-Wl,-O1 -Wl,-z,now -Wl,-z,relro -Wl,--as-needed -Wl,--no-copy-dt-needed-entries -Wl,--sort-common -Wl,--hash-style=gnu"
RUSTFLAGS="-C opt-level=2"

MAKEFLAGS="-j$(nproc)"
NINJAFLAGS="-j$(nproc)"

DEBUG_CFLAGS="-g"
DEBUG_CXXFLAGS="-g"
DEBUG_RUSTFLAGS="-C debuginfo=2"
```

For build latest mesa you can use my [mesa build script](https://github.com/h0tc0d3/arch-packages/tree/master/mesa).
Edit `mesa/mesa.conf` file and set dri, gallium, vulkan drivers for build.
More info you can find here [Mesa OpenGL](https://wiki.archlinux.org/title/OpenGL), [mesa-git package](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=mesa-git), [Mesa Documentation](https://docs.mesa3d.org/systems.html). Before build mesa you are also need installed [libclc](https://aur.archlinux.org/packages/libclc12-git/).

You can use my performance important packages which optimized for LLVM [Arch Packages](https://github.com/h0tc0d3/arch-packages).

## Build Linux kernel with LLVm and Clang

For build kernel with LLVM read first [kernel.org llvm](https://www.kernel.org/doc/html/latest/kbuild/llvm.html).
For use LTO need to set **LLVM_IAS=1**. I recommend ThinLTO. **General architecture-dependent option** --> **Link Time Optimization (LTO)** and then **Clang ThinLTO (EXPERIMENTAL)**. Also, you can enable strong stack protection. Highly recommend! **General architecture-dependent options** put * against **Stack Protector buffer overflow detection** и **Strong Stack Protector**.

You can automate linux kernel build with my script: <https://github.com/h0tc0d3/kbuild>

<https://clang.llvm.org/docs/ThinLTO.html>
<http://blog.llvm.org/2016/06/thinlto-scalable-and-incremental-lto.html>
