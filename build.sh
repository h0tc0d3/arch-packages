#!/usr/bin/bash
# Author: Grigory Vasilyev <echo "h0tc0d3(-*A*-)g-m*a-i-l(-d#t-)c#m" | sed -e 's/-//ig;s/*//ig;s/(A)/@/i;s/#/o/ig;s/(dot)/./i'>
# License: Apache License 2.0

SOURCE_DIR=${0%/*}
if [[ "${SOURCE_DIR}" == "." ]]; then
  SOURCE_DIR=${PWD}
fi

FORCE=0
ALL=0
ALL_YES=0

set -euo pipefail

IFS=$' '

PACKAGES=(
  zstd libpng libxml2 libjpeg-turbo mesa pixman glib2 gtk2 gtk3 gtk4
  qt5-base icu freetype2 pango fontconfig harfbuzz cairo libepoxy
  wayland-protocols egl-wayland libx11 xtrans libice libsm libxt
  libxrender xorg-server xorgproto libxv libxpm libxfixes libxtst
  xorg-xauth xorg-util-macros libxext libxres libxi libxmu libxaw libxkbfile
  libpciaccess xorg-font-util libxfont2 xorg-xkbcomp libxshmfence
  xorg-setxkbmap kwin plasma-workspace plasma-framework glibc libva
  xcb-proto libxcb xcb-util xcb-util-renderutil xcb-util-image libxdamage
  xcb-util-cursor xcb-util-keysyms xcb-util-wm libunwind libdrm libxxf86vm
  libffi libvdpau expat wayland lm_sensors libinput xf86-input-libinput
)

IPACKAGES=(
  zlib-ng zstd libxml2 libjpeg-turbo libpng mesa qt5-base icu
  freetype2 fontconfig pango harfbuzz xorg-util-macros libxkbfile libxext
  libxres libxi libpciaccess libdrm xorg-font-util libunwind
  libxfont2 xorg-xkbcomp xorg-setxkbmap xorg-xauth wayland-protocols
  egl-wayland xorgproto libxxf86vm libxfixes libxdamage libxv pixman cairo
  libx11 xtrans libice libsm libxt libxtst libxpm libxmu libxaw libxrender
  libepoxy libxshmfence libva libvdpau xorg-server kwin libffi expat wayland
  plasma-workspace plasma-framework glib2 gtk2 gtk3 libinput xf86-input-libinput gtk4
  xcb-proto libxcb xcb-util xcb-util-renderutil xcb-util-image
  xcb-util-cursor xcb-util-keysyms xcb-util-wm lm_sensors
)

install-keys() {

  local keys=()
  local count=1
  local validpgpkeys=()
  for package in "${IPACKAGES[@]}"; do

    # shellcheck disable=SC1090 disable=SC1091
    source "${SOURCE_DIR:?}/${package}/PKGBUILD"

    for gpgkey in "${validpgpkeys[@]}"; do

      if ! gpg --check-sigs "${gpgkey}" 2>/dev/null | grep -iq "pub"; then
        keys+=("${gpgkey}")
        ((count++))
      fi

    done

  done

  if [[ ${count} -gt 1 ]]; then
    echo "Import keys: gpg --recv-keys ${keys[*]}"
    # shellcheck disable=2086
    gpg --recv-keys ${keys[*]}
  fi

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

  local IDEPENDENCIES=('cmake' 'git')
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

build_package() {

  local package=${1}
  local MAKEPKG_FLAGS="-sci"
  if [[ ${FORCE} -eq 1 ]]; then
    MAKEPKG_FLAGS="-scfi"
  fi

  cd "${SOURCE_DIR}/${package}" || (
    echo -e "\E[1;31m[-] Can't cd to ${SOURCE_DIR}/${package} directory! \E[0m"
    exit 1
  )
  echo -e "\E[1;33m[+] Build ${package} package\E[0m"
  makepkg "${MAKEPKG_FLAGS}"

}

build_question() {

  local package=${1}

  while true; do

    echo -ne "\n\E[1;33mBuild ${package}? [Y/n] \E[0m"
    read -r yn

    case ${yn} in
    [Yy])
      build_package "${package}"
      break
      ;;
    [Nn])
      break
      ;;
    *) echo -e "\E[1;31mPlease answer Y or N! \E[0m" ;;
    esac

  done

}

build() {

  local yn=0
  local pkgname=""
  local pkginfo=""
  local iepoch=""
  local ipkgver=""
  local ipkgrel=""

  for package in "${IPACKAGES[@]}"; do

    if [[ ${ALL} -eq 1 ]]; then

      if [[ ${ALL_YES} -eq 1 ]]; then
        build_package "${package}"
      else
        build_question "${package}"
      fi

    else

      # shellcheck disable=SC1090 disable=SC1091
      source "${SOURCE_DIR:?}/${package}/PKGBUILD"

      if pacman -Qs "${pkgname[0]}" >/dev/null; then

        pkginfo="$(pacman -Qi "${pkgname[0]}" | grep -oE "[0-9]*:*[0-9a-zA-Z\._\+]+-[0-9]+" | head -n 1)"

        if [[ "${pkginfo}" == *":"* ]]; then

          iepoch="${pkginfo%:*}"
          ipkgver="$(echo "${pkginfo##*:}" | cut -f1 -d-)"
          ipkgrel="${pkginfo##*-}"

          if [[ "${iepoch}" == "${epoch:?}" && "${ipkgver}" == "${pkgver:?}" && "${ipkgrel}" == "${pkgrel:?}" ]]; then
            continue
          fi

        else

          ipkgver="${pkginfo%%-*}"
          ipkgrel="${pkginfo##*-}"

          if [[ "${ipkgver}" == "${pkgver:?}" && "${ipkgrel}" == "${pkgrel:?}" ]]; then
            continue
          fi

        fi

      fi

      if [[ ${ALL_YES} -eq 1 ]]; then
        build_package "${package}"
      else
        build_question "${package}"
      fi

    fi

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
    FORCE=1
    ;;
  -a | --all)
    shift
    ALL=1
    ;;
  -y | --yes)
    shift
    ALL_YES=1
    ;;
  -h | --help)
    echo -e "\nUSAGE: $(basename "$0") [options]...\n\n" \
      " --install, -i\t\t\tBuild and install packages\n" \
      " --uninstall, --revert, -u\tUninstall packages and revert system\n" \
      " --install-keys, -k\t\tInstall GPG keys required for build\n" \
      " --install-deps, -d\t\tInstall build dependencies\n" \
      " --force, -f\t\t\tForce rebuild packages\n" \
      " --check, -c\t\t\tCheck packages versions\n" \
      " --all, -a\t\t\tBuild all packages(if not set only updates & not installed)\n" \
      " --yes, -y\t\t\tAnswer yes for build all packages\n"

    exit 0
    ;;
  *) shift ;;
  esac
done

$COMMAND
exit 0
