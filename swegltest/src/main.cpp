#include <stdlib.h>
#include <SDL/SDL.h>

//#include <windows.h>

#include "main.h"
#include "font.h"
#include <swegl/swegl.hpp>
#include "freon/OnScopeExit.hpp"

swegl::Scene* BuildScene();
SDL_Surface *InitSDL();
void VideoWorks(SDL_Surface *screen);
int KeyboardWorks();

swegl::Scene *scene;
swegl::Camera *camera;
swegl::ViewPort *viewport1;
swegl::ViewPort *viewport2;
swegl::R000Virtual *renderer1 = NULL;
swegl::R000Virtual *renderer2 = NULL;
Font font("ascii.bmp");
char keys[256];
int zoom = 0;

#if defined(_DEBUG) || defined(DEBUG)
	unsigned int g_trianglesdrawn;
	unsigned int g_pixelsdrawn;
#endif

#if defined(_DEBUG) || defined(DEBUG)
void AssertFailed(char * cond, char * filename, int line)
{
//	char tmp[1024];
//	sprintf(tmp, "[%s] is FALSE at l.%d of %s\n", cond, line, filename);
//	OutputDebugString(tmp);
}
#endif

int main(int argc, char *argv[])
{
	ON_SCOPE_EXIT(SDL_Quit);

	SDL_Surface *screen = InitSDL(); // freed by SDL_Quit
	if (screen == nullptr)
		return -1;

	memset(keys, 0, 256);

	scene = BuildScene();

	//*
	camera = new swegl::Camera(1.0f * SCR_WIDTH/SCR_HEIGHT);
	viewport1 = new swegl::ViewPort(0,0,           SCR_WIDTH,SCR_HEIGHT,screen);
	renderer1 = new swegl::R008NoTexelArtefact(scene, camera, viewport1);
	/**/
	/*
	camera = new Camera((SCR_WIDTH/2.0f)/SCR_HEIGHT);
	viewport1 = new ViewPort(0,0,           SCR_WIDTH/2,SCR_HEIGHT,screen);
	viewport2 = new ViewPort(SCR_WIDTH/2,0, SCR_WIDTH/2,SCR_HEIGHT,screen);
	renderer1 = new R007Bilinear(scene, camera, viewport1);
	renderer2 = new R008NoTexelArtefact(scene, camera, viewport2);
	/**/
	/*
	camera = new Camera((SCR_WIDTH)/(SCR_HEIGHT/2.0f));
	viewport1 = new ViewPort(0,0,            SCR_WIDTH,SCR_HEIGHT/2,screen);
	viewport2 = new ViewPort(0,SCR_HEIGHT/2, SCR_WIDTH,SCR_HEIGHT/2,screen);
	renderer1 = new R008NoTexelArtefact(scene, camera, viewport1);
	renderer2 = new R009Antialiasing(scene, camera, viewport2);
	/**/
	while (1)
	{
		VideoWorks(screen);
		if (int a=KeyboardWorks() < 0)
			return -a;
	}
	return 0;
}


SDL_Surface *InitSDL()
{
	// Initialize SDL's subsystems - in this case, only video.
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		return nullptr;
	}

	SDL_WM_GrabInput(SDL_GRAB_OFF);
	SDL_ShowCursor(0);

	// Register SDL_Quit to be called at exit; makes sure things are
	// cleaned up when we quit.
	atexit(SDL_Quit);

	// Attempt to create a SCR_WIDTHxSCR_HEIGHT window with 32bit pixels.
	return SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 32, SDL_SWSURFACE);
}

