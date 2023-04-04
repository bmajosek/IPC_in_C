#define _GNU_SOURCE
#include <signal.h>
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>


#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

struct quemsg {
    mqd_t queue;
    mqd_t currqueue;
};
volatile sig_atomic_t last_signal = 0;

void sethandler(void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sig_handler(int sig) {
	last_signal = sig;
}
void mq_handler(union sigval sv) {
    int ni;
    unsigned msg_prio;
	struct quemsg* tmp;
	tmp = (struct quemsg*)sv.sival_ptr;
    for (;;) {
        if(mq_receive(tmp->currqueue, (char *)&ni, sizeof(int), &msg_prio) < 1) {		
			if (errno == EAGAIN)
				break;
			else
				ERR("mq_receive");
        }
        printf("MQ: got\n");
		if (TEMP_FAILURE_RETRY(mq_send(tmp->queue, (const char *)&ni, 1, msg_prio))) ERR("mq_send");
        	printf("Send\n");
    }
}
void Notifyset(struct sigevent noti[4],mqd_t pin[4], char* name[4]) {
	struct quemsg tmp[3];
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = sizeof(int);
	noti[3].sigev_value.sival_ptr = &pin;
	noti[3].sigev_notify = SIGEV_THREAD;
	noti[3].sigev_notify_attributes = NULL;
	noti[3].sigev_notify_function = mq_handler;
	name[3] = malloc(sizeof(char)*9);
	snprintf(name[3],9,"/WYDANIE");
	mq_unlink(name[3]);
	if ((pin[3] = TEMP_FAILURE_RETRY(mq_open(name[3], O_RDWR | O_NONBLOCK | O_CREAT, 0666, &attr))) == (mqd_t)-1) ERR("mq open in");
	if (mq_notify(pin[3], &noti[3]) < 0) ERR("mq_notify");
	for(int i = 0;i<3;i++) {		
		tmp[i].currqueue = pin[i];
		tmp[i].queue = pin[3];
		noti[i].sigev_value.sival_ptr = tmp;
        noti[i].sigev_notify = SIGEV_THREAD;
        noti[i].sigev_notify_attributes = NULL;
		noti[i].sigev_notify_function = mq_handler;
		if (mq_notify(pin[i], &noti[i]) < 0) ERR("mq_notify");
	}
}
void usage() {
	fprintf(stderr, "USAGE: invalid args\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	mqd_t pin[4];
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = sizeof(int);
    struct sigevent noti[4];
	unsigned msg_prio;
    char* name[4];
	uint8_t ni;
	sigset_t mask, oldmask;
	sethandler(sig_handler, SIGALRM);
	sethandler(sig_handler, SIGINT);
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGALRM);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
    for(int i = 0;i < 3;i++) {
		name[i] = malloc(sizeof(char)*8);
        snprintf(name[i],8,"/DANIE%d",i + 1);
		mq_unlink(name[i]);
        if ((pin[i] = TEMP_FAILURE_RETRY(mq_open(name[i], O_RDWR  | O_NONBLOCK | O_CREAT, 0666, &attr))) == (mqd_t)-1) ERR("mq open in");
	}
	Notifyset(noti,pin,name);
	alarm(10);
	while (last_signal != SIGINT && last_signal != SIGALRM) sigsuspend(&oldmask);
	for(int i = 0;i < 3;i++)
		for(;;) {
			if (mq_receive(pin[i], (char *)&ni, sizeof(int), &msg_prio) < 1) {
				if (errno == EAGAIN) break;
				else ERR("mq_receive");
			}
			printf("nie realziuje sie %d\n",msg_prio);
		}
    for(int i = 0;i<4;i++) {
        mq_close(pin[i]);
	    if (mq_unlink(name[i])) ERR("mq unlink");
        free(name[i]);
    }
	return EXIT_SUCCESS;
}
