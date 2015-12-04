#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sbush.h>

char* get_var(char** var_list, char* var_name){//cut var_value if length > MAX_VAR_SIZE
	char *var_value;
	int i, j, nlen, vlen;

	var_value = (char*) malloc(MAX_VAR_SIZE * sizeof(char));

	if(var_list == NULL){
		free(var_value);
		return NULL;
	}

	i = 0;
	nlen = min(strlen(var_name), MAX_VAR_SIZE);
	while(i < MAX_ENV_VAR_NUM && var_list[i]!=NULL && strncmp(var_name, var_list[i], nlen)!=0)
		i++;

	if (var_list[i] != NULL && 0 == strncmp(var_name, var_list[i], nlen)){
		//debug_printf("get_var:i = %d, var_list[i] = %s\n",i, var_list[i]);
		vlen = min(strlen(var_list[i]) - nlen, MAX_VAR_SIZE - 1);
		//debug_printf("get_var:nlen = %d, vlen= %d var_list[i] = %s\n",nlen, vlen, var_list[i]);
		//strncpy(var_value, var_list[i] + nlen, vlen); 
		for(j = 0; j < vlen; j++){
			var_value[j] = var_list[i][nlen+j];
		}
		//debug_printf("get_var:after strncpy: var_value = %s\n",var_value);
		var_value[vlen] = '\0';
		return var_value;
	}

	free(var_value);
	return NULL;
}

void set_PS1(char** var_list, char* new_PS1, char** envp){
	if (NULL == parse_PS1(new_PS1, envp)){
		printf("error in set_PS1: unsupported PS1 format!\n");
		return;
	}else
		set_var(var_list, "PS1=", new_PS1);
}

void set_path(char** var_list, char* new_path){
	char *old_path;
	char *new_path_entry;
	int i, j, len;

	new_path_entry = (char*) malloc(MAX_VAR_SIZE * sizeof(char));

	i = 0; j = 0;
	old_path = get_var(var_list, "PATH=");
	while(new_path[i] != '\0' && i < MAX_VAR_SIZE && j < MAX_VAR_SIZE){
		if(new_path[i] != '$')
			new_path_entry[j++] = new_path[i++];
		else if(   new_path[i+1] == 'P' &&
			   new_path[i+2] == 'A' &&
			   new_path[i+3] == 'T' &&
			   new_path[i+4] == 'H') {
				if(old_path != NULL){
					len = strlen(old_path);
					strncpy(new_path_entry + j, old_path, len);
					debug_printf("set_path:len = %d, old_path =%s\n",len,old_path);	
					j += len;
					i += 5;
					continue;	
				}else{
					i += 5;
					continue;
				}
			}
		else{
			printf("error in set_path: unaccepted PATH format!\n");
			free(new_path_entry);
			return;
		}
	}

	if( j < MAX_VAR_SIZE && i < MAX_VAR_SIZE){
		new_path_entry[j] = '\0';
		set_var(var_list, "PATH=", new_path_entry);
	}else{
		printf("error in set_path: PATH value too long!\n");
		free(new_path_entry);
	}
	
	return;
}

void set_var(char** var_list, char* var_name, char* var_value){
	char *var_entry;
	int i, len, vlen;

	// copy variable name to var_entry
	var_entry = (char*) malloc(MAX_VAR_SIZE * sizeof(char));
	len = min(strlen(var_name), MAX_VAR_SIZE - 1);
	strncpy(var_entry, var_name, len);

	// copy variable value to var_entry
	vlen = min(strlen(var_value),MAX_VAR_SIZE - len - 1);
	strncpy(var_entry + len, var_value, vlen);
	var_entry[len+vlen] = '\0';

	// find existing variable
	i = 0;
	while(i < MAX_SH_VAR_NUM && var_list[i]!=NULL && strncmp(var_name, var_list[i], len)!=0)
		i++;

	if (MAX_SH_VAR_NUM == i){
		free(var_entry);
		printf("error in set_var: no more room for new shell variables!");
		return;
	}

	var_list[i] = var_entry; // no matter it exists or not i points to a usable location,just fill in it!
	return;
}

