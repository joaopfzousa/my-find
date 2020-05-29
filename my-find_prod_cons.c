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




void * listDir(void * param);
void remove_all_chars(char* str, char c);

typedef int (*PARAM)(struct dirent * entry, char * value, struct stat file_stat);

typedef struct arg {
    PARAM opt;
    char * value;
}ARG;

typedef struct thread_data {
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

PATHS * aloc_memory_path(char* path)
{
    PATHS* new = (PATHS*) malloc(sizeof(PATHS));
    new->pnext = NULL;
    new->base_path = (char *) malloc(sizeof(char) * strlen(path));
    strcpy(new->base_path, path);
   return new;
}


int name (struct dirent * entry, char * value, struct stat file_stat)
{
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
    printf("Find by iname: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int empty (struct dirent * entry, char * value, struct stat file_stat) 
{
    //printf("Find by empty: %s\n", value);
     if(S_ISDIR(file_stat.st_mode)){
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
    printf("Find by mmin: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int size (struct dirent * entry, char * value, struct stat file_stat) 
{
    printf("Find by size: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

T_DATA read_command(int n_args, char **arg_list)
{
    T_DATA t_data;

    t_data.base_path = malloc(sizeof(char) * 300);
    t_data.base_path = arg_list[1];

    for(int k = 2; k < n_args; k++)
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
    return t_data;
}

void consome(char * path_consome) 
{
    int j = 0;
    DIR *dir;
    struct dirent *entry;
    char *path = malloc(sizeof(char) * 300);
    struct stat file_stat;

    if ((dir = opendir(path_consome)) == NULL)
        perror("opendir() error");
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

int produz(int val) {
    srandom(val);
    int r = random() % 100;
    return r;
}

char * listDir(void * param)
{
    struct thread_data * my_data;
    my_data = (struct thread_data *) param;

    T_DATA thread_data;
    pthread_t thread_id;
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
                if(S_ISDIR(file_stat.st_mode))
                {
                    path = strcat(path, "/");

                    thread_data.base_path = malloc(sizeof(char) * 300);
                    strcpy(thread_data.base_path, path);
                    listDir(&thread_data);
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

    while(1)
    {
        //pode ser o diretorio
        int item =  produz(my_data->val);

        semaphore_wait(semPodeProd);
            pthread_mutex_lock(&trinco_p);
                buf[prodptr] = item;
                prodptr = (prodptr + 1) % N;
            pthread_mutex_unlock(&trinco_p);
        semaphore_signal(semPodeCons);
    }

}


void * consumidor(void * param)
{
    char* item;
    while(1)
    {
         semaphore_wait(semPodeCons);
            pthread_mutex_lock(&trinco_c);
                item = buf[consptr];
                buf[consptr] = -1;
                prodptr = (prodptr + 1) % N;
            pthread_mutex_unlock(&trinco_c);
        semaphore_signal(semPodeProd);

        // verificar todas as ocorrencias do diretorio
        consome(item);
    }
}

void * print_state(void * unused)
{
    int i = 0;
    while(TRUE){
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

int main(int argc, char *argv[])
{
    struct thread_data th_data_array_prod;
    struct thread_data th_data_array_cons[NCons];

    pthread_t tid_state;
    pthread_t thread_id;

    th_data_array_prod = read_command (argc, argv);

    semaphore_create(mach_task_self(), &semPodeProd, SYNC_POLICY_FIFO, N);
    semaphore_create(mach_task_self(), &semPodeCons, SYNC_POLICY_FIFO, 0);

    pthread_create(&thread_id, NULL, &produtor, &th_data_array_prod);
    

    for(int i=0;i<NCons;i++)
        pthread_create(&thread_id, NULL, &consumidor, NULL);

    pthread_create(&tid_state, NULL, &print_state, NULL);
    pthread_join(tid_state, NULL);

    return 0;
}

void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
} 