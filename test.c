#include <stdio.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char buf[100];
	char i=0;
	memset(buf,0,100);

	int fp = open("/dev/keylog0",O_RDWR);
	write(fp,"test func ronit",9);
	i=read(fp,buf,30);
	buf[i++]='\n';
	printf("buf=%s\n",buf);

	return 0;
}

