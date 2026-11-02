<p align="center">
<img src="https://i.postimg.cc/JhMRf2RZ/claudemods-03-17-2025.gif">

<div align="center">

  <a href="https://www.linux.org" target="_blank"><img src="https://img.shields.io/badge/OS-Linux-e06c75?style=for-the-badge&logo=linux" /></a>
  <a href="https://isocpp.org" target="_blank"><img src="https://img.shields.io/badge/LANG-C++-00599C?style=for-the-badge&logo=cplusplus" /></a>
  <a href="https://www.qt.io" target="_blank"><img src="https://img.shields.io/badge/GUI-Qt6-41CD52?style=for-the-badge&logo=qt&logoColor=white" /></a>
  <a href="https://linuxcontainers.org/incus/" target="_blank"><img src="https://img.shields.io/badge/CONTAINERS-Incus-3b82f6?style=for-the-badge&logo=linuxcontainers&logoColor=white" /></a>
  <a href="https://kde.org/plasma-desktop/" target="_blank"><img src="https://img.shields.io/badge/DESKTOP-KDE_Plasma-1d99f3?style=for-the-badge&logo=kde&logoColor=white" /></a>

  <a href="https://archlinux.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Arch-56b6c2?style=for-the-badge&logo=arch-linux" /></a>
  <a href="https://ubuntu.com/" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Ubuntu-E95420?style=for-the-badge&logo=Ubuntu" /></a>
  <a href="https://www.debian.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Debian-CE0058?style=for-the-badge&logo=Debian" /></a>
  <a href="https://getfedora.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Fedora-51A2DA?style=for-the-badge&logo=fedora" /></a>
  <a href="https://www.opensuse.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-openSUSE-73BA25?style=for-the-badge&logo=opensuse" /></a>
  <a href="https://linuxmint.com" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Linux_Mint-87CF3E?style=for-the-badge&logo=linuxmint&logoColor=white" /></a>
  <a href="https://alpinelinux.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Alpine-0D597F?style=for-the-badge&logo=alpine-linux" /></a>
  <a href="https://www.gentoo.org" target="_blank"><img src="https://img.shields.io/badge/DISTRO-Gentoo-54487A?style=for-the-badge&logo=gentoo" /></a>

  <a href="https://claude.com/claude-code" target="_blank">
    <img src="https://img.shields.io/badge/Built_Using-Claude_Code-D97757?style=for-the-badge&logo=claude&logoColor=white" alt="Built Using Claude Code">
  </a>

  ## [ Guide ](#guide)

  ## [ Support Me ](https://www.paypal.com/paypalme/claudemods?country.x=GB&locale)

</div>

