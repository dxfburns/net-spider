/*
 * config.h
 *
 *  Created on: Jun 3, 2014
 *      Author: root
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#define DEBUG
#include <stdio.h>
#include <getopt.h>
#include <string>
using namespace std;

const int INF = 0x7fffffff;
const int MAXLEN = 500000;
const char* const short_opt = "hn:u:k:t:";

const struct option long_opt[] = { { "help", 0, NULL, 'h' }, { "nurl", 1, NULL, 'n' }, { "url", 1, NULL, 'u' }, { "key", 1, NULL, 'k' }, { "timeout", 1, NULL,
		't' }, { 0, 0, 0, 0 } };

#endif /* CONFIG_H_ */
