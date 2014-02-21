//wrong code
command* c1, *c2;

if (fork() == 0)
	evaluate_command(c1);
else
	evaluate_command(c2);
---------------
//correct code, evaluate all commands in com in parallel
command* com[10];

for (int i = 0; i < 10; i++)
	if (fork() == 0)
		evaluate_command(com[i]);
---------------
struct command_node
{
//remember ignore next and prev of stream

command_stream* str;
command_node** next;
command_node** prev;
}

