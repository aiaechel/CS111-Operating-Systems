#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

void
format_function (char* array, int beg, int end, int* reserved)
{
    int index = beg, less = end + 1, greater = end + 1, flag = 0, word = 0;
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
            else if(isalnum(array[index]))
                return;
        }
        if(!word)
        {
            if(isalnum(array[index]))
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
/*
    command_t ret;
    command reference;
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
    
*/
    int i;
    printf("The whole thing is: ");
    for(i = beg; i < end + 1; i++)
    {
        printf("%c", array[i]);
    }
    printf("\nHere are the parts:\n");
    if(!reserved) {
    printf("main: ");
    for(i = beg; i < less && i < greater; i++)
    {
            printf("%c", array[i]);
    }
    printf("|\n"); }
    if(less < end + 1)
    {
        printf("input: ");
        for(i = less + 1; i < greater; i++)
        {
            printf("%c", array[i]);
        }
        printf("|\n");
        //first->input = remove_whitespace(array, less + 1, (greater < end + 1 ? greater - 1 : index - 1))
    }
    if(greater < end + 1)
    {
        printf("output: ");
        for(i = greater + 1; i < end + 1; i++)
        {
            printf("%c", array[i]);
        }
        printf("|\n");
        //first->output = remove_whitespace(array, greater + 1, index - 1);
    }
    //first->status = -1;
    //first->type = SIMPLE_COMMAND;
    //Andrew
    /*if(index == end + 1)
    {
        return first;
    }
    second = format_function(array, index + 1, end, array[index]);
    container = (command_t) checked_malloc(sizeof(reference));
    container->status = -1;
    container->type = (type == '|' ? PIPE_COMMAND : SEQUENCE_COMMAND);
    container->u.command[0] = first;
    container->u.command[1] = second;
    return container;*/
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

/*void
compound_cmd(char* array, int beg, int* end, int type)
{
    //beg is right after the first word
    int word_beg = beg, word_end = *end;
    int reserved = 0;
    char stop = read_word(array, &word_beg, &word_end);
    reserved = check_reserved_word(array, word_beg, word_end);
    while(1)
    {
        if(reserved)
        {
            int sub_end = *end;
            compound_cmd
        }
        stop = read_word(array, &word_beg, &word_end);
    }
}
*/

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
    char* text = NULL;
    //char* text = (char*) _malloc(end - beg + 2);
    printf("Here it is: ");
    for(i = 0; i < end - beg + 1; i++)
    {
        //text[i] = array[beg + i];
        printf("%c", array[beg + i]);
    }
    printf(".\n");
    return text;
}


void
complete_command(char* array, int beg, int* end)
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
    
    
    
    /*int word_beg = beg, word_end = *end, reserve_check, break_flag = 0;
    int first_index = 0, cur_index = *end, last_semi = *end + 1;
    int i;
    //char splitter = read_word(array, &word_beg, &word_end);
    char prev_splitter = 0;
    while(word_beg < (*end) + 1)
    {
        if(array[word_beg] == ';' || array[word_beg] == '\n')
        {
            printf("Sequence Command: \n");
            printf("First: ");
            for(i = first_index; i < word_beg; i++)
            {
                printf("%c", array[i]);
            }
            printf("\nSecond: ");
            for(i = word_beg + 1; i < (*end) + 1; i++)
            {
                printf("%c", array[i]);
            }
            printf("\n");
            first_index = word_beg + 1;
            last_semi = word_beg;
            prev_splitter = array[word_beg];
        }
        else if(array[word_beg] == '|')
        {
            printf("Pipe Command: \n");
            printf("First: ");
            for(i = first_index; i < word_beg; i++)
            {
                printf("%c", array[i]);
            }
            printf("\nSecond: ");
            for(i = word_beg + 1; i < (*end) + 1; i++)
            {
                printf("%c", array[i]);
            }
            printf("\n");
            first_index = word_beg + 1;
            prev_splitter = array[word_beg];
        }
        else if(array[word_beg] == '(')
        {
            
        }
        word_beg++;
        /*reserve_check = check_reserved_word(array, word_beg, word_end);
        if(reserved)
        {
            switch(reserve_check)
            {
                case 0:
                    break;
                case 1:
                case 5:
                case 8:
                    //compound_cmd
                    break;
                case 2:
                    if(reserved == 1)
                    {
                        
                    }
                break;
            }
        }
        if(reserve_check)
        {
            //compound_cmd
        }
    }
    for(i = first_index; i < word_beg; i++)
    {
        printf("%c", array[i]);
    }
    printf("\n");*/
}

void
pipe_command(char* array, int beg, int end, int* locations, int num_locations)
{
    int i, index = beg, first = 2;
    //command_t container, current, c0, c1;
    if(locations == NULL)
    {
        //return format_command(array, beg, end);
        for(i = beg; i < end + 1; i++)
            printf("%c", array[i]);
        printf("\n");
        return;
    }
    for(i = 0; i < num_locations + 1; i++)
    {
    /*
        current = format_command(array, index, location[i] - 1);
        index = location[i] + 1;
    
        if(first)
        {
            if(first == 2)
            {
                c0 = current;
            }
            if(first == 1)
            {
                c1 = current;
                container->u.data[0] = c1;
                container->u.data[1] = c2;
            }
            first--;
        }
        else
        {
            c0 = container;
            container = (command_t) checked_malloc(sizeof(reference));
            container->type = PIPE_COMMAND;
            container->status = -1;
            container->u.data[0] = c0;
            container->u.data[1] = current;
        }
    */
    }
    //return container;
}

