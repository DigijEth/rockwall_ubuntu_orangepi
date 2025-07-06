# Orange Pi 5 Plus Kernel Builder Makefile
# Compiler and build configuration

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = 
TARGET = builder
INSTALLER = installer
SOURCE = builder.c
INSTALLER_SOURCE = installer.c
INSTALL_DIR = /usr/local/bin
MAN_DIR = /usr/local/share/man/man1
DOC_DIR = /usr/local/share/doc/orangepi-kernel-builder

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(SOURCE)
	@echo "Building Orange Pi 5 Plus Kernel Builder with Mali GPU support..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)
	@echo "Build completed successfully!"

# Build the installer
$(INSTALLER): $(INSTALLER_SOURCE)
	@echo "Building cross-platform installer..."
	$(CC) $(CFLAGS) -o $(INSTALLER) $(INSTALLER_SOURCE) $(LDFLAGS)
	@echo "Installer build completed!"

# Build both tools
build-all: $(TARGET) $(INSTALLER)
	@echo "All tools built successfully!"

# Install using the C-based installer (recommended)
install: $(TARGET) $(INSTALLER)
	@echo "Running cross-platform installer..."
	sudo ./$(INSTALLER)

# Alternative: install using the C installer with options
install-custom: $(TARGET) $(INSTALLER)
	@echo "Running installer with custom options..."
	sudo ./$(INSTALLER) --verbose

# Manual installation (without the installer)
install-manual: $(TARGET)
	@echo "Installing Orange Pi Kernel Builder manually..."
	sudo cp $(TARGET) $(INSTALL_DIR)/
	sudo chmod +x $(INSTALL_DIR)/$(TARGET)
	@echo "Manual installation completed!"
	@echo "You can now run 'builder' from anywhere."

# Uninstall the tool
uninstall:
	@echo "Uninstalling Orange Pi Kernel Builder..."
	sudo rm -f $(INSTALL_DIR)/$(TARGET)
	sudo rm -f $(INSTALL_DIR)/$(INSTALLER)
	sudo rm -f /etc/bash_completion.d/builder
	sudo rm -f /usr/share/vulkan/icd.d/mali.json
	sudo rm -f /etc/OpenCL/vendors/mali.icd
	@echo "Uninstallation completed!"
	@echo "Note: Kernel and Mali drivers remain installed"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET) $(INSTALLER)
	@echo "Clean completed!"

