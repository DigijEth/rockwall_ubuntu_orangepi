#!/bin/bash

# Orange Pi 5 Plus Kernel Builder Installation Script
# This script sets up the build environment and installs the kernel builder tool

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Configuration
INSTALL_DIR="/usr/local/bin"
SOURCE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TOOL="orangepi-kernel-builder"

# Logging function
log() {
    local level=$1
    shift
    local message="$*"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    case $level in
        "INFO")
            echo -e "${CYAN}[$timestamp]${NC} ${message}"
            ;;
        "SUCCESS")
            echo -e "${GREEN}[$timestamp]${NC} ${BOLD}${message}${NC}"
            ;;
        "WARNING")
            echo -e "${YELLOW}[$timestamp]${NC} ${message}"
            ;;
        "ERROR")
            echo -e "${RED}[$timestamp]${NC} ${BOLD}${message}${NC}"
            ;;
    esac
}

# Check if running as root
check_root() {
    if [[ $EUID -eq 0 ]]; then
        log "ERROR" "This installation script should NOT be run as root."
        log "INFO" "Please run as a regular user. The script will use sudo when needed."
        exit 1
    fi
}

# Check system requirements
check_system() {
    log "INFO" "Checking system requirements..."
    
    # Check if running on Ubuntu/Debian
    if ! command -v apt &> /dev/null; then
        log "ERROR" "This script is designed for Ubuntu/Debian systems with apt package manager."
        exit 1
    fi
    
    # Check Ubuntu version
    if [[ -f /etc/os-release ]]; then
        source /etc/os-release
        log "INFO" "Detected: $PRETTY_NAME"
        
        # Check if it's Ubuntu 24.04 or later
        if [[ "$ID" == "ubuntu" ]]; then
            version_major=$(echo "$VERSION_ID" | cut -d. -f1)
            version_minor=$(echo "$VERSION_ID" | cut -d. -f2)
            
            if [[ $version_major -lt 24 ]] || [[ $version_major -eq 24 && $version_minor -lt 4 ]]; then
                log "WARNING" "Ubuntu 24.04 or later is recommended for best compatibility."
            fi
        fi
    fi
    
    # Check available disk space
    available_space=$(df /tmp | awk 'NR==2 {print $4}')
    required_space=10485760  # 10GB in KB
    
    if [[ $available_space -lt $required_space ]]; then
        log "WARNING" "Less than 10GB free space available. Kernel compilation requires significant disk space."
    fi
    
    # Check if we're on the target architecture or cross-compiling
    arch=$(uname -m)
    case $arch in
        "aarch64")
            log "INFO" "Running on ARM64 architecture (native compilation)"
            ;;
        "x86_64")
            log "INFO" "Running on x86_64 architecture (cross-compilation mode)"
            ;;
        *)
            log "WARNING" "Untested architecture: $arch"
            ;;
    esac
    
    log "SUCCESS" "System requirements check completed"
}

# Install basic dependencies
install_dependencies() {
    log "INFO" "Installing basic dependencies..."
    
    # Update package lists
    sudo apt update
    
    # Install basic build tools
    sudo apt install -y \
        build-essential \
        gcc \
        g++ \
        make \
        git \
        wget \
        curl \
        sudo
    
    log "SUCCESS" "Basic dependencies installed"
}

# Compile the kernel builder tool
compile_tool() {
    log "INFO" "Compiling Orange Pi Kernel Builder..."
    
    cd "$SOURCE_DIR"
    
    # Check if source file exists
    if [[ ! -f "orangepi_kernel_builder.c" ]]; then
        log "ERROR" "Source file orangepi_kernel_builder.c not found in $SOURCE_DIR"
        exit 1
    fi
    
    # Compile the tool
    if gcc -Wall -Wextra -O2 -std=c99 -o "$BUILD_TOOL" orangepi_kernel_builder.c; then
        log "SUCCESS" "Compilation completed successfully"
    else
        log "ERROR" "Compilation failed"
        exit 1
    fi
}

# Install the kernel builder tool
install_tool() {
    log "INFO" "Installing Orange Pi Kernel Builder to $INSTALL_DIR..."
    
    # Copy the compiled binary
    sudo cp "$SOURCE_DIR/$BUILD_TOOL" "$INSTALL_DIR/"
    sudo chmod +x "$INSTALL_DIR/$BUILD_TOOL"
    
    # Verify installation
    if command -v "$BUILD_TOOL" &> /dev/null; then
        log "SUCCESS" "Orange Pi Kernel Builder installed successfully"
        log "INFO" "You can now run '$BUILD_TOOL' from anywhere"
    else
        log "ERROR" "Installation verification failed"
        exit 1
    fi
}

