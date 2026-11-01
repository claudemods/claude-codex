<p align="center">
<img src="https://i.postimg.cc/JhMRf2RZ/claudemods-03-17-2025.gif">

<div align="center">

  <a href="https://www.linux.org" target="_blank"><img src="https://img.shields.io/badge/OS-Linux-e06c75?style=for-the-badge&logo=linux" /></a>
  <a href="https://isocpp.org" target="_blank"><img src="https://img.shields.io/badge/LANG-C++23-00599C?style=for-the-badge&logo=cplusplus" /></a>
  <a href="https://linuxcontainers.org/incus/" target="_blank"><img src="https://img.shields.io/badge/CONTAINERS-Incus-3b82f6?style=for-the-badge&logo=linuxcontainers&logoColor=white" /></a>
  <a href="https://kde.org/plasma-desktop/" target="_blank"><img src="https://img.shields.io/badge/DESKTOP-KDE_Plasma-1d99f3?style=for-the-badge&logo=kde&logoColor=white" /></a>

  <a href="https://archlinux.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Arch-56b6c2?style=for-the-badge&logo=arch-linux" /></a>
  <a href="https://ubuntu.com/" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Ubuntu-E95420?style=for-the-badge&logo=Ubuntu" /></a>
  <a href="https://www.debian.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Debian-CE0058?style=for-the-badge&logo=Debian" /></a>
  <a href="https://getfedora.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Fedora-51A2DA?style=for-the-badge&logo=fedora" /></a>
  <a href="https://www.opensuse.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-openSUSE-73BA25?style=for-the-badge&logo=opensuse" /></a>
  <a href="https://linuxmint.com" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Linux_Mint-87CF3E?style=for-the-badge&logo=linuxmint&logoColor=white" /></a>

  <a href="https://claude.com/claude-code" target="_blank">
    <img src="https://img.shields.io/badge/Built_Using-Claude_Code-D97757?style=for-the-badge&logo=claude&logoColor=white" alt="Built Using Claude Code">
  </a>

  ## [ Guide ](#usage)

  ## [ Support Me ](https://www.paypal.com/paypalme/claudemods?country.x=GB&locale)

</div>

# Claude Codex CCU - Container Creator Utility

**Claude Codex Compendium v1.0 01-11-2026** (formerly CCCU, the Claudemods Container Creator Utility). The command is `cccu` (also `claude-codex-ccu` and `claude-codex ccu`); its window is the **Containers** tab of **Claude Codex Cloner**.