void 
format_command(char* array, int beg, int end)
{
/*
    int word_beg = beg, word_end = end, reserve;
    char splitter;
    command_t sub_command = NULL, container;
    while(array[beg] == ' ' || array[beg] == '\t')
        word_beg++;
    splitter = read_word(array, &word_beg, &word_end);
    reserve = check_reserved_word(array, word_beg, word_end);
    if(reserve)
    {
        sub_command = compound_cmd();
    }
    else if(splitter == '(')
    {
        sub_command = subshell(array, &word_beg, end);
    }
    container = format_function(array, word_end + 1, end);
    return container;
*/
}

void 
subshell(char* array, int* beg, int end)
{
    /*
    int sub_beg = *beg + 1, sub_end = end, num_parens = 1, index = *beg + 1;
    command_t inside, container;
    command reference;
    while(index < sub_end + 1)
    {
        if(array[index] == '(')
        {
            num_parens++;
        }
        else if(array[index] == ')')
        {
            num_parens--;
            if(num_parens == 0)
                break;
        }
    }
    if(!num_parens)
    {
        inside = complete_command(array, sub_beg, index);
        container = (command_t) checked_malloc(sizeof(reference));
        container->type = SUBSHELL_COMMAND;
        container->status = -1;
        container->u.command[0] = inside;
        return container;
    }
    else
    {
        //some error
    }
    */
}

/*
void
free_command(command_t c)
{
    switch (c->type)
    {
    case SIMPLE_COMMAND:
        free(c->u.word[0]);
        free(c->u.word);
        break;
    case IF_COMMAND:
        if(c->u.command[2])
            free_command(c->u.command[2]);
        if(c->u.command[1])
            free_command(c->u.command[1]);
        free_command(c->u.command[0]);
        break;
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
*/


void
make_error (int linenum)
{
    printf("%d:There is a syntax error here.", linenum);
}

void
split_everything(char* array, int beg, int end)
{
    int index = beg, word_end = end, line_num = 1, reserved = 0;
    int in_command = 0, command_start = beg, command_end = end, num_semi = 0, first_command = 0,
        num_endl = 0, num_parens = 0, little_command = 0, invalid = 0, greater = 0, less = 0, compound_line = 0;
    int compound_count[9] = {0,0,0,0,0,0,0,0,0};
    //int** locations = (int**) malloc(sizeof(int*));
    //locations[0] = (int*) malloc(sizeof(int));
    char prev_char = '\n', prev_rel_char = '\n', cur_char;
    //command_t error = NULL, list;
    printf("Start\n");
    while(index < end + 1)
    {
        //true
        cur_char = array[index];
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
                break;
            case '(':
                num_parens++;
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
                if(!num_parens || prev_rel_char == '(' || prev_rel_char == '|' || prev_rel_char == ';' || prev_rel_char == '<' || prev_rel_char == '>')
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
                if(!check_char(cur_char) || ) //!check_char
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
                    if(!little_command)
                    {
                        little_command = 1;
                        cur_char = read_word(array, &index, &word_end)
                        reserved = check_reserved_word(array, index, word_end);
                        switch(reserved)
                        {
                            case 0: //not_reserved
                                break;
                            case 1: //if
                            case 5: //while
                            case 8: //until
                                compound_count[reserved]++;
                                compound = 1;
                                compound_count[0]++;
                                break;
                            case 2: //then
                                if(!compound_count[1])
                                    invalid = 1;
                                compound_count[2]++;
                                break;
                            case 3: //else
                                if(!compound_count[2])
                                    invalid = 1;
                                compound_count[3]++;
                                break;
                            case 6: //do
                                if(!compound_count[5] && !compound_count[8])
                                    invalid = 1
                                compound_count[6]++;
                                break;
                            case 4: //fi
                                if(compound_count[1] && compound_count[2])
                                {
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
            if(prev_rel_char == '|' || prev_rel_char == '<' || prev_rel_char == '>' || num_parens)
            {
                make_error(line_num);
                return;
            }
        }
        prev_char = array[index];
        index++;
    }
    int i;
    in_command = 0;
    for(i = command_start; i < end + 1; i++)
    {
        printf("%c", array[i]);
    }
    printf(".\nEnd\n");
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
int main()
{
    char* test = "((a)) < a";
    int start = 0;
    int end = strlen(test) - 1;
    int word_end = end;
    int a = 0;
    int i;
    int* reserved = &a;
    char b = read_word(test, &start, &word_end);
    a = check_reserved_word(test, start, end);
    split_everything(test, start, end);
    /*printf("Stopped at: %c.\n", (start == end ? test[end-start] : b));
    for(i = start; i < end + 1; i++)
    {
        printf("%c", test[i]);
    }
    printf("\nReserved? %d", a);*/
    //split_everything(test, start, end);
    return 0;
}