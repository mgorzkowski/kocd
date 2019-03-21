#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define msg_max_length 1024

char* device_name = "/dev/kocd";

int main()
{
	int fd = open(device_name, O_RDWR);
	char msg[msg_max_length+1] = {0};
	int how_many_bytes = 0;
	if (fd < 0)
	{
		printf("cannot open the file\n");
		return -1;
	}
	scanf("%d", &how_many_bytes);
	int l = read(fd, msg, how_many_bytes);
	printf("readed %d characters: %s\n", l, msg);
	close(fd);
	return 0;
}
