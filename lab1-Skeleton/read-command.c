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
#include <stdlib.h>
#include <ctype.h>
#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */
struct command_stream_t
{
    command_t* command_list;
};
int check_char(int a, int flag);
void free_everything(command_t* list, int size);
char* remove_whitespace(char* array, int beg, int end);
int check_reserved_word(char* array, int beg, int end);
char read_word(char* array, int* beg, int* end);
command_t format_function (char* array, int beg, int end, command_t reserved);
void split_everything(char* array, int beg, int end);
void make_error (int linenum);

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    command_stream_t ret;
    char a, prevChar = '\n';
    size_t max_size = 1024, size = 0;
    char* everything = (char*) checked_malloc(max_size);
    while((a = get_next_byte(get_next_byte_argument)))
    {
        if(size == max_size - 2)
            everything = (char*) checked_grow_alloc(everything, &max_size);
        everything[size++] = a;
        prevChar = a;
    }
    ret.command_list = format_everything(everything, 0, size - 1);
    free(everything);
    return ret;
}    
command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  return 0;
}
void
split_everything(char* array, int beg, int end)
{
    int index = beg, word_end = end, line_num = 1, reserved = 0;
    int in_command = 0, command_start = beg, command_end = end, num_semi = 0, first_command = 0, cpd_sub = 0, found_char = 0, found_word = 0,
        num_endl = 0, num_parens = 0, little_command = 0, invalid = 0, greater = 0, less = 0, compound_line = 0, done_size = 0;
    int compound_count[9] = {0,0,0,0,0,0,0,0,0};
    int locations[4] = {-1,-1,-1,-1};
    int* done_check = NULL;
    char prev_char = '\n', prev_rel_char = '\n', cur_char;
    printf("Start\n");
    while(index < end + 1)
    {
        //true
        cur_char = array[index];
        printf("%c\n", cur_char);
        switch(cur_char)
        {
            case '\t':
            case ' ': 
                break;
            case ';':
                //and ; be
                if(prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '(')
                {
                    invalid = 1;
                    break;
                }
                num_semi++;
                less = 0;
                greater = 0;
                found_char = 0;
                if(little_command == 1)
                {
                    little_command = 0;
                    command_end = index - 1;
                }
                break;
            case '\n':
                if(prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>')
                {
                    invalid = 1;
                    break;
                }
                line_num++;
                num_endl++;
                found_char = 0;
                if(little_command == 1)
                {
                    little_command = 0;
                    command_end = index - 1;
                }
                less = 0;
                greater = 0;
                //found_end_line = index;
                break;
            case '|':
                if(prev_rel_char == ';' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '|')
                    invalid = 1;
                less = 0;
                greater = 0;
                found_char = 0;
                break;
            case '(':
                num_parens++;
                if(prev_rel_char == '<' || prev_rel_char == '>' || (check_char(prev_rel_char, 2) && prev_rel_char != '\n' && prev_rel_char != '|'
                    && prev_rel_char != ';' && prev_rel_char != '('))
                {
                    make_error(line_num);
                    return;
                }
                if(!in_command)
                {
                    in_command = 1;
                    command_start = index;
                }
                if(!little_command)
                {
                    little_command = 1;
                    
                }
                break;
            case ')':
                if(!num_parens || prev_rel_char == '(' || prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>')
                {
                    make_error(line_num);
                    return;
                }
                num_parens--;
                break;
            case '<':
                if(prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '('
                    || less || greater)
                {
                    invalid = 1;
                }
                less++;
                break;
            case '>':
                if(prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '(' || greater)
                {
                    invalid = 1;
                }
                greater++;
                break;
            case '#':
                if(prev_rel_char == '\n')
                {
                    while (array[index] != '\n' && index < end + 1)
                    {
                        index++;
                    }
                    index++;
                    line_num++;
                }
                else
                    invalid = 1;
                break;
            default:
                if(!check_char(cur_char, 1)) //!check_char
                {
                    invalid = 1;
                    break;
                }
                else
                {
                    if(!in_command)
                    {
                        in_command = 1;
                        command_start = index;
                    }
                    if(first_command && check_char(cur_char, 2))
                    {
                        invalid = 1;
                        break;
                    }
                    if(!little_command || compound_count[0])
                    {
                        little_command = 1;
                        word_end = end;
                        prev_char = read_word(array, &index, &word_end);
                        reserved = check_reserved_word(array, index, word_end);
                        
                    int i;
                    for(i = index; i < word_end + 1; i++)
                    {
                        printf("%c", array[i]);
                    }
                    printf("\nReserved: %d\n", reserved);
                        if(reserved && !found_char)
                        {
                            if(!cpd_sub)
                            {
                                cpd_sub = reserved;
                                locations[0] = index;
                            }
                            if(reserved == 1 || reserved == 5 || reserved == 8)
                            {
                                
                            }
                            else if(prev_rel_char != ';' && prev_rel_char != '\n' && prev_rel_char != '|' && prev_rel_char != '(')
                            {
                                invalid = 1;
                                reserved = 0;
                            }
                            else if(!(reserved == 1 || reserved == 5 || reserved == 8) && (prev_rel_char == '|' || prev_rel_char == '(' || !found_word))
                            {
                                invalid = 1;
                                reserved = 0;
                            }
                            printf("Invalid: %d\n", invalid);
                            found_word = 0;
                            switch(reserved)
                            {
                                case 0: //not_reserved
                                    break;
                                case 1: //if
                                    compound_count[reserved]++;
                                    compound_count[0]++;
                                    break;
                                case 5: //while
                                case 8: //until
                                    if(done_size < compound_count[5] + compound_count[8] + 1)
                                    {
                                        done_check = (int* ) checked_realloc(done_check, sizeof(int)*(compound_count[5] + compound_count[8] + 1));
                                        done_size++;
                                    }
                                    done_check[compound_count[5] + compound_count[8]] = reserved;
                                    compound_count[reserved]++;
                                    compound_count[0]++;
                                    break;
                                case 2: //then
                                    if(!compound_count[1] )
                                        invalid = 1;
                                    if(cpd_sub == 1)
                                    if(compound_count[1] == 1 && !compound_count[5] && !compound_count[8])
                                        locations[1] = index;
                                    compound_count[2]++;
                                    break;
                                case 3: //else
                                    if(cpd_sub == 1)
                                    if(compound_count[1] == 1 && !compound_count[5] && !compound_count[8])
                                        locations[2] = index;
                                    if(!compound_count[2])
                                        invalid = 1;
                                    compound_count[3]++;
                                    break;
                                case 6: //do
                                    if(cpd_sub == 5 || cpd_sub == 8)
                                    {
                                        if((compound_count[5] == 1 && !compound_count[1] && !compound_count[8]) || (compound_count[8] == 1 && !compound_count[1] && !compound_count[5]))
                                            locations[1] = index;
                                    }
                                    if(!compound_count[5] && !compound_count[8])
                                        invalid = 1;
                                    compound_count[6]++;
                                    break;
                                case 4: //fi
                                    if(compound_count[1] && compound_count[2])
                                    {
                                    if(cpd_sub == 1)
                                        if(compound_count[1] == 1 && !compound_count[5] && !compound_count[8])
                                            locations[3] = index;
                                        compound_count[1]--;
                                        compound_count[2]--;
                                        if(compound_count[3])
                                            compound_count[3]--;
                                        compound_count[0]--;
                                    }
                                    else
                                        invalid = 1;
                                    break;
                                case 7: //done
                                    if(compound_count[6])
                                    {
                                        if(compound_count[5] + compound_count[8] == 1)
                                            if(!compound_count[1])
                                                locations[2] = index;
                                        compound_count[6]--;
                                        if(compound_count[5])
                                            compound_count[5]--;
                                        else if(compound_count[8])
                                            compound_count[8]--;
                                        else
                                            invalid = 1;
                                        compound_count[0]--;
                                    }
                                    else
                                        invalid = 1;
                                    break;    
                                default:
                                    break;
                            }
                            if(!compound_count[0]) //if done with compounds
                            {
                                first_command = 1;
                                free(done_check);
                                done_check = NULL;
                            }
                            index = word_end;
                        }
                        else
                        {
                            
                            found_word = 1;
                            found_char = 1;
                            index = word_end;
                        }
                    }
                    num_endl = 0;
                    num_semi = 0;
                }
                
        }
        if(num_semi > 1)
        {   
            invalid = 1;
        }
        if(invalid)
        {
            make_error(line_num);
            return;
        }
        if(num_endl == 2 && cur_char != ' ' && cur_char != '\t')
        {
            int i;
            in_command = 0;
            if(num_parens)
            {
                make_error(line_num);
                return;
            }
            for(i = command_start; i < command_end + 1; i++)
            {
                printf("%c", array[i]);
            }
            printf("\n");
            //add to array: command_start, command_end
        }
        else if(num_endl > 2)
        {
            index++;
            continue;
        }
        if(cur_char != ' ' && cur_char != '\t')
            prev_rel_char = array[index];
        if(index == end)
        {
            if(prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>' || num_parens || compound_count[0])
            {
                make_error(line_num);
                return;
            }
        }
        index++;
    }
    in_command = 0;
    int i;
    for(i = command_start; i < end + 1; i++)
    {
        printf("%c", array[i]);
    }
    printf(".\nEnd\n");
    printf("Locations: %d %d %d %d", locations[0], locations[1], locations[2], locations[3]);
}
/*
command_t* 
format_everything(char* array, int beg, int end)
{
    int word_beg, word_end, reserved = 0, num_commands = 0, valid_command = 0, int compound_end = end;
    char splitter = 0;
    command_t* command_list;
    command_t comp_command, cur_command, container;
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
            container = (command_t) checked_malloc(sizeof(struct command));
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
*/

command_t
format_function (char* array, int beg, int end, command_t reserved)
{
    int index = beg, less = end + 1, greater = end + 1, flag = 0, word = 0;
    command_t ret;
    while (index < end + 1)
    {
        if(reserved && !flag)
        {
            if(array[index] == '\n' || array[index] == ';')
                return NULL;
            else if(isspace(array[index]))
            {
                index++;
                beg++;
                continue;
            }
            else if(check_char(array[index], 0))
                return NULL;
        }
        if(!word)
        {
            if(check_char(array[index], 0))
                word = 1;
        }
        if(array[index] == '<')
        {
            if (greater < end + 1 || less < end + 1 || (!reserved && !word))
                return NULL;
            less = index;
            flag = 1;
            word = 0;
        }
        else if(array[index] == '>')
        {
            if(greater < end + 1 || (!reserved && !word) || (reserved && less < end + 1 && !word))
                return NULL;
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
                return NULL;
            }
        }
        index++;
    }
    if(index == beg || !word)
        return NULL;
    if(reserved)
    {
        ret = reserved;
    }
    else
    {
        ret = (command_t) checked_malloc(sizeof(struct command));
        ret->u.word = (char**) checked_malloc(sizeof(char*) * 2);
        ret->u.word[0] = remove_whitespace(array, beg, (less < end + 1 ? less - 1 : greater - 1));
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
		return array[begindex];
	while(begindex != endindex + 1)
	{
		if(!check_char(array[begindex], 0))
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
			
		}
        else if(array[begindex] == ';' || array[begindex] == '|' || array[begindex] == '(' || array[begindex] == ')' || array[begindex] == '<' || array[begindex] == '>')
			{
				if(*beg == begindex)
				{
					*end = *beg;
				}
				break;
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
    text[i] = '\0';
    return text;
}

void
free_everything(command_t* list, int size)
{
    int i;
    if(list != NULL)
        for(i = 0; i < size; i++)
        {
            if(list[i] != NULL)
                free(list[i]);
        }
        free(list);
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

void
make_error (int linenum)
{
    printf("%d:There is a syntax error here.", linenum);
}