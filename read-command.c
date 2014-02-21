// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define false 0
#define true 1
typedef short bool;

int argc;
char** argv;

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

   int cur_shellnum = 1;
   
void copy (char* src, char* dest, int size)
{
	int i;
	for (i = 0; i < size; i++)
		dest[i] = src[i];
}

char* concat(char* str1, char* str2)
{
	if (!str1)
		return str2;
	if (!str2)
		return str1;
	int l1 = strlen(str1), l2 = strlen(str2), i;
	char* out = malloc(l1+l2);
	for (i = 0	;i < l1; ++i)
		out[i] = str1[i];
	for (		;i < l1+l2; ++i)
		out[i] = str2[i-l1];
	return out;
}

char* spaceclean(char* str)
{
	char* out = str;
	while (out && *out == ' ') out++;

	if (!out)
		return 0;

	char* end = out + strlen(out) - 1;
	while (end && end > out && *end == ' ') end--;

	*(end+1) = 0;
	return out;
}
  
typedef struct command_stream
{
	struct command_stream* next;
	struct command_stream* prev;
	command* root;
	
} command_stream;

typedef enum
{
	WORD_TOKEN, //ls foo
	SEMICOLON_TOKEN, // ;
	PIPE_TOKEN, // |
	AND_TOKEN, // &&
	OR_TOKEN, // ||
	LEFT_PAREN_TOKEN, // (
	RIGHT_PAREN_TOKEN, // )
	GREATER_TOKEN, // >
	LESS_TOKEN, // <
	NEWLINE_TOKEN, // \n
	NULL_TOKEN, //
} tokentype;

int precedence(tokentype t)
{
	switch(t)
	{
		case SEMICOLON_TOKEN:
			return -6;
		case PIPE_TOKEN:
			return -3;
		case AND_TOKEN:
			return -4;
		case OR_TOKEN:
			return -4;
		case GREATER_TOKEN:
			return -2;
		case LESS_TOKEN:
			return -2;
		default:
			return -99999;
	}
}

tokentype tokentypeOf(char c)
{
	switch(c)
	{
		case '(':
			return LEFT_PAREN_TOKEN;break;
		case ')':
			return RIGHT_PAREN_TOKEN;break;
		case '<':
			return LESS_TOKEN;break;
		case '>':
			return GREATER_TOKEN;break;
		case ';':
			return SEMICOLON_TOKEN;break;
		case '\n':
			return NEWLINE_TOKEN;break;
		default:
			return NULL_TOKEN;
	}
}

typedef struct token
{
	tokentype t;
	char** words;
	struct token* next;
	struct token* prev;
	int sub;
} token;

int find_sp(char* string, int size)
{
	int i;
	for (i = 0; i < size; i++)
		if (string[i] == ' ')
			return i;
	return -1;
}

int count_sp(char* string, int size)
{
	int i, count=0;
	for (i = 0; i < size; i++)
		if (string[i] == ' ')
			count++;
	return count? count : 1;
}

void addToken(token** start, token** end, token* temptoken)
{
	token* newtoken = malloc(sizeof(token));
	newtoken->t = temptoken->t;
	newtoken->words = temptoken->words;
	newtoken->sub = temptoken->sub;
	
	if (*start == NULL && *end == NULL) //this is 1st element
	{
		*end = *start = newtoken;
		(*end)->prev = (*end)->next = NULL;
	}
	else if ((*end)->next) //end is not really the end
	{
		newtoken->prev = *end;
		newtoken->next = (*end)->next;
		(*end)->next = newtoken;
		newtoken->next->prev = newtoken;
	}
	else
	{
		newtoken->next = NULL;
		newtoken->prev = *end;
		(*end)->next = newtoken;
		*end = newtoken;
	}
}

token* popToken(token** start, token** end)
{
	if (!(*end)) return 0;
	token* output = *end;
	if (*start == *end)
		*start = *end = NULL;
	else
	{
		*end = (*end)->prev;
		(*end)->next = NULL;
	}
	return output;
}

