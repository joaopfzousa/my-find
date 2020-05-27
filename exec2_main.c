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
void prompt(void);

typedef int (*PARAM)(struct dirent * entry, char * value);

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
    //printf("Aloquei memoria path\n");
    PATHS* new = (PATHS*) malloc(sizeof(PATHS));
    new->pnext = NULL;
    new->base_path = (char *) malloc(sizeof(char) * strlen(path));
    strcpy(new->base_path, path);
   return new;
}

/*
OCCUR * aloc_memory_occur()
{
    //printf("Aloquei memoria occur\n");
    OCCUR* new = (OCCUR*) malloc(sizeof(OCCUR));
    new->path = NULL;
    new->thread = 0;
    new->n_paths = 0;
    new->pnext = NULL;
   return new;
}
*/

void insert_occur(pthread_t thread, char* path)
{
    printf("occurrences.n_paths = %d\n", occurrences.n_paths);

    PATHS * new =  aloc_memory_path(path);
      
    //primeira inserçãp
    if(occurrences.n_paths == 0)
    {
        printf("Inseri 1\n");
        occurrences.thread_id = thread;
        occurrences.pfirst = new;
        occurrences.n_paths += 1;
        occurrences.pnext = NULL;
        return;
    }else if(occurrences.n_paths != 0 && occurrences.pnext == NULL)
    {
        OCCUR temp = occurrences;
        while(temp.pnext != NULL)
        {
            if(pthread_equal(temp.thread_id , thread))
            {
                printf("Inseri 2 same\n");
                PATHS * temp_lastPath = temp.pfirst;
                temp_lastPath->pnext = NULL;
                temp.pfirst = new;
                new->pnext = temp_lastPath;
                temp.n_paths++;
                return;
            }
        }
        
        printf("Inseri 2 new\n");
        OCCUR * new_occur = (OCCUR*) malloc(sizeof(OCCUR));
        new_occur->thread_id = thread;
        new_occur->pfirst = new;
        new_occur->n_paths++;
        new_occur->pnext = NULL;
        
        temp.pnext = new_occur;
    }
    
    //find /Users/joaopfzousa/Documents/Faculdade/SO/ -name list_dir.c
    
    //Depois da primeira inserção
    OCCUR *temp = occurrences.pnext;
    
    while(temp != NULL)
    {
        printf("entrei");
        if(pthread_equal(temp->thread_id , thread))
        {
            printf("Inseri 2 same\n");
            PATHS * temp_lastPath = temp->pfirst;
            temp_lastPath->pnext = NULL;
            temp->pfirst = new;
            new->pnext = temp_lastPath;
            temp->n_paths++;
            return;
        }
        temp = temp->pnext;
    }
    
    printf("Inseri 2 new\n");
    OCCUR * new_occur = (OCCUR*) malloc(sizeof(OCCUR));
    new_occur->thread_id = thread;
    new_occur->pfirst = new;
    new_occur->n_paths++;
    new_occur->pnext = NULL;
    
    temp = new_occur;
}

void print_occur()
{

    puts("entrei print");
        
    OCCUR temp = occurrences;

    while(temp.pnext != NULL)
    {
        PATHS * temp_newPath = temp.pfirst;
        while(temp_newPath != NULL)
        {
            printf("[%lu] -> %s\n", temp.thread_id, temp_newPath->base_path);
            temp_newPath = temp_newPath->pnext;
        }
        temp = temp;
    }
}


int name (struct dirent * entry, char * value)
{
    if(strstr(value , entry->d_name) != NULL)
    {
        return 1; // return 1 if match found
    }
    return 0; // return 1 if match not found
}

