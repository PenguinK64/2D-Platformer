/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#include <stdlib.h>


const float GROUND_Y = 400.0f;

const int windowWidth = 1290;
const int windowHeight = 720;
const int worldRightBound = 5000;

const float playerWidth = 30;
const float playerHeight = 40;

float ACCELERATION = 2000.f;
float FRICTION = 3.f;
float JUMP_IMPLUSE = -300.f;
float GRAVITY = 400.f;
float stopspeed;

int score = 0.0f;
int deaths = 0.0f;


bool needsRespawn = false;

//Ground and Collectable
bool isGrounded;
bool collected = false;
bool isntcollected = true;

bool win = false;


// Body structure to hold player data
typedef struct {

	bool isAlive;
	bool isAsleep;
	bool isPlayer;
	bool isCollectable;
	bool ResolveCollision;
	bool isGrounded;

	Vector2 position;
	Vector2 size;
	Vector2 velocity;

	float accel;
	float friction;

	Vector2 force;
	float mass;
	float invMass;

	Color color;


}Body;

//const int MAX_BODIES = 16;	// new way to define constant but not working in visual studio
#define MAX_BODIES 16	// old way to define constant
Body bodies[MAX_BODIES];

Body* playerBox;
Body* pushBox;
Body* groundBody1;
Body* groundBody2;
Body* groundBody3;
Body* platformBody1;
Body* voidBody;
Body* winBody;
Body* collectableBody;

typedef enum GameState {
	GAME_START,
	GAME_RUNNING,
	GAME_WIN,
	GAME_LOSE
} GameState;

GameState g_currentGameState = GAME_START;

// Initialise a body with position, size
void InitBody(Body* b, Vector2 pos, Vector2 bounds, float mass, Color color) {

	if (!b) {
		return;
	}

	b->isAlive = true;
	b->ResolveCollision = true;
	b->isAsleep = false;
	b->position = pos;
	b->size = bounds;
	b->velocity = (Vector2){ 0.0f, 0.0f };
	b->color = color;


	b->accel = ACCELERATION;
	b->friction = FRICTION;

	b->force = (Vector2){ 0.f, 0.f };
	b->mass = mass;
	b->invMass = 0.f;

	if (mass > 0.f)
	{
		b->invMass = 1.f / mass;
	}
}




// Returns true if Box A and Box B are overlapping

bool TestAABB(Body* bodyA, Body* bodyB)
{
	// Check for null pointers
	if (!bodyA || !bodyB) {
		return false;
	}
	bool overlapX = false;
	bool overlapY = false;

	overlapX = (bodyA->position.x < bodyB->position.x + bodyB->size.x) &&
		(bodyA->position.x + bodyA->size.x > bodyB->position.x);

	overlapY = (bodyA->position.y < bodyB->position.y + bodyB->size.y) &&
		(bodyA->position.y + bodyA->size.y > bodyB->position.y);

	return overlapX && overlapY;
}



// Resolve collision between two bodies by moving the first body out of the second

