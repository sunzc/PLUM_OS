#ifndef _TARFS_H
#define _TARFS_H

typedef struct tarfs_file{
	char mode[16];
	int free;
	void *start_addr;
	uint64_t size;
	uint64_t pos;	
}tarfs_file;

#define MAX_TARFS_FILE	20
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

#endif
