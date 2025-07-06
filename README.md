# Orange Pi 5 Plus Kernel Builder

A comprehensive tool for building optimized Linux kernels for the Orange Pi 5 Plus with RK3588 SoC and Mali G610 GPU hardware acceleration support.

## 🚀 Features

### Core Functionality
- **Automated kernel building** for Orange Pi 5 Plus (RK3588)
- **Ubuntu 25.04 integration** with proper package management
- **Cross-compilation support** from x86_64 to ARM64
- **Automated dependency installation** and environment setup
- **Clean build management** with artifact cleanup

### Mali G610 GPU Support
- **Hardware-accelerated graphics** rendering
- **OpenCL 2.2 compute support** for AI/ML workloads
- **Vulkan 1.2 graphics API** for modern applications
- **Hardware video decode/encode** acceleration
- **EGL and OpenGL ES support** for graphics applications

### Advanced Features
- **Firmware blob management** for Mali G610
- **Driver installation and configuration**
- **Symbolic link management** for GPU libraries
- **GPU functionality verification**
- **Detailed logging and error reporting**

## 📋 Requirements

### System Requirements
- **Operating System**: Ubuntu 24.04+ or Debian 12+ (recommended)
- **Architecture**: ARM64 (native) or x86_64 (cross-compilation)
- **Memory**: At least 4GB RAM (8GB+ recommended)
- **Disk Space**: Minimum 10GB free space
- **Network**: Internet connection for downloading sources and blobs

### Hardware Compatibility
- **Primary Target**: Orange Pi 5 Plus (RK3588)
- **Compatible**: Other RK3588-based boards (Rock 5B, etc.)
- **GPU**: Mali G610 MP4 (Valhall architecture)

## 🛠️ Installation

### Quick Installation (Recommended - C-based cross-platform)
```bash
# Clone the repository
git clone https://github.com/digijeth/orangepi-kernel-builder.git
cd orangepi-kernel-builder

# Build the installer
make orangepi-installer

# Run the cross-platform installer
sudo ./orangepi-installer
```

### Alternative Installation Methods
```bash
# Build both tools and install
make build-all install

# Install with custom options
make build-all
sudo ./orangepi-installer --verbose --skip-desktop

# Manual installation (minimal)
make
sudo make install-manual
```

### From Debian Package
```bash
# Create and install .deb package
make deb
sudo dpkg -i orangepi-kernel-builder_1.0.0_arm64.deb
```

### Cross-Platform Compatibility
The installer is written in C and supports:
- **Linux Distributions**: Ubuntu, Debian, CentOS, RHEL, Fedora, Arch Linux, openSUSE
- **Package Managers**: apt, yum, dnf, pacman, zypper
- **Architectures**: x86_64 (cross-compilation), aarch64 (native)
- **Installer Options**: `--verbose`, `--skip-desktop`, `--skip-shell`, `--force`

## 🏃 Quick Start

### Basic Usage
```bash
# Build kernel with all defaults (Mali GPU enabled)
sudo orangepi-kernel-builder

# Clean build with 8 parallel jobs
sudo orangepi-kernel-builder --clean -j 8

# Build specific kernel version
sudo orangepi-kernel-builder --version 6.10.0

# Build without installing
sudo orangepi-kernel-builder --no-install
```

### GPU-Specific Options
```bash
# Build with all GPU features (default)
sudo orangepi-kernel-builder --enable-gpu --enable-opencl --enable-vulkan

# Build without GPU support
sudo orangepi-kernel-builder --disable-gpu

# Build with OpenCL only
sudo orangepi-kernel-builder --disable-vulkan --enable-opencl

# Verify GPU installation after build
sudo orangepi-kernel-builder --verify-gpu
```

### Convenient Aliases
After installation, these aliases are available:
```bash
opi-build          # sudo orangepi-kernel-builder
opi-build-clean    # sudo orangepi-kernel-builder --clean
opi-build-quick    # sudo orangepi-kernel-builder --no-install
opi-build-nogpu    # sudo orangepi-kernel-builder --disable-gpu
```

