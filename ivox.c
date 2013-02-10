/*
ivox.c - Copyright 2013 by Waitman Gobble
read COPYING for LICENSE Information
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BS 1024 

int
main (int argc, char *argv[])
{
	char ef[24] = {0};
	FILE *efp;
	int fd = -1;
	int n,rval,wval;
	char *bp;
	strlcpy(ef,"/tmp/ivox.XXXXXX",sizeof ef);

	fd = mkstemp(ef);
	efp = fdopen(fd,"w+");
	char *buf;
	buf=malloc(sizeof(char)*BS);

	while ((rval = read(STDIN_FILENO, buf, BS)) > 0)
	{
		n = rval;
		bp = buf;
		do {
			wval = write(fd, bp, n);
			bp += wval;
		} while (n -= wval);
	}
	close(fd);
	
	char cmd[1024]={0};
	sprintf(cmd,"/home/da3m0n8t3r/elmboxo/elmboxo \"%s\"",ef); 
	system(cmd);
	unlink(ef);
	return (1);
}