<div align="center">

  [![Ko-Fi](https://img.shields.io/badge/Ko--fi-F16061?style=for-the-badge&label=claudemods&color=3399FF&Linux&logo=ko-fi&logoColor=white)](https://ko-fi.com/claudemods)
  [![GitHub Sponsors](https://img.shields.io/badge/sponsor-30363D?style=for-the-badge&label=claudemods&color=A836FF&logo=GitHub-Sponsors&logoColor=#white)](https://github.com/sponsors/claudemods)

</div>

<div align="center">

# Claude Codex — Compendium v1.0

**Version:** Compendium v1.0 · **Date:** 01-11-2026

<p align="center"> Sailing the 7 seas like Penguin's Eggs, Remastersys, Refracta, Systemback and father Knoppix! </p>

![C++](https://img.shields.io/badge/C++-17%20%2F%2023-blue) ![Qt](https://img.shields.io/badge/Qt-6-green) ![Status](https://img.shields.io/badge/status-beta-orange) ![License](https://img.shields.io/badge/license-MIT-green)

**Clone your running Linux into a bootable live ISO or an Incus container — then install it back to disk.**<br>
One window, one command, one build script, one updater.

</div>

---

## Contents

- [What it is](#what-it-is)
- [The tools](#the-tools)
- [Quick start](#quick-start)
- [Guide](#guide)
  - [Claude Codex Cloner — the window](#claude-codex-cloner--the-window)
  - [Build a live ISO](#build-a-live-iso)
  - [Build the ISO from init (on the next boot)](#build-the-iso-from-init-on-the-next-boot)
  - [Write a USB stick](#write-a-usb-stick)
  - [Install the clone to a disk](#install-the-clone-to-a-disk)
  - [Containers (Claude Codex CCU)](#containers-claude-codex-ccu)
  - [Updates](#updates)
  - [Commands](#commands)
- [build.sh in detail](#buildsh-in-detail)
- [Supported distributions](#supported-distributions)
- [How the pieces fit](#how-the-pieces-fit)
- [How four projects became one](#how-four-projects-became-one)
- [Troubleshooting and logs](#troubleshooting-and-logs)
- [Requirements](#requirements)

---

## What it is

```
                        ┌──────────────────── Claude Codex Cloner (one window) ────────────────────┐
                        │   Backup  ·  Containers  ·  Updates  ·  Guide                            │
                        └───────────┬──────────────────────────┬────────────────────────────────────┘
                                    │                          │
        your running system ── Claude Codex Backup ──▶ live ISO ──▶ USB stick ──▶ Claude Codex Install ──▶ new disk
                    │                                     ▲
                    └── Claude Codex CCU ──▶ Incus container ─┘  (Create ISO: the same engine, inside the container)
```

- The ISO is a **full clone**: your users, passwords, settings and `/home` go in exactly as they
  are. No live account is added, and the installer has no users page.
- **One engine** (`cloner/src`) builds every ISO — on the host and inside containers.
- **One installer** (Claude Codex Install, formerly ClaudeInstall) replaces Calamares everywhere.
- **One window** (Claude Codex Cloner), **one command** (`claude-codex`), **one build script**
  (`./build.sh`) and **one updater** (`claude-codex update`).

## The tools

Every tool shows **Compendium v1.0 01-11-2026** in its window, banner, About box or `--help`.

| Tool | What it does | Start it | Folder |
|---|---|---|---|
| **Claude Codex Cloner** | the window: Backup, Containers, Updates and Guide in tabs | `claude-codex-cloner`, or *Claude Codex Cloner* in the applications menu | `app/` |
| **Claude Codex Backup** | clones the running system into a bootable live ISO, writes USB sticks; also a terminal menu and batch commands | the **Backup** tab · `claude-codex backup` (`claudebackup-linux`) | `cloner/` |
| **Claude Codex Install** | the installer: puts the clone on a disk (or installs a new system, or claudemods distros online) | *Install &lt;distro&gt;* on the live desktop · `claude-codex install` (`claudeinstall`, `claudeinstall-tui`) | `installer/` |
| **Claude Codex CCU** | the Container Creator Utility: clones the system into an Incus container, installs that setup to a drive, exports images, ISOs of containers | the **Containers** tab · `claude-codex ccu` (`cccu`) | `cccu/` |
| **claude-codex** | the one command: a menu of everything, status, update | `claude-codex` | `claude-codex` |
| **build.sh** | builds, installs and configures all of the above | `./build.sh` | `build.sh` |

The single commands still work, plus the `claude-codex-backup`, `claude-codex-install`,
`claude-codex-install-tui` and `claude-codex-ccu` links. Internal names (`/etc/claudeinstall`,
`~/.config/claudecodexbackup`, library and package names) are unchanged, so existing setups keep
working.

## Quick start

```bash
git clone https://github.com/claudemods/claude-codex.git
```

```bash
cd claude-codex && ./build.sh
```

1. Run `./build.sh` as **your normal user**. It asks a few questions (Enter = the default) and for
   the root password when it needs it.
2. It installs the packages, **builds everything** (the installer alone is about 1,000 files),
   then **installs everything** — every step is shown live.
3. The **Complete** screen lists each tool (*built + installed*, *FAILED* with its log, or
   *skipped*) and how to start it. Nothing is started for you.

Then open **Claude Codex Cloner** from the applications menu — or type `claude-codex`.

Later, to update everything: the **Updates** tab, or

```bash
claude-codex update
```

---

## Guide

### Claude Codex Cloner — the window

One window for the whole suite. It opens maximised (**F11**: full screen). Run it as your user: it asks
for the administrator password **once**, when it opens, and every task then runs without asking again
(cancel it, and the next task that needs root asks). Nothing runs before you press its button. Every task
log has **Full Screen Log**, which shows it live in its own full-screen window. The header shows whether
the window is unlocked, whether CCU is installed and whether an update is available; the Backup sidebar
shows your distro and the size, used and free space of the system (df -h).

| Tab | What is on it |
|---|---|
| **Backup** | *Overview* (what is ready, with fix buttons) · *Build ISO* · *Settings* · *Edit Configs* · *Write USB* · *Install* · *Maintenance* |
| **Containers** | Claude Codex CCU: *Install to a drive* · *Set up on this system* · *Export* · *Create ISO* · *Manage containers* |
| **Updates** | every tool with its state, the latest release on GitHub, *Update Everything* with live progress |
| **Guide** | this guide, and the cloner's notes for your distro |

Each feature lives in **one** place: USB sticks are written on *Backup > Write USB* (also the
ISOs made on the Containers tab — *Open Write USB* takes you there), and updates happen on
*Updates* only.

### Build a live ISO

**In the window — Backup tab:**

1. **Overview** — fix anything marked red: a missing tool, the boot files, free space.
2. **Settings** — ISO name, work / output / clone folders, squashfs or erofs, zstd level, a
   copy-to-RAM boot entry, leaving package caches out. The terminal menu reads the same settings.
3. **Edit Configs** (optional) — the ISO's GRUB menu, boot text and initramfs config, and
   Claude Codex Install's configuration. Files open directly; *Save* writes them (installer files
   as root, with a `.bak`), *Reset* goes back to the generated version.
4. **Build ISO** — every step with live progress, a colour log, elapsed time and *Cancel* (it
   stops the running program and removes every mount).
5. The ISO is saved in the output folder (default `~/Downloads`).

**In a terminal:** `claude-codex backup` opens the menu (*Create Bootable ISO*). For scripts,
`claudebackup-linux --help-batch` lists the batch commands, e.g. `--status`, `--build-iso`,
`--set level=5`, `--write-usb ISO DEVICE`.

### Build the ISO from init (on the next boot)

Like CCU's ISO build on reboot, Backup can build the ISO **at boot, before the login screen**.
Nothing else is running then, so no file changes while the system is copied — a cleaner clone.

| Where | How |
|---|---|
| Window | *Backup > Build ISO* > **Build On Next Boot** (the button then reads **Cancel Boot Build**) |
| Terminal menu | **Create Bootable ISO On Next Boot (from init)** — select it again to cancel |
| Command line | `sudo claudebackup-linux --build-on-boot` / `--cancel-boot-build` |

On the next boot the one-shot `claude-codex-backup-boot.service` takes the console (VT1) and builds
the ISO with your saved settings and live progress; the login screen waits for it. On success the
PC reboots after a 5-second countdown; on failure the error stays 30 seconds, then the login screen
starts (log: `/var/log/claudecodexbackup.log`). The service switches itself off *before* building
and its files are left out of the image, so it runs exactly once. Needs systemd (not on Alpine or
OpenRC Gentoo).

### Write a USB stick

*Backup > Write USB*: pick the ISO (the newest ones of the output folder are listed, *Browse...* for
any other), pick the drive (removable drives first; the running system's disks are refused), type
the device name to confirm. The whole drive is erased. Test a new ISO in a virtual machine (UEFI)
before you rely on it.

### Install the clone to a disk

Boot the live ISO. Its desktop has **Install &lt;distro&gt;** (`claudebackup-install` in a
terminal), which starts **Claude Codex Install** as root:

1. Language and keyboard
2. Partitions — erase a disk, or your own layout (ext4, btrfs zstd:22, xfs; optional LUKS). The
   default file system is the one you chose in build.sh
3. Summary, then install: the image is unpacked, then fstab, machine-id, the kernel fix-up,
   initramfs and GRUB are done for the new disk
4. Reboot — with the accounts of the cloned system

With the **terminal** interface only, the shortcut opens `claudeinstall-tui` in a terminal.
**Alpine** ISOs use the cloner's own *Install Cloned System To Disk* (*Backup > Install*).

Other installer modes (build.sh): **new** — a normal distro installer with users and packages;
**distro** — the claudemods distributions online (pacstrap + profile, Arch).

### Containers (Claude Codex CCU)

CCU runs your system as an Incus system container with its own btrfs storage. The **Display**
setting decides how it reaches the screen:

| Display | For | How |
|---|---|---|
| **Desktop session** | a PC with a GPU | the host's SDDM offers a *Container Plasma* session; it shows the container's desktop through the cage compositor, with the GPU |
| **Native** | a PC with a GPU | the container's own login screen on the console |
| **Server** | a machine without a usable GPU (rented server, BMC/iKVM screen) | Plasma Wayland in software, plus remote access: **KRDP** (RDP, port 3389) and **SSH** (host on 22, container on 2222) — the setup steps are printed when it finishes |

| Containers tab | What it does |
|---|---|
| **Install to a drive** | wipes a drive (or uses two existing partitions next to another system): EFI partition, a small tty host with Incus, and your desktop container, created on the first boot with live progress |
| **Set up on this system** | installs Incus, SDDM, cage and the rest with your package manager, then creates the container; reboot afterwards |
| **Export** | writes this system as an image (`.tar.zst` ...) to clone on another machine |
| **Create ISO** | a live ISO of a container's system, built *inside* the container on the next boot, with Claude Codex Install on it — then *Open Write USB* |
| **Manage containers** | start, stop, *Switch to it* (the one the host session shows), *Duplicate* |

Every field shows **SET** or **NOT SET**; *Start* is enabled once nothing required is missing, and
*Dry run* prints what would happen. The passwords you type go to CCU through stdin, never the
command line. Terminal: `claude-codex ccu` (the menu) or `sudo cccu --help`. All modes:
[cccu/README.md](cccu/README.md).

### Updates

The window checks GitHub when it opens (the header shows the result). **Updates > Update
Everything** downloads the latest release and runs its `build.sh` with **the answers of your last
build**, so every tool is rebuilt and reinstalled together; a tool that does not compile keeps its
old version. *Rebuild even when it is up to date* repairs an installation.

```bash
claude-codex update --check
```

```bash
claude-codex update
```

```bash
claude-codex update --force
```

Your answers are kept in `/etc/claude-codex/build.conf`; run `./build.sh` again to change them.
The source the updater builds is kept in `/var/cache/claude-codex/source`. The cloner's own
*Update* (menu) and `--update` run the same suite updater.

### Commands

| Command | What it does |
|---|---|
| `claude-codex` | a menu of everything |
| `claude-codex cloner` | the window |
| `claude-codex backup [args]` | the cloner: its menu, or batch commands (`--help-batch`) |
| `claude-codex ccu [args]` | Claude Codex CCU, as root |
| `claude-codex install [--tui]` | Claude Codex Install, as root |
| `claude-codex status` | what is installed, the build choices, the source |
| `claude-codex update [--check] [--force] [--branch NAME]` | update everything |
| `claude-codex help` / `version` | |

---

## build.sh in detail

**The questions** (Enter = the default):

| Question | Choices |
|---|---|
| Image type of the ISO | squashfs (default) / erofs |
| Build Claude Codex Install? | yes (default) / no |
| What it installs | **the cloned system** (default) / **a new system** (users, packages) / **claudemods distributions online** (Arch only) |
| Its interface | **GUI** (default) / **TUI** (`claudeinstall-tui`) / **both** |
| Default filesystem of the installed system (clone mode) | ext4 (default) / btrfs zstd:22 |
| Build Claude Codex Cloner (the window)? | yes (default) / no |
| Build Claude Codex CCU? | yes (default) / no — Arch, Debian, Fedora and openSUSE only |

**What it does then:**

1. **Packages** (root) — the build and ISO tools for your distro, plus every dependency of each
   tool you picked (Qt 6, KPMcore, KF6, yaml-cpp, polkit-qt6, GRUB, efibootmgr, the file-system
   tools, ckbcomp on Arch ...)
2. **Build** (as you), all of it before anything is installed, each part with a banner:
   `BUILD 1/4` the cloner · `BUILD 2/4` the window · `BUILD 3/4` Claude Codex Install (about 1,000
   files; every error in `installer/build/errors.log`) · `BUILD 4/4` Claude Codex CCU (GCC 14+)
3. **Install** (root, one step) — everything that compiled. Clone mode configures
   `/etc/claudeinstall` to install the clone (on Arch with the claudemods preset via
   `claudeconfig`); distro mode puts the profiles, images and `needed-files` in
   `/usr/share/claudeinstall/`. Then `claude-codex`, its polkit rule, and your answers in
   `/etc/claude-codex/build.conf`
4. **Desktop shortcut** (clone mode) — *Install &lt;distro&gt;*; it is part of the clone, so the
   live ISO's desktop has it too
5. **Complete** — the table of results and how to start each tool. Nothing is started; if a tool
   you picked failed, build.sh exits with an error

**Options** skip the questions: `--yes`, `--erofs`, `--btrfs` / `--ext4`, `--no-installer`,
`--installer-mode=clone|new|distro`, `--installer-ui=gui|tui|both`, `--no-gui`, `--no-cccu`.
`./build.sh help` lists them.

There is **one** build script. The old ones are merged into it:

| Old script | Now in `build.sh` |
|---|---|
| claudebackup-linux `dev-release/build.sh` | host tools, bootloaders, the cloner, the Alpine installer tools |
| claudeinstall-linux `build.sh` | `install_claudeinstall_deps`, `build_claudeinstall`, `install_claudeinstall` |
| CCCU `build.sh` | `install_cccu_deps`, `cccu_compiler`, `build_cccu`, `install_cccu` |
| CCCU `src/remaster/provision.sh` | `--root-phase` and `--container-installer`, which CCU runs inside containers |

CCU carries this same `build.sh` into its containers (its provisioning bundle), so the host and the
containers run identical steps.

## Supported distributions

`build.sh` detects your distro from `ID` / `ID_LIKE` in `/etc/os-release`, builds only the tools
that support it, and says which it skips.

| Family | Backup (cloner) | Cloner window | Install (on the ISO) | CCU (containers) |
|---|:---:|:---:|:---:|:---:|
| **Arch** | ✅ | ✅ | ✅ | ✅ |
| **Debian / Ubuntu** | ✅ | ✅ | ✅ | ✅ |
| **Fedora** | ✅ | ✅ | ✅ | ✅ |
| **openSUSE** | ✅ | ✅ | ✅ | ✅ |
| **Gentoo** | ✅ (squashfs) | ✅ (Backup tab) | ✅ | ❌ |
| **Alpine** | ✅ (squashfs) | ✅ (Backup tab) | ❌ — the cloner's own installer | ❌ |
| **Void** | ❌ | ❌ | known to the installer, not to build.sh or the cloner | ❌ |

| Tool | Distributions it recognises |
|---|---|
| **Backup** and the window | **Arch:** Arch, CachyOS, EndeavourOS, Garuda, Archcraft, RebornOS · **Debian:** Debian, Ubuntu, KDE neon, Linux Mint, Pop!_OS, Kali, Devuan, openKylin · **Fedora:** Fedora, Nobara, RHEL · **openSUSE:** Tumbleweed, Slowroll, Leap · **Gentoo:** Gentoo, Calculate, Funtoo, Pentoo · **Alpine** |
| **Install** | the families above (except Alpine) when it installs a clone; its own configuration also knows Manjaro, KaOS, Raspberry Pi OS, CentOS, Rocky, AlmaLinux, SLES, postmarketOS and Void |
| **CCU** | **Arch:** Arch, CachyOS, EndeavourOS, Garuda, ArcoLinux, Manjaro · **Debian:** Debian, Ubuntu, Kubuntu, KDE neon, Linux Mint, Pop!_OS, MX Linux (systemd boot) · **Fedora:** Fedora, CentOS, AlmaLinux, Rocky Linux, RHEL, Nobara · **openSUSE:** Leap, Tumbleweed |

Limits:

- **Backup:** erofs needs `mkfs.erofs` and kernel support (Arch, Debian, Fedora, openSUSE). Gentoo
  and Alpine build squashfs only; Gentoo kernels need `CONFIG_SQUASHFS`, `CONFIG_OVERLAY_FS`,
  `CONFIG_ISO9660_FS` and `CONFIG_BLK_DEV_LOOP`.
- **Install:** built and tested on the Arch family (CachyOS, Qt 6) so far; the other families'
  build dependencies are untested. On Fedora it replaces Anaconda in the ISO.
- **CCU:** systemd on the host and in the container, Incus, GCC 14+ (or Clang 17+), CMake 3.25+.
  Not on NixOS, Gentoo, Slackware, Void, Devuan, Alpine, KDE Linux or Artix. Arch is the main
  target; the other families are implemented but not yet tested on real machines.

Status: beta. Test every ISO and the installer in a virtual machine (UEFI) first — installing
erases the target disk.

## How the pieces fit

```
running system ──claudebackup-linux──▶ live ISO (squashfs/erofs image + Claude Codex Install + shortcut)
                                            │
                       live session: "Install <distro>" → claudebackup-install
                                            │  (pkexec, matches unpackfs to the real image type)
                                            ▼
                   Claude Codex Install: partition → unpack image → fstab, machine-id
                                         → kernel fix-up → initramfs → GRUB → reboot
```

| | ISO engine | Installer on the ISO |
|---|---|---|
| Backup (host) | `cloner/src` | Claude Codex Install, built on the host by `build.sh` |
| CCU *Create ISO* (inside a container) | `cloner/src` (as `cccu-iso-engine`) | Claude Codex Install, built in the container by `build.sh --container-installer` |
| Alpine (host only) | `cloner/src` | the cloner's own *Install Cloned System To Disk* |

| Family | Image the installer unpacks | Initramfs step | GRUB |
|---|---|---|---|
| Arch | `/run/arch/bootmnt/LiveOS/rootfs.img` | `initcpiocfg` + `initcpio` (the running kernel's preset) | `grub` |
| Debian / Ubuntu | `/run/live/medium/live/filesystem.squashfs` | `initramfs` | `grub` |
| Fedora | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` + boot-entry fix-up | `grub2` |
| openSUSE | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` | `grub2` |
| Gentoo | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` | `grub` |
| Alpine | the cloner's own installer | `mkinitfs` | `grub` |

The window runs as your user. When it opens it starts itself once more as a small administrator helper
(`claude-codex-cloner --root-helper`, through pkexec - polkit rule `org.claudemods.claude-codex.cloner`):
that is the one password prompt. Its root tasks run through that helper, which only starts the suite's own
tools (the cloner, `cccu`, `claude-codex`, the installer) and stops when the window closes. In a terminal,
`claude-codex` uses sudo as before.

## How four projects became one

1. **CCCU + claudebackup.** CCCU (Claudemods Container Creator Utility) was the first merge: to
   make a live ISO of a container it carried its own copy of the claudebackup-linux engine, a
   variant of its `build.sh` (`provision.sh`) and the Calamares zips.
2. **claudebackup + claudeinstall-linux.** Then claudebackup-linux and claudeinstall-linux were
   merged into this compendium: ClaudeInstall replaced Calamares, the cloner got batch commands,
   and a Qt6 app was built on them.
3. **Everything together.** CCCU joined as `cccu/`: its engine copy is gone (`cccu-iso-engine` is
   built from `cloner/src`, which got CCU's container improvements), and its provisioning bundle
   carries the compendium's `build.sh` and the installer sources, so `cccu iso` builds Claude Codex
   Install inside the container. See [Claude Codex Install in CCU](cccu/README.md#claudeinstall-in-cccu).
4. **One window, one command, one updater.** The Backup app and the CCU window became one
   **Claude Codex Cloner** with tabs (nothing duplicated: one USB writer, one updater, one guide),
   the terminal tools got one front door (`claude-codex`), and `claude-codex update` updates the
   whole suite with your saved answers.

## Troubleshooting and logs

| What | Where |
|---|---|
| **A build.sh run** | `logs/<date-time>/` in the compendium folder (`logs/latest` = the newest): `build.txt` is the whole run as plain text, plus one log per step — `0-packages`, `1-backup`, `2-cloner-window`, `3-install-*`, `4-ccu`, `5-install`. A tool that fails has its errors repeated on the Complete screen |
| The cloner (ISO builds, USB writes) | `/var/log/claudecodexbackup.log` |
| The installer launcher on the live ISO | `/tmp/claudecodexbackup-install-<uid>.log` |
| Claude Codex Install build errors | `logs/latest/3-install-errors.log` (also in `installer/build/`) |
| CCU | `/var/log/cccu/` (`install.log`, `container.log`, `export.log`, `iso.log`) |
| A boot-time ISO build | `journalctl -b -u claude-codex-backup-boot` |
| A CCU desktop session that does not start | `journalctl -b -u sddm`, `~/.local/share/sddm/wayland-session.log`, `incus info <container> --show-log` |

- **A tool shows FAILED on the Complete screen** — its errors are printed right there; the full log it names is in `logs/latest/`. Fix the reported problem (often a missing package), install what it reports
  missing, run `./build.sh` again.
- **"Not authorised"** — the password dialog was cancelled; polkit (`pkexec`) and a polkit agent
  must be installed.
- **Containers tab: "cannot list the containers"** — your user is not in `incus-admin`; press
  *Refresh* (it asks for the password).
- **Something looks old after an update** — restart Claude Codex Cloner; `claude-codex status`
  shows what is installed.

## Requirements

Linux with systemd for the boot-time build and CCU; Qt 6.2+ for the window; polkit for the
password prompts. `build.sh` installs everything else. CCU additionally needs GCC 14+ / Clang 17+,
CMake 3.25+ and Incus (which CCU installs itself).

---

<div align="center">

**[ Support Me ](https://www.paypal.com/paypalme/claudemods?country.x=GB&locale)** · [claudemodsreloaded.co.uk](https://www.claudemodsreloaded.co.uk)

</div>
