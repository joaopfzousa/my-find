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

typedef int (*PARAM)(/*struct dirent * entry,*/ char * value);

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


int name (/*struct dirent * entry,*/ char * value) {
    printf("Find by name: %s\n", value);

    //todo

    return 1; // return 1 if match found
}

int type (/*struct dirent * entry,*/ char * value) {
    printf("Find by type: %s\n", value);

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

void read_command( char *cmd, char **arg_list)
{
    char *token;
    int i = 0;
    fgets(cmd, MAX_CMD, stdin);
    cmd[strlen(cmd)-1] = '\0';
    token = strtok(cmd, " ");
    while (token != NULL)
    {
        arg_list[i++] = token;
        token = strtok(NULL, " ");
    }
    arg_list[i] = NULL;
}

int main(int argc, char **argv)
{
    char cmd[MAX_CMD]; 
    char *arg_list[5];

    int i=0;

	T_DATA t_data = { .args={NULL, ""}, .n_args=0 };

    t_data.args[t_data.n_args].opt = name;
	t_data.args[t_data.n_args].value = ".txt";
	t_data.n_args++;

	t_data.args[t_data.n_args].opt = type;
	t_data.args[t_data.n_args].value = "f";
	t_data.n_args++;

    while (TRUE)
    {
        prompt();
        read_command (cmd, arg_list);
        
        if(strcmp(arg_list[0], "find") == 0)
        {
            listDir("/Users/joaopfzousa/Documents/Faculdade/SO/", 0);
        }else if(strcmp(arg_list[0], "clear") == 0)
        {
           printf("\033[H\033[J");
        }
    }

    // Loop over entries

		// for each entry
		for (i=0 ; i<t_data.n_args ; i++)
			if (!t_data.args[i].opt(/*entry,*/ t_data.args[i].value))
				break;

		i == t_data.n_args ? printf("match\n") : printf("No match\n"); 

	// end loop over entries
    
    return 0;
}