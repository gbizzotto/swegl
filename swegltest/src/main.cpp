
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <swegl/swegl.hpp>
#include "main.h"
#include "font.h"

#if defined(_DEBUG) || defined(DEBUG)
	unsigned int g_trianglesdrawn;
	unsigned int g_pixelsdrawn;
#endif


class SDLWrapper
{
public:
	char keys[256];
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Surface *surface;

	SDLWrapper()
		:keys{0}
	{
		// Initialize SDL's subsystems - in this case, only video.
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
			throw;
		}

		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_ShowCursor(0);

		window = SDL_CreateWindow("swegl test",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			SCR_WIDTH,
			SCR_HEIGHT,
			0);
		if (window == nullptr)
			throw;

		renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer == nullptr)
			throw;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

		texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,
			SCR_WIDTH, SCR_HEIGHT);
		if (texture == nullptr)
			throw;

		surface = SDL_CreateRGBSurface(0, SCR_WIDTH, SCR_HEIGHT, 32, 0, 0, 0, 0);
		if (surface == nullptr)
			throw;
	}
	~SDLWrapper()
	{
		SDL_Quit();
	}
};

swegl::Scene BuildScene();
void VideoWorks(SDLWrapper &, swegl::Renderer &, Font &);
int KeyboardWorks(SDLWrapper &, swegl::Camera &);

int main(int argc, char *argv[])
{
	SDLWrapper sdl;

	swegl::Scene scene = BuildScene();
	Font font("ascii.bmp");

	//*
	swegl::Camera camera(1.0f * SCR_WIDTH/SCR_HEIGHT);
	swegl::ViewPort viewport1(0, 0, SCR_WIDTH,SCR_HEIGHT, sdl.surface);
	swegl::Renderer renderer1(scene, camera, viewport1);
	/**/
	/*
	camera = new Camera((SCR_WIDTH/2.0f)/SCR_HEIGHT);
	viewport1 = new ViewPort(0,0,           SCR_WIDTH/2,SCR_HEIGHT,screen);
	viewport2 = new ViewPort(SCR_WIDTH/2,0, SCR_WIDTH/2,SCR_HEIGHT,screen);
	renderer1 = new R007Bilinear(scene, camera, viewport1);
	renderer2 = new R008NoTexelArtefact(scene, camera, viewport2);
	//*/
	/*
	camera = new Camera((SCR_WIDTH)/(SCR_HEIGHT/2.0f));
	viewport1 = new ViewPort(0,0,            SCR_WIDTH,SCR_HEIGHT/2,screen);
	viewport2 = new ViewPort(0,SCR_HEIGHT/2, SCR_WIDTH,SCR_HEIGHT/2,screen);
	renderer1 = new R008NoTexelArtefact(scene, camera, viewport1);
	renderer2 = new R009Antialiasing(scene, camera, viewport2);
	//*/
	while (1)
	{
		VideoWorks(sdl, renderer1, font);
		if (int a=KeyboardWorks(sdl, camera) < 0)
			return -a;
	}
	return 0;
}


swegl::Scene BuildScene()
{
	auto texture = std::make_shared<swegl::Texture>("tex.bmp");
	//swegl::Texture *bumpmap = new swegl::Texture("bumpmap.bmp");
	swegl::Scene s;
	//*
	swegl::Mesh tore = swegl::MakeTore(20, texture);
	tore.GetWorldMatrix().Translate(0.0f, 0.0f, 8.0f);
	//tore->SetBumpMap(bumpmap);
	s.push_back(std::move(tore));
	//*/

	//*
	swegl::Mesh cube = swegl::MakeCube(1.0f, texture);
	cube.GetWorldMatrix().RotateX(0.5f);
	cube.GetWorldMatrix().RotateZ(1.5f);
	cube.GetWorldMatrix().Translate(0.0f, 0.5f, 6.0f);
	//c->SetBumpMap(bumpmap);
	s.push_back(std::move(cube));
	/**/

	/*
	Texture *dummy_texture = new Texture(0x00FF00FF);
	RectangleTriangle *tri = new RectangleTriangle(0.5f, 0.5f, dummy_texture);
	tri->m_worldmatrix.Translate(-0.2f, -0.2f, 0.8f);
	s->AddMesh(tri);
	//*/
	return s;
}

