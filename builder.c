// builder.c
// Orange Pi 5 Plus Linux Kernel Builder with Mali G610 GPU Support
// 
// This file should be saved as: builder.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#define VERSION "1.0.0"
#define BUILD_DIR "/tmp/kernel_build"
#define LOG_FILE "/tmp/kernel_build.log"
#define MAX_CMD_LEN 2048
#define MAX_PATH_LEN 512

// Color codes for output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// Structure to hold configuration
typedef struct {
    char kernel_version[64];
    char build_dir[MAX_PATH_LEN];
    char cross_compile[128];
    char arch[16];
    char defconfig[64];
    int jobs;
    int verbose;
    int clean_build;
    int install_gpu_blobs;
    int enable_opencl;
    int enable_vulkan;
} build_config_t;

// Function prototypes
int execute_command(const char *cmd, int show_output);
int check_root_permissions(void);
int setup_build_environment(void);
int install_prerequisites(void);
int download_kernel_source(build_config_t *config);
int download_ubuntu_rockchip_patches(void);
int download_mali_blobs(build_config_t *config);
int install_mali_drivers(build_config_t *config);
int setup_opencl_support(build_config_t *config);
int setup_vulkan_support(build_config_t *config);
int configure_kernel(build_config_t *config);
int build_kernel(build_config_t *config);
int install_kernel(build_config_t *config);
int cleanup_build(build_config_t *config);
void print_usage(const char *program_name);
void print_header(void);
void log_message(const char *level, const char *message);
int create_directory(const char *path);
int check_dependencies(void);
int verify_gpu_installation(void);

// Global variables
FILE *log_fp = NULL;

// Logging function
void log_message(const char *level, const char *message) {
    time_t now;
    char *timestamp;
    
    time(&now);
    timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline
    
    printf("[%s%s%s] %s%s%s\n", 
           COLOR_CYAN, timestamp, COLOR_RESET,
           strcmp(level, "ERROR") == 0 ? COLOR_RED : 
           strcmp(level, "SUCCESS") == 0 ? COLOR_GREEN :
           strcmp(level, "WARNING") == 0 ? COLOR_YELLOW : COLOR_RESET,
           message, COLOR_RESET);
    
    if (log_fp) {
        fprintf(log_fp, "[%s] [%s] %s\n", timestamp, level, message);
        fflush(log_fp);
    }
}

// Execute command with logging
int execute_command(const char *cmd, int show_output) {
    char log_cmd[MAX_CMD_LEN + 100];
    int result;
    
    if (show_output) {
        printf("%s%s%s\n", COLOR_BLUE, cmd, COLOR_RESET);
        snprintf(log_cmd, sizeof(log_cmd), "%s 2>&1 | tee -a %s", cmd, LOG_FILE);
    } else {
        snprintf(log_cmd, sizeof(log_cmd), "%s >> %s 2>&1", cmd, LOG_FILE);
    }
    
    result = system(log_cmd);
    
    if (result != 0) {
        log_message("ERROR", "Command failed");
        return -1;
    }
    
    return 0;
}

// Check if running as root
int check_root_permissions(void) {
    if (geteuid() != 0) {
        log_message("ERROR", "This tool requires root privileges. Please run with sudo.");
        return -1;
    }
    return 0;
}

// Create directory if it doesn't exist
int create_directory(const char *path) {
    struct stat st = {0};
    
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg), "Failed to create directory: %s", path);
            log_message("ERROR", error_msg);
            return -1;
        }
    }
    return 0;
}

// Setup build environment
int setup_build_environment(void) {
    log_message("INFO", "Setting up build environment...");
    
    // Create build directory
    if (create_directory(BUILD_DIR) != 0) {
        return -1;
    }
    
    // Open log file
    log_fp = fopen(LOG_FILE, "a");
    if (!log_fp) {
        log_message("WARNING", "Could not open log file");
    }
    
    // Update package lists
    if (execute_command("apt update", 1) != 0) {
        log_message("ERROR", "Failed to update package lists");
        return -1;
    }
    
    log_message("SUCCESS", "Build environment setup completed");
    return 0;
}