void addToken_make(token*** start, token*** end, tokentype t, char* tempchars, int cwp, bool paren) //copy memory, paren is no longer used
{
	//first transfer tokchars to a 2-d chararray only if it's relevant
	int i;
	bool x = paren;
	char** list;
	int TOT_SIZE, TOT_LINES;
	char* t_tokchars = malloc(cwp+1);
	for (i=0; i < cwp; i++)
		t_tokchars[i] = tempchars[i];
	t_tokchars[i] = ' ';
	char* tokchars = malloc(cwp+1);
	int j=0;
	i= 0;
	while (i < cwp+1)
	{
		if (t_tokchars[i] == ' ')
		{	
			tokchars[j++] = ' ';
			while (i < cwp+1 && t_tokchars[i] == ' ')
				i++;
		}
		else
			tokchars[j++] = t_tokchars[i++];
	}
	int clistpos;
	if (cwp > 0)
	{
		TOT_SIZE = cwp+1;
		TOT_LINES = count_sp(tokchars, TOT_SIZE);
		list = (char**) malloc((TOT_LINES+1)*sizeof(char*));
		int space = find_sp(tokchars, TOT_SIZE);
		clistpos = 0;
		int bytespos = 0;
		while (space != -1 && bytespos != TOT_SIZE)
		{
			list[clistpos] = (char*) malloc(space+1);
			for (i=0; i <= space; i++)
				list[clistpos][i] = tokchars[bytespos+i];
			
			bytespos += space+1;
			space = find_sp(tokchars+bytespos, TOT_SIZE-bytespos);
			clistpos++;
		}
		for (i = 0; i < clistpos; i++)
		{
			char* str = spaceclean(list[i]);
			if (!str || *str == 0)
			{
				list[i] = 0;
				break;
			}
			else //design project: replaces all occurrences of $n with argv[n]
			{
				int sl = strlen(str);
				char* out = malloc(sl+1);
				int diff = 0;
				for (j = 0; j < sl; j++) //j represents index in out, j-diff represents index in str; start with $1 and go through $9
					if (str[j-diff] == '$' && str[j-diff+1] && '0' <= str[j-diff+1] && str[j-diff+1] <= '9')
					{
						int narg = (int)(str[j-diff+1] - '0');
						if (!strcmp(argv[1], "-p") || !strcmp(argv[2], "-p") || !strcmp(argv[1], "-t") || !strcmp(argv[2], "-t"))
							narg += 2;
						else narg += 1;
						int al = strlen(argv[narg]);
						sl += al-2;
						char* temp = calloc(sl+1, 1);
						int k;
						for (k = 0; k < j; k++)
							temp[k] = out[k];
						out = temp;
						strncpy(out+j, argv[narg], al+1);
						diff += al - 2;
						j+= al-1;
					}
					else
						out[j] = str[j-diff];
				out[sl] = 0;
				list[i] = out;
			}
		}
	}
	
	token* newtoken = malloc(sizeof(token));
	newtoken->words = list;
	newtoken->t = t;
	if (t == LEFT_PAREN_TOKEN)
		newtoken->sub = ++cur_shellnum;
	else if (t == RIGHT_PAREN_TOKEN)
		newtoken->sub = cur_shellnum--;
	else
		newtoken->sub = cur_shellnum;
	addToken(*start, *end, newtoken);
}

token* infixToPostfix(token* start)
{
	token* final_start = NULL;
	token* final_end = NULL;
	
	token* stack_start = NULL;
	token* stack_end = NULL;
	
	for (; start != NULL; start = start->next)
		if (start->t == WORD_TOKEN) //if operand, then add to final
			addToken(&final_start, &final_end, start);
		else if (start->t == LEFT_PAREN_TOKEN) //if left paren, then add it to stack
			addToken(&stack_start, &stack_end, start);
		else if (start->t == RIGHT_PAREN_TOKEN) //if right paren, pop stack until matching left paren
		{
			while (stack_end && stack_end->t != LEFT_PAREN_TOKEN)
			{
				token* output = popToken(&stack_start, &stack_end);
				addToken(&final_start, &final_end, output);
			}
			popToken(&stack_start, &stack_end);
		}
		else if (start->t == SEMICOLON_TOKEN && start->sub == 1)
		{
			while (stack_end && stack_end->t != LEFT_PAREN_TOKEN)
				addToken(&final_start, &final_end, popToken(&stack_start, &stack_end));
			token* semitoken = malloc(sizeof(token));
			semitoken->t = SEMICOLON_TOKEN;
			semitoken->sub = 1;
			addToken(&final_start, &final_end, semitoken);
		}
		else if (stack_end == NULL || precedence(start->t) > precedence(stack_end->t)) //if more important operator than top, then add to stack
			addToken(&stack_start, &stack_end, start);
		else if (precedence(start->t) < precedence(stack_end->t)) //if less important operator than top, then keep transferring until I'm more important and then add me to stack
		{
			token* output;
			while (stack_end != NULL)
			{
				if (precedence(start->t) > precedence(stack_end->t))
					break;
				addToken(&final_start, &final_end, popToken(&stack_start, &stack_end));
			}
			addToken(&stack_start, &stack_end, start);
		}
		else if (precedence(start->t) ==precedence(stack_end->t)) //if same, then pop stack and push me to stack
		{
			addToken(&final_start, &final_end, popToken(&stack_start, &stack_end));
			addToken(&stack_start, &stack_end, start);
		}
	while (stack_end)
		addToken(&final_start, &final_end, popToken(&stack_start, &stack_end));
	return final_start;
}

