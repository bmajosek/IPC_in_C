#define _GNU_SOURCE
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define MAX_CLIENTS 8
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_MSG 128
#define BACKLOG 3

volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig)
{
	do_work = 0;
}

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

int make_socket(int domain, int type)
{
	int sock;
	sock = socket(domain, type, 0);
	if (sock < 0)
		ERR("socket");
	return sock;
}

int bind_tcp_socket(uint16_t port)
{
	struct sockaddr_in addr;
	int socketfd, t = 1;
	socketfd = make_socket(PF_INET, SOCK_STREAM);
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)))
		ERR("setsockopt");
	if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		ERR("bind");
	if (listen(socketfd, BACKLOG) < 0)
		ERR("listen");
	return socketfd;
}

int add_new_client(int sfd)
{
	int nfd;
	if ((nfd = TEMP_FAILURE_RETRY(accept(sfd, NULL, NULL))) < 0) {
		if (EAGAIN == errno || EWOULDBLOCK == errno)
			return -1;
		ERR("accept");
	}
	return nfd;
}
void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket port\n", name);
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));
		if (c < 0)
			return c;
		if (0 == c)
			return len;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	int c;
	size_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}
void communicate(int cfd, int hosts[])
{
	ssize_t size, sizeSend, sizeOfMsg;
	int8_t addr;
    char *isThere, Msg[MAX_MSG], smMsg;
    if ((size = bulk_read(hosts[cfd - 1], (char *)&addr, sizeof(int8_t))) < 0)
        ERR("read:a");
    addr -= '0';
    printf("receive: %d\n", addr);
    if ((sizeOfMsg = bulk_read(hosts[cfd - 1], Msg, sizeof(char)*MAX_MSG)) < 0)
        ERR("read");
    if(addr == 9) {
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if (hosts[i] >= 0  && bulk_write(hosts[i], Msg, sizeof(char)*MAX_MSG) < 0 && errno != EPIPE)
                ERR("write:");
        }
    }
    else if(addr < 9 && (sizeSend = bulk_write(hosts[addr], Msg, sizeof(char)*MAX_MSG)) < 0) {
        printf("unkown host\n");
        isThere = "Wrong add\n";
        if (bulk_write(hosts[cfd - 1], isThere, strlen(isThere)) < 0 && errno != EPIPE)
            ERR("write:");
    }
    if (sizeSend > 0 && size > 0) {
        printf("send\n");
    }
    int flags = fcntl(hosts[cfd - 1], F_GETFL, 0);
    fcntl(hosts[cfd - 1], F_SETFL, flags | O_NONBLOCK);
    if ((sizeOfMsg = read(hosts[cfd - 1], &smMsg, sizeof(char))) > 0) {
        printf("message too long\n");
    }
    fcntl(hosts[cfd - 1], F_SETFL, flags);
}

void doServer(int fdT, int hosts[])
{
    for(int i = 0; i < MAX_CLIENTS; i++) {
        hosts[i] = -1;
    }
	int fdmax, number = 0;
	fd_set base_rfds, rfds;
	sigset_t mask, oldmask;
	FD_ZERO(&base_rfds);
	FD_SET(fdT, &base_rfds);
	fdmax = fdT;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	while (do_work) {
		rfds = base_rfds;
		if (pselect(fdmax + 1, &rfds, NULL, NULL, NULL, &oldmask) > 0) {
			if (FD_ISSET(fdT, &rfds))
				hosts[number++] = add_new_client(fdT);
			if (hosts[number - 1] >= 0)
				communicate(number, hosts);
		} else {
			if (EINTR == errno)
				continue;
			ERR("pselect");
		}
	}
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
    for(int i = 0; i < number; i++)
        if (TEMP_FAILURE_RETRY(close(hosts[i])) < 0)
            ERR("close");
}
int main(int argc, char **argv)
{
	int fdT, hosts[MAX_CLIENTS];
	int new_flags;
	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	if (sethandler(sigint_handler, SIGINT))
		ERR("Seting SIGINT:");
	fdT = bind_tcp_socket(atoi(argv[1]));
	new_flags = fcntl(fdT, F_GETFL) | O_NONBLOCK;
	fcntl(fdT, F_SETFL, new_flags);
	doServer(fdT, hosts);
	if (TEMP_FAILURE_RETRY(close(fdT)) < 0)
		ERR("close");
	fprintf(stderr, "Server has terminated.\n");
	return EXIT_SUCCESS;
}