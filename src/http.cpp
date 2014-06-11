/*
 * http.cpp
 *
 *  Created on: Jun 3, 2014
 *      Author: root
 */
#include "config.h"
#include "http.h"

extern set<unsigned int> Set;
extern URL url;
extern queue<URL> que;
extern struct epoll_event events[31];
extern struct timeval t_st, t_ed;
extern int epfd;
extern int cnt;
extern int sum_byte;
extern int pending;
//extern int MAX_URL;
//extern int TIMEOUT;
extern bool is_first_url;
extern double time_used;
extern pthread_mutex_t que_lock;
extern pthread_mutex_t conn_lock;

struct hostent* Host;

//string START_URL = "http://baike.baidu.com/view/1177115.htm";
//string KEYWORD = "1177115"; // "yuanmeng001";
string START_URL = "http://blog.csdn.net/yuanmeng001";
string KEYWORD = "index";
int MAX_URL = INF;
int TIMEOUT = 20;

int GetHostByName(const string& hname) {
	Host = gethostbyname(hname.c_str());
	if (Host == 0) {
		return -1;
	}
	return 1;
}

int SetNoBlocking(const int& sockfd) {
	int opts = fcntl(sockfd, F_GETFL);
	if (opts < 0) {
		return -1;
	}

	opts |= O_NONBLOCK;
	if (fcntl(sockfd, F_SETFL, opts) < 0) {
		return -1;
	}

	return 1;
}

int ConnectWeb(int& sockfd) {
	struct sockaddr_in server_addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		return -1;
	}

#ifdef DEBUG
	puts("Create socket OK");
#endif

	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	server_addr.sin_addr = *((struct in_addr*) Host->h_addr);
	//server_addr.sin_addr.s_addr = inet_addr("baike.baidu.com");//htonl(INADDR_ANY);

	int ret = connect(sockfd, (struct sockaddr*) (&server_addr), sizeof(server_addr));
	if (ret < 0) {
		return -1;
	}

#ifdef DEBUG
	puts("Connect OK");
#endif

	pthread_mutex_lock(&conn_lock);
	++pending;
	pthread_mutex_unlock(&conn_lock);

	return 1;
}

int SendRequest(int sockfd, URL& url_t) {
	string request;
	string Uagent = UAGENT, Conn = CONN, Accept = ACCEPT;
	request = "GET /" + url_t.GetFile() + " HTTP/1.1\r\nHost: " + url_t.GetHost() + "\r\nUser-Agent: " + Uagent + "\r\nAccept: " + Accept + "\r\nConnection: "
			+ Conn + "\r\n\r\n";
	int d, total = request.length(), send = 0;
	while (send < total) {
		d = write(sockfd, request.c_str() + send, total - send);
		if (d < 0) {
			return -1;
		}
		send += d;
	}
#ifdef DEBUG
	puts("Write in socket OK");
#endif

	return 1;
}

/* Calc_Time_Sec()
 * this function used to calculate the diffrent time between
 * two time. the time is based on struct timeval:
 *	struct timeval
 *	{
 *		__time_t tv_sec;        // Seconds.
 *		__suseconds_t tv_usec;    // Microseconds.
 *	};
 */
double Calc_Time_Sec(struct timeval st, struct timeval ed) {
	double sec = ed.tv_sec - st.tv_sec;
	double usec = ed.tv_usec - st.tv_usec;

	return sec + usec / 1000000;
}

