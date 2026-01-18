
#include "Graphics\window.h"
#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>

#define PI 3.14159265359

struct GameObject
{
	glm::vec3 pos;
	glm::vec3 scale;
	glm::vec3 velocity;
	float health;
	bool active;
	int type;
};

enum GameState { SEWERS, STREET, MEN_LASAGNA, KEY_PUZZLE, BOSS_RESCUE, GAME_OVER };

float deltaTime = 0.0f;
float lastFrame = 0.0f;
Window window("Maw: A Father's Quest", 1024, 768);
Camera camera;

// Collision Logic (AABB)
bool checkCollision(const GameObject& a, const GameObject& b)
{
	if (!a.active || !b.active) return false;

	// Simple box collision
	float a_half_x = a.scale.x * 0.5f;
	float a_half_z = a.scale.z * 0.5f; 
	float b_half_x = b.scale.x * 0.5f;
	float b_half_z = b.scale.z * 0.5f;

	bool collisionX = std::abs(a.pos.x - b.pos.x) < (a_half_x + b_half_x);
	bool collisionZ = std::abs(a.pos.z - b.pos.z) < (a_half_z + b_half_z);

	return collisionX && collisionZ;
}

int main()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader waterShader("Shaders/water_vertex_shader.glsl", "Shaders/water_fragment_shader.glsl");
	Shader uiShader("Shaders/ui_vertex_shader.glsl", "Shaders/ui_fragment_shader.glsl");

	MeshLoaderObj loader;

	Texture t_wood; t_wood.id = loadBMP("Resources/Textures/wood.bmp"); t_wood.type = "texture_diffuse";
	Texture t_rock; t_rock.id = loadBMP("Resources/Textures/rock.bmp"); t_rock.type = "texture_diffuse";
    Texture t_orange; t_orange.id = loadBMP("Resources/Textures/orange.bmp"); t_orange.type = "texture_diffuse";
    Texture t_dirty_water; t_dirty_water.id = loadBMP("Resources/Textures/dirty_water.bmp"); t_dirty_water.type = "texture_diffuse";
    Texture t_rat; t_rat.id = loadBMP("Resources/Textures/mouse.bmp"); t_rat.type = "texture_diffuse";
    Texture t_cat; t_cat.id = loadBMP("Resources/Textures/cat_color.bmp"); t_cat.type = "texture_diffuse";

    std::vector<Texture> texWood = { t_wood };
    std::vector<Texture> texRock = { t_rock };
    std::vector<Texture> texOrange = { t_orange };
    std::vector<Texture> texWater = { t_dirty_water };
    std::vector<Texture> texRat = { t_rat };
    std::vector<Texture> texCat = { t_cat };

    Mesh sphere = loader.loadObj("Resources/Models/sphere.obj", texOrange);
	Mesh cube = loader.loadObj("Resources/Models/cube.obj", texWood);
	Mesh terrain = loader.loadObj("Resources/Models/plane1.obj", texRock);
	Mesh water = loader.loadObj("Resources/Models/plane1.obj", texWater);
    Mesh ratMesh = loader.loadObj("Resources/Models/rat.obj", texRat);
    Mesh catMesh = loader.loadObj("Resources/Models/cat.obj", texCat);

	GameState state = SEWERS;

	GameObject player;
	player.pos = glm::vec3(0.0f, 0.5f, 0.0f);
	player.scale = glm::vec3(1.0f);
	player.health = 100.0f;
	player.active = true;
	player.type = 0;

	// maw's attack
	bool attackInProgress = false;
	float attackTimer = 0.0f;
	const float attackDuration = 0.5f; 
	const float attackHitTime = 0.25f; 
	bool attackHitApplied = false;

	// maw orientation 
	float playerYaw = 0.0f;

	// entity vectors
	std::vector<GameObject> enemies;
	std::vector<GameObject> obstacles;
	std::vector<GameObject> items;

	//enemy spawn
	auto spawnRat = [&](glm::vec3 p) {
		GameObject g; g.pos = p; g.scale = glm::vec3(0.5f); g.health = 20.0f; g.active = true; g.type = 1;
		enemies.push_back(g);
		};
	auto spawnCar = [&](glm::vec3 p) {
		GameObject g; g.pos = p; g.scale = glm::vec3(2.0f, 1.0f, 4.0f); g.velocity = glm::vec3(15.0f, 0.0f, 0.0f); g.active = true; g.type = 4;
		obstacles.push_back(g);
		};
	auto spawnMan = [&](glm::vec3 p) {
		GameObject g; g.pos = p; g.scale = glm::vec3(1.0f, 2.0f, 1.0f); g.health = 50.0f; g.active = true; g.type = 2;
		enemies.push_back(g);
		};
	auto spawnBoss = [&](glm::vec3 p) {
		GameObject g; g.pos = p; g.scale = glm::vec3(3.0f); g.health = 200.0f; g.active = true; g.type = 3;
		enemies.push_back(g);
		};

	// Sewers Level Setup
	for (int i = 0; i < 10; i++) {
		spawnRat(glm::vec3((rand() % 20 - 10), 0.5f, (rand() % 20 - 10)));
	}
	std::cout << "LEVEL 1: SEWERS - Defeat 5 Rats!" << std::endl;

	glEnable(GL_DEPTH_TEST);

	while (!window.isPressed(GLFW_KEY_ESCAPE) && glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Movement
		float speed = 10.0f * deltaTime;
		if (window.isPressed(GLFW_KEY_W)) player.pos.z -= speed;
		if (window.isPressed(GLFW_KEY_S)) player.pos.z += speed;
		if (window.isPressed(GLFW_KEY_A)) player.pos.x -= speed;
		if (window.isPressed(GLFW_KEY_D)) player.pos.x += speed;

		// Rotate player with Q/E
		bool rotated = false;
		if (window.isPressed(GLFW_KEY_Q)) { playerYaw += 1500.0f * deltaTime; rotated = true; } // degrees per second
		if (window.isPressed(GLFW_KEY_E)) { playerYaw -= 1500.0f * deltaTime; rotated = true; }
		if (rotated) {
			// Yaw Debug
			std::cout << "Player yaw: " << playerYaw << std::endl;
		}

		// Camera Rotation
		if (window.isPressed(GLFW_KEY_LEFT)) camera.rotateOy(50.0f * deltaTime);
		if (window.isPressed(GLFW_KEY_RIGHT)) camera.rotateOy(-50.0f * deltaTime);
		if (window.isPressed(GLFW_KEY_UP)) camera.rotateOx(50.0f * deltaTime);
		if (window.isPressed(GLFW_KEY_DOWN)) camera.rotateOx(-50.0f * deltaTime);
		glm::vec3 camPos = camera.getCameraPosition();

		// Combat (Space to Attack)
		static bool prevSpacePressed = false;
		bool currentSpacePressed = window.isPressed(GLFW_KEY_SPACE);
		// Start attack
		if (currentSpacePressed && !prevSpacePressed && !attackInProgress) {
			attackInProgress = true;
			attackTimer = 0.0f;
			attackHitApplied = false;
			std::cout << "Attack started" << std::endl;
		}
		prevSpacePressed = currentSpacePressed;

	
		const float attackRange = 3.0f;
		const float attackDamage = 40.0f;
		if (attackInProgress) {
			attackTimer += deltaTime;
			if (!attackHitApplied && attackTimer >= attackHitTime) {
				for (auto& e : enemies) {
					if (!e.active) continue;
					float dx = player.pos.x - e.pos.x;
					float dz = player.pos.z - e.pos.z;
					float dist = std::sqrt(dx * dx + dz * dz);
					if (dist < attackRange) {
						e.health -= attackDamage;
						if (e.health <= 0) e.active = false;
						std::cout << "Hit enemy at dist=" << dist << " newHealth=" << e.health << std::endl;
					}
				}
				attackHitApplied = true;
			}
			if (attackTimer >= attackDuration) {
				attackInProgress = false;
				attackTimer = 0.0f;
				attackHitApplied = false;
				std::cout << "Attack ended" << std::endl;
			}
		}

		if (state == SEWERS) {
			int deadRats = 0;
			for (auto& e : enemies) if (!e.active && e.type == 1) deadRats++;

			if (deadRats >= 5) {
				state = STREET;
				enemies.clear();
				for (int i = 0; i < 5; i++) spawnCar(glm::vec3(-50.0f, 0.5f, -10.0f - (i * 5.0f)));
				std::cout << "LEVEL 2: STREET - Avoid Cars!" << std::endl;
			}
		}
		else if (state == STREET) {
			for (auto& o : obstacles) {
				if (o.type == 4) {
					o.pos.x += 15.0f * deltaTime;
					if (o.pos.x > 50.0f) o.pos.x = -50.0f; 
					if (checkCollision(player, o)) player.health -= 10.0f * deltaTime;
				}
			}
			if (player.pos.z < -30.0f) {
				state = MEN_LASAGNA;
				obstacles.clear();
				spawnMan(glm::vec3(-2.0f, 1.0f, -40.0f));
				spawnMan(glm::vec3(2.0f, 1.0f, -40.0f));
				// Lasagna
				GameObject l; l.pos = glm::vec3(0.0f, 0.5f, -45.0f); l.scale = glm::vec3(0.5f); l.active = true; l.type = 5;
				items.push_back(l);
				std::cout << "LEVEL 3: GUARDS - Defeat men and eat Lasagna!" << std::endl;
			}
		}
		else if (state == MEN_LASAGNA) {
			bool menDead = true;
			for (auto& e : enemies) if (e.active) menDead = false;

			// Check Lasagna Collision
			for (auto& i : items) {
				if (i.active && i.type == 5 && checkCollision(player, i)) {
					i.active = false;
					player.health = 100.0f; // Heal
					if (menDead) {
						state = KEY_PUZZLE;
						items.clear();
						// Key Item
						GameObject k; k.pos = glm::vec3(0.0f, 0.5f, -50.0f); k.scale = glm::vec3(0.3f); k.active = true; k.type = 6;
						items.push_back(k);
						std::cout << "LEVEL 4: PUZZLE - Find the Key!" << std::endl;
					}
				}
			}
		}
		else if (state == KEY_PUZZLE) {
			for (auto& i : items) {
				if (i.active && i.type == 6 && checkCollision(player, i)) {
					i.active = false;
					state = BOSS_RESCUE;
					spawnBoss(glm::vec3(0.0f, 1.5f, -60.0f));
					// Kitten (Objective)
					GameObject k; k.pos = glm::vec3(0.0f, 0.5f, -70.0f); k.scale = glm::vec3(0.5f); k.active = true; k.type = 7;
					items.push_back(k);
					std::cout << "LEVEL 5: BOSS - Save the Kitten!" << std::endl;
				}
			}
		}
		else if (state == BOSS_RESCUE) {
			bool bossDead = true;
			for (auto& e : enemies) if (e.active && e.type == 3) {
				bossDead = false;
				// Boss Logic: Move towards player
				glm::vec3 dir = player.pos - e.pos;
				float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
				if (len > 0.1f) e.pos += (dir / len) * 2.0f * deltaTime;
				if (checkCollision(player, e)) player.health -= 20.0f * deltaTime;
			}

			if (bossDead) {
				for (auto& i : items) {
					if (i.active && i.type == 7 && checkCollision(player, i)) {
						std::cout << "YOU WIN! KITTEN SAVED!" << std::endl;
						state = GAME_OVER;
					}
				}
			}
		}
		glm::mat4 Projection = glm::perspective(45.0f, (float)window.getWidth() / (float)window.getHeight(), 0.1f, 1000.0f);
		glm::mat4 View = glm::lookAt(camera.getCameraPosition(), camera.getCameraPosition() + camera.getCameraViewDirection(), camera.getCameraUp());

        if (state == SEWERS) {
            waterShader.use();
            {
                glm::mat4 Model = glm::mat4(1.0f);
                Model = glm::translate(Model, glm::vec3(0.0f, -1.0f, 0.0f));
                Model = glm::scale(Model, glm::vec3(50.0f, 1.0f, 50.0f));
                glm::mat4 MVP = Projection * View * Model;
                glUniformMatrix4fv(glGetUniformLocation(waterShader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
                glUniform1f(glGetUniformLocation(waterShader.getId(), "time"), currentFrame);
                water.draw(waterShader);
            }
        }

		shader.use();
		glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), 1.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), 0.0f, 50.0f, 0.0f);
		glm::vec3 camP = camera.getCameraPosition();
		glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camP.x, camP.y, camP.z);
		auto DrawMesh = [&](Mesh& m, glm::vec3 pos, glm::vec3 s) {
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, pos);
			Model = glm::scale(Model, s);
			glm::mat4 MVP = Projection * View * Model;
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &Model[0][0]);
			glUniform1i(glGetUniformLocation(shader.getId(), "useTexture"), 1);
			m.draw(shader);
			};

		if (player.active) {
			if (attackInProgress) {
				
				float t = attackTimer / attackDuration; 
				float swing = -4.0f * (t - 0.5f) * (t - 0.5f) + 1.0f;
				glm::vec3 attackOffset = glm::vec3(0.0f, 0.2f * swing, -0.3f * swing);
				glm::vec3 attackScale = player.scale * (1.0f + 0.1f * swing);
				glm::mat4 Model = glm::mat4(1.0f);
				Model = glm::translate(Model, player.pos + attackOffset);
				Model = glm::rotate(Model, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
				Model = glm::scale(Model, attackScale);
				glm::mat4 MVP = Projection * View * Model;
				glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &Model[0][0]);
				glUniform1i(glGetUniformLocation(shader.getId(), "useTexture"), 1);
				catMesh.draw(shader);
			} else {
				glm::mat4 idleModel = glm::mat4(1.0f);
				idleModel = glm::translate(idleModel, player.pos);
				idleModel = glm::rotate(idleModel, glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
				idleModel = glm::scale(idleModel, player.scale);
				glm::mat4 idleMVP = Projection * View * idleModel;
				glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &idleMVP[0][0]);
				glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &idleModel[0][0]);
				glUniform1i(glGetUniformLocation(shader.getId(), "useTexture"), 1);
				catMesh.draw(shader);
			}
		}

		if (state == SEWERS) {
			glm::mat4 ModelGray = glm::mat4(1.0f);
			ModelGray = glm::translate(ModelGray, glm::vec3(0.0f, -1.5f, 0.0f));
			ModelGray = glm::scale(ModelGray, glm::vec3(10.0f, 1.0f, 50.0f));
			glm::mat4 MVPGray = Projection * View * ModelGray;
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &MVPGray[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &ModelGray[0][0]);
			glUniform1i(glGetUniformLocation(shader.getId(), "useTexture"), 0);
			glUniform3f(glGetUniformLocation(shader.getId(), "overrideColor"), 0.4f, 0.4f, 0.4f);
			terrain.draw(shader);
		} else {
			DrawMesh(terrain, glm::vec3(0.0f, -1.5f, 0.0f), glm::vec3(10.0f, 1.0f, 50.0f));
		}

        for (auto& e : enemies) if (e.active) {
            if (e.type == 1) DrawMesh(ratMesh, e.pos, e.scale); else DrawMesh(sphere, e.pos, e.scale);
        }
        for (auto& o : obstacles) if (o.active) DrawMesh(cube, o.pos, o.scale);
        for (auto& i : items) if (i.active) {
            if (i.type == 7) DrawMesh(catMesh, i.pos, i.scale); else DrawMesh(cube, i.pos, i.scale);
        }

		// Health Bar
		glDisable(GL_DEPTH_TEST); // Draw on top
		uiShader.use();

		glm::mat4 uiModel = glm::mat4(1.0f);
		float hpRatio = player.health / 100.0f;
		if (hpRatio < 0) hpRatio = 0;
		uiModel = glm::translate(uiModel, glm::vec3(-0.8f, 0.9f, 0.0f)); // Top Left
		uiModel = glm::scale(uiModel, glm::vec3(0.2f * hpRatio, 0.05f, 1.0f));

		glUniformMatrix4fv(glGetUniformLocation(uiShader.getId(), "model"), 1, GL_FALSE, &uiModel[0][0]);
		glUniform3f(glGetUniformLocation(uiShader.getId(), "color"), 1.0f, 0.0f, 0.0f);

		cube.draw(uiShader);

		glEnable(GL_DEPTH_TEST);

		window.update();
	}

	return 0;
}

void processKeyboardInput()
{
	
}