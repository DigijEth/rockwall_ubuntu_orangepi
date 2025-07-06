# Orange Pi 5 Plus Kernel Builder Makefile
# Compiler and build configuration

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS = 
TARGET = orangepi-kernel-builder
SOURCE = orangepi_kernel_builder.c
INSTALL_SCRIPT = install.sh
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

# Install the tool using the installation script
install: $(TARGET) $(INSTALL_SCRIPT)
	@echo "Running installation script..."
	@chmod +x $(INSTALL_SCRIPT)
	@./$(INSTALL_SCRIPT)

# Manual installation (without the script)
install-manual: $(TARGET)
	@echo "Installing Orange Pi Kernel Builder manually..."
	sudo cp $(TARGET) $(INSTALL_DIR)/
	sudo chmod +x $(INSTALL_DIR)/$(TARGET)
	@echo "Manual installation completed!"
	@echo "You can now run 'orangepi-kernel-builder' from anywhere."

# Uninstall the tool
uninstall:
	@echo "Uninstalling Orange Pi Kernel Builder..."
	sudo rm -f $(INSTALL_DIR)/$(TARGET)
	sudo rm -f /etc/bash_completion.d/orangepi-kernel-builder
	sudo rm -f /usr/share/vulkan/icd.d/mali.json
	sudo rm -f /etc/OpenCL/vendors/mali.icd
	@echo "Uninstallation completed!"
	@echo "Note: Kernel and Mali drivers remain installed"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(TARGET)
	@echo "Clean completed!"

# Create a comprehensive deb package with Mali GPU support
deb: $(TARGET)
	@echo "Creating Debian package with Mali GPU support..."
	mkdir -p package/DEBIAN
	mkdir -p package/usr/local/bin
	mkdir -p package/usr/local/share/doc/orangepi-kernel-builder
	mkdir -p package/etc/bash_completion.d
	
	# Copy main executable
	cp $(TARGET) package/usr/local/bin/
	
	# Copy documentation
	echo "Orange Pi 5 Plus Kernel Builder" > package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "=================================" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "A comprehensive tool for building Linux kernels optimized for the" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "Orange Pi 5 Plus with RK3588 SoC and Mali G610 GPU support." >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "Features:" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "- Mali G610 GPU hardware acceleration" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "- OpenCL 2.2 compute support" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "- Vulkan 1.2 graphics API" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "- Hardware video decode/encode" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "- Ubuntu 25.04 integration" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	echo "Usage: sudo orangepi-kernel-builder [OPTIONS]" >> package/usr/local/share/doc/orangepi-kernel-builder/README
	
	# Create bash completion
	echo '# Orange Pi Kernel Builder bash completion' > package/etc/bash_completion.d/orangepi-kernel-builder
	echo '_orangepi_kernel_builder() {' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    local cur prev opts' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    COMPREPLY=()' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    cur="${COMP_WORDS[COMP_CWORD]}"' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    prev="${COMP_WORDS[COMP_CWORD-1]}"' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    opts="--help --version --jobs --build-dir --clean --enable-gpu --disable-gpu --enable-opencl --disable-opencl --enable-vulkan --disable-vulkan"' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '    return 0' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo '}' >> package/etc/bash_completion.d/orangepi-kernel-builder
	echo 'complete -F _orangepi_kernel_builder orangepi-kernel-builder' >> package/etc/bash_completion.d/orangepi-kernel-builder
	
	# Create control file
	echo "Package: orangepi-kernel-builder" > package/DEBIAN/control
	echo "Version: 1.0.0" >> package/DEBIAN/control
	echo "Section: devel" >> package/DEBIAN/control
	echo "Priority: optional" >> package/DEBIAN/control
	echo "Architecture: arm64" >> package/DEBIAN/control
	echo "Maintainer: Orange Pi Kernel Builder <builder@orangepi.com>" >> package/DEBIAN/control
	echo "Description: Linux kernel builder for Orange Pi 5 Plus with Mali GPU support" >> package/DEBIAN/control
	echo " A comprehensive tool for building Linux kernels optimized for the Orange Pi" >> package/DEBIAN/control
	echo " 5 Plus with RK3588 SoC running Ubuntu 25.04. Features include Mali G610 GPU" >> package/DEBIAN/control
	echo " hardware acceleration, OpenCL 2.2 compute support, Vulkan 1.2 graphics API," >> package/DEBIAN/control
	echo " and hardware video decode/encode capabilities." >> package/DEBIAN/control
	echo "Depends: build-essential, gcc-aarch64-linux-gnu, git, wget, curl" >> package/DEBIAN/control
	echo "Recommends: vulkan-tools, mesa-opencl-icd, clinfo" >> package/DEBIAN/control
	echo "Homepage: https://github.com/orangepi-kernel-builder" >> package/DEBIAN/control
	
	# Create postinst script for Mali GPU setup
	echo '#!/bin/bash' > package/DEBIAN/postinst
	echo 'set -e' >> package/DEBIAN/postinst
	echo 'echo "Orange Pi Kernel Builder installed successfully!"' >> package/DEBIAN/postinst
	echo 'echo "Run '\''sudo orangepi-kernel-builder --help'\'' to get started."' >> package/DEBIAN/postinst
	echo 'echo "For Mali GPU support, run with default settings or --enable-gpu"' >> package/DEBIAN/postinst
	chmod +x package/DEBIAN/postinst
	
	# Create prerm script
	echo '#!/bin/bash' > package/DEBIAN/prerm
	echo 'set -e' >> package/DEBIAN/prerm
	echo 'echo "Removing Orange Pi Kernel Builder..."' >> package/DEBIAN/prerm
	chmod +x package/DEBIAN/prerm
	
	# Build the package
	dpkg-deb --build package orangepi-kernel-builder_1.0.0_arm64.deb
	rm -rf package
	@echo "Debian package created: orangepi-kernel-builder_1.0.0_arm64.deb"

