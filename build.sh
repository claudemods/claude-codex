#!/bin/sh
# build.sh - Claude Codex Compendium v1.0 01-11-2026, in one go:
#   Claude Codex Backup  (the cloner claudebackup-linux: terminal menu + batch commands)
#   Claude Codex Cloner  (the one Qt6 window claude-codex-cloner: Backup, Containers, Updates, Guide)
#   Claude Codex Install (the installer, ClaudeInstall: claudeinstall / claudeinstall-tui)
#   Claude Codex CCU     (the Container Creator Utility: cccu)
#   claude-codex         (the one command: menu, every tool, status, update)
#   1. detects the distro
#   2. installs the build tools and the host tools the ISO build uses (per distro)
#   3. builds and installs the cloner (cloner/, terminal menu + batch commands)
#   4. builds and installs Claude Codex Cloner, the window (app/)
#   5. builds and installs Claude Codex Install (installer/) and configures it to install the CLONE from
#      the live ISO (unpacks its squashfs/erofs image; no users page - the clone keeps its users).
#      Alpine has no Claude Codex Install here: the live ISO gets the cloner's own "Install Cloned System
#      To Disk" instead (its tools are installed in step 2).
#   6. builds and installs CCCU (cccu/: clone this system into an Incus container; its "Create ISO"
#      uses the same cloner engine and Claude Codex Install) - Arch, Debian, Fedora, openSUSE; needs GCC 14+
#   7. installs claude-codex, keeps your answers in /etc/claude-codex/build.conf (claude-codex update
#      reuses them), puts an "Install <distro>" shortcut on the Desktop (it is part of the clone, so the
#      live ISO's desktop has it), then shows what was built and how to start each tool (it starts nothing)
# Plain POSIX sh, so it also runs on a stock Alpine (sh build.sh) without bash.
#
#   ./build.sh                 everything, asking a few questions
#   ./build.sh --yes           everything with the defaults, no questions
#   ./build.sh --no-installer  skip Claude Codex Install          ./build.sh --no-gui   skip the window
#   ./build.sh --btrfs         installer default: btrfs zstd:22 (instead of ext4)
#   ./build.sh --erofs         image type erofs (instead of squashfs)
#   ./build.sh --no-cccu       skip CCCU (the container creator)
#   ./build.sh --installer-mode=clone|new|distro
#                              Claude Codex Install's mode (asked when not given): clone installs the cloned
#                              live ISO; new = new-system installer (users, packages); distro = the
#                              claudemods distributions online (pacstrap + profile, Arch only)
#   ./build.sh --installer-ui=gui|tui|both
#                              Claude Codex Install's interface (asked when not given): Qt6 window, terminal
#                              (claudeinstall-tui) or both. --installer-tui = both
#   ./build.sh help            this text
# Later: claude-codex update  (downloads the latest release and runs this script again with your answers)
set -e

SELF="$(cd "$(dirname "$0")" && pwd)/$(basename "$0")"
SUITE="$(dirname "$SELF")"
CLONER="$SUITE/cloner"
GUI_SRC="$SUITE/app"
GUI_BUILD="$SUITE/build-app"
INSTALLER="$SUITE/installer"
CCCU_SRC="$SUITE/cccu"
TARGET="claudebackup-linux"
GUI_TARGET="claude-codex-cloner"

# Messages are light green (like the app's [ccb] lines), the output of the commands after them is
# dark green; "==>" and errors are red. say() leaves the terminal dark green for the commands' output.
say() { printf '\033[1;31m==>\033[0;92m %s\033[0;32m\n' "$*"; }
die() { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }
printf '\033[32m'

# Single-quote one argument for a shell command line (for su -c)
q() { printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"; }

# ---- privilege helper (sudo, doas, run0, pkexec or su) ----------------------
as_root() {
    if [ "$(id -u)" -eq 0 ]; then "$@"
    elif command -v sudo   >/dev/null 2>&1; then sudo "$@"
    elif command -v doas   >/dev/null 2>&1; then doas "$@"
    elif command -v run0   >/dev/null 2>&1; then run0 "$@"
    elif command -v pkexec >/dev/null 2>&1; then pkexec "$@"
    else
        cmd=""
        for a in "$@"; do cmd="$cmd $(q "$a")"; done
        su root -c "$cmd"
    fi
}

# ---- distro detection (ID, then each ID_LIKE entry, as cloner/src/distro.h does) ----
detect_family() {
    [ -r /etc/os-release ] || die "/etc/os-release not found"
    # shellcheck disable=SC1091
    . /etc/os-release
    FAMILY=""
    for id in ${ID:-} ${ID_LIKE:-}; do
        case "$(printf '%s' "$id" | tr '[:upper:]' '[:lower:]')" in
            debian|ubuntu|linuxmint|pop|kali|devuan|openkylin|quirinux|abellha|neon) FAMILY=debian ;;
            arch|archlinux|archcraft|cachyos|endeavouros|garuda|rebornos)            FAMILY=arch ;;
            fedora|nobara|rhel)                                                      FAMILY=fedora ;;
            opensuse*|suse)                                                          FAMILY=opensuse ;;
            alpine)                                                                  FAMILY=alpine ;;
            gentoo|calculate|funtoo|pentoo)                                          FAMILY=gentoo ;;
        esac
        [ -n "$FAMILY" ] && break
    done
    [ -n "$FAMILY" ] || die "unsupported distro: ${PRETTY_NAME:-${ID:-unknown}}"
    DISTRO_ID="$(printf '%s' "${ID:-linux}" | tr '[:upper:]' '[:lower:]')"
    DISTRO_NAME="${PRETTY_NAME:-${NAME:-${ID:-Linux}}}"
}

# ---- squashfs / erofs ----------------------------------------------------------
# One image type for everything: the ISO the cloner builds and what Claude Codex Install unpacks.
erofs_supported() {
    case "$FAMILY" in alpine|gentoo) return 1 ;; esac
    return 0
}

install_squashfs_tools() {
    say "Installing squashfs-tools"
    case "$FAMILY" in
        debian)   apt-get install -y squashfs-tools ;;
        arch)     pacman -S --needed --noconfirm squashfs-tools ;;
        fedora)   dnf install -y squashfs-tools ;;
        opensuse) zypper --non-interactive --no-refresh install --no-recommends squashfs ;;
        alpine)   apk add squashfs-tools ;;
        gentoo)   emerge --noreplace --ask=n sys-fs/squashfs-tools ;;
    esac
}

install_erofs_utils() {
    erofs_supported || return 0
    command -v mkfs.erofs >/dev/null 2>&1 && return 0
    say "Installing erofs-utils (optional: only needed for erofs images)"
    case "$FAMILY" in
        debian)   apt-get install -y erofs-utils ;;
        arch)     pacman -S --needed --noconfirm erofs-utils ;;
        fedora)   dnf install -y erofs-utils ;;
        opensuse) zypper --non-interactive --no-refresh install --no-recommends erofs-utils ;;
    esac || say "erofs-utils is not available here - only the squashfs image format will work"
}

# Sets IMAGE (squashfs | erofs) and EROFS (1 when erofs was chosen and the kernel can mount it)
choose_image_format() {
    IMAGE=squashfs
    EROFS=0
    if ! erofs_supported; then
        say "$DISTRO_NAME: the image type is squashfs (erofs is not supported on this distro)"
        return 0
    fi
    if [ "$WANT_EROFS" = 1 ]; then
        reply=2
    elif [ "$ASSUME_YES" = 1 ]; then
        reply=1
    else
        echo "System image type for the ISO (the cloner and Claude Codex Install both use it):"
        echo "  1) Squashfs (default)"
        echo "  2) Erofs"
        reply=$(choose "Enter your choice (1 or 2):")
    fi
    if [ "$reply" = 2 ]; then
        # The live ISO boots this kernel, so it must be able to mount erofs
        if grep -qw erofs /proc/filesystems 2>/dev/null || modinfo erofs >/dev/null 2>&1; then
            IMAGE=erofs
            EROFS=1
        else
            say "This kernel has no erofs support (CONFIG_EROFS_FS), so the ISO could not boot an erofs image - using squashfs"
        fi
    fi
}

# The cloner reads its settings from ~/.config/claudebackup-linux/configuration.txt
save_image_format() {
    CFGDIR="$DESK_HOME/.config/claudebackup-linux"
    CFGFILE="$CFGDIR/configuration.txt"
    mkdir -p "$CFGDIR"
    if [ -f "$CFGFILE" ] && grep -q '^imageFormat=' "$CFGFILE"; then
        sed -i "s/^imageFormat=.*/imageFormat=$IMAGE/" "$CFGFILE"
    else
        echo "imageFormat=$IMAGE" >> "$CFGFILE"
    fi
    if [ -n "$DESK_USER" ]; then chown "$DESK_USER" "$CFGDIR" "$CFGFILE" 2>/dev/null || true; fi
    say "Image type set to $IMAGE"
}

finalize_image_format() {
    if [ "$IMAGE" = erofs ] && ! command -v mkfs.erofs >/dev/null 2>&1; then
        say "mkfs.erofs is not available on $DISTRO_NAME - using squashfs"
        IMAGE=squashfs
    fi
    save_image_format
}

# =============================================================================
# Root phase: everything that installs packages or writes system files
# =============================================================================

