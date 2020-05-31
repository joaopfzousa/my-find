#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#undef _POSIX_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#define MAX_CMD 100
#define TRUE 1

void * listDir(void * param);
void remove_all_chars(char* str, char c);

typedef int (*PARAM)(struct dirent * entry, char * value, struct stat file_stat);

typedef struct arg {
    PARAM opt;
    char * value;
}ARG;

typedef struct thread_data {
    char * base_path;
    ARG args[50];
    int n_args;
}T_DATA;

typedef struct paths {
    char *base_path;
    struct paths *pnext;
}PATHS;

typedef struct occur {
    pthread_t thread_id;
    PATHS *pfirst;
    int n_paths;
    struct occur *pnext;
}OCCUR;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
OCCUR  occurrences = {.thread_id = NULL, .pnext = NULL, .pfirst = NULL, .n_paths = 0};


PATHS * aloc_memory_path(char* path)
{
    PATHS* new = (PATHS*) malloc(sizeof(PATHS));
    new->pnext = NULL;
    new->base_path = (char *) malloc(sizeof(char) * strlen(path));
    strcpy(new->base_path, path);
    return new;
}

void insert_occur(pthread_t thread, char* path)
{
    //printf("occurrences.n_paths = %d\n", occurrences.n_paths);
    PATHS * new =  aloc_memory_path(path);

    //primeira inserçãp
    if(occurrences.n_paths == 0)
    {
        //printf("Inseri 1\n");
        occurrences.thread_id = thread;
        occurrences.pfirst = new;
        occurrences.n_paths += 1;
        occurrences.pnext = NULL;
        return;
    }

    //find /Users/joaopfzousa/Documents/Faculdade/SO/ -name list_dir.c

    //Depois da primeira inserção
    OCCUR *temp = &occurrences;

    while(temp->pnext != NULL)
    {
        //quando é igual
        if(pthread_equal(temp->thread_id , thread))
        {
            PATHS * temp_firstPath = temp->pfirst;
            temp->pfirst = new;
            new->pnext = temp_firstPath;
            temp->n_paths++;
            return;
        }
        temp = temp->pnext;
    }

    //não é igaul, nova ocurrencia 
    OCCUR * new_occur = (OCCUR*) malloc(sizeof(OCCUR));
    new_occur->thread_id = thread;
    new_occur->pfirst = new;
    new_occur->n_paths++;
    new_occur->pnext = NULL;

    temp->pnext = new_occur;
}

void print_occur()
{
    OCCUR *temp = &occurrences;

    while(temp != NULL)
    {
        PATHS * temp_newPath = temp->pfirst;
        while(temp_newPath != NULL)
        {
            printf("[%lu] -> %s\n", temp->thread_id, temp_newPath->base_path);
            temp_newPath = temp_newPath->pnext;
        }
        temp = temp->pnext;
    }
}

char *strupr(char *nome_ficheiro) 
{
    for (int i = 0; i < strlen(nome_ficheiro); i++) 
    {
        nome_ficheiro[i] = toupper(nome_ficheiro[i]);
    }
    return nome_ficheiro;
}

int name (struct dirent * entry, char * value, struct stat file_stat)
{
    char * value_last_c = NULL;

    char * entry_last_C = NULL;

    if (value[0] == '*') 
    {
        char c = value[1];

        if ((strrchr(value, c)) != NULL && strrchr(entry->d_name, c) != NULL) 
        {
            value_last_c = strrchr(value, c);
            entry_last_C = strrchr(entry->d_name, c);

            if ((strcmp(value_last_c, entry_last_C) == 0)) 
            {
                return 1; // return 1 if match found
            } else {
                return 0; // return 1 if match not found
            }
        }
    }

    if(strstr(value , entry->d_name) != NULL)
    {
        return 1; // return 1 if match found
    }
    return 0; // return 1 if match not found
}