swegl::Scene* BuildScene()
{
	swegl::Texture *t = new swegl::Texture("tex.bmp");
	swegl::Texture *bumpmap = new swegl::Texture("bumpmap.bmp");
	swegl::Scene *s = new swegl::Scene();
	//*
	swegl::Tore *tore = new swegl::Tore(20, t);
	tore->GetWorldMatrix().Translate(0.0f, 0.0f, 8.0f);
	//tore->SetBumpMap(bumpmap);
	s->AddMesh(tore);
	/**/

	//*
	swegl::Cube *c = new swegl::Cube(1.0f, t);
	c->GetWorldMatrix().RotateX(0.5f);
	c->GetWorldMatrix().RotateZ(1.5f);
	c->GetWorldMatrix().Translate(0.0f, 0.5f, 6.0f);
	//c->SetBumpMap(bumpmap);
	s->AddMesh(c);
	/**/

	/*
	Texture *dummy_texture = new Texture(0x00FF00FF);
	RectangleTriangle *tri = new RectangleTriangle(0.5f, 0.5f, dummy_texture);
	tri->m_worldmatrix.Translate(-0.2f, -0.2f, 0.8f);
	s->AddMesh(tri);
	/**/
	return s;
}

void VideoWorks(SDL_Surface *screen)
{
	static int totalTicks = 0;
	static int tickCount  = 0;
	static char fps[32] = {0};

	int fpsticks = SDL_GetTicks();

	// Lock surface if needed
	if (SDL_MUSTLOCK(screen))
		if (SDL_LockSurface(screen) < 0)
			return;

	renderer1->Render();
	if (renderer2 != NULL)
		renderer2->Render();

	totalTicks += SDL_GetTicks() - fpsticks;
	tickCount++;

	// Print fps
	if (tickCount == 50)
	{
		sprintf(fps, "%f mspf", totalTicks/50.0);
		totalTicks = 0;
		tickCount  = 0;
	}
	font.Print(fps, 10, 10, screen);
	#if defined(_DEBUG) || defined(DEBUG)
		sprintf_s(fps, "%d tris, %d pixels", g_trianglesdrawn, g_pixelsdrawn);
		font.Print(fps, 10, 26, screen);
		g_trianglesdrawn = g_pixelsdrawn = 0;
	#endif

	// Unlock if needed
	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	// Tell SDL to update the whole screen
	SDL_UpdateRect(screen, 0, 0, SCR_WIDTH, SCR_HEIGHT);
}

