/*
 * web.cpp
 *
 *  Created on: Jun 3, 2014
 *      Author: root
 */
#include "web.h"

extern set<unsigned int> Set;
extern URL url;
extern queue<URL> que;
extern string keyword;
//extern pthread_mutex_t que_lock;
//extern pthread_mutex_t set_lock;
const unsigned int MOD = 0x7fffffff;

unsigned int Hash(const char* s) {
	unsigned int h = 0, g;

	while (*s) {
		h = (h << 4) + *s++;
		g = h & 0xf0000000;
		if (g) {
			h ^= g >> 24;
		}
		h &= ~g;
	}

	return h % MOD;
}

int SetUrl(URL& url, string& str) {
	string src(str);

	if (src.length() == 0 || src.find('#') != string::npos) {
		return -1;
	}

	int len = src.length();

	while (src[len - 1] == '/') {
		src[--len] = '\0';
	}

	if (src.compare(0, 7, "http://") == 0) {
		src = src.assign(src, 7, len);
	}

	unsigned int p = src.find_first_of(':');
	if (p != string::npos) {
		url.SetHost(string(src, 0, p));
		int c = p + 1, port = 0, len_src = src.length();
		while (std::isdigit(src[c]) && c < len_src) {
			port = port * 10 + src[c++] - '0';
		}
		url.SetPort(port);
		src = string(src, c, src.length());
	} else {
		p = src.find_first_of('/');
		if (p != string::npos) {
			url.SetHost(string(src, 0, p));
			src = string(src, p, src.length());
		} else {
			url.SetHost(src);
		}
		url.SetPort(80);
	}

	p = src.find_first_of('/');
	if (p != string::npos) {
		url.SetFile(src);
	} else {
		url.SetFile("/");
	}

	return 1;
}

void ToLower(string& str) {
	for (string::size_type i = 0; i < str.length(); ++i) {
		if (str[i] >= 'A' && str[i] <= 'Z') {
			str[i] = str[i] + ('a' - 'A');
		}
	}
}

#include <boost/thread.hpp>
string GetValue(string& html, const string key) {
	string::size_type pos = html.find(key);
	if (pos == string::npos)
		return "";

	pos += key.size();
	char c = html[pos];
	string value;
	while (c != '"' && pos < html.size()) {
		value += c;
		c = html[++pos];
	}

	return value;
}

string GetKeyword(string& src) {
	string keys[] = { "id=\"kw\"", "id=\"word\"" };
	unsigned len = sizeof(keys) / sizeof(keys[0]);
	for (size_t i = 0; i < len; ++i) {
		string key(keys[i]);
		string::size_type pos = src.find(key);
		if (pos == string::npos)
			continue;

		unsigned begin(pos), end(pos);
		char c(src[begin]);
		while (c != '<' && begin > 0) {
			c = src[--begin];
		}

		c = src[end];
		while (c != '/' && end < src.size()) {
			c = src[++end];
		}

		string sub_html = src.substr(begin, end - begin);
		return GetValue(sub_html, "value=\"");
	}

	return "";
}

void Analyse(string& src) {
	unsigned int p = 0, p1, p2;
	int flag;
	URL url;
	string str;

	while ((p = src.find("href")) != string::npos) {
		int pp = p + 4;
		while (src[pp] == ' ') {
			pp++;
		}
		if (src[pp] != '=') {
			++p;
			continue;
		} else {
			p = pp;
			p1 = pp + 1;
		}

		while (src[p1] == ' ') {
			++p1;
		}
		if (src[p1] == '\"') {
			flag = 0;
		} else if (src[p1] == '\'') {
			flag = 1;
		} else {
			flag = 3;
		}

		switch (flag) {
		case 0:
			p2 = src.find_first_of('\"', ++p1);
			break;
		case 1:
			p2 = src.find_first_of('\'', ++p1);
			break;
		case 3:
			p2 = p1;
			while (src[p2] != ' ' && src[p2] != '>') {
				++p2;
			}
		}

		str = string(src, p1, p2 - p1);
		if (str.find(keyword) == string::npos) {
			src = src.substr(p2 + 1, src.length() - p2);
			continue;
		} else {
			if (str.find("http://") == string::npos) {
				int pos = 1;
				while (str[pos] == '/') {
					++pos;
				}
				str = string(str, pos - 1);
				str.insert(0, "http://" + url.GetHost());
			} else if (str.find(url.GetHost()) == string::npos) {
				continue;
			}

			if (SetUrl(url, str) < 0) {
				continue;
			}
		}

#ifdef DEBUG
		cout<<p<<" " <<p1<<" " <<p2<<" " << url.GetFile() << endl;
#endif

		unsigned int hash_val = Hash(url.GetFile().c_str());
		char tmp[31];

		sprintf(tmp, "%010u", hash_val);

		if (Set.find(hash_val) == Set.end()) {
			boost::mutex mu_set;
			mu_set.lock();
			//pthread_mutex_lock(&set_lock);
			Set.insert(hash_val);
			//pthread_mutex_unlock(&set_lock);
			mu_set.unlock();

			url.SetFname(string(tmp) + ".html");

			boost::mutex mu_que;
			mu_que.lock();
			//pthread_mutex_lock(&que_lock);
			que.push(url);
			//pthread_mutex_unlock(&que_lock);
			mu_que.unlock();
		}
		src = src.substr(p2 + 1, src.length() - p2);
		//src.insert(p1 - 1, "\"" + string(tmp) + "html\" ");
	}
}
