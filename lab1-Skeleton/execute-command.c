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

#include <time.h>
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
  //error (0, 0, "warning: profiling not yet implemented");
  return open(name, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void
finish_profiling (int fd)
{
  close(fd);
}

int
command_status (command_t c)
{
  return c->status;
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

int
execute_command (command_t c, int profiling)
{
  int pipefd[2];
    pid_t child_pid;
    int fds[4] = {-1, -3, -3, -7};
    int exit_status;
    char** the_word;

    switch(c->type)
    {
        case SIMPLE_COMMAND: {
            struct timespec time_ended, start, end;
	    struct rusage usage;
	    int num_chars = 0;
	    double end_time, real_time, user_time, system_time;
	    char str[1024];
	    clock_gettime(CLOCK_MONOTONIC, &start);
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
		clock_gettime(CLOCK_MONOTONIC, &end);
		clock_gettime(CLOCK_REALTIME, &time_ended);
		getrusage(RUSAGE_CHILDREN, &usage);
		c->status = WEXITSTATUS(exit_status);
	      if(profiling >= 0)
	      {
		end_time = time_ended.tv_sec + time_ended.tv_nsec / 1000000000.0;
		real_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
		system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
		num_chars += snprintf(str, 1024, "%f %f %f %f", end_time, real_time, user_time, system_time);
		if(c->status)
		{
		    num_chars += snprintf(str + num_chars, 1024 - num_chars, " [%d]", child_pid);
		}
		else
		{
		    char* newline;
		    the_word = c->u.word;
		    while(the_word[0] != NULL && num_chars < 1023)
		    {
		      while((newline = strchr(the_word[0], '\n')))
			 *newline = ' ';
		      num_chars += snprintf(str + num_chars, 1024 - num_chars, " %s", the_word[0]);
		      the_word++;
		    }
		}
		str[num_chars++] = '\n';
		write(profiling, (void*) str, num_chars);
	      }
            }
            else
            {
                fprintf(stderr, "Failed to fork!\n");
                exit(1);
            }
            break;
	}
        case SEQUENCE_COMMAND:
            execute_command(c->u.command[0], profiling);
            c->status = execute_command(c->u.command[1], profiling);
            break;
        case SUBSHELL_COMMAND:
            set_redirect(c, fds);
            c->status = execute_command(c->u.command[0], profiling);
            reset_redirect(fds);
            break;
        case PIPE_COMMAND:
	{
            if(pipe(pipefd) == -1)
            {
                fprintf(stderr, "Failed to make a pipe!\n");
                exit(1);
            }
            struct timespec time_ended, start, end;
	    struct rusage usage;
	    int num_chars = 0;
	    double end_time, real_time, user_time, system_time;
	    char str[1024];
	    clock_gettime(CLOCK_MONOTONIC, &start);
            child_pid = fork();
            if(child_pid == 0)
            {
                dup2(pipefd[1], 1);
                close(pipefd[0]);
                execute_command(c->u.command[0], profiling);
                close(pipefd[1]);
                close(1);
		/*		
		clock_gettime(CLOCK_MONOTONIC, &end);
		clock_gettime(CLOCK_REALTIME, &time_ended);
		getrusage(RUSAGE_SELF, &usage);
	        if(profiling >= 0)
	        {
		    end_time = time_ended.tv_sec + time_ended.tv_nsec / 1000000000.0;
		    real_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		    user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
		    system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
		    num_chars += snprintf(str, 1024, "%f %f %f %f [%d]\n", end_time, real_time, user_time, system_time, getpid());		    
		    write(profiling, (void*) str, num_chars);
	        }
		*/
                exit(WEXITSTATUS(exit_status));
            }
            else if(child_pid > 0)
            {
                fds[0] = dup(0);
                close(pipefd[1]);
                dup2(pipefd[0], 0);
		//                c->status = execute_command(c->u.command[1], profiling);
		waitpid(child_pid, &exit_status, 0);
				
		clock_gettime(CLOCK_MONOTONIC, &end);
		clock_gettime(CLOCK_REALTIME, &time_ended);
		getrusage(RUSAGE_CHILDREN, &usage);
	        if(profiling >= 0)
	        {
		    end_time = time_ended.tv_sec + time_ended.tv_nsec / 1000000000.0;
		    real_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
		    user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
		    system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
		    num_chars += snprintf(str, 1024, "%f %f %f %f [%d]\n", end_time, real_time, user_time, system_time, child_pid);		    
		    write(profiling, (void*) str, num_chars);
   	        }
		c->status = execute_command(c->u.command[1], profiling);
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
	}
        case IF_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            if(!execute_command(c->u.command[0], profiling))
                c->status = execute_command(c->u.command[1], profiling);
            else if(c->u.command[2] != NULL)
                c->status = execute_command(c->u.command[2], profiling);
            reset_redirect(fds);
            break;
        case WHILE_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            while(!execute_command(c->u.command[0], profiling))
                c->status = execute_command(c->u.command[1], profiling);
            reset_redirect(fds);
            break;
        case UNTIL_COMMAND:
            c->status = 0;
            set_redirect(c, fds);
            while(execute_command(c->u.command[0], profiling))
                c->status = execute_command(c->u.command[1], profiling);
            reset_redirect(fds);
            break;
        default:
            break;
    }
    return c->status;
}
