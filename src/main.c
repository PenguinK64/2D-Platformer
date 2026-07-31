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



//Ground and Collectable
bool isGrounded;
bool collected = false;
bool isntcollected = true;

bool win = false;


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




// Returns true if Box A and Box B are overlapping

bool TestAABB(Body bodyA, Body bodyB)
{
	bool overlapX = false;
	bool overlapY = false;

	overlapX = (bodyA.position.x < bodyB.position.x + bodyB.size.x) &&
		(bodyA.position.x + bodyA.size.x > bodyB.position.x);

	overlapY = (bodyA.position.y < bodyB.position.y + bodyB.size.y) &&
		(bodyA.position.y + bodyA.size.y > bodyB.position.y);

	return overlapX && overlapY;
}


// Resolve collision between two bodies by moving the first body out of the second

void ResolveCollision(Body* a, Body b)
{
	float overlapLeft = (a->position.x + a->size.x) - b.position.x;
	float overlapRight = (b.position.x + b.size.x) - a->position.x;
	float overlapTop = (a->position.y + a->size.y) - b.position.y;
	float overlapBottom = (b.position.y + b.size.y) - a->position.y;

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

	if (pushDirection == 0)
	{
		a->position.x -= minOverlap;
		a->velocity.x = 0.0f;
	}
	else if (pushDirection == 1)
	{
		a->position.x += minOverlap;
		a->velocity.x = 0.0f;
	}
	else if (pushDirection == 2)
	{
		a->position.y -= minOverlap;
		a->velocity.y = 0.0f;
	}
	else if (pushDirection == 3)
	{
		a->position.y += minOverlap;
		a->velocity.y = 0.0f;
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

	//Initalise Moving bodies
	Body box;
	InitBody(&box, (Vector2) { 100.f, 300.f }, (Vector2) { playerWidth, playerHeight }, 1.f);


	// Initialise Static bodies
	Body groundBody;
	InitBody(&groundBody, (Vector2) {0.f, 400.f }, (Vector2) { 1000.f, 400.f }, 0.f);

	Body groundBody2;
	InitBody(&groundBody2, (Vector2) { 1500.f, 400.f }, (Vector2) { 500.f, 400.f }, 0.f);

	Body groundBody3;
	InitBody(&groundBody3, (Vector2) { 2500.f, 400.f }, (Vector2) { 3000.f, 400.f }, 0.f);


	Body platformBody;
	InitBody(&platformBody, (Vector2) { 200.0f, 300.0f }, (Vector2) { 150.0f, 20.0f }, 0.0f);

	Body VoidBody;
	InitBody(&VoidBody, (Vector2) { 0.f, 700.f }, (Vector2) { 5000.f, 100.f }, 0.f);

	Body WinBody;
	InitBody(&WinBody, (Vector2) { 4950.f, 300.f }, (Vector2) { 50.f, 100.f }, 0.f);
	


	//intialise collectable body
	Body collectableBody;
	InitBody(&collectableBody, (Vector2) { 275.0f, 200.0f }, (Vector2) { 20.0f, 20.0f }, 0.0f);
		

	
	//camera
	Camera2D camera = { 0 };
	camera.target = (Vector2){ 0,0 };
	camera.offset = (Vector2){ (float)windowWidth / 2, (float)windowHeight / 2, };
	camera.rotation = 0.f;
	camera.zoom = 1.5f;

	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float dt = GetFrameTime();

		camera.target = (Vector2){ box.position.x, box.position.y-20 };

		printf("%f\n", box.position.x);

		//Player Movement
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {

			box.velocity.x += box.accel * dt;
			
		}

		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {

			box.velocity.x -= box.accel * dt;

		}

		if (IsKeyDown(KEY_SPACE) && isGrounded)
		{

			box.velocity.y += JUMP_IMPLUSE * box.invMass;
			isGrounded = false;

		}

		//deceleration
		box.velocity.x -= box.velocity.x * box.friction * dt;
	

		// making sure player can decelerate to 0
		if (abs(box.velocity.x) < stopspeed * dt)
		{
			box.velocity.x = 0;
		}

		//3. step physics
		StepPhysics(&box, dt, GRAVITY);

		
	
		
		// Collision resolution
		if (TestAABB(box, groundBody))
		{
			ResolveCollision(&box, groundBody);
			isGrounded = true;
			
		}

		if (TestAABB(box, groundBody2))
		{
			ResolveCollision(&box, groundBody2);
			isGrounded = true;

		}

		if (TestAABB(box, groundBody3))
		{
			ResolveCollision(&box, groundBody3);
			isGrounded = true;

		}

		if (TestAABB(box, platformBody))
		{
			ResolveCollision(&box, platformBody);
			isGrounded = true;
		}

		if (TestAABB(box, WinBody))
		{
			win = true;
			
		}

		if (TestAABB(box, VoidBody))
		{
			deaths += 1;
			box.position.x = 100.f;
			box.position.y = 300.f;
			box.velocity.x = 0.f;
			box.velocity.y = 0.f;
			printf("%d\n", deaths);
			
		}
		



		if (TestAABB(box, collectableBody))
		{
			collected = true;
	
			if (isntcollected)
			{
				score += 1;
				isntcollected = false;
			}
			
		}

		

		//stop player from going off screen
		if (box.position.x < -0) {

			box.position.x = -0;
			box.velocity.x = 0;
		}
		if (box.position.x > worldRightBound - playerWidth) {
			box.position.x = worldRightBound - playerWidth;
			box.velocity.x = 0;
		}

		// drawing
		BeginDrawing();
		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLUE);


		BeginMode2D(camera);


		//Background
		DrawRectangle(500, 100, 300, 100, WHITE);
		DrawRectangle(2000, 0, 300, 150, WHITE);
		DrawRectangle(1000, -50, 400, 200, WHITE);
		DrawRectangle(3000, 0, 300, 100, WHITE);
		DrawRectangle(4000, 80, 200, 100, WHITE);
		DrawRectangle(4250, 10, 200, 100, WHITE);
		DrawRectangle(3500, 100, 150, 50, WHITE);

			// draw our player to the screen
		DrawRectangleV(box.position, box.size, RED);
		

		DrawRectangleV(groundBody.position, groundBody.size, DARKGREEN);
		DrawRectangleV(groundBody2.position, groundBody2.size, DARKGREEN);
		DrawRectangleV(groundBody3.position, groundBody3.size, DARKGREEN);
		DrawRectangleV(VoidBody.position, VoidBody.size, ORANGE);

		DrawRectangleV(platformBody.position, platformBody.size, DARKBROWN);

		DrawRectangleV(WinBody.position, WinBody.size, GOLD);

		



		//bounds
		DrawRectangle(-500, -2000, 500, 3000, DARKBROWN);
		DrawRectangle(worldRightBound, -2000, 500, 3000, DARKBROWN);

		//making the collectable dissapear after the player collects it
		if (isntcollected)
		{ 
		DrawRectangleV(collectableBody.position, collectableBody.size, GOLD);
		}

		if (win)
		{
			DrawRectangle(box.position.x- 500, box.position.y-400, 1500, 1000, BLUE);
			DrawText("YOU WON!", box.position.x-50, box.position.y-50, 30, RED);
			box.accel = 0.f;
			JUMP_IMPLUSE = 0.f;
			
			
			
		}
		
		if  (!win)
		{
			DrawText(TextFormat("Score: %d", score), (int)(box.position.x), (int)(box.position.y - 20), 10.f, BLACK);
			DrawText(TextFormat("Deaths: %d", deaths), (int)(box.position.x), (int)(box.position.y - 30), 10.f, BLACK);
		}



		EndMode2D;



		//score text
		

		// end the frame and get ready for the next one  (display frame, poll input, etc...)
		EndDrawing();
	}

	// cleanup
	
	// destroy the window and cleanup the OpenGL context
	CloseWindow();
	return 0;
}