// Install build prerequisites
int install_prerequisites(void) {
    log_message("INFO", "Installing build prerequisites...");
    
    const char *packages[] = {
        // Basic build tools
        "build-essential",
        "gcc-aarch64-linux-gnu",
        "g++-aarch64-linux-gnu",
        "libncurses-dev",
        "gawk",
        "flex",
        "bison",
        "openssl",
        "libssl-dev",
        "dkms",
        "libelf-dev",
        "libudev-dev",
        "libpci-dev",
        "libiberty-dev",
        "autoconf",
        "llvm",
        // Additional tools
        "git",
        "wget",
        "curl",
        "bc",
        "rsync",
        "kmod",
        "cpio",
        "python3",
        "python3-pip",
        "device-tree-compiler",
        // Ubuntu kernel build dependencies
        "fakeroot",
        "kernel-package",
        "pkg-config-dbgsym",
        // Mali GPU and OpenCL/Vulkan support
        "mesa-opencl-icd",
        "vulkan-tools",
        "vulkan-utils",
        "vulkan-validationlayers",
        "libvulkan-dev",
        "ocl-icd-opencl-dev",
        "opencl-headers",
        "clinfo",
        // Media and hardware acceleration
        "va-driver-all",
        "vdpau-driver-all",
        "mesa-va-drivers",
        "mesa-vdpau-drivers",
        // Development libraries
        "libegl1-mesa-dev",
        "libgles2-mesa-dev",
        "libgl1-mesa-dev",
        "libdrm-dev",
        "libgbm-dev",
        "libwayland-dev",
        "libx11-dev",
        "meson",
        "ninja-build",
        NULL
    };
    
    char cmd[MAX_CMD_LEN * 2];
    int i;
    
    // Install all packages at once
    strcpy(cmd, "DEBIAN_FRONTEND=noninteractive apt install -y");
    for (i = 0; packages[i] != NULL; i++) {
        strcat(cmd, " ");
        strcat(cmd, packages[i]);
    }
    
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to install prerequisites");
        return -1;
    }
    
    // Install additional Ubuntu kernel build dependencies
    if (execute_command("apt build-dep -y linux linux-image-unsigned-$(uname -r)", 1) != 0) {
        log_message("WARNING", "Failed to install some kernel build dependencies");
    }
    
    log_message("SUCCESS", "Prerequisites installed successfully");
    return 0;
}

// Download kernel source
int download_kernel_source(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    char source_dir[MAX_PATH_LEN];
    
    log_message("INFO", "Downloading kernel source...");
    
    snprintf(source_dir, sizeof(source_dir), "%s/linux", config->build_dir);
    
    // Change to build directory
    if (chdir(config->build_dir) != 0) {
        log_message("ERROR", "Failed to change to build directory");
        return -1;
    }
    
    // Clone Ubuntu Rockchip kernel source with Mali GPU support
    snprintf(cmd, sizeof(cmd), 
             "git clone --depth 1 --branch ubuntu-rockchip-6.8-opi5 "
             "https://github.com/Joshua-Riek/linux-rockchip.git linux");
    
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to clone Ubuntu Rockchip kernel, trying mainline...");
        
        // Fallback to mainline kernel
        snprintf(cmd, sizeof(cmd),
                 "git clone --depth 1 --branch v%s "
                 "https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux",
                 config->kernel_version);
        
        if (execute_command(cmd, 1) != 0) {
            log_message("ERROR", "Failed to download kernel source");
            return -1;
        }
    }
    
    log_message("SUCCESS", "Kernel source downloaded successfully");
    return 0;
}

// Download Ubuntu Rockchip patches
int download_ubuntu_rockchip_patches(void) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Downloading Ubuntu Rockchip patches...");
    
    // Clone Ubuntu Rockchip repository for patches and configs
    snprintf(cmd, sizeof(cmd),
             "git clone --depth 1 "
             "https://github.com/Joshua-Riek/ubuntu-rockchip.git ubuntu-rockchip");
    
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to download Ubuntu Rockchip patches");
        return 0; // Non-critical
    }
    
    log_message("SUCCESS", "Ubuntu Rockchip patches downloaded");
    return 0;
}

