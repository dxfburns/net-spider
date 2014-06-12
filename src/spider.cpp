//============================================================================
// Name        : net-spider.cpp
// Author      : Xuefeng Du
// Version     :
// Copyright   : adSage
// Description : Hello World in C++, Ansi-style
//============================================================================

#include "http.h"
//#include "config.h"

/* global data definition
 */

struct epoll_event events[31];	// return events which will be dealed with.
struct timeval t_st, t_ed;		// store time value. used to calculate avg-speed.
set<unsigned int> Set;			// the 'hash-table'.
URL url;						// first url we set.
queue<URL> que;					// url wait-queue.
int cnt;						// record how many urls we have fetched.
int sum_byte;					// record how many bytes we have fetched.
int pending;					// record pending urls that wait to deal with
int epfd;						// record the epoll fd.
bool is_first_url;				// judge whether is the first url.
double time_used;               // record totally time costed.
//pthread_mutex_t que_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t set_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t conn_lock = PTHREAD_MUTEX_INITIALIZER;
string input;
string HtmFile;
string keyword;

extern string START_URL;               // = "http://hi.baidu.com/shouzhewei/home";
extern string KEYWORD; //= "shouzhewei"; // "yuanmeng001";
extern int MAX_URL;
extern int TIMEOUT;

void init() {
	cnt = 0;
	sum_byte = 0;
	pending = 0;
	is_first_url = true;
	input = START_URL;
	keyword = KEYWORD;
	time_used = 0;
}

void Usage() {
	printf("usage\n");
	printf("-h    --help    print the usage of this spider.                 default    0\n");
	printf("-n    --nurl    set the number of url you want to get.          default    INF\n");
	printf("-u    --url     set the first url you want to start fetch.      default    \"\"\n");
	printf("-k    --key     set the key word of the url to fetch web pages. default    \"\"\n");
	printf("-t    --timeout set timeout for epoll(waitque is empty).        default    20\n");
	printf("\nexample\n");
	printf("./spider -h\n");
	printf("./spider -n 1000 -u http://hi.baidu.com/xxx -k xxx\n");
	printf("./spider --url http://hi.baidu.com/xxx --key xxx\n");
	printf("./spider --url http://hi.baidu.com/xxx -t 15\n");
}

int Parse(int argc, char** argv) {
	int ret = 0;
	int c = getopt_long(argc, argv, short_opt, long_opt, NULL);
	while (c != -1) {
		switch (c) {
		case 'h':
			return -2;
		case 'n':
			MAX_URL = atoi(optarg);
			break;
		case 'u':
			START_URL = string(optarg);
			break;
		case 'k':
			KEYWORD = string(optarg);
			break;
		case 't':
			TIMEOUT = atoi(optarg);
			break;
		default:
			return -1;
		}

		++ret;
	}

	return ret;
}

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
using namespace boost;

boost::mutex b_que_lock;
boost::mutex b_set_lock;
boost::mutex b_conn_lock;

