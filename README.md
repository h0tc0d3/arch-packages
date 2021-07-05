# Arch Linux performance important packages

We open official packages repository: **[Arch Linux Club](https://archlinux.club/)**

All packages optimized for build with LLVM and Clang.

Usage: `./build.sh --help`

```text
USAGE: build.sh [options]...

  --install, -i                 Build and install packages
  --uninstall, --revert, -u     Uninstall packages and revert system
  --install-keys, -k            Install GPG keys required for build
  --install-deps, -d            Install build dependencies
  --force, -f                   Force rebuild packages
  --check, -c                   Check packages versions
  --all, -a                     Build all packages(if not set only updates & not installed)
  --yes, -y                     Answer yes for build all packages
```

Before build packages please run `./build.sh --install-keys` and then `./build.sh --install-deps`!

Check packages versions from this repo and Arch Linux official: `./build.sh --check`.
It's helps to fast find changes of packages and faster update them in this repo.
On right side this repo package version and on left side arch linux package version.

```text
[+] zstd 1.5.0-1
[+] libpng 1.6.37-3
[+] libjpeg-turbo 2.1.0-1
[+] mesa 21.1.2-1
[+] pixman 0.40.0-1
[-] glib2 2.68.3-1 -> 2.68.2-1
[+] gtk2 2.24.33-2
[+] gtk3 1:3.24.29-2
[+] gtk4 1:4.2.1-2
[+] qt5-base 5.15.2+kde+r196-1
[+] icu 69.1-1
[+] freetype2 2.10.4-1
[+] pango 1:1.48.5-1
[+] fontconfig 2:2.13.93-4
[+] harfbuzz 2.8.1-1
[+] cairo 1.17.4-5
[+] wayland-protocols 1.21-1
[+] egl-wayland 1.1.7-1
[+] xorg-server 1.20.11-1
[+] xorgproto 2021.4-1
[+] xorg-xauth 1.1-2
[+] xorg-util-macros 1.19.3-1
[+] xorg-xkbcomp 1.4.5-1
[+] xorg-setxkbmap 1.3.2-2
[+] kwin 5.22.0-1
[+] plasma-workspace 5.22.0-2
[+] glibc 2.33-5
```

Before build packages install `pacman -Syu llvm llvm-libs clang lld libclc` and edit yours **/etc/makepkg.conf** and remove debug flags **-fvar-tracking-assignments**, add strings:

```bash
export CC=clang
export CXX=clang++
export LD=ld.lld
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
export HOSTLD=ld.lld
```

Yours  **/etc/makepkg.conf** can be like this:

```bash
CARCH="x86_64"
CHOST="x86_64-pc-linux-gnu"

CARCH="x86_64"
CHOST="x86_64-pc-linux-gnu"
#-- Compiler and Linker Flags
export CC=clang
export CXX=clang++
export LD=ld.lld
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
export HOSTLD=ld.lld

CPPFLAGS="-D_FORTIFY_SOURCE=2"
CFLAGS="-fdiagnostics-color=always -pipe -O2 -march=native -fstack-protector-strong"
CXXFLAGS="-fdiagnostics-color=always -pipe -O2 -march=native -fstack-protector-strong"
LDFLAGS="-Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now"
RUSTFLAGS="-C opt-level=2"
#-- Make Flags: change this for DistCC/SMP systems
MAKEFLAGS="-j$(nproc)"
NINJAFLAGS="-j$(nproc)"
#-- Debugging flags
DEBUG_CFLAGS="-g"
DEBUG_CXXFLAGS="-g"
#DEBUG_CFLAGS="-g -fvar-tracking-assignments"
#DEBUG_CXXFLAGS="-g -fvar-tracking-assignments"
#DEBUG_RUSTFLAGS="-C debuginfo=2"
```

Before build Mesa edit `mesa/mesa.conf` file and set dri, gallium, vulkan drivers for build.
More info you can find here [Mesa OpenGL](https://wiki.archlinux.org/title/OpenGL), [mesa-git package](https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=mesa-git), [Mesa Documentation](https://docs.mesa3d.org/systems.html)

```bash
DRI_DRIVERS="i915,i965,r200,r100,nouveau"
GALLIUM_DRIVERS="r300,r600,radeonsi,nouveau,svga,swrast,virgl,iris,zink"
VULKAN_DRIVERS="amd,intel,swrast"
```
