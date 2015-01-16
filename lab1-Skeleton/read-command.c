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
struct command_stream
{
    command_t* command_list;
    int counter;
};
int check_char(int a, int flag);
char* remove_whitespace(char* array, int beg, int end);
int check_reserved_word(char* array, int beg, int end);
char read_word(char* array, int* beg, int* end);
void free_stream(command_stream_t c);
void free_command(command_t c);
command_t* split_everything(char* array, int beg, int end);
command_t format_function (char* array, int beg, int end, command_t reserved);
command_t compound_cmd(char* array, int beg, int end);
command_t complete_command(char* array, int beg, int end);
command_t pipe_command(char* array, int beg, int end, int* pipe_locations, int pipe_start);
command_t subshell(char* array, int beg, int end);
command_t format_command(char* array, int beg, int end);
void make_error (int linenum);
void find_semi_pipes(char* array, int beg, int end, int** semicolon, int** pipeline);

//reads all chars in stream, sends everything to 
command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    command_stream_t ret = (command_stream_t) checked_malloc(sizeof(struct command_stream));
    char a, prevChar = '\n';
    size_t max_size = 1024, size = 0;
    char* everything = (char*) checked_malloc(max_size);
    while((a = get_next_byte(get_next_byte_argument)) != EOF)
    {
        if(size == max_size - 2)
            everything = (char*) checked_grow_alloc(everything, &max_size);
        everything[size++] = a;
        prevChar = a;
	//printf("%c:", a);
    }
    ret->command_list = split_everything(everything, 0, size - 1);
    ret->counter = 0;
    free(everything);
    return ret;
}    
command_t
read_command_stream (command_stream_t s)
{
  if(s == NULL)
	  return NULL;
  return s->command_list[s->counter++];
}
command_t*
split_everything(char* array, int beg, int end)
{
    int index = beg, word_end = end, line_num = 1, reserved = 0;
    int in_command = 0, command_start = beg, command_end = end, num_semi = 0, first_command = 0, cpd_sub = 0, found_char = 0, found_word = 0,
        num_endl = 0, num_parens = 0, little_command = 0, invalid = 0, greater = 0, less = 0, last_reserved = 0, done_size = 0, command_count = 0;
    int compound_count[9] = {0,0,0,0,0,0,0,0,0};
    int* done_check = NULL;
    char prev_char = '\n', prev_rel_char = '\n', cur_char;
    command_t* command_array = (command_t*) checked_malloc(sizeof(command_t));
    command_array[command_count] = NULL;
    command_t current = NULL;
    //printf("\nStart-------------------------------------------------------\n");
    while(index < end + 1 && isspace(array[index]))
    {
        if(array[index] == '\n')
            line_num++;
        index++;
    }
    while(index < end + 1)
    {
        //true
        cur_char = array[index];
        //printf("%c.\n", cur_char);
        switch(cur_char)
        {
            case '\t':
            case ' ': 
                break;
            case ';':
                if(prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '(' || (cpd_sub && !found_word && !first_command))
                {
                    invalid = 1;
                    break;
                }
                num_semi++;
                less = 0;
                greater = 0;
                found_char = 0;
                first_command = 0;
                if(little_command == 1)
                {
                    little_command = 0;
                    command_end = index - 1;
                }
                break;
            case '\n':
                if(prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>') //account for a | \n b please
                {
                    invalid = 1;
                    break;
                }
                if((cpd_sub && num_endl == 1) || prev_rel_char == ';')
                {
                    array[index] = ' ';
                    line_num++;
                    break;
                }
                line_num++;
                num_endl++;
                found_char = 0;
                first_command = 0;
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
                if(prev_rel_char == ';' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '|' || (cpd_sub && !found_word))
                {
                    invalid = 1;
                    break;
                }
                less = 0;
                greater = 0;
                found_char = 0;
                break;
            case '(':
                num_parens++;
                if(prev_rel_char == '<' || prev_rel_char == '>' || (check_char(prev_rel_char, 2) && prev_rel_char != '\n' && prev_rel_char != '|'
                    && prev_rel_char != ';' && prev_rel_char != '(' && !cpd_sub))
                {
                    invalid = 1;
                    break;
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
                num_semi = 0;
                num_endl = 0;
                break;
            case ')':
                if(!num_parens || prev_rel_char == '(' || prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>')
                {
                    invalid = 1;
                    break;
                }
                num_parens--;
                num_semi = 0;
                num_endl = 0;
                found_char = 0;
                
                break;
            case '<':
                if((prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '('
                    || less || greater))
                {
                    invalid = 1;
                    break;
                }
                first_command = 0;
                less++;
                break;
            case '>':
                if(prev_rel_char == ';' || prev_rel_char == '|' || prev_rel_char == '\n' || prev_rel_char == '<' || prev_rel_char == '>' || prev_rel_char == '(' || greater)
                {
                    invalid = 1;
                    break;
                }
                first_command = 0;
                greater++;
                break;
            case '#':
                if(prev_rel_char == '\n')
                {
                    while (array[index] != '\n' && index < end + 1)
                        index++;
                    index++;
                    if(index != end + 1)
                        line_num++;
                    continue;
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
                        if(!prev_char)
                        {
                            invalid = 1;
                            break;
                        }
                        reserved = check_reserved_word(array, index, word_end);
                        /*int i;
                        for(i = index; i < word_end + 1; i++)
                        {
                            printf("%c", array[i]);
                        }
                        printf(".\nReserved: %d\n", reserved);*/
                        if(reserved && !found_char)
                        {
                            if(!cpd_sub)
                            {
                                cpd_sub = reserved;
                                found_char = 0;
                            }
                            
                            if(reserved == 1 || reserved == 5 || reserved == 8)
                            {
                                if(last_reserved != 4 && last_reserved != 7)
                                {
                                    
                                }
                                else if(prev_rel_char != ';' && prev_rel_char != '\n' && prev_rel_char != '|' && prev_rel_char != '(')
                                {
                                    invalid = 1;
                                    break;
                                }
                            }
                            else if(prev_rel_char != ';' && prev_rel_char != '\n')
                            {
                                invalid = 1;
                                break;
                            }
                            //printf("Invalid: %d\n", invalid);
                            last_reserved = reserved;
                            found_word = 0;
                            switch(reserved)
                            {
                                case 0: //not_reserved
                                    break;
                                case 1: //if
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    compound_count[reserved]++;
                                    compound_count[0]++;
                                    break;
                                case 5: //while
                                case 8: //until
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    if(done_size < compound_count[5] + compound_count[8] + 1)
                                    {
                                        done_check = (int* ) /*checked_*/realloc(done_check, sizeof(int)*(compound_count[5] + compound_count[8] + 1));
                                        done_size++;
                                    }
                                    done_check[compound_count[5] + compound_count[8]] = reserved;
                                    compound_count[reserved]++;
                                    compound_count[0]++;
                                    break;
                                case 2: //then
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    if(!compound_count[1] )
                                        invalid = 1;
                                    compound_count[2]++;
                                    break;
                                case 3: //else
                                    if(!compound_count[2])
                                        invalid = 1;
                                    compound_count[3]++;
                                    break;
                                case 6: //do
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    if(!compound_count[5] && !compound_count[8])
                                        invalid = 1;
                                    compound_count[6]++;
                                    break;
                                case 4: //fi
                                    if(compound_count[1] && compound_count[2])
                                    {
                                    if(cpd_sub == 1)
                                        compound_count[1]--;
                                        compound_count[2]--;
                                        if(compound_count[3])
                                            compound_count[3]--;
                                        compound_count[0]--;
                                        first_command = 1;
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    }
                                    else
                                        invalid = 1;
                                    break;
                                case 7: //done
                                    if(compound_count[6])
                                    {
                                        if(!compound_count[6] && (!compound_count[5] || !compound_count[8]))
                                            invalid = 1;
                                        compound_count[6]--;
                                        compound_count[done_check[compound_count[5] + compound_count[8] - 1]]--;
                                        compound_count[0]--;
                                        first_command = 1;
                                    //    printf("Compound count: %d\n", compound_count[0]);
                                    }
                                    else
                                        invalid = 1;
                                    break;    
                                default:
                                    break;
                            }
                            if(!compound_count[0]) //if done with compounds
                            {
                                found_word = 0;
                                cpd_sub = 0;
                                free(done_check);
                                done_check = NULL;
                                done_size = 0;
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
            invalid = 1;        
        if(invalid)
        {
            make_error(line_num);
            return command_array;
        }
        if(num_endl == 2 && cur_char != ' ' && cur_char != '\t')
        {
            //int i;
            in_command = 0;
            if(num_parens)
            {
                make_error(line_num);
                return command_array;
            }
            current = complete_command(array, command_start, command_end);
            command_array = (command_t*) checked_realloc(command_array, sizeof(command_t)*(command_count + 2));
            command_array[command_count++] = current;
            command_array[command_count] = NULL;
            /*for(i = command_start; i < command_end + 1; i++)
            {
                printf("%c", array[i]);
            }
            printf("\n-------------------------------------------------------\n");*/
            //add to array: command_start, command_end
        }
        else if(num_endl > 2)
        {
            index++;
            continue;
        }
        if(cur_char != ' ' && cur_char != '\t')
            prev_rel_char = cur_char;
        if(index == end)
        {
            if(prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>' || num_parens || compound_count[0])
            {
                make_error(line_num);
                return command_array;
            }
        }
        index++;
    }
    in_command = 0;
    if(!invalid)
    {
        current = complete_command(array, command_start, end);
        command_array = (command_t*) checked_realloc(command_array, sizeof(command_t)*(command_count + 2));
        command_array[command_count++] = current;
        command_array[command_count] = NULL;
    }
    return command_array;
    /*int i;
    for(i = command_start; i < end + 1; i++)
    {
        printf("%c", array[i]);
    }*/
    //printf("\n-------------------------------------------------------End\n");
}

command_t
format_function (char* array, int beg, int end, command_t reserved)
{
    int index = beg, less = end + 1, greater = end + 1;
    while (index < end + 1)
    {
        if(array[index] == '<')
            less = index;
        else if(array[index] == '>')
            greater = index;
        if(index == beg)
            if(isspace(array[index]))
                beg++;
        index++;
    }
    command_t ret;
    if(reserved)
        ret = reserved;
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

command_t
compound_cmd(char* array, int beg, int end)
{
  int word_beg, word_end = 0; int index = beg; int stop = end; 
  int if_num;
  int while_num, until_num = 0;
  
  int first_if = beg;
  int first_then = -1;
  int first_else = -1;
  int first_fi = end;
  
  int first_while = beg;
  int first_do = -1;
  int first_done = end;
  int first_until = beg;

  int type = 0;
  int reserved = 0;
  
  command_t container = NULL;
  
  //this part is getting the type of command
  word_beg = index;
  word_end = stop;
  read_word(array, &word_beg, &word_end);
  type = check_reserved_word(array, word_beg, word_end);
  
  container = (command_t)checked_malloc(sizeof(struct command));
  container->status = -1;
  
  if(type == 1) //if statement
  {
	index = word_beg + 2;
	if_num = 1;
	container->type = IF_COMMAND;
	while(index != stop)
    {
        word_beg = index;
        word_end = stop;
        read_word(array, &word_beg, &word_end);
        reserved = check_reserved_word(array, word_beg, word_end);
		if(reserved == 1) //if
		{
			if_num++;
		}
		else if(reserved == 4) //fi
		{
			if_num--;
		}
		else if(reserved == 2) //then
		{
			if(if_num == 1)
				first_then = word_beg;
		}
		else if (reserved == 3) //else
		{
			if(if_num == 1)
				first_else = word_beg;
		}
		index = word_end + 1;
	}
	if(first_else == -1)
	{
		container->u.command[0] = complete_command(array, first_if + 2, first_then + 4);
		container->u.command[1] = complete_command(array, first_then + 4, first_fi + 2);
		container->u.command[2] = NULL;
	}
	else
	{
		container->u.command[0] = complete_command(array, first_if + 2, first_then + 4);
		container->u.command[1] = complete_command(array, first_then + 4, first_else + 4);
		container->u.command[2] = complete_command(array, first_else + 4, first_fi + 2);
	}
  }
  if(type == 5 || type == 8)
  {
	  if(type == 5)
	  {
		while_num = 1;
		container->type = WHILE_COMMAND;
	  }
	  if(type == 8)
      {
		until_num = 1;
		container->type = UNTIL_COMMAND;
	  }
	  index = word_beg + 5;
	  while(index != stop)
    {
        word_beg = index;
        word_end = stop;
        read_word(array, &word_beg, &word_end);
        reserved = check_reserved_word(array, word_beg, word_end);
		if(type == 5 && reserved == 5) //while
		{
			while_num++;
		}
		else if(type == 8 && reserved == 8) //until
		{
			until_num++;
		}
		else if(reserved ==  6) //do
		{
			if(type == 5 && while_num == 1)
			{
				first_do = word_beg;
			}
			else if(type == 8 && until_num == 1)
			{
				first_do = word_beg;
			}
		}
		else if (reserved == 7) //done
		{
			
			if(type == 5)
				while_num--;
			else if(type == 8)
				until_num--;
		}
		index = word_end + 1;
	}
	if(type == 5) //while
	{
		container->u.command[0] = complete_command(array, first_while + 5, first_do + 2);
		container->u.command[1] = complete_command(array, first_do + 2, first_done + 4);
		container->u.command[2] = NULL;
	}
	if (type == 8) //until
	{
		container->u.command[0] = complete_command(array, first_until + 5, first_do + 2);
		container->u.command[1] = complete_command(array, first_do + 2, first_done + 4);
		container->u.command[2] = NULL;
	}
  }
  return container;
}

command_t
complete_command(char* array, int beg, int end)
{
    
    //semi_locations is array of locations of semicolons, last number in array is -1, if it exists
    //pipe_locations is array of locations of pipes, last number in array is -1, if it exists
    int i = 0, j = 0, first = 1, loc1, loc2;
    int* semi_locations = NULL;
    int* pipe_locations = NULL;
    while(isspace(array[beg]) || array[beg] == ';')
        beg++;
    while(isspace(array[end]) || array[end] == ';')
        end--;
    find_semi_pipes(array, beg, end, &semi_locations, &pipe_locations);
    command_t container = NULL, c0, current = NULL;
    while(semi_locations[i] != -1)
    {
        loc1 = (first ? beg : semi_locations[i]);
	loc2 = (first ? semi_locations[i] : (semi_locations[i + 1] == -1 ? end : semi_locations[i + 1]));
        while(pipe_locations[j] != -1 && pipe_locations[j] < loc1)
            j++;
        while(isspace(array[loc1]) || array[loc1] == ';')
            loc1++;
        while(isspace(array[loc2]) || array[loc2] == ';')
            loc2--;
        current = pipe_command(array, loc1, loc2, pipe_locations, j);
        if(first)
        {
            container = current;
            first = 0;
	    continue;
        }
        else
        {
            c0 = container;
            container = (command_t) checked_malloc(sizeof(struct command));
            container->type = SEQUENCE_COMMAND;
            container->status = -1;
            container->u.command[0] = c0;
            container->u.command[1] = current;
        
        }
        i++;
    }
    
    if(container == NULL)
        container = pipe_command(array, beg, end, pipe_locations, 0);
    free(semi_locations);
    free(pipe_locations);
    return container;
}

command_t
pipe_command(char* array, int beg, int end, int* pipe_locations, int pipe_start)
{
    int i = pipe_start, first = 1, loc1, loc2;
    command_t container = NULL, current, c0;
    
    while(pipe_locations[i] != -1 && pipe_locations[i] < end)
    {
        loc1 = (first ? beg : pipe_locations[i]);
	loc2 = (first ? pipe_locations[i] : (pipe_locations[i + 1] == -1 ? end : pipe_locations[i + 1]));
        
        while(isspace(array[loc1]) || array[loc1] == '|')
            loc1++;
        while(isspace(array[loc2]) || array[loc2] == '|')
            loc2--;
        current = format_command(array, loc1, loc2);
    
        if(first)
        {
            container = current;
            first = 0;
	    continue;
        }
        else
        {
            c0 = container;
            container = (command_t) checked_malloc(sizeof(struct command));
            container->type = PIPE_COMMAND;
            container->status = -1;
            container->u.command[0] = c0;
            container->u.command[1] = current;
        }
        i++;
    }
    if(container == NULL)
        container = format_command(array, beg, end);
    return container;
}

command_t 
subshell(char* array, int beg, int end)
{
    command_t inside, container;
    inside = complete_command(array, beg, end);
    container = (command_t) checked_malloc(sizeof(struct command));
    container->type = SUBSHELL_COMMAND;
    container->status = -1;
    container->u.command[0] = inside;
    return container;
}

command_t
format_command(char* array, int beg, int end)
{
    int word_beg, word_end = end, reserve;
    char splitter;
    while(array[beg] == ' ' || array[beg] == '\t')
        beg++;
    word_beg = beg;
    splitter = read_word(array, &word_beg, &word_end);
    reserve = check_reserved_word(array, word_beg, word_end);
	command_t sub_command = NULL, container = NULL;
	
	int index = word_end + 1;
	int if_num = -1;
	int uwhile_num = -1;
	int type = -1;
	int stop = -1;
	int while_yes = 0;
    if(reserve)
    {
		if(reserve == 1)
		{
			if_num = 1;
			while(index != end + 1)
			{
			  word_end = end;
				read_word(array, &index, &word_end);
				type = check_reserved_word(array, index, word_end);
				if (type == 1) //if
					if_num++;
				if (type == 4)
				{
					if_num--;
					if(if_num == 0)
					{
						stop = index;
						break;
					}
				}
				index = word_end + 1;
			}
		}
		if(reserve == 5 || reserve == 8)
		{
			if(reserve == 5)
				while_yes = 1; //while_yes is 1 when it's a while command
			uwhile_num = 1;
			while(index != end + 1)
			{
			  word_end = end;
				read_word(array, &index, &word_end);
				type = check_reserved_word(array, index, word_end);
				if (type == 5 && while_yes == 1)
					uwhile_num++;
				if (type == 8 && while_yes == 0)
				{
					uwhile_num++;
				}
				if(type == 7) //done
				{
					uwhile_num--;
					if(uwhile_num == 0)
					{
						stop = index;
						break;
					}	
				}
				index = word_end + 1;
			}
		}
        sub_command = compound_cmd(array, word_beg, stop);
        word_end = end;
        read_word(array, &stop, &word_end);
	word_end++;
	word_beg = word_end;
    }
    else if(splitter == '(')
    {
        word_end = end;
        while(array[word_end] != ')')
            word_end--;
        word_end--;
        sub_command = subshell(array, word_beg + 1, word_end);
	word_end++;
	word_beg = word_end;
    }
    container = format_function(array, word_beg, end, sub_command);
    return container;
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
            else
                return 0;
			
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
    return text;
}

void find_semi_pipes(char* array, int beg, int end, int** semicolon, int** pipeline)
{
    int index = beg, word_end = end, first_reserved = 0, sub_cmd = 0, num_parens = 0;
    int num_res = 0;
    char a;
    size_t num_semis = 0, num_pipes = 0, max_size = 10;
    int* semis = (int*) checked_malloc(sizeof(int)*max_size);
    int* pipes = (int*) checked_malloc(sizeof(int)*max_size);
    while(index < end + 1)
    {
      word_end = end;
        a = read_word(array, &index, &word_end);
        sub_cmd = check_reserved_word(array, index, word_end);
        if(sub_cmd)
        {   
            if(!first_reserved)
                first_reserved = sub_cmd;
            switch(sub_cmd)
            {
                case 1: //if
                case 5: //while
                case 8: //until
                    if(sub_cmd == first_reserved || (first_reserved*sub_cmd == 40))
                        num_res++;
                    break;
                case 4: //fi
                    if(first_reserved == 1)
                        num_res--;
                    break;
                case 7: //done
                    if(first_reserved == 5 || first_reserved == 8)
                        num_res--;
                    break;
                default:
                    break;
            }
            if(!num_res)
            {
                sub_cmd = 0;
                first_reserved = 0;
            }
            index = word_end;
        }
        else if(a == '(')
        {
            num_parens++;
            index++;
            while(index < end + 1)
            {
                if(array[index] == ')')
                    num_parens--;
                else if(array[index] == '(')
                    num_parens++;
                if(!num_parens)
                    break;
                index++;
            }
        }
        else
        {
            index = word_end;
            while(index < end + 1 && array[index] != ';' && array[index] != '\n' && array[index] != '|')
                index++;
            if(first_reserved)
            {
            }
            else if(index == end || index == end + 1)
            {
                semis[num_semis] = -1;
                pipes[num_pipes] = -1;
                *semicolon = semis;
                *pipeline = pipes;
                return;
            }
            else if(array[index] == ';' || array[index] == '\n')
            {
                if(num_semis == max_size)
                {
                    max_size *= 2;
                    semis = (int*) realloc(semis, sizeof(int)*max_size);
                    pipes = (int*) realloc(semis, sizeof(int)*max_size);
                }
                semis[num_semis++] = index;
            }
            else if(array[index] == '|')
            {
                if(num_pipes == max_size)
                {
                    max_size *= 2;
                    semis = (int*) realloc(semis, sizeof(int)*max_size);
                    pipes = (int*) realloc(semis, sizeof(int)*max_size);
                }
                pipes[num_pipes++] = index;
            }
        }
        index++;
    }
     semis[num_semis] = -1;
                pipes[num_pipes] = -1;
                *semicolon = semis;
                *pipeline = pipes;
                return;

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
            return 1;
        case ';':
        case '|':
        case '(':
        case ')':
        case '>':
        case '<':
            return (flag != 2);
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
free_stream(command_stream_t c)
{
    int i;
    if(c != NULL)
    for(i = 0; c->command_list[i] != NULL; i++)
    {
        free_command(c->command_list[i]);
    }
    free(c);
}
//If not work or unexpected output, don't need to return command_t anymore; just print syntax error, and return from split everything
void
make_error (int linenum)
{
    fprintf(stderr, "%d: Syntax error found!", linenum);
}
