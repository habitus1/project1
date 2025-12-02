#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#define LOG_FILE "organize_log.txt" // 최종

char ROOT[512]; // ⭐ 전체 정리 기준이 되는 상위 폴더 경로 저장

// ------------------------- 공통 유틸 함수 -------------------------

long get_file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
        return st.st_size;
    return 0;
}

const char *get_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename)
        return NULL;
    return dot + 1;
}

void make_folder(const char *path, const char *folder)
{
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, folder);

    struct stat st = {0};
    if (stat(full_path, &st) == -1)
    {
        if (mkdir(full_path, 0700) == 0)
        {
            printf("[폴더 생성] %s\n", full_path);
        }
    }
}

void write_log(const char *oldpath, const char *newpath, const char *ext, long filesize)
{
    FILE *fp = fopen(LOG_FILE, "a");
    if (!fp)
        return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(fp,
            "[%04d-%02d-%02d %02d:%02d:%02d] %ld bytes | %s | %s → %s\n",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec,
            filesize, ext ? ext : "no_ext", oldpath, newpath);

    fclose(fp);
}

// ------------------------- 중복 파일명 처리 -------------------------

void generate_unique_filename(char *path)
{
    if (access(path, F_OK) != 0)
        return;

    char base[512], ext[64], temp[512];
    char *dot = strrchr(path, '.');

    if (!dot)
    {
        strcpy(base, path);
        ext[0] = '\0';
    }
    else
    {
        strncpy(base, path, dot - path);
        base[dot - path] = '\0';
        strcpy(ext, dot);
    }

    int count = 1;
    while (1)
    {
        snprintf(temp, sizeof(temp), "%s(%d)%s", base, count, ext);
        if (access(temp, F_OK) != 0)
        {
            strcpy(path, temp);
            break;
        }
        count++;
    }
}

// ------------------------- 재귀 정리 함수 -------------------------

void organize_recursive(const char *path)
{
    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *entry;
    char fullpath[512];

    while ((entry = readdir(dir)) != NULL)
    {

        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR)
        {
            organize_recursive(fullpath);
        }
        else if (entry->d_type == DT_REG)
        {
            const char *ext = get_extension(entry->d_name);
            char ext_folder[64];

            if (ext)
                strcpy(ext_folder, ext);
            else
                strcpy(ext_folder, "no_ext");

            // ⭐ 변경된 핵심
            // --- test1, test2가 아니라 ROOT 아래로 폴더 생성 ---
            make_folder(ROOT, ext_folder);

            char oldpath[512], newpath[512];
            snprintf(oldpath, sizeof(oldpath), "%s/%s", path, entry->d_name);
            snprintf(newpath, sizeof(newpath), "%s/%s/%s", ROOT, ext_folder, entry->d_name);

            generate_unique_filename(newpath);

            long size = get_file_size(oldpath);

            if (rename(oldpath, newpath) == 0)
            {
                printf("[이동 완료] %s → %s/\n", entry->d_name, ext_folder);
                write_log(oldpath, newpath, ext, size);
            }
            else
            {
                perror("파일 이동 실패");
            }
        }
    }

    closedir(dir);
}

// ------------------------- 메뉴 & 로그 -------------------------

void show_menu()
{
    printf("\n==============================\n");
    printf("   📁 파일 정리 자동화 프로그램\n");
    printf("==============================\n");
    printf("1. 전체 정리\n");
    printf("2. 로그 보기\n");
    printf("3. 종료\n");
    printf("==============================\n");
}

void read_log()
{
    FILE *fp = fopen(LOG_FILE, "r");
    if (!fp)
    {
        printf("로그가 없습니다.\n");
        return;
    }

    char line[512];

    printf("\n===== 정리 로그 =====\n");
    while (fgets(line, sizeof(line), fp))
    {
        printf("%s", line);
    }
    printf("=====================\n");

    fclose(fp);
}

// ------------------------- main -------------------------

int main()
{
    int choice;

    while (1)
    {
        show_menu();
        printf("선택: ");
        scanf("%d", &choice);
        getchar();

        if (choice == 1)
        {
            printf("정리할 폴더 경로 입력: ");
            fgets(ROOT, sizeof(ROOT), stdin);
            ROOT[strcspn(ROOT, "\n")] = '\0';

            organize_recursive(ROOT);
            printf("=== 전체 정리 완료! ===\n");
        }
        else if (choice == 2)
        {
            read_log();
        }
        else if (choice == 3)
        {
            printf("프로그램 종료.\n");
            break;
        }
        else
        {
            printf("잘못된 선택입니다.\n");
        }
    }

    return 0;
}
