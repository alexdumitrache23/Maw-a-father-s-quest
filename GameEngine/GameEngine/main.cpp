
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
	float yaw = 0.0f;
	glm::vec3 velocity;
	float health;
	bool active;
	bool attacking;
	float attackTimer; 
	float attackCooldown;
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
	const float wallPadding = 0.5f;

	// Simple box collision
	float a_half_x = a.scale.x * 0.5f;
	float a_half_z = a.scale.z * 0.5f;
	float b_half_x = b.scale.x * 0.5f;
	float b_half_z = b.scale.z * 0.5f;
	if (a.type == 10 || b.type == 10) {
		a_half_x += wallPadding;
		a_half_z += wallPadding;
		b_half_x += wallPadding;
		b_half_z += wallPadding;
	}

	bool collisionX = std::abs(a.pos.x - b.pos.x) < (a_half_x + b_half_x);
	bool collisionZ = std::abs(a.pos.z - b.pos.z) < (a_half_z + b_half_z);

	return collisionX && collisionZ;
}

// Resolve AABB overlap between two objects on XZ plane
void resolveOverlap(GameObject &a, GameObject &b)
{
    if (!a.active || !b.active) return;
    if (!checkCollision(a, b)) return;

	const bool aIsWall = (a.type == 10);
	const bool bIsWall = (b.type == 10);
	const float wallPadding = 0.5f;

    float a_half_x = a.scale.x * 0.5f;
    float a_half_z = a.scale.z * 0.5f;
    float b_half_x = b.scale.x * 0.5f;
    float b_half_z = b.scale.z * 0.5f;
	if (aIsWall || bIsWall) {
		a_half_x += wallPadding;
		a_half_z += wallPadding;
		b_half_x += wallPadding;
		b_half_z += wallPadding;
	}

    float dx = a.pos.x - b.pos.x;
    float dz = a.pos.z - b.pos.z;

    float overlapX = (a_half_x + b_half_x) - std::abs(dx);
    float overlapZ = (a_half_z + b_half_z) - std::abs(dz);

    if (overlapX <= 0.0f && overlapZ <= 0.0f) return;

    // minimal translation vector along smaller overlap
    if (overlapX < overlapZ) {
        float sign = (dx >= 0.0f) ? 1.0f : -1.0f;
        float move = overlapX + 0.001f;
		if (aIsWall && !bIsWall) {
			b.pos.x -= sign * move;
		} else if (bIsWall && !aIsWall) {
			a.pos.x += sign * move;
		} else {
			// if one is player (type 0) move only the other
			if (a.type == 0) {
				b.pos.x -= sign * move;
			} else if (b.type == 0) {
				a.pos.x += sign * move;
			} else {
				a.pos.x += sign * (move * 0.5f);
				b.pos.x -= sign * (move * 0.5f);
			}
		}
    } else {
        float sign = (dz >= 0.0f) ? 1.0f : -1.0f;
        float move = overlapZ + 0.001f;
		if (aIsWall && !bIsWall) {
			b.pos.z -= sign * move;
		} else if (bIsWall && !aIsWall) {
			a.pos.z += sign * move;
		} else {
			if (a.type == 0) {
				b.pos.z -= sign * move;
			} else if (b.type == 0) {
				a.pos.z += sign * move;
			} else {
				a.pos.z += sign * (move * 0.5f);
				b.pos.z -= sign * (move * 0.5f);
			}
		}
    }
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
    Texture t_green_attack; t_green_attack.id = loadBMP("Resources/Textures/green_attack.bmp"); t_green_attack.type = "texture_diffuse";
    Texture t_cat_attack; t_cat_attack.id = loadBMP("Resources/Textures/cat_attack_texture.bmp"); t_cat_attack.type = "texture_diffuse";
	Texture t_sewer_walls; t_sewer_walls.id = loadBMP("Resources/Textures/sewer_walls.bmp"); t_sewer_walls.type = "texture_diffuse";

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
    Mesh greenSphere = loader.loadObj("Resources/Models/sphere.obj", { t_green_attack });
    Mesh furBall = loader.loadObj("Resources/Models/fur_ball.obj", { t_cat_attack });
	Mesh sewerWall = loader.loadObj("Resources/Models/sewer_wall.obj", { t_sewer_walls });
    Mesh catMesh = loader.loadObj("Resources/Models/cat.obj", texCat);

	GameState state = SEWERS;

	GameObject player;
	player.pos = glm::vec3(0.0f, 0.5f, 0.0f);
	player.scale = glm::vec3(1.0f);
	player.health = 100.0f;
	player.active = true;
	player.type = 0;

	// maw's attack
	float catShootCooldown = 1.2f;
	float catShootTimer = catShootCooldown;

	// maw orientation 
	float playerYaw = 10350.0f; // start cat rotated 180 degrees

	// entity vectors
	std::vector<GameObject> enemies;
	std::vector<GameObject> projectiles;
	std::vector<GameObject> furProjectiles;
	std::vector<GameObject> sewerWalls;
	std::vector<GameObject> obstacles;
	std::vector<GameObject> items;

	{
		const float sideOffset = 12.0f / 1.5f;
		const int segments = 16;
		const float segmentSpacing = 7.0f;
		const float y = 0.0f;
		const float zStart = player.pos.z;
		for (int i = 0; i < segments; ++i) {
			float z = zStart - i * segmentSpacing;

			GameObject wl; wl.pos = glm::vec3(player.pos.x - sideOffset, y, z); wl.scale = glm::vec3(6.0f, 6.0f, 6.0f) / 50.0f; wl.active = true; wl.type = 10;
			wl.yaw = 5500.0f;
			GameObject wr; wr.pos = glm::vec3(player.pos.x + sideOffset, y, z); wr.scale = glm::vec3(6.0f, 6.0f, 6.0f) / 50.0f; wr.active = true; wr.type = 10;
			wr.yaw = 5500.0f;

			sewerWalls.push_back(wl);
			sewerWalls.push_back(wr);
		}
	}

	//enemy spawn - place rats in a small non-overlapping radius around p
	auto spawnRat = [&](glm::vec3 p) {
		const float corridorHalfWidth = (12.0f / 1.5f) - 0.8f;
		const float spawnRadius = 3.0f;
		const int maxAttempts = 30;
		GameObject g;
		g.scale = glm::vec3(1.2f);
		g.health = 40.0f;
		g.active = true;
		g.attacking = false;
		g.attackCooldown = 2.0f;
        g.attackTimer = g.attackCooldown; // start with cooldown
		g.type = 1;
		bool placed = false;
		for (int a = 0; a < maxAttempts; ++a) {
			float rx = ((rand() % 1000) / 1000.0f) * 2.0f * spawnRadius - spawnRadius;
			float rz = ((rand() % 1000) / 1000.0f) * 2.0f * spawnRadius - spawnRadius;
			glm::vec3 candidate = glm::vec3(p.x + rx, p.y, p.z + rz);
			if (state == SEWERS) {
				candidate.x = glm::clamp(candidate.x, player.pos.x - corridorHalfWidth, player.pos.x + corridorHalfWidth);
			}
			g.pos = candidate;
			bool coll = false;
			for (auto &ex : enemies) { if (checkCollision(g, ex)) { coll = true; break; } }
			for (auto &ob : obstacles) { if (!coll && checkCollision(g, ob)) { coll = true; break; } }
			for (auto &it : items) { if (!coll && checkCollision(g, it)) { coll = true; break; } }
			if (!coll) { placed = true; break; }
		}
		if (!placed) g.pos = p; // fallback
		enemies.push_back(g);
	};

    // spawn a projectile (green sphere)
    auto spawnProjectile = [&](const glm::vec3 &pos, const glm::vec3 &vel) {
        GameObject p; p.pos = pos; p.velocity = vel; p.scale = glm::vec3(0.002f); p.active = true; p.attacking = false; p.attackTimer = 3.0f; p.type = 8; // type 8 = projectile
        projectiles.push_back(p);
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

    // Sewers Level Setup - spawn rats after a short delay once player starts moving
    bool ratsSpawned = false;
    float ratsSpawnTimer = 0.0f; // seconds until initial rats spawn after movement
    bool playerMoved = false;

	glEnable(GL_DEPTH_TEST);

	while (!window.isPressed(GLFW_KEY_ESCAPE) && glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		static float camYaw = 0.0f;
		static float camPitch = -15.0f;
		const float camRotSpeed = 500.0f;
		if (window.isPressed(GLFW_KEY_LEFT)) camYaw += camRotSpeed * deltaTime;
		if (window.isPressed(GLFW_KEY_RIGHT)) camYaw -= camRotSpeed * deltaTime;
		if (window.isPressed(GLFW_KEY_UP)) camPitch += camRotSpeed * deltaTime;
		if (window.isPressed(GLFW_KEY_DOWN)) camPitch -= camRotSpeed * deltaTime;
		camPitch = glm::clamp(camPitch, -80.0f, 20.0f);

    // spawn rats 5 seconds after the player first moves, in waves: 2,2,3,1,2 (total 10)
    static std::vector<int> ratWaves = {2,2,3,1,2};
    static int currentWaveIndex = 0;
    static float waveTimer = 0.0f;
    const float waveInterval = 5.0f; // seconds between waves (increased to slow spawning)
    const int totalRatsToSpawn = 10;
    static int totalSpawned = 0;

    if (!playerMoved) {
        if (window.isPressed(GLFW_KEY_W) || window.isPressed(GLFW_KEY_A) || window.isPressed(GLFW_KEY_S) || window.isPressed(GLFW_KEY_D)) {
            playerMoved = true;
        }
    }

    if (playerMoved && !ratsSpawned) {
        ratsSpawnTimer += deltaTime;
        if (ratsSpawnTimer >= 5.0f) {
            ratsSpawned = true;
            currentWaveIndex = 0;
            waveTimer = 0.0f;
            totalSpawned = 0;
            std::cout << "LEVEL 1: SEWERS - Defeat 10 Rats!" << std::endl;
        }
    }

    // handle wave spawning after initial delay
    if (ratsSpawned && totalSpawned < totalRatsToSpawn) {
        waveTimer += deltaTime;
        if (waveTimer >= waveInterval && currentWaveIndex < (int)ratWaves.size()) {
            int toSpawn = ratWaves[currentWaveIndex];
            // don't exceed total
            if (totalSpawned + toSpawn > totalRatsToSpawn) toSpawn = totalRatsToSpawn - totalSpawned;
            for (int s = 0; s < toSpawn; ++s) {
                spawnRat(player.pos + glm::vec3((rand() % 10 - 5), 0.0f, (rand() % 10 - 5)));
                totalSpawned++;
            }
            currentWaveIndex++;
            waveTimer = 0.0f;
        }
    }

		// Movement
		float speed = 10.0f * deltaTime;
		if (window.isPressed(GLFW_KEY_W)) player.pos.z -= speed;
		if (window.isPressed(GLFW_KEY_S)) player.pos.z += speed;
		if (window.isPressed(GLFW_KEY_A)) player.pos.x -= speed;
		if (window.isPressed(GLFW_KEY_D)) player.pos.x += speed;

		// Rotate player with Q/E
		bool rotated = false;
		if (window.isPressed(GLFW_KEY_Q)) { playerYaw += 5000.0f * deltaTime; rotated = true; } // degrees per second
		if (window.isPressed(GLFW_KEY_E)) { playerYaw -= 5000.0f * deltaTime; rotated = true; }
		if (rotated) {
			// Yaw Debug
			std::cout << "Player yaw: " << playerYaw << std::endl;
		}

		glm::vec3 camTarget = player.pos + glm::vec3(0.0f, 0.5f, 0.0f);
		glm::vec3 baseOffset(0.0f, 2.0f, 6.0f);
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(camYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		rot = glm::rotate(rot, glm::radians(camPitch), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camPos = camTarget + glm::vec3(rot * glm::vec4(baseOffset, 1.0f));
		camera.setPosition(camPos);
		camera.lookAt(camTarget);

		// Combat (Space to Attack)
		static bool prevSpacePressed = false;
		bool currentSpacePressed = window.isPressed(GLFW_KEY_SPACE);
		catShootTimer += deltaTime;
		if (currentSpacePressed && !prevSpacePressed && catShootTimer >= catShootCooldown) {
			catShootTimer = 0.0f;

			glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(playerYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::vec3 baseDir = glm::normalize(glm::vec3(rot * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
			glm::vec3 rightDir = glm::normalize(glm::cross(baseDir, glm::vec3(0.0f, 1.0f, 0.0f)));

			auto spawnFurProjectile = [&](const glm::vec3& dir) {
				GameObject p;
				p.pos = player.pos + glm::vec3(0.0f, 0.5f, 0.0f) + dir * 0.8f;
				p.velocity = dir * 9.0f;
				p.scale = glm::vec3(0.009f);
				p.active = true;
				p.attacking = false;
				p.attackTimer = 2.0f;
				p.type = 9;
				furProjectiles.push_back(p);
			};

			spawnFurProjectile(baseDir);
			spawnFurProjectile(glm::normalize(baseDir + rightDir * 0.45f));
			spawnFurProjectile(glm::normalize(baseDir - rightDir * 0.45f));
		}
		prevSpacePressed = currentSpacePressed;

		// Rat AI: chase player, melee when close, and spit projectiles when at range
		{
			const float ratAttackDist = 6.0f; // preferred max spit range
			const float ratSpeed = 2.0f;
			// preferred distances
			const float minDistance = 2.0f; // too close, back off
			const float maxDistance = 5.0f; // too far, move closer
			for (auto& e : enemies) {
				if (!e.active || e.type != 1) continue;
				float dx = player.pos.x - e.pos.x;
				float dz = player.pos.z - e.pos.z;
				float dist = std::sqrt(dx * dx + dz * dz);

				if (dist < minDistance) {
					// back off to maintain distance
					glm::vec3 away = glm::normalize(glm::vec3(-dx, 0.0f, -dz));
					e.pos += away * ratSpeed * deltaTime;
					e.attacking = false;
				} else if (dist > maxDistance) {
					// move closer until within optimal range
					glm::vec3 toward = glm::normalize(glm::vec3(dx, 0.0f, dz));
					e.pos += toward * ratSpeed * deltaTime;
					e.attacking = false;
				} else {
					// optimal range: perform ranged attack (spit) with cooldown
					e.attacking = true;
					e.attackTimer -= deltaTime;
                    if (e.attackTimer <= 0.0f) {
                        // aim slightly lower than the cat's Y so projectiles target torso/ground
                        float aimLower = 0.5f; // meters below cat's Y
                        float targetY = player.pos.y - aimLower;
                        float dy = targetY - e.pos.y;
                        glm::vec3 dir = glm::normalize(glm::vec3(dx, dy, dz));
                        spawnProjectile(e.pos + glm::vec3(0.0f, 0.7f, 0.0f), dir * 6.5f);
                        e.attackTimer = e.attackCooldown;
                    }
				}
			}
		}

		// Update projectiles: move, lifetime, and collision with player
		for (auto& p : projectiles) if (p.active) {
			p.pos += p.velocity * deltaTime;
			p.attackTimer -= deltaTime; // lifetime
			float dx = player.pos.x - p.pos.x;
			float dz = player.pos.z - p.pos.z;
			float dist = std::sqrt(dx*dx + dz*dz);
            if (dist < 0.3f) {
                // projectile hit
                player.health -= 25.0f;
                p.active = false;
            }
			if (p.attackTimer <= 0.0f) p.active = false;
		}

		for (auto& f : furProjectiles) if (f.active) {
			f.pos += f.velocity * deltaTime;
			f.attackTimer -= deltaTime;
			for (auto& e : enemies) if (e.active && e.type == 1) {
				if (checkCollision(f, e)) {
					e.active = false;
					f.active = false;
					break;
				}
			}
			if (f.attackTimer <= 0.0f) f.active = false;
		}

		// Resolve overlaps between enemies so they don't stack
		for (size_t i = 0; i < enemies.size(); ++i) {
			for (size_t j = i + 1; j < enemies.size(); ++j) {
				resolveOverlap(enemies[i], enemies[j]);
			}
		}

		// Prevent entities from passing through sewer walls
		if (state == SEWERS) {
			for (auto& w : sewerWalls) if (w.active) {
				resolveOverlap(player, w);
				for (auto& e : enemies) if (e.active) {
					if (e.type == 0) resolveOverlap(e, w);
					else resolveOverlap(w, e);
				}
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
		auto DrawMesh = [&](Mesh& m, glm::vec3 pos, glm::vec3 s, float yawDeg = 0.0f) {
			glm::mat4 Model = glm::mat4(1.0f);
			Model = glm::translate(Model, pos);
			if (yawDeg != 0.0f) {
				Model = glm::rotate(Model, glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f));
			}
			Model = glm::scale(Model, s);
			glm::mat4 MVP = Projection * View * Model;
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(glGetUniformLocation(shader.getId(), "model"), 1, GL_FALSE, &Model[0][0]);
			glUniform1i(glGetUniformLocation(shader.getId(), "useTexture"), 1);
			m.draw(shader);
			};

		if (player.active) {
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
            if (e.type == 1) {
                DrawMesh(ratMesh, e.pos, e.scale);
            } else DrawMesh(sphere, e.pos, e.scale);
        }
        // draw projectiles
        for (auto& p : projectiles) if (p.active) {
            DrawMesh(greenSphere, p.pos, p.scale);
        }
		for (auto& f : furProjectiles) if (f.active) {
			DrawMesh(furBall, f.pos, f.scale);
		}
		if (state == SEWERS) {
			for (auto& w : sewerWalls) if (w.active) {
				DrawMesh(sewerWall, w.pos, w.scale, w.yaw);
			}
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