int type (struct dirent * entry, char * value) {
    printf("Find by type: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int iname (struct dirent * entry, char * value) {
    printf("Find by iname: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int empty (struct dirent * entry, char * value) {
    printf("Find by empty: %s\n", value);

    //0 -> ficheiros / 64 -> diretórios

    return 1; // return 1 if match found
}

int executable (struct dirent * entry, char * value) {
    printf("Find by executable: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int mmin (struct dirent * entry, char * value) {
    printf("Find by mmin: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int size (struct dirent * entry, char * value) {
    printf("Find by size: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

T_DATA read_command( char *cmd, char **arg_list)
{
    char *token;
    fgets(cmd, MAX_CMD, stdin);
    cmd[strlen(cmd)-1] = '\0';
    token = strtok(cmd, " ");

    T_DATA t_data = { .args={NULL, ""}, .n_args=0 };
    if(strcmp(token, "find") == 0 )
    {
        arg_list[0] = token;
        token = strtok(NULL, " ");
        t_data.base_path = malloc(sizeof(char) * 300);
        t_data.base_path = token;
        token = strtok(NULL, " ");
    }
    
    while (token != NULL)
    {
        if(strcmp(token, "-name") == 0 || strcmp(token, "-iname") == 0 || strcmp(token, "-type") == 0 || strcmp(token, "-empty") == 0|| strcmp(token, "-executable") == 0 || strcmp(token, "-mmin") == 0 || strcmp(token, "-size") == 0)
        {
            remove_all_chars(token, '-');

            if(strcmp(token, "name") == 0)
            {
                t_data.args[t_data.n_args].opt = name;
            }else if(strcmp(token, "iname") == 0){
                t_data.args[t_data.n_args].opt = iname;
            }else if(strcmp(token, "type") == 0){
                t_data.args[t_data.n_args].opt = type;
            }else if(strcmp(token, "empty") == 0){
                t_data.args[t_data.n_args].opt = empty;
            }else if(strcmp(token, "executable") == 0){
                t_data.args[t_data.n_args].opt = executable;
            }else if(strcmp(token, "mmin") == 0){
                t_data.args[t_data.n_args].opt = mmin;
            }else if(strcmp(token, "size") == 0){
                t_data.args[t_data.n_args].opt = size;
            }

            //printf("t_data.args[%d].opt = %s\n", t_data.n_args, t_data.args[t_data.n_args].opt);

            if(strcmp(token, "empty") == 0 || strcmp(token, "executable") == 0 )
            {
                t_data.args[t_data.n_args].value = NULL;
                //printf("t_data.args[%d].value = %s\n", t_data.n_args, t_data.args[t_data.n_args].value);
            }else{
                token = strtok(NULL, " ");
                t_data.args[t_data.n_args].value = token;
                //printf("t_data.args[%d].value = %s\n", t_data.n_args, t_data.args[t_data.n_args].value);
            }
            t_data.n_args++;
        }
        token = strtok(NULL, " ");
    }

    return t_data;
}

int main(int argc, char **argv)
{
    char cmd[MAX_CMD];
    char *arg_list[5];

    while (TRUE)
    {
        prompt();
        T_DATA thread_data;
        pthread_t thread_id;
        thread_data = read_command (cmd, arg_list);

        if(strcmp(arg_list[0], "find") == 0)
        {
            pthread_create(&thread_id, NULL, &listDir, &thread_data);
            pthread_join(thread_id, NULL);

            print_occur();
        }else if(strcmp(arg_list[0], "clear") == 0)
        {
           printf("\033[H\033[J");
        }
    }
    return 0;
}

void * listDir(void * param)
{
    struct thread_data * my_data;
    my_data = (struct thread_data *) param;

    //printf(" my_data->n_args = %d\n", my_data->n_args);

    T_DATA thread_data[20];
    pthread_t thread_id[20];
    int i = 0, j = 0;

    DIR *dir;
    struct dirent *entry;
    char *path = malloc(sizeof(char) * 300);
    struct stat file_stat;

    //printf("\n%s\n\n", my_data->base_path);

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
                
                if (!my_data->args[j].opt(entry, my_data->args[j].value))
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
            
                
            /*
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
            */
           
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
                i++ ;
            }
        }

        strcpy(path, "");
        }
        closedir(dir);
    }

    for(int j = 0; j < i; j++)
        pthread_join(thread_id[j], NULL);
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

void prompt(){
    char path[100];
    getcwd(path, sizeof(path));

    char *res;
    res = strrchr(path, '/');
    printf("%s$", res);
}
