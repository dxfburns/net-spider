/*
 * http.h
 *
 *  Created on: Jun 3, 2014
 *      Author: root
 */

#ifndef HTTP_H_
#define HTTP_H_

#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fstream>
#include "config.h"
#include "web.h"

struct argument {
	URL url;
	int sockfd;
	argument(){}
};

#define UAGENT "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/33.0.1750.117 Safari/537.36"
#define ACCEPT "*/*"
#define CONN "keep-alive"

int GetHostByName(const string&);
int SetNoBlocking(const int&);
int ConnectWebHost(int&);
int SendRequest(int, URL&);
double Calc_Time_Sec(struct timeval st, struct timeval ed);
void* GetResponse(void*);

#endif /* HTTP_H_ */
