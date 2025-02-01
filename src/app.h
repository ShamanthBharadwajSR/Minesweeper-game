#ifndef APP_H
#define APP_H

// forward declarations
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;

// Functions used in main()
int mns_ShouldRun();

void mns_Init(int argc, char** argv);
void mns_Shutdown();

void mns_Update();

void mns_InitFrame();
void mns_RenderFrame();

void mns_PollEvents();

//
//	Point2i/2f
//
typedef struct Point2i
{
	int x, y;
} Point2i;

typedef struct Point2f
{
	float x, y;
} Point2f;

Point2i mns_WorldSpaceToScreenSpace(const Point2f* p2f);
Point2f mns_ScreenSpaceToWorldSpace(const Point2i* p2i);

//
//	Application
//
typedef struct Application
{
	SDL_Window* window;
	SDL_Renderer* renderer;
	const char* appName;
	int width, height, running;
	float delta, aspectRatio;
} Application;

//
//	Texture
//
typedef struct Texture
{
	void* data;
	int width, height;
	SDL_Texture* texture;
} Texture;

Texture mns_CreateTextureFromFile(const char* name);
Texture mns_CreateTextureFromBin(unsigned int* data, int width, int height);
void mns_FreeTexture(Texture* texture);
void mns_DrawTextureWithPos2i(Texture* texture, int x, int y);
void mns_DrawTextureWithPos2f(Texture* texture, float x, float y);
void mns_DrawTextureWithBounds2i(Texture* texture, int x, int y, int w, int h);
void mns_DrawTextureWithBounds2f(Texture* texture, float x, float y, float w, float h);

// Global variables
extern Application* mns_Application;

#endif