char* parse_PS1(char* ps1, char** envp){
	/** here we provide a very limitted PS1 settings, we only support
	\h: hostname
	\u: user name
	\w: working directory
	\$: the prompt
	*/
	char *ps1_value, *tmp, *username;
	int i, j, len;

	if (ps1 == NULL)
		return NULL;
	
	ps1_value = (char*) malloc(MAX_VAR_SIZE * sizeof(char));
	tmp = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	tmp[MAX_BUFFER_SIZE - 1] = '\0';

	i = 0; j = 0;
	while(ps1[i]!='\0' && i < MAX_VAR_SIZE && j < MAX_VAR_SIZE){
		if(ps1[i] != '\\'){
			ps1_value[j++] = ps1[i++];
		}else{
			switch(ps1[i + 1]){
				case 'h':
					gethostname(tmp, MAX_BUFFER_SIZE - 1);
					break;
				case 'u':
					username=get_var(envp, "USER=");
					if(username != NULL)
						strncpy(tmp,username,strlen(username));
					else
						tmp = NULL;
					break;
				case 'w':
					getcwd(tmp, MAX_BUFFER_SIZE - 1);
					username=get_var(envp, "USER=");
					if(0 == strncmp(tmp+6,username,strlen(username))){
						tmp[0]='~';
						strncpy(tmp+1, tmp+strlen(username)+6, strlen(tmp) - strlen(username) - 6);
						tmp[strlen(tmp) - strlen(username) - 6 + 1] = '\0';
					}
					break;
				case '$':
					tmp[0] = '$';
					tmp[1] = '\0';
					break;
				default:
					free(tmp);
					free(ps1_value);
					printf("error in parse_PS1: not supported PS1 format!\n");
					return NULL;
			}

			if(tmp != NULL) {
				len = min(strlen(tmp), MAX_VAR_SIZE - j - 1);
				strncpy(ps1_value+j, tmp, len);
				j += len; i += 2;
			} else {
				i += 2;
			}
			continue;
		}
	}

	if(j < MAX_VAR_SIZE)
		ps1_value[j] = '\0';
	else
		ps1_value[MAX_VAR_SIZE - 1] = '\0';

	free(tmp);

	return ps1_value;
}

char** parse_cmds(char* cmd, int* arg_size){
	char** arg_list;//TODO:assume initialized to NULLs
	char *p, *tmp;
	int i, j;
	int quote_flag;
	
	arg_list = (char**) malloc(MAX_ARG_NUM * sizeof(char*));	

	j = 0;// arg number, < MAX_ARG_NUM
	p = cmd;
	debug_printf("szc:parse_cmds:cmd is %s\n", cmd);
	while(*p && *p != '#' && *p != '\n' && j<MAX_ARG_NUM){
		while(*p == ' ')
			p++;// skip spaces

		i = 0;
		tmp = (char*) malloc(MAX_CMD_SIZE * sizeof(char));//TODO:suppose malloc always success
		quote_flag=0;
		while(*p && *p != '\n' && *p != '#' && i < MAX_CMD_SIZE - 1){
			if(*p == '"' && quote_flag == 0)
				quote_flag = 1;
			else if(*p == '"' && quote_flag == 1){
				if(*(p+1) != ' ' && *(p+1) != '\n' && *(p+1) != '\0'){//ugly input like ABC="ab c"d
					printf("error in parse_cmds: ugly input!\n");
					free(tmp);//TODO maybe we should free the whole arg_list
					return NULL;
				} else {
					tmp[i++] = *p++;
					break;
				}
			}

			if(*p == ' ' && quote_flag == 0)
				break;
			else
				tmp[i++] = *p++;
		}
		
		//take one arg
		if(i > 0 && i < MAX_CMD_SIZE){
			tmp[i] = '\0';
			arg_list[j++] = tmp;
		}else if(i >= MAX_CMD_SIZE){
			printf("error: argument too long!\n");
			return NULL;
		}else if(*p == '\n' || *p == '#' || !*p)
			break;		
		
		debug_printf("szc:parse_cmds:tmp is %s , j = %d\n", tmp, j);
	}
	
	if(j >= MAX_ARG_NUM){
			printf("error: too many arguments!\n");
			return NULL;
	}

	*arg_size = j;
	arg_list[j] = NULL;
	return arg_list;
}

int min(int a, int b){
	return a>b ? b:a;
}

int max(int a, int b){
	return a<b ? b:a;
}

int does_file_exist(const char *filename) {
    int fd = open(filename, O_RDONLY);
    return fd < 0?0:1;
}

