// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t cmd, int time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
//  error (1, 0, "command execution not yet implemented");
	int status;
	pid_t child;
	int fd[2];
	switch (cmd->type)
	{
		case SIMPLE_COMMAND:
			child = fork();
			if (!child)
			{
				//handle redirects then execution
				int fd_in, fd_out;
				if (cmd->input)
				{
					if ((fd_in = open(cmd->input, O_RDONLY, 0666)) == -1)
						error(1, 0, "cannot open input file!");
					if (dup2(fd_in, 0) == -1)
						error(1, 0, "cannot do input redirect!");
				}
				if (cmd->output)
				{
					if ((fd_out = open(cmd->output, O_WRONLY | O_CREAT, 0666)) == -1)
						error(1, 0, "cannot open output file!");
					if (dup2(fd_out, 1) == -1)
						error(1, 0, "cannot do output redirect!");
				}
				execvp(cmd->u.word[0], cmd->u.word);
				error(1,0,"cannot execute command");
			}
			else if (child > 0)
			{
				waitpid(child, &status, 0);
				cmd->status = status;
			}
			else
				error(1, 0, "cannot create child process!");
			break;
		case AND_COMMAND:
			//run left child command first, on its success, run right child command
			execute_command(cmd->u.command[0], time_travel);
			//printf("and status: %d\n", cmd->u.command[0]->status);
			if (cmd->u.command[0]->status == 0)
			{
				//run second command cmd2
				execute_command(cmd->u.command[1], time_travel);
				cmd->status = cmd->u.command[1]->status;
			}
			else
				cmd->status = cmd->u.command[0]->status;
			break;
		case OR_COMMAND:
			//run left child command first, on its failure, run right child command
			execute_command(cmd->u.command[0], time_travel);
			//printf("or status: %d\n", cmd->u.command[0]->status);
			if (cmd->u.command[0]->status != 0)
			{
				//run second command cmd2
				execute_command(cmd->u.command[1], time_travel);
				cmd->status = cmd->u.command[1]->status;
			}
			else
				cmd->status = cmd->u.command[0]->status;
			break;
		case PIPE_COMMAND:
			//create 2 processes for each, redirect output of cmd1 to input of cmd2
			if (pipe(fd) == -1)
				error(1, 0, "cannot create pipe");
			child = fork();
			if (!child)
			{
				//child writes to pipe
			//	printf("before close");
			//	close(fd[0]);
			//	printf("before dup2");
				if (dup2(fd[1], STDOUT_FILENO) == -1)
					error(1, 0, "cannot redirect output");
			//	printf("executing command %s", cmd->u.command[0]->u.word[0]);
				execute_command(cmd->u.command[0], time_travel);
			//	printf("finishing command %s", cmd->u.command[0]->u.word[0]);
				close(fd[1]);
				exit(cmd->u.command[0]->status);
			}
			else if (child > 0)
			{
				//parent reads the pipe
				waitpid(child, &status, 0);
				cmd->u.command[0]->status = status;
				close(fd[1]);
				if (dup2(fd[0], STDIN_FILENO) == -1)
					error(1, 0, "cannot redirect input");
				execute_command(cmd->u.command[1], time_travel);
				close(fd[0]);
				cmd->status = cmd->u.command[1]->status;
				//printf("status: %d\n", cmd->status);
				//exit(cmd->status);
			}
			else
				error(1, 0, "could not create child process!");
			break;
		case SUBSHELL_COMMAND:
			execute_command(cmd->u.subshell_command, time_travel);
			break;
		case SEQUENCE_COMMAND:
			execute_command(cmd->u.command[0], time_travel);
			execute_command(cmd->u.command[1], time_travel);
		default:
			break;
	}
}
