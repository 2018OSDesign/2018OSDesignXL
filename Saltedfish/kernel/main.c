
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "keyboard.h"

#define MAX_FILES       80
#define MAX_DIRS        50

char location[MAX_FILENAME_LEN]="root";
char files[MAX_FILES][MAX_FILENAME_LEN];
int filequeue[MAX_FILES];
int filecount=0;
char dirs[MAX_DIRS][MAX_FILENAME_LEN];
int dirqueue[MAX_FILES];
int dircount=0;
int b_c_priority[2] = {1,1};

void ProcessManage();
void initFS();

void ClearWithHelp();
void WelcomeLoad();
void information();
void welcome();

int isDir(const char * filepath);

void getFilepath(char *filepath, char * filename);
void getDirFilepath(char *filepath, char * filename);
void getDirpathAndFilename(char * dirpath, char * filename, char * filepath);

int getFreeFilePos();
int getFreeDirPos();
int getPosInDirQueue(char * filepath);


void addFileIntoDir(const char * dirpath, char * filename);
void deleteFileFromDir(const char * dirpath, char * filename);

void createFile(char * filepath, char *filename, char * buf);
void createDir(char * filepath, char *filename);
void readFile(char * filename);
void editAppand(const char * filepath, char * str);
void editCover(const char * filepath, char * str);
void deleteFile(char * filepath);
void deleteDir(char * filepath);
void ls();
void cd(char * dirname);
void cdback();

/*======================================================================*
                            kernel_main
 *======================================================================*/
int selectShowProcess = 0;
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	struct task* p_task;
	struct proc* p_proc= proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
    u8    privilege;
    u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;
	for (i = 0; i < NR_TASKS+NR_PROCS; i++) {
	        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
			prio      = 15;
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
			prio      = 5;
                }

		strcpy(p_proc->name, p_task->name);	/* name of the process */
		p_proc->pid = i;			/* pid */

		p_proc->run_count = 0;

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		/* p_proc->nr_tty		= 0; */

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;
		p_proc->run_state = 1;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}
	proc_table[5].run_state = 0;
	proc_table[6].run_state = 0;

        /* proc_table[NR_TASKS + 0].nr_tty = 0; */
        /* proc_table[NR_TASKS + 1].nr_tty = 1; */
        /* proc_table[NR_TASKS + 2].nr_tty = 1; */

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	init_clock();
    init_keyboard();

	restart();


	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/*======================================================================*
                               Guess Number
 *======================================================================*/
unsigned int seed = 0x49827316;

void shift_srand(unsigned int new_seed){
	seed = new_seed;
}

int shift_rand(){
	seed = (seed<<7) + (seed>>7);
	return (seed % 999 + 1);
}

int chartoint(const char *s){
	int num, i;
	char ch;
	num = 0;
	for(i = 0; i < 3 ; i++){
		ch = s[i];
		if(ch < '0' || ch > '9')
			break;
		num = num * 10 + (ch - '0');
	}
	return num;
}
void GuessNumber(){
	printf("              ***************************************************\n");
	printf("              *               Guess number                      *\n");
	printf("              ***************************************************\n");
	printf("              *                                                 *\n");
	printf("              *              Enter q to quit                    *\n");
	printf("              *                                                 *\n");
	printf("              ***************************************************\n\n");
	int stop = 0;
	int answer_num, guess_num;
	answer_num = shift_rand();
	printf("I have a number between 1 and 999.\nCan you guess it?\nShow me the answer:");
	char in_put[128];
	read(0, in_put, 128);
	guess_num = chartoint(in_put);
	while(guess_num != -1){
		if(guess_num == answer_num){
			printf("Excellent! You guessed the number!\nWould you like to play again?(y/n)  ");
			char temp[128];
			read(0,temp,128);
			switch(temp[0]){
				case 'y':printf("I have a number between 1 and 999.\nCan you guess it?\nShow me the answer:");
					 read(0, in_put, 128);
					 guess_num = chartoint(in_put);
					 answer_num = shift_rand();
					 break;
				case 'n':stop = 1;
					 break;
			}
			if(stop == 1)
				break;
		}
		while(guess_num < answer_num){
			printf("My number is bigger.Try again: ");
			read(0, in_put, 128);
			guess_num = chartoint(in_put);
			if(in_put[0] == 'q')
			break;
		}
		while(guess_num > answer_num){
			printf("My number is smaller.Try again: ");
			read(0, in_put, 128);
			guess_num = chartoint(in_put);
			if(in_put[0] == 'q')
			break;
		}
		if(in_put[0] == 'q')
			break;
	}
	
}


/*======================================================================*
                               Caculator
 *======================================================================*/
#define chartonumber(x) (x-'0')
void Caculator(){
	printf("**************************************************************\n");
	printf("*                         calculator                         *\n");
	printf("**************************************************************\n");
	printf("*                                                            *\n");
	printf("*                      Enter q to quit                       *\n");
	printf("*                                                            *\n");
	printf("**************************************************************\n");
	
	while(1){
		int result;

		char bufr[128];
		read(0, bufr, 128);
		int i = 0;
		int first_number = 0,second_number = 0;
		while(bufr[i] >= '0' && bufr[i] <= '9'){
			i++;
		}
		for(int j = 0; j < i;j++){
			first_number = first_number * 10 + (bufr[j] - '0');
		}
		for(int m = i + 1; bufr[m] <= '9' && bufr[m] >= '0';m++){
			second_number = second_number * 10 + (bufr[m] - '0');
		}

		switch(bufr[i])
	
		{

    			case '+':result=first_number + second_number;break;

			case '-':result=first_number - second_number;break;

			case '*':result=first_number * second_number;break;

			case '/':result=first_number / second_number;break;
	

		}
		if(bufr[0] == 'q')
			break;
		printf("%d %c %d = %d", first_number, bufr[i], second_number,result);
	}
	
}


/*======================================================================*
                               PushBox
 *======================================================================*/

