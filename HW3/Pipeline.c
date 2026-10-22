#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Pipeline.h"
#include "deq.h"
#include "error.h"

typedef struct {
  Deq processes;
  int fg;			// not "&"
} *PipelineRep;

extern Pipeline newPipeline(int fg) {
  PipelineRep r=(PipelineRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->processes=deq_new();
  r->fg=fg;
  return r;
}

extern void addPipeline(Pipeline pipeline, Command command) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_tail_put(r->processes,command);
}

extern int sizePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  return deq_len(r->processes);
}

static void execute(Pipeline pipeline, Jobs jobs, int *jobbed, int *eof) {
  PipelineRep r=(PipelineRep)pipeline;
  int previous=-1;
  pid_t *pids=malloc(sizeof(*pids)*sizePipeline(r));
  if (!pids)
    ERROR("malloc() failed");
  int count=0;
  for (int i=0; i<sizePipeline(r) && !*eof; i++) {
    int next[2]={-1,-1};
    if (i+1<sizePipeline(r) && pipe(next)==-1)
      ERROR("pipe() failed");
    pids[count++]=execCommand(deq_head_ith(r->processes,i),pipeline,jobs,
                              jobbed,eof,r->fg,previous,next[1],
                              next[0],sizePipeline(r)==1);
    if (previous!=-1 && close(previous)==-1)
      ERROR("close() failed");
    if (next[1]!=-1 && close(next[1])==-1)
      ERROR("close() failed");
    previous=next[0];
  }
  if (previous!=-1 && close(previous)==-1)
    ERROR("close() failed");
  if (r->fg)
    for (int i=0; i<count; i++)
      if (pids[i]>0 && waitpid(pids[i],0,0)==-1)
        ERROR("waitpid() failed");
  free(pids);
}

extern void execPipeline(Pipeline pipeline, Jobs jobs, int *eof) {
  PipelineRep r=(PipelineRep)pipeline;
  int jobbed=0;
  execute(pipeline,jobs,&jobbed,eof);
  if (r->fg || !jobbed)
    freePipeline(pipeline);
}

extern void freePipeline(Pipeline pipeline) {
  PipelineRep r=(PipelineRep)pipeline;
  deq_del(r->processes,freeCommand);
  free(r);
}
