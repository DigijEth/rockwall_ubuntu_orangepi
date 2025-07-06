// installer.c
// Orange Pi 5 Plus Cross-Platform Installer

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>

#define VERSION "1.0.0"
#define INSTALLER_NAME "orangepi-installer"
#define KERNEL_BUILDER_NAME "orangepi-kernel-builder"
#define MAX_CMD_LEN 2048
#define MAX_PATH_LEN 512
#define MAX_LINE_LEN 1024

// Color codes for cross-platform output
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

// Configuration structure
typedef struct {
    char install_dir[MAX_PATH_LEN];
    char source_dir[MAX_PATH_LEN];
    int skip_desktop;
    int skip_shell;
    int verbose;
    int force_install;
} installer_config_t;

// Function prototypes
int execute_command(const char *cmd, int show_output);
int check_system_requirements(void);
int check_root_permissions(void);
int detect_package_manager(char *pm_name, size_t size);
int install_build_dependencies(const char *package_manager);
int compile_kernel_builder(installer_config_t *config);
int install_kernel_builder(installer_config_t *config);
int create_desktop_entry(installer_config_t *config);
int setup_shell_integration(installer_config_t *config);
int create_completion_file(installer_config_t *config);
int verify_installation(installer_config_t *config);
void print_usage(const char *program_name);
void print_header(void);
void log_message(const char *level, const char *message);
int create_directory(const char *path);
int file_exists(const char *path);
int copy_file(const char *src, const char *dest);
int write_file(const char *path, const char *content);
char* get_current_directory(void);
int check_disk_space(const char *path, long required_mb);

// Global variables
FILE *log_fp = NULL;

// Cross-platform logging function
void log_message(const char *level, const char *message) {
    time_t now;
    char *timestamp;
    
    time(&now);
    timestamp = ctime(&now);
    if (timestamp) {
        timestamp[strlen(timestamp) - 1] = '\0'; // Remove newline
    }
    
    printf("[%s%s%s] %s%s%s\n", 
           COLOR_CYAN, timestamp ? timestamp : "Unknown", COLOR_RESET,
           strcmp(level, "ERROR") == 0 ? COLOR_RED : 
           strcmp(level, "SUCCESS") == 0 ? COLOR_GREEN :
           strcmp(level, "WARNING") == 0 ? COLOR_YELLOW : COLOR_RESET,
           message, COLOR_RESET);
    
    if (log_fp) {
        fprintf(log_fp, "[%s] [%s] %s\n", timestamp ? timestamp : "Unknown", level, message);
        fflush(log_fp);
    }
}

// Cross-platform command execution
int execute_command(const char *cmd, int show_output) {
    int result;
    
    if (show_output) {
        printf("%s%s%s\n", COLOR_BLUE, cmd, COLOR_RESET);
    }
    
    result = system(cmd);
    
    if (result != 0) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Command failed with exit code %d: %s", result, cmd);
        log_message("ERROR", error_msg);
        return -1;
    }
    
    return 0;
}

// Check if running as root
int check_root_permissions(void) {
    if (geteuid() != 0) {
        log_message("ERROR", "This installer requires root privileges. Please run with sudo.");
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
            snprintf(error_msg, sizeof(error_msg), "Failed to create directory: %s (%s)", path, strerror(errno));
            log_message("ERROR", error_msg);
            return -1;
        }
    }
    return 0;
}

// Check if file exists
int file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0);
}

// Copy file from source to destination
int copy_file(const char *src, const char *dest) {
    FILE *source, *target;
    char buffer[4096];
    size_t bytes;
    
    source = fopen(src, "rb");
    if (!source) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot open source file: %s", src);
        log_message("ERROR", error_msg);
        return -1;
    }
    
    target = fopen(dest, "wb");
    if (!target) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot create destination file: %s", dest);
        log_message("ERROR", error_msg);
        fclose(source);
        return -1;
    }
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes, target) != bytes) {
            log_message("ERROR", "Error writing to destination file");
            fclose(source);
            fclose(target);
            return -1;
        }
    }
    
    fclose(source);
    fclose(target);
    
    // Set executable permissions
    chmod(dest, 0755);
    
    return 0;
}