void ResolveCollision(Body* a, Body* b)
{
	if (!a || !b) {
		return;
	}

	float overlapLeft = (a->position.x + a->size.x) - b->position.x;
	float overlapRight = (b->position.x + b->size.x) - a->position.x;
	float overlapTop = (a->position.y + a->size.y) - b->position.y;
	float overlapBottom = (b->position.y + b->size.y) - a->position.y;

	float minOverlap = overlapLeft;
	int pushDirection = 0; // 0: Left, 1: Right, 2: Top, 3: Bottom

	if (overlapRight < minOverlap)
	{
		minOverlap = overlapRight;
		pushDirection = 1;
	}

	if (overlapTop < minOverlap)
	{
		minOverlap = overlapTop;
		pushDirection = 2;
	}

	if (overlapBottom < minOverlap)
	{
		minOverlap = overlapBottom;
		pushDirection = 3;
	}

	// Calculate how much each body shuold move based on mass
	float totalInvMass = a->invMass + b->invMass;
	if (totalInvMass == 0.0f) {
		return; // Both bodies are static, no resolution needed
	}

	float ratioA = a->invMass / totalInvMass;
	float ratioB = b->invMass / totalInvMass;

	// Horizontal collision resolution
	if (pushDirection == 0 || pushDirection == 1)
	{
		a->position.x += (pushDirection == 0 ? -minOverlap : minOverlap) * ratioA;
		b->position.x += (pushDirection == 0 ? minOverlap : -minOverlap) * ratioB;

		// If hitting a static wall, stop. Otherwise share momentum.
		if (a->invMass == 0.0f) {
			b->velocity.x = 0.0f;
		}
		else if (b->invMass == 0.0f) {
			a->velocity.x = 0.0f;
		}
		else {
			float totalMass = a->mass + b->mass;
			float combined = (a->velocity.x * a->mass + b->velocity.x * b->mass) / totalMass;
			a->velocity.x = combined;
			b->velocity.x = combined;
		}
	}

	// vertical collision resolution
	else if (pushDirection == 2 || pushDirection == 3)
	{
		a->position.y += (pushDirection == 2 ? -minOverlap : minOverlap) * ratioA;
		b->position.y += (pushDirection == 2 ? minOverlap : -minOverlap) * ratioB;

		// If hitting static floor/ceiling, stop. Otherwise share momentum.
		if (a->invMass == 0.0f) {
			b->velocity.y = 0.0f;
		}
		else if (b->invMass == 0.0f) {
			a->velocity.y = 0.0f;
		}
		else {
			// Share vertical momentum (stop falling through each other)
			float totalMass = a->mass + b->mass;
			float combinedY = (a->velocity.y * a->mass + b->velocity.y * b->mass) / totalMass;
			a->velocity.y = combinedY;
			b->velocity.y = combinedY;

			// Apply surface friction!
			// Whichever body ended up on top should inherit the horizontal velocity
			// of the body underneath it, so they move together (e.g. riding a pushed box).
			// pushDirection was computed before the move, so it tells us unambiguously
			// which body is on top: 2 means a is on top of b, 3 means b is on top of a.
			// Skip this for the player so they can still slide/steer freely while
			// standing on top of something instead of being locked to its velocity.
			if (pushDirection == 2 && !a->isPlayer) {
				a->velocity.x = b->velocity.x;
			}
			else if (pushDirection == 3 && !b->isPlayer) {
				b->velocity.x = a->velocity.x;
			}
		}
	}
}



//Step the physics for a body
void StepPhysics(Body* b, float dt, float gravity)
{
	if (b->invMass == 0.0f)
	{
		return;
	}

	b->velocity.x += dt * (b->force.x * b->invMass);
	b->velocity.y += dt * (gravity + (b->force.y * b->invMass));

	b->position.x += b->velocity.x * dt;
	b->position.y += b->velocity.y * dt;

	b->force = (Vector2){ 0.0f, 0.0f };
}

void HandleInput(Body* b, float dt)
{
	if (!b) {
		return;
	}

	if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
	{

		b->velocity.x += b->accel * dt;

	}

	if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
	{

		b->velocity.x -= b->accel * dt;

	}

	if (IsKeyDown(KEY_UP) || (IsKeyPressed(KEY_SPACE)))
	{
		// Only allow jumping if the body is grounded
		if (b->isGrounded) {
			b->velocity.y += JUMP_IMPLUSE * b->invMass;
			b->isGrounded = false; // Reset grounded state after jump
		}
	}

	//deceleration
	//b->velocity.x -= b->velocity.x * b->friction * dt;

	// making sure player can decelerate to 0
	if (abs(b->velocity.x) < stopspeed * dt)
	{
		b->velocity.x = 0;
	}
	//stop player from going off screen
	if (bodies[0].position.x < -0) {

		bodies[0].position.x = -0;
		bodies[0].position.x = 0;
	}
	if (bodies[0].position.x > worldRightBound - playerWidth) {
		bodies[0].position.x = worldRightBound - playerWidth;
		bodies[0].velocity.x = 0;
	}
}

