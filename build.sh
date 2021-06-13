#!/usr/bin/bash
# Author: Grigory Vasilyev <echo "h0tc0d3(-*A*-)g-m*a-i-l(-d#t-)c#m" | sed -e 's/-//ig;s/*//ig;s/(A)/@/i;s/#/o/ig;s/(dot)/./i'>
# License: Apache License 2.0

SOURCE_DIR=${0%/*}
if [[ "${SOURCE_DIR}" == "." ]]; then
  SOURCE_DIR=${PWD}
fi

MAKEPKG_FLAGS="-ci"
set -euo pipefail

IFS=$' '

PACKAGES=(
  zstd libpng libjpeg-turbo mesa pixman glib2 gtk2 gtk3 gtk4
  qt5-base icu freetype2 pango fontconfig harfbuzz cairo
  wayland-protocols egl-wayland xorg-server xorgproto
  xorg-xauth xorg-util-macros xorg-xkbcomp xorg-setxkbmap
  kwin plasma-workspace glibc
)

IPACKAGES=(
  zlib-ng zstd libjpeg-turbo libpng mesa qt5-base icu
  freetype2 fontconfig pango harfbuzz xorg-util-macros
  xorg-xkbcomp xorg-setxkbmap xorg-xauth wayland-protocols
  egl-wayland xorgproto pixman cairo xorg-server kwin
  plasma-workspace glib2 gtk2 gtk3 gtk4
)

install-keys() {

  gpg --recv-keys '7273542B39962DF7B299931416792B4EA25340F8' # glibc Carlos O'Donell
  gpg --recv-keys 'BC7C7372637EC10C57D7AA6579C43DFBF1CF2187' # glibc Siddhesh Poyarekar

  gpg --recv-keys '923B7025EE03C1C59F42684CF0942E894B2EAFA0' # glib2 Philip Withnall (https://endlessos.org/) <pwithnall@endlessos.org>

  gpg --recv-keys '0338C8D8D9FDA62CF9C421BD7EC2DBB6F4DBF434' # The libjpeg-turbo Project (Signing key for official binaries) <information@libjpeg-turbo.org>

  gpg --recv-keys '4EF4AC63455FC9F4545D9B7DEF8FE99528B52FFD' # Zstd source signing keys

  gpg --recv-keys '8048643BA2C840F4F92A195FF54984BFA16C640F' # libpng Glenn Randers-Pehrson (mozilla) <glennrp@gmail.com>

  gpg --recv-keys '3BB639E56F861FA2E86505690FDD682D974CA72A' # xorg-util-macros, xorg-xkbcomp "Matt Turner <mattst88@gmail.com>"
  gpg --recv-keys '4A193C06D35E7C670FA4EF0BA2FB9E081F2D130E' # xorg-util-macros "Alan Coopersmith <alan.coopersmith@oracle.com>"

  gpg --recv-keys '3C2C43D9447D5938EF4551EBE23B7E70B467F0BF' # xorg-proto "Peter Hutterer (Who-T) <office@who-t.net>"

  gpg --recv-keys '3C2C43D9447D5938EF4551EBE23B7E70B467F0BF' # xorg-xkbcomp Peter Hutterer (Who-T) <office@who-t.net>
  gpg --recv-keys 'A66D805F7C9329B4C5D82767CCC4F07FAC641EFF' # xorg-xkbcomp, wayland-protocols "Daniel Stone <daniels@collabora.com>"
  gpg --recv-keys 'DD38563A8A8224537D1F90E45B8A2D50A0ECD0D3' # xorg-xkbcomp "Adam Jackson <ajax@benzedrine.nwnk.net>"

  gpg --recv-keys '8307C0A224BABDA1BABD0EB9A6EEEC9E0136164A' # wayland-protocols Jonas Ådahl

  gpg --recv-keys '7B27A3F1A6E18CD9588B4AE8310180050905E40C' # xorg
  gpg --recv-keys 'C383B778255613DFDB409D91DB221A6900000011' # xorg
  gpg --recv-keys 'DD38563A8A8224537D1F90E45B8A2D50A0ECD0D3' # xorg
  gpg --recv-keys '3BB639E56F861FA2E86505690FDD682D974CA72A' # xorg

  gpg --recv-keys '995ED5C8A6138EB0961F18474C09DD83CAAA50B2' # xorg-xauth "Adam Jackson <ajax@nwnk.net>"

  gpg --recv-keys '58E0C111E39F5408C5D3EC76C1A60EACE707FDA5' # freetype2 Werner Lemberg <wl@gnu.org>

  gpg --recv-keys 'BA90283A60D67BA0DD910A893932080F4FB419E3' # icu "Steven R. Loomis (filfla-signing) <srloomis@us.ibm.com>"
  gpg --recv-keys '9731166CD8E23A83BEE7C6D3ACA5DBE1FD8FABF1' # icu "Steven R. Loomis (ICU Project) <srl@icu-project.org>"
  gpg --recv-keys 'FFA9129A180D765B7A5BEA1C9B432B27D1BA20D7' # icu "Fredrik Roubert <fredrik@roubert.name>"
  gpg --recv-keys 'E4098B78AFC94394F3F49AA903996C7C83F12F11' # icu "keybase.io/srl295 <srl295@keybase.io>"
  gpg --recv-keys '4569BBC09DA846FC91CBD21CE1BBA44593CF2AE0' # icu "Steven R. Loomis (codesign-qormi) <srloomis@us.ibm.com>"
  gpg --recv-keys '0E51E7F06EF719FBD072782A5F56E5AFA63CCD33' # icu "Craig Cornelius (For use with ICU releases) <ccornelius@google.com>"

  gpg --recv-keys '2D1D5B0588357787DE9EE225EC94D18F7F05997E' # kwin Jonathan Riddell <jr@jriddell.org>
  gpg --recv-keys '0AAC775BB6437A8D9AF7A3ACFE0784117FBCE11D' # kwin Bhushan Shah <bshah@kde.org>
  gpg --recv-keys 'D07BD8662C56CB291B316EB2F5675605C74E02CF' # kwin David Edmundson <davidedmundson@kde.org>
  gpg --recv-keys '1FA881591C26B276D7A5518EEAAF29B42A678C20' # kwin Marco Martin <notmart@gmail.com>

  gpg --recv-keys '53E6B47B45CEA3E0D5B7457758D0EE648A48B3BB' # plasma-framework

}

check() {

  local pkginfo=""
  local iepoch=""
  local ipkgver=""
  local ipkgrel=""

  for package in "${PACKAGES[@]}"; do

    pkginfo="$(pacman -Si "${package}" | grep -oE "[0-9]*:*[0-9a-zA-Z\._\+]+-[0-9]+" | head -n 1)"
    # shellcheck disable=SC1090 disable=SC1091
    source "${SOURCE_DIR:?}/${package}/PKGBUILD"

    if [[ "${pkginfo}" == *":"* ]]; then

      iepoch="${pkginfo%:*}"
      ipkgver="$(echo "${pkginfo##*:}" | cut -f1 -d-)"
      ipkgrel="${pkginfo##*-}"

      if [[ "${iepoch}" == "${epoch:?}" && "${ipkgver}" == "${pkgver:?}" && "${ipkgrel}" == "${pkgrel:?}" ]]; then
        echo -e "\E[1;32m[+]\E[0m ${package} ${iepoch}:${ipkgver}-${ipkgrel}"
      else
        echo -e "\E[1;31m[-]\E[0m ${package} ${epoch}:${pkgver}-${pkgrel} \E[1;33m-->\E[0m ${iepoch}:${ipkgver}-${ipkgrel}"
      fi

    else

      ipkgver="${pkginfo%%-*}"
      ipkgrel="${pkginfo##*-}"

      if [[ "${ipkgver}" == "${pkgver:?}" && "${ipkgrel}" == "${pkgrel:?}" ]]; then
        echo -e "\E[1;32m[+]\E[0m ${package} ${ipkgver}-${ipkgrel}"
      else
        echo -e "\E[1;31m[-]\E[0m ${package} ${pkgver}-${pkgrel} \E[1;33m->\E[0m ${ipkgver}-${ipkgrel}"
      fi

    fi

  done

}

install-deps() {

  local IDEPENDENCIES=('cmake')
  local count=1
  for package in "${PACKAGES[@]}"; do
    # shellcheck disable=SC1090 disable=SC1091
    source "${SOURCE_DIR:?}/${package}/PKGBUILD"
    IDEPENDENCIES+=("${makedepends[@]}")
    IDEPENDENCIES+=("${depends[@]}")
  done

  local DEPENDENCIES=("${IDEPENDENCIES[@]}")
  IDEPENDENCIES=()
  while IFS=$'\n' read -r line; do IDEPENDENCIES+=("${line}"); done < <(echo "${DEPENDENCIES[*]}" | tr ' ' '\n' | sort -u)
  IFS=$' '

  DEPENDENCIES=()
  for package in "${IDEPENDENCIES[@]}"; do
    if [[ "${package:?}" != "mesa" && "${package:?}" != "mesa-libgl" ]]; then
      if ! pacman -Qs "${package:?}" >/dev/null; then
        DEPENDENCIES+=("${package:?}")
        ((count++))
      fi
    fi
  done

  if [[ ${count} -gt 1 ]]; then
    # shellcheck disable=SC2086
    sudo pacman -Syu ${DEPENDENCIES[*]}
  fi

}

build() {

  local yn=0
  for package in "${IPACKAGES[@]}"; do
    while true; do
      echo -ne "\n\E[1;33mBuild ${package}? [Y/n] \E[0m"
      read -r yn
      case ${yn} in
      [Yy])
        cd "${SOURCE_DIR}/${package}" || (
          echo -e "\E[1;31m[-] Can't cd to ${SOURCE_DIR}/${package} directory! \E[0m"
          exit 1
        )
        echo -e "\E[1;33m[+] Build ${package} package\E[0m"
        makepkg "${MAKEPKG_FLAGS}"
        break
        ;;
      [Nn])
        break
        ;;
      *) echo -e "\E[1;31mPlease answer Y or N! \E[0m" ;;
      esac
    done
  done

}

revert() {

  echo -e "\E[1;33m[+] Revert packages! \E[0m"
  echo -e "zlib ${PACKAGES[*]} xorg-server-xephyr xorg-server-xvfb xorg-server-xnest xorg-server-common xorg-server-devel"
  # shellcheck disable=SC2086
  sudo pacman -Syu zlib ${PACKAGES[*]} gtk3-docs gtk3-demos freetype2-docs freetype2-demos \
    qt5-xcb-private-headers pango-docs harfbuzz-icu xorg-server-xephyr xorg-server-xvfb xorg-server-xnest \
    xorg-server-common xorg-server-devel gtk4-docs gtk4-demos gtk-update-icon-cache plasma-wayland-session
  echo -e "\E[1;32mPlease open https://wiki.archlinux.org/title/Xorg and install your graphic driver! \E[0m"

}

COMMAND=build
for arg in "$@"; do
  case "${arg}" in
  -i | --install)
    shift
    COMMAND=build
    ;;
  -u | --uninstall | --revert)
    shift
    COMMAND=revert
    ;;
  -k | --install-keys)
    shift
    COMMAND=install-keys
    ;;
  -d | --install-deps)
    shift
    COMMAND=install-deps
    ;;
  -c | --check)
    shift
    COMMAND=check
    ;;
  -f | --force)
    shift
    MAKEPKG_FLAGS="-cfi"
    ;;
  -h | --help)
    echo -e "\nUSAGE: $(basename "$0") [options]...\n\n" \
      " --install, -i\t\t\tBuild and install packages\n" \
      " --uninstall, --revert, -u\tUninstall packages and revert system\n" \
      " --install-keys, -k\t\tInstall GPG keys required for build\n" \
      " --install-deps, -d\t\tInstall build dependencies\n" \
      " --force, -f\t\t\tForce rebuild packages\n" \
      " --check, -c\t\t\tCheck packages versions\n" \
    exit 0
    ;;
  *) shift ;;
  esac
done

$COMMAND
exit 0
