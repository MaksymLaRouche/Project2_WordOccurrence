#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#define INITIAL_BUFFER_SIZE 4096
#define INITIAL_ARRAY_CAPACITY 128

// Structure to store word occurrence counts
typedef struct {
    char *word;
    int count;
} WordEntry;

// Global dynamic array for word storage
WordEntry *word_entries = NULL;
size_t word_count = 0;
size_t word_capacity = 0;

// Forward declarations
void process_path(const char *path, const char *suffix);
void process_directory(const char *dir_path, const char *suffix);
void process_file(const char *file_path);
void parse_and_count_text(const char *text);
void add_word(const char *word_str, size_t length);
void print_results(void);
int ends_with(const char *str, const char *suffix);
int compare_words(const void *a, const void *b);

int main(int argc, char *argv[]) {
    char *suffix = ".txt"; // Default suffix
    int has_paths = 0;

    // 1. Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-s", 2) == 0) {
            if (strlen(argv[i]) > 2) {
                suffix = argv[i] + 2;
            } else if (i + 1 < argc) {
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

    // 2. Sort and print final aggregated results
    if (word_count > 0) {
        qsort(word_entries, word_count, sizeof(WordEntry), compare_words);
        print_results();
    }

    // 3. Free allocated memory
    for (size_t i = 0; i < word_count; i++) {
        free(word_entries[i].word);
    }
    free(word_entries);

    return 0;
}

// Helper to check file suffix
int ends_with(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (str_len < suffix_len) return 0;
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

// Read file contents dynamically using open() and read()
void process_file(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    size_t total_size = 0;
    size_t capacity = INITIAL_BUFFER_SIZE;
    char *buffer = malloc(capacity);
    if (!buffer) {
        perror("Memory allocation failed");
        close(fd);
        return;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer + total_size, capacity - total_size - 1)) > 0) {
        total_size += bytes_read;
        if (total_size >= capacity - 1) {
            capacity *= 2;
            char *temp = realloc(buffer, capacity);
            if (!temp) {
                perror("Memory reallocation failed");
                free(buffer);
                close(fd);
                return;
            }
            buffer = temp;
        }
    }

    if (bytes_read == -1) {
        perror("Error reading file");
    }

    buffer[total_size] = '\0';
    close(fd);

    // Parse words from the loaded file buffer
    parse_and_count_text(buffer);
    free(buffer);
}

// Recursive directory traversal using opendir(), readdir(), stat()
void process_directory(const char *dir_path, const char *suffix) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignore hidden files and directories starting with '.'
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

// Determine if path is file or directory
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

// Tokenize text according to project rules for apostrophes and hyphens
void parse_and_count_text(const char *text) {
    const char *p = text;

    while (*p != '\0') {
        // Skip non-word characters (anything that cannot start a word)
        while (*p != '\0' && !isalpha((unsigned char)*p) && *p != '\'') {
            p++;
        }
        if (*p == '\0') break;

        const char *start = p;
        p++; // advance past start character

        // Scan through word characters
        while (*p != '\0') {
            if (isalpha((unsigned char)*p) || *p == '\'') {
                p++;
            } else if (*p == '-') {
                // Hyphen rule: must be preceded and followed by a letter
                if (isalpha((unsigned char)*(p - 1)) && isalpha((unsigned char)*(p + 1))) {
                    p++;
                } else {
                    break; // Invalid hyphen position acts as separator
                }
            } else {
                break; // Separator encountered
            }
        }

        size_t len = p - start;
        if (len > 0) {
            // Trim trailing apostrophes or hyphens if any edge cases occur
            while (len > 0 && (start[len - 1] == '-' || start[len - 1] == '\'')) {
                len--;
            }
            if (len > 0) {
                add_word(start, len);
            }
        }
    }
}

// Add or increment word in our global dynamic data structure
void add_word(const char *word_str, size_t length) {
    // Check if word already exists (case-sensitive)
    for (size_t i = 0; i < word_count; i++) {
        if (strncmp(word_entries[i].word, word_str, length) == 0 && word_entries[i].word[length] == '\0') {
            word_entries[i].count++;
            return;
        }
    }

    // Expand capacity if needed
    if (word_count >= word_capacity) {
        word_capacity = word_capacity == 0 ? INITIAL_ARRAY_CAPACITY : word_capacity * 2;
        WordEntry *temp = realloc(word_entries, word_capacity * sizeof(WordEntry));
        if (!temp) {
            perror("Memory allocation failed");
            exit(1);
        }
        word_entries = temp;
    }

    // Allocate and store new word
    word_entries[word_count].word = malloc(length + 1);
    if (!word_entries[word_count].word) {
        perror("Memory allocation failed");
        exit(1);
    }
    memcpy(word_entries[word_count].word, word_str, length);
    word_entries[word_count].word[length] = '\0';
    word_entries[word_count].count = 1;
    word_count++;
}

// Comparator for sorting: descending count, then lexicographical for ties
int compare_words(const void *a, const void *b) {
    const WordEntry *wa = (const WordEntry *)a;
    const WordEntry *wb = (const WordEntry *)b;

    if (wb->count != wa->count) {
        return wb->count - wa->count;
    }
    return strcmp(wa->word, wb->word);
}

// Print results using write() to standard output
void print_results(void) {
    for (size_t i = 0; i < word_count; i++) {
        char line_buf[1024];
        int len = snprintf(line_buf, sizeof(line_buf), "%s %d\n", word_entries[i].word, word_entries[i].count);
        if (len > 0) {
            write(STDOUT_FILENO, line_buf, len);
        }
    }
}