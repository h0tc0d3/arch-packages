#!/usr/bin/python
# SELinux ALPM hook
# Relabel installed files after install or update

import os
import re
import sys
import time
import subprocess

# Path to restorecon binary
restorecon = "/usr/bin/restorecon"

# The install hooks of packages create files which got labelled with the wrong SELinux user
# (e.g. sysadm_u instead of system_u). Relabel all these files too.
# Check the order of this list with: sed -n '/^GEN_DIRS=/,/^)/{/^ /p}' |LANG=C sort -c
RELABLE_DIRS = [
    # linux: 99-linux.hook
    '/boot/',
    # ca-certificates-utils: update-ca-trust.hook
    '/etc/ca-certificates/extracted/',
    # gconf: gconf-install.hook
    '/etc/gconf/',
    # glibc install: ldconfig -r .
    '/etc/ld.so.cache',
    # archlinux-keyring install: pacman-key --populate archlinux
    '/etc/pacman.d/gnupg/',
    # ca-certificates-utils: update-ca-trust.hook
    '/etc/ssl/certs/',
    # gnupg install: install units in /etc/systemd/user/sockets.target.wants
    '/etc/systemd/user/',
    # texlive-bin install: mktexlsr
    '/etc/texmf/ls-R',
    # systemd: udev-hwdb.hook
    '/etc/udev/hwdb.bin',
    # unbound: unbound-key.hook
    '/etc/unbound/trusted-key.key',
    # gdk-pixbuf2: gdk-pixbuf-query-loaders.hook
    '/usr/lib/gdk-pixbuf-*/*/loaders.cache',
    # ghc: ghc-register.hook, ghc: ghc-unregister.hook
    '/usr/lib/ghc-*/package.conf.d/',
    # glib2: gio-querymodules.hook
    '/usr/lib/gio/modules/',
    # graphviz install: dot -c
    '/usr/lib/graphviz/',
    # gtk2: gtk-query-immodules-2.0.hook
    '/usr/lib/gtk-2.0/',
    # gtk3: gtk-query-immodules-3.0.hook
    '/usr/lib/gtk-3.0/',
    # glibc install: locale-gen
    '/usr/lib/locale/locale-archive',
    # dkms: 70-dkms-install.hook
    '/usr/lib/modules/',
    # vlc: update-vlc-plugin-cache.hook
    '/usr/lib/vlc/plugins/plugins.dat',
    # lib32-gdk-pixbuf2 install: gdk-pixbuf-query-loaders-32 --update-cache
    '/usr/lib32/gdk-pixbuf-*/*/loaders.cache',
    # lib32-glib2: gio-querymodules-32.hook
    '/usr/lib32/gio/modules/',
    # lib32-gtk3: gtk-query-immodules-3.0-32.hook
    '/usr/lib32/gtk-3.0/',
    # mono install: cert-sync /etc/ssl/certs/ca-certificates.crt
    '/usr/share/.mono/certs',
    # desktop-file-utils: update-desktop-database.hook
    '/usr/share/applications/mimeinfo.cache',
    # ghc: ghc-rebuild-doc-index.hook
    '/usr/share/doc/ghc/html/libraries/',
    # xorg-mkfontdir: xorg-mkfontdir.hook
    '/usr/share/fonts/',
    # glib2: glib-compile-schemes.hook
    '/usr/share/glib-2.0/schemas/',
    # gtk-update-icon-cache: gtk-update-icon-cache.hook
    '/usr/share/icons/',
    # texinfo: texinfo-install.hook
    '/usr/share/info/dir',
    # keepass install
    '/usr/share/keepass/',
    # shared-mime-info: update-mime-database.hook
    '/usr/share/mime/',
    # texlive-bin: mktexlsr.hook
    '/usr/share/texmf*/ls-R',
    # vim-runtime: vimdoc.hook
    '/usr/share/vim/vimfiles/doc/tags',
    # fontconfig: fontconfig.hook
    '/var/cache/fontconfig/',
    # glibc install: ldconfig -r .
    '/var/cache/ldconfig/',
    # man-db timer
    '/var/cache/man/',
    # pacman
    '/var/cache/pacman/',
    # pacman
    '/var/lib/pacman/',
    # dkms: 70-dkms-install.hook
    '/var/lib/dkms/',
    # systemd install: journalctl --update-catalog
    '/var/lib/systemd/catalog/database',
    # texlive-bin: mktexlsr.hook
    '/var/lib/texmf/'
]


def main():

    try:

        # Current work directory
        cwd = os.getcwd()

        # Check hook is running in root '/' directory
        if cwd != '/':
            print((
                "Hook was not run in the root directory!\n"
                "Current working directory: {0}").format(cwd),
                file=sys.stderr
            )
            sys.exit(1)

        # Check SELinux status
        process = subprocess.run(
            ['sestatus'],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            universal_newlines=True
        )

        # Exit if SELinux disabled.
        if not re.search(r"^SELinux status:\s*enabled", process.stdout, re.IGNORECASE):
            sys.exit(0)

        # Check restorecon exist
        if not os.path.exists(restorecon):
            print(
                "{0} not exist!".format(restorecon),
                file=sys.stderr
            )
            sys.exit(1)

    except Exception as error:

        print(error, file=sys.stderr)
        sys.exit(1)

    time_hour_ago = time.time() - 3600

    print("Relabel package files...")

    for path in sys.stdin:

        if path == "\n":
            break

        try:

            path = path.rstrip()

            if os.path.lexists(path):

                process = subprocess.run(
                    [restorecon, '-F', path],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    universal_newlines=True
                )

            else:

                print((
                    "Installed path does not exist: {0}").format(path),
                    file=sys.stderr
                )

        except Exception as error:

            print(error, file=sys.stderr)

    print("Relabeling generated directories...")

    for path in RELABLE_DIRS:

        try:

            path = path.rstrip()

            if (os.path.lexists(path) and (
                os.stat(path).st_ctime > time_hour_ago or
                os.stat(path).st_mtime > time_hour_ago
            )):

                process = subprocess.run(
                    [restorecon, '-rF', path],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    universal_newlines=True
                )

        except Exception as error:

            print(error, file=sys.stderr)


if __name__ == "__main__":
    main()