// Write content to file
int write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "w");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot create file: %s", path);
        log_message("ERROR", error_msg);
        return -1;
    }
    
    if (fputs(content, file) == EOF) {
        log_message("ERROR", "Error writing to file");
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

// Get current directory
char* get_current_directory(void) {
    static char cwd[MAX_PATH_LEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return cwd;
    }
    return NULL;
}

// Check available disk space
int check_disk_space(const char *path, long required_mb) {
    char cmd[MAX_CMD_LEN];
    FILE *fp;
    long available_kb = 0;
    
    snprintf(cmd, sizeof(cmd), "df %s | tail -1 | awk '{print $4}'", path);
    fp = popen(cmd, "r");
    
    if (fp) {
        char space_str[32];
        if (fgets(space_str, sizeof(space_str), fp)) {
            available_kb = atol(space_str);
        }
        pclose(fp);
    }
    
    long required_kb = required_mb * 1024;
    if (available_kb < required_kb) {
        char warning_msg[256];
        snprintf(warning_msg, sizeof(warning_msg), 
                "Warning: Only %ld MB available, %ld MB recommended", 
                available_kb / 1024, required_mb);
        log_message("WARNING", warning_msg);
        return 0; // Warning, not error
    }
    
    return 1;
}

// Detect package manager
int detect_package_manager(char *pm_name, size_t size) {
    if (system("which apt >/dev/null 2>&1") == 0) {
        strncpy(pm_name, "apt", size - 1);
        return 0;
    } else if (system("which yum >/dev/null 2>&1") == 0) {
        strncpy(pm_name, "yum", size - 1);
        return 0;
    } else if (system("which dnf >/dev/null 2>&1") == 0) {
        strncpy(pm_name, "dnf", size - 1);
        return 0;
    } else if (system("which pacman >/dev/null 2>&1") == 0) {
        strncpy(pm_name, "pacman", size - 1);
        return 0;
    } else if (system("which zypper >/dev/null 2>&1") == 0) {
        strncpy(pm_name, "zypper", size - 1);
        return 0;
    }
    
    log_message("ERROR", "No supported package manager found (apt, yum, dnf, pacman, zypper)");
    return -1;
}

// Check system requirements
int check_system_requirements(void) {
    char arch[64];
    FILE *fp;
    
    log_message("INFO", "Checking system requirements...");
    
    // Check architecture
    fp = popen("uname -m", "r");
    if (fp) {
        if (fgets(arch, sizeof(arch), fp)) {
            arch[strcspn(arch, "\n")] = '\0';
            char arch_msg[128];
            snprintf(arch_msg, sizeof(arch_msg), "Detected architecture: %s", arch);
            log_message("INFO", arch_msg);
            
            if (strcmp(arch, "aarch64") != 0 && strcmp(arch, "x86_64") != 0) {
                log_message("WARNING", "Untested architecture detected");
            }
        }
        pclose(fp);
    }
    
    // Check OS
    if (file_exists("/etc/os-release")) {
        log_message("INFO", "Linux system detected");
    } else {
        log_message("WARNING", "Operating system may not be fully supported");
    }
    
    // Check disk space (10GB recommended)
    check_disk_space("/tmp", 10240);
    
    log_message("SUCCESS", "System requirements check completed");
    return 0;
}

// Install build dependencies
int install_build_dependencies(const char *package_manager) {
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Installing build dependencies...");
    
    // Update package lists first
    if (strcmp(package_manager, "apt") == 0) {
        if (execute_command("apt update", 1) != 0) {
            log_message("WARNING", "Failed to update package lists");
        }
        
        snprintf(cmd, sizeof(cmd), 
                "DEBIAN_FRONTEND=noninteractive apt install -y "
                "build-essential gcc g++ make git wget curl sudo "
                "libncurses-dev flex bison openssl libssl-dev");
    } else if (strcmp(package_manager, "yum") == 0 || strcmp(package_manager, "dnf") == 0) {
        snprintf(cmd, sizeof(cmd), 
                "%s install -y "
                "gcc gcc-c++ make git wget curl sudo "
                "ncurses-devel flex bison openssl-devel", package_manager);
    } else if (strcmp(package_manager, "pacman") == 0) {
        snprintf(cmd, sizeof(cmd), 
                "pacman -S --noconfirm "
                "base-devel git wget curl sudo "
                "ncurses flex bison openssl");
    } else if (strcmp(package_manager, "zypper") == 0) {
        snprintf(cmd, sizeof(cmd), 
                "zypper install -y "
                "gcc gcc-c++ make git wget curl sudo "
                "ncurses-devel flex bison openssl-devel");
    } else {
        log_message("ERROR", "Unsupported package manager");
        return -1;
    }
    
    if (execute_command(cmd, 1) != 0) {
        log_message("ERROR", "Failed to install build dependencies");
        return -1;
    }
    
    log_message("SUCCESS", "Build dependencies installed successfully");
    return 0;
}

// Compile the kernel builder
int compile_kernel_builder(installer_config_t *config) {
    char cmd[MAX_CMD_LEN];
    char source_file[MAX_PATH_LEN];
    char binary_file[MAX_PATH_LEN];
    
    log_message("INFO", "Compiling Orange Pi Kernel Builder...");
    
    // Construct paths
    snprintf(source_file, sizeof(source_file), "%s/builder.c", config->source_dir);
    snprintf(binary_file, sizeof(binary_file), "%s/%s", config->source_dir, KERNEL_BUILDER_NAME);
    
    // Check if source file exists
    if (!file_exists(source_file)) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Source file not found: %s", source_file);
        log_message("ERROR", error_msg);
        return -1;
    }
    
    // Compile with appropriate flags
    snprintf(cmd, sizeof(cmd), 
            "gcc -Wall -Wextra -O2 -std=c99 -o %s %s",
            binary_file, source_file);
    
    if (execute_command(cmd, config->verbose) != 0) {
        log_message("ERROR", "Compilation failed");
        return -1;
    }
    
    // Verify compilation
    if (!file_exists(binary_file)) {
        log_message("ERROR", "Compiled binary not found");
        return -1;
    }
    
    log_message("SUCCESS", "Compilation completed successfully");
    return 0;
}

