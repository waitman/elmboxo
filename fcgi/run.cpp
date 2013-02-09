/*
run.cpp Waitman Gobble 2013-02-09
see COPYING for LICENCE Information
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <fcgiapp.h>
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
#include <vector>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#define LISTENSOCK_FILENO 0
#define LISTENSOCK_FLAGS 0

using namespace std;
using namespace boost;

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
		int size = 200;
	
//CONTENT BEFORE SIZE

		char uri[255];
		string the_page;
		strcpy(uri,FCGX_GetParam("REQUEST_URI",cgi.envp));
		syslog (LOG_INFO, "Request: %s", uri);
		vector <string> pcs;
		split(pcs,uri,is_any_of("/"));
		if (pcs.size()<1) {
			the_page = "/www/bin/static/index.html";
		} else {
			the_page = "/www/bin/static/"+pcs[pcs.size()-1]+".html";
		}
		syslog (LOG_INFO, "Static: %s", the_page.c_str());

		string content =
			get_file_contents(the_page);


//lua test
		L = lua_open();
		luaL_openlibs(L);
		luaL_dofile(L,"/www/bin/static/test.lua");
		string op = luamain();
		content += op;
		lua_close(L);

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
