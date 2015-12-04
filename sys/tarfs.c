#include <sys/sbunix.h>
#include <sys/string.h>
#include <sys/proc.h>
#include <sys/tarfs.h>

//#define DEBUG_TARFS

extern task_struct *current;


static void show_tarfs_header(struct posix_header_ustar *p);
static uint64_t octstr_to_int(char *size_str); 
static void zero_tarfs_file_entry(tarfs_file entry);

void init_tarfs() {
	/* show some info about tarfs content */
	show_tarfs_header((struct posix_header_ustar *) &_binary_tarfs_start);
}

void test_tarfs() {
	int i, fd;
	void *buf;
	char *filename="bin/hello";

	if ((fd = tarfs_open(filename,"r")) == -1) {
		printf("file not found!\n");
		return;
	}

	printf("fd : %d\n", fd);
	buf = tarfs_read(fd, 100);

	printf("file %s first 100 bytes:\n", filename);
	for (i = 0; i< 100; i++)
		printf("%c",buf+i);

	printf("file %s second 100 bytes:\n", filename);
	buf = tarfs_read(fd, 100);
	for (i = 0; i< 100; i++)
		printf("%c",buf+i);
	
	return;
}

static void zero_tarfs_file_entry(tarfs_file entry) {
	entry.mode[0] = '\0';
	entry.name[0] = '\0';
	entry.free = 0;
	entry.start_addr = NULL;
	entry.size = 0;
	entry.pos = 0;
}
		
void *traverse_tarfs(char *filename, void *start_addr, int (*checker)(const char *, const char *), int *type) {
	void *p;
	void *res = NULL;
	int len, namesz;
	char name[64];

	namesz = strlen(filename);
	// traverse from start_addr, usually is &_binary_tarfs_start;
	p = start_addr;
	while (p < (void *)&_binary_tarfs_end) {
		/* directory */
		if (((struct posix_header_ustar *)p)->typeflag[0] == '5') {
			/* handle dir */
			if (filename[namesz - 1] != '/') {
				strncpy(name, filename, namesz);
				name[namesz] = '/';
				name[namesz + 1] = '\0';
			} else
				strncpy(name, filename, namesz);
			if (checker(name, ((struct posix_header_ustar *)p)->name) == 0) {
				*type = FT_DIR;
				res = p;
				break;
			} else {
				p += TARFS_HEADER;
				continue;
			}
#ifdef DEBUG_TARFS
			show_tarfs_header((struct posix_header_ustar *)p);
#endif
		}

		/* handle file */
		if (checker(filename, ((struct posix_header_ustar *)p)->name) != 0) {
			len = get_filesz((struct posix_header_ustar *)p) + TARFS_HEADER;

#ifdef DEBUG_TARFS
			show_tarfs_header((struct posix_header_ustar *)p);
			printf("[open] file size :0x%lx, after round_to_512: 0x%lx\n", len, ROUND_TO_512(len));
#endif

			p += ROUND_TO_512(len);
			continue;
		} else {

#ifdef DEBUG_TARFS
			show_tarfs_header((struct posix_header_ustar *)p);
#endif

			*type = FT_FILE;
			res = p;
			break;
		}
	}

	return res;
}

int tarfs_open(char *filename, char *mode) {
	int i, type, skip = 0;
	int isroot = 0;
	void *res = NULL;
	char newfname[NAME_SIZE];

#ifdef DEBUG_TARFS
	printf("[tarfs_open]filename:%s\n",filename);
#endif

	// we faked root directory, but when open it we should remove it
	while(filename[skip] == '/')
		skip++;

	strncpy(newfname, filename + skip, strlen(filename) - skip);

#ifdef DEBUG_TARFS
	printf("[tarfs_open]nfilename:%s\n",newfname);
#endif

	//&_binary_tarfs_end
	if (strcmp(filename, "/") == 0 || strcmp(filename, "//") == 0 || strcmp(filename, "///") == 0) {
		isroot = 1;
		res = (void *)1;	// special value for root
	} else
		res = traverse_tarfs(newfname, &_binary_tarfs_start, strcmp, &type);

	if (res == NULL)
		return -1;

	// we need to fake root dir '/' here
	for (i = START_FD; i < MAX_FILE_NUM; i++) {
		if (current->file_array[i].free == 0) {
			assert(strlen(mode) < 16);
			assert(strlen(filename) < FILE_NAME_SIZE);

			current->file_array[i].free = 1;
			if (isroot) {
				current->file_array[i].start_addr = &_binary_tarfs_start;
				strncpy(current->file_array[i].name, "/", 1);
#ifdef DEBUG_TARFS
				printf("[tarfs_open] filename stored:%s\n", current->file_array[i].name);
#endif
				type = FT_DIR;
			} else {
				current->file_array[i].start_addr = res + TARFS_HEADER;
				strncpy(current->file_array[i].name, newfname, strlen(newfname));
#ifdef DEBUG_TARFS
				printf("[tarfs_open] filename stored:%s\n", current->file_array[i].name);
#endif
			}


			strncpy(current->file_array[i].mode, mode, strlen(mode));

			if (type == FT_FILE) {
				current->file_array[i].size = ROUND_TO_512(get_filesz(res));
				current->file_array[i].type = FT_FILE;
			} else if (type == FT_DIR) {
				/* when it's directory, start_addr points the next TARFS_HEADER following this dir */
				current->file_array[i].type = FT_DIR;
			}

			return i;
		}
	}

	panic("ERROR:[in tarfs_open] no free file struct cache!\n");
	return -1;
}	

void * tarfs_read(int fd, uint64_t size) {
	void *p;

#ifdef DEBUG_TARFS
	printf("[tarfs_read]fd = %d\n", fd);
#endif
	assert(fd >= START_FD && fd < MAX_FILE_NUM);
	assert(current->file_array[fd].free > 0);
	assert(current->file_array[fd].pos + size <= current->file_array[fd].size);

	p = current->file_array[fd].start_addr + current->file_array[fd].pos;
	current->file_array[fd].pos += size;

	return p;
}

void tarfs_close(int fd) {
	assert(fd >= START_FD && fd < MAX_FILE_NUM);
	zero_tarfs_file_entry(current->file_array[fd]);
}

static void show_tarfs_header(struct posix_header_ustar *p) {
	printf("name	 :%s\n", p->name);
	printf("mode     :%s, ", p->mode);
	printf("uid      :%s,", p->uid);
	printf("gid      :%s,", p->gid);
	printf("size     :%s,", p->size);
	printf("mtime    :%s,", p->mtime);
	printf("checksum :%s,", p->checksum);
	printf("typeflag :%s,", p->typeflag);
	printf("linkname :%s,", p->linkname);
	printf("magic    :%s,", p->magic);
	printf("version  :%s,", p->version);
	printf("uname    :%s,", p->uname);
	printf("gname    :%s,", p->gname);
	printf("devmajor :%s,", p->devmajor);
	printf("devminor :%s,", p->devminor);
	printf("prefix   :%s,", p->prefix);
	printf("pad	 :%s\n", p->pad);
}

uint64_t get_filesz(struct posix_header_ustar *p) {
	return octstr_to_int(((struct posix_header_ustar *)p)->size);
}

static uint64_t octstr_to_int(char *size_str) {
	int i;
	uint64_t size;

	i = 0; size = 0;
	while(i < strlen(size_str)) {
		size = size * 8 + (size_str[i++] - '0');
	}

#ifdef DEBUG_TARFS
	printf("[octstr_to_int] size_string:%s, size:0x%lx\n", size_str, size);
#endif
	return size;
}
