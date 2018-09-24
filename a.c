#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>


typedef struct {
	long int mtype;
	int data;	
} msg_t;
int msgqueue_id = 0;

void* key_run(void* data)
{
	msg_t	msg;
	msg.mtype = 1;
	msg.data = 10;
	
	while (1) {
		msg.data++;
		printf("1. \r\n");
		msgsnd(msgqueue_id, (void*)&msg, sizeof(msg_t) - sizeof(long int), 0);
		sleep(1);
	}
}

void alarm_func(int data)
{
	static msg_t	msg;
	msg.mtype = 1;
	msg.data = 100;
	int ret = 0;
	
	msg.data++;
	ret = msgsnd(msgqueue_id, (void*)&msg, sizeof(msg_t) - sizeof(long int), 0);
	if (ret != 0) {
		perror("msgsnd failed \r\n");
	}
	alarm(2);
}

void main(int argc, char* argv[])
{
	pthread_t	pthread_handle;
	msg_t	msg;
	
	msgqueue_id = msgget('K', 0666 | IPC_CREAT);
	pthread_create(&pthread_handle, NULL, key_run, NULL);

	signal(SIGALRM, alarm_func);
	alarm(3);
	
	while (1)
    {
        if (msgrcv(msgqueue_id, (void *)&msg, sizeof(msg_t) - sizeof(long int), 0, 0) == -1)
        {
        	if (errno == EINTR)
				continue;
            fprintf(stderr, "msgrcv failed width erro: %d", errno);
        }
 
        printf("You wrote: %d\n", msg.data);
 
    }
	return;
}
