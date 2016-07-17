#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#define DEV_FILE "/dev/ssd0"

int main(int argc, char *argv[])
{
	int fd, ret = 0;
	char buf[128]="";
	char wr_buf[]="Hey i reach to you are you capable of holding me";
	int mode = O_RDONLY;
	if(argv[1]) 
		mode = O_WRONLY;
	fprintf(stderr,"opening mode %s\n",mode == O_WRONLY?"O_WRONLY":"O_RDONLY"); 	
	fd = open(DEV_FILE, mode);
	if (fd < 0) {
		perror(DEV_FILE);
		return 0;
	} 
	if (mode == O_WRONLY) {
		ret = write(fd,wr_buf,strlen(wr_buf));
		fprintf(stderr,"bytes:buf %d out of %d\n",ret,strlen(wr_buf));
	} else { 
		fprintf(stderr,"sleep distrubance %d\n",ret);
		ret = sleep(10);
		fprintf(stderr,"sleep distrubance %d\n",ret);
		ret = read(fd,buf,sizeof(buf));
		fprintf(stderr,"bytes:buf %d:%s %d\n",ret,buf,strlen(buf));
	}
	close(fd);
return 0;
}