// Download Mali GPU blobs and libraries
int download_mali_blobs(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Downloading Mali G610 GPU blobs and libraries...");
    
    // Create Mali directory
    if (create_directory("/tmp/mali_install") != 0) {
        return -1;
    }
    
    if (chdir("/tmp/mali_install") != 0) {
        log_message("ERROR", "Failed to change to Mali install directory");
        return -1;
    }
    
    // Download Mali firmware blob
    log_message("INFO", "Downloading Mali CSF firmware...");
    snprintf(cmd, sizeof(cmd),
             "wget -O mali_csffw.bin "
             "https://github.com/JeffyCN/mirrors/raw/libmali/firmware/g610/mali_csffw.bin");
    
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to download Mali firmware");
        return -1;
    }
    
    // Download Mali userspace driver library
    log_message("INFO", "Downloading Mali userspace driver...");
    snprintf(cmd, sizeof(cmd),
             "wget -O libmali-valhall-g610-g6p0-x11-wayland-gbm.so "
             "https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-x11-wayland-gbm.so");
    
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to download Mali userspace driver");
        return -1;
    }
    
    // Download OpenCL/Vulkan capable version if requested
    if (config->enable_vulkan) {
        log_message("INFO", "Downloading Mali Vulkan-enabled driver...");
        snprintf(cmd, sizeof(cmd),
                 "wget -O libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so "
                 "https://github.com/JeffyCN/mirrors/raw/libmali/lib/aarch64-linux-gnu/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so");
        
        if (execute_command(cmd, 1) != 0) {
            log_message("WARNING", "Failed to download Mali Vulkan driver, using standard version");
        }
    }
    
    // Clone libmali repository for additional components
    log_message("INFO", "Downloading additional Mali components...");
    snprintf(cmd, sizeof(cmd),
             "git clone --depth 1 --branch libmali "
             "https://github.com/tsukumijima/libmali-rockchip.git libmali-src");
    
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to download additional Mali components");
    }
    
    log_message("SUCCESS", "Mali GPU blobs downloaded successfully");
    return 0;
}

// Install Mali drivers and setup
int install_mali_drivers(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Installing Mali G610 drivers and firmware...");
    
    // Install Mali firmware
    if (execute_command("cp /tmp/mali_install/mali_csffw.bin /lib/firmware/", 1) != 0) {
        log_message("ERROR", "Failed to install Mali firmware");
        return -1;
    }
    
    // Install Mali userspace driver
    if (execute_command("cp /tmp/mali_install/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/", 1) != 0) {
        log_message("ERROR", "Failed to install Mali userspace driver");
        return -1;
    }
    
    // Create symbolic links for different APIs
    log_message("INFO", "Creating Mali driver symbolic links...");
    
    const char *mali_links[] = {
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libMali.so",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libMali.so.1",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libmali.so",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libmali.so.1",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libEGL.so.1",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libGLESv1_CM.so.1",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libGLESv2.so.2",
        "ln -sf /usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so /usr/lib/libgbm.so.1",
        NULL
    };
    
    int i;
    for (i = 0; mali_links[i] != NULL; i++) {
        if (execute_command(mali_links[i], 0) != 0) {
            log_message("WARNING", "Failed to create some Mali symbolic links");
        }
    }
    
    // Install Vulkan driver if requested
    if (config->enable_vulkan) {
        if (access("/tmp/mali_install/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so", F_OK) == 0) {
            log_message("INFO", "Installing Mali Vulkan driver...");
            if (execute_command("cp /tmp/mali_install/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so /usr/lib/", 1) != 0) {
                log_message("WARNING", "Failed to install Mali Vulkan driver");
            } else {
                // Create Vulkan-specific links
                execute_command("ln -sf /usr/lib/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so /usr/lib/libvulkan_mali.so", 0);
            }
        }
    }
    
    // Update library cache
    if (execute_command("ldconfig", 1) != 0) {
        log_message("WARNING", "Failed to update library cache");
    }
    
    log_message("SUCCESS", "Mali drivers installed successfully");
    return 0;
}

// Setup OpenCL support
int setup_opencl_support(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    if (!config->enable_opencl) {
        return 0;
    }
    
    log_message("INFO", "Setting up OpenCL support for Mali G610...");
    
    // Create OpenCL vendor directory
    if (create_directory("/etc/OpenCL/vendors") != 0) {
        return -1;
    }
    
    // Create Mali OpenCL ICD file
    FILE *icd_file = fopen("/etc/OpenCL/vendors/mali.icd", "w");
    if (icd_file) {
        fprintf(icd_file, "/usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so\n");
        fclose(icd_file);
    } else {
        log_message("ERROR", "Failed to create Mali OpenCL ICD file");
        return -1;
    }
    
    // Set permissions
    execute_command("chmod 644 /etc/OpenCL/vendors/mali.icd", 0);
    
    log_message("SUCCESS", "OpenCL support configured successfully");
    return 0;
}

