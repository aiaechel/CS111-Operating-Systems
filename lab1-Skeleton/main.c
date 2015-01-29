// UCLA CS 111 Lab 1 main program

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

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "command.h"

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
  error (1, 0, "usage: %s [-p PROF-FILE | -t] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  struct timespec time_ended, start, end;
  struct rusage r_usage;
  int num_chars = 0;
  double end_time, real_time, user_time, system_time;
  char str[1024];
  int command_number = 1;
  bool print_tree = false;
  char const *profile_name = 0;
  program_name = argv[0];
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (;;)
    switch (getopt (argc, argv, "p:t"))
      {
      case 'p': profile_name = optarg; break;
      case 't': print_tree = true; break;
      default: usage (); break;
      case -1: goto options_exhausted;
      }
 options_exhausted:;

  // There must be exactly one file argument.
  if (optind != argc - 1)
    usage ();

  script_name = argv[optind];
  FILE *script_stream = fopen (script_name, "r");
  if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
  command_stream_t command_stream =
        make_command_stream (get_next_byte, script_stream);
  int profiling = -1;
  int status = 0;
  if (profile_name)
    {
      profiling = prepare_profiling (profile_name);
      if (profiling < 0)
	error (1, errno, "%s: cannot open", profile_name);
    }

  command_t last_command = NULL;
  command_t command;
  while ((command = read_command_stream (command_stream)))
  {
    if (print_tree)
	{
	  printf ("# %d\n", command_number++);
	  print_command (command);
	}
    else
	{
	  last_command = command;
	  execute_command (command, profiling);
	}
  }
  
  status = last_command ? command_status(last_command) : 0;
  free_stream(command_stream);
  fclose(script_stream);
  if(profiling >= 0)
  { 
    clock_gettime(CLOCK_MONOTONIC, &end);
    clock_gettime(CLOCK_REALTIME, &time_ended);
    getrusage(RUSAGE_SELF, &r_usage);
    end_time = time_ended.tv_sec + time_ended.tv_nsec / 1000000000.0;
    real_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    user_time = r_usage.ru_utime.tv_sec + r_usage.ru_utime.tv_usec / 1000000.0;
    system_time = r_usage.ru_stime.tv_sec + r_usage.ru_stime.tv_usec / 1000000.0;
    num_chars += snprintf(str, 1024, "%f %f %f %f", end_time, real_time, user_time, system_time);
    num_chars += snprintf(str + num_chars, 1024 - num_chars, " [%d]", getpid());
    str[num_chars++] = '\n';
    write(profiling, (void*) str, num_chars);
    close(profiling);
  }
  return print_tree || !last_command ? 0 : status;
}
