# Arch Linux performance important packages

We open official packages repository: **[Arch Linux Club](https://www.archlinux.club/)**

All packages optimized for build with LLVM and Clang.

## SELinux

For SELinux install packages:
```bash
pacman -Syu \
    psmisc+clang findutils+clang iproute2+clang libsepol+clang libselinux+clang \
    libsemanage+clang checkpolicy+clang secilc+clang policycoreutils+clang \
    pambase+clang sudo+clang coreutils+clang shadow+clang openssh+clang \
    mcstrans+clang dbus-glib+clang restorecond+clang setools+clang \
    selinux-python+clang selinux-dbus+clang python-gobject+clang \
    selinux-gui+clang selinux-sandbox+clang
```

Take care, SELinux packages not include `logrotate`, `cronie`, `setroubleshoot` packages. `setroubleshoot` having bugs and not working on Arch Linux. `logrotate`, `cronie` you have to build with `--with-selinux` flag.

## SELinux reference policy installation

Edit `/etc/selinux/config` and replace `SELINUX=disabled` to `SELINUX=permissive`.

```bash
git clone https://github.com/SELinuxProject/refpolicy.git
cd refpolicy
su
make bare
```

Edit `build.conf` and replace `#DISTRO = redhat` to `DISTRO = arch`,
`SYSTEMD = n` to `SYSTEMD = y`.

```bash
make conf
make install
make load

reboot

su
restorecon -r /
```

## Compare package version from this repo and Arch Linux official

For compare use script: `./check`

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

Yours  **/etc/makepkg.conf** can be like this:

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