// Install the kernel builder
int install_kernel_builder(installer_config_t *config) {
    char source_path[MAX_PATH_LEN];
    char dest_path[MAX_PATH_LEN];
    
    log_message("INFO", "Installing Orange Pi Kernel Builder...");
    
    // Create installation directory
    if (create_directory(config->install_dir) != 0) {
        return -1;
    }
    
    // Construct paths
    snprintf(source_path, sizeof(source_path), "%s/%s", config->source_dir, KERNEL_BUILDER_NAME);
    snprintf(dest_path, sizeof(dest_path), "%s/%s", config->install_dir, KERNEL_BUILDER_NAME);
    
    // Copy the binary
    if (copy_file(source_path, dest_path) != 0) {
        return -1;
    }
    
    // Verify installation
    if (!file_exists(dest_path)) {
        log_message("ERROR", "Installation verification failed");
        return -1;
    }
    
    log_message("SUCCESS", "Orange Pi Kernel Builder installed successfully");
    return 0;
}

// Create desktop entry
int create_desktop_entry(installer_config_t *config) {
    char desktop_dir[MAX_PATH_LEN];
    char desktop_file[MAX_PATH_LEN];
    char content[2048];
    char *home_dir;
    
    if (config->skip_desktop) {
        return 0;
    }
    
    log_message("INFO", "Creating desktop entry...");
    
    // Get home directory (try multiple methods for cross-platform compatibility)
    home_dir = getenv("HOME");
    if (!home_dir) {
        home_dir = getenv("USERPROFILE"); // Windows fallback
    }
    if (!home_dir) {
        log_message("WARNING", "Cannot determine home directory, skipping desktop entry");
        return 0;
    }
    
    snprintf(desktop_dir, sizeof(desktop_dir), "%s/.local/share/applications", home_dir);
    snprintf(desktop_file, sizeof(desktop_file), "%s/orangepi-kernel-builder.desktop", desktop_dir);
    
    // Create desktop directory
    if (create_directory(desktop_dir) != 0) {
        log_message("WARNING", "Cannot create desktop directory");
        return 0;
    }
    
    // Create desktop entry content
    snprintf(content, sizeof(content),
            "[Desktop Entry]\n"
            "Version=1.0\n"
            "Type=Application\n"
            "Name=Orange Pi Kernel Builder\n"
            "Comment=Build optimized Linux kernels for Orange Pi 5 Plus with Mali GPU support\n"
            "Exec=x-terminal-emulator -e sudo %s/%s\n"
            "Icon=applications-development\n"
            "Terminal=true\n"
            "Categories=Development;System;\n"
            "Keywords=kernel;build;orangepi;mali;gpu;\n",
            config->install_dir, KERNEL_BUILDER_NAME);
    
    if (write_file(desktop_file, content) != 0) {
        log_message("WARNING", "Failed to create desktop entry");
        return 0;
    }
    
    // Set executable permissions
    chmod(desktop_file, 0755);
    
    log_message("SUCCESS", "Desktop entry created");
    return 0;
}

