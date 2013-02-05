#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <syslog.h>

#define MAX_MESSAGES 170000

void
dolog(const char *v) {
        openlog("elmboxo",LOG_PID,1);
        syslog(LOG_INFO,v);
        closelog();
}

void 
rf(char *f) {

	long int markers[MAX_MESSAGES];
	long int headers[MAX_MESSAGES];
	long int finish[MAX_MESSAGES];

	int header=0;
	int msg_number=0;
	long int last=0;
	long int this_pos=0;
	int ln=0;

	FILE *file = fopen(f,"r");
	if (file!=NULL)
	{
		char line[256]={0};
		while (fgets(line,sizeof line, file)!=NULL)
		{

			ln++;
			char *loc = strstr(line,"From ");
			if (loc)
			{
				if ((loc-line)==0)
				{
					if (msg_number>0) finish[msg_number-1]=this_pos;
					markers[msg_number]=ftell(file);
					msg_number++;
					header=1;
				}
			} else {
				if (header>0) {
					if (strlen(line)==1) {
						header=0;
						headers[msg_number]=
							ftell(file)-1;
					}
				}
			}
			this_pos = ftell(file);
		}
		last=ftell(file);
		fclose(file);
	} else {
		perror(f);
	}

	char msg[1024] = {0};
	sprintf(msg,"Found %i msgs in %s. (%i bytes)",msg_number,f,last);
	dolog(msg);
	finish[msg_number-1]=last;
	while (msg_number>0) {
		//printf("%i mess\n",msg_number);
		msg_number--;
		char cmd[1024]={0};
		sprintf(cmd,"./pmess \"%s\" %i %i %i",f,markers[msg_number],headers[msg_number+1],finish[msg_number]);
		system(cmd);
		//printf("%s\n",cmd);
	}
}

int
main (int argc, char **argv)
{
	argc--;
	*argv++;
	while (argc--)
		rf(*argv++);
	exit(EXIT_SUCCESS);
}
