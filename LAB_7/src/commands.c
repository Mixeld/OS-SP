
#include "commands.h"
#include "record.h"
#include "file_ops.h"
#include "locks.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

/* LST — отобразить все записи */
void cmd_list(int fd) {
    int cnt = record_count(fd);
    if (cnt <= 0) { printf("Файл пуст\n"); return; }

    for (int i = 0; i < cnt; i++) {
        struct record_s rec;
        if (lock_record(fd, i, F_RDLCK) == -1) {
            perror("lock RDLCK"); continue;
        }
        if (read_record(fd, i, &rec) == 0) {
            printf("[%d] ФИО: %-30s Адрес: %-30s Сем: %u\n",
                   i, rec.name, rec.address, rec.semester);
        }
        lock_record(fd, i, F_UNLCK);
    }
}

/* GET — показать одну запись */
void cmd_get(int fd, int rec_no) {
    int cnt = record_count(fd);
    if (rec_no < 0 || rec_no >= cnt) {
        printf("Неверный номер записи (0..%d)\n", cnt - 1);
        return;
    }
    struct record_s rec;
    if (lock_record(fd, rec_no, F_RDLCK) == -1) { perror("lock"); return; }
    if (read_record(fd, rec_no, &rec) == 0) {
        printf("[%d] ФИО: %s\n     Адрес: %s\n     Семестр: %u\n",
               rec_no, rec.name, rec.address, rec.semester);
    }
    lock_record(fd, rec_no, F_UNLCK);
}

/* PUT — реализация алгоритма из методички */
void cmd_put(int fd, int rec_no) {
    int cnt = record_count(fd);
    if (rec_no < 0 || rec_no >= cnt) {
        printf("Неверный номер записи\n"); return;
    }

    struct record_s REC, REC_WRK, REC_NEW;
    char buf[256];

    /* REC <-- get(Rec_No) */
    if (lock_record(fd, rec_no, F_RDLCK) == -1) { perror("lock"); return; }
    if (read_record(fd, rec_no, &REC) == -1) {
        perror("read"); lock_record(fd, rec_no, F_UNLCK); return;
    }
    lock_record(fd, rec_no, F_UNLCK);

    /* REC_WRK <-- REC */
    REC_WRK = REC;

    /* Модификация */
    printf("Текущее ФИО: %s\nНовое (Enter — без изм.): ", REC_WRK.name);
    read_line(buf, sizeof(buf));
    if (buf[0]) { strncpy(REC_WRK.name, buf, 79); REC_WRK.name[79] = 0; }

    printf("Текущий адрес: %s\nНовый: ", REC_WRK.address);
    read_line(buf, sizeof(buf));
    if (buf[0]) { strncpy(REC_WRK.address, buf, 79); REC_WRK.address[79] = 0; }

    printf("Текущий семестр: %u\nНовый: ", REC_WRK.semester);
    read_line(buf, sizeof(buf));
    if (buf[0]) REC_WRK.semester = (uint8_t)atoi(buf);

Again:
    if (memcmp(&REC_WRK, &REC, sizeof(REC)) != 0) {
        /* lock(Rec_No) — исключительная */
        if (lock_record(fd, rec_no, F_WRLCK) == -1) { perror("WRLCK"); return; }

        /* REC_NEW <-- get(Rec_No) */
        if (read_record(fd, rec_no, &REC_NEW) == -1) {
            perror("read"); lock_record(fd, rec_no, F_UNLCK); return;
        }

        if (memcmp(&REC_NEW, &REC, sizeof(REC)) != 0) {
            lock_record(fd, rec_no, F_UNLCK);
            printf(">> Запись изменена другим процессом!\n");
            printf(">> Новое: %s | %s | сем=%u\n",
                   REC_NEW.name, REC_NEW.address, REC_NEW.semester);
            REC = REC_NEW;
            printf(">> Применить ваши изменения поверх? (y/n): ");
            read_line(buf, sizeof(buf));
            if (buf[0] == 'y' || buf[0] == 'Y') goto Again;
            printf("Отмена\n");
            return;
        }

        /* put(REC_WRK, Rec_No) */
        if (write_record(fd, rec_no, &REC_WRK) == -1) perror("write");
        else printf("Запись сохранена\n");

        lock_record(fd, rec_no, F_UNLCK);
    } else {
        printf("Изменений нет\n");
    }
}

/* ADD — добавить запись */
void cmd_add(int fd) {
    struct record_s rec;
    char buf[256];
    memset(&rec, 0, sizeof(rec));

    printf("ФИО: ");     read_line(rec.name, sizeof(rec.name));
    printf("Адрес: ");   read_line(rec.address, sizeof(rec.address));
    printf("Семестр: "); read_line(buf, sizeof(buf));
    rec.semester = (uint8_t)atoi(buf);

    int cnt = record_count(fd);
    if (lock_record(fd, cnt, F_WRLCK) == -1) { perror("lock"); return; }
    if (write_record(fd, cnt, &rec) == -1) perror("write");
    else printf("Добавлена запись [%d]\n", cnt);
    lock_record(fd, cnt, F_UNLCK);
}