// Setup Vulkan support  
int setup_vulkan_support(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    if (!config->enable_vulkan) {
        return 0;
    }
    
    log_message("INFO", "Setting up Vulkan support for Mali G610...");
    
    // Create Vulkan ICD directory
    if (create_directory("/usr/share/vulkan/icd.d") != 0) {
        return -1;
    }
    
    // Create Mali Vulkan ICD file
    FILE *icd_file = fopen("/usr/share/vulkan/icd.d/mali.json", "w");
    if (icd_file) {
        fprintf(icd_file, "{\n");
        fprintf(icd_file, "    \"file_format_version\": \"1.0.0\",\n");
        fprintf(icd_file, "    \"ICD\": {\n");
        
        if (access("/usr/lib/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so", F_OK) == 0) {
            fprintf(icd_file, "        \"library_path\": \"/usr/lib/libmali-valhall-g610-g6p0-wayland-gbm-vulkan.so\",\n");
        } else {
            fprintf(icd_file, "        \"library_path\": \"/usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so\",\n");
        }
        
        fprintf(icd_file, "        \"api_version\": \"1.2.131\"\n");
        fprintf(icd_file, "    }\n");
        fprintf(icd_file, "}\n");
        fclose(icd_file);
    } else {
        log_message("ERROR", "Failed to create Mali Vulkan ICD file");
        return -1;
    }
    
    // Set permissions
    execute_command("chmod 644 /usr/share/vulkan/icd.d/mali.json", 0);
    
    log_message("SUCCESS", "Vulkan support configured successfully");
    return 0;
}

// Configure kernel with Mali GPU support
int configure_kernel(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    char kernel_dir[MAX_PATH_LEN];
    
    log_message("INFO", "Configuring kernel with Mali GPU support...");
    
    snprintf(kernel_dir, sizeof(kernel_dir), "%s/linux", config->build_dir);
    
    if (chdir(kernel_dir) != 0) {
        log_message("ERROR", "Failed to change to kernel directory");
        return -1;
    }
    
    // Set environment variables for cross-compilation
    setenv("ARCH", config->arch, 1);
    setenv("CROSS_COMPILE", config->cross_compile, 1);
    
    // Clean previous build artifacts if requested
    if (config->clean_build) {
        log_message("INFO", "Cleaning previous build artifacts...");
        if (execute_command("make mrproper", 1) != 0) {
            log_message("WARNING", "Failed to clean build artifacts");
        }
    }
    
    // Use Orange Pi 5 Plus specific defconfig
    snprintf(cmd, sizeof(cmd), "make %s", config->defconfig);
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to use specific defconfig, trying generic...");
        
        // Fallback to generic arm64 defconfig
        if (execute_command("make defconfig", 1) != 0) {
            log_message("ERROR", "Failed to configure kernel");
            return -1;
        }
    }
    
    // Enable RK3588 and Mali GPU specific options
    log_message("INFO", "Enabling RK3588, Mali GPU, and hardware acceleration configurations...");
    
    const char *config_options[] = {
        // Basic RK3588 support
        "CONFIG_ARCH_ROCKCHIP=y",
        "CONFIG_ARM64=y",
        "CONFIG_ROCKCHIP_RK3588=y", 
        "CONFIG_COMMON_CLK_RK808=y",
        "CONFIG_ROCKCHIP_IOMMU=y",
        "CONFIG_ROCKCHIP_PM_DOMAINS=y",
        "CONFIG_ROCKCHIP_THERMAL=y",
        
        // Display and GPU support
        "CONFIG_DRM=y",
        "CONFIG_DRM_ROCKCHIP=y",
        "CONFIG_ROCKCHIP_VOP2=y",
        "CONFIG_DRM_PANFROST=y",
        "CONFIG_DRM_PANEL_BRIDGE=y",
        "CONFIG_DRM_PANEL_SIMPLE=y",
        
        // Mali GPU kernel driver support
        "CONFIG_MALI_MIDGARD=m",
        "CONFIG_MALI_PLATFORM_NAME=\"devicetree\"",
        "CONFIG_MALI_CSF_SUPPORT=y",
        "CONFIG_MALI_DEVFREQ=y",
        "CONFIG_MALI_DMA_FENCE=y",
        
        // Memory and DMA support
        "CONFIG_DMA_CMA=y",
        "CONFIG_CMA=y",
        "CONFIG_CMA_SIZE_MBYTES=128",
        "CONFIG_DMA_SHARED_BUFFER=y",
        "CONFIG_SYNC_FILE=y",
        
        // Hardware acceleration
        "CONFIG_PHY_ROCKCHIP_INNO_USB2=y",
        "CONFIG_PHY_ROCKCHIP_NANENG_COMBO_PHY=y",
        "CONFIG_ROCKCHIP_SARADC=y",
        "CONFIG_MMC_DW_ROCKCHIP=y",
        "CONFIG_PCIE_ROCKCHIP_HOST=y",
        
        // Video codec support
        "CONFIG_STAGING_MEDIA=y",
        "CONFIG_VIDEO_ROCKCHIP_RGA=m",
        "CONFIG_VIDEO_ROCKCHIP_VDEC=m",
        "CONFIG_ROCKCHIP_VPU=y",
        "CONFIG_VIDEO_HANTRO=m",
        
        // Power management
        "CONFIG_CPU_FREQ=y",
        "CONFIG_CPU_FREQ_DEFAULT_GOV_ONDEMAND=y",
        "CONFIG_CPU_FREQ_GOV_PERFORMANCE=y",
        "CONFIG_CPU_FREQ_GOV_POWERSAVE=y",
        "CONFIG_CPU_FREQ_GOV_USERSPACE=y",
        "CONFIG_CPU_FREQ_GOV_SCHEDUTIL=y",
        "CONFIG_CPUFREQ_DT=y",
        "CONFIG_ARM_ROCKCHIP_CPUFREQ=y",
        
        // Additional GPU and graphics options
        "CONFIG_FB=y",
        "CONFIG_FB_SIMPLE=y",
        "CONFIG_LOGO=y",
        "CONFIG_LOGO_LINUX_CLUT224=y",
        
        NULL
    };
    
    FILE *config_file = fopen(".config", "a");
    if (config_file) {
        int i;
        for (i = 0; config_options[i] != NULL; i++) {
            fprintf(config_file, "%s\n", config_options[i]);
        }
        fclose(config_file);
    }
    
    // Run olddefconfig to resolve dependencies
    if (execute_command("make olddefconfig", 1) != 0) {
        log_message("WARNING", "Failed to resolve config dependencies");
    }
    
    log_message("SUCCESS", "Kernel configured successfully with Mali GPU support");
    return 0;
}