// Create bash completion file
int create_completion_file(installer_config_t *config) {
    char completion_file[MAX_PATH_LEN];
    char content[2048];
    
    log_message("INFO", "Creating bash completion...");
    
    snprintf(completion_file, sizeof(completion_file), "/etc/bash_completion.d/%s", KERNEL_BUILDER_NAME);
    
    // Create completion content
    snprintf(content, sizeof(content),
            "# Orange Pi Kernel Builder bash completion\n"
            "\n"
            "_%s() {\n"
            "    local cur prev opts\n"
            "    COMPREPLY=()\n"
            "    cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
            "    prev=\"${COMP_WORDS[COMP_CWORD-1]}\"\n"
            "    \n"
            "    opts=\"--help --version --jobs --build-dir --clean --defconfig --cross-compile\n"
            "          --verbose --no-install --cleanup --enable-gpu --disable-gpu\n"
            "          --enable-opencl --disable-opencl --enable-vulkan --disable-vulkan\n"
            "          --verify-gpu\"\n"
            "    \n"
            "    case ${prev} in\n"
            "        --version|-v)\n"
            "            COMPREPLY=( $(compgen -W \"6.8.0 6.9.0 6.10.0\" -- ${cur}) )\n"
            "            return 0\n"
            "            ;;\n"
            "        --jobs|-j)\n"
            "            COMPREPLY=( $(compgen -W \"1 2 4 8 16\" -- ${cur}) )\n"
            "            return 0\n"
            "            ;;\n"
            "        --build-dir|-d)\n"
            "            COMPREPLY=( $(compgen -d -- ${cur}) )\n"
            "            return 0\n"
            "            ;;\n"
            "        *)\n"
            "            ;;\n"
            "    esac\n"
            "    \n"
            "    COMPREPLY=( $(compgen -W \"${opts}\" -- ${cur}) )\n"
            "    return 0\n"
            "}\n"
            "\n"
            "complete -F _%s %s\n",
            KERNEL_BUILDER_NAME, KERNEL_BUILDER_NAME, KERNEL_BUILDER_NAME);
    
    if (write_file(completion_file, content) != 0) {
        log_message("WARNING", "Failed to create bash completion");
        return 0;
    }
    
    chmod(completion_file, 0644);
    log_message("SUCCESS", "Bash completion installed");
    return 0;
}

