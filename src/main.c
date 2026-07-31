/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#include <stdlib.h>

#define MOVE_SPEED 350.0f


const float GROUND_Y = 400.0f;

const int windowWidth = 1290;
const int windowHeight = 720;

const float playerWidth = 30;
const float playerHeight = 40;

float ACCELERATION = 800.f;
float FRICTION = 1.f;
float JUMP_IMPLUSE = -80.f;
float GRAVITY = 400.f;
float stopspeed;


// Body structure to hold player data
typedef struct {

	Vector2 position;
	Vector2 size;
	Vector2 velocity;

	float accel;
	float friction;

	Vector2 force;
	float mass;
	float invMass;


}Body;

// Initialise a body with position, size
void InitBody(Body* b, Vector2 pos, Vector2 bounds, float mass) {
	b->position = pos;
	b->size = bounds;
	b->velocity = (Vector2){ 0.0f, 0.0f };

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

//Step the physics for a body
void StepPhysics(Body* b, float dt, float gravity)
{
	if (b->invMass == 0.f)
	{
		return;
	}

	// 1. Calculate acceleration from accumulated forces (a = F / m)
	Vector2 resultingAcceleration;
	resultingAcceleration.x = b->force.x * b->invMass;
	resultingAcceleration.y = b->force.y * b->invMass;

	// 2. Apply gravity to the vertical acceleration
	resultingAcceleration.y += gravity;

	// 3. Integrate acceleration into velocity (Semi-Implicit Euler: Step 1)
	b->velocity.x += resultingAcceleration.x * dt;
	b->velocity.y += resultingAcceleration.y * dt;

	// Integrate velocity into position
	b->position.x += b->velocity.x * dt;
	b->position.y += b->velocity.y * dt;

	// 5. Clear accumulated forces for the next frame
	b->force = (Vector2){ 0.0f, 0.0f };
}







int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Load a texture from the resources directory
	Texture wabbit = LoadTexture("wabbit_alpha.png");


	// Create the window and set FPS
	InitWindow(windowWidth, windowHeight, "2D Platformer from scrach");
	SetTargetFPS(60);
	
	stopspeed = ACCELERATION / 2.f;

	Body box;
	InitBody(&box, (Vector2) { 100.f, 300.f }, (Vector2) { playerWidth, playerHeight }, 1.f);




	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float dt = GetFrameTime();

		//Player Movement
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {

			box.velocity.x += box.accel * dt;

		}

		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {

			box.velocity.x -= box.accel * dt;

		}

		if (IsKeyDown(KEY_SPACE))  
		{

			box.velocity.y += JUMP_IMPLUSE * box.invMass;

		}

		//deceleration
		box.velocity.x -= box.velocity.x * box.friction * dt;
		box.position.x += box.velocity.x * dt;
	

		// making sure player can decelerate to 0
		if (abs(box.velocity.x) < stopspeed * dt)
		{
			box.velocity.x = 0;
		}

		//3. step physics
		StepPhysics(&box, dt, GRAVITY);

		if (box.position.y + box.size.y > GROUND_Y)
		{
			box.position.y = GROUND_Y - box.size.y;
			//printf("%f\n", box.position.y);
			box.velocity.y = 0.f;
		}

		//stop player from going off screen
		if (box.position.x < 0) {

			box.position.x = 0;
			box.velocity.x = 0;
		}
		if (box.position.x > windowWidth - playerWidth) {
			box.position.x = windowWidth - playerWidth;
			box.velocity.x = 0;
		}

		


		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLUE);

		// draw our texture to the screen
		DrawRectangleV(box.position, box.size, RED);
		//awTexture(wabbit, box.position.x, box.position.y, WHITE);
		

		DrawLine(0, (int)GROUND_Y, windowWidth, (int)GROUND_Y, DARKGREEN);
		
		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	// unload our texture so it can be cleaned up
	UnloadTexture(wabbit);

	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