void VideoWorks(SDLWrapper & sdl, swegl::Renderer & renderer1, Font & font)
{
	static int totalTicks = 0;
	static int tickCount  = 0;
	static char fps[32] = { 0 };


	int fpsticks = SDL_GetTicks();

	renderer1.Render();

	totalTicks += SDL_GetTicks() - fpsticks;
	tickCount++;

	// Print fps
	if (tickCount == 50)
	{
		sprintf(fps, "%f mspf", totalTicks/50.0);
		totalTicks = 0;
		tickCount  = 0;
	}
	font.Print(fps, 10, 10, sdl.surface);
	#if defined(_DEBUG) || defined(DEBUG)
		static char tris[64] = { 0 };
		sprintf_s(tris, "%d tris, %d pixels", g_trianglesdrawn, g_pixelsdrawn);
		font.Print(tris, 10, 26, sdl.surface);
		g_trianglesdrawn = g_pixelsdrawn = 0;
	#endif

	// Tell SDL to update the whole screen
	SDL_UpdateTexture(sdl.texture, NULL, sdl.surface->pixels, SCR_WIDTH * sizeof(Uint32));
	SDL_RenderCopy(sdl.renderer, sdl.texture, NULL, NULL);
	SDL_RenderPresent(sdl.renderer);
}

int KeyboardWorks(SDLWrapper & sdl, swegl::Camera & camera)
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
				camera.RotateX(-cameraxrotation);
				camera.RotateY(event.motion.xrel / 2000.0f);
				cameraxrotation += event.motion.yrel / 2000.0f;
				camera.RotateX(cameraxrotation);
				break;

			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
					return -10;
				else if (event.key.keysym.sym == SDLK_d)
					sdl.keys['d'] = 1;
				else if (event.key.keysym.sym == SDLK_a)
					sdl.keys['a'] = 1;
				else if (event.key.keysym.sym == SDLK_w)
					sdl.keys['w'] = 1;
				else if (event.key.keysym.sym == SDLK_s)
					sdl.keys['s'] = 1;
				else if (event.key.keysym.sym == SDLK_e)
					sdl.keys['e'] = 1;
				else if (event.key.keysym.sym == SDLK_q)
					sdl.keys['q'] = 1;
				else if (event.key.keysym.sym == SDLK_i)
					sdl.keys['i'] = 1;
				else if (event.key.keysym.sym == SDLK_k)
					sdl.keys['k'] = 1;
				else if (event.key.keysym.sym == SDLK_l)
					sdl.keys['l'] = 1;
				else if (event.key.keysym.sym == SDLK_j)
					sdl.keys['j'] = 1;
				else if (event.key.keysym.sym == SDLK_o)
					sdl.keys['o'] = 1;
				else if (event.key.keysym.sym == SDLK_u)
					sdl.keys['u'] = 1;
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
					sdl.keys['d'] = 0;
				else if (event.key.keysym.sym == SDLK_a)
					sdl.keys['a'] = 0;
				else if (event.key.keysym.sym == SDLK_w)
					sdl.keys['w'] = 0;
				else if (event.key.keysym.sym == SDLK_s)
					sdl.keys['s'] = 0;
				else if (event.key.keysym.sym == SDLK_e)
					sdl.keys['e'] = 0;
				else if (event.key.keysym.sym == SDLK_q)
					sdl.keys['q'] = 0;
				else if (event.key.keysym.sym == SDLK_i)
					sdl.keys['i'] = 0;
				else if (event.key.keysym.sym == SDLK_k)
					sdl. keys['k'] = 0;
				else if (event.key.keysym.sym == SDLK_l)
					sdl.keys['l'] = 0;
				else if (event.key.keysym.sym == SDLK_j)
					sdl.keys['j'] = 0;
				else if (event.key.keysym.sym == SDLK_o)
					sdl.keys['o'] = 0;
				else if (event.key.keysym.sym == SDLK_u)
					sdl.keys['u'] = 0;
					break;
			case SDL_QUIT:
				return -3;
		}
	}

	float multiplier = (SDL_GetTicks()-keystick) / 1.0f;
	//if ( (SDL_GetTicks())-keystick > 100 )
	{
		if (sdl.keys['d'])
			camera.RotateY(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['a'])
			camera.RotateY(multiplier * 3.14159f / -2000.0f);
		if (sdl.keys['w'])
			camera.RotateX(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['s'])
			camera.RotateX(multiplier * 3.14159f / -2000.0f);
		if (sdl.keys['e'])
			camera.RotateZ(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['q'])
			camera.RotateZ(multiplier * 3.14159f / -2000.0f);

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
				camera.RotateX(-cameraxrotation);

		if (sdl.keys['i'])
			camera.Translate(0.0f, 0.0f, multiplier * 0.004f);
		if (sdl.keys['k'])
			camera.Translate(0.0f, 0.0f, multiplier * -0.004f);
		if (sdl.keys['l'])
			camera.Translate(multiplier * 0.004f, 0.0f, 0.0f);
		if (sdl.keys['j'])
			camera.Translate(multiplier * -0.004f, 0.0f, 0.0f);
		if (sdl.keys['o'])
			camera.Translate(0.0f, multiplier * 0.004f, 0.0f);
		if (sdl.keys['u'])
			camera.Translate(0.0f, multiplier * -0.004f, 0.0f);

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
				camera.RotateX(cameraxrotation);

		keystick = SDL_GetTicks();
	}

	return 0;
}