// Setup shell integration
int setup_shell_integration(installer_config_t *config) {
    char shell_rc[MAX_PATH_LEN];
    char *home_dir;
    char aliases[1024];
    FILE *file;
    char line[MAX_LINE_LEN];
    int aliases_exist = 0;
    
    if (config->skip_shell) {
        return 0;
    }
    
    log_message("INFO", "Setting up shell integration...");
    
    // Create bash completion
    create_completion_file(config);
    
    // Get home directory
    home_dir = getenv("HOME");
    if (!home_dir) {
        log_message("WARNING", "Cannot determine home directory for shell integration");
        return 0;
    }
    
    // Determine shell RC file
    if (getenv("BASH_VERSION")) {
        snprintf(shell_rc, sizeof(shell_rc), "%s/.bashrc", home_dir);
    } else if (getenv("ZSH_VERSION")) {
        snprintf(shell_rc, sizeof(shell_rc), "%s/.zshrc", home_dir);
    } else {
        snprintf(shell_rc, sizeof(shell_rc), "%s/.profile", home_dir);
    }
    
    // Check if aliases already exist
    if (file_exists(shell_rc)) {
        file = fopen(shell_rc, "r");
        if (file) {
            while (fgets(line, sizeof(line), file)) {
                if (strstr(line, "orangepi-kernel-builder")) {
                    aliases_exist = 1;
                    break;
                }
            }
            fclose(file);
        }
    }
    
    if (!aliases_exist) {
        // Create aliases content
        snprintf(aliases, sizeof(aliases),
                "\n# Orange Pi Kernel Builder aliases\n"
                "alias opi-build='sudo %s/%s'\n"
                "alias opi-build-clean='sudo %s/%s --clean'\n"
                "alias opi-build-quick='sudo %s/%s --no-install'\n"
                "alias opi-build-nogpu='sudo %s/%s --disable-gpu'\n",
                config->install_dir, KERNEL_BUILDER_NAME,
                config->install_dir, KERNEL_BUILDER_NAME,
                config->install_dir, KERNEL_BUILDER_NAME,
                config->install_dir, KERNEL_BUILDER_NAME);
        
        // Append to shell RC file
        file = fopen(shell_rc, "a");
        if (file) {
            fputs(aliases, file);
            fclose(file);
            log_message("SUCCESS", "Shell aliases added");
        } else {
            log_message("WARNING", "Failed to add shell aliases");
        }
    } else {
        log_message("INFO", "Shell aliases already exist");
    }
    
    return 0;
}

// Verify installation
int verify_installation(installer_config_t *config) {
    char binary_path[MAX_PATH_LEN];
    char cmd[MAX_CMD_LEN];
    
    log_message("INFO", "Verifying installation...");
    
    snprintf(binary_path, sizeof(binary_path), "%s/%s", config->install_dir, KERNEL_BUILDER_NAME);
    
    // Check if binary exists and is executable
    if (!file_exists(binary_path)) {
        log_message("ERROR", "Binary not found in installation directory");
        return -1;
    }
    
    // Test if binary runs
    snprintf(cmd, sizeof(cmd), "%s --help > /dev/null 2>&1", binary_path);
    if (system(cmd) != 0) {
        log_message("ERROR", "Binary does not execute properly");
        return -1;
    }
    
    log_message("SUCCESS", "Installation verification completed");
    return 0;
}

