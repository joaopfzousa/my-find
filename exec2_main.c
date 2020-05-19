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

#define MAX_CMD 100
#define TRUE 1

typedef int (*PARAM)(struct dirent * entry, char * value);

typedef struct arg {
    PARAM opt;
    char * value;
}ARG;

typedef struct thread_data {
	//path
	//...
    ARG args[50];
    int n_args;
}T_DATA;


int name (struct dirent * entry, char * value) {
    printf("Find by name: %s\n", value);

    //todo

    return 1; // return 1 if match found
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


void prompt(){
    char path[100];
    getcwd(path, sizeof(path));

    char *res;
    res = strrchr(path, '/');
    printf("%s$", res);
}

void listDir(const char *name)
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

                for (int i=0 ; i<t_data.n_args ; i++)
                    if (!t_data.args[i].opt(path, t_data.args[i].value))
                        break;

                i == t_data.n_args ? printf("match\n") : printf("No match\n"); 

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
                    listDir(path);
                }
            } 
            strcpy(path, "");
        }
        closedir(dir);
    }
}

void remove_all_chars(char* str, char c) {
    char *pr = str, *pw = str;
    while (*pr) {
        *pw = *pr++;
        pw += (*pw != c);
    }
    *pw = '\0';
}


T_DATA read_command( char *cmd, char **arg_list)
{
    char *token;
    int i = 0;
    fgets(cmd, MAX_CMD, stdin);
    cmd[strlen(cmd)-1] = '\0';
    token = strtok(cmd, " ");

    if(strcmp(token, "find") == 0 )
    {
        arg_list[0] = token;
        token = strtok(NULL, " ");
        arg_list[1] = token;
        token = strtok(NULL, " ");
    }

    T_DATA t_data = { .args={NULL, ""}, .n_args=0 };
    while (token != NULL)
    {    
        if(strcmp(token, "-name") == 0 || strcmp(token, "-iname") == 0 || strcmp(token, "-type") == 0 || strcmp(token, "-empty") == 0|| strcmp(token, "-executable") == 0 || strcmp(token, "-mmin") == 0 || strcmp(token, "-size") == 0)
        {
            remove_all_chars(token, '-');
            //printf("token option = %s\n", token);
            t_data.args[t_data.n_args].opt = (PARAM) token;

            if(strcmp(token, "empty") == 0 || strcmp(token, "executable") == 0 )
            {
                t_data.args[t_data.n_args].value = NULL;
                 //printf("token  value = %s\n", t_data.args[t_data.n_args].value);
            }else{
                token = strtok(NULL, " ");
                //printf("token  value = %s\n", token);
                t_data.args[t_data.n_args].value = token;
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
        T_DATA t_data = { .args={NULL, ""}, .n_args=0 };
        t_data = read_command (cmd, arg_list);

        int i=0;
        if(strcmp(arg_list[0], "find") == 0)
        {            
            char *caminho = arg_list[1];
       
            listDir(caminho);
            // Loop over entries

            
            // for each entry
            
           
            

        // end loop over entries
            //listDir("/Users/joaopfzousa/Documents/Faculdade/SO/", 0);
        }else if(strcmp(arg_list[0], "clear") == 0)
        {
           printf("\033[H\033[J");
        }
    }

    return 0;
}