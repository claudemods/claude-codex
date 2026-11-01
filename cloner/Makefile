CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
LDLIBS   := -pthread
PREFIX   ?= /usr

TARGET  := claudebackup-linux
HDRS    := $(wildcard src/*.h)
# the suite's VERSION when the cloner is built inside the compendium, else its own version.txt
VERSION := $(shell cat ../VERSION 2>/dev/null || cat version.txt 2>/dev/null)

# The Arch ISO tree zip, embedded by src/resources.h with .incbin
ZIPS   := resources/build-image-arch-img.zip

$(TARGET): src/main.cpp $(HDRS) $(ZIPS) version.txt
	$(CXX) $(CXXFLAGS) -DCBL_VERSION='"$(VERSION)"' -o $@ src/main.cpp $(LDLIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -d $(DESTDIR)/etc/claudecodexbackup
	install -Dm644 config/custom.exclude.list $(DESTDIR)/etc/claudecodexbackup/custom.exclude.list
	# pkexec rule for Claude Codex Cloner's Backup tab: one password prompt, then kept for a few minutes
	install -Dm644 config/org.claudemods.claudebackup.policy $(DESTDIR)$(PREFIX)/share/polkit-1/actions/org.claudemods.claudebackup.policy
	# Claude Codex Backup's icon (menu entry, window, the Alpine "Install" shortcut)
	install -Dm644 config/claude-codex-backup.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/claude-codex-backup.svg

clean:
	rm -f $(TARGET)

.PHONY: install clean