# Create a comprehensive deb package with Mali GPU support
deb: $(TARGET) $(INSTALLER)
	@echo "Creating Debian package with Mali GPU support..."
	mkdir -p package/DEBIAN
	mkdir -p package/usr/local/bin
	mkdir -p package/usr/local/share/doc/builder
	mkdir -p package/etc/bash_completion.d
	
	# Copy executables
	cp $(TARGET) package/usr/local/bin/
	cp $(INSTALLER) package/usr/local/bin/
	
	# Copy documentation
	echo "Orange Pi 5 Plus Kernel Builder" > package/usr/local/share/doc/builder/README
	echo "=================================" >> package/usr/local/share/doc/builder/README
	echo "" >> package/usr/local/share/doc/builder/README
	echo "A comprehensive cross-platform tool for building Linux kernels optimized" >> package/usr/local/share/doc/builder/README
	echo "for the Orange Pi 5 Plus with RK3588 SoC and Mali G610 GPU support." >> package/usr/local/share/doc/builder/README
	echo "" >> package/usr/local/share/doc/builder/README
	echo "Features:" >> package/usr/local/share/doc/builder/README
	echo "- Mali G610 GPU hardware acceleration" >> package/usr/local/share/doc/builder/README
	echo "- OpenCL 2.2 compute support" >> package/usr/local/share/doc/builder/README
	echo "- Vulkan 1.2 graphics API" >> package/usr/local/share/doc/builder/README
	echo "- Hardware video decode/encode" >> package/usr/local/share/doc/builder/README
	echo "- Ubuntu 25.04 integration" >> package/usr/local/share/doc/builder/README
	echo "- Cross-platform installer (C-based)" >> package/usr/local/share/doc/builder/README
	echo "" >> package/usr/local/share/doc/builder/README
	echo "Usage: sudo builder [OPTIONS]" >> package/usr/local/share/doc/builder/README
	echo "Install: sudo installer" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	
	# Create bash completion
	echo '# Orange Pi Kernel Builder bash completion' > package/etc/bash_completion.d/builder
	echo '_builder() {' >> package/etc/bash_completion.d/builder
	echo '    local cur prev opts' >> package/etc/bash_completion.d/builder
	echo '    COMPREPLY=()' >> package/etc/bash_completion.d/builder
	echo '    cur="${COMP_WORDS[COMP_CWORD]}"' >> package/etc/bash_completion.d/builder
	echo '    prev="${COMP_WORDS[COMP_CWORD-1]}"' >> package/etc/bash_completion.d/builder
	echo '    opts="--help --version --jobs --build-dir --clean --enable-gpu --disable-gpu --enable-opencl --disable-opencl --enable-vulkan --disable-vulkan"' >> package/etc/bash_completion.d/builder
	echo '    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )' >> package/etc/bash_completion.d/builder
	echo '    return 0' >> package/etc/bash_completion.d/builder
	echo '}' >> package/etc/bash_completion.d/builder
	echo 'complete -F _builder builder' >> package/etc/bash_completion.d/builder
	
	# Create control file
	echo "Package: builder" > package/DEBIAN/control
	echo "Version: 1.0.0" >> package/DEBIAN/control
	echo "Section: devel" >> package/DEBIAN/control
	echo "Priority: optional" >> package/DEBIAN/control
	echo "Architecture: arm64" >> package/DEBIAN/control
	echo "Maintainer: Orange Pi Kernel Builder <builder@orangepi.com>" >> package/DEBIAN/control
	echo "Description: Cross-platform Linux kernel builder for Orange Pi 5 Plus with Mali GPU" >> package/DEBIAN/control
	echo " A comprehensive cross-platform tool for building Linux kernels optimized for the" >> package/DEBIAN/control
	echo " Orange Pi 5 Plus with RK3588 SoC running Ubuntu 25.04. Features include Mali G610" >> package/DEBIAN/control
	echo " GPU hardware acceleration, OpenCL 2.2 compute support, Vulkan 1.2 graphics API," >> package/DEBIAN/control
	echo " hardware video decode/encode, and a cross-platform C-based installer." >> package/DEBIAN/control
	echo "Depends: build-essential, gcc, git, wget, curl" >> package/DEBIAN/control
	echo "Recommends: gcc-aarch64-linux-gnu, vulkan-tools, mesa-opencl-icd, clinfo" >> package/DEBIAN/control
	echo "Homepage: https://github.com/orangepi-kernel-builder" >> package/DEBIAN/control
	
	# Create postinst script
	echo '#!/bin/bash' > package/DEBIAN/postinst
	echo 'set -e' >> package/DEBIAN/postinst
	echo 'echo "Orange Pi Kernel Builder installed successfully!"' >> package/DEBIAN/postinst
	echo 'echo "Main tool: sudo builder --help"' >> package/DEBIAN/postinst
	echo 'echo "Installer: sudo installer --help"' >> package/DEBIAN/postinst
	echo 'echo "For Mali GPU support, run with default settings or --enable-gpu"' >> package/DEBIAN/postinst
	chmod +x package/DEBIAN/postinst
	
	# Create prerm script
	echo '#!/bin/bash' > package/DEBIAN/prerm
	echo 'set -e' >> package/DEBIAN/prerm
	echo 'echo "Removing Orange Pi Kernel Builder..."' >> package/DEBIAN/prerm
	chmod +x package/DEBIAN/prerm
	
	# Build the package
	dpkg-deb --build package builder_1.0.0_arm64.deb
	rm -rf package
	@echo "Debian package created: builder_1.0.0_arm64.deb"

# Create a quick test build
test: $(TARGET) $(INSTALLER)
	@echo "Running basic tests..."
	./$(TARGET) --help
	./$(INSTALLER) --help
	@echo "Basic tests completed!"

# Development build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET) $(INSTALLER)
	@echo "Debug build completed!"

# Static analysis (requires cppcheck)
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c99 $(SOURCE)
	cppcheck --enable=all --std=c99 $(INSTALLER_SOURCE)

# Memory check (requires valgrind)
memcheck: debug
	@echo "Running memory check on main tool..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --help
	@echo "Running memory check on installer..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(INSTALLER) --help

# Performance profiling (requires gprof)
profile: CFLAGS += -pg
profile: $(TARGET) $(INSTALLER)
	@echo "Profile build completed. Run the programs and use 'gprof' to analyze."

# Create source distribution
dist:
	@echo "Creating source distribution..."
	mkdir -p builder-1.0.0
	cp $(SOURCE) $(INSTALLER_SOURCE) Makefile README.md builder-1.0.0/
	tar -czf builder-1.0.0.tar.gz builder-1.0.0/
	rm -rf builder-1.0.0/
	@echo "Source distribution created: builder-1.0.0.tar.gz"

# Cross-compile for different architectures
cross-compile-arm64: CC=aarch64-linux-gnu-gcc
cross-compile-arm64: $(TARGET) $(INSTALLER)
	@echo "Cross-compiled for ARM64"

cross-compile-x86: CC=gcc
cross-compile-x86: $(TARGET) $(INSTALLER)
	@echo "Compiled for x86_64"

