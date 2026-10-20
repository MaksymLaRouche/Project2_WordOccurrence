#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    char *suffix = ".txt"; // Default suffix
    int has_paths = 0;

    // Loop through arguments to look for -s
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-s", 2) == 0) {
            // Case 1: "-s.md" (suffix is attached in the same argument)
            if (strlen(argv[i]) > 2) {
                suffix = argv[i] + 2;
            }
            // Case 2: "-s" followed by ".md" in the next argument
            else if (i + 1 < argc) {
                suffix = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing argument for -s\n");
                return 1;
            }
        } else {
            // This argument is a file or directory path
            has_paths = 1;
            printf("Target path: %s\n", argv[i]);
        }
    }

    printf("Using suffix: %s\n", suffix);

    if (!has_paths) {
        fprintf(stderr, "Usage: %s [-s suffix] file_or_directory...\n", argv[0]);
        return 1;
    }

    return 0;
}