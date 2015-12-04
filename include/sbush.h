#define MAX_CMD_SIZE 256 // max sbush cmd size
#define MAX_ARG_NUM 8    // max argument number in a sbush cmd
#define MAX_SH_VAR_NUM 8 // max shell variable number
#define MAX_ENV_VAR_NUM 256 // max environment variable number
#define MAX_VAR_SIZE 2048 // max shell variable size
#define MAX_DIR_SIZE 256 // max directory/path size
#define MAX_BUFFER_SIZE 2048 // hostname size
#define MAX_SCRIPT_SIZE 4096 // max script file size
#define PAGE_SIZE (1<<12) // page size 4KB
//#define DEBUG

#define assert(cond) do {                                  \
                if (!(cond)) {                             \
                        printf("assert fail in %s, at line %d\n",__func__, __LINE__);     \
			while(1);				\
                }                                               \
        } while (0)

char** parse_cmds(char* cmd, int* arg_size); // parse user cmds into a list of strings
char* get_var(char** var_list, char* var_name); // get variable value by its name in var_list
void set_var(char** var_list, char* var_name, char* var_value); // set variable value in var_list
void set_path(char** var_list, char* path_value); // set path value, e.g. PATH=$PATH:/usr/bin
void set_PS1(char** var_list, char* PS1_value, char** envp); // set PS1 value, e.g. PS1=\u@\h:\w\$
char* parse_PS1(char* ps1, char** envp); // parse PS1 symbols into readable string
int min(int a, int b);
int max(int a, int b);
int does_file_exist(const char *filename);
char* locate_executable_file(char** var_list, char* filename);
char* myfgets(char* buf, int size, int fd);
void handle_signal(int signal);
char *get_fullpath(char *path, char *envp[]);
int list_file(char *fullpath);

#if defined(DEBUG) // code borrowed from kernel code piece with little modification
static void debug_printf(char *fmt, ...)
{
        char buffer[2048];
        va_list ap;

        va_start(ap, fmt);
        vsprintf(buffer, fmt, ap);
        va_end(ap);

        printf("%s",buffer);
}
#else
#define debug_printf(x...) do { } while (0)
#endif

