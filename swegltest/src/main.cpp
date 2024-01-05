
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <utttil/perf.hpp>

#include <swegl/swegl.hpp>
#include "main.h"
#include "font.h"

#include <swegl/Data/model.hpp>
#include <swegl/Render/renderer.hpp>

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
		// Initialize SDL's subsystems
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

swegl::scene build_scene();
int KeyboardWorks(SDLWrapper &, swegl::Camera &, swegl::scene & scene);

int main(int argc, char *argv[])
{
	SDLWrapper sdl;

	swegl::scene scene = build_scene();
	Font font("ascii.bmp");

	//*
	swegl::Camera camera(1.0f * SCR_WIDTH/SCR_HEIGHT);
	swegl::ViewPort viewport1(0, 0, SCR_WIDTH,SCR_HEIGHT, sdl.surface);
	float * zbuffer = new float[viewport1.m_w*viewport1.m_h];
	swegl::renderer renderer(scene, camera, viewport1, zbuffer);
	//*/
	
	utttil::measurement_point mp("frame");

	while (1)
	{
		{
			utttil::measurement m(mp);

			viewport1.Clear();
			std::fill(zbuffer, zbuffer+viewport1.m_w*viewport1.m_h, std::numeric_limits<std::remove_pointer<decltype(zbuffer)>::type>::max());

			font.Print(std::to_string(mp.status()/1000000).c_str(), 10, 10, sdl.surface);

			//VideoWorks(sdl, renderer1, font);
			renderer.render();

			if (int a=KeyboardWorks(sdl, camera, scene) < 0)
				return -a;

			// Tell SDL to update the whole screen
			SDL_UpdateTexture(sdl.texture, NULL, sdl.surface->pixels, SCR_WIDTH * sizeof(Uint32));
			SDL_RenderCopy(sdl.renderer, sdl.texture, NULL, NULL);
			SDL_RenderPresent(sdl.renderer);
		}

	}
	return 0;
}

swegl::scene build_scene()
{
	auto texture_dice = std::make_shared<swegl::Texture>("dice.bmp");
	auto texture_grid = std::make_shared<swegl::Texture>("tex.bmp");
	//swegl::Texture *bumpmap = new swegl::Texture("bumpmap.bmp");
	swegl::scene s;

	s.ambient_light_intensity = 0.2f;

	s.sun_direction = swegl::Vec3f{1.0, -1.0, 1.0};
	s.sun_direction.Normalize();
	s.sun_intensity = 0.8;

	s.point_source_lights.emplace_back(swegl::point_source_light{{10.0,10.0,10.0},0.2});

	//*
	auto tore = swegl::make_tore(100, texture_grid);
	tore.orientation = swegl::Matrix4x4::Identity;
	tore.orientation.RotateZ(0.5);
	tore.position = swegl::Vec3f(0.0f, 0.0f, 7.5f);
	//tore.SetBumpMap(bumpmap);
	s.models.push_back(std::move(tore));
	//*/

	//*
	auto cube = swegl::make_cube(1.0f, texture_dice);
	cube.orientation = swegl::Matrix4x4::Identity;
	cube.position = swegl::Vec3f(0.0f, 0.0f, 5.0f);
	//c->SetBumpMap(bumpmap);
	s.models.push_back(std::move(cube));
	//*/

	auto tri = swegl::make_tri(1, texture_dice);
	tri.orientation = swegl::Matrix4x4::Identity;
	tri.position = swegl::Vec3f(0.0f, 0.0f, 5.0f);
	s.models.push_back(std::move(tri));

	return s;
}


int KeyboardWorks(SDLWrapper & sdl, swegl::Camera & camera, swegl::scene & scene)
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
				else if (event.key.keysym.sym == SDLK_t)
					sdl.keys['t'] = 1;
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
				else if (event.key.keysym.sym == SDLK_t)
					sdl.keys['t'] = 0;
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

		if (sdl.keys['t'])
		{
			scene.models[0].orientation.RotateY(0.02);
			scene.models[1].orientation.RotateY(0.01);
			scene.models[1].orientation.RotateX(0.001);
		}

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
			camera.RotateX(cameraxrotation);

		keystick = SDL_GetTicks();
	}

	return 0;
}
