#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint snowglobe_meshes_for_texture = 0;
Load< MeshBuffer > snowglobe_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("snow-globe.pnct"));
	snowglobe_meshes_for_texture = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

GLuint snow_texture = 0;
Load< MeshBuffer > snow_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("snow.pnct"));
	snow_texture = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > snowglobe_scene(LoadTagDefault, []() -> Scene const * {
	Scene s(data_path("snow-globe.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
		Mesh const &mesh = snowglobe_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = snowglobe_meshes_for_texture;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
	uint32_t copies = 200;
	for (uint32_t i = 0; i < copies; i++) {
		s.load(data_path("snow.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name) {
		Mesh const &mesh = snow_meshes->lookup(mesh_name);

		transform->name = "Snow" + std::to_string(i);
		transform->position = {0.0f, 0.0f, -5.0f}; // hide below ground for now

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = snow_texture;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;
		});
	}
	return new Scene(s);
});

void PlayMode::reset_snow_position(uint32_t i) {
	Particle p = snow[i];
	// generate random distance and angle
	// sourced partly from https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
	std::random_device rd;
    std::mt19937 gen(rd());
	// radius distribution is proportional to r
	std::uniform_real_distribution<float> mag(0.0f, 1.0f);
	float r = std::sqrtf(mag(gen)) * (bound_radius - globe_radius);
    std::uniform_real_distribution<float> ang(0.0f, 2.0f * float(M_PI));
	float angle = ang(gen);
	std::uniform_real_distribution<float> alt(snow_height - snow_height_variation, snow_height + snow_height_variation);
	p.transform->position = base_position;
	p.transform->position += glm::vec3{r * std::cos(angle), r * std::sin(angle), alt(gen)};

	std::uniform_real_distribution<float> v(snowfall_speed - snowfall_speed_variation, snowfall_speed + snowfall_speed_variation);
	p.fall_speed = v(gen);
}

PlayMode::PlayMode() : scene(*snowglobe_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Base") base = &transform;
		if (transform.name == "Globe") globe = &transform;
		if (transform.name.substr(0, 4) == "Snow" && transform.name != "Snow_test") {
			Particle p;
			p.transform = &transform;
			p.id = std::stoul(&transform.name[4]);
			snow.push_back(p);
		}
	}
	if (base == nullptr) throw std::runtime_error("Base not found.");

	base_rotation = base->rotation;
	base_position = base->position;
	globe_position = globe->position;

	if (snow.size() < copies) throw std::runtime_error("Not enough snow.");

	for (Particle p: snow) {
		reset_snow_position(p.id);
	}

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (!game_over) {
		// slowly rotates through [0,1):
		rotator += elapsed / 5.0f;
		rotator -= std::floor(rotator);
	}
	for (Particle p: snow) {
		p.transform->position.z -= elapsed * p.fall_speed;
		glm::vec3 cur_center = base->position + globe->position;
		cur_center.z += globe_elevation;
		if (!game_over) {
			if (glm::length(p.transform->position - cur_center) <= globe_radius) {
				points++;
				reset_snow_position(p.id);
			}
			else if (p.transform->position.z < -1.0f) {
				reset_snow_position(p.id);
			}
		}
	}

	base->rotation = base_rotation * glm::angleAxis(
		glm::radians(-360.0f * rotator),
		glm::vec3(0.0f, 0.0f, 1.0f)
	);

	total_elapsed += elapsed;
	game_over = total_elapsed > time_limit;

	// move the globe:
	{

		//combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed) move.x = -1.0f;
		if (!left.pressed && right.pressed) move.x = 1.0f;
		if (down.pressed && !up.pressed) move.y = -1.0f;
		if (!down.pressed && up.pressed) move.y = 1.0f;

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * base_speed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		// glm::vec3 frame_up = frame[1];
		glm::vec3 frame_forward = -frame[2];
		
		// remove z-coordinate to prevent height movement
		frame_right.z = 0.0f;
		frame_forward.z = 0.0f;

		base->position += move.x * frame_right + move.y * frame_forward;
		glm::vec3 diff = base->position - base_position;
		// stay within the circle
		if (glm::length(diff) >= bound_radius) {
			base->position = base_position + bound_radius * glm::normalize(diff);
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(-0.6f, 0.0f,-0.8f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	float time_dark = std::max(0.2f, 0.2f + 0.8f * (1.0f - total_elapsed / time_limit));
	glClearColor(time_dark * 0.5f, time_dark * 0.7f, time_dark * 0.8f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.
	// glEnable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	// glDisable(GL_BLEND);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		std::string info;
		if (game_over) {
			info = "Game over!"
				" | Snow collected: " + std::to_string(points);
		}
		else {
			int time_left = std::max(0, (int)(std::ceilf(time_limit - total_elapsed)));
			info = "WASD to move snow globe"
				" | Snow collected: " + std::to_string(points) +
				" | Time left: " + std::to_string(time_left) + " s";
		}
		lines.draw_text(info,
			glm::vec3(-aspect + 0.1f * H, 0.9f - 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H * 0.9f, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(info,
			glm::vec3(-aspect + 0.1f * H + ofs, 0.9f - 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H * 0.9f, 0.0f),
			glm::u8vec4(0x00, 0x00, 0xff, 0x00));
	}
}
