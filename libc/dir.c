#include<stdlib.h>
#include<syscall.h>

/* borrow a lot from 
 * https://chromium.googlesource.com/native_client/nacl-newlib/+/bf66148d14c7fca26b9198dd5dc81e743893bb66/newlib/libc/posix/(opendir.c, readdir.c)
 */
int getdents(unsigned int fd, struct dirent *dirp, unsigned int count){
	return (int) syscall_3(SYS_getdents, (uint64_t) fd, (uint64_t) dirp, (uint64_t) count);
} 

void *opendir(const char *name){
	DIR *dirp;
	int fd;

	if((fd = open(name, 0)) == -1)
		return NULL;

	if((dirp = (DIR *)malloc(sizeof(DIR))) == NULL){
		close(fd);
		return NULL;
	}

	dirp->dd_buf = malloc(512);
	dirp->dd_len = 512;

	if(dirp->dd_buf == NULL) {
		free(dirp);
		close(fd);
		return NULL;
	}

	dirp->dd_fd = fd;
	dirp->dd_loc = 0;
	dirp->dd_seek = 0;

	return dirp;
}

struct dirent *readdir(void *dir){
	DIR* dirp = (DIR *)dir;
	struct dirent *dp;

	for(;;){
		if(dirp->dd_loc == 0) {
			dirp->dd_size = getdents (dirp->dd_fd,
						  (struct dirent *)dirp->dd_buf,
						  dirp->dd_len);
			if(dirp->dd_size <= 0) {
				return NULL;
			}
		}

		if(dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}

		dp = (struct dirent *)(dirp->dd_buf + dirp->dd_loc);
		if(dp->d_reclen <= 0 ||
			dp->d_reclen > dirp->dd_len + 1 - dirp->dd_loc) {
			return NULL;
		}

		dirp->dd_loc += dp->d_reclen;
		if(dp->d_ino == 0)
			continue;

		return dp;
	}
}

int closedir(void *dir){
	int rc;

	rc = close(((DIR *)dir)->dd_fd);

	free((void*)((DIR *)dir)->dd_buf);
	free((void*)dir);

	return rc;
}
