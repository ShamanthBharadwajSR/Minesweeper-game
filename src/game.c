#include <SDL.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"
#include "app.h"

enum CellCode
{
	CellEmpty = 0u,
	CellMine = 1u,
	CellExposed = 1 << 1,
	CellFlagged = 1 << 2,
};

typedef struct Map
{
	uint32_t* cells;
	int width, height, numMines;
} Map;

static Map s_Map =
{
	NULL,
	10, 10,
	10
};

enum TextureID 
{ 
	T_GameWon,
	T_GameOver,
	T_Closed, 
	T_Flagged,
	T_Exposed, 
	T_Mine, 
	T_One, 
	T_Two, 
	T_Three, 
	T_Four, 
	T_Five, 
	T_Six, 
	T_Seven, 
	T_Eight,
	T_Size, // used in for loop
};
static Texture textures[T_Size] = { 0 };

static int firstClick = 1, gameOver = 0, gameWon = 0, tilesLeft = 0;

//
//	Map
//
static uint32_t getCell(int x, int y)
{
	return s_Map.cells[x + y * s_Map.width];
}

static uint32_t* getCellPtr(int x, int y)
{
	return &(s_Map.cells[x + y * s_Map.width]);
}

static void setCell(int x, int y, uint32_t cell)
{
	s_Map.cells[x + y * s_Map.width] = cell;
}

static int toIndex(int x, int y)
{
	return x + y * s_Map.width;
}

//
//	CellCode
//
static void setMine(uint32_t* cell, uint32_t flag)
{
	*cell &= 0xfffffffe; // clear mine
	*cell |= flag;
}

static int isMine(uint32_t cell)
{
	return cell & CellMine;
}

static void setExposed(uint32_t* cell, uint32_t flag)
{
	*cell &= 0xfffffffd; // clear exposed flag
	*cell |= (flag << 1);
}

static int isExposed(uint32_t cell)
{
	return cell & CellExposed;
}

static int isFlagged(uint32_t cell)
{
	return cell & CellFlagged;
}

static void setFlagged(uint32_t* cell, uint32_t flag)
{
	*cell &= 0xfffffffb; // clear flag
	*cell |= (flag << 2);
}

static void setCellNum(uint32_t* cell, uint32_t num)
{
	*cell |= (num << 8);
}

static int getCellNum(uint32_t cell)
{
	return (int)((cell & 0x0000ff00) >> 8);
}

//
//	Game
//
static void getSurroundingCells(int position, int neighbours[8], int* count)
{
	int top = 1, bottom = 1, left = 1, right = 1;
	
	if (position < s_Map.width)
		top = 0;
	else if (position >= s_Map.width * (s_Map.height - 1))
		bottom = 0;

	if (position % s_Map.width == 0)
		left = 0;
	else if (position % s_Map.width == s_Map.width - 1)
		right = 0;

	*count = 0;

	if (top)
	{
		neighbours[(*count)++] = position - s_Map.width;
		if (right)
			neighbours[(*count)++] = position - s_Map.width + 1;
		if (left)
			neighbours[(*count)++] = position - s_Map.width - 1;
	}
	if (bottom)
	{
		neighbours[(*count)++] = position + s_Map.width;
		if (right)
			neighbours[(*count)++] = position + s_Map.width + 1;
		if (left)
			neighbours[(*count)++] = position + s_Map.width - 1;
	}
	if (left)
		neighbours[(*count)++] = position - 1;
	if (right)
		neighbours[(*count)++] = position + 1;

}

static void generateMines(int except)
{
	int minesLeft = s_Map.numMines;
	int maxSize = s_Map.width * s_Map.height;
	while (minesLeft > 0)
	{
		int slot = rand() % maxSize;
		// we found a position that isn't already a mine
		// and is not the tile the play clicked on
		if (slot != except && !isMine(s_Map.cells[slot]))
		{
			setMine(&s_Map.cells[slot], 1);
			minesLeft--;
		}
	}

	for (int i = 0; i < s_Map.width; i++)
	{
		for (int j = 0; j < s_Map.height; j++)
		{
			if (isMine(getCell(i, j)))
				continue;

			int mineCount = 0, count, neighbours[8];
			getSurroundingCells(toIndex(i, j), neighbours, &count);

			for (int k = 0; k < count; k++)
				if (isMine(s_Map.cells[neighbours[k]]))
					mineCount++;

			setCellNum(getCellPtr(i, j), mineCount);
		}
	}
}

