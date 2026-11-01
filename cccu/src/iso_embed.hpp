#pragma once

namespace cccu::iso_embed {

struct Blob {
    const char* begin;
    const char* end;
    unsigned long long size() const { return static_cast<unsigned long long>(end - begin); }
};

// Compiled into the cccu binary (see iso_embed.cpp), so `cccu iso` needs no files next to it.
Blob engine();        // cccu-iso-engine: the compendium's ISO engine (cloner/src), a static program run in the container
Blob provision();     // compendium-provision.tar.gz: the compendium's build.sh (--root-phase installs the ISO tools,
                      // --container-installer builds Claude Codex Install) and the Claude Codex Install sources (installer/)
Blob bootloaders();   // BIOS/UEFI boot files for the non-Arch families (empty when the build had no network: then the
                      // provisioning downloads them in the container)

}  // namespace cccu::iso_embed