# Show build information
info:
	@echo "Orange Pi 5 Plus Kernel Builder Build Information"
	@echo "================================================="
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Main Target: $(TARGET)"
	@echo "Installer Target: $(INSTALLER)"
	@echo "Main Source: $(SOURCE)"
	@echo "Installer Source: $(INSTALLER_SOURCE)"
	@echo "Install Directory: $(INSTALL_DIR)"
	@echo ""
	@echo "Features:"
	@echo "- Cross-platform C-based installer"
	@echo "- Mali G610 GPU hardware acceleration"
	@echo "- OpenCL 2.2 compute support"
	@echo "- Vulkan 1.2 graphics API"
	@echo "- Hardware video decode/encode"
	@echo "- Ubuntu 25.04 integration"
	@echo "- Support for multiple package managers"

# Check Mali GPU prerequisites
check-mali:
	@echo "Checking Mali GPU prerequisites..."
	@echo "Checking for Mali firmware..."
	@if [ -f /lib/firmware/mali_csffw.bin ]; then \
		echo "✓ Mali firmware found"; \
	else \
		echo "✗ Mali firmware not found (will be installed during kernel build)"; \
	fi
	@echo "Checking for Mali userspace driver..."
	@if [ -f /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so ]; then \
		echo "✓ Mali userspace driver found"; \
	else \
		echo "✗ Mali userspace driver not found (will be installed during kernel build)"; \
	fi
	@echo "Checking OpenCL support..."
	@if command -v clinfo >/dev/null 2>&1; then \
		echo "✓ clinfo tool available"; \
	else \
		echo "✗ clinfo tool not found (install with package manager)"; \
	fi
	@echo "Checking Vulkan support..."
	@if command -v vulkaninfo >/dev/null 2>&1; then \
		echo "✓ vulkaninfo tool available"; \
	else \
		echo "✗ vulkaninfo tool not found (install with package manager)"; \
	fi

# Check cross-compilation support
check-cross:
	@echo "Checking cross-compilation support..."
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		echo "✓ ARM64 cross-compiler available"; \
	else \
		echo "✗ ARM64 cross-compiler not found (install gcc-aarch64-linux-gnu)"; \
	fi
	@echo "Current architecture: $$(uname -m)"
	@echo "Supported target architectures: x86_64, aarch64"

# Help target
help:
	@echo "Orange Pi 5 Plus Kernel Builder Makefile"
	@echo "========================================"
	@echo ""
	@echo "Main targets:"
	@echo "  all          - Build the kernel builder tool (default)"
	@echo "  build-all    - Build both main tool and installer"
	@echo "  install      - Install using the C-based installer (recommended)"
	@echo "  install-custom - Install with custom installer options"
	@echo "  install-manual - Install manually without the installer"
	@echo "  uninstall    - Remove the tools from system"
	@echo "  clean        - Remove build artifacts"
	@echo ""
	@echo "Package targets:"
	@echo "  deb          - Create a Debian package"
	@echo "  dist         - Create source distribution"
	@echo ""
	@echo "Development targets:"
	@echo "  test         - Run basic tests"
	@echo "  debug        - Build with debug symbols"
	@echo "  analyze      - Run static code analysis"
	@echo "  memcheck     - Run memory leak detection"
	@echo "  profile      - Build for performance profiling"
	@echo ""
	@echo "Cross-compilation targets:"
	@echo "  cross-compile-arm64 - Cross-compile for ARM64"
	@echo "  cross-compile-x86   - Compile for x86_64"
	@echo ""
	@echo "Information targets:"
	@echo "  info         - Show build information"
	@echo "  check-mali   - Check Mali GPU prerequisites"
	@echo "  check-cross  - Check cross-compilation support"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Mali GPU Features:"
	@echo "  • Hardware-accelerated graphics rendering"
	@echo "  • OpenCL 2.2 compute support"
	@echo "  • Vulkan 1.2 graphics API"
	@echo "  • Hardware video decode/encode acceleration"
	@echo "  • EGL and OpenGL ES support"
	@echo ""
	@echo "Cross-platform Support:"
	@echo "  • Works on x86_64 and ARM64 systems"
	@echo "  • Supports multiple Linux distributions"
	@echo "  • Multiple package manager support (apt, yum, dnf, pacman, zypper)"
	@echo "  • C-based installer for maximum compatibility"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build the main tool"
	@echo "  make build-all          # Build tool and installer"
	@echo "  make install            # Build and install with C installer"
	@echo "  make clean build-all install # Complete rebuild and install"
	@echo "  make deb                # Create Debian package"
	@echo "  make check-mali         # Check Mali GPU support"

# Phony targets
.PHONY: all build-all install install-custom install-manual uninstall clean deb test debug analyze memcheck profile dist cross-compile-arm64 cross-compile-x86 info check-mali check-cross help
