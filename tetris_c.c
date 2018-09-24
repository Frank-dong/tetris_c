#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>

#include "tetris_c.h"

#define true 1
#define false 0
#define DIMENSION	4

#define set_pos(x, y)	printf("\033[%d;%dH", y + 1, 2*x+1)
#define clear_screen() 	printf("\033[2J")
#define hide_cursor()	printf("\033[?25l")
#define show_cursor()	printf("\033[?25h")
#define paint_elem(c)	printf("\033[%dm  ", 40 + c)
#define close_all()		printf("\033[0m");

typedef enum {
	DRAW,
	CLEAR
} draw_flag;

typedef struct{
	unsigned char block[DIMENSION][DIMENSION];
	int info;
} block_t;

struct canvas {
	unsigned char** parray;
	int length;
	int high;
};
/*-------------------------------*/

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

/**
 * 以{x, y}坐标为坐标原点，画出图案
 * flag:DRAW标识绘图，CLEAR标识清除
 */
int draw(block_t* b, int x, int y, draw_flag flag)
{
	int i = 0, j = 0;
	
	if (!b || x < 0 || y < 0 || 
		(flag != DRAW && flag != CLEAR))
		return false;

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
	fflush(stdout);
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

	for (i = 0; i < DIMENSION; ++i)
		for (j = 0; j < DIMENSION; ++j)
			b->block[i][j] = tmp.block[DIMENSION-1-j][i];

	return true;
}

struct termios stored_settings;
void key_init()
{
	int in; 
	struct termios new_settings;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;
	//new_settings.c_oflag &= ~(OPOST);
	new_settings.c_lflag &= ~(ICANON | ECHO);
	new_settings.c_cc[VTIME] = 0;
	tcgetattr(0,&stored_settings);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0,TCSANOW, &new_settings);
	
	//in = getchar();
	     
	
	//return in; 
}
void key_deinit()
{
	tcsetattr(0,TCSANOW, &stored_settings);		//最后需要回复到原来的配置
	return;
}

void init(struct canvas* pcanv)
{
	int i = 0, j = 0;

	clear_screen();
	hide_cursor();
	key_init();

	pcanv->parray = (unsigned char**)malloc((sizeof(unsigned char*))*(pcanv->high));
	for (i = 0; i < pcanv->high; ++i)
		(pcanv->parray)[i] = (unsigned char*)malloc(sizeof(unsigned char)*(pcanv->length));
	
	
	for (i = 0; i < pcanv->length; ++i)
		for (j = 0; j < pcanv->high; ++j)
			if (i == 0 || i == pcanv->length-1 || j == 0 || j == pcanv->high-1)
				(pcanv->parray)[i][j] = 1,draw_elem(i, j, 2);
			else
				(pcanv->parray)[i][j] = 0,draw_elem(i, j, 0);
}

void deinit(struct canvas* pcanv)
{
	int	i = 0;
	int j = 0;
	
	close_all();
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

int ismove()
{
	return true;
}

void play(struct canvas* pcanv)
{
	block_t elems[7] = {
		{
		 {{0,1,0,0},
	      {1,1,1,0},
	      {0,0,0,0},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,0,0,0},
	      {1,1,0,0},
	      {0,0,1,1},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,0,0,0},
	      {0,0,1,1},
	      {1,1,0,0},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,0,0,0},
	      {1,1,1,1},
	      {0,0,0,0},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,0,0,0},
	      {0,1,1,0},
	      {0,1,1,0},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,1,0,0},
	      {0,1,0,0},
	      {0,1,1,0},
	      {0,0,0,0}},
		  0
	    },
	    {
		 {{0,0,1,0},
	      {0,0,1,0},
	      {0,1,1,0},
	      {0,0,0,0}},
		  0
	    },  
	};
	int index = 0;
	int times = 0;
	int i = 0, j = 0;
	int x = 0, y = 0;
	
	srand(getpid());
	while (1) {
		times = rand()%4;
		index = rand()%7;
		for (i = 0; i < times; ++i)	//随机出现一个随机变换后的图案
			revolve(&elems[index]);

		x = pcanv->length/2;
		while(1) {				//每隔1s下落一次
			draw(&elems[index], x, y, DRAW);
			if (!ismove())		//判断当前是否可以移动
				break;
			sleep(1);
			draw(&elems[index], x, y++, CLEAR);
		}
	}

	
	return;
}

void main(int argc, char* argv)
{
	int length = 20;
	int high   = 40;

	struct canvas canv;
	canv.high = 40;
	canv.length = 20;
		
	init(&canv);
	play(&canv);
	deinit(&canv);
}

/**
问题：
1. 实际使用的空间比mallo的空间大，free的时候就会报错。


*/