> Part of the **Claude Codex Compendium v1.0** (`cccu/`). Build it with the compendium's `../build.sh`; it needs the compendium next to it (`../cloner`, `../installer`).
>
> **ClaudeInstall is merged into CCCU.** The live ISOs CCCU builds of a container now install themselves with
> **ClaudeInstall** (the compendium's Qt6 installer, `../installer`) instead of Calamares. CCCU carries the
> ClaudeInstall sources inside its own binary, builds ClaudeInstall in the container and sets it up for the clone -
> see [ClaudeInstall in CCCU](#claudeinstall-in-cccu).

> *Sailing The Seven Seas like Penguin's Eggs, Remastersys, Refracta, Systemback and father Knoppix!*

Clone the Linux system you already run into an **Incus system container** with its own sized (and optionally
compressed and immutable) storage, and boot straight into its KDE Plasma desktop on a minimal tty host.

## Contents

1. [Overview](#overview)
2. [Quick start](#quick-start)
3. [The interface](#the-interface)
4. [Build](#build)
5. [Desktop version (Qt6)](#desktop-version-qt6)
6. [Live ISOs and several containers](#live-isos-and-several-containers)
7. [ClaudeInstall in CCCU](#claudeinstall-in-cccu)
8. [Usage](#usage)
9. [The tty host system](#the-tty-host-system)
10. [Container storage](#container-storage)
11. [Supported systems](#supported-systems)
12. [Logging](#logging)
13. [Source layout](#source-layout)
14. [FAQ](#faq)
15. [Status and roadmap](#status-and-roadmap)

## Overview

CCCU takes a Linux system - a running machine, or an image made with `--export` - and turns it into an Incus
system container: a full systemd system that shares the host kernel (it is **not** a virtual machine, so GPU,
input and sound work natively). The container is hosted on a small tty system, either on a freshly partitioned
drive or on the machine you are running now. The configured user is logged in automatically into the
container's KDE Plasma session.

```
drive / running system
 +- EFI boot (FAT32, GRUB)                (drive install only)
 +- tty host --- Incus --- SDDM + cage
        +- container "desktop"            your cloned Linux: KDE Plasma and all your apps
               +- own btrfs storage       sized by you, compressed, optionally immutable
```

## Quick start

```bash
# 0. build and install, directly (dependencies are installed for you)
../build.sh   # from the compendium folder: ./build.sh

# 1. on the machine you want to clone
sudo cccu --export desktop.tar.zst

# 2. on the machine that will host the container
#    (or boot an Arch live ISO and choose "Install to a drive" to wipe a drive)
sudo cccu              # terminal version
                       # or the Containers tab of Claude Codex Cloner (application menu)
```

## The interface

This is the terminal version; the [desktop version](#desktop-version-qt6) offers the same modes and status boxes
in a window.

Running `cccu` without arguments opens the main menu. Each mode then shows a **settings screen**: every setting
is a row with a status box - **green `SET`** when it has a value, **red `NOT SET`** when a required value is
still missing. Type a row number to change it, `S` to start (only possible when nothing is red), `Q` to quit.

```
+- INSTALL TO A DRIVE ---------------------------------------------------------+
|  #  SETTING                VALUE                             STATUS          |
+------------------------------------------------------------------------------+
|  1  Target drive (WIPED)   /dev/sda                           [   SET   ]    |
|  2  Desktop image          -                                  [ NOT SET ]    |
|  3  tty host system        live ISO image (default)           [   SET   ]    |
|  ...                                                                         |
+------------------------------------------------------------------------------+
|  [ MISSING ]  1 required setting not set yet                                 |
+------------------------------------------------------------------------------+
```

Every copy shows what it does. A file copy (the desktop image, the installer, `resolv.conf`) prints the source,
the destination and the size, then a live line with percentage, bytes copied, speed and ETA, and ends with a
summary (size, time, average speed, how much stayed sparse). A folder copy prints the number of files and
folders and the total size first, and the same summary at the end. Cloning with rsync shows its overall progress
and the file/size statistics; extracting an image shows the percentage read. Other long silent commands (such as
writing an export image) report a real percentage, measured from the bytes the command has read.

While an installation runs, every step prints a header with a counter (`[3/12]`), an overall progress bar and
the elapsed time. A live status line (spinner, step time, total time) stays at the bottom and keeps moving even
while a long command is silent. Messages carry level tags - `[ INFO ]`, `[  OK  ]`, `[ WARN ]`, `[ FAIL ]` -
and what the commands print is shown in dark green. Colour and the live line switch off automatically when the
output is not a terminal or `NO_COLOR` is set.

**First boot after an installation.** The `cccu-firstboot` service runs on a real terminal (VT1), so the same
progress display appears live while the container is created; when it finishes, the login screen (SDDM, or the
container's own display manager in native mode) takes over the screen. The
output is also available with `journalctl -u cccu-firstboot -f`.

## Build

Requires GCC 14+ (or Clang 17+) with C++23 and CMake 3.25+. The terminal tool needs no other libraries.

Build it with the compendium's one build script, from the top folder:

```bash
./build.sh
```

It installs the dependencies, builds `cccu` and the ISO engine together with the other Claude Codex tools, and
installs them (`/usr/local/bin/cccu`, plus the polkit rule the window uses). Answer *yes* to "Build Claude Codex
CCU". The dependencies are the compiler, CMake 3.25+, `rsync`, `tar`, `zstd`, `squashfs-tools`, `parted`,
`dosfstools`, `e2fsprogs`, `btrfs-progs`, `curl` (the ISO boot files are fetched once and carried inside `cccu`), a
static libc for the ISO engine (`glibc-static` on Fedora, `glibc-devel-static` on openSUSE) and polkit. Incus itself
is installed later by CCCU when it sets up a host.

**By hand** (inside the compendium, it needs `../cloner` and `../installer`):

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo ./build/cccu
```

## Desktop version (Qt6)

The window is the **Containers** tab of **Claude Codex Cloner** (`../app`, *Claude Codex Cloner* in the application
menu). It offers every mode: tabs (*Install to a drive*, *Set up on this system*, *Export*, *Create ISO*, *Manage
containers*), the settings of the tab in a two-column grid, and progress, log and buttons at the bottom. Every
setting has the same status box as the terminal menu - green `SET`, red `NOT SET`, grey `OPTIONAL` - and **Start**
unlocks only when nothing required is red. A progress bar follows the steps, and the log shows the output live.

* It is a front-end: it runs the `cccu` tool with your settings, so both versions behave identically and write the
  same logs.
* The window runs as your user. Each task runs `cccu` through `pkexec`: the desktop's password dialog, kept for a
  few minutes (polkit rule `org.claudemods.claude-codex.ccu`).
* The user and root passwords are passed to the tool through stdin (`--secrets-stdin`) and set with `chpasswd`;
  they never appear on a command line, in the environment or in the log.
* The container list is read as your user when you are in the `incus-admin` group; otherwise *Refresh* asks for the
  password.
* Before a drive install it asks you to type the device path to confirm the wipe. **Export** always asks where to
  save the image and verifies that the file exists afterwards.
* A finished ISO is written to a USB stick on the window's *Backup > Write USB* (one USB writer for the suite);
  `cccu iso-usb` does the same in a terminal.
* **Dry run** shows what would happen without changing anything. **Cancel** stops the running operation (it asks
  for the password once more, since the tool runs as root).
## Live ISOs and several containers

**Create ISO** (menu 4, the Containers tab's *Create ISO*) turns the system inside a container into a bootable live ISO. The
claudebackup engine is the **Claude Codex Compendium's cloner** (`../cloner/src`, the same code as
`claudebackup-linux`); it is built as `cccu-iso-engine` and carried inside the `cccu` binary, together with the boot
files and the compendium's provisioning bundle (its `build.sh` and the ClaudeInstall sources). The engine itself is
never compiled in the container. `cccu iso` pushes the engine into the container and runs it **there**, so the ISO is
made with the container's own kernel, package manager and tools: Arch, Debian, Fedora and openSUSE families (the
ones CCCU can clone). CCCU installs the ISO tools (xorriso, squashfs, bootloaders) and, when the clone
has none, a kernel inside the container first. With the installer on, ClaudeInstall is built inside the container the
first time (the compendium's `build.sh --container-installer`), configured to install the clone and given an
"Install" Desktop shortcut; the finished ISO is copied out to your folder. Progress of the
squashfs / erofs step is shown live like every other step (a percentage in the window, the live line on the console). The
engine is linked statically, so it starts in any container; on Fedora/openSUSE building it needs `glibc-static`
(`glibc-devel-static`).

```bash
sudo cccu iso --name desktop --iso-out ~/isos --iso-level 6 --iso-format squashfs
sudo cccu iso --name desktop --iso-out ~/isos --on-reboot      # set up now, build on the next boot (the only way the GUI builds)
sudo cccu iso --name desktop --iso-out ~/isos --on-console     # build now, progress on VT1 like the first-boot service
sudo cccu iso-usb --iso-file ~/isos/claudecodexbackup-arch-2026-10-30_1200.iso --drive /dev/sdX
```

The service shows the animated squashfs/erofs bar, then a green "Creating the ISO" bar for the ISO stage. When the
ISO is saved, the PC reboots after a 5 second countdown (a failed build does not reboot: it shows the error, waits
30 seconds and goes on to the login screen).

The ISO is saved in `~/Downloads` unless you choose another folder: the window asks where to save it every time
(starting in Downloads), the terminal menu and `--iso-out` default to it.

Settings: ISO name and tag, image format (squashfs / erofs), zstd level, RAM boot entry, package-cache cleaning,
ClaudeInstall installer (ext4 or btrfs zstd:22). The ISO needs free space inside the container of roughly twice its size.

**Containers** (menu 5, the Containers tab's *Manage containers*): keep several containers and choose which one the host session shows.

```bash
sudo cccu containers
sudo cccu duplicate desktop desktop-test --switch   # full copy (incus copy), then make it the active one
sudo cccu switch desktop                            # "Container Plasma" now shows this one; the previous is stopped
```

The active container is recorded in `/etc/cccu/active`, which the host's `cccu-session` reads at login.
Immutable containers cannot be duplicated yet.

## ClaudeInstall in CCCU

CCCU used to put **Calamares** on its ISOs: it carried `calamares-files.zip`, `claudemods.zip` and a copy of
claudebackup's `build.sh` (`provision.sh`) that installed Calamares inside the container. All of that is gone. ClaudeInstall
is merged in instead, so an ISO made with CCCU installs exactly like an ISO made with `claudebackup-linux` on the host.

**What `cccu` carries now** (built into the binary by CMake):

| Inside `cccu` | What it is |
|---|---|
| `cccu-iso-engine` | the compendium's cloner engine (`../cloner/src`), linked statically |
| `compendium-provision.tar.gz` | the compendium's `build.sh` + the ClaudeInstall sources (`../installer`, without `needed-files/`) - packed at build time |
| `bootloaders.tar.gz` | BIOS/UEFI boot files for the non-Arch families (`resources/`, fetched once by `build.sh`) |

**What `cccu iso` does in the container** (once per container and settings; a marker in `/var/lib/cccu-iso` skips it
next time):

```
cccu iso
 +- push the engine          -> /usr/local/bin/cccu-iso-engine
 +- push + unpack the bundle -> /opt/cccu-iso/{build.sh, installer/, resources/}
 +- build.sh --root-phase <family> 1 0 0
 |     ISO tools (xorriso, squashfs, mtools ...), bootloaders, pkexec,
 |     GRUB / efibootmgr / rsync and ClaudeInstall's build dependencies
 +- build.sh --container-installer <btrfs> <erofs>
 |     builds + installs ClaudeInstall (installer/build.sh install --clone --gui)
 |     writes /etc/claudeinstall for the clone (unpackfs = the ISO's image, no users page,
 |     kernel fix-up, initramfs, GRUB), the claudebackup-install launcher and the
 |     "Install <distro>" Desktop shortcut for every user of the container
 +- remove /opt/cccu-iso, then run the engine (CBL_KERNEL = the container's kernel)
```

The ClaudeInstall set-up is the same one the compendium writes on a host (`build.sh --configure-installer`):

| Family | ClaudeInstall unpacks | Initramfs | GRUB |
|---|---|---|---|
| Arch | `/run/arch/bootmnt/LiveOS/rootfs.img` | `initcpiocfg` + `initcpio` (preset of the container's kernel) | `grub` |
| Debian / Ubuntu | `/run/live/medium/live/filesystem.squashfs` | `initramfs` | `grub` |
| Fedora | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` + boot-entry fix-up | `grub2` |
| openSUSE | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` | `grub2` |

Notes:

* **The first ISO of a container takes longer**: ClaudeInstall is compiled inside it (with the container's own Qt6
  and compiler, so it matches that system). Later ISOs of the same container reuse it.
* Inside a container `uname -r` names the HOST's kernel, so CCCU passes the container's own kernel as `CBL_KERNEL`
  to both the provisioning (Arch's initcpio preset) and the engine.
* If ClaudeInstall cannot be built in the container, the ISO is still made - just without an installer - and CCCU
  says so. Choose *No installer* (`--no-installer`) to skip it altogether; `--installer-btrfs` makes btrfs zstd:22
  the installer's default filesystem.
* In the live ISO: double-click **Install &lt;distro&gt;** on the Desktop (or run `claudebackup-install`). It asks
  for the password, matches the unpack settings to the ISO's real image type (squashfs / erofs) and starts
  ClaudeInstall. Its log is `/tmp/claudecodexbackup-install-<uid>.log`.

## Display modes (the toggle)

How the container's desktop reaches the screen is one choice, `--display` in the terminal tool and the *Display* row
(a toggle) in the window and the menu:

| Mode | What it does | For |
|---|---|---|
| `session` (default, as before) | the HOST runs SDDM; its "Container Plasma" session shows the container's desktop through the kiosk compositor cage | a normal machine with a GPU |
| `server` | the container runs its own login on the screen (VT1) and Plasma Wayland draws in software (`KWIN_COMPOSE=Q`, `LIBGL_ALWAYS_SOFTWARE=1`); no cage, no host login screen, the host stays on text consoles | a machine WITHOUT a usable GPU, e.g. a rented server whose only screen is the BMC / iKVM chip |
| `native` | like `server`, but with the GPU | a machine with a GPU that should not use cage |

```bash
sudo cccu setup --image desktop.tar.zst --user alice --size 60 --display server --yes
```

The mode is set when the container is created. In `server` and `native` mode switch the screen to the container with
`chvt 1` (as root) or Ctrl+Alt+F1; the host keeps tty2. Server mode has not been tested on real hardware yet.

## Usage

| Menu entry | What it does |
|---|---|
| 1. Install to a drive | Wipes a drive: GPT, EFI + system partition (ext4 or btrfs), GRUB, Incus, and creates the container on the first boot |
| 2. Set up on this system | Installs what the host needs (Incus, `btrfs-progs`, SDDM, `cage`, GPU and audio packages) with the system's own package manager, creates the user and the container. Reboot afterwards: SDDM becomes the display manager |
| 3. Export this machine | Writes the running system as a `.tar.zst` image to clone elsewhere |
| 4. Create a live ISO | Builds a bootable live ISO of the container's system, inside the container (see above) |
| 5. Containers | Lists the containers, duplicates one, switches the one the host session shows |

Everything can also be given on the command line; `--yes` runs unattended without the menu. `cccu install ...`
is menu 1, `cccu setup ...` is menu 2.

```bash
sudo cccu install --drive /dev/sda --image desktop.tar.zst --user alice --size 60 --compress zstd:22 --yes
sudo cccu setup --clone-host --user alice --size 60 --yes
```

Unattended runs set passwords from the environment (`CCCU_USER_PASSWORD`, `CCCU_ROOT_PASSWORD`); without them
the tool asks on the terminal.

| Option | Description |
|---|---|
| `--drive <dev>` | drive to wipe (install mode) |
| `--image <file>` | system image (`.tar` `.tar.zst` `.tar.xz` `.tar.gz`, squashfs, erofs) |
| `--clone-host` | clone the running machine instead of an image |
| `--hostname <name>` | hostname of the new tty system (default `cccu`) |
| `--user <name>` | account on the tty host: created if missing, added to `incus-admin`, logged in automatically |
| `--container-user <name>` | user inside the cloned system whose desktop runs (default: same as `--user`) |
| `--size <N>[GiB]` | container disk size - there is no default |
| `--compress <level>` | `zstd:22` `zstd:15` `zstd:9` `zstd:3` `zlib:9` `lzo` `none` |
| `--immutable` | mount the container's system read-only |
| `--fs ext4\|btrfs` | host system filesystem |
| `--keep-kernel` `--keep-caches` `--no-start` `--dry-run` `--yes` | see `cccu --help` |
| `iso --iso-out <dir>` | build a live ISO (`--iso-name` `--iso-level` `--iso-format` `--iso-tag` `--no-ram` `--keep-pkg-cache` `--no-installer` `--installer-btrfs` `--on-console` `--on-reboot`) |
| `iso-usb --iso-file <f> --drive <dev>` | write an ISO to a drive |
| `containers` `duplicate <src> <new> [--switch]` `switch <name>` `start` `stop` | manage the containers |

### Install next to another system (existing partitions)

`--drive` wipes a whole disk. To keep another system on the same disk (say Debian on p1/p2), give two EXISTING
partitions instead - the window has *Install to > Two existing partitions*, the terminal menu has the same choice:

```bash
sudo cccu install --boot-part /dev/nvme0n1p3 --root-part /dev/nvme0n1p4 --image desktop.tar.zst \
     --host-iso my-arch-clone.iso --user alice --size 60 --fs btrfs --yes
```

Only those two partitions are formatted: the boot one as FAT32 (it is marked as the EFI system partition; `--keep-boot`
leaves it as it is when it is FAT32 already) and the root one as `--fs`. The partition table and everything else on the
disk stay untouched, GRUB goes to the boot partition with its own firmware entry named `CCCU` (the other system's entry
stays), and mounted partitions, the running system and partitions that are too small are refused. To get the other
system into the new GRUB menu, install `os-prober` in the new system and run `grub-mkconfig`.

CCCU can run from an installed Debian, Ubuntu, Fedora or openSUSE as well as from a live ISO: it installs the tools
it needs (`parted`, `dosfstools`, `squashfs-tools` ...) with the system's own package manager. The tty host can come
from an ISO made by claudebackup (its `LiveOS/rootfs.img` is found and unpacked like any other live ISO).

## The tty host system

A drive install needs a small system to run Incus on. "tty host system" in the settings chooses where it comes
from:

| Source | Option | What happens |
|---|---|---|
| Live ISO you booted (default) | - | the root image of the running live ISO is installed |
| Clone of the running system | `--clone-system` | the system you booted is copied with rsync |
| System image file | `--host-image <file>` | a squashfs / erofs / tar image is extracted |
| **ISO file** | `--host-iso <file>` | the ISO is mounted, its root file system found and extracted |
| **Base package list** | `--host-bootstrap <list>` | a fresh base system is downloaded and installed from a built-in list |

**ISO files.** CCCU finds the root file system by itself: Arch (`airootfs.sfs` / `.erofs`), Debian live
(`live/filesystem.squashfs`), Ubuntu and Mint (`casper/filesystem.squashfs`, also the layered `minimal` images),
Fedora and relatives (`LiveOS/squashfs.img` / `rootfs.img`), or else the largest squashfs/erofs image on the ISO.
Because a live system boots by searching for its ISO, CCCU afterwards removes the live-boot parts (archiso,
casper, live-boot, dracut-live, the installer) and installs a kernel if the ISO has none, so the system boots
from the drive. Netinstall ISOs that only download their system have no root file system: use a base list.

**Base package lists.** You choose one; nobody types a package list. The list is built in and shown in the
menu (and as a tooltip in the GUI):

| List | Installed with | Packages |
|---|---|---|
| `arch` | `pacstrap` | `base linux linux-firmware networkmanager sudo nano` |
| `debian` | `debootstrap`, Debian stable | kernel, systemd, dbus, udev, locales, network-manager, sudo, nano, initramfs-tools, ... |
| `ubuntu` | `debootstrap`, Ubuntu 24.04 LTS | the same, with the Ubuntu kernel |
| `fedora` | `dnf --installroot`, same release as the live system | `@core kernel dracut-config-generic NetworkManager sudo nano` |

The tool needed (`pacstrap`, `debootstrap`) is installed in the live system when missing, and the packages are
verified with the distribution's signing keys (they are never skipped; if the keys are missing, CCCU stops and
says which package provides them - on Arch the Debian/Ubuntu keyrings are in the AUR). The Fedora list needs a
Fedora live system. `--host-packages <file>` adds your own extra packages (one per line) to a list. After the base
system is in place, CCCU installs Incus, the bootloader and the rest of what the host needs, exactly as for any
other source.

```bash
sudo cccu install --drive /dev/sda --image desktop.tar.zst --host-iso ~/Downloads/debian-live.iso --user alice --size 60 --yes
sudo cccu install --drive /dev/sda --image desktop.tar.zst --host-bootstrap debian --user alice --size 60 --yes
```

## Container storage

The container gets its **own btrfs storage pool** (loop-backed, so the size is enforced). When you choose the
size, CCCU reads the image and suggests one:

```
Your image is 4.2 GiB (zstd-compressed tar); unpacked it needs roughly 12.6 GiB.
Leave room for updates and new apps: about 19 GiB or more is a sensible size.
```

| Compression | Notes |
|---|---|
| `zstd:22` | strongest (default in the menu) |
| `zstd:15`, `zstd:9`, `zstd:3` | strong, balanced, fast |
| `zlib:9`, `lzo` | alternatives |
| `none` | no compression |

With **immutable** enabled the container's root disk is read-only. `/etc`, `/var`, `/home`, `/root`, `/opt`,
`/srv` and `/usr/local` live on their own volumes and stay writable. To update the system:

```bash
incus config device set desktop root readonly=false   # restart, update, then set it back to true
```

## Supported systems

The installer needs systemd inside the tty host and inside the container, and detects the system from
`/etc/os-release`; an unsupported one is refused with the reason before anything is changed.

| Family | Systems | Package manager | Status |
|---|---|---|---|
| Arch | Arch, CachyOS, EndeavourOS, Garuda, ArcoLinux, Manjaro | `pacman` | primary target |
| Debian | Debian, Ubuntu, Kubuntu, KDE neon, Linux Mint, Pop!_OS, MX Linux (systemd boot only) | `apt` | implemented, needs field testing |
| Fedora | Fedora, CentOS, AlmaLinux, Rocky Linux, RHEL, Nobara | `dnf` | implemented, needs field testing |
| openSUSE | Leap, Tumbleweed | `zypper` | implemented, needs field testing |
| Not supported | NixOS, Gentoo, Slackware, Void, Devuan, KDE Linux, Artix | - | no systemd, or no installable package tree |

## Logging

Every run writes a structured log to `/var/log/cccu/`:

| File | Content |
|---|---|
| `install.log` | the drive installation |
| `container.log` | container creation (including the first-boot run) and diagnostics |
| `export.log` | exports |
| `iso.log` | ISO builds (including building ClaudeInstall in the container) and writing an ISO to USB |

Inside the container the engine also logs to `/var/log/claudecodexbackup.log`, and ClaudeInstall's own build
errors (if any) are in `/opt/cccu-iso/installer/build/errors.log` until the run finishes.

One record per line - `date time LEVEL text`:

```
============================================================================
 Claude Codex CCU  -  Container Creator Utility  -  Compendium v1.0 01-11-2026
 started : 2026-10-29 14:03:11.482
 host    : live
 command : cccu install --drive /dev/sda --image desktop.tar.zst --yes
============================================================================
2026-10-29 14:03:12.044 STEP  [2/12] Partitioning and formatting /dev/sda
2026-10-29 14:03:12.051 CMD   #5 wipefs -a /dev/sda
2026-10-29 14:03:12.402 OUT   #5 | /dev/sda: 2 bytes were erased at offset 0x00000200
2026-10-29 14:03:12.440 DONE  #5 exit=0 duration=0.39s
2026-10-29 14:03:12.441 FAIL  #6 exit=1 duration=0.02s
2026-10-29 14:41:50.007 SUMMARY result=failed exit=1 steps=7 commands=48 failed_commands=1 warnings=2 errors=1 duration=2318.5s
```

Levels: `INFO` `OK` `WARN` `ERROR` (what was shown), `STEP` (a pipeline step), `CMD`/`OUT`/`DONE`/`FAIL` (a command,
its output, its result - tied together by the command number `#n`), and a final `SUMMARY`. Colour codes and
progress-bar redraws are stripped from the log. Quick checks:

```bash
grep -E ' (FAIL|ERROR|WARN) ' /var/log/cccu/install.log
grep ' #6 ' /var/log/cccu/install.log        # everything about command 6
```

## Source layout

`build.sh`: installs the dependencies and builds everything. `packaging/`: application-menu entry and icon.

`gui/`: the Qt6 desktop version (`mainwindow` window, settings pages and live log; `main`).

`src/`: `bootstrap` ISO extraction and the built-in base package lists, `main` command line and run control, `menu` main menu and settings screens, `util` terminal UI, progress
and logging, `proc` command execution, `hostinstall` drive installation, `drive` partitioning, `installer`
container pipeline and running-system setup, `distro` family detection and per-distro commands, `incus` Incus
wrapper, `profiles` container profile, `sources` image/clone/export, `rootfs` system adaptation, `config` options,
`iso` / `iso_embed` ISO creation inside a container, `containers` duplicate and switch.

From the compendium (not copied into `cccu/`):

| Path | Used for |
|---|---|
| `../cloner/src`, `../cloner/resources` | the ISO engine and its Arch boot tree (built as `cccu-iso-engine`) |
| `../installer` | ClaudeInstall, packed into the provisioning bundle and built in the container |
| `../build.sh` | the provisioning: `--root-phase` (ISO tools) and `--container-installer` (ClaudeInstall) |

`resources/bootloaders.tar.gz` is fetched by `build.sh`. There is no `src/remaster/` or `provision.sh` any more.

## FAQ

**Is the container a virtual machine?** No. It is an Incus system container that shares the host kernel.

**Is there a login screen?** No. The configured user is logged in automatically; there is no option to turn it off.

**Do I create a user for the container?** No - the container is a clone, so its users already exist and the
settings screen lists them. The tty host account can have a different name.

**Do I have to give a size?** Yes. There is no default; the settings screen suggests one from your image.

**Where are the logs?** `/var/log/cccu/` - see [Logging](#logging).

**Which installer is on the ISOs?** ClaudeInstall, built inside the container - see
[ClaudeInstall in CCCU](#claudeinstall-in-cccu). Calamares is no longer used anywhere.

**Why does the first "Create ISO" take so long?** It compiles ClaudeInstall in the container once. Turn the installer
off (`--no-installer`) for a quick ISO without it.

## Status and roadmap

Experimental. The Arch path is the primary target. The Debian, Fedora and openSUSE paths, sized/compressed/
immutable storage and the running-system mode are implemented but not yet field-tested - please report what
breaks.

ClaudeInstall in the container's ISOs (replacing Calamares) is new in the compendium and not field-tested yet.

Planned: Docker containers from a cloned system, bootstrapping a base for other distros without a ready-made
image, field-testing of the non-Arch paths, non-systemd inits.

---

<div align="center">

**[ Support Me ](https://www.paypal.com/paypalme/claudemods?country.x=GB&locale)** · [claudemodsreloaded.co.uk](https://www.claudemodsreloaded.co.uk)

</div>