void StepPhysicsForAllBodies(Body* bodies, float dt, float gravity)
{
	if (!bodies) {
		return;
	}

	for (int i = 0; i < MAX_BODIES; i++) {
		if (bodies[i].isAlive && bodies[i].invMass > 0.0f) {
			// Apply drag to the X axis before stepping the physics
			bodies[i].velocity.x -= bodies[i].velocity.x * bodies[i].friction * dt;
			StepPhysics(&bodies[i], dt, gravity);
		}
	}
}

void ResolveCollisionsForAllBodies(Body* bodies)
{
	if (!bodies) {
		return;
	}

	for (int i = 0; i < MAX_BODIES; i++) {
		if (bodies[i].isAlive && bodies[i].invMass > 0.0f) {
			// Check for collisions with all other bodies, but only check each pair once
			for (int j = i + 1; j < MAX_BODIES; j++) {
				if (i != j && bodies[j].isAlive) {
					if (TestAABB(&bodies[i], &bodies[j])) {

						if (TestAABB(playerBox, winBody))
						{
							win = true;


						}
						
						if (TestAABB(playerBox, voidBody))
						{
							needsRespawn = true;

						}


						if (TestAABB(playerBox, collectableBody))
						{


							if (bodies[9].isAlive)
							{
								score += 1;
								
								bodies[9].isAlive = false;

							}


						}

						// Do a quick isGrounded check to see if the body is on top of another body
						// if one of the bodies is a static body (invMass == 0.0f) and the other is a dynamic body (invMass > 0.0f)
						if (bodies[i].invMass > 0.0f || bodies[j].invMass > 0.0f) {
							if (bodies[i].position.y + bodies[i].size.y <= bodies[j].position.y + 5.0f) {
								bodies[i].isGrounded = true;
							}
						}


						ResolveCollision(&bodies[i], &bodies[j]);

						if (needsRespawn) {
							
							deaths += 1;

							bodies[0].position.x = 100.f;
							bodies[0].position.y = 300.f;
							bodies[0].velocity.x = 0.f;
							bodies[0].velocity.y = 0.f;

							needsRespawn = false;
						}
						
					}
				}
			}
		}
	}

}

void DrawAllBodies(Body* bodies)
{
	if (!bodies) {
		return;
	}

	for (int i = 0; i < MAX_BODIES; i++) {
		
		if (bodies[i].isAlive) {
			DrawRectangleV(bodies[i].position, bodies[i].size, bodies[i].color);
		}
	}
}

void DrawGameUI()
{
	// return state
	switch (g_currentGameState) {
	case GAME_START:
		DrawRectangle(bodies[0].position.x - 500, bodies[0].position.y - 400, 1500, 1000, BLUE);
		win = false;
		DrawText("Puzzle Platformer", bodies[0].position.x-200, bodies[0].position.y-200, 50, GREEN);
		DrawText("Press Enter to Start", bodies[0].position.x - 200, bodies[0].position.y - 100, 20, DARKGRAY);
		break;
	case GAME_RUNNING:
		
		if (!win)
		{
		DrawText(TextFormat("Score: %d \nDeaths: %d", score, deaths), bodies[0].position.x - 400, bodies[0].position.y - 200, 20, DARKGRAY);
		
		}
		//DrawText("Arrows: Move | Space: Jump", 10, windowHeight - 30, 20, WHITE);
		//DrawText("Get Green Box in hole", windowWidth / 2 + 50, windowHeight - 30, 20, WHITE);
	
		break;
	//case GAME_PAUSED:
		//DrawText("Game Paused\nPress Key to Resume", SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 20, DARKGRAY);
		//break;
	case GAME_WIN:
		
		
		DrawRectangle(bodies[0].position.x - 500, bodies[0].position.y - 400, 1500, 1000, BLUE);
		DrawText("YOU WON!\nPress Key to Restart", bodies[0].position.x - 200, bodies[0].position.y - 200, 30, RED);
		//DrawText("        You Win!\nPress Key to Restart", bodies[0].position.x - 100, bodies[0].position.y - 100, 20, DARKGRAY);
		break;
	case GAME_LOSE:
		DrawText("        You Lose!\nPress Key to Restart", bodies[0].position.x - 200, bodies[0].position.y - 200, 20, DARKGRAY);
		break;
	}
}

