#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>

typedef struct {
    int only_links;   
    int only_dirs;     
    int only_files;    
    int sort_output;   
    int show_stats;    
    char *start_dir;   
} options_t;

typedef struct {
    int total_files;      
    int total_dirs;       
    int total_links;      
    int total_others;     
    int errors;           
    size_t total_size;    
} stats_t;


static stats_t stats = {0, 0, 0, 0, 0, 0};

static char **file_list = NULL;
static int file_count = 0;
static int file_capacity = 0;

static void print_usage(const char *program_name) {
    printf("Использование: %s [опции] [директория]\n", program_name);
    printf("Опции:\n");
    printf("  -l    показывать только символические ссылки\n");
    printf("  -d    показывать только каталоги\n");
    printf("  -f    показывать только файлы\n");
    printf("  -s    сортировать вывод\n");
    printf("  -c    показать статистику после обхода\n");
    printf("  -h    показать эту справку\n");
    printf("\nПримеры:\n");
    printf("  %s -s /home/user    # рекурсивно обойти /home/user с сортировкой\n", program_name);
    printf("  %s -lf .            # показать только ссылки и файлы в текущей директории\n", program_name);
    printf("  %s -c /var/log      # обойти /var/log и показать статистику\n", program_name);
}

static void print_stats(const char *dir_path) {
    printf("\n========== СТАТИСТИКА ==========\n");
    printf("Директория обхода: %s\n", dir_path);
    printf("--------------------------------\n");
    printf("Файлов:           %d\n", stats.total_files);
    printf("Директорий:       %d\n", stats.total_dirs);
    printf("Симв. ссылок:     %d\n", stats.total_links);
    printf("Других типов:     %d\n", stats.total_others);
    printf("--------------------------------\n");
    printf("Всего объектов:   %d\n", 
           stats.total_files + stats.total_dirs + stats.total_links + stats.total_others);
    printf("Ошибок:           %d\n", stats.errors);
    
    if (stats.total_size > 0) {
        if (stats.total_size < 1024) {
            printf("Общий размер:     %zu B\n", stats.total_size);
        } else if (stats.total_size < 1024 * 1024) {
            printf("Общий размер:     %.2f KB\n", stats.total_size / 1024.0);
        } else if (stats.total_size < 1024 * 1024 * 1024) {
            printf("Общий размер:     %.2f MB\n", stats.total_size / (1024.0 * 1024));
        } else {
            printf("Общий размер:     %.2f GB\n", stats.total_size / (1024.0 * 1024 * 1024));
        }
    }
    printf("================================\n");
}

static int add_to_list(const char *path) {
    if (file_count >= file_capacity) {
        int new_capacity = file_capacity == 0 ? 1024 : file_capacity * 2;
        char **new_list = realloc(file_list, new_capacity * sizeof(char *));
        
        if (!new_list) {
            fprintf(stderr, "Ошибка: не удалось выделить память для списка\n");
            return -1;
        }
        
        file_list = new_list;
        file_capacity = new_capacity;
    }
    
    file_list[file_count] = strdup(path);
    if (!file_list[file_count]) {
        fprintf(stderr, "Ошибка: не удалось скопировать путь\n");
        return -1;
    }
    
    file_count++;
    return 0;
}

static int compare_strings(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcoll(sa, sb);
}

static void print_sorted(void) {
    if (file_count > 0) {
        qsort(file_list, file_count, sizeof(char *), compare_strings);
        
        for (int i = 0; i < file_count; i++) {
            printf("%s\n", file_list[i]);
            free(file_list[i]);
        }
        
        free(file_list);
        file_list = NULL;
        file_count = 0;
        file_capacity = 0;
    }
}

static void update_stats(const struct stat *st, int is_error) {
    if (is_error) {
        stats.errors++;
        return;
    }
    
    if (S_ISREG(st->st_mode)) {
        stats.total_files++;
        stats.total_size += st->st_size;
    } else if (S_ISDIR(st->st_mode)) {
        stats.total_dirs++;
    } else if (S_ISLNK(st->st_mode)) {
        stats.total_links++;
    } else {
        stats.total_others++;
    }
}

static int should_print(const char *path, const struct stat *st, const options_t *opts) {
    (void)path;  
    
    if (!opts->only_links && !opts->only_dirs && !opts->only_files)
        return 1;
    
    if (opts->only_links && S_ISLNK(st->st_mode))
        return 1;
    
    if (opts->only_dirs && S_ISDIR(st->st_mode))
        return 1;
    
    if (opts->only_files && S_ISREG(st->st_mode))
        return 1;
    
    return 0;
}

static void walk_dir(const char *dir_path, const options_t *opts) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Ошибка открытия %s: %s\n", dir_path, strerror(errno));
        stats.errors++;
        return;
    }
    
    struct dirent *entry;
    char full_path[4096];
    struct stat st;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        size_t dir_len = strlen(dir_path);
        if (dir_len > 0 && dir_path[dir_len - 1] == '/') {
            snprintf(full_path, sizeof(full_path), "%s%s", dir_path, entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        }
        
        if (lstat(full_path, &st) == -1) {
            fprintf(stderr, "Ошибка получения информации о %s: %s\n", 
                    full_path, strerror(errno));
            stats.errors++;
            continue;
        }
        
        update_stats(&st, 0);
        
        if (should_print(full_path, &st, opts)) {
            if (opts->sort_output) {
                if (add_to_list(full_path) != 0) {
                    continue;
                }
            } else {
                if (opts->show_stats) {
                    static int printed = 0;
                    if (++printed % 1000 == 0) {
                        fprintf(stderr, "\rОбработано объектов: %d", 
                                stats.total_files + stats.total_dirs + 
                                stats.total_links + stats.total_others);
                    }
                }
                printf("%s\n", full_path);
            }
        }
        
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            walk_dir(full_path, opts);
        }
    }
    
    closedir(dir);
}

static int parse_options(const char *opt_str, options_t *opts) {
    for (int i = 0; opt_str[i]; i++) {
        switch (opt_str[i]) {
            case 'l':
                opts->only_links = 1;
                break;
            case 'd':
                opts->only_dirs = 1;
                break;
            case 'f':
                opts->only_files = 1;
                break;
            case 's':
                opts->sort_output = 1;
                break;
            case 'c':
                opts->show_stats = 1;
                break;
            case 'h':
                return -1;
            default:
                fprintf(stderr, "Предупреждение: неизвестная опция '-%c'\n", opt_str[i]);
                break;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    
    options_t opts = {0, 0, 0, 0, 0, "."};
    char *dir = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (parse_options(argv[i] + 1, &opts) != 0) {
                print_usage(argv[0]);
                return 0;
            }
        } else {
            dir = argv[i];
        }
    }
    
    if (!dir) {
        dir = opts.start_dir;
        
        if (argc == 1) {
            print_usage(argv[0]);
            return 0;
        }
    }
    
    struct stat st;
    if (stat(dir, &st) != 0) {
        fprintf(stderr, "Ошибка: '%s' - %s\n", dir, strerror(errno));
        return 1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Ошибка: '%s' не является директорией\n", dir);
        return 1;
    }
    
    walk_dir(dir, &opts);
    
    if (opts.sort_output) {
        print_sorted();
    }
    
    
    if (opts.show_stats) {
        print_stats(dir);
    }
    
    return 0;
}