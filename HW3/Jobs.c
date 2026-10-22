#include "Jobs.h"
#include "deq.h"
#include "error.h"
#include <sys/wait.h>
#include <errno.h>

extern Jobs newJobs() {
  return deq_new();
}

extern void addJobs(Jobs jobs, Pipeline pipeline) {
  deq_tail_put(jobs,pipeline);
}

extern int sizeJobs(Jobs jobs) {
  return deq_len(jobs);
}

extern void reapJobs(Jobs jobs) {
  (void)jobs;
  int status;
  pid_t result;
  while ((result=waitpid(-1,&status,WNOHANG))>0);
  if (result==-1 && errno != ECHILD)
    ERROR("waitpid() failed");
}

extern void freeJobs(Jobs jobs) {
  while (waitpid(-1,0,0)>0);
  if (errno != ECHILD && errno != 0)
    ERROR("waitpid() failed");
  deq_del(jobs,freePipeline);
}