# Gentoo: emerge refuses to work without the ::gentoo repository and a valid profile. Systems
# installed from a live ISO often have neither (no repo, make.profile copied as a folder).
gentoo_prepare_portage() {
    REPO=$(portageq get_repo_path / gentoo 2>/dev/null || true)
    [ -n "$REPO" ] || REPO=/var/db/repos/gentoo
    if [ ! -d "$REPO/profiles" ]; then
        say "No Gentoo repository in $REPO, fetching it (emerge-webrsync)"
        mkdir -p "$REPO"
        emerge-webrsync || emerge --sync
    fi
    [ -d "$REPO/profiles" ] || die "the Gentoo repository could not be fetched into $REPO"

    MP=/etc/portage/make.profile
    if [ ! -L "$MP" ] || [ ! -e "$MP" ]; then
        WANT=""
        if [ -f "$MP/parent" ]; then
            WANT=$(head -n 1 "$MP/parent" | sed -e 's#^gentoo:##' -e 's#.*/profiles/##')
        fi
        if [ -z "$WANT" ] || [ ! -d "$REPO/profiles/$WANT" ]; then
            GARCH=$(portageq envvar ARCH 2>/dev/null || echo amd64)
            LIST=$(eselect profile list 2>/dev/null | grep '(stable)' | awk '{print $2}')
            WANT=""
            for suffix in /desktop/plasma /desktop ""; do
                WANT=$(echo "$LIST" | grep -E "^default/linux/$GARCH/[0-9.]+$suffix\$" | sort -V | tail -n 1)
                [ -n "$WANT" ] && break
            done
        fi
        [ -n "$WANT" ] || die "no Gentoo profile found - pick one with: eselect profile list / set"
        if [ -e "$MP" ] || [ -L "$MP" ]; then
            rm -rf "$MP.livecd-backup"
            mv "$MP" "$MP.livecd-backup"
        fi
        say "Setting the Gentoo profile to $WANT"
        eselect profile set "$WANT"
    fi
}

# Build tools (compiler, make) + the host tools the ISO build runs, per distro
install_host_tools() {
    say "Installing build and host tools for $DISTRO_NAME"
    # CCCU (cccu iso) runs the engine it carries itself: no compiler in the container unless
    # Claude Codex Install is built there (CBL_NO_COMPILER=1 then is not set)
    CXX_DEB="g++ make"; CXX_ARCH="gcc make"; CXX_FEDORA="gcc-c++ make"; CXX_SUSE="gcc-c++ make"; CXX_APK="build-base"
    if [ -n "${CBL_NO_COMPILER:-}" ]; then
        CXX_DEB=""; CXX_ARCH=""; CXX_FEDORA=""; CXX_SUSE=""; CXX_APK=""
    fi
    # shellcheck disable=SC2086
    case "$FAMILY" in
        debian)
            apt-get update
            apt-get install -y $CXX_DEB xorriso mtools dosfstools curl tar unzip \
                live-boot live-boot-initramfs-tools ;;
        arch)
            pacman -S --needed --noconfirm $CXX_ARCH libisoburn mtools dosfstools tar unzip \
                mkinitcpio device-mapper gnupg ;;
        fedora)
            dnf install -y $CXX_FEDORA xorriso mtools dosfstools curl tar unzip dracut-live ;;
        opensuse)
            # refresh the repositories once; the later zypper calls reuse that metadata (--no-refresh)
            zypper --non-interactive refresh || true
            zypper --non-interactive --no-refresh install --no-recommends $CXX_SUSE xorriso mtools dosfstools curl tar unzip ;;
        alpine)
            apk add $CXX_APK xorriso mtools dosfstools curl tar util-linux ;;
        gentoo)
            gentoo_prepare_portage
            emerge --noreplace --ask=n dev-libs/libisoburn sys-fs/mtools sys-fs/dosfstools \
                net-misc/curl app-arch/unzip sys-kernel/dracut sys-fs/lvm2 ;;
    esac
}

# Qt6 Widgets + CMake for the window (app/)
install_gui_deps() {
    say "Installing Qt6 and CMake for Claude Codex Cloner (the window)"
    case "$FAMILY" in
        debian)   apt-get install -y cmake qt6-base-dev ;;
        arch)     pacman -S --needed --noconfirm cmake qt6-base ;;
        fedora)   dnf install -y cmake qt6-qtbase-devel ;;
        opensuse) zypper --non-interactive --no-refresh install --no-recommends cmake qt6-base-devel ;;
        alpine)   apk add cmake qt6-qtbase-dev ;;
        gentoo)   emerge --noreplace --ask=n dev-build/cmake dev-qt/qtbase:6 ;;
    esac
}

# BIOS/UEFI boot files the live ISO is made from (Arch uses the cloner's embedded claudemods zip
# instead). Kept in a persistent folder; the cloner only reads it (BOOTLOADER_DIR in cloner/src/common.h).
BOOTLOADER_URL="https://github.com/pieroproietti/penguins-bootloaders/releases/download/v26.1.16/bootloaders.tar.gz"
BOOTLOADER_DIR="/usr/local/share/claudebackup-linux/bootloaders"
install_bootloaders() {
    [ "$FAMILY" = arch ] && return 0
    [ -f "$BOOTLOADER_DIR/.download-complete" ] && return 0
    # CCCU carries the boot files inside itself (resources/ of its provisioning bundle): no download
    if [ -s "$SUITE/resources/bootloaders.tar.gz" ]; then
        say "Installing the bootloaders (BIOS/UEFI boot files for the live ISO)"
        rm -rf "$BOOTLOADER_DIR"
        mkdir -p "$BOOTLOADER_DIR"
        if tar -xzf "$SUITE/resources/bootloaders.tar.gz" --strip-components=1 -C "$BOOTLOADER_DIR"; then
            echo ok > "$BOOTLOADER_DIR/.download-complete"
            return 0
        fi
        say "The carried bootloader file could not be unpacked, downloading them instead"
    fi
    say "Downloading the bootloaders (BIOS/UEFI boot files for the live ISO)"
    rm -rf "$BOOTLOADER_DIR"
    mkdir -p "$BOOTLOADER_DIR"
    if curl -fL --progress-bar --connect-timeout 20 --retry 3 --speed-time 30 --speed-limit 1024 "$BOOTLOADER_URL" -o "$BOOTLOADER_DIR.tar.gz" &&
       tar -xzf "$BOOTLOADER_DIR.tar.gz" --strip-components=1 -C "$BOOTLOADER_DIR"; then
        echo ok > "$BOOTLOADER_DIR/.download-complete"
        rm -f "$BOOTLOADER_DIR.tar.gz"
    else
        rm -rf "$BOOTLOADER_DIR" "$BOOTLOADER_DIR.tar.gz"
        die "the bootloaders could not be downloaded from $BOOTLOADER_URL"
    fi
}

# Alpine: the tools the live ISO's "Install Cloned System To Disk" needs (they go into the clone)
install_alpine_installer_tools() {
    say "Installing the Alpine installer tools"
    PKGS="sfdisk e2fsprogs btrfs-progs dosfstools squashfs-tools grub efibootmgr"
    [ "$(uname -m)" = "x86_64" ] && PKGS="$PKGS grub-bios grub-efi"
    # shellcheck disable=SC2086
    apk add $PKGS
}

# What Claude Codex Install needs on the installed system's side: rsync / GRUB / EFI tools, per distro
# (the old Calamares packages pulled the filesystem tools in as dependencies; ClaudeInstall is built
# from source, so they are named here: its partition step formats ext4 / btrfs / xfs / FAT and LUKS)
install_installer_runtime() {
    say "Installing GRUB, efibootmgr, rsync and the filesystem tools (used by Claude Codex Install in the live ISO)"
    ARCH=$(uname -m)
    case "$FAMILY" in
        debian)
            PKGS="rsync efibootmgr dosfstools os-prober e2fsprogs btrfs-progs xfsprogs cryptsetup console-setup"
            [ "$ARCH" = "x86_64" ] && PKGS="$PKGS grub-pc-bin grub-efi-amd64-bin"
            # shellcheck disable=SC2086
            apt-get install -y $PKGS ;;
        arch)
            pacman -S --needed --noconfirm rsync efibootmgr dosfstools grub os-prober e2fsprogs btrfs-progs xfsprogs cryptsetup
            # ckbcomp (keyboard module: console keymap from the X layout) was in the old Calamares zip;
            # CachyOS / Chaotic ship it, plain Arch only has it in the AUR
            pacman -S --needed --noconfirm ckbcomp 2>/dev/null ||
                say "ckbcomp is not in your repositories (AUR on plain Arch) - the keyboard step falls back to the X layout only" ;;
        fedora)
            PKGS="rsync efibootmgr dosfstools e2fsprogs btrfs-progs xfsprogs cryptsetup"
            [ "$ARCH" = "x86_64" ] && PKGS="$PKGS grub2-pc-modules grub2-efi-x64-modules"
            # shellcheck disable=SC2086
            dnf install -y $PKGS ;;
        opensuse)
            PKGS="rsync efibootmgr dosfstools e2fsprogs btrfsprogs xfsprogs cryptsetup"
            [ "$ARCH" = "x86_64" ] && PKGS="$PKGS grub2-i386-pc grub2-x86_64-efi"
            # shellcheck disable=SC2086
            zypper --non-interactive --no-refresh install --no-recommends $PKGS ;;
        gentoo)
            emerge --noreplace --ask=n net-misc/rsync sys-boot/grub sys-boot/efibootmgr sys-fs/dosfstools \
                sys-fs/e2fsprogs sys-fs/btrfs-progs sys-fs/xfsprogs sys-fs/cryptsetup ;;
    esac
}

# The app and the installer shortcut ask for the password through pkexec (polkit)
install_pkexec() {
    command -v pkexec >/dev/null 2>&1 && return 0
    say "Installing pkexec (polkit), used to ask for the administrator password"
    case "$FAMILY" in
        debian)   apt-get install -y pkexec || apt-get install -y policykit-1 ;;
        arch)     pacman -S --needed --noconfirm polkit ;;
        fedora)   dnf install -y polkit ;;
        opensuse) zypper --non-interactive --no-refresh install --no-recommends polkit ;;
        alpine)   apk add polkit ;;
        gentoo)   emerge --noreplace --ask=n sys-auth/polkit ;;
    esac || true
    command -v pkexec >/dev/null 2>&1 || say "pkexec could not be installed - the app's root tasks and the Install shortcut need it"
}