void ResetBox(Body* box, Vector2 position)
{
	if (!box) {
		return;
	}

	box->position = position;
	box->velocity = (Vector2){ 0.0f, 0.0f };
}

void GameStateMachine(float dt) {

	// If any of the boxes fall below the screen, reset their position to the top of the screen
	Vector2 resetPushBoxPosition = (Vector2){ 300.f, 300.f };
	Vector2 resetPlayerBoxPosition = (Vector2){ 100.f, 300.f };

	switch (g_currentGameState) {
	case GAME_START:
		ACCELERATION = 2000.f;
		if (IsKeyPressed(KEY_ENTER)) {
			g_currentGameState = GAME_RUNNING;
			ResetBox(playerBox, resetPlayerBoxPosition);
			ResetBox(pushBox, resetPushBoxPosition);
		}

		// reset points
		score = 0;
		deaths = 0;

		break;
	case GAME_RUNNING:
		HandleInput(playerBox, dt);
		//win condition
		if (win)
		{
			g_currentGameState = GAME_WIN;
		}
		break;
		
	//case GAME_PAUSED:
		//if (IsKeyPressed(KEY_ENTER)) {
		//	g_currentGameState = GAME_RUNNING;
		//}
		//break;



	case GAME_WIN:
		ACCELERATION = 0;
		ResetBox(playerBox, resetPlayerBoxPosition);
		ResetBox(pushBox, resetPushBoxPosition);
		if (IsKeyPressed(KEY_ENTER)) {
			score = 0;
			deaths = 0;
			g_currentGameState = GAME_START;
		}
		break;
	case GAME_LOSE:
		ResetBox(playerBox, resetPlayerBoxPosition);
		ResetBox(pushBox, resetPushBoxPosition);
		if (IsKeyPressed(KEY_ENTER)) {
			score = 0;
			deaths = 0;
			g_currentGameState = GAME_START;
		}
		break;
	}
}

