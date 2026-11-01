#ifndef CLI_H
#define CLI_H

#include <csignal>
#include "menu.h"

// Non-interactive commands. The Qt6 app (claudebackup-linux-qt6) drives the cloner through them, so
// the GUI and the menu share one engine; they work from scripts too. Info commands run as the user,
// the others need root (the app starts them through pkexec, the terminal through sudo/doas/...).
// Output is the normal coloured log; "@@..." lines are markers for the app (see remaster::marker).
namespace cli {

struct Command {
    const char* name;
    bool root;
    const char* usage;
};

inline const std::vector<Command>& commands() {
    static const std::vector<Command> c = {
        {"--status",            false, "key=value lines: distro, tools, settings, paths"},
        {"--plan",              false, "the build steps for this distro (step=name|description)"},
        {"--guide",             false, "the guide for this distro"},
        {"--set",               false, "KEY=VALUE ...  change saved settings (isoName, workDir, outputDir, level,\n"
                                       "                        ramMode, isoTag, cloneDir, imageFormat, cleanCache)"},
        {"--check-update",      false, "latest=<version> and newer=0|1"},
        {"--build-iso",         true,  "build the ISO with the saved settings (--console: plain output, no app markers)"},
        {"--build-on-boot",     true,  "build the ISO on the next boot, from init, before the login screen"},
        {"--cancel-boot-build", true,  "cancel a build set up with --build-on-boot"},
        {"--write-usb",         true,  "ISO DEVICE  write an ISO to a whole drive (erases it)"},
        {"--install-alpine",    true,  "DISK ext4|btrfs 0|1  Alpine live ISO: install the clone (1 = zstd:22)"},
        {"--mkinitcpio-config", true,  "Arch: put 11-dm-initramfs.rules in place"},
        {"--seed-custom",       true,  "NAME  create your editable copy of a boot/initramfs file"},
        {"--preview-custom",    true,  "NAME  what the build would generate for that file (not used by the build)"},
        {"--write-system-file", true,  "PATH  replace an installer config file with stdin (keeps PATH.bak)"},
        {"--launch-installer",  true,  "start Claude Codex Install for the clone"},
        {"--update",            true,  "download, build and install the latest cloner"},
        {"--help-batch",        false, "this list"},
    };
    return c;
}

// The first argument naming a command
inline const Command* find(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++)
        for (const auto& c : commands())
            if (std::string(argv[i]) == c.name) return &c;
    return nullptr;
}

// Arguments after the command, without our own --options
inline std::vector<std::string> operands(int argc, char* argv[], const Command& cmd) {
    std::vector<std::string> out;
    bool after = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == cmd.name) { after = true; continue; }
        if (after && !startsWith(a, "--")) out.push_back(a);
    }
    return out;
}

inline bool hasFlag(int argc, char* argv[], const char* flag) {
    for (int i = 1; i < argc; i++) if (std::string(argv[i]) == flag) return true;
    return false;
}

// --cancel-on-eof: the app keeps our stdin open and closes it to cancel. Then everything we started
// (mksquashfs, dd, scripts; one process group) is stopped and the mounts of a build are removed.
inline void cancelOnEof() {
    setpgid(0, 0);   // our own group, so the kill below never reaches the app that started us
    std::thread([] {
        char buf[256];
        while (read(STDIN_FILENO, buf, sizeof(buf)) > 0) {}
        logWarning("Cancelled - stopping and cleaning up...");
        signal(SIGTERM, SIG_IGN);
        kill(0, SIGTERM);
        usleep(300000);
        remaster::marker("result cancelled");
        if (!remaster::g_workDir.empty()) remaster::onSignal(SIGTERM);
        _exit(130);
    }).detach();
}

inline void kv(const std::string& k, const std::string& v) { std::cout << k << "=" << v << "\n"; }
inline std::string b(bool v) { return v ? "1" : "0"; }

