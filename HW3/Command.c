#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <readline/history.h>

#include "Command.h"
#include "error.h"

typedef struct {
  char *file;
  char **argv;
  char *input;
  char *output;
} *CommandRep;

#define BIARGS CommandRep r, int *eof, Jobs jobs
#define BINAME(name) bi_##name
#define BIDEFN(name) static void BINAME(name) (BIARGS)
#define BIENTRY(name) {#name,BINAME(name)}

typedef struct {
  char *s;
  void (*f)(BIARGS);
} Builtin;

static char *owd=0;
static char *cwd=0;

static void builtin_args(CommandRep r, int n) {
  char **argv=r->argv;
  for (n++; *argv++; n--);
  if (n)
    ERROR("wrong number of arguments to builtin command"); // warn
}

BIDEFN(exit) {
  builtin_args(r,0);
  *eof=1;
}

BIDEFN(pwd) {
  builtin_args(r,0);
  if (!cwd)
    cwd=getcwd(0,0);
  printf("%s\n",cwd);
}

BIDEFN(cd) {
  builtin_args(r,1);
  char *previous=getcwd(0,0);
  if (!previous)
    ERROR("getcwd() failed");
  char *target;
  if (strcmp(r->argv[1],"-")==0) {
    if (!owd)
      ERROR("cd: OLDPWD not set");
    target=strdup(owd);
  } else {
    target=strdup(r->argv[1]);
  }
  if (chdir(target))
    ERROR("chdir() failed");
  free(owd);
  owd=previous;
  free(cwd);
  cwd=getcwd(0,0);
  if (!cwd)
    ERROR("getcwd() failed");
  free(target);
}

BIDEFN(history) {
  builtin_args(r,0);
  HIST_ENTRY **entries=history_list();
  if (!entries)
    return;
  for (int i=0; entries[i]; i++)
    printf("%s\n",entries[i]->line);
}

static int builtin(BIARGS) {
  static const Builtin builtins[]={
    BIENTRY(exit),
    BIENTRY(pwd),
    BIENTRY(cd),
    BIENTRY(history),
    {0,0}
  };
  int i;
  for (i=0; builtins[i].s; i++)
    if (!strcmp(r->file,builtins[i].s)) {
      builtins[i].f(r,eof,jobs);
      return 1;
    }
  return 0;
}

static int isBuiltin(CommandRep r) {
  return !strcmp(r->file,"exit") || !strcmp(r->file,"pwd") ||
         !strcmp(r->file,"cd") || !strcmp(r->file,"history");
}

static char **getargs(T_words words) {
  int n=0;
  T_words p=words;
  while (p) {
    p=p->words;
    n++;
  }
  char **argv=(char **)malloc(sizeof(char *)*(n+1));
  if (!argv)
    ERROR("malloc() failed");
  p=words;
  int i=0;
  while (p) {
    argv[i++]=strdup(p->word->s);
    p=p->words;
  }
  argv[i]=0;
  return argv;
}

extern Command newCommand(T_command command) {
  CommandRep r=(CommandRep)malloc(sizeof(*r));
  if (!r)
    ERROR("malloc() failed");
  r->argv=getargs(command->words);
  r->file=r->argv[0];
  r->input=command->input ? strdup(command->input) : 0;
  r->output=command->output ? strdup(command->output) : 0;
  return r;
}

static void redirect(CommandRep r) {
  if (r->input) {
    int fd=open(r->input,O_RDONLY);
    if (fd==-1)
      ERROR("open() failed");
    if (dup2(fd,STDIN_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(fd)==-1)
      ERROR("close() failed");
  }
  if (r->output) {
    int fd=open(r->output,O_WRONLY|O_CREAT|O_TRUNC,0666);
    if (fd==-1)
      ERROR("open() failed");
    if (dup2(fd,STDOUT_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(fd)==-1)
      ERROR("close() failed");
  }
}

static void restore(int input, int output) {
  if (input != -1) {
    if (dup2(input,STDIN_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(input)==-1)
      ERROR("close() failed");
  }
  if (output != -1) {
    if (dup2(output,STDOUT_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(output)==-1)
      ERROR("close() failed");
  }
}

static void child(CommandRep r, int input, int output, int unused) {
  int eof=0;
  Jobs jobs=newJobs();
  if (input!=-1) {
    if (dup2(input,STDIN_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(input)==-1)
      ERROR("close() failed");
  }
  if (output!=-1) {
    if (dup2(output,STDOUT_FILENO)==-1)
      ERROR("dup2() failed");
    if (close(output)==-1)
      ERROR("close() failed");
  }
  if (unused!=-1 && close(unused)==-1)
    ERROR("close() failed");
  redirect(r);
  if (builtin(r,&eof,jobs)) {
    fflush(stdout);
    _exit(0);
  }
  execvp(r->argv[0],r->argv);
  ERROR("execvp() failed");
  exit(0);
}

extern pid_t execCommand(Command command, Pipeline pipeline, Jobs jobs,
			 int *jobbed, int *eof, int fg,
			 int input, int output, int unused, int parentBuiltin) {
  CommandRep r=command;
  if (fg && parentBuiltin && isBuiltin(r)) {
    int savedInput=r->input ? dup(STDIN_FILENO) : -1;
    int savedOutput=r->output ? dup(STDOUT_FILENO) : -1;
    if ((r->input && savedInput==-1) ||
        (r->output && savedOutput==-1))
      ERROR("dup() failed");
    redirect(r);
    builtin(r,eof,jobs);
    fflush(stdout);
    restore(savedInput,savedOutput);
    return -1;
  }
  if (!*jobbed) {
    *jobbed=1;
    if (!fg)
      addJobs(jobs,pipeline);
  }
  int pid=fork();
  if (pid==-1)
    ERROR("fork() failed");
  if (pid==0)
    child(r,input,output,unused);
  return pid;
}

extern void freeCommand(Command command) {
  CommandRep r=command;
  char **argv=r->argv;
  while (*argv)
    free(*argv++);
  free(r->argv);
  free(r->input);
  free(r->output);
  free(r);
}

extern void freestateCommand() {
  if (cwd) free(cwd);
  if (owd) free(owd);
}