enum command_type tokenToCommandType(tokentype t)
{
	switch (t)
	{
		case PIPE_TOKEN:
			return PIPE_COMMAND;
		case AND_TOKEN:
			return AND_COMMAND;
		case OR_TOKEN:
			return OR_COMMAND;
		case SEMICOLON_TOKEN:
			return SEQUENCE_COMMAND;
		default:
			return SIMPLE_COMMAND;
	}
}

void addCommand(command** start, command** end, command* newcommand)
{
	if (*start == NULL && *end == NULL) //this is 1st element
	{
		*end = *start = newcommand;
		(*end)->prev = (*end)->next = NULL;
	}
	else
	{
		newcommand->next = NULL;
		newcommand->prev = *end;
		(*end)->next = newcommand;
		*end = newcommand;
	}
}

command* popCommand(command** start, command** end)
{
	if (!(*end)) return 0;
	command* output = *end;
	if (*start == *end)
		*start = *end = NULL;
	else
	{
		*end = (*end)->prev;
		(*end)->next = NULL;
	}
	return output;
}

void addStream(command_stream** start, command_stream** end, command_stream* newcommand)
{
	if (!(*start) && !(*end)) //this is 1st element
	{
		*end = *start = newcommand;
		(*end)->prev = (*end)->next = NULL;
	}
	else
	{
		newcommand->next = NULL;
		newcommand->prev = *end;
		(*end)->next = newcommand;
		*end = newcommand;
	}
}

