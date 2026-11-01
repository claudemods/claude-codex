CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pthread
LDLIBS   := -pthread
PREFIX   ?= /usr

TARGET  := claudebackup-linux
HDRS    := $(wildcard src/*.h)
VERSION := $(shell cat version.txt 2>/dev/null)

# The Arch ISO tree zip, embedded by src/resources.h with .incbin
ZIPS   := resources/build-image-arch-img.zip

$(TARGET): src/main.cpp $(HDRS) $(ZIPS) version.txt
	$(CXX) $(CXXFLAGS) -DCBL_VERSION='"$(VERSION)"' -o $@ src/main.cpp $(LDLIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	install -d $(DESTDIR)/etc/$(TARGET)
	install -Dm644 config/custom.exclude.list $(DESTDIR)/etc/$(TARGET)/custom.exclude.list
	# pkexec rule for the Qt6 app: one password prompt, then kept for a few minutes
	install -Dm644 config/org.claudemods.claudebackup.policy $(DESTDIR)$(PREFIX)/share/polkit-1/actions/org.claudemods.claudebackup.policy

clean:
	rm -f $(TARGET)

.PHONY: install clean
