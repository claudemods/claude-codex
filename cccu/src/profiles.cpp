#include "profiles.hpp"

#include "incus.hpp"

#include <sstream>

#include <grp.h>

namespace cccu::profiles {
namespace {

// Device classes a desktop session needs through the container's device cgroup.
constexpr const char* kDeviceAllow[] = {
    "c 4:* rwm",     // virtual terminals
    "c 5:* rwm",     // /dev/tty, /dev/console, /dev/ptmx
    "c 13:* rwm",    // input
    "c 29:* rwm",    // framebuffer
    "c 116:* rwm",   // ALSA sound
    "c 226:* rwm",   // DRM (GPU cards + render nodes)
    "c 10:223 rwm",  // /dev/uinput
    "c 10:229 rwm",  // /dev/fuse
    "c 10:200 rwm",  // /dev/net/tun
};

void char_device(std::ostringstream& y, const std::string& key, const std::string& path) {
    y << "  " << key << ":\n    type: unix-char\n    source: " << path << "\n    path: " << path
      << "\n    required: \"false\"\n";
}

void bind(std::ostringstream& y, const std::string& key, const std::string& path, bool readonly) {
    y << "  " << key << ":\n    type: disk\n    source: " << path << "\n    path: " << path
      << "\n    required: \"false\"\n";
    if (readonly) y << "    readonly: \"true\"\n";
}

void gpu_device(std::ostringstream& y) {
    y << "devices:\n  gpu:\n    type: gpu\n";
    if (const struct group* video = ::getgrnam("video")) y << "    gid: \"" << video->gr_gid << "\"\n";
    y << "    mode: \"0660\"\n";
}

std::string session_yaml() {
    std::ostringstream y;
    y << "description: cccu session - nested desktop on the host session (sockets are added at login)\n"
         "config:\n"
         "  security.privileged: \"true\"\n"
         "  security.nesting: \"true\"\n";
    gpu_device(y);
    return y.str();
}

// The container owns the screen (its own login on a VT). gpu = true: Native, with the GPU passed through.
// gpu = false: Server - no usable GPU (the only screen is the BMC / iKVM chip), so no GPU device: Incus
// would refuse to start a container whose GPU device does not exist. /dev/dri is bound in instead (optional):
// the BMC chip (ASPEED and the like) is still a DRM display device, and KWin's only backend on a real screen
// is DRM - without it the session never starts. Plasma draws in software on it (rootfs).
std::string screen_yaml(bool gpu) {
    std::ostringstream y;
    y << (gpu ? "description: cccu desktop - SDDM / Plasma Login on a real VT with GPU, input and sound\n"
              : "description: cccu server - login on a real VT, software rendering (no GPU), input and sound\n")
      << "config:\n"
         "  security.privileged: \"true\"\n"
         "  security.nesting: \"true\"\n"
         "  raw.lxc: |-\n"
         "    lxc.apparmor.profile = unconfined\n";
    for (const char* rule : kDeviceAllow) y << "    lxc.cgroup2.devices.allow = " << rule << "\n";

    if (gpu) {
        gpu_device(y);
    } else {
        y << "devices:\n";
        bind(y, "dri", "/dev/dri", false);
    }

    for (int vt = 0; vt <= 7; ++vt) char_device(y, "tty" + std::to_string(vt), "/dev/tty" + std::to_string(vt));
    char_device(y, "fb0", "/dev/fb0");
    char_device(y, "uinput", "/dev/uinput");
    char_device(y, "fuse", "/dev/fuse");
    bind(y, "input", "/dev/input", false);
    bind(y, "snd", "/dev/snd", false);
    // Host udev database: lets logind/libinput inside see seat tags and device properties
    // without running a second udevd against the same devices.
    bind(y, "udevdb", "/run/udev", true);
    return y.str();
}

}  // namespace

const char* name(Display display) {
    switch (display) {
        case Display::Native: return "cccu-desktop";
        case Display::Server: return "cccu-server";
        case Display::Session: break;
    }
    return "cccu-session";
}

Result install(Display display) {
    switch (display) {
        case Display::Native: return incus::ensure_profile(name(display), screen_yaml(true));    // GPU on the container's own VT
        case Display::Server: return incus::ensure_profile(name(display), screen_yaml(false));   // no GPU: software rendering
        case Display::Session: break;
    }
    // Default: the host's login shows the container's Plasma through cage, with the GPU (as before server mode existed)
    return incus::ensure_profile(name(display), session_yaml());
}

}  // namespace cccu::profiles