int main() {
	init();

	int ret = SetUrl(url, input);
	if (!ret) {
		puts("input url error");
		return ret;
	}

	ret = GetHostByName(url.GetHost());
	if (!ret) {
		puts("gethostbyname error\n");
		exit(1);
	}

	cout << "Start fetching url: " << url.GetHost() << url.GetFile() << endl;

	unsigned int hash_val = Hash(url.GetFile().c_str());
	char tmp[31];

	sprintf(tmp, "%010u", hash_val);
	url.SetFname(string(tmp) + ".html");
	que.push(url);

	mutex::scoped_lock mu(b_set_lock);
	//pthread_mutex_lock(&set_lock);
	Set.insert(hash_val);
	//pthread_mutex_unlock(&set_lock);
	mu.unlock();

	epfd = epoll_create(50);

	int n = (que.size() >= 30) ? 30 : que.size();
	gettimeofday(&t_st, NULL);

	int timeout;
	for (int i = 0; i < n; ++i) {
		b_que_lock.lock();
//		pthread_mutex_lock(&que_lock);
		URL url_t = que.front();
		que.pop();
//		pthread_mutex_unlock(&que_lock);
		b_que_lock.unlock();

		int sock_fd;
		timeout = 0;
		ret = ConnectWeb(sock_fd);
		while (ret < 0 && timeout < 10) {
			++timeout;
			ret = ConnectWeb(sock_fd);
		}

		if (timeout > 10) {
			perror("Create socket");
			exit(1);
		}

		ret = SetNoBlocking(sock_fd);
		if (ret < 0) {
			perror("SetNoBlocking");
			exit(1);
		}

		ret = SendRequest(sock_fd, url_t);
		if (ret < 0) {
			perror("SendRequest");
			exit(1);
		}

		struct argument* arg = new struct argument;
		struct epoll_event ev;
		arg->url = url_t;
		arg->sockfd = sock_fd;
		ev.data.ptr = arg;
		ev.events = EPOLLIN | EPOLLOUT | EPOLLET;

		ret = epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);
		if (ret < 0) {
			perror("EPOLL_CTL_ADD");
			exit(1);
		}
	}

	/* this dead loop below deal with all events.(to every event, it will create a
	 * new thread to execute).
	 */
	timeout = 0;
	while (true) {
		if (cnt > MAX_URL) {
			break;
		}

		int n = epoll_wait(epfd, events, 30, 2000);
		if (timeout >= TIMEOUT) {
			printf("wait time out for %d times\n", TIMEOUT);
			break;
		}
		if (n < 0) {
			perror("epoll_wait_error");
			break;
		} else if (n == 0 && que.empty()) {
			++timeout;
			continue;
		}

		timeout = 0;

		// create pthread for every event and call GetResponse().
		for (int i = 0; i < n; ++i) {
			struct argument* arg = (struct argument*) events[i].data.ptr;

//			pthread_attr_t pAttr;
//			pthread_t thread;
//
//			pthread_attr_init(&pAttr);
//			pthread_attr_setstacksize(&pAttr, 8 * 1024 * 1024);
//			pthread_attr_setdetachstate(&pAttr, PTHREAD_CREATE_DETACHED);
//
//			int r = pthread_create(&thread, &pAttr, GetResponse, arg);
//			if (r) {
//				perror("thread create failed");
//				continue;
//			}
//
//			pthread_attr_destroy(&pAttr);
//			GetResponse(arg);

			function<void*(void*)> func = bind(GetResponse, _1);
			func(arg);
//			boost::thread th(func, arg);

			struct epoll_event ev;
			int r = epoll_ctl(epfd, EPOLL_CTL_DEL, arg->sockfd, &ev);
			if (r == -1) {
				epoll_ctl(epfd, EPOLL_CTL_MOD, arg->sockfd, &ev);
				perror("epoll_ctl_del");
				continue;
			}

//			th.join();
		}
	}

	struct tm* p;
	time_t ti;
	time(&ti);
	p = gmtime(&ti);
	gettimeofday(&t_ed, NULL);
	time_used = Calc_Time_Sec(t_st, t_ed);

	// print summary infomations
	printf("\n                           STATISTICS\n");
	printf("----------------------------------------------------------------------\n");
	printf("fetch target:           %s\n", input.c_str());
	printf("totally urls fetched:   %d\n", cnt);
	printf("totally byte fetched:   %.2fKB\n", sum_byte / 1024.0);
	printf("totally time cost:      %.2fs\n", time_used);
	printf("average download speed: %.2fKB/s\n", sum_byte / 1024.0 / (time_used - TIMEOUT));
	printf("auther:                 xxx\n");
	printf("date:                   %04d-%02d-%02d\n", (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday);
	printf("time:                   %02d:%02d:%02d\n", (p->tm_hour + 8) % 24, p->tm_min, p->tm_sec);
	printf("----------------------------------------------------------------------\n");

	close(epfd);

	return 0;
}
