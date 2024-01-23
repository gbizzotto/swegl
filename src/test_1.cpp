
#include "headers.hpp"

#include <memory>
#include <stdlib.h>
#include <sstream>

#include <utttil/perf.hpp>

#include <swegl/swegl.hpp>
#include <swegl/misc/font.hpp>
#include <swegl/misc/sdl.hpp>

#include <swegl/data/model.hpp>
#include <swegl/render/renderer.hpp>
#include <swegl/render/vertex_shaders.hpp>
#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/post_shaders.hpp>

swegl::scene_t build_scene()
{
	auto texture_dice     = std::make_shared<swegl::texture_t>("resources/dice.bmp");
	auto texture_grid     = std::make_shared<swegl::texture_t>("resources/tex.bmp");
	auto texture_mercator = std::make_shared<swegl::texture_t>("resources/mercator.bmp");
	//swegl::Texture *bumpmap = new swegl::Texture("bumpmap.bmp");
	swegl::scene_t s;

	s.ambient_light_intensity = 0.2f;

	s.sun_direction = swegl::normal_t{1.0, -1.0, -1.0};
	s.sun_direction.normalize();
	s.sun_intensity = 0.3;

	s.point_source_lights.emplace_back(swegl::point_source_light{{0.0, 3.0, -5.0}, 0.6});
	s.point_source_lights.emplace_back(swegl::point_source_light{{0.5, 2.0, -5.0}, 100});

	//*
	auto tore = swegl::make_tore(100, texture_grid);
	tore.orientation = swegl::matrix44_t::Identity;
	tore.orientation.rotate_z(0.5);
	tore.position = swegl::vertex_t(0.0f, 0.0f, -7.5f);
	//tore.SetBumpMap(bumpmap);
	s.models.emplace_back(std::move(tore));
	//*/

	//*
	auto cube = swegl::make_cube(1.0f, texture_dice);
	cube.orientation = swegl::matrix44_t::Identity;
	cube.position = swegl::vertex_t(0.0f, 0.0f, -5.0f);
	//c->SetBumpMap(bumpmap);
	s.models.emplace_back(std::move(cube));
	//*/

	//*
	auto sphere = swegl::make_sphere(100, 2.0f, texture_mercator);
	sphere.orientation = swegl::matrix44_t::Identity;
	sphere.position = swegl::vertex_t(3.0f, 0.0f, -6.0f);
	//c->SetBumpMap(bumpmap);
	s.models.emplace_back(std::move(sphere));
	//*/

	//*
	auto tri = swegl::make_tri(1, texture_dice);
	tri.orientation = swegl::matrix44_t::Identity;
	tri.position = swegl::vertex_t(1.0f, 2.5f, -5.1f);
	s.models.emplace_back(std::move(tri));
	//*/

	//*
	auto cube2 = swegl::make_cube(0.1f, texture_dice);
	cube2.orientation = swegl::matrix44_t::Identity;
	cube2.position = s.point_source_lights[0].position;
	//c->SetBumpMap(bumpmap);
	s.models.emplace_back(std::move(cube2));
	//*/
	
	//*
	auto cube3 = swegl::make_cube(0.1f, texture_dice);
	cube3.orientation = swegl::matrix44_t::Identity;
	cube3.position = s.point_source_lights[1].position;
	//c->SetBumpMap(bumpmap);
	s.models.emplace_back(std::move(cube3));
	//*/

	for (auto & model : s.models)
		model.mesh.vertices.reserve(model.mesh.vertices.size()+2);

	return s;
}

