#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define msg_max_length 1024

char* device_name = "/dev/kocd";

int main()
{
	int fd = open(device_name, O_RDWR);
	char msg[msg_max_length] = {};
	if (fd < 0)
	{
		printf("cannot open the file\n");
		return -1;
	}
	scanf("%s", msg);
	int l = write(fd, msg, strlen(msg));
	printf("writed %d characters\n", l);
	close(fd);
	return 0;
}
