#ifndef COMMAND_H
#define COMMAND_H

typedef void *Command;

#include "Tree.h"
#include "Jobs.h"
#include "Sequence.h"
#include <sys/types.h>

extern Command newCommand(T_command command);

extern pid_t execCommand(Command command, Pipeline pipeline, Jobs jobs,
			 int *jobbed, int *eof, int fg,
			 int input, int output, int unused, int parentBuiltin);

extern void freeCommand(Command command);
extern void freestateCommand();

#endif
