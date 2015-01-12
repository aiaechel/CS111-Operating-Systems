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

#include <stdio.h>
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
            fclose(get_next_byte_argument);
            error(1, 0, "%d: Syntax error found: char %c is not allowed!", line_num, a);
            return 0;
        }
        if(a == '\n')
            line_num++;
        everything = (char*) checked_realloc(everything, size + 1);
        everything[size++] = a;
    }
    command_t* command_list = format_everything(everything, 0, size - 1);
    fclose(get_next_byte_argument);
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
format_everything(char* array, int beg, int end)
{
    int word_beg, word_end, reserved = 0, num_commands = 0, valid_command = 0, int compound_end = end;
    char splitter = 0;
    command_t* command_list;
    command_t comp_command, cur_command, container;
    command reference;
    while(beg < end + 1)
    {
        word_beg = beg;
        word_end = end;
        read_word(array, &word_beg, &word_end);
        reserved = check_reserved_word(array, word_beg, word_end);
        if(reserved)
        {
            comp_command = compound_cmd(array, word_beg, &word_end, reserved);
            if(comp_command == NULL)
            {
                //some error here
            }
            compound_end = word_end;
            valid_command = 1;
            /*cur_command = compound_cmd(array, &word_beg, &word_end, reserved);
            if(cur_command == NULL)
            {
                free_everything(command_list, num_commands, array);
                error(1, 0, "Command not in correct format!\n");
            }
            command_list = (command_t*) checked_realloc(command_list, num_commands + 1);
            command_list[num_commands++] = command_list;
            beg = word_end + 1;
            while(beg != end + 1)
            {
                if(array[beg] == ';' || array[beg] == '\n')
                {
                    sequence = 1;
                }
            }*/
        }
        while(word_end < end + 1 && array[word_end] != '|' && array[word_end] != ';' && array[word_end] != '\n')
        {
            word_end++;
        }
        if(valid_command)
        {
            cur_command = format_function(array, word_beg, word_end - 1, '|', comp_command, compound_end + 1);
        }
        else
        {
            cur_command = format_function(array, word_beg, word_end - 1);
        }
        if(splitter == '|' || splitter == '\n' || splitter == ';')
        {
            container = (command_t) checked_malloc(sizeof(reference));
            container->u[0] = command_list[num_commands - 1];
            container->u[1] = cur_command;
            container->status = -1;
            container->type = splitter == '|' ? PIPE_COMMAND : SEQUENCE_COMMAND;
            command_list[num_commands] = container;
        }
        else
        {
            command_list = (command_t*) checked_realloc(command_list, num_commands + 1);
            command_list[num_commands++] = cur_command;
        }
        splitter = array[word_end];
        beg = word_end + 1;
        if(splitter == ';' || splitter == '\n')
        {
            if(beg < end + 1 && array[beg] == '\n')
            {
                valid_command = 0;
                compound_end = end;
                compound_command = NULL;
                cur_command = NULL;
                container = NULL;
                splitter = 0;
            }
        }
    }
}

command_t
compound_cmd(char* array, int beg, int* end, int type)
{
    
}

command_t
format_function (char* array, int beg, int end, char type = '|', command_t compound = NULL, int compound_end = 0)
{
    int index = beg, less = end + 1, greater = end + 1;
    command_t first, second, container;
    command reference;
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
    if(index == beg)
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
check_reserved_word(char* array, int beg, int end)
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

void
free_everything(command_t* list, int size, char* array = NULL)
{
    if(list != NULL)
        for(int i = 0; i < size; i++)
        {
            if(list[i] != NULL)
                free(list[i]);
        }
        free(list);
    if(array != NULL)
        free(array);
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