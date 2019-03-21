#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

char *device_name = "/dev/kocd";
#define RST_BUF	0xF0

int main()
{
	int fd = open(device_name, O_RDWR);
	ioctl(fd, RESET_NO, 0);
	close(fd);
	return 0;
}
