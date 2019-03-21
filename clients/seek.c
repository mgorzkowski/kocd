#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define msg_max_length 1024

char* device_name = "/dev/kocd";

int main()
{
	int fd = open(device_name, O_RDWR);
	int mode = 0, offset = 0;
	if (fd < 0)
	{
		printf("cannot open the file\n");
		return -1;
	}
	scanf("%d %d", &offset, &mode);
	int ret = lseek(fd, offset, mode);
	printf("ret %d\n", ret);
	close(fd);
	return 0;
}
