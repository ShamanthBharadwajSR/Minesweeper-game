#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "game.h"

#define DEF_WIDTH 600
#define DEF_HEIGHT 600

static Application s_App = 
{
	NULL, NULL,
	"Minesweeper",
	DEF_WIDTH, DEF_HEIGHT, 1,
	0.16f
};

static uint64_t now, last;

Application* mns_Application = NULL;

int mns_ShouldRun()
{
	return s_App.running;
}

void mns_Init(int argc, char** argv)
{
	{
		int rendererFlags, windowFlags;

		rendererFlags = SDL_RENDERER_ACCELERATED;

		windowFlags = SDL_WINDOW_RESIZABLE;

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			printf("Couldn't initialize SDL: %s\n", SDL_GetError());
			exit(1);
		}

		s_App.window = SDL_CreateWindow(s_App.appName,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
			s_App.width, s_App.height, 
			windowFlags);

		if (!s_App.window)
		{
			printf("Failed to open %d x %d window: %s\n", s_App.width, s_App.height, SDL_GetError());
			exit(1);
		}

		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

		s_App.renderer = SDL_CreateRenderer(s_App.window, -1, rendererFlags);

		if (!s_App.renderer)
		{
			printf("Failed to create renderer: %s\n", SDL_GetError());
			exit(1);
		}
	}

	s_App.aspectRatio = (float)s_App.width / (float)s_App.height;
	mns_Application = &s_App;

	mns_GameInit();
}

void mns_Shutdown()
{
	mns_GameShutdown();

	SDL_DestroyRenderer(s_App.renderer);
	SDL_DestroyWindow(s_App.window);
	SDL_Quit();
}

void mns_Update()
{
	mns_GameUpdate(s_App.delta);
}

void mns_InitFrame()
{
	last = SDL_GetPerformanceCounter();

	SDL_GetWindowSize(s_App.window, &s_App.width, &s_App.height);
	s_App.aspectRatio = (float)s_App.width / (float)s_App.height;

	SDL_SetRenderDrawColor(s_App.renderer, 0, 0xcc, 0xff, 255);
	SDL_RenderClear(s_App.renderer);
}

void mns_RenderFrame()
{
	mns_GameRender();

	SDL_RenderPresent(s_App.renderer);
}

void mns_PollEvents()
{
	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT:
			s_App.running = 0;
			break;

		case SDL_MOUSEBUTTONDOWN:
		{
			int mx, my;
			uint32_t ms = SDL_GetMouseState(&mx, &my);
			mns_GameOnClick(mx, my, ms);
			break;
		}

		default:
			break;
		}
	}

	// end of while loop (each frame)
	now = SDL_GetPerformanceCounter();

	s_App.delta = (float)((now - last) * 1000) / (float)SDL_GetPerformanceFrequency();
	s_App.delta *= 1e-3f;
}

Point2i mns_WorldSpaceToScreenSpace(const Point2f* p2f)
{
	Point2i p2i = { 0 };
	if (s_App.aspectRatio > 1.f) // landscape -> y : -1 to 1
	{
		p2i.y = (int)(((p2f->y + 1.f) / 2.f) * s_App.height);
		p2i.x = (int)((((p2f->x / s_App.aspectRatio) + 1.f) / 2.f) * s_App.width);
	}
	else // portrait -> x : -1 to 1
	{
		p2i.x = (int)(((p2f->x + 1.f) / 2.f) * s_App.width);
		p2i.y = (int)((((p2f->y * s_App.aspectRatio) + 1.f) / 2.f) * s_App.height);
	}

	return p2i;
}

Point2f mns_ScreenSpaceToWorldSpace(const Point2i* p2i)
{
	Point2f p2f = { 0 };
	if (s_App.aspectRatio > 1.f) // landscape -> y : -1 to 1
	{
		p2f.y = ((float)p2i->y / (float)s_App.height) * 2.f - 1.f;
		p2f.x = (((float)p2i->x / (float)s_App.width) * 2.f - 1.f) * s_App.aspectRatio;
	}
	else // portrait -> x : -1 to 1
	{
		p2f.x = ((float)p2i->x / (float)s_App.width) * 2.f - 1.f;
		p2f.y = (((float)p2i->y / (float)s_App.height) * 2.f - 1.f) / s_App.aspectRatio;
	}

	return p2f;
}

Texture mns_CreateTextureFromFile(const char* name)
{
	Texture t = { 0 };

	int comp;
	t.data = stbi_load(name, &t.width, &t.height, &comp, STBI_rgb_alpha);

	if (t.data)
		t = mns_CreateTextureFromBin((uint32_t*)t.data, t.width, t.height);
	else
		printf("Failed to load texture %s\n", name);
		
	return t;

}

Texture mns_CreateTextureFromBin(unsigned int* data, int width, int height)
{
	Texture t = { 0 };

	t.data = (void*)data;
	t.width = width;
	t.height = height;

	SDL_Surface* s = SDL_CreateRGBSurfaceWithFormatFrom(t.data, t.width, t.height, 8, t.width * 4, SDL_PIXELFORMAT_ABGR8888);
	t.texture = SDL_CreateTextureFromSurface(s_App.renderer, s);
	SDL_FreeSurface(s);

	return t;
}

void mns_FreeTexture(Texture* texture)
{
	if (!texture->texture || !texture->data)
		return;

	SDL_DestroyTexture(texture->texture);
	free(texture->data);
}

void mns_DrawTextureWithPos2i(Texture* texture, int x, int y)
{
	SDL_Rect rect = { x, y };
	SDL_QueryTexture(texture->texture, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(s_App.renderer, texture->texture, NULL, &rect);
}

void mns_DrawTextureWithPos2f(Texture* texture, float x, float y)
{
	Point2f p2f = { x, y };
	Point2i p2i = mns_WorldSpaceToScreenSpace(&p2f);
	mns_DrawTextureWithPos2i(texture, p2i.x, p2i.y);
}

void mns_DrawTextureWithBounds2i(Texture* texture, int x, int y, int w, int h)
{
	SDL_Rect rect = { x, y, w, h };
	SDL_RenderCopy(s_App.renderer, texture->texture, NULL, &rect);
}

void mns_DrawTextureWithBounds2f(Texture* texture, float x, float y, float w, float h)
{
	Point2f p2f1 = { x, y }, p2f2 = { w + x, h + y };
	Point2i p2i1 = mns_WorldSpaceToScreenSpace(&p2f1);
	Point2i p2i2 = mns_WorldSpaceToScreenSpace(&p2f2);
	mns_DrawTextureWithBounds2i(texture, p2i1.x, p2i1.y, p2i2.x - p2i1.x, p2i2.y - p2i1.y);
}