char* locate_executable_file(char **var_list, char *filename){
	char *path, *pre, *p;
	char *buf;
	int last_len, cur_len;
	path = get_var(var_list,"PATH=");

	// check if file exists in current working directory
	if(does_file_exist(filename))
		return filename;

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));

	pre = path;
	last_len = strlen(pre);
	while(last_len > 0 && pre != NULL && (p = strchr(pre, ':')) != NULL){
		cur_len = strlen(p);
		strncpy(buf, pre, last_len - cur_len);
		if (buf[last_len - cur_len - 1] != '/') {
			buf[last_len - cur_len] = '/';
			strncpy(buf+last_len - cur_len + 1, filename, strlen(filename));
			buf[last_len - cur_len + 1 + strlen(filename)] = '\0';
		} else {
			strncpy(buf+last_len - cur_len, filename, strlen(filename));
			buf[last_len - cur_len + strlen(filename)] = '\0';
		}

		if((strlen(filename) + last_len - cur_len + 1) > MAX_BUFFER_SIZE - 1){
			printf("error in locate_executable_file: filename too long!\n");
			free(buf);
			return NULL;
		}

		if(does_file_exist(buf))
			return buf;

		last_len = cur_len - 1;
		pre = p + 1;
	}

	if(last_len > 0 && pre != NULL && (p = strchr(pre, ':')) == NULL){
		cur_len = strlen(pre);
		strncpy(buf, pre, cur_len);
		buf[cur_len] = '/';

		if((strlen(filename) + cur_len + 1) < MAX_BUFFER_SIZE - 1){
			strncpy(buf + cur_len + 1, filename, strlen(filename));
		} else {
			printf("error in locate_executable_file: filename too long!\n");
			free(buf);
			return NULL;
		}

		buf[cur_len + 1 + strlen(filename)] = '\0';
		if(does_file_exist(buf))
			return buf;

	}

	printf("error in locate_executable_file: filename does not exist!\n");
	free(buf);
	return NULL;
}

char* myfgets(char* buf, int size, int fd){
	char cmd_buf[MAX_CMD_SIZE];
	int ret, i;

	i = 0;
	while((ret = read(fd, cmd_buf, 1)) > 0 && cmd_buf[0] != '\n' && cmd_buf[0] != '\0' && i< size)
		buf[i++] = cmd_buf[0];

	buf[i] = '\0';
	return  buf;	
}

void handle_signal(int signal) {
	printf("^C\n");
}

char *get_fullpath(char *path, char *envp[]) {
	int len, len1;
	char hc, *ptr, *buf = NULL;

	if (!path) //check if it's empty path
		return NULL;

	buf = (char*) malloc(MAX_BUFFER_SIZE * sizeof(char));
	hc = path[0];

	if (hc != '/' && hc != '~') { // a relative path
		ptr = getcwd(NULL, MAX_BUFFER_SIZE - 1);

		if (ptr != NULL) {
			len = strlen(ptr);
			len1 = strlen(path);

			assert(len + len1 < MAX_BUFFER_SIZE);

			strncpy(buf, ptr, len);
			if(ptr[len - 1] != '/')
				buf[len++] = '/';

			strncpy(buf + len, path, len1);
			buf[len1 + len] = '\0';
		} else
			printf("ERROR: can't get current working directory!\n");
	}else if(hc == '~'){
		ptr = get_var(envp, "HOME=");

		if(ptr != NULL){
			len = strlen(ptr);
			len1 = strlen(path) - 1;	//exclude '~'

			assert(len + len1 < MAX_BUFFER_SIZE);

			strncpy(buf, ptr, len);
			if(ptr[len - 1] != '/')
				buf[len++] = '/';

			if(strlen(path) > 1){
				strncpy(buf + len, path + 1, len1);
				buf[len1 + len] = '\0';
			} else
				buf[len] = '\0';

		} else
			printf("error in execute: can't get env HOME!\n");

	} else {
		strncpy(buf, path,strlen(path));
		buf[strlen(path)] = '\0';
	}

	return buf;
}

int list_file(char *fullpath) {
	struct dirent *entry;
	DIR *dir;	

//	printf("[list_file] fullpath:0x%lx\n",fullpath);

	if(fullpath)
		dir = opendir(fullpath);
	else
		return -1;

	if (!dir)
		return -1;
	else{
		while ((entry = readdir(dir)) != NULL) {
			entry->d_name[10] = '\0';
			printf("%s ", entry->d_name);
		}

//		printf("[list_file] fullpath:0x%lx\n",fullpath);

		printf("\n");
	}

	return 0;
}