# Called through as_root: $1 family, $2 installer (1/0), $3 alpine tools (1/0), $4 gui (1/0),
# $5 cccu (1/0, optional)
root_phase() {
    detect_family
    [ "$1" = "$FAMILY" ] || die "the distro detected as root differs ($DISTRO_NAME)"
    install_host_tools
    install_squashfs_tools
    install_erofs_utils
    install_bootloaders
    install_pkexec
    if [ "$4" = 1 ]; then install_gui_deps; fi
    if [ "$FAMILY" = alpine ]; then
        if [ "$3" = 1 ]; then install_alpine_installer_tools; fi
    elif [ "$2" = 1 ]; then
        install_installer_runtime
        install_claudeinstall_deps
    fi
    if [ "${5:-0}" = 1 ]; then install_cccu_deps; fi
}

# =============================================================================
# Claude Codex Install configuration for the clone (root). Replaces the Calamares setup of the old
# claudebackup-linux: same module sequence and the same image paths, now for Claude Codex Install.
# $1 = 1 for btrfs zstd:22 as default, $2 = 1 for an erofs image
# =============================================================================
configure_installer() {
    detect_family
    CFG=/etc/claudeinstall
    MOD=$CFG/modules
    SHARE=/usr/share/claudeinstall
    BTRFS="$1"
    SOURCEFS=squashfs
    [ "$2" = "1" ] && SOURCEFS=erofs
    command -v claudeinstall >/dev/null 2>&1 || die "claudeinstall is not installed"

    # Where the family's live initramfs leaves the image during the live session
    case "$FAMILY" in
        arch)   UNPACK=/run/arch/bootmnt/LiveOS/rootfs.img ;;        # claudemods mkinitcpio hooks
        debian) UNPACK=/run/live/medium/live/filesystem.squashfs ;;  # live-boot
        *)      UNPACK=/run/initramfs/live/LiveOS/squashfs.img ;;    # dracut dmsquash-live
    esac
    case "$FAMILY" in
        arch)            GRUB=grub;  INITRAMFS="initcpiocfg initcpio" ;;
        debian)          GRUB=grub;  INITRAMFS=initramfs ;;
        fedora|opensuse) GRUB=grub2; INITRAMFS=dracut ;;
        gentoo)          GRUB=grub;  INITRAMFS=dracut ;;
    esac

    if [ -f "$CFG/settings.conf" ] && [ ! -f "$CFG/.claudebackup" ] && [ ! -d "$CFG.orig" ]; then
        say "Keeping the previous Claude Codex Install config in $CFG.orig"
        cp -a "$CFG" "$CFG.orig"
    fi
    mkdir -p "$MOD" "$CFG/scripts"
    rm -f "$MOD/unpackfs-clone.conf"
    : > "$CFG/.claudebackup"

    SYSTEMD=0
    [ -d /run/systemd/system ] && SYSTEMD=1

    # --- settings.conf -------------------------------------------------------
    EXEC="partition mount unpackfs"
    [ "$SYSTEMD" = 1 ] && EXEC="$EXEC machineid"
    EXEC="$EXEC fstab locale keyboard localecfg shellprocess@ccbkernel $INITRAMFS grubcfg bootloader"
    # Fedora: after the boot loader, fix what the clone carries over from the old system (boot entries)
    [ "$FAMILY" = fedora ] && EXEC="$EXEC shellprocess@ccbfedora"
    [ -f "$MOD/preservefiles.conf" ] && EXEC="$EXEC preservefiles"
    EXEC="$EXEC umount"
    {
        echo "# Written by Claude Codex Compendium build.sh: install the CLONE from the live ISO."
        echo "# No users page - the clone keeps its accounts. Rerun build.sh to regenerate."
        echo "---"
        echo "modules-search: [ local ]"
        echo "instances:"
        echo "- id: ccbkernel"
        echo "  module: shellprocess"
        echo "  config: shellprocess_ccbkernel.conf"
        if [ "$FAMILY" = fedora ]; then
            echo "- id: ccbfedora"
            echo "  module: shellprocess"
            echo "  config: shellprocess_ccbfedora.conf"
        fi
        echo "sequence:"
        echo "- show:"
        for m in welcome locale keyboard partition summary; do echo "  - $m"; done
        echo "- exec:"
        for m in $EXEC; do echo "  - $m"; done
        echo "- show:"
        echo "  - finished"
        echo "branding: default"
        echo "prompt-install: true"
        echo "dont-chroot: false"
        echo "oem-setup: false"
        echo "disable-cancel: false"
        echo "disable-cancel-during-exec: true"
        echo "hide-back-and-next-during-exec: true"
        echo "quit-at-end: false"
    } > "$CFG/settings.conf"

    # --- module configs ------------------------------------------------------
    FSTYPE=ext4
    [ "$BTRFS" = 1 ] && FSTYPE=btrfs

    cat > "$MOD/unpackfs.conf" << EOF
---
unpack:
    - source: "$UNPACK"
      sourcefs: "$SOURCEFS"
      destination: ""
EOF

    cat > "$MOD/partition.conf" << EOF
---
efiSystemPartition: "/boot/efi"
userSwapChoices:
    - none
    - small
    - file
alwaysShowPartitionLabels: true
initialPartitioningChoice: erase
initialSwapChoice: none
defaultFileSystemType: "$FSTYPE"
availableFileSystemTypes: ["ext4","btrfs","xfs"]
EOF

    # btrfs subvolumes and mount options from the claudemods preset; compress=zstd:22 only for btrfs
    for m in "$SHARE/presets/claudemods/modules/mount.conf" "$INSTALLER/config/claudemods/modules/mount.conf"; do
        if [ -f "$m" ]; then cp -f "$m" "$MOD/mount.conf"; break; fi
    done
    [ -f "$MOD/mount.conf" ] || die "mount.conf of the claudemods preset not found"
    if [ "$BTRFS" != 1 ]; then sed -i 's/, compress=zstd:22//' "$MOD/mount.conf"; fi

    cat > "$MOD/fstab.conf" << EOF
---
crypttabOptions: luks
tmpOptions:
    default:
        tmpfs: false
        options: ""
    ssd:
        tmpfs: false
        options: ""
EOF

    cat > "$MOD/bootloader.conf" << EOF
---
efiBootLoader: "grub"
kernelSearchPath: "/usr/lib/modules"
kernelPattern: "^vmlinuz.*"
grubInstall: "$GRUB-install"
grubMkconfig: "$GRUB-mkconfig"
grubCfg: "/boot/$GRUB/grub.cfg"
grubProbe: "$GRUB-probe"
efiBootMgr: "efibootmgr"
efiBootloaderId: "$DISTRO_ID"
installEFIFallback: true
installHybridGRUB: false
EOF

    printf '%s\n' '---' 'overwrite: false' 'keepDistributor: true' > "$MOD/grubcfg.conf"
    case "$FAMILY" in
        arch)
            # initcpio preset = the live kernel's package (linux-cachyos, linux, linux-zen, ...). Inside a
            # container (CCCU) uname -r is the HOST's kernel: CBL_KERNEL names the container's own.
            KREL="${CBL_KERNEL:-$(uname -r)}"
            PKGBASE=$(cat "/usr/lib/modules/$KREL/pkgbase" 2>/dev/null || echo linux)
            printf '%s\n' '---' "kernel: $PKGBASE" 'be_unsafe: false' > "$MOD/initcpio.conf" ;;
        debian)
            printf '%s\n' '---' 'kernel: "all"' 'be_unsafe: false' > "$MOD/initramfs.conf" ;;
        *)
            printf '%s\n' '---' 'options: [ "-f" ]' > "$MOD/dracut.conf" ;;
    esac
    printf '%s\n' '---' 'systemd: true' 'dbus: true' 'dbus-symlink: true' > "$MOD/machineid.conf"

    # The image is a bind of "/", so a separate /boot (vfat ESP as /boot, Gentoo, Fedora) is empty
    # in the clone: give the installed system a kernel before the initramfs step. Runs on the live
    # system (dontChroot); Claude Codex Install passes its target root as ${ROOT}.
    cat > "$CFG/scripts/ccb-kernel.sh" << 'EOF'
#!/bin/sh
R="${1:-}"
[ -n "$R" ] || R=$(awk '$2 ~ /^\/tmp\/(claudeinstall|calamares)-root-[^\/]*$/ {print $2}' /proc/mounts | head -n 1)
[ -d "$R" ] || { echo "ccb-kernel: target root not found"; exit 1; }
KV=$(uname -r)
if ls "$R"/boot/vmlinuz-* "$R"/boot/kernel-* >/dev/null 2>&1; then
    exit 0
fi
# Arch: mkinitcpio presets expect /boot/vmlinuz-<pkgbase> (vmlinuz-linux, vmlinuz-linux-cachyos)
NAME="vmlinuz-$KV"
[ -f "$R/usr/lib/modules/$KV/pkgbase" ] && NAME="vmlinuz-$(cat "$R/usr/lib/modules/$KV/pkgbase")"
mkdir -p "$R/boot"
for k in "$R/usr/lib/modules/$KV/vmlinuz" "$R/lib/modules/$KV/vmlinuz" \
         /run/live/medium/live/vmlinuz /run/initramfs/live/live/vmlinuz /run/arch/bootmnt/boot/vmlinuz-x86_64; do
    if [ -f "$k" ]; then
        cp "$k" "$R/boot/$NAME"
        echo "ccb-kernel: /boot was empty, installed $k as /boot/$NAME"
        break
    fi
done
[ -f "$R/boot/$NAME" ] || { echo "ccb-kernel: no kernel found for $KV"; exit 1; }
# Fedora's GRUB boots BLS entries: create the entry for the copied kernel
if [ "@FAMILY@" = fedora ] && [ -x "$R/usr/bin/kernel-install" ]; then
    chroot "$R" kernel-install add "$KV" "/boot/$NAME" || true
