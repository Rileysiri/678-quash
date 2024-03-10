#include "deque.h"

IMPLEMENT_DEQUE_STRUCT(process_queue, pid_t);
IMPLEMENT_DEQUE(process_queue, pid_t);

typedef struct Job{
	int job_id;
	process_queue pid_queue;
	pid_t pid;
	char* cmd;
} Job;

IMPLEMENT_DEQUE_STRUCT(job_queue, struct Job);
IMPLEMENT_DEQUE(job_queue, struct Job);




