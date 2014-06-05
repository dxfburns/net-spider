/*
 * web.h
 *
 *  Created on: Jun 3, 2014
 *      Author: root
 */

#ifndef WEB_H_
#define WEB_H_

#include <iostream>
#include <cstdio>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <queue>
#include <set>

using namespace std;

class URL {
public:
	URL() {
		Host = "";
		Port = 0;
		File = "";
		Fname = "";
	}
	void SetHost(const string& host) {
		Host = host;
	}
	string GetHost() {
		return Host;
	}
	void SetPort(int port) {
		Port = port;
	}
	int GetPort() {
		return Port;
	}
	void SetFile(const string& file) {
		File = file;
	}
	string GetFile() {
		return File;
	}
	void SetFname(const string& fname) {
		Fname = fname;
	}
	string GetFname() {
		return Fname;
	}
	~URL() {
	}
private:
	string Host;
	int Port;
	string File;
	string Fname;
};

unsigned int hash(const char*);
int SetUrl(URL&, string&);
void ToLower(string&);
void Analyse(string&);

#endif /* WEB_H_ */