// Build kernel
int build_kernel(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Building kernel with Mali GPU support (this may take a while)...");
    
    // Set environment variables
    setenv("ARCH", config->arch, 1);
    setenv("CROSS_COMPILE", config->cross_compile, 1);
    
    // Build kernel image
    snprintf(cmd, sizeof(cmd), "make -j%d Image", config->jobs);
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to build kernel image");
        return -1;
    }
    
    // Build device tree blobs
    snprintf(cmd, sizeof(cmd), "make -j%d dtbs", config->jobs);
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to build device tree blobs");
        return -1;
    }
    
    // Build modules (including Mali GPU driver)
    snprintf(cmd, sizeof(cmd), "make -j%d modules", config->jobs);
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to build kernel modules");
        return -1;
    }
    
    log_message("SUCCESS", "Kernel built successfully with Mali GPU support");
    return 0;
}

// Install kernel
int install_kernel(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Installing kernel and Mali GPU modules...");
    
    // Install modules (including Mali GPU driver)
    if (execute_command("make modules_install", 1) != 0) {
        log_message("ERROR", "Failed to install kernel modules");
        return -1;
    }
    
    // Install device tree blobs
    if (execute_command("make dtbs_install", 1) != 0) {
        log_message("WARNING", "Failed to install device tree blobs");
    }
    
    // Copy kernel image
    snprintf(cmd, sizeof(cmd), "cp arch/arm64/boot/Image /boot/vmlinuz-%s-opi5plus-mali", config->kernel_version);
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to copy kernel image");
        return -1;
    }
    
    // Copy System.map
    snprintf(cmd, sizeof(cmd), "cp System.map /boot/System.map-%s-opi5plus-mali", config->kernel_version);
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to copy System.map");
    }
    
    // Copy config
    snprintf(cmd, sizeof(cmd), "cp .config /boot/config-%s-opi5plus-mali", config->kernel_version);
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to copy kernel config");
    }
    
    // Update initramfs
    snprintf(cmd, sizeof(cmd), "update-initramfs -c -k %s-opi5plus-mali", config->kernel_version);
    if (execute_command(cmd, 1) != 0) {
        log_message("WARNING", "Failed to update initramfs");
    }
    
    // Update bootloader
    if (execute_command("u-boot-update", 1) != 0) {
        log_message("WARNING", "Failed to update u-boot configuration");
    }
    
    log_message("SUCCESS", "Kernel installed successfully with Mali GPU support");
    return 0;
}

