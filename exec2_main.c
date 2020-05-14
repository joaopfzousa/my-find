#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#undef _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <string.h>

void listDir(const char *name, int indent)
{
    DIR *dir;
    struct dirent *entry;

    struct stat file_stat;

    char *base_path = malloc(sizeof(char) * 300);
    char *path = malloc(sizeof(char) * 300);
    strcpy(base_path, name);

    if ((dir = opendir(base_path)) == NULL)
        perror("opendir() error");
    else
    {
        printf("\n\n%s\n", base_path);
        while ((entry = readdir(dir)) != NULL)
        {
            sprintf(path, "%s%s", base_path, entry->d_name);
            
            if (stat(path, &file_stat) == 0)
            {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".DS_Store") == 0 || strcmp(entry->d_name, ".git") == 0)
                    continue;

                printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
                
                printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
                printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
                printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
                printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
                printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
                printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
                printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
                printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
                printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

                printf(" %u\t%lld\t%s\n", file_stat.st_uid, file_stat.st_size, entry->d_name);

                if(S_ISDIR(file_stat.st_mode))
                {
                    sprintf(path, "%s/", path);
                    listDir(path, 0);
                }
            } 
            strcpy(path, "");
        }
        closedir(dir);
    }
}

int main(int argc, char **argv)
{
    listDir("/Users/joaopfzousa/Documents/Faculdade/SO/", 0);
    return 0;
}