int handle_keyboard_events(swegl::sdl_t & sdl, swegl::camera_t & camera, swegl::scene_t & scene)
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
				camera.rotate_x(-cameraxrotation);
				camera.rotate_y(event.motion.xrel / 2000.0f);
				cameraxrotation += event.motion.yrel / 2000.0f;
				camera.rotate_x(cameraxrotation);
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
			camera.rotate_y(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['a'])
			camera.rotate_y(multiplier * 3.14159f / -2000.0f);
		if (sdl.keys['w'])
			camera.rotate_x(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['s'])
			camera.rotate_x(multiplier * 3.14159f / -2000.0f);
		if (sdl.keys['e'])
			camera.rotate_z(multiplier * 3.14159f / 2000.0f);
		if (sdl.keys['q'])
			camera.rotate_z(multiplier * 3.14159f / -2000.0f);

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
				camera.rotate_x(-cameraxrotation);

		if (sdl.keys['i'])
			camera.translate(0.0f, 0.0f, multiplier * 0.004f);
		if (sdl.keys['k'])
			camera.translate(0.0f, 0.0f, multiplier * -0.004f);
		if (sdl.keys['l'])
			camera.translate(multiplier * 0.004f, 0.0f, 0.0f);
		if (sdl.keys['j'])
			camera.translate(multiplier * -0.004f, 0.0f, 0.0f);
		if (sdl.keys['o'])
			camera.translate(0.0f, multiplier * 0.004f, 0.0f);
		if (sdl.keys['u'])
			camera.translate(0.0f, multiplier * -0.004f, 0.0f);

		if (sdl.keys['t'])
		{
			scene.models[0].orientation.rotate_x(0.02);
			scene.models[1].orientation.rotate_y(0.01);
			scene.models[1].orientation.rotate_z(0.001);
		}
		if (sdl.keys['g'])
		{
			static swegl::matrix44_t rot1 = []() { swegl::matrix44_t r = swegl::matrix44_t::Identity; r.rotate_z(0.017); return r; }();
			static swegl::matrix44_t rot2 = []() { swegl::matrix44_t r = swegl::matrix44_t::Identity; r.rotate_y(0.040); return r; }();

			scene.point_source_lights[0].position = transform(scene.point_source_lights[0].position, rot1);
			scene.point_source_lights[1].position = transform(scene.point_source_lights[1].position, rot2);
			scene.models[4].position = scene.point_source_lights[0].position;
			scene.models[5].position = scene.point_source_lights[1].position;
		}

		if (sdl.keys['i'] || sdl.keys['k'] || sdl.keys['j'] || sdl.keys['l'] || sdl.keys['o'] || sdl.keys['u'])
			camera.rotate_x(cameraxrotation);

		keystick = SDL_GetTicks();
	}

	return 0;
}


int main()
{
	swegl::sdl_t sdl(10, 1600, 800, 600, "test_1");

	swegl::scene_t scene = build_scene();
	font_t font("resources/ascii.bmp");

	std::shared_ptr<swegl::pixel_shader_t>  pixel_shader_full  = std::make_shared<swegl::pixel_shader_light_and_texture<swegl::pixel_shader_lights_phong, swegl::pixel_shader_texture_bilinear>>();
	std::shared_ptr<swegl::pixel_shader_t>  pixel_shader_basic = std::make_shared<swegl::pixel_shader_light_and_texture<swegl::pixel_shader_lights_flat, swegl::pixel_shader_t>>();
	//std::shared_ptr<swegl::post_shader_t>   post_shader_null   = std::make_shared<swegl::post_shader_t>();
	//std::shared_ptr<swegl::post_shader_t>   post_shader_DOF    = std::make_shared<swegl::post_shader_depth_box>(5, 5);

	//swegl::viewport_t viewport1(200, 000, sdl.w-200, sdl.h- 00, sdl.surface, pixel_shader_full , post_shader_null );
	//swegl::viewport_t viewport2(  0, 30,        200,       300, sdl.surface, pixel_shader_basic, post_shader_null);
	
	swegl::viewport_t viewport(0, 0, sdl.w, sdl.h, sdl.surface, pixel_shader_full);
	swegl::post_shader_depth_box post_shader_DOF(5, 5, viewport);
	swegl::post_shader_t post_shader_null;
	viewport.set_post_shader(post_shader_DOF);

	
	utttil::measurement_point mp("frame");
	for(;;)
	{
		{
			utttil::measurement m(mp);

			sdl.clear(0, 0, 100, 30);

			//swegl::render(scene, viewport1, viewport2);
			swegl::render(scene, viewport);

			font.Print(std::to_string(mp.status()/1000000).c_str(), 10, 10, sdl.surface);

			if (handle_keyboard_events(sdl, viewport.camera(), scene) < 0)
				break;

			sdl.update_frame();
		}

	}
	return 0;
}
