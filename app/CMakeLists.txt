# claude-codex-cloner: Claude Codex Cloner, the one Qt6 window of Claude Codex Compendium v1.0 01-11-2026.
# Tabs: Backup (the cloner, ../cloner), Containers (Claude Codex CCU, ../cccu), Updates (the whole suite),
# Guide. It runs as your user; each task asks for the administrator password through pkexec.
# Built and installed by the compendium's build.sh; by hand:
#   cmake -S app -B build-app -DCMAKE_BUILD_TYPE=Release && cmake --build build-app
#   sudo cmake --install build-app
cmake_minimum_required(VERSION 3.16)
project(claude-codex-cloner VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt 6.2+ (Qt::SingleShotConnection, QProcess::finished without overloads)
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
include(GNUInstallDirs)

add_executable(claude-codex-cloner
    src/main.cpp
    src/MainWindow.cpp src/MainWindow.h
    src/BackupView.cpp src/BackupView.h
    src/CcuView.cpp src/CcuView.h
    src/UpdatesView.cpp src/UpdatesView.h
    src/GuideView.cpp src/GuideView.h
    src/Backend.cpp src/Backend.h
    src/TaskPanel.cpp src/TaskPanel.h
    src/AnsiLog.cpp src/AnsiLog.h
)
target_link_libraries(claude-codex-cloner PRIVATE Qt6::Widgets)
target_compile_options(claude-codex-cloner PRIVATE -Wall -Wextra)

install(TARGETS claude-codex-cloner RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/claude-codex-cloner.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/claude-codex-cloner.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