// Verify GPU installation and functionality
int verify_gpu_installation(void) {
    log_message("INFO", "Verifying Mali GPU installation...");
    
    // Check if Mali firmware exists
    if (access("/lib/firmware/mali_csffw.bin", F_OK) != 0) {
        log_message("ERROR", "Mali firmware not found");
        return -1;
    }
    
    // Check if Mali driver library exists
    if (access("/usr/lib/libmali-valhall-g610-g6p0-x11-wayland-gbm.so", F_OK) != 0) {
        log_message("ERROR", "Mali driver library not found");
        return -1;
    }
    
    // Test OpenCL if enabled
    if (access("/etc/OpenCL/vendors/mali.icd", F_OK) == 0) {
        log_message("INFO", "Testing OpenCL functionality...");
        if (execute_command("clinfo 2>/dev/null | grep -i mali", 0) == 0) {
            log_message("SUCCESS", "OpenCL Mali support detected");
        } else {
            log_message("WARNING", "OpenCL Mali support not detected (may need reboot)");
        }
    }
    
    // Test Vulkan if enabled
    if (access("/usr/share/vulkan/icd.d/mali.json", F_OK) == 0) {
        log_message("INFO", "Testing Vulkan functionality...");
        if (execute_command("vulkaninfo 2>/dev/null | grep -i mali", 0) == 0) {
            log_message("SUCCESS", "Vulkan Mali support detected");
        } else {
            log_message("WARNING", "Vulkan Mali support not detected (may need reboot)");
        }
    }
    
    log_message("SUCCESS", "GPU installation verification completed");
    return 0;
}

// Cleanup build artifacts
int cleanup_build(build_config_t *config) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Cleaning up build artifacts...");
    
    snprintf(cmd, sizeof(cmd), "rm -rf %s", config->build_dir);
    if (execute_command(cmd, 0) != 0) {
        log_message("WARNING", "Failed to cleanup build directory");
    }
    
    // Cleanup Mali installation directory
    if (execute_command("rm -rf /tmp/mali_install", 0) != 0) {
        log_message("WARNING", "Failed to cleanup Mali install directory");
    }
    
    log_message("SUCCESS", "Cleanup completed");
    return 0;
}

// Check system dependencies
int check_dependencies(void) {
    struct stat st;
    
    // Check if we're on Ubuntu/Debian
    if (stat("/etc/debian_version", &st) != 0) {
        log_message("ERROR", "This tool is designed for Ubuntu/Debian systems");
        return -1;
    }
    
    // Check architecture
    FILE *fp = popen("uname -m", "r");
    if (fp) {
        char arch[32];
        if (fgets(arch, sizeof(arch), fp)) {
            arch[strcspn(arch, "\n")] = '\0';
            if (strcmp(arch, "aarch64") != 0 && strcmp(arch, "x86_64") != 0) {
                log_message("WARNING", "Untested architecture detected");
            }
        }
        pclose(fp);
    }
    
    // Check available disk space (at least 10GB recommended)
    fp = popen("df /tmp | tail -1 | awk '{print $4}'", "r");
    if (fp) {
        char space[32];
        if (fgets(space, sizeof(space), fp)) {
            long available_kb = atol(space);
            if (available_kb < 10485760) { // 10GB in KB
                log_message("WARNING", "Less than 10GB free space available in /tmp");
            }
        }
        pclose(fp);
    }
    
    return 0;
}

