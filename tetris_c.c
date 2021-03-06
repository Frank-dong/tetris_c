#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <errno.h>

#include <sys/time.h>

#include "tetris_c.h"

#define true 1
#define false 0
#define DIMENSION	4

#define KEY_QUIT	0x71
#define KEY_UP		0x41
#define KEY_DOWN	0x42
#define KEY_LEFT	0x44
#define KEY_RIGHT	0x43
#define KEY_PAUSE	0x20


#define set_pos(x, y)	printf("\033[%d;%dH", x + 1, 2*y+1)
#define clear_screen() 	printf("\033[2J")
#define hide_cursor()	printf("\033[?25l")
#define show_cursor()	printf("\033[?25h")
#define show_cursor()	printf("\033[?25h")
#define paint_elem(c)	printf("\033[%dm%d ", 40 + c, c)
#define close_all()		printf("\033[0m");
#define fresh_screen()	fflush(stdout)

typedef enum {
	DRAW,
	CLEAR
} draw_flag;

enum MSG_TYPE{
	KEY_TYPE = 1,
	SIG_TYPE,
	CTR_TYPE
};

enum MSG_KEY_VALUE {
	MSG_KEY_QUIT,
	MSG_KEY_UP,
	MSG_KEY_DOWN,
	MSG_KEY_LEFT,
	MSG_KEY_RIGTH,
	MSG_KEY_PAUSE
};

enum MSG_CTR_VALUE {
	MSG_CTR_GAMEOVER
};

enum BLOCK_MODE {
	BLOCK_NORMAL,
	BLOCK_SQUARE,
	BLOCK_LINE
};

typedef struct{
	unsigned char block[DIMENSION][DIMENSION];
	int info;
} block_t;

struct canvas {
	unsigned char** parray;
	int length;
	int high;
};

typedef struct {
	long int mtype;
	int data;	
} msg_t;

/*-------------------------------*/
int msgqueue_id = 0;


void random_color(block_t* b)
{
	int i = 0, j = 0;

	srand(getpid());
	for (i = 0; i < DIMENSION; ++i)
		for (j = 0; j < DIMENSION; ++j)
			if (b->block[i][j])
				b->block[i][j] = rand()%7+1;
}

void draw_elem(int x, int y, int color)
{
	set_pos(x, y);
	paint_elem(color);
}

void show_time()
{
	struct timeval tm;

	gettimeofday(&tm, NULL);
	printf("This time is [ %ld - %ld ]\r\n", tm.tv_sec, tm.tv_usec);
}


/**
 * 以{x, y}坐标为坐标原点，画出图案
 * flag:DRAW标识绘图，CLEAR标识清除
 */
int draw(block_t* b, int x, int y, draw_flag flag)
{
	int i = 0, j = 0;

	if (DRAW == flag) {
		for (i = 0; i < DIMENSION; ++i)
			for (j = 0; j < DIMENSION; ++j)
				if (b->block[i][j])
					draw_elem(x+i, y+j, b->block[i][j]);
	} else { 
		for (i = 0; i < DIMENSION; ++i)
			for (j = 0; j < DIMENSION; ++j)
				if (b->block[i][j])
					draw_elem(x+i, y+j, 0);
	}
	fresh_screen();
	return true;
}

/**
 * 调用一次，则顺时针旋转90度
 * 后面你可以改进成传递一个旋转角度的参数
 */
int revolve(block_t* b)
{
	int i = 0, j = 0;
	block_t tmp;
	
	if (!b)
		return false;

	memcpy(&tmp, b, sizeof(block_t));

	switch (b->info) {
	case BLOCK_NORMAL:
		for (i = 0; i < DIMENSION-1; ++i)
			for (j = 0; j < DIMENSION-1; ++j)
				b->block[i][j] = tmp.block[DIMENSION-2-j][i];
		break;
	case BLOCK_LINE:
		for (i = 0; i < DIMENSION; ++i)
			for (j = 0; j < DIMENSION; ++j)
				b->block[i][j] = tmp.block[DIMENSION-1-j][i];
		break;	
	case BLOCK_SQUARE:
		break;
	default :
		break;
	}

	return true;
}