int map[12][9] = {
	{0,0,1,1,1,1,1,0,0},
	{0,0,1,0,4,0,1,0,0},
	{1,1,1,3,0,3,1,1,1},
	{1,0,0,3,0,3,0,0,1},
	{1,0,4,0,4,0,4,0,1},
	{1,1,1,0,7,0,1,1,1},
	{0,0,1,0,7,0,1,0,0},
	{0,0,1,3,4,3,1,0,0},
	{0,0,1,0,4,0,1,0,0},
	{0,0,1,3,4,3,1,0,0},
	{0,0,1,0,2,0,1,0,0},
	{0,0,1,1,1,1,1,0,0}
};

int win(){
	int k = 0,i = 0, j = 0;
	for(i = 0; i < 12; i++){
		for(j = 0; j < 9; j++){
			if(map[i][j] == 3){
				k++;
			}
		}
	}
	if(k == 0){
		return 1;
	}
	return 0;
}

int draw_map(){
	int m = 0, n = 0;
	for(m = 0; m < 12; m++){
		printf("          ");
		for(n = 0; n < 9; n++){
			switch(map[m][n]){
				case 0:
					printf("  ");
					break;
				case 1:
					printf(" #");
					break;
				case 2:
					printf(" ^");
					break;
				case 3:
					printf(" $");
					break;
				case 4:
					printf(" @");
					break;
				case 7:
					printf(" *");
					break;
				case 6:
					printf(" &");
					break;
			}
		}
		printf("\n");
	}
	printf("\n");
	return 0;
}

int push(char act){
	int line, row;
	int i = 0, j = 0;
	for(i =0; i < 12;i++){
		for(j = 0;j < 9;j++){
			if(map[i][j] == 2||map[i][j] == 6){
				line = i;
				row = j;
			}
		}
	}
	switch(act){
		case 'w':
			if(map[line-1][row] == 0||map[line-1][row] == 4){
				map[line-1][row] += 2;
				map[line][row] -= 2;
			}
			else if(map[line-1][row] == 3||map[line-1][row] == 7){
				if(map[line-2][row] == 0|| map[line-2][row] == 4){
					map[line-1][row] -= 1;
					map[line][row] -= 2;
					map[line-2][row] += 3;
				}
			}
			break;
		case 's':
			if(map[line+1][row] == 0 || map[line+1][row] == 4){
				map[line+1][row] += 2;
				map[line][row] -= 2;
			}
			else if(map[line+1][row] == 3 || map[line+1][row] == 7){
				if(map[line+2][row] == 0 || map[line+2][row] == 4){
					map[line-1][row] -= 1;
					map[line][row] -= 2;
					map[line-2][row] += 3;
				}
			}
			break;
		case 'a':
			if(map[line][row-1] == 0 || map[line][row-1] == 4){
				map[line][row-1] += 2;
				map[line][row] -= 2;
			}
			else if(map[line][row-1] == 3 || map[line][row-1] == 7){
				if(map[line][row-2] == 0 || map[line][row-2] == 4){
					map[line][row-2] += 3;
					map[line][row-1] -= 1;
					map[line][row] -= 2;
				}
			}
			break;
		case 'd':
			if(map[line][row+1] == 0 || map[line][row+1] == 4){
				map[line][row+1] += 2;
				map[line][row] -= 2;
			}
			else if(map[line][row+1] == 3 || map[line][row+1] == 7){
				if(map[line][row+2] == 0 || map[line][row+2] == 4){
					map[line][row+2] += 3;
					map[line][row+1] -= 1;
					map[line][row] -= 2;
				}
			}
			break;
	}
}

void push_act(){
	
	while(1){
		clear();
		printf("**************************************************************\n");
		printf("*                         Push Box                           *\n");
		printf("**************************************************************\n");
		printf("*                                                            *\n");
		printf("*                      ^ means person                        *\n");
		printf("*                      $ means box                           *\n");
		printf("*                      @ means destination                   *\n");
		printf("*                      # means wall                          *\n");
		printf("*                      * means box + destination             *\n");
		printf("*                      & means person + destination          *\n");
		printf("*                      Enter q to quit                       *\n");
		printf("*                                                            *\n");
		printf("**************************************************************\n");
		draw_map();
		int tag = win();
		printf("Use w,s,a,d to control(^ person $ box @ destination): ");
		char in_put[128];
		int r = read(0,in_put,128);
		if(in_put[0] == 'q'){
			break;
		}
		if(tag){
			break;
		}
		push(in_put[0]);
	}
}

/*======================================================================*
                               Pick Sticks
 *======================================================================*/

void PickSticks(){
	int total = 0;
	printf("**************************************************************\n");
	printf("*                         Pick Sticks                        *\n");
	printf("**************************************************************\n");
	printf("*                                                            *\n");
	printf("*                       Enter q to quit                      *\n");
	printf("*                                                            *\n");
	printf("**************************************************************\n");
	while(1){
		printf("Decide the number of the sticks(10 < x < 100): ");
		char in_put[128];
		in_put[0] = '0';
		in_put[1] = '0';
		read(0,in_put,128);
		total = (in_put[0] - '0') * 10 + (in_put[1] - '0');
		if(in_put[0] == 'q'){
			break;
		}
		while(in_put[1] == '\n'){
			printf("Please enter the right number(10 < x < 100): ");
			read(0,in_put,128);
			total = (in_put[0] - '0') * 10 + (in_put[1] - '0');
		}
		printf("Do you want to take first?(y/n): ");
		read(0,in_put,128);
		int temp = 4,temp_r = 3;
		if(in_put[0] == 'y'){
			while(total > 0){
				printf("--------------- now %d sticks left ---------------\n",total);
				printf("You: ");
				read(0,in_put,128);
				temp = in_put[0] - '0';
				while(temp < 1||temp > 4){
					printf("You(1~4): ");
					read(0,in_put,128);
					temp = in_put[0] - '0';
				}
				total -= temp;
				if(total <= 0){
					printf("You lose!\n");
					break;
				}
				temp_r = 5 - temp;
				total -= temp_r;
				if(total <= 0){
					if(temp_r == 1){
						printf("You win!\n");
						break;
					}
					else{
						if((total + temp_r - 1) <= 0){
							printf("You win!\n");
							break;
						}
						else
							temp_r -= (1 - total);
							total = 1;
					}
				}
				printf("Robot: %d\n",temp_r );
			}
		}
		else{
			while(total > 0){
				printf("--------------- now %d sticks left ---------------\n",total);
				temp_r = 5 - temp;
				total -= temp_r;
				if(total <= 0){
					if(temp_r == 1){
						printf("You win!\n");
						break;
					}
					else{
						if((total + temp_r - 1) <= 0){
							printf("You win!\n");
							break;
						}
						else
							temp_r -= (1 - total);
							total = 1;
					}
				}
				printf("Robot: %d\n",temp_r);
				printf("You: ");
				read(0,in_put,2);
				temp = in_put[0] - '0';
				while(temp < 1||temp > 4){
					printf("You(1~4): ");
					read(0,in_put,128);
					temp = in_put[0] - '0';
				}
				total -= temp;
				if(total <= 0){
					printf("You lose!\n");
					break;
				}

			}
		}
		printf("Do you want to play again?(y/n): ");
		read(0,in_put,128);
		if(in_put[0] == 'n'){
			break;
		}
	}
}