inline int printStatus() {
    const Distro& d = sysDistro();
    Context c = remaster::makeContext(currentOptions());
    std::string missing;
    for (const auto& m : remaster::missingTools(d)) missing += (missing.empty() ? "" : " ") + m;

    kv("app", APP_NAME);
    kv("version", APP_VERSION);
    kv("release", APP_RELEASE);
    kv("bootBuildSupported", b(bootbuild::supported()));
    kv("bootBuildScheduled", b(bootbuild::scheduled()));
    kv("user", USERNAME);
    kv("distro", d.prettyName);
    kv("distroId", d.distroID);
    kv("family", d.family);
    kv("familyLabel", d.familyLabel);
    kv("arch", d.arch);
    kv("supported", b(d.supported()));
    kv("missingTools", missing);
    kv("bootFiles", b(remaster::bootFilesReady(d)));
    kv("mkinitcpio", d.family == "archlinux" ? b(remaster::mkinitcpioConfigReady()) : "na");
    kv("installer", remaster::installerAvailable(d) ? INSTALLER_BIN : d.family == "alpine" ? "alpine" : "none");
    kv("installerReady", remaster::installerAvailable(d) ? b(remaster::installerReady())
                                                         : b(d.family == "alpine" && remaster::alpineInstallerReady()));
    kv("installerLauncher", INSTALLER_LAUNCHER);
    kv("erofsSupported", b(d.erofsSupported()));
    kv("erofsKernel", b(native::kernelHasErofs()));
    kv("mkfsErofs", b(commandExists("mkfs.erofs")));
    kv("compression", remaster::compressionFor(d));
    kv("isoName", mcfg.isoName);
    kv("workDir", mcfg.workDir);
    kv("outputDir", mcfg.outputDir);
    kv("level", std::to_string(mcfg.level));
    kv("ramMode", b(mcfg.ramMode));
    kv("isoTag", mcfg.isoTag);
    kv("cloneDir", mcfg.cloneDir);
    kv("imageFormat", mcfg.imageFormat);
    kv("cleanCache", b(mcfg.cleanCache));
    kv("defaultWorkDir", DEFAULT_WORK_DIR);
    kv("defaultCloneDir", defaultCloneDir());
    kv("effectiveWorkDir", c.workDir);
    kv("effectiveCloneDir", c.cloneDir);
    kv("isoOutputDir", isoOutputDir());
    kv("isoPath", c.isoPath);
    kv("isoTreeDir", isoTreeDirPath());
    kv("bootParams", families::bootParams(c));
    kv("configFile", configFilePath());
    kv("customDir", customDirPath());
    kv("logFile", LOG_FILE);
    for (const auto& f : remaster::customFilesFor(d))
        kv("customFile", f.name + "|" + f.label + "|" + b(isRegularFile(customDirPath() + "/" + f.name)));
    if (remaster::installerAvailable(d))
        for (const auto& f : remaster::installerConfigFiles(d))
            kv("installerFile", f.first + "|" + f.second + "|" + b(isRegularFile(f.second)));
    std::cout << std::flush;
    return 0;
}

inline int printPlan() {
    Context c = remaster::makeContext(currentOptions());
    for (const auto& s : plan::build(c)) kv("step", s.name + "|" + s.description);
    std::cout << std::flush;
    return 0;
}

// --set KEY=VALUE ...: validated like the menu's Setup items
inline int setValues(const std::vector<std::string>& args) {
    if (args.empty()) { logError("--set needs KEY=VALUE arguments"); return 2; }
    for (const auto& a : args) {
        size_t eq = a.find('=');
        if (eq == std::string::npos) { logError("not KEY=VALUE: " + a); return 2; }
        const std::string k = a.substr(0, eq);
        std::string v = trim(a.substr(eq + 1));
        if (k == "isoName") mcfg.isoName = v;
        else if (k == "workDir" || k == "outputDir" || k == "cloneDir") {
            while (v.size() > 1 && v.back() == '/') v.pop_back();
            if (!v.empty() && v[0] != '/') { logError(k + " must be an absolute path"); return 2; }
            if (k == "cloneDir" && v == "/") { logError("the clone directory cannot be /"); return 2; }
            (k == "workDir" ? mcfg.workDir : k == "outputDir" ? mcfg.outputDir : mcfg.cloneDir) = v;
        } else if (k == "level") {
            int l = atoi(v.c_str());
            if (l < 1 || l > 22) { logError("level must be 1-22"); return 2; }
            mcfg.level = l;
        } else if (k == "ramMode") mcfg.ramMode = v == "1";
        else if (k == "cleanCache") mcfg.cleanCache = v == "1";
        else if (k == "isoTag") {
            if (v.find_first_of(" \t\"'") != std::string::npos) { logError("the ISO tag cannot contain spaces or quotes"); return 2; }
            mcfg.isoTag = v;
        } else if (k == "imageFormat") {
            if (v != "squashfs" && v != "erofs") { logError("imageFormat is squashfs or erofs"); return 2; }
            if (v == "erofs" && !sysDistro().erofsSupported()) { logError(sysDistro().prettyName + " only builds squashfs images"); return 2; }
            mcfg.imageFormat = v;
        } else { logError("unknown setting: " + k); return 2; }
    }
    saveConfig();
    return 0;
}