// Print program header
void print_header(void) {
    printf("%s%s", COLOR_BOLD, COLOR_CYAN);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("    Orange Pi 5 Plus Kernel Builder Installer v%s\n", VERSION);
    printf("    Cross-platform installer for RK3588 with Mali GPU support\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("%s", COLOR_RESET);
}

// Print usage information
void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("Options:\n");
    printf("  --install-dir <path>     Installation directory (default: /usr/local/bin)\n");
    printf("  --source-dir <path>      Source directory (default: current directory)\n");
    printf("  --skip-desktop          Skip desktop entry creation\n");
    printf("  --skip-shell            Skip shell integration setup\n");
    printf("  --verbose               Verbose output\n");
    printf("  --force                 Force installation even if checks fail\n");
    printf("  -h, --help              Show this help\n\n");
    printf("This installer will:\n");
    printf("  1. Check system requirements and dependencies\n");
    printf("  2. Install build tools using the system package manager\n");
    printf("  3. Compile the Orange Pi Kernel Builder from source\n");
    printf("  4. Install the tool system-wide\n");
    printf("  5. Setup shell integration and desktop entry\n");
    printf("  6. Verify the installation\n\n");
    printf("Supported systems:\n");
    printf("  • Linux (Ubuntu, Debian, CentOS, RHEL, Fedora, Arch, openSUSE)\n");
    printf("  • Package managers: apt, yum, dnf, pacman, zypper\n");
    printf("  • Architectures: x86_64, aarch64 (ARM64)\n");
}