/*======================================================================*
                               Mines Weeper
 *======================================================================*/
int mine_rand(){
	seed = (seed<<7) + (seed>>7);
	return (seed % 64);
}

void print_mapshow(char *s){
	for(int i = 0;i < 64; i++){
		if(i % 8 == 0){
			printf("\n");
		}
		printf("%c |",s[i]);
	}
	printf("\n");
}

void print_mine(int *r){
	for(int i = 0;i < 64; i++){
		if(i % 8 == 0){
			printf("\n");
		}
		printf("%d |",r[i]);
	}
	printf("\n");
}

void mine_weeper(){
	char map_show[64];//show to user
	int map_mine[64];//mine map
	while(1){
		for(int i = 0;i < 64;i++){
			map_show[i] = '*';
			map_mine[i] = 0;
		}
		for(int i = 0;i < 12;i++){
			map_mine[mine_rand()] = 1;
		}
		int mine_num = 0;
		for(int i = 0; i < 64;i++){
			if(map_mine[i] == 1)
				mine_num++;
		}
		int pos[8][2] = {{-1,-1} ,{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
		int mine_around = 0;
		char line_in[128], row_in[128];//input
		int line = 0,row = 0;
		int tag = 0;//map sig
		int win_tag = 0;
		while(1){
			clear();
			printf("**************************************************************\n");
			printf("*                         Mines Weeper                       *\n");
			printf("**************************************************************\n");
			printf("*                                                            *\n");
			printf("*                       Enter q to quit                      *\n");
			printf("*                                                            *\n");
			printf("**************************************************************\n");
			print_mapshow(map_show);
			printf("Enter the row: ");
			read(0,row_in,128);
			if(row_in[0] == 'q'){
				break;
			}
			printf("Enter thr line: ");
			read(0,line_in,128);
			if(line_in[0] == 'q'){
				break;
			}
			line = line_in[0] - '0';
			row = row_in[0] - '0';
			tag = (line - 1) * 8 -1 + row;
			if(map_mine[tag] == 1){
				map_mine[tag] = 9;
				print_mine(map_mine);
				printf("Boom!! You failed!\n");
				break;
			}
			else{
				mine_around = 0;
				int temp = 0;
				while(temp < 8){
					int temp_row = row;
					int temp_line = line;
					temp_row += pos[temp][0];
					temp_line += pos[temp][1];
					if(temp_row > 0 && temp_row <=8 && temp_line >0 && temp_line <= 8){
						tag = (temp_line - 1) * 8 - 1 + temp_row;
						if(map_mine[tag] == 1){
							mine_around++;
						}
					}
					temp++;
				}
				tag = (line - 1) * 8 -1 + row;
				switch(mine_around){
					case 0:  
						map_show[tag]='0';  
						break;  
              				case 1:  
                 				map_show[tag]='1';  
                       				break;  
                			case 2:  
                    				map_show[tag]='2';  
                       				break;  
                			case 3:  
                    				map_show[tag]='3';  
                       				break;  
                			case 4:  
                    				map_show[tag]='4';  
                       				break;  
                			case 5:  
                    				map_show[tag]='5';  
                       				break;  
                			case 6:  
                    				map_show[tag]='6';  
                       				break;  
                			case 7:  
                    				map_show[tag]='7';  
                       				break;  
                			case 8:  
                    				map_show[tag]='8';  
                       				break; 
				}
				win_tag++;
				if(win_tag == 64 - mine_num){
					printf("Congratulations! You win!\n");
					break;
				}
			}	
		}
		printf("Do you want to play again?(y/n): ");
		char choise[128];
		read(0,choise,128);
		while(choise[0] != 'y' && choise[0] != 'n'){
			printf("Please enter the correct order(y/n): ");
			read(0,choise,128);
		}
		if(choise[0] == 'y')
			continue;
		else if(choise[0] == 'n')
			break;

	}
}


void Appchoose(){
	while(1){
		printf("              ***************************************************\n");
		printf("              *                      App                        *\n");
		printf("              ***************************************************\n");
		printf("              *              Please choos the app               *\n");
		printf("              *                   1.Guess number                *\n");
		printf("              *                   2.calculator                  *\n");
		printf("              *                   3.Push Box                    *\n");
		printf("              *                   4.Pick Sticks                 *\n");
		printf("              *                   5.Mines Weeper                *\n");
		printf("              *                 Enter q to quit                 *\n");
		printf("              ***************************************************\n\n");
		char choise[128];
		read(0,choise,128);
		if(strcmp(choise, "1") == 0){
			GuessNumber();
			continue;
		}
		else if(strcmp(choise, "2") == 0){
			Caculator();
			continue;
		}
		else if(strcmp(choise, "3") == 0){
			push_act();
			continue;
		}
		else if(strcmp(choise, "4") == 0){
			PickSticks();
			continue;
		}
		else if(strcmp(choise, "5") == 0){
			mine_weeper();
			continue;
		}
		else if(strcmp(choise, "q") == 0){
			break;
		}
	}
}

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	
	int i, n;

	char tty_name[] = "/dev_tty0";

	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[128];
	char arg1[MAX_FILENAME_LEN];
	char arg2[MAX_FILENAME_LEN];
	char filepath[MAX_FILENAME_LEN];



	WelcomeLoad();
//	char filename[MAX_FILENAME_LEN+1] = "zsp01";
	const char bufw[80] = {0};
//	const int rd_bytes = 3;
//	char bufr[rd_bytes];

	initFS();
	ClearWithHelp();

	while (1) {
		memset(rdbuf,0,128);
		memset(cmd,0,128);
		memset(arg1,0,MAX_FILENAME_LEN);
		memset(arg2,0,MAX_FILENAME_LEN);

		printf("%s $",location);
		int r = read(0, rdbuf, 70);
		rdbuf[r] = 0;

		int argc=0;
		char * argv[PROC_ORIGIN_STACK];
		char * p=rdbuf;
		char * s;
		int word = 0;
		char ch;
		do{
			ch=*p;
			if(*p != ' ' && *p != 0 && !word){
				s = p;
				word = 1;
			}
			if((*p == ' ' || *p ==0)&& word){
				word = 0;
				argv[argc++] = s;
				*p = 0;
			}
			p++;
		}while(ch);
		argv[argc] = 0;
		
		int fd = open(argv[0],O_RDWR);
		if(fd == -1){
			if(rdbuf[0]){
				int i = 0,j = 0;
				// get command
				while(rdbuf[i] != ' ' && rdbuf[i] != 0)
				{
					cmd[i]=rdbuf[i];
					i++;
				}
				i++;
				// get arg1
				while(rdbuf[i] != ' ' && rdbuf[i] != 0)
				{
					arg1[j]=rdbuf[i];
					i++;
					j++;
				}
				i++;
				j=0;
				// get arg2
				while(rdbuf[i] != ' ' && rdbuf[i] != 0)
				{
					arg2[j]=rdbuf[i];
					i++;
					j++;
				}
				// w e l c o m e
				if(strcmp(cmd,"welcome")==0)
				{	
					output();
					
				}
				//c l e a r
				else if(strcmp(cmd,"clear")==0)
				{
					clear();
					output();
					
				}
				//app
				else if(strcmp(cmd,"app")==0)
				{
					Appchoose();
					continue;
				}
 				// show process
				else if(strcmp(cmd,"proc")==0)
				{
					ProcessManage();
				}
				//kill a process
				else if(strcmp(cmd,"kill")==0)
				{
				}
				//make a process
				else if(strcmp(cmd,"mkpro")==0)
				{
				}
				//show help message
				else if(strcmp(cmd,"help")==0)
				{
					help();
				}
				else if(strcmp(cmd,"mkfile")==0)
				{	
					if(arg1[0] == '#')
					{
						printf("Irregular filename");
						continue;
					}
					strcpy(filepath,location);
					getFilepath(filepath,arg1);
					printf("%s %s\n",arg1,arg2);
					createFile(filepath,arg1,arg2);
					memset(filepath,0,MAX_FILENAME_LEN);
				}
				// create a dir
				else if(strcmp(cmd,"mkdir")==0)
				{
					if(arg1[0] == '#')
					{
						printf("Irregular filename");
						continue;
					}
					strcpy(filepath, location);
					getDirFilepath(filepath, arg1);
					createDir(filepath, arg1);
					memset(filepath, 0, MAX_FILENAME_LEN);
				}
				//read a file
				else if(strcmp(cmd,"read") == 0)
				{
					if(arg1[0]=='#')
					{
						printf("Irregular filename");
						continue;
					}
					readFile(arg1);
					memset(filepath, 0, MAX_FILENAME_LEN);
					
				}
				//edit a file cover
				else if(strcmp(cmd,"edit")==0)
				{
					if(arg1[0] == '#')
					{
						printf("Irregular filename");
						continue;
					}
					strcpy(filepath, location);
					getFilepath(filepath, arg1);
					editCover(filepath, arg2);
					memset(filepath, 0, MAX_FILENAME_LEN);
				}
				//edit a file appand
				else if(strcmp(cmd,"edit+")==0)
				{
					if(arg1[0]=='#')
					{
						printf("Irregular filename");
						continue;
					}
					strcpy(filepath, location);
					getFilepath(filepath, arg1);
					editAppand(filepath, arg2);
					memset(filepath, 0, MAX_FILENAME_LEN);
				}
				//delete a file
				else if(strcmp(cmd,"delete")==0)
				{
					if(arg1[0]=='#')
					{
						printf("Irregular filename!");
						continue;
					}
					strcpy(filepath,location);
					getFilepath(filepath,arg1);
					deleteFile(filepath);
					memset(filepath,0,MAX_FILENAME_LEN);
				}
				//delete a dir
				else if(strcmp(cmd,"deletedir")==0)
				{
					if (arg1[0] == '#')
					{
						printf("Irregular filename!");
						continue;
					}
					strcpy(filepath, location);
					getDirFilepath(filepath, arg1);
					deleteDir(filepath);
					memset(filepath, 0, MAX_FILENAME_LEN);
				}
				//ls
				else if(strcmp(cmd,"ls")==0)
				{
					ls();
				}
				//cd
				else if(strcmp(cmd,"cd")==0)
				{
					if (arg1[0] == '#')
					{
						printf("Irregular filename!");
						continue;
					}
					else if (strcmp(arg1, "..") == 0)
					{
						cdback();
					}
					else
					{
						cd(arg1);
					}
				}
				else if(strcmp(cmd,"information")==0)
				{
					information();
				}
				else if (strcmp(cmd,"print")==0)
				{	
					printf("%s\n",arg1);
				}
				else if (strcmp(rdbuf, "chat") == 0)
				{
			    		clear();
			
					while (1) 
					{
						printl("[You /] ");
						int r = read(fd_stdin, rdbuf, 70);
						rdbuf[r] = 0;
			 			//show();
			     		   	if (strcmp(rdbuf, "Who are you") == 0)
			    		   	 {
							printf("I/m saliya~\n");
							continue;
			   		    	 }
						else if (strcmp(rdbuf, "Where are you from") == 0)
						{
							printf("ALLAD LAND^_^ ! \n");
							continue;
				
						}
			     			  else if (strcmp(rdbuf, "How are you") == 0)
						{
							printf("I'm going to find a real fighter...! ! \n");
							continue;
						}
						else if (strcmp(rdbuf, "Mirror, mirror, who is the most handsome man in the world") == 0)
						{

							printf("hehe! \n");
							continue;
						}
				
						else if (strcmp(rdbuf, "Byebye") == 0)
						{
							printf("bye~~\n");
							sleep(2);
							ClearWithHelp();
							break;
						}
						

						else 
				
							printf("Don't understand what you said...\n");
			
					}
				}
	
				else
					printf("Command not found, please check!\n");

			}
		}
		else {
			close(fd);

		}
			
	 	//show();
              
		
	}
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	/*char tty_name[] = "/dev_tty1";

	int fd_stdin  = open(tty_name, O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);

	char rdbuf[128];
	char cmd[8];
	char filename[120];
	char buf[1024];
	int m,n;
	printf("                        ==================================\n");
	printf("                                    File Manager           \n");
	printf("                                 Kernel on Orange's \n\n");
	printf("                        ==================================\n");
	while (1) {
		printf("$ ");
		int r = read(fd_stdin, rdbuf, 70);
		rdbuf[r] = 0;
		
		

		if (strcmp(rdbuf, "help") == 0)
		{
			printf("=============================================================================\n");
			printf("Command List     :\n");
			printf("1. create [filename]       : Create a new file \n");
			printf("2. read [filename]         : Read the file\n");
			printf("3. write [filename]        : Write at the end of the file\n");
			printf("4. delete [filename]       : Delete the file\n");
            printf("5. chat                    : Do you want to talk to me?0.0\n");
			printf("6. lab                    : Display the help message\n");
			printf("7. help                    : Display the help message\n");
			printf("==============================================================================\n");		
		}
		else if (strcmp(rdbuf, "help") == 0)
		{
			
		}
		else
		{
			int fd;
			int i = 0;
			int j = 0;
			char temp = -1;
			while(rdbuf[i]!=' ')
			{
				cmd[i] = rdbuf[i];
				i++;
			}
			cmd[i++] = 0;
			while(rdbuf[i] != 0)
			{
				filename[j] = rdbuf[i];
				i++;
				j++;
			}
			filename[j] = 0;

			if (strcmp(cmd, "create") == 0)
			{
				fd = open(filename, O_CREAT | O_RDWR);
				if (fd == -1)
				{
					printf("Failed to create file! Please check the fileaname!\n");
					continue ;
				}
				buf[0] = 0;
				write(fd, buf, 1);
				printf("File created: %s (fd %d)\n", filename, fd);
				close(fd);
			}
			else if (strcmp(cmd, "read") == 0)
			{
				fd = open(filename, O_RDWR);
				if (fd == -1)
				{
					printf("Failed to open file! Please check the fileaname!\n");
					continue ;
				}
				
				n = read(fd, buf, 1024);
				
				printf("%s\n", buf);
				close(fd);

			}
			else if (strcmp(cmd, "write") == 0)
			{
				fd = open(filename, O_RDWR);
				if (fd == -1)
				{
					printf("Failed to open file! Please check the fileaname!\n");
					continue ;
				}

				m = read(fd_stdin, rdbuf,80);
				rdbuf[m] = 0;
				
				n = write(fd, rdbuf, m+1);
				close(fd);
			}
			else if (strcmp(cmd, "delete") == 0)
			{
				m=unlink(filename);
				if (m == 0)
				{
					printf("File deleted!\n");
					continue;
				}
				else
				{
					printf("Failed to delete file! Please check the fileaname!\n");
					continue;
				}

			}
			else 
			{
				printf("Command not found, Please check!\n");
				continue;
			}

			
			
		}
		
			
	}

	assert(0); /* never arrive here */
	while(1){
	if(proc_table[5].run_state == 1){
	disp_str("b");

	}
	}  
}





void TestC()
{	
	while(1){
	if(proc_table[6].run_state == 1){
	disp_str("c");
	}
	}       
}



/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

void ClearWithHelp()
{
	clear();
	output();
	help();
}

void WelcomeLoad() {
	clear();
	milli_delay(10000);
	printf("*     *   *      *       *         **         *   *       ********    **  \n");
	printf("*   *     *      *       *         **       *  * *  *     *           **  \n");
	milli_delay(10000);
	printf("* *       *      *       *         **      *    *    *    ********    **  \n");
	printf("*   *     *      *       *                *     *     *   *               \n");
	printf("*     *   *      ******  ******    **     *     *     *   ********    **  \n");
	printf("                                                                          \n");
	milli_delay(20000);
	printf("                ************        ************                          \n");
	printf("                ************        ************                          \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);
	printf("                    *   *              *   *                              \n");
	milli_delay(10000);

}

void output() 
{

	printf(" ||           ****                   000    0   0    00000  0000  000      ||\n");
	printf(" || ***     *********               00     0 0  0      0    00    0  00    ||\n");
	printf(" || *****  **         ***           0000  00000 0      0    0000  0   00   ||\n");
	printf(" || *******      **      ***          00  0   0 0      0    00    0  00    ||\n");
	printf(" || *******      **        **       000   0   0 0000   0    0000  000      ||\n");
	printf(" || *******             ***                                                ||\n");
	printf(" || *****  **         ***           0000  00000   000   0  0               ||\n");
	printf(" || ***     *********               0       0    00     0  0               ||\n");
	printf(" ||           ****                  0000    0    0000   0000               ||\n");
	printf(" ||                                 0       0      00   0  0               ||\n");
	printf(" ||                                 0     00000  000    0  0               ||\n");
	printf("=============================================================================\n");
	printf("       ooooo     ooooooo         ooo      oooo     ooo      ooooo    oooooooo\n");
	printf("    oooo  oooo   ooo  oooo      ooooo     ooooo    ooo   ooooo  ooo  ooo     \n");
	printf("   ooo      ooo  ooo   ooo      oo ooo    oooooo   ooo  ooo          ooo     \n");
	printf("   ooo      ooo  oooooooo      oo   ooo   ooo oooo ooo  ooo  oooooo  oooooooo\n");
	printf("   ooo      ooo  ooo oooo     ooooooooo   ooo   oooooo  ooo     ooo  ooo     \n");
	printf("   oooo    oooo  ooo   ooo   ooo     ooo  ooo    ooooo  oooo    ooo  ooo     \n");
	printf("     oooooooo    ooo    ooo ooo      ooo  ooo     oooo    oooooooo   oooooooo\n");
	printf("=============================================================================\n");
}

void clear()
{
	clear_screen(0,console_table[current_console].cursor);
	console_table[current_console].crtc_start = 0;
	console_table[current_console].cursor = 0;
	
}

void help()
{
	printf("=============================================================================\n");
	printf("Command List                    :\n");
	printf("1. welcome                      : hello~\n");
	printf("2. app                          : choose the apps\n");
	printf("3. clear                        : Clear the screen\n");
	printf("4. help                         : Show this help message\n");
	printf("5. chat                         : Do you want to talk to me?0.0\n");
	printf("6. mkfile    [filename][str]    : make a new file\n");
	printf("7. mkdir     [directoryname]    : make a new directory\n");
	printf("8. read      [filename]         :read a file\n");
	printf("9. delete    [filename]         :delete a file\n");
	printf("10. deletedir [directoryname]    :delete a directory\n");
	printf("11 ls                           :show files in the catalog\n");
	printf("12. cd       [directoryname]    :enter a dir {cd .. = cdback}\n");
	printf("13. edit     [filename][str]    :edit file cover\n");
	printf("14. edit+    [filename][str]    :edit file expand\n");
    	printf("15. information                 : Show students' information\n");
	printf("16. print    [str]              :print string\n");
	printf("17. proc                        :show process\n");

	printf("==============================================================================\n");		
}


void information()
{
	printf("            ================================\n");
	printf("            |**                          **|\n");
	printf("            |**by Lv MuChuang & Xu Ren He**|\n");
	printf("            |**                          **|\n");
	printf("            |**         Thank You        **|\n");
	printf("            ================================\n");  
}	
void sleep(int pauseTime){
	int i = 0;
	for(i=0;i<pauseTime*1000000;i++){
;
}
}


void ProcessManage()
{
	clear();
	int i;
	printf("=============================================================================\n");
	printf("============================Process Manager==================================\n");
	printf("=                                                                           =\n");
	printf("===========   PID      |    name       | priority    | running?   =========\n");
	//进程号，进程名，优先级，是否是系统进程，是否在运行
	printf("=---------------------------------------------------------------------------=\n");
	for ( i = 0 ; i < NR_TASKS + NR_PROCS ; ++i )//逐个遍历
	{
//		if ( proc_table[i].priority == 0) continue;//系统资源跳过
		if(!selectShowProcess)
		{ if(proc_table[i].p_flags == 0) {
			if(i == 5 || i == 6) {
				printf("======         %d              %s             %d           %d  ============\n", proc_table[i].pid, proc_table[i].name, b_c_priority[i-5],proc_table[i].run_state);continue;
			}
		printf("======         %d              %s             %d           %d  ============\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority,proc_table[i].run_state);
	}
		}
		else{
			if(proc_table[i].p_flags == -1) continue;
						if(i == 5 || i == 6) {
				printf("======         %d              %s             %d           %d  ============\n", proc_table[i].pid, proc_table[i].name, b_c_priority[i-5],proc_table[i].run_state);continue;
			}
		printf("======         %d              %s             %d           %d  ============\n", proc_table[i].pid, proc_table[i].name, proc_table[i].priority,proc_table[i].run_state);
		}	
	}

}

/*****************************************************************************
*							File System
*****************************************************************************/

/*****************************************************************************
*								Init FS
*****************************************************************************/
void initFS()
{
	int fd = -1, n = 0;
	char bufr[1024];
	char filepath[MAX_FILENAME_LEN];
	char dirpath[MAX_FILENAME_LEN];
	char filename[MAX_FILENAME_LEN];

	memset(filequeue, 0, MAX_FILES);
	memset(dirqueue, 0, MAX_DIRS);

	fd = open("root", O_CREAT | O_RDWR);
	close(fd);

	fd = open("root", O_RDWR);
	write(fd, bufr, 1024);
	close(fd);

	/*fd = open("root", O_RDWR);
	n = read(fd, bufr, 1024);
	close(fd);

	int i, k;
	for (i = 0, k = 0; i < n; i++)
	{

		if (bufr[i] != ' ')
		{
			filepath[k] = bufr[i];
			k++;
		}
		else
		{
			while (bufr[i] == ' ')
				i++;

			if (strcmp(filepath, "") == 0)
				continue;

			getDirpathAndFilename(dirpath, filename, filepath);
			if (filename[0] == '#')
			{
				strcpy(dirs[dircount], filepath);
				dirqueue[dircount] = 1;
				dircount++;
			}
			else
			{
				strcpy(dirs[dircount], filepath);
				filequeue[filecount] = 1;
				filecount++;
			}

			fd = open(filepath, O_CREAT | O_RDWR);
			close(fd);

			k = 0;

			if (bufr[i] == 0)
				break;

			i--;
		}
	}*/
}

/*****************************************************************************
*							Identity a Directory
*****************************************************************************/
int isDir(const char * filepath)
{
	int pos = getPosInDirQueue(filepath);
	if (pos != -1)
	{
		return 1;
	}
	return 0;
}

/*****************************************************************************
*                             Get Filepath
*****************************************************************************/
void getFilepath(char *filepath, char * filename)
{
	strjin(filepath, filename, '_');
}

/*****************************************************************************
*                         Get Directory Filepath
*****************************************************************************/
void getDirFilepath(char *filepath, char * filename)
{
	strcat(filepath, "_");
	strjin(filepath, filename, '#');
}

/*****************************************************************************
*                   Get Dirpath And Filename/Dirname From Filepath
*****************************************************************************/
void getDirpathAndFilename(char * dirpath, char * filename, char * filepath)
{

	char str[MAX_FILENAME_LEN];
	int i, k;

	memset(dirpath, 0, MAX_FILENAME_LEN);
	memset(filename, 0, MAX_FILENAME_LEN);

	for (i = 0, k = 0; filepath[i] != 0; i++)
	{
		if (filepath[i] != '_')
		{
			str[k] = filepath[i];
			k++;
		}
		else
		{
			strcat(dirpath, str);
			strcat(dirpath, "_");
			memset(str, 0, MAX_FILENAME_LEN);
			k = 0;
		}
	}
	dirpath[strlen(dirpath) - 1] = 0;
	strcpy(dirpath, dirpath);
	strcpy(filename, str);

}

/*****************************************************************************
*						Get a Free Pos in FileQueue
*****************************************************************************/
int getFreeFilePos()
{
	int i = 0;
	for (i = 0; i < MAX_FILES; i++)
	{
		if (filequeue[i] == 0)
			return i;
	}
	printf("The number of files is full!!\n");
	return -1;
}

/*****************************************************************************
*						Get a Free Pos in DirQueue
*****************************************************************************/
int getFreeDirPos()
{
	int i = 0;
	for (i = 0; i < MAX_DIRS; i++)
	{
		if (dirqueue[i] == 0)
			return i;
	}
	printf("The number of folders is full!!\n");
	return -1;
}

/*****************************************************************************
*						Get Dir's Pos in FileQueue
*****************************************************************************/
int getPosInDirQueue(char * filepath)
{
	int i = 0;
	for (i = 0; i < MAX_FILES; i++)
	{
		if (strcmp(dirs[i], filepath) == 0)
			return i;
	}
	return -1;
}


/*****************************************************************************
*						Add Filename Into Dir
*****************************************************************************/
void addFileIntoDir(const char * dirpath, char * filename)
{
	int fd = -1;

	if (strcmp(dirpath, "root") == 0)
	{
		fd = open("root", O_RDWR);
		
	}
	else
	{
		fd = open(dirpath, O_RDWR);
	}

	if (fd == -1)
	{
		printf("%s has not been found!\n", dirpath);
		return;
	}

	strcat(filename, " ");
	editAppand(dirpath, filename);
}

/*****************************************************************************
*						Delete Filename From Dir
*****************************************************************************/
void deleteFileFromDir(const char * dirpath, char * filename)
{

	/*char bufr[MAX_USER_FILE * MAX_FILENAME_LEN];
	char bufw[MAX_USER_FILE * MAX_FILENAME_LEN];*/
	char bufr[1024];
	char bufw[1024];
	char buf[MAX_FILENAME_LEN];
	int fd = -1, n = 0;

	fd = open(dirpath, O_RDWR);

	if (fd == -1)
	{
		printf("%s has not been found!!\n", dirpath);
		return;
	}

	n = read(fd, bufr, 1024);

	int i, k;
	for (i = 0, k = 0; i < n; i++)
	{
		if (bufr[i] != ' ')
		{
			buf[k] = bufr[i];
			k++;
		}
		else
		{
			buf[k] = 0;
			k = 0;

			if (strcmp(buf, filename) == 0)
				continue;

			strcat(bufw, buf);
			strcat(bufw, " ");
		}
	}
	printf("%s\n", bufw);
	
	editCover(dirpath, bufw);

	close(fd);
}

/*****************************************************************************
*							 Create File
*****************************************************************************/
void createFile(char * filepath, char *filename, char * buf)
{
	int fd = -1, pos = -1;
	
	fd = open(filepath, O_CREAT | O_RDWR);
	printf("file name: %s\n content: %s\n", filename, buf);
	if (fd == -1)
	{
		printf("New file failed. Please check and try again!!\n");
		return;
	}
	else if (fd == -2)
	{
		printf("File already exist!!\n");
		return;
	}

	write(fd, buf, strlen(buf));
	close(fd);

	pos = getFreeFilePos();
	filequeue[pos] = 1;
	strcpy(files[pos], filepath);
	filecount++;

	addFileIntoDir(location, filename);
}

/*****************************************************************************
*							 Create Directory
*****************************************************************************/
void createDir(char * filepath, char *filename)
{
	int fd = -1, pos = -1;

	fd = open(filepath, O_CREAT | O_RDWR);
	printf("Folder name: %s\n", filename);
	if (fd == -1)
	{
		printf("New folder failed. Please check and try again!!\n");
		return;
	}
	else if (fd == -2)
	{
		printf("Folder already exists!!\n");
		return;
	}

	close(fd);

	pos = getFreeDirPos();
	dirqueue[pos] = 1;
	strcpy(dirs[pos], filepath);
	dircount++;


	char str[MAX_FILENAME_LEN] = "#";
	strcat(str, filename);
	addFileIntoDir(location, str);
}

/*****************************************************************************
*								Read File
*****************************************************************************/
void readFile(char * filename)
{
	char filepath[MAX_FILENAME_LEN];
	strcpy(filepath, location);
	getDirFilepath(filepath, filename);
	if (isDir(filepath))
	{
		printf("Cannot read folder!!\n");
		return;
	}

	int fd = -1;
	int n;
	char bufr[1024] = "";

	strcpy(filepath, location);
	getFilepath(filepath, filename);
	fd = open(filepath, O_RDWR);
	if (fd == -1)
	{
		printf("Opening file error. Please check and try again!\n");
		return;
	}

	n = read(fd, bufr, 1024);
	bufr[n] = 0;
	printf("%s(fd=%d) : %s\n", filepath, fd, bufr);
	close(fd);
}

/*****************************************************************************
*							Edit File Cover
*****************************************************************************/
void editCover(const char * filepath, char * str)
{
	char empty[1024];
	int fd = -1;
	fd = open(filepath, O_RDWR);
	if (fd == -1)
	{

		printf("Opening file error. Please check and try again!!\n");
		return;
	}
	memset(empty, 0, 1024);
	write(fd, empty, 1024);
	close(fd);
	fd = open(filepath, O_RDWR);
	write(fd, str, strlen(str));
	close(fd);
}

/*****************************************************************************
*							Edit File Appand
*****************************************************************************/
void editAppand(const char * filepath, char * str)
{
	int fd = -1;
	char bufr[1024];
	char empty[1024];

	fd = open(filepath, O_RDWR);
	if (fd == -1)
	{
		printf("Opening file error. Please check and try again!!\n");
		return;
	}

	read(fd, bufr, 1024);
	close(fd);

	fd = open(filepath, O_RDWR);
	write(fd, empty, 1024);
	close(fd);

	strcat(bufr, str);

	fd = open(filepath, O_RDWR);
	write(fd, bufr, strlen(bufr));
	close(fd);
}
/*****************************************************************************
*							   Delete File
*****************************************************************************/
void deleteFile(char * filepath)
{
	if (filecount == 0)
	{
		printf("Error, no file to delete!\n");
		return;
	}

	if (unlink(filepath) != 0)
	{
		printf("Deleting file error. Please check and try again!\n");
		return;
	}

	int i;
	for (i = 0; i < filecount; i++)
	{
		if (strcmp(files[i], filepath) == 0)
		{
			memset(files[i], 0, MAX_FILENAME_LEN);
			filequeue[i] = 0;
			filecount--;
			break;
		}
	}

	/* delete filename from user's dir */
	char dirpath[MAX_FILENAME_LEN];
	char filename[MAX_FILENAME_LEN];
	getDirpathAndFilename(dirpath, filename, filepath);

	deleteFileFromDir(dirpath, filename);
}

/*****************************************************************************
*							 Delete Directory
*****************************************************************************/
void deleteDir(char * filepath)
{
	if (dircount == 0)
	{
		printf("Error, no folder to delete!!\n");
		return;
	}

	char dirfile[MAX_FILENAME_LEN];
	char rdbuf[1024];
	int fd = -1, n = 0;
	char filename[MAX_FILENAME_LEN];
	fd = open(filepath, O_RDWR);
	if (fd == -1)
	{
		printf("Deleting folder error. Please check and try again!!\n");
		return;
	}

	n = read(fd, rdbuf, 1024);

	int i, k;
	for (i = 0, k = 0; i < n; i++)
	{

		if (rdbuf[i] != ' ')
		{
			dirfile[k] = rdbuf[i];
			k++;
		}
		else
		{
			dirfile[k] = 0;
			k = 0;

			char path[MAX_FILENAME_LEN];
			strcpy(path, filepath);
			strjin(path, filename, '_');

			if (dirfile[0] == '#')
			{
				deleteDir(path);
			}
			else
			{
				deleteFile(path);
			}
		}
	}
	close(fd);

	if (unlink(filepath) != 0)
	{
		printf("Deleting folder error. Please check and try again!\n");
		return;
	}

	for (i = 0; i < dircount; i++)
	{
		if (strcmp(dirs[i], filepath) == 0)
		{
			memset(dirs[i], 0, MAX_FILENAME_LEN);
			dirqueue[i] = 0;
			dircount++;
			break;
		}
	}

	char dirpath[MAX_FILENAME_LEN];

	getDirpathAndFilename(dirpath, filename, filepath);
	deleteFileFromDir(dirpath, filename);
}

/*****************************************************************************
*						List All Files in the Directory
*****************************************************************************/
void ls()
{
	int fd = -1;
	char bufr[1024];

	fd = open(location, O_RDWR);

	if (fd == -1)
	{
		printf("Error opening file\n");
		return;
	}

	read(fd, bufr, 1024);
	printf("%s\n", bufr);
	close(fd);
}

/*****************************************************************************
*									cd
*****************************************************************************/
void cd(char * dirname)
{
	char filepath[MAX_FILENAME_LEN];
	strcpy(filepath, location);
	getDirFilepath(filepath, dirname);
	if (!isDir(filepath))
	{
		printf("NO folder %s!\n", dirname);
		return;
	}

	strcat(location, "_");
	strcat(location, dirname);
}

/*****************************************************************************
*							Go Back To Previous Directory
*****************************************************************************/
void cdback()
{
	if (strcmp(location, "root") == 0)
	{
		printf("root");
		return;
	}

	char dirpath[MAX_FILENAME_LEN];
	char filename[MAX_FILENAME_LEN];

	getDirpathAndFilename(dirpath, filename, location);
	strcpy(location, "root");
}




/*****************************************************************************
 *                               fork
 *****************************************************************************/
/*PUBLIC int fork()
{
	MESSAGE msg;
	msg.type = FORK;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);
	assert(msg.RETVAL == 0);

	return msg.PID;
}*/
/*****************************************************************************
 *                                execv
 *****************************************************************************/
/*PUBLIC int execv(const char *path, char * argv[])
{
	char **p = argv;
	char arg_stack[PROC_ORIGIN_STACK];
	int stack_len = 0;

	while(*p++) {
		assert(stack_len + 2 * sizeof(char*) < PROC_ORIGIN_STACK);
		stack_len += sizeof(char*);
	}

	*((int*)(&arg_stack[stack_len])) = 0;
	stack_len += sizeof(char*);

	char ** q = (char**)arg_stack;
	for (p = argv; *p != 0; p++) {
		*q++ = &arg_stack[stack_len];

		assert(stack_len + strlen(*p) + 1 < PROC_ORIGIN_STACK);
		strcpy(&arg_stack[stack_len], *p);
		stack_len += strlen(*p);
		arg_stack[stack_len] = 0;
		stack_len++;
	}

	MESSAGE msg;
	msg.type	= EXEC;
	msg.PATHNAME	= (void*)path;
	msg.NAME_LEN	= strlen(path);
	msg.BUF		= (void*)arg_stack;
	msg.BUF_LEN	= stack_len;

	send_recv(BOTH, TASK_MM, &msg);
	assert(msg.type == SYSCALL_RET);

	return msg.RETVAL;
}*/
/*****************************************************************************
 *                                wait
 *****************************************************************************/
/**
 * Wait for the child process to terminiate.
 * 
 * @param status  The value returned from the child.
 * 
 * @return  PID of the terminated child.
 *****************************************************************************/
/*PUBLIC int wait(int * status)
{
	MESSAGE msg;
	msg.type   = WAIT;

	send_recv(BOTH, TASK_MM, &msg);

	*status = msg.STATUS;

	return (msg.PID == NO_TASK ? -1 : msg.PID);
}
*/