void* GetResponse(void* argument) {
	struct argument* ptr = (struct argument*) argument;
	struct stat buf;
	int timeout, flen, sockfd = ptr->sockfd;
	char ch[2], buffer[8192], head_buffer[2048], tmp[MAXLEN];
	URL url_t = ptr->url;

	//*tmp = 0;
	//clear buffer
	bzero(ch, 2);

	int j = 0, n;
//	while (true) {
//		n = read(sockfd, ch, 1);
//		if (n < 0) {
//			if (errno == EAGAIN) {
//				sleep(1);
//				continue;
//			}
//		}
//		if (*ch == '>') {
//			head_buffer[j++] = *ch;
//			break;
//		}
//	}
//	head_buffer[j++] = 0;

	int need = sizeof(buffer);
	timeout = 0;
	memset(tmp, '\0', MAXLEN);

	n = read(sockfd, buffer, need);
	if (!n)
		return NULL;

	string head_str(buffer);
	string::size_type pos = head_str.find("<html");
	string top = head_str.substr(pos, head_str.length());
	sum_byte += (head_str.length() - pos);
	strncat(tmp, top.c_str(), head_str.length() - pos);

	while (true) {
		n = read(sockfd, buffer, need);

		// we think that when read() return 0, the Web Page has fetched over
		if (!n)
			break;
		if (n < 0) {
			if (errno == EAGAIN) {
				++timeout;

				if (timeout > 10)
					break;
				sleep(1);
				continue;
			} else {
				perror("read");
				return NULL;
			}
		} else {
			sum_byte += n;
			timeout = 0;
			strncat(tmp, buffer, n);
		}
	}
	close(sockfd);

	string HtmFile(tmp);
	cout << "Page Content: " << endl;
	cout << HtmFile << endl;
	Analyse(HtmFile);
	flen = HtmFile.length();

	int dir = chdir("Pages");
	if (!dir) {
		perror("Pages does not exist");
		return NULL;
	}

	int fd = open(url_t.GetFname().c_str(), O_CREAT | O_EXCL | O_RDWR, 00770);
	/* check whether needs re-fetch */
	if (fd < 0) {
		if (errno == EEXIST) {
			stat(url_t.GetFname().c_str(), &buf);
			int len = buf.st_size;
			if (len >= flen) {
				goto NEXT;
			} else {
				fd = open(url_t.GetFname().c_str(), O_RDWR | O_TRUNC, 00770);
				if (fd < 0) {
					perror("file open error");
					goto NEXT;
				}
			}
		} else {
			perror("File open error");
			goto NEXT;
		}
	}
	write(fd, HtmFile.c_str(), HtmFile.length());

	NEXT: close(fd);
	pthread_mutex_lock(&conn_lock);
	--pending;
	++cnt;
	gettimeofday(&t_ed, NULL);
	time_used = Calc_Time_Sec(t_st, t_ed);
	printf("S:%-6.2fKB  I:%-5dP:%-5dC:%-5d", flen / 1024.0, que.size(), pending, cnt);
	printf("[re-]fetch:[%s]->[%s]\n", url_t.GetFile().c_str(), url_t.GetFname().c_str());
	pthread_mutex_unlock(&conn_lock);

	// judge how many new url in wait-queue can pop()
	if (pending <= 30) {
		n = 5;
	} else {
		n = 1;
	}

	for (int i = 0; i < n; ++i) {
		if (que.empty()) {
			if (is_first_url) {
				is_first_url = false;
				sleep(1);
				continue;
			} else {
				break;
			}
		}

		pthread_mutex_lock(&que_lock);
		URL m_url = que.front();
		que.pop();
		pthread_mutex_unlock(&que_lock);

		int sock_fd;
		timeout = 0;

		while (ConnectWeb(sock_fd) < 0 && timeout < 10) {
			++timeout;
		}
		if (timeout >= 10) {
			perror("Create socket");
			return NULL;
		}

		if (SetNoBlocking(sock_fd) < 0) {
			perror("setnoblock");
			return NULL;
		}

		if (SendRequest(sock_fd, url_t) < 0) {
			perror("send request");
			return NULL;
		}

		struct argument* arg = new struct argument;
		struct epoll_event ev;
		arg->url = url_t;
		arg->sockfd = sock_fd;
		ev.data.ptr = arg;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

		epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
	}

	pthread_exit(NULL);
}