# Create desktop entry (optional)
create_desktop_entry() {
    local desktop_dir="$HOME/.local/share/applications"
    local desktop_file="$desktop_dir/orangepi-kernel-builder.desktop"
    
    if [[ -d "$HOME/.local/share/applications" ]] || mkdir -p "$desktop_dir"; then
        log "INFO" "Creating desktop entry..."
        
        cat > "$desktop_file" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Orange Pi Kernel Builder
Comment=Build optimized Linux kernels for Orange Pi 5 Plus with Mali GPU support
Exec=gnome-terminal -- sudo $INSTALL_DIR/$BUILD_TOOL
Icon=applications-development
Terminal=true
Categories=Development;System;
Keywords=kernel;build;orangepi;mali;gpu;
EOF
        
        chmod +x "$desktop_file"
        log "SUCCESS" "Desktop entry created"
    else
        log "WARNING" "Could not create desktop entry"
    fi
}

# Setup completion and aliases
setup_shell_integration() {
    log "INFO" "Setting up shell integration..."
    
    # Create bash completion
    local completion_dir="/etc/bash_completion.d"
    local completion_file="$completion_dir/orangepi-kernel-builder"
    
    if [[ -d "$completion_dir" ]]; then
        sudo tee "$completion_file" > /dev/null << 'EOF'
# Orange Pi Kernel Builder bash completion

_orangepi_kernel_builder() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    opts="--help --version --jobs --build-dir --clean --defconfig --cross-compile 
          --verbose --no-install --cleanup --enable-gpu --disable-gpu 
          --enable-opencl --disable-opencl --enable-vulkan --disable-vulkan 
          --verify-gpu"
    
    case ${prev} in
        --version|-v)
            COMPREPLY=( $(compgen -W "6.8.0 6.9.0 6.10.0" -- ${cur}) )
            return 0
            ;;
        --jobs|-j)
            COMPREPLY=( $(compgen -W "1 2 4 8 16" -- ${cur}) )
            return 0
            ;;
        --build-dir|-d)
            COMPREPLY=( $(compgen -d -- ${cur}) )
            return 0
            ;;
        --defconfig)
            COMPREPLY=( $(compgen -W "rockchip_linux_defconfig defconfig" -- ${cur}) )
            return 0
            ;;
        *)
            ;;
    esac
    
    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
    return 0
}

complete -F _orangepi_kernel_builder orangepi-kernel-builder
EOF
        log "SUCCESS" "Bash completion installed"
    fi
    
    # Add alias suggestions to user's shell rc files
    local shell_rc=""
    if [[ -n "$BASH_VERSION" ]]; then
        shell_rc="$HOME/.bashrc"
    elif [[ -n "$ZSH_VERSION" ]]; then
        shell_rc="$HOME/.zshrc"
    fi
    
    if [[ -n "$shell_rc" && -f "$shell_rc" ]]; then
        if ! grep -q "orangepi-kernel-builder" "$shell_rc"; then
            log "INFO" "Adding helpful aliases to $shell_rc..."
            cat >> "$shell_rc" << EOF

# Orange Pi Kernel Builder aliases
alias opi-build='sudo orangepi-kernel-builder'
alias opi-build-clean='sudo orangepi-kernel-builder --clean'
alias opi-build-quick='sudo orangepi-kernel-builder --no-install'
alias opi-build-nogpu='sudo orangepi-kernel-builder --disable-gpu'
EOF
            log "SUCCESS" "Shell aliases added. Run 'source $shell_rc' to activate them."
        fi
    fi
}