int KeyboardWorks()
{
	static int keystick = SDL_GetTicks();
	static float cameraxrotation;
	SDL_Event event;

	// Poll for events, and handle the ones we care about.
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:
				camera->RotateX(-cameraxrotation);
				camera->RotateY(event.motion.xrel / 2000.0f);
				cameraxrotation += event.motion.yrel / 2000.0f;
				camera->RotateX(cameraxrotation);
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					return -10;
				else if (event.key.keysym.sym == SDLK_d)
					keys['d'] = 1;
				else if (event.key.keysym.sym == SDLK_a)
					keys['a'] = 1;
				else if (event.key.keysym.sym == SDLK_w)
					keys['w'] = 1;
				else if (event.key.keysym.sym == SDLK_s)
					keys['s'] = 1;
				else if (event.key.keysym.sym == SDLK_e)
					keys['e'] = 1;
				else if (event.key.keysym.sym == SDLK_q)
					keys['q'] = 1;
				else if (event.key.keysym.sym == SDLK_i)
					keys['i'] = 1;
				else if (event.key.keysym.sym == SDLK_k)
					keys['k'] = 1;
				else if (event.key.keysym.sym == SDLK_l)
					keys['l'] = 1;
				else if (event.key.keysym.sym == SDLK_j)
					keys['j'] = 1;
				else if (event.key.keysym.sym == SDLK_o)
					keys['o'] = 1;
				else if (event.key.keysym.sym == SDLK_u)
					keys['u'] = 1;
				else if (event.key.keysym.sym == SDLK_PLUS) {
					if (zoom < 3) zoom ++;
				}
				else if (event.key.keysym.sym == SDLK_MINUS) {
					if (zoom > 0) zoom --;
				}
				break;

			case SDL_KEYUP:
				// If escape is pressed, return (and thus, quit)

				/*
				else if (event.key.keysym.sym == SDLK_h)
				scene->m_worldmatrix->RotateY(3.14159f / 180.0f);
				else if (event.key.keysym.sym == SDLK_f)
				scene->m_worldmatrix->RotateY(3.14159f / -180.0f);
				else if (event.key.keysym.sym == SDLK_t)
				scene->m_worldmatrix->RotateX(3.14159f / 180.0f);
				else if (event.key.keysym.sym == SDLK_g)
				scene->m_worldmatrix->RotateX(3.14159f / -180.0f);
				else if (event.key.keysym.sym == SDLK_y)
				scene->m_worldmatrix->RotateZ(3.14159f / 180.0f);
				else if (event.key.keysym.sym == SDLK_r)
				scene->m_worldmatrix->RotateZ(3.14159f / -180.0f);
				*/

				if (event.key.keysym.sym == SDLK_ESCAPE)
					return -2;
				else if (event.key.keysym.sym == SDLK_d)
					keys['d'] = 0;
				else if (event.key.keysym.sym == SDLK_a)
					keys['a'] = 0;
				else if (event.key.keysym.sym == SDLK_w)
					keys['w'] = 0;
				else if (event.key.keysym.sym == SDLK_s)
					keys['s'] = 0;
				else if (event.key.keysym.sym == SDLK_e)
					keys['e'] = 0;
				else if (event.key.keysym.sym == SDLK_q)
					keys['q'] = 0;
				else if (event.key.keysym.sym == SDLK_i)
					keys['i'] = 0;
				else if (event.key.keysym.sym == SDLK_k)
					keys['k'] = 0;
				else if (event.key.keysym.sym == SDLK_l)
					keys['l'] = 0;
				else if (event.key.keysym.sym == SDLK_j)
					keys['j'] = 0;
				else if (event.key.keysym.sym == SDLK_o)
					keys['o'] = 0;
				else if (event.key.keysym.sym == SDLK_u)
					keys['u'] = 0;
					break;
			case SDL_QUIT:
				return -3;
		}
	}

	float multiplier = (SDL_GetTicks()-keystick) / 1.0f;
	//if ( (SDL_GetTicks())-keystick > 100 )
	{
		if (keys['d'])
			camera->RotateY(multiplier * 3.14159f / 2000.0f);
		if (keys['a'])
			camera->RotateY(multiplier * 3.14159f / -2000.0f);
		if (keys['w'])
			camera->RotateX(multiplier * 3.14159f / 2000.0f);
		if (keys['s'])
			camera->RotateX(multiplier * 3.14159f / -2000.0f);
		if (keys['e'])
			camera->RotateZ(multiplier * 3.14159f / 2000.0f);
		if (keys['q'])
			camera->RotateZ(multiplier * 3.14159f / -2000.0f);

		if (keys['i'] || keys['k'] || keys['j'] || keys['l'] || keys['o'] || keys['u'])
				camera->RotateX(-cameraxrotation);

		if (keys['i'])
			camera->Translate(0.0f, 0.0f, multiplier * 0.004f);
		if (keys['k'])
			camera->Translate(0.0f, 0.0f, multiplier * -0.004f);
		if (keys['l'])
			camera->Translate(multiplier * 0.004f, 0.0f, 0.0f);
		if (keys['j'])
			camera->Translate(multiplier * -0.004f, 0.0f, 0.0f);
		if (keys['o'])
			camera->Translate(0.0f, multiplier * 0.004f, 0.0f);
		if (keys['u'])
			camera->Translate(0.0f, multiplier * -0.004f, 0.0f);

		if (keys['i'] || keys['k'] || keys['j'] || keys['l'] || keys['o'] || keys['u'])
				camera->RotateX(cameraxrotation);

		keystick = SDL_GetTicks();
	}

	return 0;
}
