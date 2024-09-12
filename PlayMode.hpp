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
	Scene::Transform *globe = nullptr;
	glm::quat base_rotation;
	glm::vec3 base_position;
	glm::vec3 globe_position;
	float base_speed = 40.0f;
	float bound_radius = 45.0f;
	float globe_elevation = 4.2f;
	float globe_radius = 5.6f;

	// snow
	struct Particle {
		Scene::Transform *transform = nullptr;
		float fall_speed = 10.0f;
		uint32_t id;
	};
	std::vector<Particle> snow;
	float snow_height = 80.0f;
	float snow_height_variation = 30.0f;
	float snowfall_speed = 10.0f;
	float snowfall_speed_variation = 3.0f;
	uint32_t copies = 200;

	void reset_snow_position(uint32_t i); // reset position of snow particle i

	bool game_over = false;
	
	//camera:
	Scene::Camera *camera = nullptr;

};