// preview = true (--preview-custom): write what the build would generate to custom/.generated/NAME,
// which the build never reads - the app shows it, and only saving makes it the user's own copy
inline int seedCustom(const std::vector<std::string>& args, bool preview = false) {
    if (args.size() != 1) { logError(preview ? "--preview-custom NAME" : "--seed-custom NAME"); return 2; }
    for (const auto& f : remaster::customFilesFor(sysDistro())) {
        if (f.name != args[0]) continue;
        const std::string path = customDirPath() + (preview ? "/.generated/" : "/") + f.name;
        if (preview) unlink(path.c_str());   // always the current settings' version
        if (!isRegularFile(path) && !remaster::seedCustomFile(remaster::makeContext(currentOptions()), f.name, path)) return 1;
        // our copies live in the user's config folder and are edited as the user
        runPlain("chown " + shq(USERNAME) + " " + shq(configDirPath()) + " 2>/dev/null");
        runPlain("chown -R " + shq(USERNAME) + " " + shq(customDirPath()) + " 2>/dev/null");
        kv("path", path);
        return 0;
    }
    logError(args[0] + " is not an editable file on " + sysDistro().prettyName);
    return 2;
}

inline int writeSystemFile(const std::vector<std::string>& args) {
    if (args.size() != 1) { logError("--write-system-file PATH (content on stdin)"); return 2; }
    const std::string& path = args[0];
    if (!remaster::isEditableSystemFile(path)) { logError(path + " is not an installer config file this app may change."); return 2; }
    std::stringstream ss;
    ss << std::cin.rdbuf();
    if (isRegularFile(path) && !isRegularFile(path + ".bak")) writeFile(path + ".bak", readFile(path));
    mkdirP(dirName(path));
    if (!writeFile(path, ss.str())) { logError("cannot write " + path); return 1; }
    logSuccess("Saved " + path);
    return 0;
}

inline int run(const Command& cmd, int argc, char* argv[]) {
    const std::string name = cmd.name;
    const std::vector<std::string> args = operands(argc, argv, cmd);
    loadConfig();
    // the app reads "@@" markers; the boot build (--console) draws on the screen instead
    remaster::g_markers = !hasFlag(argc, argv, "--console");
    if (cmd.root && hasFlag(argc, argv, "--cancel-on-eof")) cancelOnEof();

    auto result = [](bool ok) { remaster::marker(std::string("result ") + (ok ? "ok" : "fail")); return ok ? 0 : 1; };

    if (name == "--help-batch") {
        std::cout << APP_NAME << " (" << APP_BIN << ") " << APP_RELEASE << " - batch commands (no menu):\n";
        for (const auto& c : commands()) std::cout << "  " << c.name << "  " << c.usage << (c.root ? "  [root]" : "") << "\n";
        std::cout << "  --cancel-on-eof   with a root command: cancel when stdin closes\n";
        return 0;
    }
    if (name == "--status") return printStatus();
    if (name == "--plan") return printPlan();
    if (name == "--guide") { std::cout << guideText(); return 0; }
    if (name == "--set") return setValues(args);
    if (name == "--check-update") {
        std::string latest = update::latestVersion();
        kv("installed", APP_VERSION);
        kv("latest", latest);
        kv("newer", b(!latest.empty() && update::isNewer(latest, APP_VERSION)));
        return latest.empty() ? 1 : 0;
    }

    mkdirP(configDirPath());
    chownToUser(configDirPath());
    if (name == "--build-iso") return result(buildISO());
    if (name == "--build-on-boot") return result(bootbuild::schedule(USERNAME));
    if (name == "--cancel-boot-build") return result(bootbuild::cancel());
    if (name == "--write-usb") {
        if (args.size() != 2) { logError("--write-usb ISO DEVICE"); return result(false); }
        return result(writeIsoToDevice(args[0], args[1]));
    }
    if (name == "--install-alpine") {
        if (!onFamily("alpine")) { logError("--install-alpine is the Alpine installer; use " + INSTALLER_NAME + " here."); return result(false); }
        if (args.size() < 2 || !startsWith(args[0], "/dev/") || (args[1] != "ext4" && args[1] != "btrfs")) {
            logError("--install-alpine DISK ext4|btrfs [0|1]");
            return result(false);
        }
        if (!remaster::alpineInstallerReady()) { logError("The Alpine installer tools are missing (run the compendium's build.sh before building the ISO)."); return result(false); }
        return result(remaster::installClonedSystemAlpine(args[0], args[1], args.size() > 2 && args[2] == "1"));
    }
    if (name == "--mkinitcpio-config") {
        if (!onFamily("archlinux")) { logError("mkinitcpio config is for the Arch family only."); return result(false); }
        return result(remaster::ensureMkinitcpioConfig());
    }
    if (name == "--seed-custom") return result(seedCustom(args) == 0);
    if (name == "--preview-custom") return result(seedCustom(args, true) == 0);
    if (name == "--write-system-file") return result(writeSystemFile(args) == 0);
    if (name == "--launch-installer") return result(launchInstaller());
    if (name == "--update") return result(update::downloadBuildInstall());
    return 2;
}

}  // namespace cli

#endif
