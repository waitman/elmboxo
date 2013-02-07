#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <syslog.h>
#include <uuid.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

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

wchar_t *
strtolower(wchar_t *src)
{
	for (int i=0;i<wcslen(src);i++) {
		src[i] = towlower(src[i]);
	}
	return (src);
}

void 
rf(char *f,long fstart, long fend, long end_body) {

	const wchar_t *tz=L"-0000-0100-0200-0300-0400-0500-0600-0700-0800-0900-1000-1100-1200+0000+0100+0200+0300+0400+0500+0600+0700+0800+0900+1000+1100+1200+1300+1400+0330+0430+0530+0630+0930+1030+1130-0330-0430-0930+1245+0545";

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
	fwide(file,1);
	
	wchar_t *buffer;
	wchar_t *loc;
	long lSize;
	size_t result;
	lSize = fend-fstart;
	wchar_t bd[71]={0};
	wchar_t contentType[255]={0};
	int has_boundary=0;

	fseek(file,fstart,SEEK_SET);
	buffer = (wchar_t*) malloc (sizeof(wchar_t)*lSize*5);
	result = fread(buffer,1,lSize,file);

	//unfold lines in header
	int j;
	for (j=0;j<wcslen(buffer)-1;j++) {
		if (buffer[j]==L'\n') {
			if ((buffer[j+1]==L' ')||(buffer[j+1]==L'\t')) {
				buffer[j]=L' ';
				j++;
			}
		}
	}

	//replace tabs with spaces
	int len;
	len = wcslen(buffer);
	for (j=0;j<len;j++) {
		if (buffer[j]==L'\t') {
			buffer[j]=L' ';
		}
	}

	//strip extra whitespace
	while (wcsstr(buffer,L"  ")) {
		for (j=0;j<len-1;j++) {
			if (buffer[j]==L' ') {
				if (buffer[j+1]==L' ') {
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
			if (buffer[j]==L':') {
				prong=j;
				reset=1;
			}
		}
		//split the header line at the first ':'
		if (buffer[j]==L'\n') {

			wchar_t d[80] = {0};
 			wchar_t r[8192] = {0};

			memcpy(&d,&buffer[start],prong-start);
			d[prong-start]='\0';
			memcpy(&r,&buffer[prong+1],j-prong-1);
			r[j-prong-1]='\0';
			//remove leading space if exists
			if (r[0]==L' ') {
				memmove(&r[0],&r[1],j-prong-2);
				r[j-prong-2]='\0';
			}
			//remove $,. from d
			for (int k=0;k<wcslen(d);k++) {
				if (d[k]==L'$') d[k]=L'_';
				if (d[j]==L'.') d[k]=L'_';
			}
			reset=0;
			start=j+1;

			wchar_t ndate[26]={0};
			int dondate=0;

			//attempt to find timezone in line to parse date
			for (int k=0;k<wcslen(tz);k+=5) {
				if (dondate<1) {
				wchar_t x[6]={0};
				memcpy(&x[0],&tz[k],5);
				x[6]='\0';
				loc = wcsstr(r,x);
				if (loc) {
					//solve -0000 in id
					if (r[loc-r-1]==L' ') {
					int dtstart=(loc-r)-21;
					if (r[dtstart]==L' ') {
						ndate[0]=L'0';
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

char mbc_ndate[1024]={0};
wcstombs(mbc_ndate,ndate,wcslen(ndate));
				bson_append_string(&b,"ph",mbc_ndate);

char mbc_d[1024]={0};
wcstombs(mbc_d,d,wcslen(d));
char mbc_r[4192]={0};
wcstombs(mbc_r,r,wcslen(r));
				bson_append_string(&b,mbc_d,mbc_r);
				bson_append_finish_object(&b);
			} else {
				char ts[255]={0};
				sprintf(ts,"%i",tt++);
				bson_append_start_object(&b,ts);

char mbc_d[1024]={0};
wcstombs(mbc_d,d,wcslen(d));
char mbc_r[4192]={0};
wcstombs(mbc_r,r,wcslen(r));

				bson_append_string(&b,mbc_d,mbc_r);
				bson_append_finish_object(&b);
			}

			if (!wcscmp(L"content-type",strtolower(d))) {
				memcpy(&contentType,&r,wcslen(r));
				contentType[wcslen(r)]='\0';
				wchar_t *boundary = wcsstr(r,L"boundary=");
				if (boundary) {
					int st = boundary-r+10;
					int en = wcslen(r);
					if (r[st+1]=='"') st++;
					for (int ci=st;ci<wcslen(r);ci++) {
						if ((r[ci]==L'"')||
							(r[ci]==L' ')||
							(r[ci]==L';')) {
								en=ci;
								break;
						}
					}
					memcpy(&bd,&r[st],en-st);
					bd[en-st+1]='\0';
					has_boundary=1;
				}
			}
		}
	}
	bson_append_finish_object(&b);
	lSize = end_body-fend;
	free(buffer);
        buffer = (wchar_t*) calloc (lSize*5,sizeof(wchar_t));
        result = fread(buffer,1,lSize,file);
	bson_append_start_object(&b,"body");



char *mbc_buffer = (char*) calloc(wcslen(buffer)*2,sizeof(char));
wcstombs(mbc_buffer,buffer,wcslen(buffer));

	bson_append_string(&b,"0",mbc_buffer);
	bson_append_finish_object(&b);

	bson_finish(&b);

	if( mongo_insert( &conn, "mail.mbox", &b, NULL ) != MONGO_OK ) {
		char err[512]={0};
		sprintf(err,"FAIL: (%d) %s %s [%i %i %i] %i %s %s",conn.err,conn.errstr,conn.lasterrstr,fstart,fend,end_body,has_boundary,contentType,bd);
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
			fwprintf(efp,L"%s",buffer);
			sprintf(err,"Errors [%s|%i|%i|%i] went to %s",f,fstart,fend,end_body,ef);
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
        setlocale(LC_CTYPE,"en_US.UTF-8");

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