# Create installation script
$(INSTALL_SCRIPT): 
	@echo "Installation script already exists: $(INSTALL_SCRIPT)"

# Create a quick test build
test: $(TARGET)
	@echo "Running basic tests..."
	./$(TARGET) --help
	@echo "Basic tests completed!"

# Development build with debug symbols
debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)
	@echo "Debug build completed!"

# Static analysis (requires cppcheck)
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --std=c99 $(SOURCE)

# Memory check (requires valgrind)
memcheck: debug
	@echo "Running memory check..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --help

# Performance profiling (requires gprof)
profile: CFLAGS += -pg
profile: $(TARGET)
	@echo "Profile build completed. Run the program and use 'gprof' to analyze."

# Create source distribution
dist:
	@echo "Creating source distribution..."
	mkdir -p orangepi-kernel-builder-1.0.0
	cp $(SOURCE) Makefile $(INSTALL_SCRIPT) README.md orangepi-kernel-builder-1.0.0/
	tar -czf orangepi-kernel-builder-1.0.0.tar.gz orangepi-kernel-builder-1.0.0/
	rm -rf orangepi-kernel-builder-1.0.0/
	@echo "Source distribution created: orangepi-kernel-builder-1.0.0.tar.gz"

# Show build information
info:
	@echo "Orange Pi 5 Plus Kernel Builder Build Information"
	@echo "================================================="
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Target: $(TARGET)"
	@echo "Source: $(SOURCE)"
	@echo "Install Directory: $(INSTALL_DIR)"
	@echo "Install Script: $(INSTALL_SCRIPT)"
	@echo ""
	@echo "Features:"
	@echo "- Mali G610 GPU hardware acceleration"
	@echo "- OpenCL 2.2 compute support"
	@echo "- Vulkan 1.2 graphics API"
	@echo "- Hardware video decode/encode"
	@echo "- Ubuntu 25.04 integration"

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
		echo "✗ clinfo tool not found (install with: sudo apt install clinfo)"; \
	fi
	@echo "Checking Vulkan support..."
	@if command -v vulkaninfo >/dev/null 2>&1; then \
		echo "✓ vulkaninfo tool available"; \
	else \
		echo "✗ vulkaninfo tool not found (install with: sudo apt install vulkan-tools)"; \
	fi

# Help target
help:
	@echo "Orange Pi 5 Plus Kernel Builder Makefile"
	@echo "========================================"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the kernel builder tool (default)"
	@echo "  install      - Install using the installation script (recommended)"
	@echo "  install-manual - Install manually without the script"
	@echo "  uninstall    - Remove the tool from system"
	@echo "  clean        - Remove build artifacts"
	@echo "  deb          - Create a Debian package"
	@echo "  test         - Run basic tests"
	@echo "  debug        - Build with debug symbols"
	@echo "  analyze      - Run static code analysis"
	@echo "  memcheck     - Run memory leak detection"
	@echo "  profile      - Build for performance profiling"
	@echo "  dist         - Create source distribution"
	@echo "  info         - Show build information"
	@echo "  check-mali   - Check Mali GPU prerequisites"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Mali GPU Features:"
	@echo "  • Hardware-accelerated graphics rendering"
	@echo "  • OpenCL 2.2 compute support"
	@echo "  • Vulkan 1.2 graphics API"
	@echo "  • Hardware video decode/encode acceleration"
	@echo "  • EGL and OpenGL ES support"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build the tool"
	@echo "  make install            # Build and install with script"
	@echo "  make clean all install  # Clean, build, and install"
	@echo "  make deb                # Create Debian package"
	@echo "  make check-mali         # Check Mali GPU support"

# Phony targets
.PHONY: all install install-manual uninstall clean deb test debug analyze memcheck profile dist info check-mali help
