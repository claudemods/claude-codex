#ifndef BOOTBUILD_H
#define BOOTBUILD_H

#include <climits>
#include "common.h"

// Build the ISO from init, like Claude Codex CCU's "--on-reboot": a one-shot systemd service runs the
// build on the console (VT1) on the next boot - after the boot, BEFORE the login screen, so no desktop
// or apps change files while the system is copied. The progress is drawn on the screen.
//   success: the service switches itself off, the PC reboots after a 5 second countdown
//   failure: the error stays on the screen for 30 seconds, then the login screen starts as usual
// The service always removes itself, so a failed build never traps the next boots.
namespace bootbuild {

const std::string UNIT      = "claude-codex-backup-boot.service";
const std::string UNIT_PATH = "/etc/systemd/system/" + UNIT;
const std::string STATE_DIR = "/var/lib/claudebackup-linux";
const std::string PENDING   = STATE_DIR + "/boot-build-pending";
const std::string SCRIPT    = STATE_DIR + "/boot-build.sh";   // what the service runs (no unit-file quoting)
const std::string DROPIN    = "10-claude-codex-backup.conf";

// systemd is running (Alpine and OpenRC Gentoo have no boot build)
inline bool supported() { return isDirectory("/run/systemd/system") && commandExists("systemctl"); }

inline bool scheduled() { return isRegularFile(PENDING) && isRegularFile(UNIT_PATH); }

// The display manager's unit (sddm.service, gdm.service ...): it waits for the build. Empty = none.
inline std::string displayManagerUnit() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/etc/systemd/system/display-manager.service", buf, sizeof(buf) - 1);
    if (n <= 0) return "";
    return baseName(std::string(buf, n));
}

inline std::string selfPath() {
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    return n > 0 ? std::string(buf, n) : "/usr/bin/" + APP_BIN;
}

// Sets the build up for the next boot, for `user` (whose settings it uses). Root only.
inline bool schedule(const std::string& user) {
    if (geteuid() != 0) { logError("Setting up the boot build needs root."); return false; }
    if (!supported()) { logError("Building at boot needs systemd; this system does not run it."); return false; }

    const std::string script =
        "#!/bin/sh\n"
        "# " + APP_NAME + " " + APP_RELEASE + ": run once by " + UNIT + " on the console (VT1)\n"
        // Switched off BEFORE the build: the image is a clone of this system, and an enabled service in
        // it would make the ISO (and every system installed from it) build again on its first boot
        "rm -f " + shq(PENDING) + "\n"
        "systemctl disable " + UNIT + " >/dev/null 2>&1\n"
        "clear\n"
        "printf '\\033[1;31m%s\\033[0m\\n\\n' " + shq(APP_NAME + "  " + APP_RELEASE + "  -  building the ISO at boot") + "\n" +
        shq(selfPath()) + " " + shq("--cbl-user=" + user) + " --build-iso --console\n"
        "rc=$?\n"
        "if [ $rc -ne 0 ]; then\n"
        "    echo\n"
        "    echo \"The ISO build failed (exit $rc). Log: " + LOG_FILE + " - continuing to the login screen in 30 s.\"\n"
        "    sleep 30\n"
        "else\n"
        "    echo\n"
        "    i=5\n"
        "    while [ $i -gt 0 ]; do printf \"\\rISO created. Rebooting in %d seconds...  \" $i; sleep 1; i=$((i-1)); done\n"
        "    echo\n"
        "    systemctl reboot\n"
        "fi\n"
        "exit $rc\n";

    const std::string dm = displayManagerUnit();
    const std::string dropinDir = dm.empty() ? "" : "/etc/systemd/system/" + dm + ".d";
    const std::string dropin = dropinDir.empty() ? "" : dropinDir + "/" + DROPIN;

    const std::string unit =
        "[Unit]\n"
        "Description=" + APP_NAME + ": build the live ISO at boot (" + APP_RELEASE + ")\n"
        "After=local-fs.target systemd-user-sessions.service getty@tty1.service\n"
        "Conflicts=getty@tty1.service\n"
        "ConditionPathExists=" + PENDING + "\n\n"
        "[Service]\n"
        "Type=oneshot\n"
        "TimeoutStartSec=infinity\n"
        "StandardInput=tty\n"
        "StandardOutput=tty\n"
        "StandardError=tty\n"
        "TTYPath=/dev/tty1\n"
        "TTYReset=yes\n"
        "TTYVHangup=yes\n"
        "Environment=TERM=linux\n"
        "ExecStartPre=-/usr/bin/kill -s RTMIN+21 1\n"          // silence boot messages while the progress is drawn
        "ExecStart=/bin/sh " + SCRIPT + "\n"
        "ExecStopPost=/usr/bin/rm -f " + PENDING + (dropin.empty() ? "" : " " + dropin) + "\n"
        "ExecStopPost=-/usr/bin/systemctl disable " + UNIT + "\n"
        "ExecStopPost=-/usr/bin/kill -s RTMIN+20 1\n"
        "ExecStopPost=-/usr/bin/systemctl --no-block start getty@tty1.service\n\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n";

    mkdirP(STATE_DIR);
    if (!writeFile(SCRIPT, script, 0755)) { logError("cannot write " + SCRIPT); return false; }
    if (!writeFile(PENDING, "pending for " + user + "\n")) { logError("cannot write " + PENDING); return false; }
    if (!dropin.empty()) {
        // the login screen waits for the build
        mkdirP(dropinDir);
        if (!writeFile(dropin, "[Unit]\nAfter=" + UNIT + "\n")) { logError("cannot write " + dropin); return false; }
    }
    if (!writeFile(UNIT_PATH, unit)) { logError("cannot write " + UNIT_PATH); return false; }
    runPlain("systemctl daemon-reload");
    if (runCommand("systemctl enable " + UNIT + " 2>&1") != 0) { logError("could not enable " + UNIT); return false; }
    logSuccess("Ready: the ISO is built on the next boot, on the console (VT1), before the login screen.");
    logNormal("Cancel before rebooting with: " + APP_BIN + " --cancel-boot-build");
    return true;
}

inline bool cancel() {
    if (geteuid() != 0) { logError("Cancelling the boot build needs root."); return false; }
    runPlain("systemctl disable " + UNIT + " >/dev/null 2>&1");
    const std::string dm = displayManagerUnit();
    if (!dm.empty()) unlink(("/etc/systemd/system/" + dm + ".d/" + DROPIN).c_str());
    unlink(PENDING.c_str());
    unlink(SCRIPT.c_str());
    unlink(UNIT_PATH.c_str());
    runPlain("systemctl daemon-reload");
    logSuccess("The boot build is cancelled.");
    return true;
}

}  // namespace bootbuild

#endif
