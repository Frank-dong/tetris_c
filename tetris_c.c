#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


#include "tetris_c.h"

#define true 0
#define false -1
#define DIMENSION	4

#define set_pos(x, y)	printf("\033[%d;%dH", x + 1, 2*y+1)
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

	if (DRAW == flag)
		for (i = 0; i < DIMENSION; ++i)
			for (j = 0; j < DIMENSION; ++j)
				if (b->block[i][j])
					draw_elem(x+i, y+j, b->block[i][j]);
	else 
		for (i = 0; i < DIMENSION; ++i)
			for (j = 0; j < DIMENSION; ++j)
				if (b->block[i][j])
					draw_elem(x+i, y+j, 0);
}

/**
 * 调用一次，则顺时针旋转90度
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

void init(int length, int high, unsigned char*** pcanvas)
{
	int i = 0, j = 0;

	clear_screen();

	*pcanvas = (unsigned char**)malloc((sizeof(unsigned char*))*high);
	for (i = 0; i < high; ++i)
		(*pcanvas)[i] = (unsigned char*)malloc(sizeof(unsigned char)*length);
	
	
	for (i = 0; i < high; ++i)
		for (j = 0; j < length; ++j)
			if (i == 0 || i == high-1 || j == 0 || j == length-1)
				(*pcanvas)[i][j] = 1,draw_elem(i, j, 2);
			else
				(*pcanvas)[i][j] = 0,draw_elem(i, j, 0);
}

void deinit(int length, int high, unsigned char** pcanvas)
{
	int	i = 0;
	int j = 0;
	
	close_all();
	for (i = 0; i < high; ++i) {
		free((unsigned char*)pcanvas[i]);
		pcanvas[i] = NULL;
	}
	free(pcanvas);
}

void main(int argc, char* argv)
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

	int length = 20;
	int high   = 40;
	unsigned char **pcanvas = NULL;
	init(length, high, &pcanvas);

	deinit(length, high, pcanvas);
	pcanvas = NULL;
}