int type (struct dirent * entry, char * value, struct stat file_stat) 
{
    char * new;

    if(S_ISDIR(file_stat.st_mode))
    {
        new = "d";
    } else{
        new = "f";
    }

    if(strcmp(value, new) == 0)
    {
        return 1; // return 1 if match found
    }

    return 0; // return 0 if match not found
}

int iname (struct dirent * entry, char * value, struct stat file_stat) 
{
    char * value_last_c = NULL;

    char * entry_last_c = NULL;

    char * valueU = strupr(value);

    char * entryU = strupr(entry->d_name);

    if (value[0] == '*') 
    {
        char c = value[1];

        if ((strrchr(valueU, c)) != NULL && strrchr(entryU, c) != NULL) 
        {
            value_last_c = strrchr(valueU, c);
            entry_last_c = strrchr(entryU, c);

            if ((strcmp(value_last_c, entry_last_c) == 0)) 
            {
                return 1; // return 1 if match found
            } else {
                return 0; // return 0 if match found
            }
        }
    }

    if ((strstr(entryU, valueU)) != NULL) 
    {
        return 1; // return 1 if match found
    }
    return 0; // return 0 if match found
}

int empty (struct dirent * entry, char * value, struct stat file_stat) 
{
    //printf("Find by empty: %s\n", value);
    if(S_ISDIR(file_stat.st_mode))
    {
        if(file_stat.st_size == 64)
        {
            return 1; // return 1 if match found
        }
    } else{
        if(file_stat.st_size == 0)
        {
            return 1; // return 1 if match found
        }
    }
    return 0; // return 0 if match not found
}

int executable (struct dirent * entry, char * value, struct stat file_stat) 
{
    if((file_stat.st_mode & S_IXUSR) && (file_stat.st_mode & S_IXGRP) && (file_stat.st_mode & S_IXOTH))
    {
        return 1; // return 1 if match found
    }
    return 0; // return 0 if match not found
}

int mmin (struct dirent * entry, char * value, struct stat file_stat) 
{
    int min = atoi(value);
    struct timespec timespec;

    int timeAux = timespec.tv_sec;
    timeAux *= 60;
    if (min >= timeAux) {
        return 1; // return 1 if match found
    } else {
        return 0; // return 0 if match not found
    }
}

int size (struct dirent * entry, char * value, struct stat file_stat) 
{
    double result = 0;
    char type_size[2];
    int aux = 0;

    for (int i = 1; value[i] != '\0'; i++) 
    {
        if (value[i] >= '0' && value[i] <= '9') 
        {
            result = (result * 10) + (value[i] - '0');
        } else {
            type_size[aux] = toupper(value[i]);
            aux++;
        }
    }

    if (strcmp(type_size, "KB") == 0) 
    {
        if (value[0] == '+' && (file_stat.st_size / 1024) > result) 
        {
            return 1; // return 1 if match found
        } else if (value[0] == '-' && (file_stat.st_size / 1024) < result) 
        {
            return 1; // return 1 if match found
        }
    } else if (strcmp(type_size, "MB") == 0) 
    {
        if (value[0] == '+' && (file_stat.st_size / 1024) > result) 
        {
            return 1; // return 1 if match found
        } else if (value[0] == '-' && (file_stat.st_size / 1024) < result) 
        {
            return 1; // return 1 if match found
        }
    } else if (strcmp(type_size, "B") == 0)
    {
        if (file_stat.st_size > result) 
        {
            return 1; // return 1 if match found
        } else if (file_stat.st_size < result) 
        {
            return 1; // return 1 if match found
        }
    }
    return 0; // return 0 if match not found
}

