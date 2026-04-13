#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <errno.h>
#include <limits.h>

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

static void print_usage(const char *program_name) {
    printf("Использование: %s [опции] [директория]\n", program_name);
    printf("Опции:\n");
    printf("  -l    показывать только символические ссылки\n");
    printf("  -d    показывать только каталоги\n");
    printf("  -f    показывать только файлы\n");
    printf("  -s    сортировать вывод\n");
    printf("  -c    показать статистику после обхода\n");
    printf("  -h    показать эту справку\n");
    printf("\nПримечание: опции -l, -d, -f можно комбинировать.\n");
    printf("Например, -lf покажет и ссылки, и файлы.\n");
    printf("\nПримеры:\n");
    printf("  %s -s /home/user    # рекурсивно обойти /home/user с сортировкой\n", program_name);
    printf("  %s -lf .            # показать только ссылки и файлы в текущей директории\n", program_name);
    printf("  %s -c /var/log      # обойти /var/log и показать статистику\n", program_name);
}

static void print_stats(const char *dir_path, const stats_t *stats) {
    printf("\n========== СТАТИСТИКА ==========\n");
    printf("Директория обхода: %s\n", dir_path);
    printf("--------------------------------\n");
    printf("Файлов:           %d\n", stats->total_files);
    printf("Каталогов:       %d\n", stats->total_dirs);
    printf("Симв. ссылок:     %d\n", stats->total_links);
    printf("Других типов:     %d\n", stats->total_others);
    printf("--------------------------------\n");
    printf("Всего объектов:   %d\n", 
           stats->total_files + stats->total_dirs + stats->total_links + stats->total_others);
    printf("Ошибок:           %d\n", stats->errors);
    
    if (stats->total_size > 0) {
        if (stats->total_size < 1024) {
            printf("Общий размер:     %zu B\n", stats->total_size);
        } else if (stats->total_size < 1024 * 1024) {
            printf("Общий размер:     %.2f KB\n", stats->total_size / 1024.0);
        } else if (stats->total_size < 1024 * 1024 * 1024) {
            printf("Общий размер:     %.2f MB\n", stats->total_size / (1024.0 * 1024));
        } else {
            printf("Общий размер:     %.2f GB\n", stats->total_size / (1024.0 * 1024 * 1024));
        }
    }
    printf("================================\n");
}

static int compare_strings(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcoll(sa, sb);
}

