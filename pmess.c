#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <syslog.h>
#include <uuid.h>
#include <locale.h>
#include <ctype.h>
#include <iconv.h>

int gn;

int
fuzzed(void)
{
	return(gn);
}

void
dolog(const char *v) {
	openlog("pmess",LOG_PID,1);
	syslog(LOG_INFO,v);
	closelog();
}

char *
strtolower(char *src)
{
	for (int i=0;i<strlen(src);i++) {
		src[i] = tolower(src[i]);
	}
	return (src);
}

char *
alpha(char *src)
{
        for (int i=0;i<strlen(src);i++) {
		if (isalpha(src[i])) {
                	src[i] = tolower(src[i]);
		} else {
			src[i] = ' ';
		}
        }
        return (src);
}


void 
rf(char *f,long fstart, long fend, long end_body) {

	const char *tz="-0000-0100-0200-0300-0400-0500-0600-0700-0800-0900-1000-1100-1200+0000+0100+0200+0300+0400+0500+0600+0700+0800+0900+1000+1100+1200+1300+1400+0330+0430+0530+0630+0930+1030+1130-0330-0430-0930+1245+0545";

	mongo conn;

        //connect to mongodb on local. if no connect, bail
        if( mongo_client( &conn, "127.0.0.1", 27017 ) != MONGO_OK ) {
                switch( conn.err ) {
                        case MONGO_CONN_NO_SOCKET:
                                dolog("FAIL: No Connecto Socket.");
                                break; 
                        case MONGO_CONN_FAIL:
                                dolog("FAIL: No Connecto.\n");
                                break;
                }
                exit( 1 );
        }


	
	char hostname[_POSIX_HOST_NAME_MAX] = {0};

	gethostname(hostname,_POSIX_HOST_NAME_MAX);

	FILE *file = fopen(f,"r");
	
	char *buffer;
	char *loc;
	long lSize;
	size_t result;
	lSize = fend-fstart;
	char bd[71]={0};
	char contentType[255]={0};
	char charset[32]={0};
	int has_charset=0;
	int has_boundary=0;

	fseek(file,fstart,SEEK_SET);
	buffer = (char*) malloc (sizeof(char)*lSize*2);
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
	bson b;

	uuid_t *store;
	char *up;
	store = (uuid_t*)malloc(sizeof(uuid_t));
	uuidgen(store,1);
	uuid_to_string(store, &up, NULL);

	char outstr[200] = {0};
	time_t t;
	struct tm *tmp;
	t = time(NULL);
	tmp = localtime(&t);
	strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);

	bson_init(&b);

	bson_set_oid_fuzz(fuzzed);
	bson_append_new_oid(&b,"_id");
	bson_append_string(&b,"UUID",up);
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
	int tt=0;

	for (j=0;j<len;j++) {
		if (reset==0) {
			if (buffer[j]==':') {
				prong=j;
				reset=1;
			}
		}
		//split the header line at the first ':'
		if (buffer[j]=='\n') {

			char d[80] = {0};
 			char r[8192] = {0};

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

			char ndate[26]={0};
			int dondate=0;

			//attempt to find timezone in line to parse date
			for (int k=0;k<strlen(tz);k+=5) {
				if (dondate<1) {
				char x[6]={0};
				memcpy(&x[0],&tz[k],5);
				x[6]='\0';
				loc = strstr(r,x);
				if (loc) {
					//solve -0000 in id
					if (r[loc-r-1]==' ') {
					int dtstart=(loc-r)-21;
					if (r[dtstart]==' ') {
						ndate[0]='0';
						memcpy(&ndate[1],&r[dtstart+1],
							25);
						ndate[26]='\0';
						dondate=1;
					} else {
						memcpy(&ndate[0],&r[dtstart],
							26);
						ndate[26]='\0';
						dondate=1;
					}
					}
				}
				}
			}

			if (dondate>0) {
				char ts[255]={0};
				sprintf(ts,"%i",tt++);
				bson_append_start_object(&b,ts);

				bson_append_string(&b,"ph",ndate);
				bson_append_string(&b,d,r);
				bson_append_finish_object(&b);
			} else {
				char ts[255]={0};
				sprintf(ts,"%i",tt++);
				bson_append_start_object(&b,ts);
				bson_append_string(&b,d,r);
				bson_append_finish_object(&b);
			}

			if (!strcmp("content-type",strtolower(d))) {
				memcpy(&contentType,&r,strlen(r));
				contentType[strlen(r)]='\0';
				char *boundary = strstr(r,"boundary=");
				if (boundary) {
					int st = boundary-r+9;
					int en = strlen(r);
					if (r[st+1]=='"') st++;
					for (int ci=st;ci<strlen(r);ci++) {
						if ((r[ci]=='"')||
							(r[ci]==' ')||
							(r[ci]==';')) {
								en=ci;
								break;
						}
					}
					memcpy(&bd,&r[st],en-st);
					bd[en-st+1]='\0';
					has_boundary=1;
				}
                                char *cs = strstr(r,"charset=");
                                if (cs) {
                                        int st = cs-r+8;
                                        int en = strlen(r);
                                        if (r[st+1]=='"') st++;
                                        for (int ci=st;ci<strlen(r);ci++) {
                                                if ((r[ci]=='"')||
                                                        (r[ci]==' ')||
                                                        (r[ci]==';')) {
                                                                en=ci;
                                                                break;
                                                }
                                        }
                                        memcpy(&charset,&r[st],en-st);
                                        charset[en-st+1]='\0';
                                        has_charset=1;
                                }

			}
		}
	}
	bson_append_finish_object(&b);
	lSize = end_body-fend;
	free(buffer);
        buffer = (char*) calloc (lSize*2,sizeof(char));
        result = fread(buffer,1,lSize,file);
	bson_append_start_object(&b,"body");

