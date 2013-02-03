#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#define MAX_MESSAGES 250000

void 
rf(char *f) {

	long int markers[MAX_MESSAGES];
	long int headers[MAX_MESSAGES];

	int header=0;
	int msg_number=0;
	int ln=0;

	mongo conn;

	char hostname[_POSIX_HOST_NAME_MAX] = {0};

	gethostname(hostname,_POSIX_HOST_NAME_MAX);
	setenv("DATEMSK","/home/da3m0n8t3r/elmboxo/pt.txt",1);

	//connect to mongodb on local. if no connect, bail
	if( mongo_client( &conn, "127.0.0.1", 27017 ) != MONGO_OK ) {
		switch( conn.err ) {
			case MONGO_CONN_NO_SOCKET:
				printf( "FAIL: No Connecto Socket.\n");
				break;
			case MONGO_CONN_FAIL:
				printf( "FAIL: No Connecto.\n");
				break;
		}
		exit( 1 );
	}

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
					//printf("%i\n",len);
					len--;
					buffer[len]='\0';
				}
			}
		}
	}

	int start = 0;
	int prong = 0;
	int reset = 0;
	char d[80] = {0};
	char r[1024] = {0};
	bson b;

	char outstr[200] = {0};
	time_t t;
	struct tm *tmp;
	t = time(NULL);
	tmp = localtime(&t);
	strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);

	bson_init(&b);
	bson_append_new_oid(&b,"_id");
	bson_append_start_object(&b,"origin");
	bson_append_string(&b,"hostname",hostname);
	bson_append_string(&b,"initdate",outstr);
	bson_append_finish_object(&b);

	bson_append_start_object(&b,"greenroom");
	bson_append_string(&b,"hostname","-");
	bson_append_int(&b,"processed",0);
	bson_append_int(&b,"seen",0);
	bson_append_int(&b,"flagged",0);
	bson_append_int(&b,"slide",0);
	bson_append_finish_object(&b);

	bson_append_start_object(&b,"headers");

	for (j=0;j<len;j++) {
		if (reset==0) {
			if (buffer[j]==':') {
				prong=j;
				reset=1;
			}
		}
		//split the header line at the first ':'
		if (buffer[j]=='\n') {
			memcpy(&d,&buffer[start],prong-start);
			d[prong-start]='\0';
			memcpy(&r,&buffer[prong+1],j-prong-1);
			r[j-prong-1]='\0';
			//remove leading space if exists
			if (r[0]==' ') {
				memmove(&r[0],&r[1],j-prong-2);
				r[j-prong-2]='\0';
			}
			//remove $,. from d
			for (int k=0;k<strlen(d);k++) {
				if (d[k]=='$') d[k]='_';
				if (d[j]=='.') d[k]='_';
			}
			reset=0;
			start=j+1;

/*
			struct tm *pt;
			for (int k=0;k<strlen(r);k++) {
			pt=getdate(&r[k]);
			if (getdate_err != 0) {
				printf("%i",getdate_err);
			} else {
				strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", pt);	
				printf("%s\n%s\n\n",r,outstr);
			}
			}
*/

			bson_append_string(&b,d,r);
		}
	}
	bson_append_finish_object(&b);
	bson_finish(&b);

	if( mongo_insert( &conn, "mail.mbox", &b, NULL ) != MONGO_OK ) {
		printf( "FAIL: %d\n", conn.err );
		exit( 1 );
	}

	bson_destroy( &b );
	mongo_destroy( &conn );
	
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