fi
exit 0
EOF
    sed -i "s/@FAMILY@/$FAMILY/" "$CFG/scripts/ccb-kernel.sh"
    chmod 755 "$CFG/scripts/ccb-kernel.sh"
    printf '%s\n' '---' 'dontChroot: true' 'timeout: 600' 'script:' '    - "/etc/claudeinstall/scripts/ccb-kernel.sh ${ROOT}"' \
        > "$MOD/shellprocess_ccbkernel.conf"

    if [ "$FAMILY" = fedora ]; then
        # The clone's boot entries, /etc/kernel/cmdline and /etc/default/grub still name the OLD
        # system's root (UUID, btrfs subvolume, live options): the installed system then hangs
        # after Plymouth. Rewrite them for the new disk, rebuild the initramfs and boot entries,
        # and let SELinux relabel the copied files.
        cat > "$CFG/scripts/ccb-fedora.sh" << 'EOF'
#!/bin/sh
R="${1:-}"
[ -n "$R" ] || R=$(awk '$2 ~ /^\/tmp\/(claudeinstall|calamares)-root-[^\/]*$/ {print $2}' /proc/mounts | head -n 1)
[ -d "$R" ] || { echo "ccb-fedora: target root not found"; exit 1; }
chroot "$R" /bin/sh -s << 'CHROOT'
ROOT_UUID=$(findmnt -no UUID / 2>/dev/null)
FSTYPE=$(findmnt -no FSTYPE / 2>/dev/null)
FSROOT=$(findmnt -no FSROOT / 2>/dev/null | sed 's/\[.*//')
[ -n "$ROOT_UUID" ] || { echo "ccb-fedora: root UUID not found, leaving the boot entries alone"; exit 0; }
ARGS="root=UUID=$ROOT_UUID ro"
if [ "$FSTYPE" = btrfs ] && [ -n "$FSROOT" ] && [ "$FSROOT" != "/" ]; then
    ARGS="$ARGS rootflags=subvol=${FSROOT#/}"
fi
echo "ccb-fedora: kernel command line: $ARGS rhgb quiet"
mkdir -p /etc/kernel
echo "$ARGS rhgb quiet" > /etc/kernel/cmdline
if [ -f /etc/default/grub ]; then
    sed -i -E '/^GRUB_CMDLINE_LINUX=/{
        s/(^|[ "])(root|rootflags|resume|rd\.lvm\.lv|rd\.luks\.[a-z.]+|rd\.live\.[a-z.]+|rd\.md\.uuid)=[^ "]*/\1/g
        s/[[:space:]]+rd\.live\.image//g
        s/[[:space:]]+(root=live:[^ "]*)//g
        s/[[:space:]]+/ /g
    }' /etc/default/grub
fi
BOOT_UUID=$(findmnt -no UUID /boot 2>/dev/null)
[ -n "$BOOT_UUID" ] || BOOT_UUID="$ROOT_UUID"
for stub in /boot/efi/EFI/fedora/grub.cfg; do
    [ -f "$stub" ] && sed -i -E "s/(--fs-uuid[[:space:]]+--set=[a-z]+[[:space:]]+)[^ ]+/\1$BOOT_UUID/" "$stub"
done
rm -f /boot/loader/entries/*.conf
for d in /usr/lib/modules/*/; do
    KV=$(basename "$d")
    IMG="$d/vmlinuz"
    [ -f "$IMG" ] || IMG="/boot/vmlinuz-$KV"
    [ -f "$IMG" ] || continue
    kernel-install add "$KV" "$IMG" || dracut -f --kver "$KV" "/boot/initramfs-$KV.img" || true
done
[ -x /usr/sbin/grub2-mkconfig ] && grub2-mkconfig -o /boot/grub2/grub.cfg 2>&1 | tail -n 3
systemctl set-default graphical.target 2>/dev/null || ln -sf /usr/lib/systemd/system/graphical.target /etc/systemd/system/default.target
touch /.autorelabel
exit 0
CHROOT
EOF
        chmod 755 "$CFG/scripts/ccb-fedora.sh"
        printf '%s\n' '---' 'dontChroot: true' 'timeout: 900' 'script:' '    - "/etc/claudeinstall/scripts/ccb-fedora.sh ${ROOT}"' \
            > "$MOD/shellprocess_ccbfedora.conf"
    fi

    REBOOT="reboot"
    [ "$SYSTEMD" = 1 ] && REBOOT="systemctl -i reboot"
    printf '%s\n' '---' 'restartNowEnabled: true' 'restartNowChecked: false' "restartNowCommand: \"$REBOOT\"" \
        > "$MOD/finished.conf"

    cat > "$MOD/welcome.conf" << EOF
---
showSupportUrl: false
showKnownIssuesUrl: false
showReleaseNotesUrl: false
requirements:
    requiredStorage: 10
    requiredRam: 1.0
    check:
        - storage
        - ram
        - root
    required:
        - storage
        - root
EOF

    # Arch: the installer's own claudemods preset (config/claudemods: module sequence with initcpio,
    # display manager, services, LUKS, network, hwclock, preservefiles; btrfs subvolumes), applied the
    # way Claude Codex Install is meant to be set up - with its claudeconfig tool - then tuned for this
    # clone. The generic files above stay as the fallback when the preset or claudeconfig is missing.
    if [ "$FAMILY" = arch ] && command -v claudeconfig >/dev/null 2>&1 && [ -f "$SHARE/presets/claudemods/settings.conf" ]; then
        say "Applying the claudemods preset (claudeconfig preset claudemods)"
        claudeconfig preset claudemods
        claudeconfig image "$SOURCEFS" "$UNPACK"
        claudeconfig kernel "$PKGBASE"
        if [ "$BTRFS" = 1 ]; then
            claudeconfig fs default btrfs
            claudeconfig btrfs compress zstd:22
        else
            claudeconfig fs default ext4
            claudeconfig btrfs compress off
        fi
        # the kernel fix-up (ccb-kernel.sh, written above) runs before the initramfs is built
        S="$CFG/settings.conf"
        if ! grep -q 'shellprocess@ccbkernel' "$S"; then
            sed -i -E 's/^([[:space:]]*-[[:space:]]*)initcpiocfg[[:space:]]*$/\1shellprocess@ccbkernel\n\1initcpiocfg/' "$S"
            grep -q '^instances:' "$S" || sed -i 's/^sequence:/instances:\n- id: ccbkernel\n  module: shellprocess\n  config: shellprocess_ccbkernel.conf\n\nsequence:/' "$S"
        fi
        grep -q 'shellprocess@ccbkernel' "$S" || say "warning: could not add the kernel fix-up step to $S"
    fi

    # The claudemods sidebar colours (claudeinstall-colors comes with Claude Codex Install)
    if command -v claudeinstall-colors >/dev/null 2>&1; then claudeinstall-colors claudemods >/dev/null 2>&1 || true; fi

    install_installer_launcher
    say "Claude Codex Install configured for $DISTRO_NAME (unpacks $UNPACK as $SOURCEFS, $FSTYPE default, $GRUB)"
}

# Starts Claude Codex Install as root from the live desktop through pkexec, keeping the session's display
# (pkexec drops it, and Claude Codex Install is a GUI). The root helper first matches the unpack settings
# to the ISO's real image (squashfs / erofs) and loads its kernel module.
install_installer_launcher() {
    mkdir -p /usr/local/lib/claudebackup
    cat > /usr/local/lib/claudebackup/install-root.sh << 'EOF'
#!/bin/sh
for conf in /etc/claudeinstall/modules/unpackfs.conf; do
    [ -f "$conf" ] || continue
    SRC=$(sed -n 's/^[[:space:]]*-[[:space:]]*source:[[:space:]]*"\{0,1\}\([^"]*\)"\{0,1\}[[:space:]]*$/\1/p' "$conf" | head -n 1)
    [ -f "$SRC" ] || continue
    TYPE=$(blkid -p -o value -s TYPE "$SRC" 2>/dev/null || true)
    case "$TYPE" in
        squashfs|erofs)
            echo "claudebackup: $SRC is $TYPE"
            sed -i -E "s#^([[:space:]]*sourcefs:[[:space:]]*).*#\1\"$TYPE\"#" "$conf"
            modprobe "$TYPE" 2>/dev/null || true ;;
    esac
done
# the Qt6 window when it was built, else the terminal installer
if command -v claudeinstall >/dev/null 2>&1; then exec claudeinstall -D 6 "$@"; fi
exec claudeinstall-tui "$@"
EOF
    chmod 755 /usr/local/lib/claudebackup/install-root.sh

    cat > /usr/local/bin/claudebackup-install << 'EOF'
#!/bin/sh
# Opens Claude Codex Install as root (the "Install ..." desktop shortcut of the claudebackup live ISO).
# Everything goes to a log, and if it does not start a message window says why, so a desktop
# click never fails silently.
LOG="/tmp/claudebackup-install-$(id -u).log"
echo "=== $(date): starting Claude Codex Install (user $(id -un), session ${XDG_SESSION_TYPE:-?})" >> "$LOG"
ROOT_HELPER=/usr/local/lib/claudebackup/install-root.sh
# Only the terminal installer (claudeinstall-tui) was built: it runs in this terminal
if ! command -v claudeinstall >/dev/null 2>&1; then
    if [ "$(id -u)" -eq 0 ]; then exec "$ROOT_HELPER" "$@"; fi
    if command -v pkexec >/dev/null 2>&1; then exec pkexec "$ROOT_HELPER" "$@"; fi
    exec sudo "$ROOT_HELPER" "$@"
fi
if [ "$(id -u)" -eq 0 ]; then
    "$ROOT_HELPER" "$@" >> "$LOG" 2>&1
elif command -v pkexec >/dev/null 2>&1; then
    pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
        XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" XDG_SESSION_TYPE="$XDG_SESSION_TYPE" \
        "$ROOT_HELPER" "$@" >> "$LOG" 2>&1
else
    echo "pkexec is not installed" >> "$LOG"
    false
fi
RC=$?
echo "=== exit code $RC" >> "$LOG"
if [ "$RC" -ne 0 ]; then
    case "$RC" in
        126) MSG="Claude Codex Install was not started: the password prompt was cancelled or refused." ;;
        127) MSG="Claude Codex Install was not started: a program was not found (claudeinstall or pkexec)." ;;
        *)   MSG="Claude Codex Install stopped with an error (exit code $RC)." ;;
    esac
    MSG="$MSG