//if it is not ascii try to convert to ascii
//temporary! 

if (has_charset) {
char chkc[200]={0};
sprintf(chkc,"%s",strtolower(charset));

if (!strstr(chkc,"ascii")) { 
	iconv_t cd = 0;
	cd = iconv_open("US-ASCII",chkc); 
	if (cd == (iconv_t) -1) {
		//cannot convert, perhaps no codepage
		char err[512]={0};
		sprintf(err,"Error: no codepage %s",charset);
		dolog(err);
	} else {
		size_t inbytes = 4;
		size_t outbytes = 4;
        	char *ures;
		inbytes=strlen(buffer);
        	ures = (char*) calloc (lSize*6,sizeof(char));
		char* ob = ures;
		const char* ib = buffer;
		outbytes = strlen(buffer);
		iconv(cd,&ib,&inbytes,&ob,&outbytes);
		bson_append_string(&b,"0",ures);
		free(ures);
	}
} else {
	//try it anyway, might fail
	bson_append_string(&b,"0",buffer);
}
} else {
	//no charset specified
        bson_append_string(&b,"0",buffer);
}
	bson_append_finish_object(&b);


//keywords - THINK about images/attachments
//strip non-alpha -- THINK about hyphenated words.
char *clean;
clean = (char*) calloc (lSize*2,sizeof(char));
sprintf(clean,"%s",alpha(buffer));
        //strip extra whitespace
        while (strstr(clean,"  ")) {
                for (j=0;j<len-1;j++) {
                        if (clean[j]==' ') {
                                if (clean[j+1]==' ') {
                                        memmove(&clean[j],&clean[j+1],
                                                (len-j-1));
                                        len--;
                                        clean[len]='\0';
                                }
                        }
                }
        }

printf("%s\n",clean);

	bson_finish(&b);

	if( mongo_insert( &conn, "mail.mbox", &b, NULL ) != MONGO_OK ) {
		char err[512]={0};
		sprintf(err,"FAIL: (%d) %s %s [%i %i %i] %i",conn.err,conn.errstr,conn.lasterrstr,fstart,fend,end_body,has_boundary);
		dolog(err);
		sprintf(err,"ContentType: %s",contentType);
		dolog(err);

		char ef[24] = {0};
		FILE *efp;
		int fd = -1;
		strlcpy(ef,"/var/wrecked/err.XXXXXX",sizeof ef);
		if ((fd = mkstemp(ef)) == -1 ||
			(efp = fdopen(fd,"w+")) == NULL) {
			if (fd!=-1) {
				unlink(ef);
				close(fd);
			}
		} else {
			fprintf(efp,"%s",buffer);
			sprintf(err,"CMD [%s|%i|%i|%i]",f,fstart,fend,end_body);
	
			dolog(err);
			sprintf(err,"Errors: %s",ef);
			dolog(err);
			close(fd);
		}

	}

	bson_destroy( &b );
	mongo_destroy( &conn );
	
	fclose(file);
}

int
main (int argc, char **argv)
{
        setlocale(LC_ALL,"");
        setlocale(LC_CTYPE,"en_US.US-ASCII");

	//printf("file: %s start: %s end: %s\n",argv[1],argv[2],argv[3]);
	/*
	char mess[255] = {0};
	sprintf(mess,"file: %s start: %s end: %s",argv[1],argv[2],argv[3]);
	dolog(mess);
	*/
	gn=atoi(argv[2]);
	rf (argv[1],atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));
	exit(EXIT_SUCCESS);
}

