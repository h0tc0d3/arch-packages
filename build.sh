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

PACKAGES=()
while IFS=$'\n' read -r line; do PACKAGES+=("${line}"); done <"${SOURCE_DIR:?}/packages.txt"

install-keys() {

  local keys=()
  local count=1
  local validpgpkeys=()
  local package=""

  for package in "${PACKAGES[@]}"; do
    if [[ "${package}" =~ ^\-.* || "${package}" =~ ^\+.* ]]; then
      package=${package:1:${#package}}
    fi
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
    if [[ "${package}" =~ ^\+.* ]]; then
      continue
    fi
    if [[ "${package}" =~ ^\-.* ]]; then
      package=${package:1:${#package}}
    fi
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
    if [[ "${package}" =~ ^\-.* || "${package}" =~ ^\+.* ]]; then
      package=${package:1:${#package}}
    fi
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

  for package in "${PACKAGES[@]}"; do

    if [[ "${package}" =~ ^\+.* ]]; then
      package=${package:1:${#package}}
    fi
    if [[ "${package}" =~ ^\-.* ]]; then
      continue
    fi

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
  # shellcheck disable=SC2046
  yes | sudo pacman -S zlib llvm llvm-libs clang compiler-rt libclc $(pacman -Qqs "\+clang" | sed -e 's/\+clang//')
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
