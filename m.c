#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid.h>

#define MAX_MESSAGES 250000

void 
rf(char *f) {

	long int markers[MAX_MESSAGES];
	long int headers[MAX_MESSAGES];

	int header=0;
	int msg_number=0;
	int ln=0;

	FILE *file = fopen(f,"r");
	if (file!=NULL)
	{
		char line[256];
		while (fgets(line,sizeof line, file)!=NULL)
		{

			ln++;
			char *loc = strstr(line,"From ");
			if (loc)
			{
				if ((loc-line)==0)
				{
					markers[msg_number]=ftell(file);
					msg_number++;
					header=1;
					
/*

uuid_t *store;
char *p;
store = (uuid_t*)malloc(sizeof(uuid_t));
uuidgen(store,1);
uuid_to_string(store, &p, NULL);
*/
//printf("MsgNo: %d, %s\n",msg_number,p);


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

		}
		fclose(file);
	/*
		while (msg_number > 0) {
			msg_number--;
			printf("%i \t\t %i %i\n",msg_number,
				markers[msg_number],headers[msg_number+1]);
		}
*/
	} else {
		perror(f);
	}

	file = fopen(f,"r");
	char *buffer;
	char *loc;
	long lSize;
	size_t result;
	lSize = headers[11]-markers[10];

	fseek(file,markers[10],SEEK_SET);
	buffer = (char*) malloc (sizeof(char)*lSize)+1;
	result = fread(buffer,1,lSize,file);

	//unfold lines in header
	int j;
	for (j=0;j<strlen(buffer)-1;j++) {
		if (buffer[j]=='\n') {
			if ((buffer[j+1]==' ')||(buffer[j+1]=='\t')) {
				buffer[j]=' ';
				j++;
			}
		}
	}

	//replace tabs with spaces
	int len;
	len = strlen(buffer);
	for (j=0;j<len;j++) {
		if (buffer[j]=='\t') {
			buffer[j]=' ';
		}
	}

	//strip extra whitespace
	while (strstr(buffer,"  ")) {
		for (j=0;j<len-1;j++) {
			if (buffer[j]==' ') {
				if (buffer[j+1]==' ') {
					memmove(&buffer[j],&buffer[j+1],
						(len-j-1));
					printf("%i\n",len);
					len--;
					buffer[len]='\0';
				}
			}
		}
	}

	printf("%s\n{END}\n",buffer);
	fclose(file);
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
