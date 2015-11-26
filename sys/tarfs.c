#include <sys/sbunix.h>
#include <sys/string.h>
#include <sys/tarfs.h>

//open DEBUG_TARFS to print info about read or open
//#define DEBUG_TARFS

#define ROUND_TO_512(n) ((n + 512 - 1) & ~(512 - 1))
tarfs_file tarfs_file_array[MAX_TARFS_FILE];

static void show_tarfs_header(struct posix_header_ustar *p);
static uint64_t get_filesz(struct posix_header_ustar *p);
static uint64_t octstr_to_int(char *size_str); 
static void zero_tarfs_file_entry(tarfs_file entry);

void init_tarfs() {
	int i;
	void *p;

	for (i = START_FD; i < MAX_TARFS_FILE; i++)
		zero_tarfs_file_entry(tarfs_file_array[i]);

	/* show some info about tarfs content */
	p = (void *)&_binary_tarfs_start;
	show_tarfs_header((struct posix_header_ustar *)p);
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
	entry.free = 0;
	entry.start_addr = NULL;
	entry.size = 0;
	entry.pos = 0;

	return;
}
		
int tarfs_open(char *filename, char *mode) {
	int i;
	uint64_t len;
	void *p;
	void *res = NULL;

	//&_binary_tarfs_end
	p = (void *)&_binary_tarfs_start;
	while (p < (void *)&_binary_tarfs_end) {
		/* skip directory */
		if (((struct posix_header_ustar *)p)->typeflag[0] == '5') {
#ifdef DEBUG_TARFS
			show_tarfs_header((struct posix_header_ustar *)p);
#endif
			p += TARFS_HEADER;
			continue;
		}

		/* handle file */
		if (strcmp(((struct posix_header_ustar *)p)->name, filename) != 0) {
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
			res = p;
			break;
		}
	}

	if (res == NULL)
		return -1;

	for (i = START_FD; i < MAX_TARFS_FILE; i++) {
		if (tarfs_file_array[i].free == 0) {
			tarfs_file_array[i].free = 1;
			assert(strlen(mode) < 16);
			strncpy(tarfs_file_array[i].mode, mode, strlen(mode));
			tarfs_file_array[i].size = ROUND_TO_512(get_filesz(res));
			tarfs_file_array[i].start_addr = res + TARFS_HEADER;
			return i;
		}
	}

	panic("ERROR:[in tarfs_open] no free file struct cache!\n");
	return 0;
}	

void * tarfs_read(int fd, uint64_t size) {
	void *p;

	printf("[tarfs_read]fd = %d\n", fd);
	assert(fd >= START_FD && fd < MAX_TARFS_FILE);
	assert(tarfs_file_array[fd].free > 0);
	assert(tarfs_file_array[fd].pos + size <= tarfs_file_array[fd].size);

	p = tarfs_file_array[fd].start_addr + tarfs_file_array[fd].pos;
	tarfs_file_array[fd].pos += size;

	return p;
}

void tarfs_close(int fd) {
	assert(fd >= START_FD && fd < MAX_TARFS_FILE);
	zero_tarfs_file_entry(tarfs_file_array[fd]);
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

static uint64_t get_filesz(struct posix_header_ustar *p) {
	return octstr_to_int(((struct posix_header_ustar *)p)->size);
}

static uint64_t octstr_to_int(char *size_str) {
	int len, i;
	uint64_t size = 0;

	i = 0;
	len = strlen(size_str);
	while(i < len) {
		size = size * 8 + (size_str[i] - '0');
		i++;
	}

#ifdef DEBUG_TARFS
	printf("[octstr_to_int] size_string:%s, size:0x%lx\n", size_str, size);
#endif
	return size;
}
