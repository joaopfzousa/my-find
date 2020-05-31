#define _POSIX_SOURCE
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#undef _POSIX_SOURCE
#include <pthread.h>
#include <mach/mach_init.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#define N 10
#define NProds 1
#define NCons 6
#define MAX_CMD 100
#define TRUE 1

//./my_find-prod.out /Users/joaopfzousa/Documents/Faculdade/SO/ -type d
void produz(char * param);
void remove_all_chars(char* str, char c);

typedef int (*PARAM)(struct dirent * entry, char * value, struct stat file_stat);

typedef struct arg {
    PARAM opt;
    char * value;
}ARG;

typedef struct thread_data {
    pthread_t  thread_id;
    char * base_path;
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

char *buf[N];
int prodptr=0, consptr=0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
OCCUR  occurrences = {.thread_id = NULL, .pnext = NULL, .pfirst = NULL, .n_paths = 0};

ARG args[5];
int n_args;

pthread_mutex_t trinco_p = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trinco_c = PTHREAD_MUTEX_INITIALIZER;

static const char *semNameProd = "semPodeProd";
semaphore_t semPodeProd;

static const char *semNameCons = "semPodeCons";
semaphore_t semPodeCons;

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

    //não é igual, nova ocurrência
    OCCUR * new_occur = (OCCUR*) malloc(sizeof(OCCUR));
    new_occur->thread_id = thread;
    new_occur->pfirst = new;
    new_occur->n_paths++;
    new_occur->pnext = NULL;

    temp->pnext = new_occur;
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

void read_command(int argc, char **arg_list)
{
    int k;
    for(k = 2; k < argc; k++)
    {
        if(strcmp(arg_list[k], "-name") == 0 || strcmp(arg_list[k], "-iname") == 0 || strcmp(arg_list[k], "-type") == 0 || strcmp(arg_list[k], "-empty") == 0|| strcmp(arg_list[k], "-executable") == 0 || strcmp(arg_list[k], "-mmin") == 0 || strcmp(arg_list[k], "-size") == 0)
        {
            remove_all_chars(arg_list[k], '-');

            if(strcmp(arg_list[k], "name") == 0)
            {
                args[n_args].opt = (PARAM) name;
            }else if(strcmp(arg_list[k], "iname") == 0){
                args[n_args].opt = (PARAM) iname;
            }else if(strcmp(arg_list[k], "type") == 0){
                args[n_args].opt = (PARAM) type;
            }else if(strcmp(arg_list[k], "empty") == 0){
                args[n_args].opt =  (PARAM) empty;
            }else if(strcmp(arg_list[k], "executable") == 0){
                args[n_args].opt = (PARAM) executable;
            }else if(strcmp(arg_list[k], "mmin") == 0){
                args[n_args].opt = (PARAM) mmin;
            }else if(strcmp(arg_list[k], "size") == 0){
                args[n_args].opt = (PARAM) size;
            }
            if(strcmp(arg_list[k], "empty") == 0 || strcmp(arg_list[k], "executable") == 0 )
            {
                args[n_args].value = NULL;
            }else{
                k++;
                args[n_args].value = arg_list[k];
            }
            n_args++;
        }
    }
}

void consome(char * path_consome) 
{
    int j = 0;
    DIR *dir;
    struct dirent *entry;
    char *path = malloc(sizeof(char) * 300);
    struct stat file_stat;

    printf("Path consome = %s\n\n", path_consome);

    if ((dir = opendir(path_consome)) == NULL)
        perror("[CONSUMIDOR] -> opendir() error");
    else
    {
        while ((entry = readdir(dir)) != NULL)
        {
            sprintf(path, "%s%s", path_consome, entry->d_name);

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".DS_Store") == 0 || strcmp(entry->d_name, ".git") == 0)
                continue;

            if (stat(path, &file_stat) == 0)
            {
                for (j = 0; j < n_args; j++)
                {
                    if (!args[j].opt(entry, args[j].value, file_stat))
                        break;
                }

                if(j == n_args)
                {
                    pthread_mutex_lock(&mutex);
                        insert_occur(pthread_self(), path);
                    pthread_mutex_unlock(&mutex);
                }
            }
            strcpy(path, "");
        }
        closedir(dir);
    }
}

