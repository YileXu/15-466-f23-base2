#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("your_file.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("your_file.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*hexapod_scene) {
	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "AllStuffsFixedOnRope") AllStuffsFixedOnRope = &transform;
		else if (transform.name == "LeftSideBound") LeftSideBound = &transform;
		else if (transform.name == "RightSideBound") RightSideBound = &transform;
	}
	if (AllStuffsFixedOnRope == nullptr) throw std::runtime_error("AllStuffsFixedOnRope not found.");
	if (LeftSideBound == nullptr) throw std::runtime_error("LeftSideBound not found.");
	if (RightSideBound == nullptr) throw std::runtime_error("RightSideBound not found.");

	LeftSideBound->position.x = -1;
	RightSideBound->position.x = 1;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		} else if (evt.key.keysym.sym == SDLK_a) {
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
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left1.downs += 1;
			left1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right1.downs += 1;
			right1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up1.downs += 1;
			up1.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down1.downs += 1;
			down1.pressed = true;
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
		} else if (evt.key.keysym.sym == SDLK_LEFT) {
			left1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up1.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down1.pressed = false;
			return true;
		}
	// } else if (evt.type == SDL_MOUSEBUTTONDOWN) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_FALSE) {
	// 		SDL_SetRelativeMouseMode(SDL_TRUE);
	// 		return true;
	// 	}
	// } else if (evt.type == SDL_MOUSEMOTION) {
	// 	if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
	// 		glm::vec2 motion = glm::vec2(
	// 			evt.motion.xrel / float(window_size.y),
	// 			-evt.motion.yrel / float(window_size.y)
	// 		);
	// 		camera->transform->rotation = glm::normalize(
	// 			camera->transform->rotation
	// 			* glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f))
	// 			* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
	// 		);
	// 		return true;
	// 	}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	//move camera:
	{
		//combine inputs into a move:
		constexpr float PlayerSpeed = 30.0f;
		glm::vec2 move = glm::vec2(0.0f);

		//make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f)) move = glm::normalize(move) * PlayerSpeed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		//glm::vec3 up = frame[1];
		//glm::vec3 frame_forward = -frame[2];

		//camera->transform->position += move.x * frame_right + move.y * frame_forward;
		if (std::abs(ropeState) < 10 && std::floor(wobble*5) != prevWobbleInt) {
			if (!pauseState) {
				if (left.pressed && left1.pressed) {
					move.x = 0.0f;
					pauseState = 1;
				}
				if (left.pressed && up1.pressed) move.x =-1.0f;
				if (left.pressed && right1.pressed) move.x = 0.0f;

				if (up.pressed && left1.pressed) move.x =-1.0f;
				if (up.pressed && up1.pressed) move.x = 0.0f;
				if (up.pressed && right1.pressed) move.x = 1.0f;

				if (right.pressed && left1.pressed) move.x = 0.0f;
				if (right.pressed && up1.pressed) move.x = 1.0f;
				if (right.pressed && right1.pressed) {
					move.x = 0.0f;
					pauseState = 2;
				}
			} else {
				if (pauseState==1 && right1.pressed){
					move.x = 1.0f;
				}
				if (pauseState==2 && left.pressed) {
					move.x =-1.0f;
				}
				pauseState = 0;
			}
			
			AllStuffsFixedOnRope->position += move.x * 0.1f * frame_right;
			ropeState += move.x;
		}
		prevWobbleInt = std::floor(wobble*5);
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
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

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

		if ((int)(10*(1-(wobble*5-std::floor(wobble*5)))) > 0) {
			lines.draw_text(std::to_string((int)(10*(1-(wobble*5-std::floor(wobble*5))))),
				glm::vec3(0, 0.5f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		} else {
			lines.draw_text("Now",
				glm::vec3(0, 0.5f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0xff, 0x00, 0x00));
		}

		if (pauseState == 1) {
			lines.draw_text("You slipped",
				glm::vec3(-1.5f, -0.75f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
			lines.draw_text("Chance",
				glm::vec3(1.3f, -0.75f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0xff, 0x00, 0x00));
		} else if (pauseState == 2) {
			lines.draw_text("You slipped",
				glm::vec3(1.3f, -0.75f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0x00, 0x00, 0x00));
			lines.draw_text("Chance",
				glm::vec3(-1.5f, -0.75f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0xff, 0x00, 0x00));
		}

		lines.draw_text(std::to_string(-ropeState),
				glm::vec3(-1.5f, 0.7f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text("/10",
				glm::vec3(-1.5f+0.1f, 0.7f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text(std::to_string(ropeState),
				glm::vec3(1.3f, 0.7f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text("/10",
				glm::vec3(1.4f, 0.7f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		

		if (ropeState == -10) {
			lines.draw_text("You win",
				glm::vec3(-1.5f, 0.8f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0xff, 0x00, 0x00));
		} else if (ropeState == 10) {
			lines.draw_text("You win",
				glm::vec3(1.3f, 0.8f, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0x00, 0xff, 0x00, 0x00));
		}

		if (left.pressed+up.pressed+right.pressed==1) {
			lines.draw_text("Ready",
				glm::vec3(-1.5f, 0, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		if (left1.pressed+up1.pressed+right1.pressed==1) {
			lines.draw_text("Ready",
				glm::vec3(1.3f, 0, 0),
				glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
	}
}