static int expose(int position)
{
	if (isFlagged(s_Map.cells[position]))
		return 0;

	if (isExposed(s_Map.cells[position]))
		return 0;

	setExposed(&s_Map.cells[position], 1);

	if (isMine(s_Map.cells[position]))
		return 1;

	tilesLeft--;

	if (getCellNum(s_Map.cells[position]) != 0)
		return 0;

	int count, neighbours[8];
	getSurroundingCells(position, neighbours, &count);

	for (int i = 0; i < count; i++)
		expose(neighbours[i]);

	return 0;
}

static void exposeAll()
{
	for (int i = 0; i < s_Map.width; i++)
		for (int j = 0; j < s_Map.height; j++)
			setExposed(getCellPtr(i, j), 1);
}

static void drawCell(uint32_t cell, float xpos, float ypos, float width, float height)
{
	Texture* t = &textures[T_Closed];
	
	if (isFlagged(cell))
		t = &textures[T_Flagged];
	else if (isExposed(cell))
	{
		if (isMine(cell))
			t = &textures[T_Mine];
		else
		{
			uint32_t count = getCellNum(cell);
			if (!count)
				t = &textures[T_Exposed];
			else
				t = &textures[T_One + count - 1];
		}
	}

	mns_DrawTextureWithBounds2f(t, xpos, ypos, width, height);
}

void mns_GameInit()
{
	srand((uint32_t)(int64_t)time(NULL));

	const char* files[] =
	{
		"assets/game_won.png",
		"assets/game_over.png",
		"assets/closed.png",
		"assets/flagged.png",
		"assets/exposed.png",
		"assets/mine.png",
		"assets/one.png",
		"assets/two.png",
		"assets/three.png",
		"assets/four.png",
		"assets/five.png",
		"assets/six.png",
		"assets/seven.png",
		"assets/eight.png",
	};

	for (int i = 0; i < T_Size; i++)
		textures[i] = mns_CreateTextureFromFile(files[i]);

	s_Map.cells = (uint32_t*)malloc(sizeof(uint32_t) * s_Map.width * s_Map.height);
	memset(s_Map.cells, 0, s_Map.height * s_Map.width * sizeof(uint32_t));

	tilesLeft = s_Map.width * s_Map.height - s_Map.numMines;
	firstClick = 1;
	gameOver = gameWon = 0;
}

void mns_GameShutdown()
{
	for (int i = 0; i < T_Size; i++)
		mns_FreeTexture(&textures[i]);
}

void mns_GameUpdate(float delta)
{
	
}

void mns_GameRender()
{
	float x = -1.f, y = -1.f;
	float wx = 2.f / s_Map.width, wy = 2.f / s_Map.height;

	for (int i = 0; i < s_Map.width; i++)
	{
		y = -1.f;
		for (int j = 0; j < s_Map.height; j++)
		{
			drawCell(getCell(i, j), x, y, wx, wy);
			y += wy;
		}
		x += wx;
	}

	Texture* t = NULL;
	
	if (gameOver)
		t = &textures[T_GameOver];
	else if (gameWon)
		t = &textures[T_GameWon];

	if (t)
	{
		float x = -0.9f, width = 1.8f;
		float height = width * (72.f / 298.f);
		float y = -(height / 2.f);
		mns_DrawTextureWithBounds2f(t, x, y, width, height);
	}
}

void mns_GameOnClick(int x, int y, int left)
{
	if (gameOver || gameWon)
		return;

	Point2i p2i = { x, y };
	Point2f worldSpace = mns_ScreenSpaceToWorldSpace(&p2i);

	if (worldSpace.x < -1.f || worldSpace.y < -1.f || worldSpace.x > 1.f || worldSpace.y > 1.f)
		return;

	worldSpace.x = (worldSpace.x + 1.f) / 2.f;
	worldSpace.y = (worldSpace.y + 1.f) / 2.f;
	int gx = (int)(worldSpace.x * s_Map.width);
	int gy = (int)(worldSpace.y * s_Map.height);
	
	if (gx < 0 || gy < 0 || gx >= s_Map.width || gy >= s_Map.height)
		return;

	if (left == 1)
	{
		// generate mines excluding the first tile clicked on
		if (firstClick)
		{
			firstClick = 0;
			generateMines(toIndex(gx, gy));
		}

		if (expose(toIndex(gx, gy)))
		{
			gameOver = 1;
			exposeAll();
		}

		if (tilesLeft <= 0)
			gameWon = 1;
	}
	else // right click
	{
		// if cell is not exposed, toggle flag state
		if (!isExposed(getCell(gx, gy)))
			setFlagged(getCellPtr(gx, gy), !isFlagged(getCell(gx, gy)));
	}

}