Details: $LOG"
    kdialog --error "$MSG" 2>/dev/null ||
        zenity --error --text="$MSG" 2>/dev/null ||
        notify-send "Install" "$MSG" 2>/dev/null ||
        xmessage "$MSG" 2>/dev/null || true
fi
exit "$RC"
EOF
    chmod 755 /usr/local/bin/claudebackup-install
    # the old Calamares launcher of claudebackup-linux, if it is still there
    rm -f /usr/local/bin/claudebackup-calamares /usr/local/lib/claudebackup/calamares-root.sh
}

# =============================================================================
# Claude Codex Install (installer/): dependencies, build, install. Clone mode, Qt6 GUI.
# (Was installer/build.sh; merged here so the compendium has one build script.)
# =============================================================================
IBUILD="$INSTALLER/build"

# Root: what building Claude Codex Install needs, per distro
install_claudeinstall_deps() {
    say "Installing Claude Codex Install's build dependencies"
    case "$FAMILY" in
        debian)
            apt-get update
            apt-get -y install build-essential cmake ninja-build git pkg-config gettext extra-cmake-modules \
                libyaml-cpp-dev libkpmcore-dev libparted-dev libicu-dev libatasmart-dev libpwquality-dev os-prober \
                rsync squashfs-tools qt6-base-dev qt6-base-private-dev qt6-tools-dev qt6-tools-dev-tools qt6-svg-dev \
                qt6-declarative-dev libkf6coreaddons-dev libkf6crash-dev libkf6i18n-dev libkf6widgetsaddons-dev \
                libpolkit-qt6-1-dev ;;
        fedora)
            dnf -y install gcc-c++ make cmake ninja-build git gettext extra-cmake-modules yaml-cpp-devel \
                kpmcore-devel parted-devel libicu-devel libatasmart-devel libpwquality-devel rsync squashfs-tools \
                qt6-qtbase-devel qt6-qtbase-private-devel qt6-qttools-devel qt6-linguist qt6-qtsvg-devel \
                qt6-qtdeclarative-devel kf6-kcoreaddons-devel kf6-kcrash-devel kf6-ki18n-devel \
                kf6-kwidgetsaddons-devel kf6-kconfig-devel kf6-kservice-devel polkit-qt6-1-devel ;;
        opensuse)
            zypper --non-interactive in gcc-c++ make cmake ninja git gettext-tools yaml-cpp-devel \
                kpmcore-devel parted-devel libicu-devel libatasmart-devel libpwquality-devel rsync squashfs \
                qt6-base-devel qt6-base-private-devel qt6-tools-devel qt6-svg-devel qt6-declarative-devel \
                kf6-extra-cmake-modules kf6-kcoreaddons-devel kf6-kcrash-devel kf6-ki18n-devel \
                kf6-kwidgetsaddons-devel polkit-qt6-1-devel ;;
        arch)
            # distro mode also needs pacstrap (arch-install-scripts), curl and unzip at install time
            pacman -Sy --noconfirm --needed base-devel cmake ninja git extra-cmake-modules yaml-cpp kpmcore \
                parted icu libatasmart libpwquality rsync squashfs-tools qt6-base qt6-tools qt6-svg \
                qt6-declarative kcoreaddons kcrash ki18n kwidgetsaddons polkit-qt6 arch-install-scripts curl unzip ;;
        gentoo)
            emerge --noreplace --ask=n dev-build/cmake dev-build/ninja dev-vcs/git kde-frameworks/extra-cmake-modules \
                dev-cpp/yaml-cpp kde-apps/kpmcore dev-qt/qtbase dev-qt/qttools kde-frameworks/kcoreaddons \
                kde-frameworks/ki18n kde-frameworks/kwidgetsaddons net-misc/rsync sys-fs/squashfs-tools ;;
    esac
}

# User (or root in a container): configure + build. One run lists every compile error (ninja -k 0)
# in installer/build/errors.log; full output in build.log. $1 = the interface: gui (default; the
# Qt6 window claudeinstall), tui (the terminal installer claudeinstall-tui) or both.
build_claudeinstall() {
    case "${1:-gui}" in
        tui)  IGUI=OFF; ITUI=ON ;;
        both) IGUI=ON;  ITUI=ON ;;
        *)    IGUI=ON;  ITUI=OFF ;;
    esac
    say "Building Claude Codex Install (installer/, interface: ${1:-gui})"
    # A partly copied tree (partition files landed in src/modules/) gives "Cannot find source file"
    M="$INSTALLER/src/modules"; P="$M/partition"
    if [ -d "$P" ]; then
        for f in Config.cpp Config.h PartitionTuiStep.cpp PartitionTuiStep.h PartitionViewStep.cpp \
                 PartitionViewStep.h partition.conf partition.schema.yaml jobs; do
            if [ -e "$M/$f" ]; then
                if [ -e "$P/$f" ]; then rm -rf "${M:?}/$f"; else mv "$M/$f" "$P/$f"; fi
                say "Repaired tree: src/modules/$f -> src/modules/partition/$f"
            fi
        done
    fi
    GEN=Ninja
    command -v ninja >/dev/null 2>&1 || command -v samu >/dev/null 2>&1 || GEN="Unix Makefiles"
    mkdir -p "$IBUILD"

    # 1. configure: shown live, kept in configure.log. If it fails NOTHING is compiled - the reason
    #    (usually a missing package: KPMcore, KF6, yaml-cpp, polkit-qt6 ...) is in its last lines.
    say "Claude Codex Install: configuring (CMake)"
    if ! run_logged "$IBUILD/configure.log" cmake -S "$INSTALLER" -B "$IBUILD" -G "$GEN" \
            -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=lib \
            -DINSTALL_CONFIG=ON -DWITH_QT6=ON -DWITH_GUI="$IGUI" -DWITH_TUI="$ITUI"; then
        say "Claude Codex Install: CMake configure FAILED - nothing was compiled. The reason is in the lines above"
        say "(usually a missing package; full output: $IBUILD/configure.log)"
        grep -E 'Could NOT find|not found|CMake Error' "$IBUILD/configure.log" | head -n 15
        return 1
    fi
    # modules CMake left out (their dependencies are missing) - they are simply not built
    if grep -q 'Skipping module' "$IBUILD/configure.log"; then
        say "Claude Codex Install: modules CMake skipped (with the reason - usually a missing dependency):"
        grep 'Skipping module' "$IBUILD/configure.log" | sed 's/\x1b\[[0-9;]*m//g' | head -n 40
    fi

    # 2. compile: shown live, kept in build.log; one run lists every error (ninja -k 0) in errors.log
    say "Claude Codex Install: compiling"
    RC=0
    if [ "$GEN" = Ninja ]; then
        run_logged "$IBUILD/build.log" cmake --build "$IBUILD" -j "$(nproc 2>/dev/null || echo 2)" -- -k 0 || RC=$?
    else
        run_logged "$IBUILD/build.log" cmake --build "$IBUILD" -j "$(nproc 2>/dev/null || echo 2)" -- -k || RC=$?
    fi
    grep -E 'error:|FAILED:|Error [0-9]' "$IBUILD/build.log" > "$IBUILD/errors.log" || true
    LAST=$(grep -oE '^\[[0-9]+/[0-9]+\]' "$IBUILD/build.log" | tail -n 1)
    if [ "$RC" -ne 0 ] || [ -s "$IBUILD/errors.log" ]; then
        NFAIL=$(grep -c 'FAILED:' "$IBUILD/errors.log" 2>/dev/null || true)
        say "Claude Codex Install: compiling FAILED${LAST:+ (got to $LAST)} - ${NFAIL:-0} file(s) did not compile"
        say "every error: $IBUILD/errors.log   full output: $IBUILD/build.log"
        return 1
    fi
    say "Claude Codex Install: compiled${LAST:+ ($LAST)}"
}

# run_logged LOGFILE COMMAND...: shows the command's output live AND keeps it in LOGFILE;
# returns the command's exit code (a plain "| tee" would return tee's)
run_logged() {
    _log=$1; shift
    _rcf=$(mktemp)
    # inside "if" set -e does not stop the group, so the exit code is always recorded
    { if "$@" 2>&1; then echo 0 > "$_rcf"; else echo $? > "$_rcf"; fi; } | tee "$_log"
    _rc=$(cat "$_rcf" 2>/dev/null || echo 1)
    rm -f "$_rcf"
    return "${_rc:-1}"
}

