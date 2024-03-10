/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>
#include <stdlib.h>
#include "quash.h"
#include "deque.h"
#include "jobs.c"

//Initialize queues for jobs and processes of a job
process_queue pid_queue;
job_queue job_q;


/***************************************************************************
 Global Variables
 ***************************************************************************/
int job_number = 1;
//booted boolean is used to check if a job queue has already been created from run_script. If not, it will be created
bool booted = false;
//create a 2d array to support use of 2 piped per command
static int pipes[2][2];

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  //Get the current working directory. This will fix the prompt path.
  char *buf = malloc(1024);
  getcwd(buf, 1024);

  // Change this to true if necessary
  *should_free = true;

  return buf;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  //Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  const char *env;
  env = getenv(env_var);
  return env;
}


// Check the status of background jobs
void check_jobs_bg_status() {
  // Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  
  //initialize a Job which will be assigned to each job in the job queue as we loop over it		
  struct Job selected_job;
  //initialize a pid which will contain the pid of the first process in a job's process queue
  pid_t front_process;

  //find number of jobs currently in the job queue in order to loop over each one
  int num_of_jobs = length_job_queue(&job_q);
  

  //loop over each job in queue
  for(int job_num = 0; job_num < num_of_jobs;job_num++){
    //select job to check progress status
    selected_job = pop_front_job_queue(&job_q);
    
    //get number of processes within selected job for looping
    int num_of_processes = length_process_queue(&selected_job.pid_queue);
    //get first process within job for use as pid
    front_process = peek_front_process_queue(&selected_job.pid_queue);
    
    //loop over each process in job process queue
    for(int process_num = 0; process_num < num_of_processes; process_num++){
      //grab front process from the queue to check status
      pid_t current_process = pop_front_process_queue(&selected_job.pid_queue);
      int status;
      
      //check status of child process. If it is still active, this will return 0, so process should be added back to job's process queue
      if(waitpid(current_process,&status,WNOHANG)==0){
        push_back_process_queue(&selected_job.pid_queue,current_process);
      }
    }
  
    //if all processes have finished and have been removed from process queue, job is finished, print job completion. Otherwise, place job back in queue to continue execution
    if(is_empty_process_queue(&selected_job.pid_queue)){
      print_job_bg_complete(selected_job.job_id, front_process, selected_job.cmd);
    }else{
      push_back_job_queue(&job_q,selected_job);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;
  execvp(exec, args);

  //if memory is not overwritten with execvp, the execution has failed
  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;
  for(int i = 0; str[i] != NULL; i++){
    printf("%s ", str[i]);
  }
  printf("\n");

  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;
  setenv(env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;
  char* new_pwd;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  //change directory and change environment variable "PWD"
  chdir(dir);
  new_pwd = getcwd(NULL, 100);
  setenv("PWD", new_pwd, 1);
  //free memory
  free(new_pwd);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd)
{
  int signal = cmd.sig;
  int job_id = cmd.job;
  /*
  //initiate job to check every job in queue
  struct Job current_job;

  //iterate over jobs in queue
  for (int job_num = 0; job_num < length_job_queue(&job_q); job_num++)
  {
    //pop job from q
    current_job = pop_front_job_queue(&job_q);
    
    //check if job_id is the id we are looking for
    if (current_job.job_id == job_id)
    {
      //if job is found, get process queue
      process_queue current_pid_q = current_job.pid_queue;
      //kill every process in queue
      while (length_process_queue(&current_pid_q) != 0)
      {
        pid_t current_pid = pop_front_process_queue(&current_pid_q);
        kill(current_pid, signal);
      }
    }
    //add jobs back to queue
    push_back_job_queue(&job_q, current_job);
  }
*/}



// Prints the current working directory to stdout
void run_pwd() {
  //Print the current working directory
  char pwd[1024];
  getcwd(pwd,sizeof(pwd));
  printf("%s\n",pwd);
  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  //Print background jobs
  //Loop through each job in job queue and print using print_job. Then add job back to queue
  int number_of_jobs = length_job_queue(&job_q);
  for(int job_num=0; job_num < number_of_jobs;job_num++){
    struct Job selected_job = pop_front_job_queue(&job_q);
    print_job(selected_job.job_id,selected_job.pid,selected_job.cmd);
    push_back_job_queue(&job_q,selected_job);
  }
  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder, int i) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  //Setup pipes, redirects, and new process
  //create pipe
  if(p_out){
    pipe(pipes[i%2]);
  }
  pid_t child_pid;
  //fork to start child process
  child_pid = fork();
  //add child to process queue
  push_back_process_queue(&pid_queue,child_pid);
  
  //if in child process, run these checks
  if(child_pid==0){
    //if recieving input, set standard input to read end of pipe
    if(p_in){
      dup2(pipes[(i-1)%2][0],STDIN_FILENO);
      close(pipes[(i-1)%2][0]);
    }
    //if sending output, set standard output to write end of pipe
    if(p_out){
      dup2(pipes[i%2][1],STDOUT_FILENO);
      close(pipes[i%2][1]);
    }
    //if redirecting input, set standard input to file
    if(r_in){
      FILE* file = fopen(holder.redirect_in, "r");
      dup2(fileno(file),STDIN_FILENO);
    }
    //if redirecting output, set standard output to file. if appending, call appropriate append
    if(r_out){
      if(r_app){
        FILE* file = fopen(holder.redirect_out,"a");
        dup2(fileno(file),STDOUT_FILENO);
      }else{
        FILE* file = fopen(holder.redirect_out,"w");
        dup2(fileno(file),STDOUT_FILENO);
      }
    }
    //run child command
    child_run_command(holder.cmd); // This should be done in the child branch of a fork
    exit(0);
  //run else block if in parent
  }else{
    if(p_out){
      //close pipe if it was opened
      close(pipes[(i)%2][1]);
    }
    //run parent command
    parent_run_command(holder.cmd); // This should be done in the parent branch of

  }
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  //check if job queue has already been initialized. If not, create queue and set status
  if(!booted){
    job_q = new_job_queue(1);
    booted=true;
  }
  //create a new process queue for job
  pid_queue = new_process_queue(1);
  //if there are no commands, return
  if (holders == NULL)
    return;
  //check status of all jobs in queue
  check_jobs_bg_status();
  //exit quash if exit is called
  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i],i);

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // Wait for all processes under the job to complete
    while(!is_empty_process_queue(&pid_queue)){
      pid_t current_pid = pop_front_process_queue(&pid_queue);
      int current_status;
      waitpid(current_pid,&current_status,0);
    }
    destroy_process_queue(&pid_queue);
  }
  else {
    // A background job.
    // Push the new job to the job queue
    // Create new job
    struct Job selected_job;
    selected_job.job_id = job_number;
    //increment job number
    job_number = job_number+1;
    //add process queue to job
    selected_job.pid_queue = pid_queue;
    //add associated cmd to job
    selected_job.cmd = get_command_string();
    //assign job_id based on process id
    selected_job.pid = peek_back_process_queue(&pid_queue);
    //add job to queue
    push_back_job_queue(&job_q,selected_job);
    //Once jobs are implemented, uncomment and fill the following line
    print_job_bg_start(selected_job.job_id, selected_job.pid, selected_job.cmd);
  }
}