struct termios stored_settings;
void key_init()
{
	int in; 
	struct termios new_settings;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= ~(ICANON | ECHO);
	new_settings.c_cc[VTIME] = 0;
	tcgetattr(0, &stored_settings);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
}
void key_deinit()
{
	tcsetattr(0, TCSANOW, &stored_settings);		//最后需要回复到原来的配置
	return;
}

void* key_run(void* data)
{
	msg_t	msg;
	int		key_value = 0;
	
	msg.mtype = KEY_TYPE;
	while (1) {
		key_value = getchar();
		switch (key_value) {
			case KEY_QUIT:
				msg.data = MSG_KEY_QUIT;
				break;
			case KEY_UP:
				msg.data = MSG_KEY_UP;
				break;
			case KEY_DOWN:
				msg.data = MSG_KEY_DOWN;
				break;
			case KEY_LEFT:
				msg.data = MSG_KEY_LEFT;
				break;
			case KEY_RIGHT:
				msg.data = MSG_KEY_RIGTH;
				break;
			case KEY_PAUSE:
				msg.data = MSG_KEY_PAUSE;
				break;
			default:
				continue;
		}
		msgsnd(msgqueue_id, (void*)&msg, sizeof(msg_t) - sizeof(long int), 0);
	}
}

void alarm_func(int data)
{
	msg_t	msg;
	
	msg.mtype = SIG_TYPE;
	msg.data = 1;
	msgsnd(msgqueue_id, (void*)&msg, sizeof(msg_t) - sizeof(long int), 0);
	
	alarm(1);
}

void update_score(struct canvas* pcanv, int score)
{
	set_pos(pcanv->high/2, pcanv->length + 2);
	printf("SCORE: %d", score);
	fresh_screen();
}


void init(struct canvas* pcanv)
{
	int i = 0, j = 0;
	pthread_t	pthread_handle;

	clear_screen();
	hide_cursor();
	key_init();

	msgqueue_id = msgget('K', 0666 | IPC_CREAT);
	pthread_create(&pthread_handle, NULL, key_run, NULL);

	signal(SIGALRM, alarm_func);
	alarm(1);

	/* length --> y轴  high --> x轴 */
	pcanv->parray = (unsigned char**)malloc((sizeof(unsigned char*))*(pcanv->high));
	for (i = 0; i < pcanv->high; ++i)
		(pcanv->parray)[i] = (unsigned char*)malloc(sizeof(unsigned char)*(pcanv->length));
	
	
	for (i = 0; i < pcanv->high; ++i)
		for (j = 0; j < pcanv->length; ++j) {
			if (i == 0 || i == pcanv->high-1 || j == 0 || j == pcanv->length-1)
				(pcanv->parray)[i][j] = 1,draw_elem(i, j, 2);
			else
				(pcanv->parray)[i][j] = 0,draw_elem(i, j, 0);
			//fresh_screen();
		}
	update_score(pcanv, 0);
}

void deinit(struct canvas* pcanv)
{
	int	i = 0;
	int j = 0;
	
	close_all();
	show_cursor();
	key_deinit();
	
	for (i = 0; i < pcanv->high; ++i) {
		free((unsigned char*)pcanv->parray[i]);
		pcanv->parray[i] = NULL;
	}
	free(pcanv->parray);
}
void show(block_t* b)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j)
			printf("%d ", b->block[i][j]);
		printf("\r\n");
	}
	printf("\r\n");
}

int ismove(struct canvas* pcanv, block_t* b, int x, int y)
{
	int i = 0, j = 0;

	for (i = 0; i < DIMENSION; ++i)
		for (j = 0; j < DIMENSION; ++j)
			if (x+i < pcanv->high && y+j < pcanv->length && b->block[i][j])
				if (pcanv->parray[x+i][y+j])
					return false;

	return true;
}


