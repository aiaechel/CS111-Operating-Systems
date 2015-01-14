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
    command_stream ret;
    command_t error = NULL;
    command reference;
    char a, prevChar = '\n';
    size_t max_size = 1024;
    char* everything = (char*) checked_malloc(max_size);
    int line_num = 1, size = 0, saved_location = 0;
    int paren_line, open_paren = 0, invalid = 0;
    while(a = get_next_byte(get_next_byte_argument))
    {
        if (prevChar == '\n' && a == '#')
		{
			while (a != '\n' && a != EOF)
			{
				prevChar = a;
				a = get_next_byte (get_next_byte_argument);
			}
		}
        else if(a == '(')
        {
            open_paren++;
            if(open_paren == 1)
            {
                paren_line = line_num;
                saved_location = size;
            }
        }
        else if(a == ')' && open_paren)
        {
            open_paren--;
            if(open_paren < 0)
            {
                paren_line = line_num;
                saved_location = size;
                break;
            }
            saved_location = 0;
        }
        if(!check_char(a, 1))
        {
            invalid = 1;
            saved_location = size;
            break;
        }
        if(a == '\n')
            line_num++;
        if(size == max_size - 2)
            everything = (char*) checked_grow_alloc(everything, &max_size);
        everything[size++] = a;
        prevChar = a;
    }
    if(open_paren)
    {
        error = (command_t) checked_malloc(sizeof(reference));
        error->type = ERROR_COMMAND;
        error->status = line_num;
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
            command_list = (command_t*) checked_realloc(command_list, sizeof(command_t) * (num_commands + 1));
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
format_function (char* array, int beg, int end, command_t reserved)
{
    int index = beg, less = end + 1, greater = end + 1, flag = 0, word = 0;
    command_t ret;
    command reference;
    while (index != end + 1)
    {
        if(reserved && !flag)
        {
            if(array[index] == '\n' || array[index] == ';')
                return;
            else if(isspace(array[index]))
            {
                index++;
                beg++;
                continue;
            }
            else if(check_char(array[index], 0))
                return;
        }
        if(!word)
        {
            if(check_char(array[index], 0))
                word = 1;
        }
        if(array[index] == '<')
        {
            if (greater < end + 1 || less < end + 1 || (!reserved && !word))
                return;
            less = index;
            flag = 1;
            word = 0;
        }
        else if(array[index] == '>')
        {
            if(greater < end + 1 || (!reserved && !word) || (reserved && less < end + 1 && !word))
                return;
            greater = index;
            flag = 1;
            word = 0;
        }
        if(index == beg)
        {
            if(isspace(array[index]))
                beg++;
            else if(!reserved && (array[index] == '<' || array[index] == '>'))
            {
                return;
            }
        }
        index++;
    }
    if(index == beg || !word)
        return;
    if(reserved)
    {
        ret = reserved;
    }
    else
    {
        ret = (command_t) checked_malloc(sizeof(reference));
        ret->u.word = (char**) checked_malloc(sizeof(char*) * 2);
        ret->u.word[0] = remove_whitespace(array, beg, (less < end + 1 ? less - 1 : greater - 1);
        ret->u.word[1] = NULL;
        ret->type = SIMPLE_COMMAND;
        ret->status = -1;
    }
    if(less < end + 1)
    {
        ret->input = remove_whitespace(array, less + 1, greater - 1);
    }
    if(greater < end + 1)
    {
        ret->output = remove_whitespace(array, greater + 1, end);
    }
    return ret;
}

char
read_word(char* array, int* beg, int* end)
{
    int begindex = *beg;
	int endindex = *end;
	int space = 1;
	if (begindex == endindex)
		return;
	while(begindex != endindex + 1)
	{
		if(!isalpha(array[begindex]))
		{
			if(isspace(array[begindex]))
			{
				if (space == 1)
				{
					begindex++;
					if(begindex != endindex + 1)
						*beg = begindex;
					continue;
				}
				else
					break;
			}
			if(array[begindex] == ';' || array[begindex] == '|' || array[begindex] == '(' || array[begindex] == ')' || array[begindex] == '<' || array[begindex] == '>')
			{
				if(*beg == begindex)
				{
					*end = *beg;
				}
				break;
			}
		}
		*end = begindex;
		space = 2;
		begindex++;
	}
	if(begindex == endindex + 1)
		begindex--;
	return array[begindex];
}

int
check_reserved_word(char* array, int beg, int end)
{
    //0: not a reserved word
	//1: if (size 2)
	//2: then (size 4)
	//3: else (size 4)
	//4: fi (size 2)
	//5: while (size 5)
	//6: do (size 2)
	//7: done (size 4)
	//8: until (size 5)
	
	if(end - beg + 1 == 2)
	{
		if(array[beg] == 'i' && array[beg + 1] == 'f') //if
			return 1;
		else if(array[beg] == 'f' && array[beg + 1] == 'i')//fi
			return 4;
		else if(array[beg] == 'd' && array[beg + 1] == 'o')//do
			return 6;
		else
			return 0;
 	}
	else if(end - beg + 1 == 4)
	{
		if(array[beg] == 't' && array[beg + 1] == 'h' && array[beg + 2] == 'e' && array[beg + 3] == 'n') //then
			return 2;
		else if(array[beg] == 'e' && array[beg + 1] == 'l' && array[beg + 2] == 's' && array[beg + 3] == 'e') //else
			return 3;
		else if(array[beg] == 'd' && array[beg + 1] == 'o' && array[beg + 2] == 'n' && array[beg + 3] == 'e') //done
			return 7;
		else
			return 0;
	}
	else if(end - beg + 1 == 5)
	{
		if(array[beg] == 'w' && array[beg + 1] == 'h' && array[beg + 2] == 'i' && array[beg + 3] == 'l' && array[beg + 4] =='e') //while
			return 5;
		else if(array[beg] == 'u' && array[beg + 1] == 'n' && array[beg + 2] == 't' && array[beg + 3] == 'i' && array[beg + 4] =='l') //until
			return 8;
		else
			return 0;
	}
	else
		return 0;
}

char*
remove_whitespace(char* array, int beg, int end)
{
    int i;
    while(isspace(array[beg]))
        beg++;
    while(isspace(array[end]) || array[end] == ';')
        end--;
    char* text = (char*) checked_malloc(end - beg + 2);
    for(i = 0; i < end - beg + 1; i++)
    {
        text[i] = array[beg + i];
    }
    test[i] = '\0';
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
check_char(int a, int flag)
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
            return (isalnum(a) || (flag && isspace(a)));
    }
}
void
free_command(command_t c)
{
    if(c == NULL)
        return;
    switch (c->type)
    {
    case ERROR_COMMAND:
        break;
    case SIMPLE_COMMAND:
        free(c->u.word[0]);
        free(c->u.word);
        break;
    case IF_COMMAND:
        free_command(c->u.command[2]);
    case UNTIL_COMMAND:
    case WHILE_COMMAND:
    case SEQUENCE_COMMAND:
    case PIPE_COMMAND:
        free_command(c->u.command[1]);
    case SUBSHELL_COMMAND:
        free_command(c->u.command[0]);
        break;
    default:
        break;
    }
    free(c);
}

void
free_stream(command_stream c)
{
    int i;
    for(i = 0; c.command_list[i] != NULL; i++)
    {
        free_command(c.command_list[i]);
    }
}