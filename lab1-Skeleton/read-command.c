// UCLA CS 111 Lab 1 command reading

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
#include "alloc.h"

#include <cstdio.h>
#include <ctype.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream
{
    command_t* command_list;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    
    char a;
    char* everything = NULL;
    int line_num = 1, size = 0;
    while(!feof(get_next_byte_argument))
    {
        a = get_next_byte(get_next_byte_argument);
        if(a == EOF)
            break;
        if(!check_char(a))
        {
            if(everything != NULL)
                free(everything);
            error(1, 0, "%d: Syntax error found: char %c is not allowed!", line_num, a);
            return 0;
        }
        if(a == '\n')
            line_num++;
        everything = (char*) checked_realloc(everything, size + 1);
        everything[size++] = a;
    }
    command_t* command_list = format_everything(everything, 0, size - 1);
    free(everything);
    return command_list;
}    
command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}

command_t* 
read_everything(char* array, int beg, int end)
{
}

command_t
compound_cmd(char* array, int beg, int end)
{
}

command_t
format_function (char* array, int beg, int end, char type = '|')
{
    int index = beg, less = end + 1, greater = end + 1;
    command_t first, second, container;
    command reference;
    if(type == ')')
    {
        int counter = 1;
        while(index != end + 1 && array[index] != ')')
        {
            if(array[index] == '(')
                counter++;
            else if(array[index] == ')')
                counter--;
            if(counter == 0)
                break;
            if(index == beg && isspace(array[index])
                beg++;
            index++;
        }
        if(counter != 0)
            return NULL;
        /*first = format_command(
          container = (command_t) malloc(sizeof(struct command));
          container->u.command[0] = first;
          container->type = SUBSHELL_COMMAND;
          container->status = -1;
          return container;
        */
    }
    while (index != end + 1 && (array[index] != '|' && (type == '|' || array[index] == ';' || array[index] == '\n'))
    {
        if(array[index] == '<')
        {
            if (greater < end + 1 || less < end + 1)
                return NULL;
            less = index;
        }
        else if(array[index] == '>')
        {
            if(greater < end + 1)
                return NULL;
            greater = index;
        }
        else if(array[index] == '(')
        {
            return format_function(array, index + 1, end, ')');
        }
        if(index == beg)
        {
            if(isspace(array[index]))
                beg++;
            else if(array[index] == '<' || array[index] == '>' || array[index] == ')')
            {
                return NULL;
            }
        }
        index++;
    }
    if(index == beg || (index == end && array[index] == '|')
        return NULL;
    first = (command_t) checked_malloc(sizeof(reference));
    first->u.word = (char**) checked_malloc(sizeof(char*));
    first->u.word[0] = remove_whitespace(array, beg, (less < end + 1 ? less - 1 : (greater < end + 1 ? greater - 1 : end)));
    if(less < end + 1)
    {
        first->input = remove_whitespace(array, less + 1, (greater < end + 1 ? greater - 1 : index - 1))
    }
    if(greater < end + 1)
    {
        first->output = remove_whitespace(array, greater + 1, index - 1);
    }
    first->status = -1;
    first->type = SIMPLE_COMMAND;
    //Andrew
    if(index == end + 1)
    {
        return first;
    }
    second = format_function(array, index + 1, end, array[index]);
    container = (command_t) checked_malloc(sizeof(reference));
    container->status = -1;
    container->type = (type == '|' ? PIPE_COMMAND : SEQUENCE_COMMAND);
    container->u.command[0] = first;
    container->u.command[1] = second;
    return container;
}

void
read_word(char* array, int* beg, int* end)
{
    
}

int
check_reserved_word(char* array, int* beg, int* end)
{
    
}

char*
remove_whitespace(char* array, int beg, int end)
{
    int i;
    while(isspace(array[beg]))
        beg++;
    while(isspace(array[end]) || array[end] == ';')
        end--;
    char* text = (char*) checked_malloc(end - beg + 1);
    for(i = 0; i < end - beg + 1; i++)
    {
        text[i] = array[beg + i];
    }
    return text;
}

int
check_char(int a)
{
    switch(a)
    {
        case '!':
        case '%':
        case '+':
        case '-':
        case ',':
        case '.':
        case '/':
        case ':':
        case '@':
        case '^':
        case '_':
        case ';':
        case '|':
        case '(':
        case ')':
        case '>':
        case '<':
            return 1;
        default:
            return (isalnum(a) || isspace(a));
    }
}