// Main function
int main(int argc, char *argv[]) {
    installer_config_t config = {
        .install_dir = "/usr/local/bin",
        .source_dir = "",
        .skip_desktop = 0,
        .skip_shell = 0,
        .verbose = 0,
        .force_install = 0
    };
    
    char package_manager[32];
    char *current_dir;
    int i;
    
    print_header();
    
    // Set default source directory to current directory
    current_dir = get_current_directory();
    if (current_dir) {
        strncpy(config.source_dir, current_dir, sizeof(config.source_dir) - 1);
    } else {
        strcpy(config.source_dir, ".");
    }
    
    // Parse command line arguments
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--install-dir") == 0) {
            if (++i < argc) {
                strncpy(config.install_dir, argv[i], sizeof(config.install_dir) - 1);
            }
        } else if (strcmp(argv[i], "--source-dir") == 0) {
            if (++i < argc) {
                strncpy(config.source_dir, argv[i], sizeof(config.source_dir) - 1);
            }
        } else if (strcmp(argv[i], "--skip-desktop") == 0) {
            config.skip_desktop = 1;
        } else if (strcmp(argv[i], "--skip-shell") == 0) {
            config.skip_shell = 1;
        } else if (strcmp(argv[i], "--verbose") == 0) {
            config.verbose = 1;
        } else if (strcmp(argv[i], "--force") == 0) {
            config.force_install = 1;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            printf("Use --help for usage information\n");
            return 1;
        }
    }
    
    // Open log file
    log_fp = fopen("/tmp/orangepi-installer.log", "a");
    
    // Check root permissions
    if (check_root_permissions() != 0) {
        goto error;
    }
    
    // Check system requirements
    if (!config.force_install && check_system_requirements() != 0) {
        goto error;
    }
    
    // Detect package manager
    if (detect_package_manager(package_manager, sizeof(package_manager)) != 0) {
        if (!config.force_install) {
            goto error;
        }
        log_message("WARNING", "Package manager detection failed, continuing anyway");
        strcpy(package_manager, "unknown");
    }
    
    char pm_msg[128];
    snprintf(pm_msg, sizeof(pm_msg), "Using package manager: %s", package_manager);
    log_message("INFO", pm_msg);
    
    // Install dependencies
    if (strcmp(package_manager, "unknown") != 0) {
        if (install_build_dependencies(package_manager) != 0) {
            if (!config.force_install) {
                goto error;
            }
            log_message("WARNING", "Dependency installation failed, continuing anyway");
        }
    }
    
    // Compile kernel builder
    if (compile_kernel_builder(&config) != 0) {
        goto error;
    }
    
    // Install kernel builder
    if (install_kernel_builder(&config) != 0) {
        goto error;
    }
    
    // Setup shell integration
    if (setup_shell_integration(&config) != 0) {
        log_message("WARNING", "Shell integration setup failed");
    }
    
    // Create desktop entry
    if (create_desktop_entry(&config) != 0) {
        log_message("WARNING", "Desktop entry creation failed");
    }
    
    // Verify installation
    if (verify_installation(&config) != 0) {
        goto error;
    }
    
    // Success message
    log_message("SUCCESS", "Orange Pi Kernel Builder installed successfully!");
    
    printf("\n%s%sInstallation Complete!%s\n", COLOR_BOLD, COLOR_GREEN, COLOR_RESET);
    printf("\n%s%sQuick Start:%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("  %ssudo %s/%s%s                    # Build with all defaults\n", COLOR_GREEN, config.install_dir, KERNEL_BUILDER_NAME, COLOR_RESET);
    printf("  %ssudo %s/%s --clean%s            # Clean build\n", COLOR_GREEN, config.install_dir, KERNEL_BUILDER_NAME, COLOR_RESET);
    printf("  %ssudo %s/%s --help%s             # Show all options\n", COLOR_GREEN, config.install_dir, KERNEL_BUILDER_NAME, COLOR_RESET);
    
    printf("\n%s%sAliases available:%s\n", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
    printf("  %sopi-build%s        # sudo %s/%s\n", COLOR_YELLOW, COLOR_RESET, config.install_dir, KERNEL_BUILDER_NAME);
    printf("  %sopi-build-clean%s  # sudo %s/%s --clean\n", COLOR_YELLOW, COLOR_RESET, config.install_dir, KERNEL_BUILDER_NAME);
    printf("  %sopi-build-quick%s  # sudo %s/%s --no-install\n", COLOR_YELLOW, COLOR_RESET, config.install_dir, KERNEL_BUILDER_NAME);
    printf("  %sopi-build-nogpu%s  # sudo %s/%s --disable-gpu\n", COLOR_YELLOW, COLOR_RESET, config.install_dir, KERNEL_BUILDER_NAME);
    
    printf("\n%s%sFeatures:%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
    printf("  • Optimized for Orange Pi 5 Plus (RK3588)\n");
    printf("  • Mali G610 GPU hardware acceleration\n");
    printf("  • OpenCL 2.2 compute support\n");
    printf("  • Vulkan 1.2 graphics API\n");
    printf("  • Hardware video decode/encode\n");
    printf("  • Ubuntu 25.04 integration\n");
    
    printf("\n%s%sImportant Notes:%s\n", COLOR_BOLD, COLOR_YELLOW, COLOR_RESET);
    printf("  • Ensure you have at least 10GB free disk space\n");
    printf("  • Kernel compilation can take 30-60 minutes\n");
    printf("  • GPU drivers require a reboot to take effect\n");
    printf("  • Always backup your system before installing a custom kernel\n");
    
    if (!config.skip_shell) {
        printf("\n%s%sTo activate shell aliases, run:%s\n", COLOR_BOLD, COLOR_CYAN, COLOR_RESET);
        printf("  %ssource ~/.bashrc%s (or restart your terminal)\n", COLOR_GREEN, COLOR_RESET);
    }
    
    printf("\n");
    
    if (log_fp) {
        fclose(log_fp);
    }
    
    return 0;
    
error:
    log_message("ERROR", "Installation failed!");
    printf("\n%s%sInstallation Failed!%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    printf("\n%s%sTroubleshooting:%s\n", COLOR_BOLD, COLOR_RED, COLOR_RESET);
    printf("  • Check the installation log: /tmp/orangepi-installer.log\n");
    printf("  • Ensure you have root privileges (run with sudo)\n");
    printf("  • Verify your internet connection for package downloads\n");
    printf("  • Check that you have sufficient disk space (>1GB)\n");
    printf("  • Try running with --force flag to skip some checks\n");
    printf("  • Ensure the source file builder.c exists\n");
    
    if (log_fp) {
        fclose(log_fp);
    }
    return 1;
}