// Print program header
void print_header(void) {
    printf("%s%s", COLOR_BOLD, COLOR_CYAN);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("    Orange Pi 5 Plus Linux Kernel Builder v%s\n", VERSION);
    printf("    Optimized for RK3588 SoC and Mali G610 GPU\n");
    printf("    Supporting Ubuntu 25.04 with Hardware Acceleration\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("%s", COLOR_RESET);
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  -v, --version <version>    Kernel version to build (default: 6.8.0)\n");
    printf("  -j, --jobs <number>        Number of parallel jobs (default: CPU cores)\n");
    printf("  -d, --build-dir <path>     Build directory (default: /tmp/kernel_build)\n");
    printf("  -c, --clean               Clean build (remove previous artifacts)\n");
    printf("  --defconfig <config>      Defconfig to use (default: rockchip_linux_defconfig)\n");
    printf("  --cross-compile <prefix>  Cross-compiler prefix (default: aarch64-linux-gnu-)\n");
    printf("  --verbose                 Verbose output\n");
    printf("  --no-install             Build only, don't install\n");
    printf("  --cleanup                Cleanup build directory after completion\n");
    printf("  --enable-gpu             Install Mali G610 GPU blobs and drivers (default: on)\n");
    printf("  --disable-gpu            Skip Mali GPU blob installation\n");
    printf("  --enable-opencl          Enable OpenCL support for Mali GPU (default: on)\n");
    printf("  --disable-opencl         Disable OpenCL support\n");
    printf("  --enable-vulkan          Enable Vulkan support for Mali GPU (default: on)\n");
    printf("  --disable-vulkan         Disable Vulkan support\n");
    printf("  --verify-gpu             Verify GPU installation after completion\n");
    printf("  -h, --help               Show this help\n\n");
    printf("Examples:\n");
    printf("  %s                                    # Build with all defaults (GPU enabled)\n", program_name);
    printf("  %s -j 8 --clean                      # Clean build with 8 jobs\n", program_name);
    printf("  %s -v 6.10.0 --no-install           # Build v6.10.0 without installing\n", program_name);
    printf("  %s --disable-gpu                     # Build without Mali GPU support\n", program_name);
    printf("  %s --disable-vulkan --enable-opencl # Build with OpenCL only\n", program_name);
    printf("\nGPU Features:\n");
    printf("  • Mali G610 hardware acceleration\n");
    printf("  • OpenCL 2.2 compute support\n");
    printf("  • Vulkan 1.2 graphics API\n");
    printf("  • Hardware video decode/encode\n");
    printf("  • EGL/OpenGL ES support\n");
}

// Main function
int main(int argc, char *argv[]) {
    build_config_t config = {
        .kernel_version = "6.8.0",
        .build_dir = BUILD_DIR,
        .cross_compile = "aarch64-linux-gnu-",
        .arch = "arm64",
        .defconfig = "rockchip_linux_defconfig",
        .jobs = 0,
        .verbose = 0,
        .clean_build = 0,
        .install_gpu_blobs = 1,  // Enable GPU by default
        .enable_opencl = 1,      // Enable OpenCL by default
        .enable_vulkan = 1       // Enable Vulkan by default
    };
    
    int no_install = 0;
    int cleanup = 0;
    int verify_gpu = 0;
    int i;
    
    print_header();
    
    // Parse command line arguments
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            if (++i < argc) {
                strncpy(config.kernel_version, argv[i], sizeof(config.kernel_version) - 1);
            }
        } else if (strcmp(argv[i], "-j") == 0 || strcmp(argv[i], "--jobs") == 0) {
            if (++i < argc) {
                config.jobs = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--build-dir") == 0) {
            if (++i < argc) {
                strncpy(config.build_dir, argv[i], sizeof(config.build_dir) - 1);
            }
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--clean") == 0) {
            config.clean_build = 1;
        } else if (strcmp(argv[i], "--defconfig") == 0) {
            if (++i < argc) {
                strncpy(config.defconfig, argv[i], sizeof(config.defconfig) - 1);
            }
        } else if (strcmp(argv[i], "--cross-compile") == 0) {
            if (++i < argc) {
                strncpy(config.cross_compile, argv[i], sizeof(config.cross_compile) - 1);
            }
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = 1;
        } else if (strcmp(argv[i], "--no-install") == 0) {
            no_install = 1;
        } else if (strcmp(argv[i], "--cleanup") == 0) {
            cleanup = 1;
        } else if (strcmp(argv[i], "--enable-gpu") == 0) {
            config.install_gpu_blobs = 1;
        } else if (strcmp(argv[i], "--disable-gpu") == 0) {
            config.install_gpu_blobs = 0;
            config.enable_opencl = 0;
            config.enable_vulkan = 0;
        } else if (strcmp(argv[i], "--enable-opencl") == 0) {
            config.enable_opencl = 1;
        } else if (strcmp(argv[i], "--disable-opencl") == 0) {
            config.enable_opencl = 0;
        } else if (strcmp(argv[i], "--enable-vulkan") == 0) {
            config.enable_vulkan = 1;
        } else if (strcmp(argv[i], "--disable-vulkan") == 0) {
            config.enable_vulkan = 0;
        } else if (strcmp(argv[i], "--verify-gpu") == 0) {
            verify_gpu = 1;
        }
    }
    
    // Set default number of jobs to CPU cores
    if (config.jobs == 0) {
        config.jobs = sysconf(_SC_NPROCESSORS_ONLN);
    }
    
    // Check dependencies and permissions
    if (check_dependencies() != 0) {
        return 1;
    }
    
    if (check_root_permissions() != 0) {
        return 1;
    }
    
    // Build process
    log_message("INFO", "Starting Orange Pi 5 Plus kernel build process with Mali GPU support");
    
    // Print configuration summary
    printf("\n%s%sBuild Configuration:%s\n", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
    printf("  Kernel Version: %s\n", config.kernel_version);
    printf("  Build Directory: %s\n", config.build_dir);
    printf("  Parallel Jobs: %d\n", config.jobs);
    printf("  Mali GPU Support: %s\n", config.install_gpu_blobs ? "Enabled" : "Disabled");
    printf("  OpenCL Support: %s\n", config.enable_opencl ? "Enabled" : "Disabled");
    printf("  Vulkan Support: %s\n", config.enable_vulkan ? "Enabled" : "Disabled");
    printf("  Clean Build: %s\n", config.clean_build ? "Yes" : "No");
    printf("\n");
    
    if (setup_build_environment() != 0) {
        goto error;
    }
    
    if (install_prerequisites() != 0) {
        goto error;
    }
    
    // Download and install Mali GPU blobs if enabled
    if (config.install_gpu_blobs) {
        if (download_mali_blobs(&config) != 0) {
            goto error;
        }
        
        if (install_mali_drivers(&config) != 0) {
            goto error;
        }
        
        if (setup_opencl_support(&config) != 0) {
            goto error;
        }
        
        if (setup_vulkan_support(&config) != 0) {
            goto error;
        }
    }
    
    if (download_kernel_source(&config) != 0) {
        goto error;
    }
    
    download_ubuntu_rockchip_patches(); // Non-critical
    
    if (configure_kernel(&config) != 0) {
        goto error;
    }
    
    if (build_kernel(&config) != 0) {
        goto error;
    }
    
    if (!no_install) {
        if (install_kernel(&config) != 0) {
            goto error;
        }
        
        // Verify GPU installation if requested
        if (verify_gpu && config.install_gpu_blobs) {
            verify_gpu_installation();
        }
    }
    
    if (cleanup) {
        cleanup_build(&config);
    }
    
    log_message("SUCCESS", "Kernel build process completed successfully!");
    
    printf("\n%s%sNext steps:%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("1. Reboot your Orange Pi 5 Plus\n");
    printf("2. Select the new kernel from the boot menu\n");
    printf("3. Verify with: uname -r\n");
    
    if (config.install_gpu_blobs) {
        printf("\n%s%sMali GPU Features Available:%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
        printf("• Hardware-accelerated graphics rendering\n");
        printf("• OpenCL 2.2 compute support (test with: clinfo)\n");
        printf("• Vulkan 1.2 graphics API (test with: vulkaninfo)\n");
        printf("• Hardware video decode/encode acceleration\n");
        printf("• EGL and OpenGL ES support\n");
        
        printf("\n%s%sGPU Testing Commands:%s\n", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
        printf("• Check OpenCL: clinfo | grep -i mali\n");
        printf("• Check Vulkan: vulkaninfo | grep -i mali\n");
        printf("• Check EGL: eglinfo | grep -i mali\n");
        printf("• GPU memory: cat /sys/kernel/debug/dri/*/gpu_memory\n");
        printf("• GPU load: cat /sys/class/devfreq/fb000000.gpu/load\n");
    }
    
    printf("\n");
    
    if (log_fp) {
        fclose(log_fp);
    }
    
    return 0;
    
error:
    log_message("ERROR", "Kernel build process failed!");
    printf("\n%s%sTroubleshooting:%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    printf("• Check the build log: %s\n", LOG_FILE);
    printf("• Ensure you have sufficient disk space (>10GB)\n");
    printf("• Verify your internet connection for downloads\n");
    printf("• Try running with --clean flag\n");
    printf("• For GPU issues, try --disable-gpu flag\n");
    
    if (log_fp) {
        fclose(log_fp);
    }
    return 1;
}