void modify_canv(struct canvas* pcanv, block_t* b, int x, int y)
{
	int i = 0, j = 0, k = 0;
	int sigle_score = 1;
	int line_num = 0;
	int line_score[DIMENSION+1] = {0};
	int flag = true;
	int start_line = 0;
	static int total_scole = 0;
	static int most_high_line = 1000;
	msg_t	msg;

	most_high_line = most_high_line > x ? x : most_high_line;
	printf("\033[30;30H %d", most_high_line);

	if (most_high_line <= 1) {
		msg.mtype = CTR_TYPE;
		msg.data = MSG_CTR_GAMEOVER;
		msgsnd(msgqueue_id, (void*)&msg, sizeof(msg_t) - sizeof(long int), 0);
		return;
	}
		
	
	//填充画布
	for (i = 0; i < DIMENSION; ++i)
		for (j = 0; j < DIMENSION; ++j)
			if (x+i < pcanv->high && y+j < pcanv->length && b->block[i][j])
				pcanv->parray[x+i][y+j] = b->block[i][j];

	//计算分数(存在隔行得分的情况)
	for (i = pcanv->high-2; i >= x; --i) {
		for (j = 1; j < pcanv->length-1; ++j) {
			if (pcanv->parray[i][j] == 0)
				break;
		}
		if (j >= pcanv->length-1) 
			line_score[k++] = i;
	}
	
	//重绘
	if (line_score[0] != 0) {
		total_scole += (1 << (k-1)); //得分分别是1 2 4 8
		update_score(pcanv, total_scole);
		for (k = 0; line_score[k] != 0; ++k) {
			for (i = line_score[k]+k; i >= most_high_line+k; --i)
				memcpy(&(pcanv->parray[i][1]), &(pcanv->parray[i-1][1]), pcanv->length-2);
			memset(&(pcanv->parray[i][1]), 0, pcanv->length-2);
		}
		for (i = pcanv->high-2; i >= most_high_line; --i)
			for (j = 1; j <= pcanv->length-2; ++j)
				if (pcanv->parray[i][j])
					draw_elem(i, j, pcanv->parray[i][j]);
				else 
					draw_elem(i, j, 0);
		fresh_screen();
	}
}


void show_over(struct canvas* pcanv)
{
	int x, y;

	x = pcanv->high/2-1;
	y = pcanv->length/2-3;

	set_pos(x++, y);
	printf("\033[43m\033[32m");
	printf("             ");
	set_pos(x++, y);
	printf("  GAME OVER  ");
	set_pos(x++, y);
	printf("             ");
	fresh_screen();

	set_pos(pcanv->high+1, 0);
}

