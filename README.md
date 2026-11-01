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

  ## [ Guide ](#quick-start)

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

</div>

Clone your running Linux system into a bootable live ISO or into an Incus container, and install
the clone back onto a disk from the live ISO, with one build script.

| Folder | What it is | Came from |
|---|---|---|
| `cloner/` | **claudebackup-linux**: the C++ cloner (terminal menu + batch commands). It is also the ISO engine that CCCU uses | `claudebackup-linux-main/dev-release` |
| `cloner-qt6/` | **claudebackup-linux-qt6**: the Qt6 desktop app for the cloner | new |
| `installer/` | **ClaudeInstall**: the Qt6 installer, used in place of Calamares | `claudeinstall-linux-main` |
| `cccu/` | **CCCU**: clones the system into an Incus container, and builds a live ISO of a container | `CCCU-main` |
| `build.sh` | builds, installs and configures everything above | replaces the separate build scripts |

## How four projects became one

1. **CCCU + claudebackup.** CCCU (Claudemods Container Creator Utility) was the first merge. To
   make a live ISO of a container, it carried its own copy of the claudebackup-linux engine
   (`src/remaster`), plus a variant of claudebackup's `build.sh` (`provision.sh`) and the
   Calamares zips. It ran all of that inside the container.
2. **claudebackup + claudeinstall-linux.** Next, claudebackup-linux and claudeinstall-linux were
   merged into this compendium. ClaudeInstall replaced Calamares, the cloner got batch commands,
   and the new Qt6 app **claudebackup-linux-qt6** was built on those commands.