# Verify installation
verify_installation() {
    log "INFO" "Verifying installation..."
    
    # Check if tool is accessible
    if command -v "$BUILD_TOOL" &> /dev/null; then
        local version_output
        version_output=$("$BUILD_TOOL" --help | head -n 5)
        log "SUCCESS" "Tool is accessible and working"
        echo -e "${BLUE}$version_output${NC}"
    else
        log "ERROR" "Tool is not accessible in PATH"
        return 1
    fi
    
    # Check basic system requirements for kernel building
    local missing_tools=()
    
    for tool in gcc make git wget; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done
    
    if [[ ${#missing_tools[@]} -gt 0 ]]; then
        log "WARNING" "Missing tools: ${missing_tools[*]}"
        log "INFO" "These will be installed when you run the kernel builder"
    else
        log "SUCCESS" "All basic tools are available"
    fi
}

# Show usage information
show_usage_info() {
    log "SUCCESS" "Installation completed successfully!"
    echo
    echo -e "${BOLD}${CYAN}Orange Pi 5 Plus Kernel Builder${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo
    echo -e "${BOLD}Quick Start:${NC}"
    echo -e "  ${GREEN}sudo orangepi-kernel-builder${NC}                    # Build with all defaults"
    echo -e "  ${GREEN}sudo orangepi-kernel-builder --clean${NC}            # Clean build"
    echo -e "  ${GREEN}sudo orangepi-kernel-builder --help${NC}             # Show all options"
    echo
    echo -e "${BOLD}Features:${NC}"
    echo -e "  • Optimized for Orange Pi 5 Plus (RK3588)"
    echo -e "  • Mali G610 GPU hardware acceleration"
    echo -e "  • OpenCL 2.2 compute support"
    echo -e "  • Vulkan 1.2 graphics API"
    echo -e "  • Hardware video decode/encode"
    echo -e "  • Ubuntu 25.04 integration"
    echo
    echo -e "${BOLD}Aliases available:${NC}"
    echo -e "  ${YELLOW}opi-build${NC}        # sudo orangepi-kernel-builder"
    echo -e "  ${YELLOW}opi-build-clean${NC}  # sudo orangepi-kernel-builder --clean"
    echo -e "  ${YELLOW}opi-build-quick${NC}  # sudo orangepi-kernel-builder --no-install"
    echo -e "  ${YELLOW}opi-build-nogpu${NC}  # sudo orangepi-kernel-builder --disable-gpu"
    echo
    echo -e "${BOLD}${YELLOW}Important Notes:${NC}"
    echo -e "  • Ensure you have at least 10GB free disk space"
    echo -e "  • Kernel compilation can take 30-60 minutes"
    echo -e "  • GPU drivers require a reboot to take effect"
    echo -e "  • Always backup your system before installing a custom kernel"
    echo
}

# Main installation function
main() {
    echo -e "${BOLD}${CYAN}Orange Pi 5 Plus Kernel Builder - Installation Script${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo
    
    check_root
    check_system
    install_dependencies
    compile_tool
    install_tool
    create_desktop_entry
    setup_shell_integration
    verify_installation
    show_usage_info
    
    echo -e "${BOLD}${GREEN}Installation completed successfully!${NC}"
    echo -e "Run ${YELLOW}'sudo orangepi-kernel-builder --help'${NC} to get started."
}

# Handle script interruption
cleanup_on_exit() {
    local exit_code=$?
    if [[ $exit_code -ne 0 ]]; then
        log "ERROR" "Installation failed with exit code $exit_code"
        echo -e "${BOLD}${RED}Installation failed!${NC}"
        echo
        echo -e "${BOLD}Troubleshooting:${NC}"
        echo -e "  • Check your internet connection"
        echo -e "  • Ensure you have sufficient disk space"
        echo -e "  • Try running the script again"
        echo -e "  • Check the system requirements"
    fi
}

# Set up signal handlers
trap cleanup_on_exit EXIT
trap 'log "ERROR" "Installation interrupted"; exit 130' INT TERM

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --help|-h)
                echo "Orange Pi 5 Plus Kernel Builder Installation Script"
                echo
                echo "Usage: $0 [OPTIONS]"
                echo
                echo "Options:"
                echo "  --help, -h          Show this help message"
                echo "  --no-desktop        Skip desktop entry creation"
                echo "  --no-shell          Skip shell integration setup"
                echo "  --install-dir DIR   Installation directory (default: /usr/local/bin)"
                echo
                echo "This script will:"
                echo "  1. Check system requirements"
                echo "  2. Install basic dependencies"
                echo "  3. Compile the kernel builder tool"
                echo "  4. Install the tool system-wide"
                echo "  5. Setup shell integration and desktop entry"
                echo
                exit 0
                ;;
            --no-desktop)
                SKIP_DESKTOP=1
                shift
                ;;
            --no-shell)
                SKIP_SHELL=1
                shift
                ;;
            --install-dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            *)
                log "ERROR" "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# Modified main function to handle arguments
main() {
    # Parse command line arguments
    parse_args "$@"
    
    echo -e "${BOLD}${CYAN}Orange Pi 5 Plus Kernel Builder - Installation Script${NC}"
    echo -e "${CYAN}═══════════════════════════════════════════════════════════════${NC}"
    echo
    
    check_root
    check_system
    install_dependencies
    compile_tool
    install_tool
    
    if [[ -z "$SKIP_DESKTOP" ]]; then
        create_desktop_entry
    fi
    
    if [[ -z "$SKIP_SHELL" ]]; then
        setup_shell_integration
    fi
    
    verify_installation
    show_usage_info
    
    echo -e "${BOLD}${GREEN}Installation completed successfully!${NC}"
    echo -e "Run ${YELLOW}'sudo orangepi-kernel-builder --help'${NC} to get started."
}

# Only run main if script is executed directly (not sourced)
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
