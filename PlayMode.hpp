#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	float rotator = 0.0f;
	float total_elapsed = 0.0f;
	float time_limit = 60.0f;
	uint32_t points = 0;

	//base rotation:
	Scene::Transform *base = nullptr;
	glm::quat base_rotation;
	glm::vec3 base_position;
	float base_speed = 40.0f;
	float bound_radius = 50.0f;
	float globe_radius = 5.0f;

	// snow
	struct Particle {
		Scene::Transform *particle_transform = nullptr;
		glm::vec3 position;
	};
	std::vector<Particle> snow;
	float snow_height = 50.0f;

	bool game_over = false;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
