// UCLA CS 111 Lab 1 command execution

// Copyright 2012-2014 Paul Eggert.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "command.h"
#include "command-internals.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <error.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
prepare_profiling (char const *name)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  error (0, 0, "warning: profiling not yet implemented");
  return -1;
}

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, int profiling)
{
  /* FIXME: Replace this with your implementation, like 'prepare_profiling'.  */
  do_command(c);
  //error (1, 0, "command execution not yet implemented");
}

void set_redirect(command_t c, int descriptors[4])
{
    if(c->input != NULL)
    {
        descriptors[2] = open(c->input, O_RDONLY);
        if(descriptors[2] < 0)
        {
            fprintf(stderr, "%s: Error opening file!", c->input);
            exit(1);
        }
        descriptors[0] = dup(0);
        dup2(descriptors[2], 0);
    }
    if(c->output != NULL)
    {
        descriptors[3] = open(c->output, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(descriptors[3] < 0)
        {
            fprintf(stderr, "%s: Error opening file!", c->output);
            exit(1);
        }
        descriptors[1] = dup(1);
        dup2(descriptors[3], 1);
    }
}

void reset_redirect(int descriptors[4])
{
    if(descriptors[2] >= 0)
    {
        close(descriptors[2]);
        dup2(descriptors[0], 0);
	close(descriptors[0]);
    }
    if(descriptors[3] >= 0)
    {
        close(descriptors[3]);
        dup2(descriptors[1], 1);
	close(descriptors[1]);
    }
}

int do_command(command_t c)
{
    int pipefd[2];
    pid_t child_pid;
    int fds[4] = {-1, -3, -3, -7};
    int exit_status;
    char** the_word;
    switch(c->type)
    {
        case SIMPLE_COMMAND:
            child_pid = fork();
            if(child_pid == 0)
            {
                set_redirect(c, fds);
		if(strcmp(c->u.word[0], "exec") == 0)
		  the_word = &(c->u.word[1]);
		else
		  the_word = c->u.word;
                execvp(the_word[0], the_word);
                fprintf(stderr, "%s: command not found\n", c->u.word[0]);
                exit(1);
		reset_redirect(fds);
            }
            else if(child_pid > 0)
            {
                waitpid(child_pid, &exit_status, 0);
		c->status = WEXITSTATUS(exit_status);
            }
            else
            {
                fprintf(stderr, "Failed to fork!\n");
                exit(1);
                //some error here
            }
            break;
        case SEQUENCE_COMMAND:
            do_command(c->u.command[0]);
            c->status = do_command(c->u.command[1]);
            break;
        case SUBSHELL_COMMAND:
            set_redirect(c, fds);
            c->status = do_command(c->u.command[0]);
            reset_redirect(fds);
            break;
        case PIPE_COMMAND:
            if(pipe(pipefd) == -1)
            {
                fprintf(stderr, "Failed to make a pipe!\n");
                exit(1);
            }
            child_pid = fork();
            if(child_pid == 0)
            {
                dup2(pipefd[1], 1);
		close(pipefd[0]);
                do_command(c->u.command[0]);
                close(pipefd[1]);
                close(1);
                exit(0);
            }
            else if(child_pid > 0)
            {
                fds[0] = dup(0);
		close(pipefd[1]);
                dup2(pipefd[0], 0);
                c->status = do_command(c->u.command[1]);
                dup2(fds[0], 0);
                close(pipefd[0]);                
                close(fds[0]);
            }
            else
            {
                fprintf(stderr, "Failed to fork!\n");
                exit(1);
            }
            break;
        case IF_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            if(!do_command(c->u.command[0]))
                c->status = do_command(c->u.command[1]);
            else if(c->u.command[2] != NULL)
                c->status = do_command(c->u.command[2]);
            reset_redirect(fds);
            break;
        case WHILE_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            while(!do_command(c->u.command[0]))
                c->status = do_command(c->u.command[1]);
            reset_redirect(fds);
            break;
        case UNTIL_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            while(do_command(c->u.command[0]))
                c->status = do_command(c->u.command[1]);
            reset_redirect(fds);
            break;
        default:
            break;
    }
    return c->status;
}