// Initialise the game state and bodies
// -----------------------------------------------------------------------------
void InitialiseGame() {
	// Get a pointer reference to an array objects in the scene
	playerBox = &bodies[0];
	pushBox = &bodies[1];
	groundBody1 = &bodies[2];
	groundBody2 = &bodies[3];
	groundBody3 = &bodies[4];
	platformBody1 = &bodies[5];
	voidBody = &bodies[6];
	winBody = &bodies[7];
	collectableBody = &bodies[9];

	// Initialise the ground bodies with a gap in the middle  
	//float gap = 55.0f;
	//float groundPieceWidth = ((float)window - gap) / 2.0f;

	//InitBody(groundBody2, (Vector2) { 0.0f, SCREEN_HEIGHT - 50.0f }, (Vector2) { groundPieceWidth, 50.0f }, 0.0f, DARKGRAY);
	//InitBody(groundBody1, (Vector2) { groundPieceWidth + gap, SCREEN_HEIGHT - 50.0f }, (Vector2) { groundPieceWidth, 50.0f }, 0.0f, DARKGRAY);


	

	// inivisible wall to the right of the screen
	//InitBody(&bodies[7], (Vector2) { SCREEN_WIDTH, 0.0f }, (Vector2) { 50.0f, (float)SCREEN_HEIGHT }, 0.0f, DARKGRAY);
	//InitBody(&bodies[8], (Vector2) { 0.0f - 50.0f, 0.0f }, (Vector2) { 50.0f, (float)SCREEN_HEIGHT }, 0.0f, DARKGRAY);


	//Initalise Moving bodies
	
	InitBody(playerBox, (Vector2) { 100.f, 300.f }, (Vector2) { playerWidth, playerHeight }, 1.f, RED);
	playerBox->isPlayer = true;

	// Initialise Static bodies
	
	InitBody(groundBody1, (Vector2) { 0.f, 400.f }, (Vector2) { 1000.f, 400.f }, 0.f, DARKGREEN);
	InitBody(groundBody2, (Vector2) { 1500.f, 400.f }, (Vector2) { 500.f, 400.f }, 0.f, DARKGREEN);
	InitBody(groundBody3, (Vector2) { 2500.f, 400.f }, (Vector2) { 3000.f, 400.f }, 0.f, DARKGREEN);

	// Green box to be pushed always starts above platform 1
	InitBody(pushBox, (Vector2) { 200.0f + 50.0f, 300.0f - 50.0f }, (Vector2) { 50.0f, 50.0f }, 1.0f, GREEN);
	
	InitBody(platformBody1, (Vector2) { 200.0f, 300.0f }, (Vector2) { 150.0f, 20.0f }, 0.0f, BROWN);

	
	InitBody(voidBody, (Vector2) { 0.f, 700.f }, (Vector2) { 5000.f, 100.f }, 0.f, ORANGE);
	voidBody->ResolveCollision = false;
	
	InitBody(winBody, (Vector2) { 4950.f, 300.f }, (Vector2) { 50.f, 100.f }, 0.f, GOLD);
	winBody->ResolveCollision = false;


	//intialise collectable body
	InitBody(collectableBody, (Vector2) { 275.0f, 200.0f }, (Vector2) { 20.0f, 20.0f }, 0.0f, GOLD);
	collectableBody->ResolveCollision = false;
		
	


}
Camera2D camera = { 0 };

void DrawGame()
{
	BeginDrawing();
	BeginMode2D(camera);
	ClearBackground(BLUE);

	//clouds
	DrawRectangle(500, 100, 300, 100, WHITE);
	DrawRectangle(2000, 0, 300, 150, WHITE);
	DrawRectangle(1000, -50, 400, 200, WHITE);
	DrawRectangle(3000, 0, 300, 100, WHITE);
	DrawRectangle(4000, 80, 200, 100, WHITE);
	DrawRectangle(4250, 10, 200, 100, WHITE);
	DrawRectangle(3500, 100, 150, 50, WHITE);

	//bounds
	DrawRectangle(-500, -2000, 500, 3000, DARKBROWN);
	DrawRectangle(worldRightBound, -2000, 500, 3000, DARKBROWN);

	DrawAllBodies(bodies);
	DrawGameUI();
	EndMode2D();
	EndDrawing();
}


int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Create the window and set FPS
	InitWindow(windowWidth, windowHeight, "2D Platformer from scrach");
	SetTargetFPS(60);
	
	stopspeed = ACCELERATION / 2.f;
	
	// Initialise the game state and bodies
	InitialiseGame();

	
	//camera
	camera.target = (Vector2){ 0,0 };
	camera.offset = (Vector2){ (float)windowWidth / 2, (float)windowHeight / 2, };
	camera.rotation = 0.f;
	camera.zoom = 1.5f;

	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float dt = GetFrameTime();

		camera.target = (Vector2){ bodies[0].position.x, bodies[0].position.y-20 };

		//printf("%d\n", score); //int
		//printf("%d\n", win); //bool
		//printf("%d\n", justDied);


		
		 // Update the game state machine based on user input and game conditions
		GameStateMachine(dt);

		// 2. Apply Drag (Friction) only to the X axis. Purposly keep this out of the step physics function to show how drag is applied before physics step.
		StepPhysicsForAllBodies(bodies, dt, GRAVITY);

		// Collision resolution
		ResolveCollisionsForAllBodies(bodies);

		// Drawing
		DrawGame();
		
	}

	// cleanup
	
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
