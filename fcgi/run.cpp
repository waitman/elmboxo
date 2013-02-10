/*
run.cpp - Copyright 2013 by Waitman Gobble <waitman@waitman.net>
see COPYING for LICENSE Information
*/
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <syslog.h>
#include <fcgiapp.h>
#include <fcgio.h>
#include <fcgi_config.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <iomanip>
#include <locale>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <vector>

#define STDIN_MAX	10000000

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#define LISTENSOCK_FILENO 0
#define LISTENSOCK_FLAGS 0

using namespace std;
using namespace boost;
using namespace boost::filesystem;

lua_State *L;

string
luamain(void)
{
	lua_getglobal(L,"main");
	lua_call(L,0,1);
	string res = (string)lua_tostring(L,-1);
	lua_pop(L,1);
	return res;
}
bool 
replace(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = str.find(from);
	if(start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

void
replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // ...
	}
}

std::string
fix_html(std::string html)
{
	replaceAll(html,"<","&lt;");
	replaceAll(html,">","&gt;");
	return (html);
}

std::string
get_file_contents(string filename)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
	if (in)
	{
		ostringstream contents;
		contents << in.rdbuf();
		in.close();
		return(contents.str());
	}
	throw(errno);
}

static long
getreq (const FCGX_Request * req, char ** content)
{
	openlog("runfcgi", LOG_CONS|LOG_NDELAY, LOG_USER);
	char * cs = FCGX_GetParam("CONTENT_LENGTH",req->envp);
	unsigned long cl = STDIN_MAX;
	if (cs) {
		cl = strtol(cs,&cs,10);
		if (*cs)
		{
			cl = STDIN_MAX;
		}
		if (cl > STDIN_MAX)
		{
			cl = STDIN_MAX;
		}
	} else {
		cl = 0;
		*content = 0;
	}
	*content = new char[cl];
	cin.read(*content, cl);
	cl = cin.gcount();

	do cin.ignore(1024); while (cin.gcount() == 1024);
	
	return cl;
}


std::string
exec(const char* cmd)
{
	FILE* pipe = popen(cmd, "r");
	if (!pipe) return "ERROR";
	char buffer[128] = {0};
	std::string result = "";
	while(!feof(pipe))
	{
		if(fgets(buffer, 128, pipe) != NULL)
			result += buffer;
	}
	pclose(pipe);
	return result;
}

template<class T>
std::string nformat(T value)
{
	std::stringstream ss;
	ss.imbue(std::locale(""));
	ss << std::fixed << value;
	return ss.str();
}

int
main(int argc, char** argv)
{
	openlog("runfcgi", LOG_CONS|LOG_NDELAY, LOG_USER);
	int err = FCGX_Init();
	if (err)
	{ 
		syslog (LOG_INFO, "FCGX_Init failed: %d", err); 
		return 1;
	}
	FCGX_Request cgi;
	err = FCGX_InitRequest(&cgi, LISTENSOCK_FILENO, LISTENSOCK_FLAGS);
	if (err)
	{
		syslog(LOG_INFO, "FCGX_InitRequest failed: %d", err);
		return 2;
	}

	std::string layout = get_file_contents("/www/bin/ui/layout.html");
	while (1)
	{
		err = FCGX_Accept_r(&cgi);
		if (err)
		{
			syslog(LOG_INFO, "FCGX_Accept_r stopped: %d", err);
			break;
		}

		fcgi_streambuf cin_fcgi_streambuf(cgi.in);
		fcgi_streambuf cout_fcgi_streambuf(cgi.out);
		fcgi_streambuf cerr_fcgi_streambuf(cgi.err);
#if HAVE_IOSTREAM_WITHASSIGN_STREAMBUF
		cin  = &cin_fcgi_streambuf;
		cout = &cout_fcgi_streambuf;
		cerr = &cerr_fcgi_streambuf;
#else
		cin.rdbuf(&cin_fcgi_streambuf);
		cout.rdbuf(&cout_fcgi_streambuf);
		cerr.rdbuf(&cerr_fcgi_streambuf);
#endif
		int size = 200;
	
//CONTENT BEFORE SIZE

		char uri[255];
		string the_page,qs;
		strcpy(uri,FCGX_GetParam("REQUEST_URI",cgi.envp));

		if (strlen(uri)==1) strcpy(uri,"index.html");

		vector <string> ex;
		char * pt;
		pt=strstr(uri,"?");
		if (pt!=NULL) {
			split(ex,uri,is_any_of("?"));
			if (!ex[0].empty()) 
				strcpy(uri,ex[0].c_str());
			if (!ex[1].empty())
				qs=ex[1];
		}

		vector <string> pcs;
		split(pcs,uri,is_any_of("/"));
		if (pcs.size()<1) {
			the_page = "/www/bin/static/index.html";
		} else {
			the_page = "/www/bin/static/"+pcs[pcs.size()-1];
		}
		if (!exists(the_page)) {
			syslog (LOG_INFO, "404NF: %s", the_page.c_str());
			the_page="/www/bin/static/404.html";
		}
			

		string content = "";


		if (!find_first(the_page,".lua")) {
			content = get_file_contents(the_page);
		} else {

			char * req;
			unsigned long cl = getreq(&cgi,&req);
			L = lua_open();
			luaL_openlibs(L);

			lua_newtable( L );
			lua_pushnumber(L,1);
			lua_pushstring(L,qs.c_str());
			lua_rawset( L, -3 );
			lua_pushnumber(L,2);
			lua_pushstring(L,req);
			lua_rawset( L, -3 );
			lua_pushliteral( L, "n" );
			lua_pushnumber( L, 2 );
			lua_rawset( L, -3 );
			lua_setglobal( L, "arg" );

			luaL_dofile(L,the_page.c_str());
			string op = luamain();
			content = op;
			lua_close(L);

		}

		string html = layout;
		replace(html, "<!--Content-->", content);

//NO CONTENT

		size += strlen(html.c_str());
		char* result = (char*) alloca(size);
		strcpy(result,
			"Status: 200 OK\r\nContent-Type: text/html\r\n\r\n");
		strcat(result, html.c_str());
    		FCGX_PutStr(result, strlen(result), cgi.out);
	}
	return 0;
}