command_stream* evaluatePostfix(token* finalTokenStream)
{
	/*
	notes on structures and types declared above
	typedef enum
	{
		WORD_TOKEN, //ls foo
		SEMICOLON_TOKEN, // ;
		PIPE_TOKEN, // |
		AND_TOKEN, // &&
		OR_TOKEN, // ||
		LEFT_PAREN_TOKEN, // ( //not relevant at this point
		RIGHT_PAREN_TOKEN, // )//not relevant at this point
		GREATER_TOKEN, // >
		LESS_TOKEN, // <
		NEWLINE_TOKEN, // \n //not relevant at this point
		NULL_TOKEN, // //not relevant at this point
	} tokentype;
	
	enum command_type
	{
		AND_COMMAND,         // A && B
		SEQUENCE_COMMAND,    // A ; B
		OR_COMMAND,          // A || B
		PIPE_COMMAND,        // A | B
		SIMPLE_COMMAND,      // a simple command
		SUBSHELL_COMMAND,    // ( A )
	};
	  
	typedef struct command_stream
	{
		struct command_stream* next;
		struct command_stream* prev;
		struct command* root;
		
	} command_stream;
	
	struct command
	{
		command* prev;
		command* next; //for linked list before tree conversion
		enum command_type type;
		// Exit status, or -1 if not known (e.g., because it has not exited yet).
		int status;
		// I/O redirections, or 0 if none.
		char *input;
		char *output;
		union
		{
			// for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
			struct command *command[2];
			// for SIMPLE_COMMAND:
			char **word;
			// for SUBSHELL_COMMAND:
			struct command *subshell_command;
		} u;
	};
	*/
	command* cmd_start = NULL;
	command* cmd_end = NULL;
	
	command_stream* str_start = NULL;
	command_stream* str_end = NULL;
	
	command_stream* firststream = malloc(sizeof(command_stream));
	addStream(&str_start, &str_end, firststream);
	
	token* curr;
	for (curr = finalTokenStream; curr != NULL; curr = curr->next)
		if (curr->t == WORD_TOKEN) //add simple command to command stack
		{
			command* newcommand = malloc(sizeof(command));
			newcommand->type = SIMPLE_COMMAND;
			newcommand->status = -1;
			newcommand->input = newcommand->output = 0;
			newcommand->u.word = curr->words;
			if (curr->prev && curr->prev->sub > 1 && curr->sub == 1) //if we just ended a subshell, then make what was there a subshell command on itself
			{
				command* prevcmd = malloc(sizeof(command));
				prevcmd->type = SUBSHELL_COMMAND;
				prevcmd->input = prevcmd->output = 0;
				prevcmd->status = -1;
				prevcmd->u.subshell_command = popCommand(&cmd_start, &cmd_end);
				addCommand(&cmd_start, &cmd_end, prevcmd);
			}
			
			addCommand(&cmd_start, &cmd_end, newcommand);
		}
		else if (curr->t == SEMICOLON_TOKEN && curr->sub == 1) //stop working on the current stream and instead make a new stream if no subshell, otherwise make a sequence command
		{
			if (curr->prev && curr->prev->sub > 1) //if we just ended a subshell, then make cmd_start a subshell command on itself
			{
				command* newcommand = malloc(sizeof(command));
				newcommand->type = SUBSHELL_COMMAND;
				newcommand->input = newcommand->output = 0;
				newcommand->status = -1;
				newcommand->u.subshell_command = cmd_start;
				cmd_start = newcommand;
			}
			str_end->root=cmd_start;
			cmd_start = cmd_end = NULL;
			command_stream* newstream = malloc(sizeof(command_stream));
			addStream(&str_start, &str_end, newstream);
		}
		else if (curr->t == LESS_TOKEN || curr->t == GREATER_TOKEN) //change the input based on what command you are redirecting
		{
			//remove two last commands and make them a subtree with correct depth level and add subtree back to command list
			command* dest = popCommand(&cmd_start, &cmd_end); //where to redirect
			command* operation = popCommand(&cmd_start, &cmd_end); //what to do, assume always simple command
			
			command* newcommand = malloc(sizeof(command));
			newcommand->type = SIMPLE_COMMAND;
			newcommand->status = -1;
			if (curr->t == LESS_TOKEN)
			{
				newcommand->input = concat(dest->u.word[0], dest->u.word[1]);
				newcommand->output= operation->output;
			}
			else
			{
				newcommand->output= concat(dest->u.word[0], dest->u.word[1]);
				newcommand->input = operation->input;
			}
			newcommand->u.word = operation->u.word;
			addCommand(&cmd_start, &cmd_end, newcommand);
		}
		else //an operation that needs to be made into a tree
		{
			command* operand2 = popCommand(&cmd_start, &cmd_end), *operand1 = popCommand(&cmd_start, &cmd_end);
			
			if ((curr->prev && curr->prev->sub > 1 && curr->sub == 1) || operand2->type == SEQUENCE_COMMAND) //if we just ended a subshell, then make operand2 a subshell command on itself
			{
				command* newcommand = malloc(sizeof(command));
				newcommand->type = SUBSHELL_COMMAND;
				newcommand->input = newcommand->output = 0;
				newcommand->status = -1;
				newcommand->u.subshell_command = operand2;
				operand2 = newcommand;
			}
			if (operand1->type == SEQUENCE_COMMAND) //convert operand1 into subshell if it was a sequence, as sequences are only possible in subshells
			{
				command* newcommand = malloc(sizeof(command));
				newcommand->type = SUBSHELL_COMMAND;
				newcommand->input = newcommand->output = 0;
				newcommand->status = -1;
				newcommand->u.subshell_command = operand1;
				operand1 = newcommand;
			}
			command* newcommand = malloc(sizeof(command));
			newcommand->type = tokenToCommandType(curr->t);
			newcommand->status = -1;
			newcommand->input = newcommand->output = 0;
			newcommand->u.command[0] = operand1;
			newcommand->u.command[1] = operand2;
			addCommand(&cmd_start, &cmd_end, newcommand);
		}
	
	str_end->root=cmd_start;
	
	return str_start;
}

/*bool parenmatch(char* buf)
{
	int i, n = strlen(buf), left=0, right=0;
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == '(')
			left++;
		else if (buf[i] == ')')
			right++;
	return (left==right);
}*/

