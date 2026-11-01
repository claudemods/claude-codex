cmake_minimum_required(VERSION 3.25)
set(CCCU_RELEASE_DATE "01-11-2026")   # Claude Codex Compendium v1.0 01-11-2026
project(cccu VERSION 1.0 DESCRIPTION "Claude Codex CCU - Container Creator Utility" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(cccu
  src/main.cpp
  src/menu.cpp
  src/bootstrap.cpp
  src/util.cpp
  src/proc.cpp
  src/config.cpp
  src/distro.cpp
  src/drive.cpp
  src/hostinstall.cpp
  src/incus.cpp
  src/profiles.cpp
  src/sources.cpp
  src/rootfs.cpp
  src/installer.cpp
  src/iso.cpp
  src/iso_embed.cpp
  src/containers.cpp
)
target_include_directories(cccu PRIVATE src)
target_compile_definitions(cccu PRIVATE CCCU_VERSION="${PROJECT_VERSION}" CCCU_DATE="${CCCU_RELEASE_DATE}")
target_compile_options(cccu PRIVATE -Wall -Wextra -Wpedantic)

# ---- the ISO engine, built into CCCU ----------------------------------------------------------------------
# The compendium's cloner engine (../cloner/src, the same code as claudebackup-linux) is compiled here as
# cccu-iso-engine and then carried INSIDE the cccu binary (src/iso_embed.cpp). `cccu iso` pushes it into the
# container and runs it there. Linked statically so it runs in any container whatever its C library.
set(COMPENDIUM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CBL_DIR ${COMPENDIUM_DIR}/cloner/src)              # the one engine of the compendium
set(CBL_RESOURCES ${COMPENDIUM_DIR}/cloner/resources)  # its Arch boot tree
if(NOT EXISTS ${CBL_DIR}/remaster.h)
  message(FATAL_ERROR "${CBL_DIR}/remaster.h not found: CCCU builds inside the Claude Codex Compendium (cccu/ next to cloner/).")
endif()
find_package(Threads REQUIRED)
add_executable(cccu-iso-engine src/iso_engine.cpp)
set_target_properties(cccu-iso-engine PROPERTIES CXX_STANDARD 17)   # the engine is C++17
target_include_directories(cccu-iso-engine PRIVATE ${CBL_DIR})
target_compile_definitions(cccu-iso-engine PRIVATE
  CBL_ARCH_ZIP="${CBL_RESOURCES}/build-image-arch-img.zip"
  CBL_TOOLS_HINT="run cccu iso, it installs the tools"
  CBL_VERSION="v${PROJECT_VERSION}")
target_compile_options(cccu-iso-engine PRIVATE -w)
target_link_libraries(cccu-iso-engine PRIVATE Threads::Threads)
include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_LINK_OPTIONS -static)
check_cxx_source_compiles("int main() { return 0; }" CCCU_CAN_LINK_STATIC)
unset(CMAKE_REQUIRED_LINK_OPTIONS)
if(CCCU_CAN_LINK_STATIC)
  target_link_options(cccu-iso-engine PRIVATE -static)
else()
  message(WARNING "No static libc here (Fedora: glibc-static, openSUSE: glibc-devel-static): the ISO engine is linked "
                  "dynamically and may not start in a container with an older C library.")
endif()

# BIOS/UEFI boot files for the non-Arch families: build.sh downloads them once into cccu/resources. Without
# the file the build still works (an empty placeholder is embedded) and the container downloads them instead.
set(CCCU_BOOTLOADERS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/bootloaders.tar.gz)
if(NOT EXISTS ${CCCU_BOOTLOADERS_FILE})
  message(WARNING "cccu/resources/bootloaders.tar.gz is missing: run build.sh (it downloads it) to carry the "
                  "Debian/Fedora/openSUSE boot files inside cccu. Until then `cccu iso` downloads them in the container.")
  set(CCCU_BOOTLOADERS_FILE ${CMAKE_BINARY_DIR}/no-bootloaders.bin)
  file(WRITE ${CCCU_BOOTLOADERS_FILE} "")
endif()

# The provisioning bundle (replaces the old provision.sh + Calamares zips): the compendium's build.sh and the
# ClaudeInstall sources. `cccu iso` unpacks it in the container; build.sh --root-phase installs the ISO tools and
# --container-installer builds ClaudeInstall there and configures it to install the clone.
# installer/needed-files (claudemods distro profiles) and build folders are left out.
set(CCCU_PROVISION_TGZ ${CMAKE_BINARY_DIR}/compendium-provision.tar.gz)
file(GLOB_RECURSE CCCU_INSTALLER_SOURCES CONFIGURE_DEPENDS
     ${COMPENDIUM_DIR}/installer/src/*.cpp ${COMPENDIUM_DIR}/installer/src/*.h ${COMPENDIUM_DIR}/installer/src/*.conf
     ${COMPENDIUM_DIR}/installer/*.conf ${COMPENDIUM_DIR}/installer/CMakeLists.txt)
add_custom_command(OUTPUT ${CCCU_PROVISION_TGZ}
  COMMAND tar -czf ${CCCU_PROVISION_TGZ} -C ${COMPENDIUM_DIR}
          --exclude=installer/build --exclude=installer/build-helper --exclude=installer/build.config
          --exclude=installer/needed-files --exclude=.git
          build.sh installer
  DEPENDS ${COMPENDIUM_DIR}/build.sh ${CCCU_INSTALLER_SOURCES}
  COMMENT "Packing the compendium provisioning bundle (build.sh + ClaudeInstall sources)"
  VERBATIM)
add_custom_target(cccu-provision DEPENDS ${CCCU_PROVISION_TGZ})

add_dependencies(cccu cccu-iso-engine cccu-provision)
set_source_files_properties(src/iso_embed.cpp PROPERTIES OBJECT_DEPENDS "${CCCU_PROVISION_TGZ};${CCCU_BOOTLOADERS_FILE}")
target_compile_definitions(cccu PRIVATE
  CCCU_ENGINE_BIN="$<TARGET_FILE:cccu-iso-engine>"
  CCCU_BOOTLOADERS_TGZ="${CCCU_BOOTLOADERS_FILE}"
  CCCU_PROVISION_TGZ="${CCCU_PROVISION_TGZ}")

install(TARGETS cccu RUNTIME DESTINATION bin)

# Qt6 desktop version (cccu-gui): a front-end that runs the cccu command line tool.
#   cmake -S . -B build -DCCCU_BUILD_GUI=ON && cmake --build build -j
option(CCCU_BUILD_GUI "Build the Qt6 GUI (cccu-gui)" OFF)
if(CCCU_BUILD_GUI)
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  set(CMAKE_AUTOMOC ON)
  add_executable(cccu-gui gui/main.cpp gui/mainwindow.cpp gui/mainwindow.hpp)
  target_compile_definitions(cccu-gui PRIVATE CCCU_GUI_VERSION="${PROJECT_VERSION}" CCCU_DATE="${CCCU_RELEASE_DATE}")
  target_link_libraries(cccu-gui PRIVATE Qt6::Widgets)
  add_dependencies(cccu-gui cccu)   # the GUI needs the backend next to it
  install(TARGETS cccu-gui RUNTIME DESTINATION bin)

  # Application-menu entry and icon, so the GUI is started by clicking (it asks for the password itself).
  include(GNUInstallDirs)
  configure_file(packaging/cccu.desktop.in ${CMAKE_BINARY_DIR}/cccu.desktop @ONLY)
  install(FILES ${CMAKE_BINARY_DIR}/cccu.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install(FILES packaging/cccu.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
