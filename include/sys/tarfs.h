#ifndef _TARFS_H
#define _TARFS_H

#define FT_DIR	0
#define FT_FILE	1
#define FILE_NAME_SIZE	128	
#define ROUND_TO_512(n) ((n + 512 - 1) & ~(512 - 1))
typedef struct tarfs_file{
	char mode[16];
	char name[FILE_NAME_SIZE];
	int free;
	int type;
	void *start_addr;
	uint64_t size;
	uint64_t pos;	
}tarfs_file;

typedef tarfs_file file;

#define START_FD	3	/* skip 0, 1, 2 on purpose */
#define TARFS_HEADER	512

extern char _binary_tarfs_start;
extern char _binary_tarfs_end;

struct posix_header_ustar {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
};

void init_tarfs(void);
int tarfs_open(char *filename, char *mode);
void *tarfs_read(int fd, uint64_t size);
void tarfs_close(int fd);
void test_tarfs();
void *traverse_tarfs(char *filename, void *start_addr, int (*f)(const char *, const char *), int *);
uint64_t get_filesz(struct posix_header_ustar *p);
#endif