/*bool redirOrderValid(char* buf)
{
	buf = spaceclean(buf);
	int i, n=strlen(buf);
	
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == '<' || buf[i] == '>')
			if (i == 0 || i == n-1)
				return false;
	return true;
}*/

/*bool goodNewline(char* buf)
{
	buf = spaceclean(buf);
	int i, n=strlen(buf), k;
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == '\n')
			for (k = i; k < n; k++)
				if (buf[k] == ' ' || buf[k] == '\t')
					continue;
				else if (buf[k] == ';')
					return false;
				else
					break;
	return true;
}*/

/*bool goodSemicolon(char* buf)
{
	int i, n=strlen(buf);
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == ';' && (i == 0 || buf[i-1]==';'))
			return false;
	return true;
}*/

/*bool goodAndor(char* buf)
{
	buf = spaceclean(buf);
	int i, n=strlen(buf);
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == '&')
		{
			if (i == 0 || i+1 >= n || buf[i+1] != '&' || i+2 >= n || buf[i+2] == '&')
				return false;
			i++;
		}
		else if (buf[i] == '|')
		{
			if (i == 0 || i+1 >= n || buf[i-1] == '\n' || i+2 >= n || buf[i+2] == '|')
				return false;
		}
	return true;
}*/

bool basicSyntax(char* buf)
{
	buf = spaceclean(buf);
	int i, n = strlen(buf), left=0, right=0, k;
	bool cmnt = false;
	for (i = 0; i < n; i++)
		if (cmnt && buf[i] == '\n')
			cmnt = false;
		else if (cmnt)
			continue;
		else if (buf[i] == '#')
			cmnt = true;
		else if (buf[i] == '(')
			left++;
		else if (buf[i] == ')')
			right++;
		else if (buf[i] == '<' || buf[i] == '>')
		{
			if (i == 0 || i == n-1)
				return false;
		}
		else if (buf[i] == '\n')
			for (k = i; k < n; k++)
				if (buf[k] == ' ' || buf[k] == '\t')
					continue;
				else if (buf[k] == ';')
					return false;
				else
					break;
		else if (buf[i] == ';' && (i == 0 || buf[i-1]==';'))
			return false;
		else if (buf[i] == '&')
		{
			if (i == 0 || i+1 >= n || buf[i+1] != '&' || i+2 >= n || buf[i+2] == '&')
				return false;
			i++;
		}
		else if (buf[i] == '|')
		{
			if (i == 0 || i+1 >= n || buf[i-1] == '\n' || (buf[i+1] == '|' && (i+2 >= n || buf[i+2] == '|')))
				return false;
		}
	
	return (left==right);
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument, int margc, char** margv)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
//  error (1, 0, "command reading not yet implemented");
//  return 0;
	
	//get all input chars to buf	
	char* buf = malloc(65536);
	int size = 65536;
	int end_buf = 0;
	int i;
	int val; 
	for (; (val = get_next_byte(get_next_byte_argument)) != EOF; )
	{
		char value = (char) val;	
		if (end_buf >= size)
		{
			char* t_buf = malloc(size *= 2);
			copy(buf, t_buf, size);
			free(buf);
			buf = t_buf;
		}
		buf[end_buf++] = value;
	}
	buf[end_buf++] = ' ';
	
	//design project: set argc and argv
	argc = margc;
	argv = margv;
	
	//handle errors
	if (!basicSyntax(buf))
		error(1, 0, "basic syntax error!");
	
	//tokenize buffer
	tokentype t = NULL_TOKEN;
	char* tokchars = malloc(128);
	int cwp = 0; //current word pointer
	bool commenting; //true or false: are we commenting?
	token* start, *end; //represents linked list of tokens
	token** andstart 	= &start;
	token** andend 		= &end;
	start = end = NULL;

	for (i = 0; i < end_buf;)
	{
		char c = buf[i];
		if (c == '#')
		{
			if (t != NULL_TOKEN)
				addToken_make(&andstart, &andend, t, tokchars, cwp, 0);
			t = NULL_TOKEN;
			free(tokchars);
			tokchars = malloc(128);
			cwp = 0;
			commenting = 1;
			i++;
		}
		else if (c == '\n' && commenting)
		{
			commenting = 0;
			i++;
		}
		else if (commenting)
		{
			i++;
			continue;
		}
		else if (c == '\r')
			continue;
		else if (c == '&')
		{
			if (t != NULL_TOKEN)
				addToken_make(&andstart, &andend, t, tokchars, cwp, 0);
			t = NULL_TOKEN;
			token* andtoken = malloc(sizeof(token));
			andtoken->t = AND_TOKEN;
			andtoken->sub = cur_shellnum;
			addToken(&start, &end, andtoken);
			t = NULL_TOKEN;
			i += 2;
			free(tokchars);
			tokchars = malloc(128);
			cwp = 0;
		}
		else if (c == '|')
		{
			if (t != NULL_TOKEN)
				addToken_make(&andstart, &andend, t, tokchars, cwp, 0);
			t = NULL_TOKEN;
			if (buf[i+1] == '|')
			{
				token* ortoken = malloc(sizeof(token));
				ortoken->t = OR_TOKEN;
				ortoken->sub = cur_shellnum;
				addToken(&start, &end, ortoken);
				t = NULL_TOKEN;
				i += 2;
			}
			else
			{
				token* pipetoken = malloc(sizeof(token));
				pipetoken->t = PIPE_TOKEN;
				pipetoken->sub = cur_shellnum;
				addToken(&start, &end, pipetoken);
				t = NULL_TOKEN;
				i += 1;
			}
			free(tokchars);
			tokchars = malloc(128);
			cwp = 0;
		}		
		else if (c == '(' || c == ')' || c == '<' || c == '>' || c == ';' || c == '\n')
		{
			if (t != NULL_TOKEN)
				addToken_make(&andstart, &andend, t, tokchars, cwp, 0);
			t = NULL_TOKEN;
			addToken_make(&andstart, &andend, tokentypeOf(c), 0, 0, (c == '(' || c == ')'));
			free(tokchars);
			tokchars = malloc(128);
			cwp = 0;
			i++;
		}
		else if (c == ' ' || c == '\t' || !c)
		{
			if (t == WORD_TOKEN)
				tokchars[cwp++] = c;
			else
			{
				free(tokchars);
				tokchars = malloc(128);
				cwp = 0;
				t = NULL_TOKEN;
			}
			i++;
		}
		else if (c == '!' || c == '%' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/' || c == ':' || c == '@' || c == '^' || c == '_' || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') || ('0' <= c && c <= '9') || c == '$')
		{
			//design project: $ sign is part of a word
			t = WORD_TOKEN;
			tokchars[cwp++] = c;
			i++;
		}
		else
		{
			int xx = 0;
			error (1, 0, "unrecognized character");
		}
	}
	if (t != NULL_TOKEN)
		addToken_make(&andstart, &andend, t, tokchars, cwp, 0);
	
	//clean token stream first
	token* clean;
	token** andclean = &clean;
	for (clean = start; clean; clean = clean->next) //start by removing adjacent newlines and checking syntax of semicolon, pipe, and, and or
		if (clean->t == NEWLINE_TOKEN)
			while (clean->next && clean->next->t == NEWLINE_TOKEN)
			{	
				clean->next = clean->next->next;
				if (clean->next) clean->next->prev = clean;
			}
		else if (clean->t == SEMICOLON_TOKEN || clean->t == PIPE_TOKEN || clean->t == AND_TOKEN || clean->t == OR_TOKEN)
		{
			if (!clean->prev || clean->prev->t == NEWLINE_TOKEN)
				error (1, 0, "illegal semicolon or operator syntax");
		}
	for (clean = start; clean; clean = clean->next) //then check operator syntax once more
		if (clean->t == AND_TOKEN || clean->t == OR_TOKEN || clean->t == PIPE_TOKEN)
		{
			if (!clean->next || clean->next->t == SEMICOLON_TOKEN || !clean->prev || clean->prev->t == SEMICOLON_TOKEN)
				error (1, 0, "illegal operator syntax");
			if (clean->next->t == NEWLINE_TOKEN)
			{
				token* temp = clean->next;
				for (; temp && temp->t == NEWLINE_TOKEN; temp = temp->next);
				if (!temp || temp->t == SEMICOLON_TOKEN)
					error (1, 0, "illegal operator syntax");
			}
		}
		else if (clean->t == LESS_TOKEN || clean->t == GREATER_TOKEN)
			if (!clean->prev || !clean->next || clean->prev->t != WORD_TOKEN || clean->next->t != WORD_TOKEN)
				error(1, 0, "bad redirection!");
			
	for (clean = start; clean; clean = clean->next) //then get rid of all newlines
	{
		if (clean->t == NEWLINE_TOKEN)
		{
			if ((clean->prev != NULL && clean->prev->t != WORD_TOKEN && clean->prev->t != RIGHT_PAREN_TOKEN) || (clean->next != NULL && clean->next->t != WORD_TOKEN && clean->next->t != LEFT_PAREN_TOKEN)) //function as whitespace
			{
				token* p = clean->prev;
				token* n = clean->next;
				p->next = n;
				if (n) n->prev = p;
				continue;
			}
			else //function as semicolon
				clean->t = SEMICOLON_TOKEN;
		}
		if (clean->t == SEMICOLON_TOKEN)
			while (clean->next && clean->next->t == SEMICOLON_TOKEN)
			{	
				clean->next = clean->next->next;
				if (clean->next) clean->next->prev = clean;
			}
	}
			
	if (start && start->t == SEMICOLON_TOKEN)
	{
		start = start->next;
		start->prev = 0;
	}
	
	
	//change token stream from infix to postfix
	token* finalTokenStream = infixToPostfix(start);
	
	command_stream* strm = evaluatePostfix(finalTokenStream);
	
	return strm;
}