# Root: install the build, its commands and the claudemods presets. $1 = the installer's mode:
#   clone   (default) install the cloned system from the live ISO - configure_installer then writes it
#   new     a new-system installer (users, packages): installer/settings.conf
#   distro  the claudemods distributions online (pacstrap + profile, Arch only): settings-distro.conf
install_claudeinstall() {
    IMODE="${1:-clone}"
    say "Installing Claude Codex Install ($IMODE mode)"
    cmake --install "$IBUILD" || return 1
    mkdir -p /etc/claudeinstall/modules
    # claudeconfig (settings), claudeinstall-colors (sidebar colours), claudeinstall-pkgfix (package names)
    for c in claudeconfig claudeinstall-colors claudeinstall-pkgfix; do
        install -Dm755 "$INSTALLER/$c" "/usr/bin/$c"
    done
    # Claude Codex Install's own icon (the GUI's CMake installs it too; a terminal-only build needs it
    # for the "Install <distro>" shortcut) - not the Calamares squid
    install -Dm644 "$INSTALLER/src/branding/default/logo.svg" /usr/share/icons/hicolor/scalable/apps/claudeinstall.svg
    for pf in "$INSTALLER"/config/*/settings.conf "$INSTALLER"/config/*/modules/*.conf; do
        [ -f "$pf" ] || continue
        install -Dm644 "$pf" "/usr/share/claudeinstall/presets/${pf#"$INSTALLER"/config/}"
    done
    # Keeps the installer log: copied into the installed system just before the disks are unmounted
    if [ -f "$INSTALLER/config/claudemods/modules/preservefiles.conf" ]; then
        install -Dm644 "$INSTALLER/config/claudemods/modules/preservefiles.conf" /etc/claudeinstall/modules/preservefiles.conf
    fi
    D=/etc/claudeinstall
    S=/usr/share/claudeinstall
    case "$IMODE" in
        new)
            cp "$INSTALLER/settings.conf" "$D/settings.conf" ;;
        distro)
            # claudemods distributions: pacstrap + profile (modules/distroprofile), picked on the "Distribution" page
            cp "$INSTALLER/settings-distro.conf" "$D/settings.conf"
            install -Dm644 "$INSTALLER/packagechooser-distro.conf" "$D/modules/packagechooser-distro.conf"
            install -Dm644 "$INSTALLER/packagechooser-regdom.conf" "$D/modules/packagechooser-regdom.conf"
            for f in distroprofile distroprofile-customize distroprofile-finalize; do
                install -Dm644 "$INSTALLER/src/modules/distroprofile/$f.conf" "$D/modules/$f.conf"
            done
            for p in "$INSTALLER"/profiles/*.conf; do install -Dm644 "$p" "$S/profiles/$(basename "$p")"; done
            for i in "$INSTALLER"/images/*.png; do install -Dm644 "$i" "$S/images/$(basename "$i")"; done
            # the small local files the profiles copy (pacman.conf, tweak scripts ...)
            if [ -d "$INSTALLER/needed-files" ]; then
                mkdir -p "$S/needed-files"
                cp -a "$INSTALLER/needed-files/." "$S/needed-files/"
            else
                say "installer/needed-files not found: the profiles' copy steps will fail"
            fi ;;
    esac
    # new / distro: keep the installer log in the installed system (clone mode lists it itself)
    if [ "$IMODE" != clone ] && ! grep -Eq '^[[:space:]]*-[[:space:]]*preservefiles' "$D/settings.conf"; then
        sed -i -E 's/^([[:space:]]*-[[:space:]]*)umount[[:space:]]*$/\1preservefiles\n\1umount/' "$D/settings.conf"
    fi
}

# =============================================================================
# CCCU (cccu/): dependencies, compiler, build, install.
# (Was cccu/build.sh; merged here so the compendium has one build script.)
# =============================================================================
CBUILD="$CCCU_SRC/build"

# Root: CCCU's build and runtime tools. Incus itself is installed later by CCCU when it sets up a host;
# its window is the Containers tab of Claude Codex Cloner (Qt6 comes with install_gui_deps).
install_cccu_deps() {
    say "Installing CCCU's build dependencies"
    case "$FAMILY" in
        arch)
            pacman -S --needed --noconfirm base-devel cmake curl rsync tar zstd squashfs-tools parted dosfstools e2fsprogs btrfs-progs polkit ;;
        debian)
            apt-get update
            apt-get install -y build-essential cmake curl rsync tar zstd squashfs-tools parted dosfstools e2fsprogs btrfs-progs
            # CCCU is C++23: older releases need g++-14 next to the default compiler
            cccu_compiler >/dev/null || apt-get install -y g++-14 || true ;;
        fedora)
            dnf install -y gcc-c++ make cmake curl rsync tar zstd squashfs-tools parted dosfstools e2fsprogs btrfs-progs glibc-static polkit ;;
        opensuse)
            zypper --non-interactive install gcc-c++ make cmake curl rsync tar zstd squashfs parted dosfstools e2fsprogs btrfsprogs glibc-devel-static polkit ;;
    esac
    return 0
}

# Prints a C++23 compiler (GCC 14+ or Clang 17+); fails when there is none
cccu_compiler() {
    for cxx in "${CXX:-}" g++-15 g++-14 g++ clang++-19 clang++-18 clang++-17 clang++; do
        [ -n "$cxx" ] && command -v "$cxx" >/dev/null 2>&1 || continue
        MACROS=$(echo | "$cxx" -dM -E -x c++ - 2>/dev/null) || continue
        if printf '%s\n' "$MACROS" | grep -q '__clang__'; then
            MAJOR=$(printf '%s\n' "$MACROS" | sed -n 's/^#define __clang_major__ //p'); NEED=17
        else
            MAJOR=$(printf '%s\n' "$MACROS" | sed -n 's/^#define __GNUC__ //p'); NEED=14
        fi
        if [ "${MAJOR:-0}" -ge "$NEED" ]; then echo "$cxx"; return 0; fi
    done
    return 1
}

# User: build cccu. The ISO boot files of the non-Arch families are fetched once into cccu/resources
# and carried inside cccu.
build_cccu() {
    CXX_FOUND=$(cccu_compiler) || { say "CCCU needs GCC 14+ or Clang 17+ (C++23); this system's compiler is too old"; return 1; }
    say "Compiler for CCCU: $CXX_FOUND"
    mkdir -p "$CCCU_SRC/resources"
    BL="$CCCU_SRC/resources/bootloaders.tar.gz"
    if [ ! -s "$BL" ]; then
        if curl -fL --progress-bar --connect-timeout 20 --retry 3 "$BOOTLOADER_URL" -o "$BL.part"; then
            mv "$BL.part" "$BL"
        else
            rm -f "$BL.part"
            say "Could not download the ISO boot files; 'cccu iso' then downloads them inside the container"
        fi
    fi
    cmake -S "$CCCU_SRC" -B "$CBUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX_FOUND" &&
        cmake --build "$CBUILD" -j "$(nproc 2>/dev/null || echo 2)"
}

# Root: install cccu (/usr/local/bin/cccu; the polkit rule for it comes with install_suite)
install_cccu() {
    cmake --install "$CBUILD" || return 1
    # the separate CCU window of earlier releases: now the Containers tab of Claude Codex Cloner
    rm -f /usr/local/bin/cccu-gui /usr/local/share/applications/cccu.desktop /usr/local/share/icons/hicolor/scalable/apps/cccu.svg
}

# =============================================================================
# Phase 2 of a normal run, as root: install everything phase 1 built, then configure.
# $1 the window built (1/0), $2 Claude Codex Install built (1/0), $3 its mode (clone|new|distro),
# $4 btrfs default (1/0), $5 erofs image (1/0), $6 CCCU built (1/0), $7 this run's answers as options
# =============================================================================
install_all() {
    say "[install] Claude Codex Backup ($TARGET)"
    make -C "$CLONER" install || return 1
    # the separate Backup window of earlier releases: now the Backup tab of Claude Codex Cloner
    rm -f /usr/bin/claudebackup-linux-qt6 /usr/share/applications/claudebackup-linux-qt6.desktop
    if [ "$1" = 1 ]; then
        say "[install] Claude Codex Cloner ($GUI_TARGET: Backup, Containers, Updates, Guide)"
        cmake --install "$GUI_BUILD" || return 1
    fi
    if [ "$2" = 1 ]; then
        install_claudeinstall "$3" || return 1
        if [ "$3" = clone ]; then
            say "[install] configuring Claude Codex Install to install the clone"
            configure_installer "$4" "$5"
        fi
    fi
    if [ "$6" = 1 ]; then
        say "[install] Claude Codex CCU"
        install_cccu || return 1
    fi
    install_codex_commands
    install_suite "${7:-}"
}

# Root: claude-codex (the one command), the polkit rule for the window's tasks, the release file, and
# this run's answers (claude-codex update runs build.sh again with them). $1 = the answers as options
install_suite() {
    say "[install] claude-codex (menu, every tool, status, update)"
    install -Dm755 "$SUITE/claude-codex" /usr/local/bin/claude-codex
    install -Dm644 "$SUITE/share/org.claudemods.claude-codex.policy" /usr/share/polkit-1/actions/org.claudemods.claude-codex.policy
    mkdir -p /usr/share/claude-codex /etc/claude-codex
    {
        echo "release=$(cat "$SUITE/RELEASE" 2>/dev/null || echo 'Compendium v1.0 01-11-2026')"
        echo "version=$(cat "$SUITE/VERSION" 2>/dev/null || echo 1.0)"
        echo "built=$(date '+%Y-%m-%d %H:%M')"
        echo "source=$SUITE"
    } > /usr/share/claude-codex/release
    if [ -n "$1" ]; then
        printf '# Written by build.sh: its answers, reused by claude-codex update. Run ./build.sh to change them.\nFLAGS="%s"\n' "$1" > /etc/claude-codex/build.conf
        say "  your answers ($1) are kept in /etc/claude-codex/build.conf"
    fi
    if command -v update-desktop-database >/dev/null 2>&1; then update-desktop-database -q 2>/dev/null || true; fi
    for d in /usr/share/icons/hicolor /usr/local/share/icons/hicolor; do
        if [ -d "$d" ] && command -v gtk-update-icon-cache >/dev/null 2>&1; then gtk-update-icon-cache -q -t "$d" 2>/dev/null || true; fi
    done
    return 0
}

# claude-codex-* commands next to the original ones (links in /usr/local/bin), for the tools that
# are installed; a link whose tool is not installed is removed. The window is claude-codex-cloner itself.
install_codex_commands() {
    say "[install] claude-codex-* commands"
    # the two windows of earlier releases are one now (claude-codex-cloner)
    rm -f /usr/local/bin/claude-codex-backup-gui /usr/local/bin/claude-codex-ccu-gui
    for pair in claude-codex-backup:claudebackup-linux \
                claude-codex-install:claudeinstall claude-codex-install-tui:claudeinstall-tui \
                claude-codex-ccu:cccu; do
        CMD=${pair%%:*}; REAL=${pair#*:}
        rm -f "/usr/local/bin/$CMD"
        TARGET_PATH=$(command -v "$REAL" 2>/dev/null) || continue
        ln -s "$TARGET_PATH" "/usr/local/bin/$CMD" && say "  $CMD -> $TARGET_PATH"
    done
    return 0
}

# ---- Desktop shortcut ------------------------------------------------------
# "Install <distro>" on the Desktop: part of the clone, so the live ISO's desktop has it.
add_desktop_shortcut() {
    if [ "$FAMILY" = alpine ]; then
        COMMENT="Install this cloned system to a disk ($TARGET alpine)"
        ICON=claude-codex-backup
        if command -v konsole >/dev/null 2>&1; then EXEC="konsole --hold -e $TARGET alpine"; IN_TERMINAL=false
        else EXEC="$TARGET alpine"; IN_TERMINAL=true; fi
    else
        COMMENT="Install this cloned system to a disk (Claude Codex Install)"
        ICON=claudeinstall
        EXEC=/usr/local/bin/claudebackup-install
        IN_TERMINAL=false
        # only the terminal installer was built: open the shortcut in a terminal (same icon)
        if ! command -v claudeinstall >/dev/null 2>&1; then IN_TERMINAL=true; fi
    fi

    ENTRY="[Desktop Entry]
Type=Application
Version=1.0
Name=Install $DISTRO_NAME
Comment=$COMMENT
Exec=$EXEC
Icon=$ICON
Terminal=$IN_TERMINAL
Categories=System;"

    HOMES=""
    if [ -n "$DESK_USER" ] || [ "$(id -u)" -ne 0 ]; then
        HOMES="$DESK_HOME"
    else
        HOMES=$(awk -F: '$3 >= 1000 && $3 < 60000 && $6 != "" && $7 !~ /(nologin|false)$/ { print $6 }' /etc/passwd)
        [ -n "$HOMES" ] || HOMES="$DESK_HOME"
    fi
    for H in $HOMES /etc/skel; do
        [ -d "$H" ] || continue
        DIR=""
        if [ "$H" != /etc/skel ] && command -v xdg-user-dir >/dev/null 2>&1; then DIR=$(HOME="$H" xdg-user-dir DESKTOP 2>/dev/null || true); fi
        if [ -z "$DIR" ] || [ "$DIR" = "$H" ]; then DIR="$H/Desktop"; fi
        OWNER=$(stat -c %U "$H" 2>/dev/null || true)
        as_root mkdir -p "$DIR"
        # the old claudebackup-linux shortcut opened Calamares
        as_root rm -f "$DIR/claudebackup-install.desktop"
        FILE="$DIR/claudebackup-install.desktop"
        printf '%s\n' "$ENTRY" | as_root tee "$FILE" >/dev/null
        as_root chmod 755 "$FILE"
        if command -v gio >/dev/null 2>&1; then gio set "$FILE" metadata::trusted true 2>/dev/null || true; fi
        if [ -n "$OWNER" ] && [ "$H" != /etc/skel ]; then as_root chown "$OWNER" "$DIR" "$FILE" 2>/dev/null || true; fi
        say "Added 'Install $DISTRO_NAME' to $DIR"
    done
    printf '%s\n' "$ENTRY" | as_root tee /usr/share/applications/claudebackup-install.desktop >/dev/null
    as_root chmod 644 /usr/share/applications/claudebackup-install.desktop
}

# CCCU (cccu iso), as root INSIDE an Incus container after --root-phase: builds and installs
# Claude Codex Install there, configures it for the clone and adds the Install shortcut, so the live ISO
# made from the container can install itself. $1 = 1 for btrfs zstd:22, $2 = 1 for an erofs image
container_installer() {
    detect_family
    [ "$FAMILY" != alpine ] || die "Alpine uses the cloner's own installer, not Claude Codex Install"
    say "Building Claude Codex Install inside the container (installer/, clone mode)"
    build_claudeinstall || die "Claude Codex Install did not build (see $IBUILD/errors.log)"
    install_claudeinstall || die "Claude Codex Install could not be installed"
    configure_installer "$1" "$2"
    install_codex_commands
    DESK_USER=""
    DESK_HOME=/root
    add_desktop_shortcut
}

case "${1:-}" in
    --root-phase)          shift; root_phase "$@"; exit 0 ;;
    --configure-installer) shift; configure_installer "$@"; exit 0 ;;
    --container-installer) shift; container_installer "$@"; exit 0 ;;
    --install-all)         shift; install_all "$@"; exit $? ;;
    --install-claudeinstall) shift; install_claudeinstall "${1:-clone}"; exit $? ;;
    --install-cccu)        install_cccu; exit $? ;;
    help|-h|--help)        sed -n '2,/^set -e/p' "$SELF" | sed '$d' | sed 's/^# \{0,1\}//'; exit 0 ;;
esac

# =============================================================================
# User phase: questions, root phase, build, install, the Complete screen (nothing is started)
# =============================================================================
ASSUME_YES=0; WANT_INSTALLER=""; WANT_GUI=1; WANT_BTRFS=""; WANT_EROFS=0; WANT_CCCU=1
IMODE=""; IUI=""   # Claude Codex Install's mode and interface: asked below unless given
for a in "$@"; do
    case "$a" in
        --yes|-y)       ASSUME_YES=1 ;;
        --no-installer) WANT_INSTALLER=0 ;;
        --installer)    WANT_INSTALLER=1 ;;
        --no-gui)       WANT_GUI=0 ;;
        --btrfs)        WANT_BTRFS=1 ;;
        --ext4)         WANT_BTRFS=0 ;;
        --erofs)        WANT_EROFS=1 ;;
        --no-launch)    ;;   # nothing is started any more; kept so old command lines still work
        --no-cccu)      WANT_CCCU=0 ;;
        --installer-mode=clone|--installer-mode=new|--installer-mode=distro)
                        IMODE="${a#--installer-mode=}"; WANT_INSTALLER=1 ;;
        --installer-mode=*) die "--installer-mode is clone, new or distro" ;;
        --installer-ui=gui|--installer-ui=tui|--installer-ui=both)
                        IUI="${a#--installer-ui=}"; WANT_INSTALLER=1 ;;
        --installer-ui=*) die "--installer-ui is gui, tui or both" ;;
        --installer-tui) IUI=both; WANT_INSTALLER=1 ;;
        *) die "unknown option: $a (see ./build.sh help)" ;;
    esac
done

for d in "$CLONER/Makefile" "$GUI_SRC/CMakeLists.txt" "$INSTALLER/CMakeLists.txt" "$CCCU_SRC/CMakeLists.txt" "$SUITE/claude-codex"; do
    [ -f "$d" ] || die "$d not found - run build.sh from the full claude-codex-compendium folder (app/, cloner/, installer/, cccu/ and claude-codex next to it). A 'fixes' folder only holds changed files and cannot build on its own."
done
[ "$(uname -s)" = "Linux" ] || die "this project only builds on Linux"
detect_family
say "Detected $DISTRO_NAME"
if [ "$IMODE" = distro ] && [ "$FAMILY" != arch ]; then
    die "--installer-mode=distro installs with pacstrap and must be built and run on an Arch-family system"
fi

# numbered choice; $1 prompt, $2 highest number; prints the number (default 1)
choose_n() {
    if [ "$ASSUME_YES" = 1 ] || [ ! -t 0 ]; then echo 1; return; fi
    printf '\033[1;92m%s\033[0;32m ' "$1" >&2
    read -r reply
    case "$reply" in
        [1-9]) if [ "$reply" -le "$2" ]; then echo "$reply"; else echo 1; fi ;;
        *) echo 1 ;;
    esac
}

# yes/no question; $2 is the default (y/n); --yes or no terminal = the default
ask() {
    if [ "$ASSUME_YES" = 1 ] || [ ! -t 0 ]; then [ "$2" = y ]; return; fi
    printf '\033[1;92m%s\033[0;32m ' "$1"
    read -r reply
    reply=$(printf '%s' "${reply:-$2}" | tr '[:upper:]' '[:lower:]')
    [ "$reply" = y ] || [ "$reply" = yes ]
}
# choice of 1/2; prints 1 or 2 (default 1)
choose() {
    if [ "$ASSUME_YES" = 1 ] || [ ! -t 0 ]; then echo 1; return; fi
    printf '\033[1;92m%s\033[0;32m ' "$1" >&2
    read -r reply
    [ "$reply" = 2 ] && echo 2 || echo 1
}

INSTALLER_ON=0; BTRFS=0; ALPINE_TOOLS=0

choose_image_format

if [ "$FAMILY" = alpine ]; then
    say "Alpine: the live ISO uses the cloner's own 'Install Cloned System To Disk' (not Claude Codex Install)"
    if ask "Install the Alpine installer tools (sfdisk, mkfs, GRUB) so the ISO can install itself? [Y/n]" y; then
        ALPINE_TOOLS=1
    fi
elif [ "$WANT_INSTALLER" = 1 ] || { [ "$WANT_INSTALLER" != 0 ] && ask "Build Claude Codex Install (the installer)? [Y/n]" y; }; then
    INSTALLER_ON=1
    # --- what it installs --------------------------------------------------
    if [ -z "$IMODE" ]; then
        echo "What should Claude Codex Install install?"
        echo "  1) The cloned system - the live ISOs built here install themselves (default)"
        echo "  2) A new system - creates users, installs packages (a normal distro installer)"
        N=2
        if [ "$FAMILY" = arch ]; then
            echo "  3) claudemods distributions online - pacstrap + profile (Spitfire / Apex)"
            N=3
        fi
        case "$(choose_n "Enter your choice (1-$N):" "$N")" in
            2) IMODE=new ;;
            3) IMODE=distro ;;
            *) IMODE=clone ;;
        esac
    fi
    if [ "$IMODE" != clone ]; then
        say "Claude Codex Install in $IMODE mode: the live ISOs made by the cloner get no 'Install' shortcut of their own"
    fi
    # --- its interface -----------------------------------------------------
    if [ -z "$IUI" ]; then
        echo "Which Claude Codex Install interface?"
        echo "  1) GUI - the Qt6 window (default)"
        echo "  2) TUI - in a terminal (claudeinstall-tui)"
        echo "  3) Both"
        case "$(choose_n "Enter your choice (1-3):" 3)" in
            2) IUI=tui ;;
            3) IUI=both ;;
            *) IUI=gui ;;
        esac
    fi
    # --- clone mode: the installed system's default filesystem --------------
    if [ "$IMODE" = clone ]; then
        if [ "$FAMILY" = fedora ]; then
            say "Fedora: Claude Codex Install replaces Anaconda - Anaconda's installer shortcuts are left out of the ISO"
        fi
        if [ -n "$WANT_BTRFS" ]; then
            BTRFS="$WANT_BTRFS"
        else
            echo "Claude Codex Install default filesystem for the installed system:"
            echo "  1) Ext4 (default)"
            echo "  2) Btrfs with level 22 compression (zstd:22)"
            [ "$(choose "Enter your choice (1 or 2):")" = 2 ] && BTRFS=1
        fi
    fi
    say "Claude Codex Install: $IMODE mode, $IUI interface"
fi
IMODE="${IMODE:-clone}"; IUI="${IUI:-gui}"
if [ "$WANT_GUI" = 1 ] && ! ask "Build Claude Codex Cloner, the window (Backup, Containers, Updates, Guide)? [Y/n]" y; then WANT_GUI=0; fi
# CCCU supports the families with systemd and an Incus package
case "$FAMILY" in
    arch|debian|fedora|opensuse) ;;
    *) [ "$WANT_CCCU" = 1 ] && say "CCCU (containers) does not support $DISTRO_NAME - skipping it"; WANT_CCCU=0 ;;
esac
if [ "$WANT_CCCU" = 1 ] && ! ask "Build Claude Codex CCU, to clone this system into Incus containers? [Y/n]" y; then WANT_CCCU=0; fi

# The user whose Desktop gets the shortcut (also when build.sh itself was started through sudo/doas)
DESK_USER=""
DESK_HOME="$HOME"
if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-${DOAS_USER:-}}" ]; then
    DESK_USER="${SUDO_USER:-$DOAS_USER}"
    DESK_HOME=$(awk -F: -v u="$DESK_USER" '$1 == u { print $6 }' /etc/passwd)
    [ -n "$DESK_HOME" ] || DESK_HOME="/home/$DESK_USER"
elif [ "$(id -u)" -eq 0 ] && [ -n "${PKEXEC_UID:-}" ]; then
    # claude-codex update from the window (pkexec)
    DESK_USER=$(awk -F: -v u="$PKEXEC_UID" '$3 == u { print $1 }' /etc/passwd)
    DESK_HOME=$(awk -F: -v u="$PKEXEC_UID" '$3 == u { print $6 }' /etc/passwd)
    [ -n "$DESK_HOME" ] || { DESK_USER=""; DESK_HOME="$HOME"; }
fi

# This run's answers as options: claude-codex update runs build.sh --yes with them
FLAGS=""
if [ "$FAMILY" != alpine ]; then
    if [ "$INSTALLER_ON" = 1 ]; then
        FLAGS="--installer-mode=$IMODE --installer-ui=$IUI"
        if [ "$IMODE" = clone ]; then
            if [ "$BTRFS" = 1 ]; then FLAGS="$FLAGS --btrfs"; else FLAGS="$FLAGS --ext4"; fi
        fi
    else
        FLAGS="--no-installer"
    fi
fi
[ "$IMAGE" = erofs ] && FLAGS="$FLAGS --erofs"
[ "$WANT_GUI" = 1 ] || FLAGS="$FLAGS --no-gui"
[ "$WANT_CCCU" = 1 ] || FLAGS="$FLAGS --no-cccu"
FLAGS="${FLAGS# }"

say "Installing packages as root"
as_root sh "$SELF" --root-phase "$FAMILY" "$INSTALLER_ON" "$ALPINE_TOOLS" "$WANT_GUI" "$WANT_CCCU"

finalize_image_format

# =============================================================================
# Phase 1: BUILD everything, as you - nothing is installed until all of it is compiled
# =============================================================================
JOBS="$(nproc 2>/dev/null || echo 1)"
GUI_OK=0; INSTALLER_OK=0; CCCU_OK=0
R_GUI=skipped; R_INSTALL=skipped; R_CCU=skipped   # for the summary: ok | failed | skipped
banner() { printf '\n\033[1;31m=====\033[1;97m %s \033[1;31m=====\033[0;32m\n' "$*"; }

banner "BUILD 1/4  Claude Codex Backup - the cloner (cloner/)"
make -C "$CLONER" -j"$JOBS" || die "Claude Codex Backup (the cloner) did not build - see the messages above"

if [ "$WANT_GUI" = 1 ]; then
    banner "BUILD 2/4  Claude Codex Cloner - the window (app/)"
    GEN=""
    if command -v ninja >/dev/null 2>&1 || command -v samu >/dev/null 2>&1; then GEN="-G Ninja"; fi
    mkdir -p "$GUI_BUILD"
    # shown live and kept in build-app/build.log; the error lines go to build-app/errors.log and are
    # repeated on the Complete screen, so they do not scroll away
    # shellcheck disable=SC2086
    if run_logged "$GUI_BUILD/configure.log" cmake -S "$GUI_SRC" -B "$GUI_BUILD" $GEN -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr &&
       run_logged "$GUI_BUILD/build.log" cmake --build "$GUI_BUILD" -j "$JOBS"; then
        GUI_OK=1; R_GUI=ok
        rm -f "$GUI_BUILD/errors.log"
    else
        R_GUI=failed
        cat "$GUI_BUILD/configure.log" "$GUI_BUILD/build.log" 2>/dev/null |
            grep -E -A3 'error:|Error|CMake Error|undefined reference' > "$GUI_BUILD/errors.log" || true
    fi
else
    banner "BUILD 2/4  Claude Codex Cloner (the window): skipped"
fi

if [ "$INSTALLER_ON" = 1 ]; then
    banner "BUILD 3/4  Claude Codex Install (installer/, $IMODE mode, $IUI) - about 1000 files"
    if build_claudeinstall "$IUI"; then
        INSTALLER_OK=1; R_INSTALL=ok
    else
        R_INSTALL=failed
    fi
else
    banner "BUILD 3/4  Claude Codex Install: skipped"
fi

if [ "$WANT_CCCU" = 1 ]; then
    banner "BUILD 4/4  Claude Codex CCU (cccu/)"
    if build_cccu; then
        CCCU_OK=1; R_CCU=ok
    else
        R_CCU=failed
    fi
else
    banner "BUILD 4/4  Claude Codex CCU: skipped"
fi

# =============================================================================
# Phase 2: INSTALL everything that compiled, as root (one password prompt)
# =============================================================================
banner "INSTALL  everything that compiled (as root)"
as_root sh "$SELF" --install-all "$GUI_OK" "$INSTALLER_OK" "$IMODE" "$BTRFS" "$EROFS" "$CCCU_OK" "$FLAGS" ||
    die "installing failed - see the messages above"
command -v "$TARGET" >/dev/null 2>&1 || die "$TARGET is not on PATH after install"
[ "$INSTALLER_OK" = 1 ] || INSTALLER_ON=0

# (the shortcut opens the clone installer: not for Claude Codex Install's new / distro modes)
if { [ "$INSTALLER_ON" = 1 ] && [ "$IMODE" = clone ]; } || [ "$ALPINE_TOOLS" = 1 ]; then add_desktop_shortcut; fi

# =============================================================================
# Complete: what was built and installed, and how to start each tool. Nothing is started here.
# =============================================================================
banner "COMPLETE - Claude Codex Compendium v1.0 01-11-2026"
row() {   # row TOOL RESULT HOW-TO-START
    case "$2" in
        ok)     _c='\033[1;92m'; _t='built + installed' ;;
        failed) _c='\033[1;91m'; _t='FAILED' ;;
        *)      _c='\033[1;33m'; _t='skipped' ;;
    esac
    printf "  \033[1;97m%-22s\033[0m ${_c}%-18s\033[0m %s\n" "$1" "$_t" "$3"
}
case "$IMODE" in
    clone) HOW_INSTALL="in the live ISO: the 'Install $DISTRO_NAME' shortcut / claudebackup-install" ;;
    *)     HOW_INSTALL="sudo claude-codex-install$([ "$IUI" != gui ] && echo '  /  sudo claude-codex-install-tui')" ;;
esac
[ "$IUI" = tui ] && [ "$IMODE" = clone ] && HOW_INSTALL="$HOW_INSTALL (runs in a terminal)"
printf '\n'
case "$R_GUI" in ok) HOW_CCU="the Containers tab  /  sudo claude-codex ccu" ;; *) HOW_CCU="sudo claude-codex ccu  (terminal menu)" ;; esac
row "Claude Codex Cloner"  "$R_GUI"     "claude-codex-cloner  /  'Claude Codex Cloner' in the menu (Backup, Containers, Updates, Guide)"
row "Claude Codex Backup"  ok           "the Backup tab  /  claude-codex backup  (terminal menu)"
row "Claude Codex Install" "$R_INSTALL" "$HOW_INSTALL"
row "Claude Codex CCU"     "$R_CCU"     "$HOW_CCU"
row "claude-codex"         ok           "claude-codex  (menu of everything)  /  claude-codex update  (update everything)"
printf '\n'
FAILED=0
if [ "$R_GUI" = failed ]; then
    FAILED=1
    say "Claude Codex Cloner did not compile. The errors (all of them: $GUI_BUILD/errors.log, full output: build.log):"
    printf '\033[0;91m'
    head -n 40 "$GUI_BUILD/errors.log" 2>/dev/null
    printf '\033[0m\n'
fi
if [ "$R_INSTALL" = failed ]; then FAILED=1; say "Claude Codex Install: every error in $IBUILD/errors.log (configure: configure.log, all output: build.log)"; fi
if [ "$R_CCU" = failed ]; then FAILED=1; say "Claude Codex CCU: see the messages above (needs GCC 14+ / Clang 17+ and CMake 3.25+; build folder $CBUILD)"; fi
printf '\033[0m'
if [ "$FAILED" = 1 ]; then
    printf '\033[1;91mNot everything you picked was built. Fix the errors above and run ./build.sh again.\033[0m\n'
    exit 1
fi
printf '\033[1;92mEverything is built and installed. Nothing was started - use the commands above.\033[0m\n'
exit 0