T_DATA read_command(int n_args, char **arg_list)
{
    T_DATA t_data = { .args={NULL, ""}, .n_args=0 };

    if(strcmp(".", arg_list[1]) == 0)
    {
        t_data.base_path = malloc(sizeof(char) * 300);
        getcwd(t_data.base_path, sizeof(t_data.base_path));
        strcat(t_data.base_path, "/");
    }else{
        t_data.base_path = malloc(sizeof(char) * 300);
        t_data.base_path = arg_list[1];
    }
    

    for(int k = 2; k < n_args; k++)
    {
        if(strcmp(arg_list[k], "-name") == 0 || strcmp(arg_list[k], "-iname") == 0 || strcmp(arg_list[k], "-type") == 0 || strcmp(arg_list[k], "-empty") == 0|| strcmp(arg_list[k], "-executable") == 0 || strcmp(arg_list[k], "-mmin") == 0 || strcmp(arg_list[k], "-size") == 0)
        {
            remove_all_chars(arg_list[k], '-');

            if(strcmp(arg_list[k], "name") == 0)
            {
                t_data.args[t_data.n_args].opt = (PARAM) name;
            }else if(strcmp(arg_list[k], "iname") == 0){
                t_data.args[t_data.n_args].opt = (PARAM) iname;
            }else if(strcmp(arg_list[k], "type") == 0){
                t_data.args[t_data.n_args].opt = (PARAM) type;
            }else if(strcmp(arg_list[k], "empty") == 0){
                t_data.args[t_data.n_args].opt =  (PARAM) empty;
            }else if(strcmp(arg_list[k], "executable") == 0){
                t_data.args[t_data.n_args].opt = (PARAM) executable;
            }else if(strcmp(arg_list[k], "mmin") == 0){
                t_data.args[t_data.n_args].opt = (PARAM) mmin;
            }else if(strcmp(arg_list[k], "size") == 0){
                t_data.args[t_data.n_args].opt = (PARAM) size;
            }
            if(strcmp(arg_list[k], "empty") == 0 || strcmp(arg_list[k], "executable") == 0 )
            {
                t_data.args[t_data.n_args].value = NULL;
            }else{
                k++;
                t_data.args[t_data.n_args].value = arg_list[k];
            }
            t_data.n_args++;
        }
    }
    return t_data;
}

int main(int argc, char *argv[])
{
    T_DATA thread_data;
    pthread_t thread_id;    
    thread_data = read_command (argc, argv);

    pthread_create(&thread_id, NULL, &listDir, &thread_data);
    pthread_join(thread_id, NULL);

    print_occur();    
    return 0;
}

void * listDir(void * param)
{
    struct thread_data * my_data;
    my_data = (struct thread_data *) param;

    T_DATA thread_data[20];
    pthread_t thread_id[20];
    int i = 0, j = 0;

    DIR *dir;
    struct dirent *entry;
    char *path = malloc(sizeof(char) * 300);
    struct stat file_stat;

    if ((dir = opendir(my_data->base_path)) == NULL)
        perror("opendir() error");
    else
    {
        while ((entry = readdir(dir)) != NULL)
        {
            sprintf(path, "%s%s", my_data->base_path, entry->d_name);

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".DS_Store") == 0 || strcmp(entry->d_name, ".git") == 0)
                continue;

            if (stat(path, &file_stat) == 0)
            {
                for (j = 0; j < my_data->n_args; j++)
                {
                    if (!my_data->args[j].opt(entry, my_data->args[j].value, file_stat))
                        break;
                }
                //printf("j = %d\n",j);
                //printf("my_data->n_args = %d\n",my_data->n_args);

                if(j == my_data->n_args)
                {
                    pthread_mutex_lock(&mutex);
                        insert_occur(pthread_self(), path);
                    pthread_mutex_unlock(&mutex);
                }

                if(S_ISDIR(file_stat.st_mode))
                {
                    path = strcat(path, "/");

                    thread_data[i].base_path = malloc(sizeof(char) * 300);
                    strcpy(thread_data[i].base_path, path);
                    thread_data[i].n_args = my_data->n_args;
                    for(int x = 0; x < my_data->n_args; x++)
                    {
                        thread_data[i].args[x] = my_data->args[x];
                    }
                    pthread_create(&thread_id[i], NULL, &listDir, &thread_data[i]);
                    i++;
                }
            }
            strcpy(path, "");
        }
        closedir(dir);
    }

    for(int x = 0; x < i; x++)
        pthread_join(thread_id[x], NULL);
    pthread_exit(NULL);
}

void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
} 