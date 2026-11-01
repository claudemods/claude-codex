# claudebackup-linux-qt6: Claude Codex Backup, the Qt6 front end of the cloner (../cloner).
# Built and installed by the compendium's build.sh; by hand:
#   cmake -S cloner-qt6 -B build-qt6 -DCMAKE_BUILD_TYPE=Release && cmake --build build-qt6
#   sudo cmake --install build-qt6
cmake_minimum_required(VERSION 3.16)
project(claudebackup-linux-qt6 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt 6.2+ (Qt::SingleShotConnection, QProcess::finished without overloads)
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets)
include(GNUInstallDirs)

add_executable(claudebackup-linux-qt6
    src/main.cpp
    src/MainWindow.cpp src/MainWindow.h
    src/Backend.cpp src/Backend.h
    src/TaskPanel.cpp src/TaskPanel.h
    src/AnsiLog.cpp src/AnsiLog.h
)
target_link_libraries(claudebackup-linux-qt6 PRIVATE Qt6::Widgets)
target_compile_options(claudebackup-linux-qt6 PRIVATE -Wall -Wextra)

install(TARGETS claudebackup-linux-qt6 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES claudebackup-linux-qt6.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