## ⚙️ Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-v, --version <ver>` | Kernel version to build | 6.8.0 |
| `-j, --jobs <num>` | Parallel compilation jobs | CPU cores |
| `-d, --build-dir <path>` | Build directory | /tmp/kernel_build |
| `-c, --clean` | Clean build artifacts | false |
| `--defconfig <config>` | Kernel defconfig to use | rockchip_linux_defconfig |
| `--cross-compile <prefix>` | Cross-compiler prefix | aarch64-linux-gnu- |
| `--verbose` | Verbose output | false |
| `--no-install` | Build only, don't install | false |
| `--cleanup` | Cleanup after completion | false |
| `--enable-gpu` | Install Mali GPU blobs | true |
| `--disable-gpu` | Skip GPU blob installation | false |
| `--enable-opencl` | Enable OpenCL support | true |
| `--disable-opencl` | Disable OpenCL support | false |
| `--enable-vulkan` | Enable Vulkan support | true |
| `--disable-vulkan` | Disable Vulkan support | false |
| `--verify-gpu` | Verify GPU after installation | false |
| `-h, --help` | Show help message | - |

## 🎯 Mali GPU Integration Details

### Supported APIs
- **OpenGL ES 1.1/2.0/3.2** - Graphics rendering
- **OpenCL 2.2** - Compute acceleration
- **Vulkan 1.2** - Low-level graphics API
- **EGL** - Platform abstraction layer

### Installed Components
1. **Mali CSF Firmware** (`mali_csffw.bin`)
2. **Userspace Driver** (`libmali-valhall-g610-g6p0-x11-wayland-gbm.so`)
3. **OpenCL ICD Configuration** (`/etc/OpenCL/vendors/mali.icd`)
4. **Vulkan ICD Configuration** (`/usr/share/vulkan/icd.d/mali.json`)
5. **Symbolic Links** for various graphics libraries

### Verification Commands
```bash
# Check OpenCL devices
clinfo | grep -i mali

# Check Vulkan support
vulkaninfo | grep -i mali

# Check EGL support
eglinfo | grep -i mali

# Monitor GPU usage
cat /sys/class/devfreq/fb000000.gpu/load

# Check GPU memory
cat /sys/kernel/debug/dri/*/gpu_memory
```

## 🔧 Build Process Overview

1. **Environment Setup**
   - Install build dependencies
   - Setup cross-compilation tools
   - Create build directories

2. **Source Download**
   - Clone Ubuntu Rockchip kernel
   - Download Mali GPU blobs
   - Get additional patches

3. **Mali Integration**
   - Install Mali firmware
   - Setup userspace drivers
   - Configure OpenCL/Vulkan

4. **Kernel Configuration**
   - Apply RK3588-specific config
   - Enable Mali GPU support
   - Configure video acceleration

5. **Compilation**
   - Build kernel image
   - Build device tree blobs
   - Build kernel modules

6. **Installation**
   - Install kernel and modules
   - Update boot configuration
   - Verify GPU functionality

## 📊 Performance Optimization

### Kernel Features Enabled
- **CPU Frequency Scaling** with multiple governors
- **GPU DevFreq** for dynamic GPU frequency
- **CMA (Contiguous Memory Allocator)** for GPU memory
- **DMA-BUF** for efficient buffer sharing
- **Hardware Video Codecs** (H.264/H.265/VP9/AV1)

### Recommended Settings
```bash
# Set CPU governor to performance
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Set GPU governor to performance
echo performance | sudo tee /sys/class/devfreq/fb000000.gpu/governor

# Check current frequencies
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
cat /sys/class/devfreq/fb000000.gpu/cur_freq
```

## 🐛 Troubleshooting

### Common Issues

#### Build Failures
```bash
# Check available disk space
df -h /tmp

# Clean and retry
sudo orangepi-kernel-builder --clean

# Check build log
tail -f /tmp/kernel_build.log
```

#### GPU Not Working
```bash
# Check if firmware is loaded
dmesg | grep -i mali

# Verify driver installation
ls -la /usr/lib/libmali*

# Test OpenCL
clinfo

# Test Vulkan
vulkaninfo
```

#### Missing Dependencies
```bash
# Update package lists
sudo apt update

# Install missing packages manually
sudo apt install build-essential gcc-aarch64-linux-gnu

# Reinstall with clean environment
sudo orangepi-kernel-builder --clean
```

### Debug Mode
```bash
# Build with debug symbols
make debug

# Run with verbose output
sudo orangepi-kernel-builder --verbose

# Memory check (if available)
make memcheck
```

## 🔄 Development

### Building from Source
```bash
# Clone repository
git clone https://github.com/your-repo/orangepi-kernel-builder.git
cd orangepi-kernel-builder

# Build debug version
make debug

# Run static analysis
make analyze

# Create distribution
make dist
```
