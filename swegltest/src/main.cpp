
#include <stdlib.h>
#include <sstream>

#include <SDL2/SDL.h>

#include <utttil/perf.hpp>

#include <swegl/swegl.hpp>
#include "main.h"
#include "font.h"

#include <swegl/data/model.hpp>
#include <swegl/render/renderer.hpp>
#include <swegl/render/vertex_shaders.hpp>
#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/post_shaders.hpp>


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

		surface = SDL_GetWindowSurface(window);
		//surface = SDL_CreateRGBSurface(0, SCR_WIDTH, SCR_HEIGHT, 32, 0, 0, 0, 0);
		if (surface == nullptr)
			throw;
	}
	~SDLWrapper()
	{
		SDL_Quit();
	}

	SDL_Surface * make_surface()
	{
		return SDL_CreateRGBSurface(0, SCR_WIDTH, SCR_HEIGHT, 32, 0, 0, 0, 0);
	}
};

swegl::scene_t build_scene();
int KeyboardWorks(SDLWrapper &, swegl::camera_t &, swegl::scene_t & scene);

int main(int argc, char *argv[])
{
	SDLWrapper sdl;

	swegl::scene_t scene = build_scene();
	Font font("ascii.bmp");

	//*
	swegl::camera_t camera(1.0f * SCR_WIDTH/SCR_HEIGHT);
	swegl::viewport_t viewport1(0, 0, SCR_WIDTH,SCR_HEIGHT, sdl.surface);
	float * zbuffer = new float[viewport1.m_w*viewport1.m_h];
	swegl::renderer renderer(scene, camera, viewport1, zbuffer);
	//*/
	
	utttil::measurement_point mp("frame");

	swegl::post_shader_t depth_shader;
	//swegl::post_shader_depth_box depth_shader(5, 5);
	
	while (1)
	{
		{
			utttil::measurement m(mp);

			viewport1.Clear();

			//std::stringstream ss;
			//ss << camera.position();
			//font.Print(ss.str().c_str(), 10, 30, sdl.surface);

			//std::stringstream ss2;
			//ss2 << camera.m_viewmatrix;
			//font.Print(ss2.str().c_str(), 10, 70, sdl.surface);

			renderer.render(depth_shader);

			font.Print(std::to_string(mp.status()/1000000).c_str(), 10, 10, sdl.surface);

			if (int a=KeyboardWorks(sdl, camera, scene) < 0)
				return -a;

			// Tell SDL to update the whole screen
			SDL_UpdateWindowSurface(sdl.window);
		}

	}
	return 0;
}

swegl::scene_t build_scene()
{
	auto texture_dice     = std::make_shared<swegl::texture_t>("dice.bmp");
	auto texture_grid     = std::make_shared<swegl::texture_t>("tex.bmp");
	auto texture_mercator = std::make_shared<swegl::texture_t>("mercator.bmp");
	//swegl::Texture *bumpmap = new swegl::Texture("bumpmap.bmp");
	swegl::scene_t s;

	s.ambient_light_intensity = 0.2f;

	s.sun_direction = swegl::normal_t{1.0, -1.0, -1.0};
	s.sun_direction.normalize();
	s.sun_intensity = 0.0;

	s.point_source_lights.emplace_back(swegl::point_source_light{{0.0, 3.0, -5.0}, 0.6});
	s.point_source_lights.emplace_back(swegl::point_source_light{{0.5, 2.0, -5.0}, 100});

	swegl::vertex_shader_t * vertex_shader_0 = new swegl::vertex_shader_standard;
	swegl::pixel_shader_t  * pixel_shader_0  = new swegl::pixel_shader_light_and_texture<swegl::pixel_shader_lights_flat, swegl::pixel_shader_texture_bilinear>;

	//*
	auto tore = swegl::make_tore(20, texture_grid);
	tore.vertex_shader = vertex_shader_0;
	tore.pixel_shader = pixel_shader_0;
	tore.orientation = swegl::matrix44_t::Identity;
	tore.orientation.RotateZ(0.5);
	tore.position = swegl::vertex_t(0.0f, 0.0f, -7.5f);
	//tore.SetBumpMap(bumpmap);
	s.models.push_back(std::move(tore));
	//*/

	//*
	auto cube = swegl::make_cube(1.0f, texture_dice);
	cube.vertex_shader = vertex_shader_0;
	cube.pixel_shader = pixel_shader_0;
	cube.orientation = swegl::matrix44_t::Identity;
	cube.position = swegl::vertex_t(0.0f, 0.0f, -5.0f);
	//c->SetBumpMap(bumpmap);
	s.models.push_back(std::move(cube));
	//*/

	//*
	auto sphere = swegl::make_sphere(20, 2.0f, texture_mercator);
	sphere.vertex_shader = vertex_shader_0;
	sphere.pixel_shader = pixel_shader_0;
	sphere.orientation = swegl::matrix44_t::Identity;
	sphere.position = swegl::vertex_t(3.0f, 0.0f, -6.0f);
	//c->SetBumpMap(bumpmap);
	s.models.push_back(std::move(sphere));
	//*/

	//*
	auto tri = swegl::make_tri(1, texture_dice);
	tri.vertex_shader = vertex_shader_0;
	tri.pixel_shader = pixel_shader_0;
	tri.orientation = swegl::matrix44_t::Identity;
	tri.position = swegl::vertex_t(1.0f, 1.0f, -5.1f);
	s.models.push_back(std::move(tri));
	//*/

	//*
	auto cube2 = swegl::make_cube(0.1f, texture_dice);
	cube2.vertex_shader = vertex_shader_0;
	cube2.pixel_shader = pixel_shader_0;
	cube2.orientation = swegl::matrix44_t::Identity;
	cube2.position = s.point_source_lights[0].position;
	//c->SetBumpMap(bumpmap);
	s.models.push_back(std::move(cube2));
	//*/
	
	//*
	auto cube3 = swegl::make_cube(0.1f, texture_dice);
	cube3.vertex_shader = vertex_shader_0;
	cube3.pixel_shader = pixel_shader_0;
	cube3.orientation = swegl::matrix44_t::Identity;
	cube3.position = s.point_source_lights[1].position;
	//c->SetBumpMap(bumpmap);
	s.models.push_back(std::move(cube3));
	//*/

	return s;
}


int KeyboardWorks(SDLWrapper & sdl, swegl::camera_t & camera, swegl::scene_t & scene)
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
				else if (event.key.keysym.sym == SDLK_g)
					sdl.keys['g'] = 1;
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
				else if (event.key.keysym.sym == SDLK_g)
					sdl.keys['g'] = 0;
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
		if (sdl.keys['g'])
		{
			static swegl::matrix44_t rot1 = []() { swegl::matrix44_t r = swegl::matrix44_t::Identity; r.RotateZ(0.017); return r; }();
			static swegl::matrix44_t rot2 = []() { swegl::matrix44_t r = swegl::matrix44_t::Identity; r.RotateY(0.04); return r; }();

			scene.point_source_lights[0].position = Transform(scene.point_source_lights[0].position, rot1);
			scene.point_source_lights[1].position = Transform(scene.point_source_lights[1].position, rot2);
			scene.models[4].position = scene.point_source_lights[0].position;
			scene.models[5].position = scene.point_source_lights[1].position;
		}

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
			camera.RotateX(cameraxrotation);

		keystick = SDL_GetTicks();
	}

	return 0;
}
