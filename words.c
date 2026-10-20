#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

// Forward declarations
void process_path(const char *path, const char *suffix);
void process_directory(const char *dir_path, const char *suffix);
void process_file(const char *file_path);
int ends_with(const char *str, const char *suffix);

int main(int argc, char *argv[]) {
    char *suffix = ".txt"; // Default suffix
    int has_paths = 0;

    // Loop through arguments to look for -s and paths
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-s", 2) == 0) {
            // Case 1: "-s.md" (suffix attached)
            if (strlen(argv[i]) > 2) {
                suffix = argv[i] + 2;
            }
            // Case 2: "-s" followed by ".md" in next arg
            else if (i + 1 < argc) {
                suffix = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing argument for -s\n");
                return 1;
            }
        } else {
            has_paths = 1;
            process_path(argv[i], suffix);
        }
    }

    if (!has_paths) {
        fprintf(stderr, "Usage: %s [-s suffix] file_or_directory...\n", argv[0]);
        return 1;
    }

    return 0;
}

// Helper function to check if a filename ends with the target suffix
int ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// Read file contents using open(), read(), and write()
void process_file(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    printf("\n=== Reading file: %s ===\n", file_path);

    // Read chunks dynamically until EOF
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        // Print the chunk to standard output using write()
        if (write(STDOUT_FILENO, buffer, bytes_read) == -1) {
            perror("Error writing to stdout");
            close(fd);
            return;
        }
    }

    if (bytes_read == -1) {
        perror("Error reading file");
    }

    close(fd);
    printf("\n===========================\n");
}

// Recursively traverse a directory
void process_directory(const char *dir_path, const char *suffix) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignore hidden files/directories starting with '.'
        if (entry->d_name[0] == '.') {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            perror("Error stat-ing path");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            process_directory(full_path, suffix);
        } else if (S_ISREG(st.st_mode)) {
            if (ends_with(entry->d_name, suffix)) {
                process_file(full_path);
            }
        }
    }

    closedir(dir);
}

// Determine if a path given by the user is a file or a directory
void process_path(const char *path, const char *suffix) {
    struct stat st;
    if (stat(path, &st) == -1) {
        perror("Error stat-ing path");
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        process_directory(path, suffix);
    } else if (S_ISREG(st.st_mode)) {
        process_file(path);
    } else {
        fprintf(stderr, "Error: %s is neither a regular file nor a directory.\n", path);
    }
}