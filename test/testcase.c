

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <getopt.h>

#include <sys/socket.h>
#include <sys/time.h>

#include <arpa/inet.h>

#define MAX_MAS_LENGTH		512
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)


int send_msg(int connfd, char *msg, int length) {

	int res = send(connfd, msg, length, 0);
	if (res < 0) {
		perror("send");
		exit(1);
	}
	return res;
}	

// 
int recv_msg(int connfd, char *msg, int length) {

	int res = recv(connfd, msg, length, 0);
	if (res < 0) {
		perror("recv");
		exit(1);
	}
	return res;

}

void equals(char *pattern, char *result, char *casename) {

	if (strcmp(pattern, result) == 0) {
		printf("==> PASS --> %s\n", casename);
	} else {
		printf("==> FAILED --> %s, '%s' != '%s'\n", casename, pattern, result);
	}
}

// "SET Name King" "SUCCESS"
void test_case(int connfd, char *msg, char *pattern, char *casename) {

	if (!msg||!pattern||!casename) return ;


	send_msg(connfd, msg, strlen(msg));

	char result[MAX_MAS_LENGTH] = {0};
	recv_msg(connfd, result, MAX_MAS_LENGTH);

	equals(pattern, result, casename);

}

void string_testcase(int connfd) {

	test_case(connfd, "SET Name King\n", "OK\r", "SETCase");
	test_case(connfd, "GET Name\n", "King\r", "GETCase");
	test_case(connfd, "SET Name Darren\n", "OK\r", "MODCase");
	test_case(connfd, "GET Name\n", "Darren\r", "GETCase");
	test_case(connfd, "DEL Name\n", "OK\r", "DELCase");
	test_case(connfd, "GET Name\n", "ERROR : NO THIS KEY\r", "GETCase");

}

void string_testcase_10w(int connfd) { // 10w

	int count = 100000;
	int i = 0;

	while (i ++ < count) {
		string_testcase(connfd);
	}

}

void rbtree_testcase_5w_node(int connfd) {

	int count = 50000;
	int i = 0;

	for (i = 0;i < count;i ++) {

		char cmd[128] = {0};

		snprintf(cmd, 128, "RSET Name%d %d King%d\n", i, i,i);
		test_case(connfd, cmd, "OK 1\r", "SETCase");
		
	}

	for (i = 0;i < count;i ++) {
		
		char cmd[128] = {0};

		snprintf(cmd, 128, "RDEL Name%d King%d\n", i, i);
		test_case(connfd, cmd, "OK\r", "DELCase");
	}
	

}


void hash_testcase_5w_node(int connfd) {

	int count = 50000;
	int i = 0;

	for (i = 0;i < count;i ++) {

		char cmd[128] = {0};

		snprintf(cmd, 128, "HSET Nam%d King%d A\n", i, i);
		test_case(connfd, cmd, "OK 1\r", "SETCase");		
	}

	for (i = 0;i < count;i ++) {
		
		char cmd[128] = {0};

		snprintf(cmd, 128, "HDEL Nam%d King%d\n", i, i);
		test_case(connfd, cmd, "OK\r", "DELCase");
	}
	

}



int connect_tcpserver(const char *ip, unsigned short port) {

	int connfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in tcpserver_addr;
	memset(&tcpserver_addr, 0, sizeof(struct sockaddr_in));

	tcpserver_addr.sin_family = AF_INET;
	tcpserver_addr.sin_addr.s_addr = inet_addr(ip);
	tcpserver_addr.sin_port = htons(port);

	int ret = connect(connfd, (struct sockaddr*)&tcpserver_addr, sizeof(struct sockaddr_in));
	if (ret) {
		perror("connect");
		return -1;
	}

	return connfd;
}

// array: 0x01, rbtree: 0x02, hash: 0x04, skiptable: 0x08

// ./test -s 192.168.146.131 -p 8888 -m 1
int main(int argc, char *argv[]) {

	int ret = 0;

	char ip[16] = {0};
	int port = 0;
	int mode = 1;

	int opt;
	while ((opt = getopt(argc, argv, "s:p:m:?")) != -1) {

		switch (opt) {

			case 's':
				strcpy(ip, optarg);
				break;

			case 'p':
				port = atoi(optarg);
				break;

			case 'm':
				mode = atoi(optarg);
				break;

			default:
				return -1;
		
		}
		
	}

	
	int connfd = connect_tcpserver(ip, port);

	if (mode & 0x1) { // array

		struct timeval tv_begin;
		gettimeofday(&tv_begin, NULL);
		
		string_testcase_10w(connfd);

		struct timeval tv_end;
		gettimeofday(&tv_end, NULL);

		int time_used = TIME_SUB_MS(tv_end, tv_begin);
		
		printf("array testcase--> time_used: %d, qps: %d\n", time_used, 600000 * 1000 / time_used);
	}

	if (mode & 0x2) { // rbtree

		struct timeval tv_begin;
		gettimeofday(&tv_begin, NULL);
		
		//rbtree_testcase_10w(connfd);
		
		rbtree_testcase_5w_node(connfd);

		struct timeval tv_end;
		gettimeofday(&tv_end, NULL);

		int time_used = TIME_SUB_MS(tv_end, tv_begin);
		
		printf("rbtree testcase-->  time_used: %d, qps: %d\n", time_used, 100000 * 1000 / time_used);
	
	}

	if (mode & 0x4) { // hash

		struct timeval tv_begin;
		gettimeofday(&tv_begin, NULL);
		
		//hash_testcase(connfd);
		hash_testcase_5w_node(connfd);

		struct timeval tv_end;
		gettimeofday(&tv_end, NULL);

		int time_used = TIME_SUB_MS(tv_end, tv_begin);
		
		printf("hash testcase-->  time_used: %d, qps: %d\n", time_used, 200000 * 1000 / time_used);

		
	
	}


}