3. **Everything together: ClaudeInstall merged into CCCU.** CCCU joined as `cccu/`.
   - **One engine.** Its private engine copy is gone. CCCU now builds its `cccu-iso-engine` from
     the compendium's `cloner/src`. CCCU's container improvements went into that shared engine:
     the `CBL_KERNEL` override (a container uses the host's kernel, so `uname -r` would name the
     wrong one), the progress bar for the ISO stage, and the `[ INFO ] ISO engine n/N` progress
     lines.
   - **ClaudeInstall inside CCCU.** CCCU's `provision.sh` and Calamares zips are replaced by a
     provisioning bundle (`compendium-provision.tar.gz`, made at build time). It holds the
     compendium's `build.sh` and the ClaudeInstall sources. `cccu iso` unpacks the bundle in the
     container, builds ClaudeInstall there (`build.sh --container-installer`), sets it up to
     install the clone and adds the *Install* Desktop shortcut. CCCU's menu, window and
     `--no-installer` / `--installer-btrfs` options now refer to ClaudeInstall. See
     [ClaudeInstall in CCCU](cccu/README.md#claudeinstall-in-cccu).

That makes four projects in one folder: **claudebackup-linux**, **claudebackup-linux-qt6**,
**claudeinstall-linux** (ClaudeInstall) and **CCCU**. They share one engine, one installer and
one build script:

| | ISO engine | Installer on the ISO |
|---|---|---|
| `claudebackup-linux` / `claudebackup-linux-qt6` (host) | `cloner/src` | ClaudeInstall, built on the host by `build.sh` |
| CCCU `cccu iso` (inside a container) | `cloner/src` (as `cccu-iso-engine`) | ClaudeInstall, built in the container by `build.sh --container-installer` |
| Alpine (host only) | `cloner/src` | the cloner's own *Install Cloned System To Disk* |

The ISO is a **full clone**. Your users, passwords, settings and `/home` go into it exactly as
they are, and no live account is added. When ClaudeInstall installs the ISO it unpacks that same
image, so the installed system has your accounts too (there is no users page).

## Supported distributions (per tool)

Each tool supports a different set of distributions. `build.sh` detects yours from `ID` and
`ID_LIKE` in `/etc/os-release`. It builds only the tools that support it and says which ones it
skips.

| Distribution family | claudebackup-linux (cloner) | claudebackup-linux-qt6 (app) | ClaudeInstall (installer on the ISO) | CCCU (containers) |
|---|:---:|:---:|:---:|:---:|
| **Arch** | ✅ | ✅ | ✅ | ✅ |
| **Debian / Ubuntu** | ✅ | ✅ | ✅ | ✅ |
| **Fedora** | ✅ | ✅ | ✅ | ✅ |
| **openSUSE** | ✅ | ✅ | ✅ | ✅ |
| **Gentoo** | ✅ (squashfs only) | ✅ | ✅ | ❌ |
| **Alpine** | ✅ (squashfs only) | ✅ | ❌, the cloner's own installer is used | ❌ |
| **Void** | ❌ | ❌ | ClaudeInstall knows Void, but the compendium's build.sh and the cloner do not support it | ❌ |

Which distributions each family includes:

| Tool | Distributions it recognises |
|---|---|
| **claudebackup-linux** and **claudebackup-linux-qt6** (the app runs the cloner, so it supports the same distros) | **Arch:** Arch, CachyOS, EndeavourOS, Garuda, Archcraft, RebornOS · **Debian:** Debian, Ubuntu, KDE neon, Linux Mint, Pop!_OS, Kali, Devuan, openKylin · **Fedora:** Fedora, Nobara, RHEL · **openSUSE:** Tumbleweed, Slowroll, Leap · **Gentoo:** Gentoo, Calculate, Funtoo, Pentoo · **Alpine** |
| **ClaudeInstall** | the families above (except Alpine) when it installs a clone. Its own build script also knows Manjaro, KaOS, Raspberry Pi OS, CentOS, Rocky, AlmaLinux, SLES, postmarketOS and Void |
| **CCCU** | **Arch:** Arch, CachyOS, EndeavourOS, Garuda, ArcoLinux, Manjaro · **Debian:** Debian, Ubuntu, Kubuntu, KDE neon, Linux Mint, Pop!_OS, MX Linux (systemd boot only) · **Fedora:** Fedora, CentOS, AlmaLinux, Rocky Linux, RHEL, Nobara · **openSUSE:** Leap, Tumbleweed |

Limits for each tool:

- **claudebackup-linux / Qt6 app:** erofs images work on Arch, Debian, Fedora and openSUSE
  (they need `mkfs.erofs` and kernel support). Gentoo and Alpine build squashfs only. Gentoo
  kernels need `CONFIG_SQUASHFS`, `CONFIG_OVERLAY_FS`, `CONFIG_ISO9660_FS` and
  `CONFIG_BLK_DEV_LOOP`. The Qt6 app needs Qt 6.2 or newer.
- **ClaudeInstall:** so far it has only been built on the Arch family (CachyOS, Qt6). The other
  families' build dependencies are untested. On Fedora it replaces Anaconda in the ISO. Alpine
  ISOs use the cloner's *Install Cloned System To Disk* instead.
- **CCCU:** needs systemd on the host and in the container, Incus, GCC 14+ (or Clang 17+) and
  CMake 3.25+. NixOS, Gentoo, Slackware, Void, Devuan, Alpine, KDE Linux and Artix are not
  supported (no systemd, or no installable package tree). Its *Create ISO* (and ClaudeInstall in
  the container) covers the Arch, Debian, Fedora and openSUSE families. Arch is the main target;
  the others are implemented but not tested on real machines yet.

Testing status: all of the cloner's families are beta. ClaudeInstall in CCCU's container ISOs is
new and untested. Test every ISO in a virtual machine (UEFI) first.

## Quick start

```bash
./build.sh
```

Run it as your normal user. It asks for the root password when it needs it (sudo, doas, run0,
pkexec or su). It asks five questions: the image type (squashfs or erofs), whether to build
ClaudeInstall, ext4 or btrfs zstd:22 as the installer's default filesystem, whether to build the
Qt6 app, and whether to build CCCU. Then it:

1. installs the build and ISO tools for your distro (squashfs-tools, xorriso, mtools, ...)
2. builds and installs `claudebackup-linux` (the cloner) and its polkit rule
3. builds and installs `claudebackup-linux-qt6` (the app)
4. builds and installs ClaudeInstall (clone mode, Qt6 GUI), then sets up `/etc/claudeinstall`
   to install the clone from the live ISO
5. builds and installs CCCU (`cccu` and `cccu-gui`). This only happens on Arch,
   Debian, Fedora and openSUSE, and needs GCC 14+. If it fails, everything else is still
   installed.
6. puts an **Install &lt;distro&gt;** shortcut on the Desktop. The shortcut is part of the clone,
   so it shows up on the live ISO's desktop too.
7. starts the app

Options: `--yes` (use the defaults, ask nothing), `--erofs`, `--btrfs` / `--ext4`,
`--no-installer`, `--no-gui`, `--no-cccu`, `--no-launch`, `--installer-mode=clone|new|distro`,
`--installer-tui`. Run `./build.sh help` to list them.

### One build script

There is **one** build script: `./build.sh` at the top. It builds all four tools, with no
separate build script per project. The four old build scripts are merged into it:

| Old script | Now in `build.sh` |
|---|---|
| claudebackup-linux `dev-release/build.sh` | host tools, bootloaders, the cloner, the Alpine installer tools |
| claudeinstall-linux `build.sh` | `install_claudeinstall_deps`, `build_claudeinstall`, `install_claudeinstall` |
| CCCU `build.sh` | `install_cccu_deps`, `cccu_compiler`, `build_cccu`, `install_cccu` |
| CCCU `src/remaster/provision.sh` | `--root-phase` and `--container-installer`, which CCCU runs inside containers |

CCCU carries this same `build.sh` into its containers (in its provisioning bundle), so the host
and the containers run identical steps.

The only other scripts in the folder are not build scripts:

| Script | What it is |
|---|---|
| `installer/claudeconfig`, `claudeinstall-colors`, `claudeinstall-pkgfix` | commands that ClaudeInstall installs on the system (to change its settings and colours) |
| `installer/needed-files/**/*.sh` | tweak scripts that ClaudeInstall's claudemods-distro mode copies onto installed systems |

Removed while merging: those four build scripts, ClaudeInstall's setup helper app
(`installer/helper/`, which only wrote settings for its old build script), and ClaudeInstall's
`ci/` folder (Calamares build-test scripts that nothing used).

## CCCU (containers)

`sudo cccu` (terminal) or **CCCU** in the applications menu (`cccu-gui`). The Qt6 app's Overview
page also has an **Open** button for it. CCCU clones the system into an Incus system container
with its own btrfs storage and boots into its KDE Plasma desktop on a minimal tty host. Its
**Create ISO** mode runs the compendium's cloner engine inside the container. With the installer
switched on, the first run builds ClaudeInstall inside the container
(`build.sh --container-installer`), so the ISO has an *Install* shortcut, the same as an ISO made
with `claudebackup-linux`. If ClaudeInstall does not build there, the ISO is still made, just
without an installer. See [cccu/README.md](cccu/README.md) for all modes.

## Using it

**Qt6 app** (`claudebackup-linux-qt6`, listed as *claudebackup linux* in the applications menu).
Run it as your user. It asks for the administrator password through polkit when a task needs it.

- **Overview**: shows what is ready on this system (tools, boot files, ClaudeInstall, free
  space), with fix buttons, and quick actions
- **Build ISO**: the build steps with live progress, a colour log, elapsed time and Cancel.
  Cancel stops the running program and removes every mount.
- **Settings**: ISO name, work, output and clone directories, image type, zstd level, RAM-mode
  boot entry, package-cache cleaning. The terminal menu uses the same settings file.
- **Edit Configs**: your own copies of GRUB, boot text and initramfs files, and ClaudeInstall's
  configuration files. Installer files are saved as root and keep a `.bak`.
- **Write USB**: writes an ISO to a drive with progress. The running system's disks can't be
  selected.
- **Install**: in the live ISO, launches ClaudeInstall. On Alpine it runs the cloner's own
  installer instead.
- **Guide** and **Maintenance**: the guide for your distro, updates, the Arch mkinitcpio config,
  and the log

**Terminal menu**: `claudebackup-linux` works the same as before. *Launch Calamares* is now
*Launch ClaudeInstall*.

**Batch commands**, for scripts and for the app: run `claudebackup-linux --help-batch` to list
them. Examples: `--status`, `--build-iso`, `--write-usb ISO DEVICE`, `--set level=5`.

## How the pieces fit

```
running system ──claudebackup-linux──▶ live ISO (squashfs/erofs image + ClaudeInstall + shortcut)
                                            │
                       live session: "Install <distro>" → claudebackup-install
                                            │  (pkexec, matches unpackfs to the real image type)
                                            ▼
                         ClaudeInstall: partition → unpack image → fstab, machine-id
                                        → kernel fix-up → initramfs → GRUB → reboot
```

| Family | Image that ClaudeInstall unpacks | Initramfs step | GRUB |
|---|---|---|---|
| Arch | `/run/arch/bootmnt/LiveOS/rootfs.img` | `initcpiocfg` + `initcpio` (running kernel's preset) | `grub` |
| Debian / Ubuntu | `/run/live/medium/live/filesystem.squashfs` | `initramfs` | `grub` |
| Fedora | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` + boot-entry fix-up | `grub2` |
| openSUSE | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` | `grub2` |
| Gentoo | `/run/initramfs/live/LiveOS/squashfs.img` | `dracut` | `grub` |
| Alpine | the cloner's own installer (*Install Cloned System To Disk*) | `mkinitfs` | `grub` |

`build.sh --configure-installer` (run as root) writes the generated files:
`/etc/claudeinstall/settings.conf`, `modules/*.conf` and `scripts/cbl-kernel.sh`. The launcher
is `/usr/local/bin/claudebackup-install`. If an earlier ClaudeInstall configuration exists, it is
kept in `/etc/claudeinstall.orig`.

## What changed from the two old projects

- Calamares is gone. The Calamares zips are no longer shipped, and nothing installs or launches
  Calamares. The old `claudebackup-calamares` launcher and its Desktop shortcut are replaced
  when you run `build.sh`.
- The cloner reports ClaudeInstall in its status, its guide and *Edit Configs*. On Fedora,
  Anaconda's launchers are left out of the image when ClaudeInstall is installed.
- The cloner has new batch commands that the Qt6 app runs, plus `@@step` progress markers for
  the app.
- `--write-usb` refuses any drive that holds the running system.
- ClaudeInstall source fixes (both were broken in the original `claudeinstall-linux-main` as well):
  - The partition module was split across `src/modules/`: `jobs/`, `PartitionViewStep`,
    `PartitionTuiStep` and `partition.schema.yaml` were loose there, next to older copies of
    `Config.cpp`/`.h` and `partition.conf`. They are now back in `src/modules/partition/`. The
    newer copies were kept, and the old loose duplicates and the duplicate `tests/` folder were
    removed.
  - `src/libclaudeinstallui/CMakeLists.txt` pointed at `utils/ErrorDialog/ErrorDialog.ui`, which
    does not exist. It now points at `widgets/ErrorDialog.ui`, where the file and its `.cpp` are.
- CCCU no longer ships an engine copy, `provision.sh` or Calamares zips. Its `cccu-iso-engine`
  is built from `cloner/src`, and its in-container provisioning uses the compendium's
  `build.sh` (`--root-phase`, then `--container-installer`).
- The cloner's update check and self-update now point at a `claude-codex` repository.
  Change `REPO_URL` and the other update URLs in `cloner/src/common.h` if you publish it
  somewhere else.

## Requirements

Linux; Qt 6.2 or newer for the app; ClaudeInstall's own dependencies, which
the compendium's `build.sh` installs. Test the ISO and the installer in a virtual machine (UEFI)
first: installing erases the target disk.

Logs: `/var/log/claudebackup-linux.log` (cloner), `/tmp/claudebackup-install-<uid>.log`
(installer launcher), and ClaudeInstall's session log.

---

<div align="center">

**[ Support Me ](https://www.paypal.com/paypalme/claudemods?country.x=GB&locale)** · [claudemodsreloaded.co.uk](https://www.claudemodsreloaded.co.uk)

</div>