static int should_print(const struct stat *st, const options_t *opts) {
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

static void update_stats(struct stat *st, stats_t *stats, int is_error) {
    if (is_error) {
        stats->errors++;
        return;
    }
    
    if (S_ISREG(st->st_mode)) {
        stats->total_files++;
        stats->total_size += st->st_size;
    } else if (S_ISDIR(st->st_mode)) {
        stats->total_dirs++;
    } else if (S_ISLNK(st->st_mode)) {
        stats->total_links++;
    } else {
        stats->total_others++;
    }
}

typedef struct {
    char **list;
    int count;
    int capacity;
} file_list_t;

static void init_file_list(file_list_t *list) {
    list->list = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void free_file_list(file_list_t *list) {
    for (int i = 0; i < list->count; i++) {
        free(list->list[i]);
    }
    free(list->list);
    list->list = NULL;
    list->count = 0;
    list->capacity = 0;
}

static int add_to_list(file_list_t *list, const char *path) {
    if (list->count >= list->capacity) {
        int new_capacity = list->capacity == 0 ? 1024 : list->capacity * 2;
        char **new_list = realloc(list->list, new_capacity * sizeof(char *));
        
        if (!new_list) {
            fprintf(stderr, "Ошибка: не удалось выделить память для списка\n");
            return -1;
        }
        
        list->list = new_list;
        list->capacity = new_capacity;
    }
    
    list->list[list->count] = strdup(path);
    if (!list->list[list->count]) {
        fprintf(stderr, "Ошибка: не удалось скопировать путь\n");
        return -1;
    }
    
    list->count++;
    return 0;
}

static void print_sorted(file_list_t *list) {
    if (list->count > 0) {
        qsort(list->list, list->count, sizeof(char *), compare_strings);
        
        for (int i = 0; i < list->count; i++) {
            printf("%s\n", list->list[i]);
        }
    }
}

static void walk_dir(const char *dir_path, const options_t *opts, 
                     stats_t *stats, file_list_t *file_list, int depth) {
    #define MAX_DEPTH 10000
    
    if (depth > MAX_DEPTH) {
        fprintf(stderr, "Ошибка: превышена максимальная глубина рекурсии в %s\n", dir_path);
        stats->errors++;
        return;
    }
    
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Ошибка открытия %s: %s\n", dir_path, strerror(errno));
        stats->errors++;
        return;
    }
    
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat st;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        // Проверка на переполнение буфера
        int needed = snprintf(full_path, sizeof(full_path), "%s/%s", 
                              dir_path, entry->d_name);
        if (needed >= (int)sizeof(full_path)) {
            fprintf(stderr, "Предупреждение: путь слишком длинный: %s/%s\n", 
                    dir_path, entry->d_name);
            stats->errors++;
            continue;
        }
        
        if (lstat(full_path, &st) == -1) {
            fprintf(stderr, "Ошибка получения информации о %s: %s\n", 
                    full_path, strerror(errno));
            stats->errors++;
            continue;
        }
        
        update_stats(&st, stats, 0);
        
        if (should_print(&st, opts)) {
            if (opts->sort_output) {
                if (add_to_list(file_list, full_path) != 0) {
                    // При ошибке памяти очищаем список и продолжаем
                    free_file_list(file_list);
                    init_file_list(file_list);
                    continue;
                }
            } else {
                if (opts->show_stats) {
                    static int printed = 0;
                    if (++printed % 1000 == 0) {
                        fprintf(stderr, "\rОбработано объектов: %d", 
                                stats->total_files + stats->total_dirs + 
                                stats->total_links + stats->total_others);
                        fflush(stderr);
                    }
                }
                printf("%s\n", full_path);
            }
        }
        
        // Рекурсивный обход поддиректорий
        if (S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode)) {
            walk_dir(full_path, opts, stats, file_list, depth + 1);
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
                fprintf(stderr, "Ошибка: неизвестная опция '-%c'\n", opt_str[i]);
                return -1;
        }
    }
    
    // Предупреждение о комбинации фильтров
    int filter_count = opts->only_links + opts->only_dirs + opts->only_files;
    if (filter_count > 1) {
        fprintf(stderr, "Предупреждение: указано несколько фильтров (-l, -d, -f), "
                "будут показаны объекты, соответствующие любому из условий\n");
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    
    options_t opts = {0, 0, 0, 0, 0, "."};
    stats_t stats = {0, 0, 0, 0, 0, 0};
    file_list_t file_list;
    init_file_list(&file_list);
    
    char *dir = NULL;
    
    // Парсинг аргументов командной строки
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '\0') {
            if (parse_options(argv[i] + 1, &opts) != 0) {
                print_usage(argv[0]);
                free_file_list(&file_list);
                return 0;
            }
        } else {
            dir = argv[i];
        }
    }
    
    // Если директория не указана, используем текущую
    if (!dir) {
        if (argc == 1) {
            print_usage(argv[0]);
            free_file_list(&file_list);
            return 0;
        }
        dir = opts.start_dir;
    }
    
    // Проверяем, существует ли директория
    struct stat st;
    if (stat(dir, &st) != 0) {
        fprintf(stderr, "Ошибка: '%s' - %s\n", dir, strerror(errno));
        free_file_list(&file_list);
        return 1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Ошибка: '%s' не является директорией\n", dir);
        free_file_list(&file_list);
        return 1;
    }
    
    // Обработка стартовой директории
    if (lstat(dir, &st) == 0) {
        update_stats(&st, &stats, 0);
        
        if (should_print(&st, &opts)) {
            if (opts.sort_output) {
                add_to_list(&file_list, dir);
            } else {
                printf("%s\n", dir);
            }
        }
    }
    
    // Обход директории
    walk_dir(dir, &opts, &stats, &file_list, 0);
    
    if (opts.sort_output) {
        print_sorted(&file_list);
    }
    
    if (opts.show_stats) {
        if (opts.sort_output && file_list.count > 0) {
            printf("\n");
        }
        print_stats(dir, &stats);
    }
    
    free_file_list(&file_list);
    
    return 0;
}