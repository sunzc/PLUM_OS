#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sbush.h>

static void execute(char** arg_list, int arg_size, char** sh_var, char** prompt, char* envp[]);

int main(int argc, char* argv[], char* envp[]){
	char *cmd, *tmp, *prompt;
	char** arg_list;
	char** SH_VAR_LIST;
	int arg_size;
	
	int fd, r;
	char *line, *script_buf, *p, *buf;
	int len;

	/* for the signal handler */
//	void (*res)(int);

	SH_VAR_LIST = (char**) malloc(MAX_SH_VAR_NUM * sizeof(char*));

	tmp = get_var(envp,"PATH=");
	if (tmp != NULL)
		set_var(SH_VAR_LIST, "PATH=", tmp);

	set_var(SH_VAR_LIST, "PS1=", "\\u@\\h:\\w\\$ ");
	tmp = get_var(SH_VAR_LIST,"PS1=");

	prompt = parse_PS1(tmp, envp);

	// read cmds from sbush scripts!
	if(argc == 2){
		len = strlen(argv[1]);
		if(len > 3 && 0 == strncmp(argv[1]+len-3,".sh", 3)){
			if (argv[1][0] == '/')
				fd = open(argv[1], O_RDONLY);
			else {
				p = locate_executable_file(SH_VAR_LIST, argv[1]);
				fd = open(p, O_RDONLY);
			}

			if(fd < 0){
				printf("error in main: file does not exists %s\n", argv[1]);
				return 0;
			}

			script_buf = (char*) malloc(MAX_SCRIPT_SIZE * sizeof(char));
			r = read(fd, script_buf, MAX_SCRIPT_SIZE);
			if(r < 0 || r >= MAX_SCRIPT_SIZE){
				printf("error in main: read script error! %s\n", argv[1]);
				return 0;
			}
				
			line = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
			buf = script_buf;
			while((p=strchr(buf,'\n')) != NULL) {
				len = strlen(buf) - strlen(p) + 1;
				strncpy(line, buf, len);
				line[len] = '\0';
				buf = p + 1;
				arg_list = parse_cmds(line, &arg_size);
				if(!arg_list[0])
					continue;
				execute(arg_list, arg_size, SH_VAR_LIST, &prompt, envp);
			}

			// last line without a '\n'
			if(buf != '\0'){
				strncpy(line, buf, strlen(buf));	
				line[strlen(buf)] = '\0';
				arg_list = parse_cmds(line, &arg_size);
				if(arg_list[0])
					execute(arg_list, arg_size, SH_VAR_LIST, &prompt, envp);
			}
			free(line);
			free(script_buf);
			close(fd);
		}

		return 0;
	}

	if(argc > 2){
		printf("OK, I'm still a baby! Don't play too much with me, or I will cry(ush)!\n");
		return 0;
	}

	/** if we are not running scripts, that is, running programs interactively
	 *  we don't Ctrl-C to terminate our shell, so just ignore it!
	 */
/*	res = signal(SIGINT, handle_signal);
	if(res == SIGERR){
		printf("signal failed!\n");
		exit(0);
		return 0;
	}
*/
	cmd =(char*) malloc(MAX_CMD_SIZE * sizeof(char));
	while(1){
		printf("%s",prompt);
		//scanf("%[^\n]%*c", cmd);
		myfgets(cmd, MAX_CMD_SIZE, 0);
		//fgets(cmd, MAX_CMD_SIZE, stdin);
		//fflush(stdin);
		arg_list = parse_cmds(cmd, &arg_size);
		if(!arg_list)
			continue;
		execute(arg_list, arg_size, SH_VAR_LIST, &prompt, envp);
	}
	return 0;
}

static void execute(char** arg_list, int arg_size, char** sh_var_list, char** prompt, char* envp[]){
	char *buf, *buf1, *ptr, *fullpath;
	int err, len, len1;
	int *status_ptr = NULL;
	pid_t pid;

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	buf[MAX_BUFFER_SIZE - 1] = '\0';
	buf1 = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	buf1[MAX_BUFFER_SIZE - 1] = '\0';

	if(arg_list[0] == NULL)
		return;

	if(0 == strcmp(arg_list[0],"cd") || 0 == strcmp(arg_list[0], "ls")){
		if(arg_size == 1){
			if(0 == strcmp(arg_list[0], "cd"))
				err = chdir(get_var(envp, "HOME"));
			else
				list_file(getcwd(NULL, MAX_BUFFER_SIZE - 1));
		} else if(arg_size == 2){

			fullpath = get_fullpath(arg_list[1], envp);
			debug_printf("execute:fullpath = %s\n", fullpath);

			if(0 == strcmp(arg_list[0],"cd")){ // cd
				err = chdir(fullpath);

				if (err != 0){
					printf("error in execute: wrong path given in cd!\n");//TODO:need to differentiate the err code
					free(buf);
					free(buf1);
					return;
				}
			}else// ls
				list_file(fullpath);
		}

	} else if((ptr=strchr(arg_list[0], '=')) != NULL) {// handles set sh variables

		if(arg_size > 1){
			printf("error in execute: unaccepted format!\n");
			free(buf);
			free(buf1);
			return;
		}

		len = strlen(ptr);// string length from '=' to the end, e.g. VAR="var_value"
		len1 = strlen(arg_list[0]);

		if(len1 > len){// get the var_name, cmd not begin with '='
			strncpy(buf, arg_list[0], len1 - len + 1);
			buf[len1 - len + 1] = '\0';
		}else{
			printf("error in execute: unknown commands!\n");
			free(buf);
			free(buf1);
			return;
		}

		// check value, "abc" equals abc, note *ptr='='
		if(ptr[1] == '"' && len >= 3 && ptr[len - 1] == '"'){
			strncpy(buf1, ptr + 2, len - 3);
			buf1[len - 3] = '\0';
		}else if(ptr[1] != '"' && ptr[len - 1] != '"'){
			strncpy(buf1, ptr + 1, len - 1);
			buf1[len - 1] = '\0';
		}else{
			printf("error in execute: unaccepted var values!\n");
			free(buf);
			free(buf1);
			return;
		}

		if(0 == strncmp(buf,"PATH=", 5))
			set_path(sh_var_list, buf1);
		else if(0 == strncmp(buf, "PS1=", 4))
			set_PS1(sh_var_list, buf1, envp);
		else
			set_var(sh_var_list, buf, buf1);
	} else if (0 == strcmp(arg_list[0],"exit") && 1 == arg_size) {
		exit(0);
		
	} else {
		ptr = locate_executable_file(sh_var_list, arg_list[0]);
		if(ptr){
			// TODO: execute this file with args from arg_list 1..
			debug_printf("executable file is %s\n",ptr);
			pid = fork();
			if(pid == 0){
				// child process
				err = execve(ptr, arg_list, envp);
				if(err < 0)
					printf("error in child process: file not executable!\n");
				exit(-1);
			} else if (pid > 0){
				if(0 != strcmp(arg_list[arg_size - 1], "&"))
					waitpid(pid, status_ptr, 0);
				free(buf);
				free(buf1);
				return;
			} else {
				printf("error in execute: fork process error!\n");
				free(buf);
				free(buf1);
				return;
			}

		} else {
			free(buf);
			free(buf1);
			return;
		}
	}

	*prompt = parse_PS1(get_var(sh_var_list, "PS1="), envp);
}
