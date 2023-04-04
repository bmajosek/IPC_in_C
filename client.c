#include <asm-generic/errno.h>
#define _GNU_SOURCE
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
    mqd_t* queues;
    int curr;
    int n;
};

void mq_handler(int sig, siginfo_t *info, void *p) {
	mqd_t *pin;
	uint8_t ni;
	unsigned msg_prio;
	pin = (mqd_t *)info->si_value.sival_ptr;
    static struct sigevent noti;
    noti.sigev_value.sival_ptr = &pin;
    noti.sigev_notify = SIGEV_THREAD;
    noti.sigev_notify_attributes = NULL;
	if (mq_notify(*pin, &noti ) < 0) ERR("mq_notify");
	for (;;) {
		if (mq_receive(*pin, (char *)&ni, 1, &msg_prio) < 1) {
			if (errno == EAGAIN) break;
			else ERR("mq_receive");
		}
		printf("MQ: got timeout from %d.\n", ni);
	}
}

void usage() {
	fprintf(stderr, "USAGE: invalid args\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
	mqd_t pin[4];
    srand(time(NULL));
	struct mq_attr attr;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = sizeof(int);
    char* name[4];
    int pid = getpid();
    uint8_t ni = 1;
    int which = rand()%3;
    unsigned msg_prio;
    struct   timespec tm;
    for(int i = 0;i<3;i++) {
        name[i] = malloc(sizeof(char)*8);
        snprintf(name[i],8,"/DANIE%d",i + 1);
        if ((pin[i] = TEMP_FAILURE_RETRY(mq_open(name[i], O_WRONLY | O_EXCL, 0666, &attr))) == (mqd_t)-1) ERR("mq open in");
        printf("%d\n",pin[i]);
    }
    if (TEMP_FAILURE_RETRY(mq_send(pin[which], (const char *)&ni, 1,pid))) ERR("mq_send");
    name[3] = malloc(sizeof(char)*9);
	snprintf(name[3],9,"/WYDANIE");
	if ((pin[3] = TEMP_FAILURE_RETRY(mq_open(name[3], O_EXCL | O_RDONLY, 0666, &attr))) == (mqd_t)-1) ERR("mq open in");
    printf("wylosowano %d\n",which);
    for(int i = 0;i<5;i++) {
        clock_gettime(CLOCK_REALTIME, &tm);
        tm.tv_sec += 1;
        if(mq_timedreceive(pin[3], (char *)&ni, sizeof(int), &msg_prio, &tm) == -1) {
            if(errno != ETIMEDOUT) ERR("time");
            else break;
        }
        if(msg_prio == pid) break;
    }  
    if(msg_prio != pid && errno == ETIMEDOUT) printf("IDE DO BARU MLECZNEGO\n");
    else if(msg_prio == pid) printf("MNIAM\n");
    for(int i = 0;i<4;i++) {
        free(name[i]);
        mq_close(pin[i]);
    }
	return EXIT_SUCCESS;
}