void play(struct canvas* pcanv)
{
	block_t elems[7] = {
		{
		 {{0,1,0,0},
	      {1,1,1,0},
	      {0,0,0,0},
	      {0,0,0,0}},
		BLOCK_NORMAL,	
	    },
	    {
		 {{1,1,0,0},
	      {0,1,1,0},
	      {0,0,0,0},
	      {0,0,0,0}},
		BLOCK_NORMAL,	
	    },
	    {
		 {{0,1,1,0},
	      {1,1,0,0},
	      {0,0,0,0},
	      {0,0,0,0}},
		BLOCK_NORMAL,	
	    },
	    {
		 {{0,0,0,0},
	      {1,1,1,1},
	      {0,0,0,0},
	      {0,0,0,0}},
		BLOCK_LINE,
	    },
	    {
		 {{1,1,0,0},
	      {1,1,0,0},
	      {0,0,0,0},
	      {0,0,0,0}},
		BLOCK_SQUARE,
	    },
	    {
		 {{0,1,0,0},
	      {0,1,0,0},
	      {0,1,1,0},
	      {0,0,0,0}},
		BLOCK_NORMAL,
	    },
	    {
		 {{0,1,0,0},
	      {0,1,0,0},
	      {1,1,0,0},
	      {0,0,0,0}},
		BLOCK_NORMAL,
	    },  
	};
	int index = 0;
	int times = 0;
	int i = 0;
	int x = 0, y = 0;
	int cx = 0, cy = 0;
	msg_t	msg;
	static char pause_flag = 0;
	block_t  tmp_block;
	
	srand(getpid());
	while (1) {
		times = rand()%4;
		index = rand()%7;
		for (i = 0; i < times; ++i)	//随机出现一个随机变换后的图案
			revolve(&elems[index]);

		//对出现的图案重新随机填色
	
		y = pcanv->length/2;
		x = 1;
		while(1) {
			draw(&elems[index], x, y, DRAW);
			cx = x;
			cy = y;
			if (msgrcv(msgqueue_id, (void *)&msg, sizeof(msg_t) - sizeof(long int), 0, 0) == -1) {
				if (errno == EINTR)
					continue;
	            fprintf(stderr, "msgrcv failed width erro: %d", errno);
	        }
			if (msg.mtype == KEY_TYPE) {
				switch(msg.data) {
					case MSG_KEY_DOWN:
						//alarm(0);
						while (ismove(pcanv, &elems[index], ++x, y)) {
							usleep(1000);
							draw(&elems[index], x-1, y, CLEAR);
							draw(&elems[index], x, y, DRAW);
						}
						--x;
						//alarm(1);
						continue;
					case MSG_KEY_UP:
						draw(&elems[index], x, y, CLEAR);
						memcpy(&tmp_block, &elems[index], sizeof(block_t));
						revolve(&tmp_block);
						if (ismove(pcanv, &tmp_block, x, y))
							memcpy(&elems[index], &tmp_block, sizeof(block_t));
						draw(&elems[index], x, y, DRAW);
						break;
					case MSG_KEY_LEFT:
						--cy;
						break;
					case MSG_KEY_RIGTH:
						++cy;
						break;
					case MSG_KEY_QUIT:
						break;
					case MSG_KEY_PAUSE:
						if (pause_flag)
							alarm(1),pause_flag = 0;
						else 
							alarm(0),pause_flag = 1;
						break;
					default:
						continue;
				}
			} else if (msg.mtype == SIG_TYPE) {
				++cx;
			} else if (msg.mtype == CTR_TYPE) {
				show_over(pcanv);
				return;
			}
			
			if (!ismove(pcanv, &elems[index], cx, cy)) {		//判断当前是否可以移动
				if (msg.data != MSG_KEY_LEFT && msg.data != MSG_KEY_RIGTH) {
					alarm(0);
					modify_canv(pcanv, &elems[index], x, y);
					alarm(1);
					break;
				}
				continue;
			}
			draw(&elems[index], x, y, CLEAR);
			x = cx;
			y = cy;
		}
	}

	
	return;
}

void main(int argc, char* argv)
{
	int length = 15;
	int high   = 20;
	struct canvas canv;

	canv.high = high;
	canv.length = length;

	init(&canv);
	play(&canv);
	deinit(&canv);
}

/**
问题：
1. 实际使用的空间比mallo的空间大，free的时候就会报错。

2. 报出这样的错误
   *** stack smashing detected ***: ./a.out terminated
   Aborted (core dumped)
   https://blog.csdn.net/haidonglin/article/details/53672208

   在判断是否可以移动、填充值等过程中，对画布的数组访问，不能越界，否则就会报错。
3. 方块旋转的矫正问题
   方块采用4x4的矩阵，在移动时，方块边界为空，肯能会移出画布。
   去掉draw接口中的参数判断，因为在判断移动等操作中，都会判断图案的每个像素点上是否有值，所以投射到画布上，是不会超出画布边界，也不会出现下标为-1的情况
4. 档图案贴在画布边上的时候，旋转后的图案可能会超出画布边界，此事要做判断，重新矫正图案的位置。
*/