command_stream_t* streamArrayFromList(command_stream* strm, int* lengthOut)
{
	command_stream* cs;
	int i;
	*lengthOut = 0;
	for (cs = strm; (cs = cs->next);)
		(*lengthOut)++;
	command_stream** array = malloc((*lengthOut) * sizeof(command_stream*));
	for (i = 0, cs = strm; i < *lengthOut && cs; i++, cs=cs->next) 
		array[i] = cs;
	return array;
}

char** getInputs(command_t cmd)
{
	int i = 0, j = 0;
	char** inp = calloc(64, sizeof(char*));
	char** left;
	char** right;
	switch (cmd->type)
	{
		case AND_COMMAND:
		case SEQUENCE_COMMAND:
		case OR_COMMAND:
		case PIPE_COMMAND:
			left = getInputs(cmd->u.command[0]);
			right= getInputs(cmd->u.command[1]);
			for (; left[i]; i++)
				inp[i] = left[i];
			for (; right[j]; j++)
				inp[i+j] = right[j];
			break;
		case SIMPLE_COMMAND:
		case SUBSHELL_COMMAND:
			for (i = 1; cmd->u.word[i]; i++) //i to navigate thru all args
				if (cmd->u.word[i][0] != '-')
					inp[j++] = cmd->u.word[i]; //j to set actual inputs
			if (cmd->input)
				inp[j] = cmd->input;
			break;
	}
	return inp;
}

