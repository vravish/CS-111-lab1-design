// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> 

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int compare(char** arr1, char** arr2) //assume arr1 is bigger than arr2
{
	int i=0, j;
	for (; arr1[i]; i++)
		for (j=0; arr2[j]; j++)
			if (strcmp(arr1[i], arr2[j]) == 0)
				return 1;
	return 0;
}

int
main (int argc, char **argv)
{
  int opt;
  int command_number = 1;
  int print_tree = 0;
  int time_travel = 0;
  program_name = argv[0];

  for (;;)
    switch (getopt (argc, argv, "pt"))
      {
      case 'p': print_tree = 1; break;
      case 't': time_travel = 1; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument. Not for the design project!!
 // if (optind != argc - 1)
 //   usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t stream =
    make_command_stream (get_next_byte, script_stream, argc, argv);

  command_t last_command = NULL;
  command_t command;
	if (!time_travel)
		while ((command = read_command_stream (stream)))
			{
				if (print_tree)
				{
					printf ("# %d\n", command_number++);
					print_command (command);
				}
				else
				{
					last_command = command;
					execute_command (command, time_travel);
				}
			}
	else
	{
		//get commands as array
		int len = 0;
		command_stream_t* array = streamArrayFromList(stream, &len);
		
		//make dependency graph as adjacency matrix
		int** graph= malloc(len * sizeof(int*)); //like R
		int* wait = calloc(len, sizeof(int));	//like W
		int* need = calloc(len, sizeof(int));
		int waitSum = 0;
		int i = 0, j,k;
		for (; i < len; i++) graph[i] = calloc(len, sizeof(int));
		
		for (i = 0; i < len; i++)
			for (j = 0; j < i; j++)
			{
				//row may depend on col; array[i] may depend on array[j]
				//	if array[i] has an input or output file that is an input or output of array[j]
				//all arguments except options are input files, and options start with minus
				//set graph[i][j] to 1 if dependent, else don't change
				//and increment wait[i] if dependent
				char** isInputs  = getInputs_pub(array[i]);
				char** isOutputs = getOutputs_pub(array[i]);
				char** jsInputs  = getInputs_pub(array[j]);
				char** jsOutputs = getOutputs_pub(array[j]);
				
				if (compare(isInputs, jsOutputs) || compare(isOutputs, jsOutputs) || compare(isInputs, jsInputs) || compare(isOutputs, jsInputs))
				{
					graph[i][j] = 1;
					wait[i]++;
					need[j]++;
					waitSum++;
				}
			}
		
		//then evaluate (resolve) dependency graph
		do
			for (i = 0; i < len; i++)
			{
				if (wait[i] != 0) continue; //don't start it if it's waiting on someone or if it's done
				int pid = fork();
				if (pid == 0)
				{
					execute_command(getCmd(array[i]),time_travel);
					wait[i] = -1;
					exit(0);
				}
				else if (pid > 0)
				{
					//when task i is done, decrement waitSum and wait[j] for tasks depending on i; if wait[j] now equals 0, start array[j] in new process
					wait[i] = -1;
					if (!need[i]) continue; //if noone needs me, continue;
					int status; //if people need me, then wait for me to finish then signal everyone
					waitpid(pid, &status, -2);
					usleep(10000);
					for (j = i+1; j < len; j++)
					{
						if (graph[j][i])
						{
							wait[j]--;
							waitSum--;
						}
						if (!wait[j])
						{
							int pid2 = fork();
							if (pid2 == 0)
							{
								execute_command(getCmd(array[j]),time_travel);
								wait[j] = -1;
								exit(0);
							}
							else if (pid2 > 0)
							{
								wait[j] = -1;
								if (!need[j]) continue;
								int status2;
								waitpid(pid2, &status2, -2);
								usleep(10000);
								for (k = j+1; k < len; k++)
									if (graph[k][j])
									{
										wait[k]--;
										waitSum--;
									}
							}
							else
								error(1,0,"cannot create child process in main!");
						}
					}
				}
				else
					error(1,0,"cannot create child process in main!");
			}
		while (waitSum);
	}

  return print_tree || !last_command ? 0 : command_status (last_command);
}
