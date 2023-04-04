#include <linux/limits.h>
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))


volatile sig_atomic_t last_signal = 0;

int sethandler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void sig_handler(int sig)
{
	last_signal = sig;
}

void sigchld_handler(int sig)
{
	pid_t pid;
	for (;;) {
		pid = waitpid(0, NULL, WNOHANG);
		if (0 == pid)
			return;
		if (0 >= pid) {
			if (ECHILD == errno)
				return;
			ERR("waitpid:");
		}
	}
}
int check(char* a,char* b)
{
	if(strlen(a) != strlen(b))
		return 0;
	for(int i = 0;i<strlen(a);i++)
	{
		if(a[i] != b[i])
			return 0;
	}
	return 1;
}
void child_work(int fdr, int fdw, int m, int R)
{
	srand(getpid());
	printf("[%d] \n", getpid());
	int t;
	int was[m + 1];
	char status[16];
	memset(status, 8,16);
	int count =0;
	int czy = 0;
	for(int i = 0;i<m ;i++)
	{
		if(czy == 0)
		{
			int czyawaria = rand() % 100;
				if(czyawaria < 5)
				 break;
			while ((count = read(R, status, 16)) == 0) {}
		}
		
		t = rand() % m;
		if(was[t] == 1)
		{
			i--;
			czy = 1;
			continue;
		}
		was[t] = 1;
		czy = 0;
		if (TEMP_FAILURE_RETRY(write(fdw, &t, sizeof(int))) < 0)
			ERR("write to pipe");
		count = 0;
		while ((count = read(R, status, 16)) == 0) {}
		printf("[%d] liczba punktow\n",status[0]);
	}
}


void create_children_and_pipes(int n, int m)
{
	int fd[n][2];
	int R[n][2];
	for(int i = 0;i<n;i++)
	{
		if(pipe(fd[i]))
			ERR("pipe");
		if(pipe(R[i]))
			ERR("pipe");
	}
	for(int i = 0;i<n;i++)
	{
		switch(fork())
		{
			case 0:
				for(int j = 0;j<n;j++)
				{
					if(j != i)
					{
						if(TEMP_FAILURE_RETRY(close(fd[j][0])))
							ERR("close");
						if(TEMP_FAILURE_RETRY(close(fd[j][1])))
							ERR("close");
						if(TEMP_FAILURE_RETRY(close(R[j][0])))
							ERR("close");
						if(TEMP_FAILURE_RETRY(close(R[j][1])))
							ERR("close");
					}
				}
				if(TEMP_FAILURE_RETRY(close(R[i][1])))
					ERR("close");
				child_work(fd[i][0],fd[i][1], m, R[i][0]);
				if(TEMP_FAILURE_RETRY(close(R[i][0])))
					ERR("close");
				if(TEMP_FAILURE_RETRY(close(fd[i][0])))
					ERR("close");
				if(TEMP_FAILURE_RETRY(close(fd[i][1])))
					ERR("close");
				exit(EXIT_SUCCESS);
			case -1:
				ERR("fork");
				exit(EXIT_FAILURE);
		}
	}


	int t;
	int status = 0;
	int scores[n][m];
	int maks = 0, ind = 0;
	char tmp[16];
	int ile = 1;
	int wyn[n];
	memset(tmp,7,16);
	for(int j = 0;j<m;j++)
	{
		maks = 0;
		ile = 1;
		for(int i = 0;i < n;i++)
		{
			scores[i][j] = 0;
			status = 0;
			if(TEMP_FAILURE_RETRY(write(R[i][1], tmp, 16)) < 0)
				ERR("write");
			while(status == 0)
			{
				status = read(fd[i][0], &t, sizeof(int));
				if (status < 0 && errno == EINTR)
					continue;
				if (status < 0)
					ERR("read header from R");
			}	
			printf("got number %d from player %d\n", t, i);
			scores[i][j] = t;
			if(t == maks)
			{
				ile++;
			}
			if(t > maks)
			{
				maks = t;
				ind = i;
			}		
		}
		for(int i = 0;i<n;i++)
		{
			if(scores[i][j] == maks)
			{
				int wynik = n / ile;
				wyn[i] += wynik;
				memset(tmp,1,16);
				tmp[0] = wynik;
				if(TEMP_FAILURE_RETRY(write(R[i][1], tmp, 16)) < 0)
					ERR("write");
				printf("%d\n", wynik);
			}
			else {
				memset(tmp,0,16);
				if(TEMP_FAILURE_RETRY(write(R[i][1], tmp, 16)) < 0 )
					ERR("write");
				printf("0\n");
			}
				
		}
	}
		
	


	for(int i = 0;i<n;i++)
	{
		if(TEMP_FAILURE_RETRY(close(fd[i][0])))
			ERR("close");
		if(TEMP_FAILURE_RETRY(close(fd[i][1])))
			ERR("close");
	}
	for(int i =0;i<n;i++)
		{
			printf("%d puntklow %d\n",i,wyn[i]);
		}
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s n\n", name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int N,M;
	if (3 != argc)
		usage(argv[0]);
	N = atoi(argv[1]);
	M = atoi(argv[2]);
	if(N == 0 || M == 0)
	{
		ERR("atoi");
	}
	if(N > 5 || N < 2 || M > 10 || M < 5)
	{
		ERR("2<=N<=5, 5<=M<=10");
	}
	create_children_and_pipes(N,M);
    return EXIT_SUCCESS;
}