char** getInputs_pub(command_stream_t strm)
{
	return getInputs(strm->root);
}

char** getOutputs(command_t cmd)
{
	int i = 0, j = 0;
	char** out = calloc(64, sizeof(char*));
	char** left;
	char** right;
	switch (cmd->type)
	{
		case AND_COMMAND:
		case SEQUENCE_COMMAND:
		case OR_COMMAND:
		case PIPE_COMMAND:
			left = getOutputs(cmd->u.command[0]);
			right= getOutputs(cmd->u.command[1]);
			for (; left[i]; i++)
				out[i] = left[i];
			for (; right[j]; j++)
				out[i+j] = right[j];
			break;
		case SIMPLE_COMMAND:
		case SUBSHELL_COMMAND:
			if (cmd->output)
				out[0] = cmd->output;
	}
	return out;
}

char** getOutputs_pub(command_stream_t strm)
{
	return getOutputs(strm->root);
}

command_t getCmd(command_stream_t strm)
{
	return strm->root;
}

bool done = false;
command_t
read_command_stream (command_stream_t s) //this does not correct the previous, only next
{
  /* FIXME: Replace this with your implementation too.  */
//  error (1, 0, "command reading not yet implemented");
//  return 0;
	if (done) return 0;
	if (!s) return 0;
	command_t out = s->root;
	
	if (s->next && out)
	{
		s->root = s->next->root;
		s->next = s->next->next;
	}
	else
		done = true;
	return out;
}