void produz(char * path_produtor)
{
    int i = 0, j = 0;

    DIR *dir;
    struct dirent *entry;
    char *path = malloc(sizeof(char) * 300);
    struct stat file_stat;

    //printf("path produz= %s\n", path_produtor);

    if ((dir = opendir(path_produtor)) == NULL)
        perror("[PRODUTOR] -> opendir() error");
    else 
    {
        while ((entry = readdir(dir)) != NULL)
        {
            sprintf(path, "%s%s", path_produtor, entry->d_name);

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, ".DS_Store") == 0 || strcmp(entry->d_name, ".git") == 0)
                continue;

            if (stat(path, &file_stat) == 0)
            {
                if(S_ISDIR(file_stat.st_mode))
                {
                    path = strcat(path, "/");

                    //printf("path  dir = %s\n", path);

                    semaphore_wait(semPodeProd);
                        pthread_mutex_lock(&trinco_p);
                            buf[prodptr] = path;
                            printf("PRODUZ : buf[prodptr] = %s\n", buf[prodptr]); 
                            prodptr = (prodptr + 1) % N;
                            printf("produz prodptr = %d\n", prodptr);
                        pthread_mutex_unlock(&trinco_p);
                    semaphore_signal(semPodeCons);
                    
                    produz(path);
                    i++;
                }
            }
            strcpy(path, "");
        }
        closedir(dir);
    }
}

void * produtor(void * param)
{
    struct thread_data *my_data;
    my_data = (struct thread_data *) param;

    //printf("path produtor = %s\n", my_data->base_path);

    produz(my_data->base_path);

    int i = 0;
    while(i < NCons)
    {
        char* item =  "bomba";
        semaphore_wait(semPodeProd);
            pthread_mutex_lock(&trinco_p);
                buf[prodptr] = item;
                prodptr = (prodptr + 1) % N;
                printf("produtor prodptr = %d\n", prodptr);
            pthread_mutex_unlock(&trinco_p);
        semaphore_signal(semPodeCons);
        i++;
    }
    
    pthread_exit(NULL);
}

void * consumidor(void * param)
{
    char* item;
    while(1)
    {
         semaphore_wait(semPodeCons);
            pthread_mutex_lock(&trinco_c);
                item = buf[consptr];
                buf[consptr] = NULL;
                consptr = (consptr + 1) % N;
                printf("consptr = %d\n", consptr);
            pthread_mutex_unlock(&trinco_c);
        semaphore_signal(semPodeProd);

        printf("item = %s\n", item);

        if(strcmp(item, "") == 0)
        {
            printf("Item vazio\n");
        }else if(strcmp(item, "bomba") == 0)
        {
            pthread_exit(NULL);
        }else{
            // verificar todas as ocorrencias do diretorio
            consome(item);
        }
    }
}

void * print_state(void * unused)
{
    int i = 0;
    while(TRUE)
    {
        pthread_mutex_lock(&trinco_c);
        pthread_mutex_lock(&trinco_p);

            for(i = 0;i < N; i++)
                printf("%s ", buf[i]);
        printf("\n");

        pthread_mutex_unlock(&trinco_p);
        pthread_mutex_unlock(&trinco_c);
        sleep(2);

        fflush(stdout);
    }
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

int main(int argc, char *argv[])
{
    struct thread_data th_data_array_prod;
    struct thread_data th_data_array_cons[NCons];

    //pthread_t tid_state;

    if(strcmp(".", argv[1]) == 0)
    {
        th_data_array_prod.base_path = malloc(sizeof(char) * 300);
        getcwd(th_data_array_prod.base_path, sizeof(th_data_array_prod.base_path));
        strcat(th_data_array_prod.base_path, "/");
    }else{
        th_data_array_prod.base_path = malloc(sizeof(char) * 300);
        th_data_array_prod.base_path = argv[1];
    }

    read_command (argc, argv);

    semaphore_create(mach_task_self(), &semPodeProd, SYNC_POLICY_FIFO, N);
    semaphore_create(mach_task_self(), &semPodeCons, SYNC_POLICY_FIFO, 0);

    //thread produtor
    pthread_create(&th_data_array_prod.thread_id, NULL, &produtor, &th_data_array_prod);
   

    for(int i=0;i<NCons;i++)
        pthread_create(&th_data_array_cons[i].thread_id, NULL, &consumidor, NULL);

    //pthread_create(&tid_state, NULL, &print_state, NULL);
    //pthread_join(tid_state, NULL);

    pthread_join(th_data_array_prod.thread_id, NULL);

    print_occur();

    return 0;
}

void remove_all_chars(char* str, char c) 
{
    char *pr = str, *pw = str;
    while (*pr) 
    {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
} 
