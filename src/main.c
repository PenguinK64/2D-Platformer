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
const int worldRightBound = 3300;

const float playerWidth = 30;
const float playerHeight = 40;

float ACCELERATION = 2000.f;
float FRICTION = 3.f;
float JUMP_IMPLUSE = -300.f;
float GRAVITY = 400.f;
float stopspeed;

int score = 0.0f;
int deaths = 0.0f;

Texture2D ArrowTexture;
Rectangle arrowSource;
Rectangle arrowDest;

int coyote_counter = 0;
int coyote_max = 10;
int buffer_counter = 0;
int buffer_max = 6;
bool jumped = true;

bool needsRespawn = false;
bool win = false;
bool blueUnlocked = false;
bool pinkUnlocked = false;


//Ground and Collectable





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
#define MAX_BODIES 40	// old way to define constant
Body bodies[MAX_BODIES];

Body* playerBox;
Body* pushBoxBlue;
Body* plateBlueBody;
Body* pushBoxPink;
Body* platePinkBody;
Body* maingroundBody;

Body* platformBody1;
Body* platformBody2;
Body* platformBody3;
Body* platformBody4;
Body* platformBody5;
Body* platformBody6;
Body* collectableBody;

Body* platformBody7;
Body* platformBody8;


Body* dividingwallBody1;
Body* dividingwallBody2;

Body* doorBlueBody;
Body* doorPinkBody;

Body* JumpPad;

Body* voidBody1;
Body* voidBody2;
Body* voidBody3;
Body* winBody;










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








	//Coyote Time
	if (!b->isGrounded)
	{
		//player is in air
		if (coyote_counter > 0)
		{ 
			coyote_counter -= 1;
		}
		
	}
	else
	{
		//player is on ground
		jumped = false;
		coyote_counter = coyote_max;
	}

	//jump buffer
	if (IsKeyPressed(KEY_UP) || (IsKeyPressed(KEY_SPACE)))
	{
		buffer_counter = buffer_max;
	}
	

	//jump
	if (!jumped)
	{
		if (b->isGrounded || coyote_counter > 0)
		{
			if (buffer_counter > 0)
			{
				b->velocity.y += JUMP_IMPLUSE * b->invMass;

				b->isGrounded = false; // Reset grounded state after jump
				jumped = true;

				buffer_counter = 0;
			}
			
		}
		
	}

	if (buffer_counter > 0)
	{
		buffer_counter -= 1;
	}
	




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
	for (int i = 0; i < MAX_BODIES; i++)
	{
		bodies[i].isGrounded = false;
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

						if (TestAABB(playerBox, voidBody1))
						{
							needsRespawn = true;

						}
						if (TestAABB(playerBox, voidBody2))
						{
							needsRespawn = true;

						}
						if (TestAABB(playerBox, voidBody3))
						{
							needsRespawn = true;

						}


						if (TestAABB(playerBox, collectableBody))
						{


							if (collectableBody->isAlive)
							{
								score += 1;

								collectableBody->isAlive = false;

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
void UpdatePuzzles()
{
	if (TestAABB(pushBoxBlue, plateBlueBody))
	{
		doorBlueBody->isAlive = false;
	}
	else
	{
		doorBlueBody->isAlive = true;
	}


	if (TestAABB(pushBoxPink, platePinkBody))
	{
		doorPinkBody->isAlive = false;
	}
	else
	{
		doorPinkBody->isAlive = true;
	}

	
	
}

void UpdateJumpPads()
{

	if (TestAABB(playerBox, JumpPad))
	{
		JUMP_IMPLUSE = -600.f;
	}
	else
	{
		JUMP_IMPLUSE = -300.f;
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
	Vector2 resetpushBoxBluePosition = (Vector2){ 850, 0.f};
	Vector2 resetPlayerBoxPosition = (Vector2){ 100.f, 300.f };
	Vector2 resetpushBoxPinkPosition = (Vector2){ 1650, -100 };

	switch (g_currentGameState) {
	case GAME_START:
		ACCELERATION = 2000.f;
		if (IsKeyPressed(KEY_ENTER)) {
			g_currentGameState = GAME_RUNNING;
			ResetBox(playerBox, resetPlayerBoxPosition);
			ResetBox(pushBoxBlue, resetpushBoxBluePosition);
			ResetBox(pushBoxPink, resetpushBoxPinkPosition);
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
		ResetBox(pushBoxBlue, resetpushBoxBluePosition);
		if (IsKeyPressed(KEY_ENTER)) {
			score = 0;
			deaths = 0;
			g_currentGameState = GAME_START;
		}
		break;
	case GAME_LOSE:
		ResetBox(playerBox, resetPlayerBoxPosition);
		ResetBox(pushBoxBlue, resetpushBoxBluePosition);
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
	 pushBoxBlue = &bodies[1];
	 pushBoxPink = &bodies[2];

	 maingroundBody = &bodies[3];
	 plateBlueBody = &bodies[4];
	 platePinkBody = &bodies[5];

	 platformBody1 = &bodies[6];
	 platformBody2 = &bodies[7];
	 platformBody3 = &bodies[8];
	 platformBody4 = &bodies[9];
	 platformBody5 = &bodies[10];
	 platformBody6 = &bodies[11];
	 collectableBody = &bodies[12];

	 platformBody7 = &bodies[13];
	 platformBody8 = &bodies[14];


	 dividingwallBody1 = &bodies[15];
	 dividingwallBody2 = &bodies[16];

	 doorBlueBody = &bodies[17];
	 doorPinkBody = &bodies[18];

	JumpPad = &bodies[19];

	voidBody1 = &bodies[20];
	voidBody2 = &bodies[21];
	voidBody3 = &bodies[22];
	winBody = &bodies[23];
	


	 // Initialise Static bodies

	 InitBody(maingroundBody, (Vector2) { 0.f, 400.f }, (Vector2) { 2300.f, 400.f }, 0.f, DARKGREEN);

	 InitBody(platformBody1, (Vector2) { 500.0f, 300.0f }, (Vector2) { 150.0f, 20.0f }, 0.0f, DARKGREEN);
	 InitBody(platformBody2, (Vector2) { 900.f, 190.f }, (Vector2) { 150.0f, 20.0f }, 0.f, DARKGREEN);
	 InitBody(platformBody3, (Vector2) { 300.f, 150.f }, (Vector2) { 150.0f, 20.0f }, 0.f, DARKGREEN);
	 InitBody(platformBody4, (Vector2) { 800.f, 50.f }, (Vector2) { 150.0f, 20.0f }, 0.f, DARKGREEN);
	 InitBody(platformBody5, (Vector2) { 700.f, -50.f }, (Vector2) { 150.0f, 20.0f }, 0.f, DARKGREEN);
	 InitBody(platformBody6, (Vector2) { 1000.f, -150.f }, (Vector2) { 150.0f, 20.0f }, 0.f, DARKGREEN);

	 InitBody(platformBody7, (Vector2) { 1550.f, -50.f }, (Vector2) { 500.0f, 100.0f }, 0.f, DARKGREEN);
	 InitBody(platformBody8, (Vector2) { 3000.f, 0.f }, (Vector2) { 300.0f, 50.0f }, 0.f, DARKGREEN);

	 InitBody(dividingwallBody1, (Vector2) { 1100.f, -700.f }, (Vector2) { 300.0f, 975.0f }, 0.f, DARKBROWN);
	 InitBody(dividingwallBody2, (Vector2) { 3000.f, -700.f }, (Vector2) { 500.0f, 500.0f }, 0.f, DARKBROWN);


	//Initalise Moving bodies
	
	InitBody(playerBox, (Vector2) { 100.f, 300.f }, (Vector2) { playerWidth, playerHeight }, 1.f, BLACK);
	playerBox->isPlayer = true;


	// init puzzle bodies
	InitBody(pushBoxBlue, (Vector2) { 850, 0 }, (Vector2) { 50.0f, 50.0f }, 1.0f, DARKBLUE);
	pushBoxBlue->isAlive = true;
	InitBody(plateBlueBody, (Vector2) { 960.f, 400.f }, (Vector2) { 70.0f, 30.0f }, 0.f, DARKBLUE);
	InitBody(doorBlueBody, (Vector2) { 1225.f, 200.f }, (Vector2) { 50.0f, 200.0f }, 0.f, DARKBLUE);

	InitBody(pushBoxPink, (Vector2) { 5000, -5000 }, (Vector2) { 50.0f, 50.0f }, 1.0f, PINK);
	pushBoxPink->isAlive = true;
	InitBody(platePinkBody, (Vector2) { 1700.f, 400.f }, (Vector2) { 70.0f, 30.0f }, 0.f, PINK);
	InitBody(doorPinkBody, (Vector2) {3100.f, -200.f }, (Vector2) { 50.0f, 200.0f }, 0.f, PINK);
	
	InitBody(JumpPad, (Vector2) { 2200.f, 380.f }, (Vector2) { 80.0f, 20.0f }, 0.f, DARKPURPLE);

	InitBody(voidBody1, (Vector2) { 2300.f, 450.f }, (Vector2) { 1000.f, 100.f }, 0.f, RED);
	InitBody(voidBody2, (Vector2) { 490.f, 370.f }, (Vector2) { 170.f, 30.f }, 0.f, RED);
	InitBody(voidBody3, (Vector2) { 1750.f, -80.f }, (Vector2) { 150.f, 30.f }, 0.f, RED);
	
	InitBody(winBody, (Vector2) { 3250.f, -200.f }, (Vector2) { 50.f, 200.f }, 0.f, GOLD);
	


	//intialise collectable body
	InitBody(collectableBody, (Vector2) { 1040.0f, -300.0f }, (Vector2) { 20.0f, 20.0f }, 0.0f, GOLD);
	
		
	


}
Camera2D camera = { 0 };

void DrawGame()
{
	BeginDrawing();
	BeginMode2D(camera);
	ClearBackground(BLUE);

	//clouds
	

	//bounds
	DrawRectangle(-500, -2000, 500, 3000, DARKBROWN);
	DrawRectangle(worldRightBound, -2000, 500, 3000, DARKBROWN);
	
	DrawTextureEx(ArrowTexture, (Vector2) { 100, 300 }, 0, 0.1, WHITE);
	DrawTextureEx(ArrowTexture, (Vector2) { 800, 300 }, -90, 0.1, WHITE);
	DrawTextureEx(ArrowTexture, (Vector2) { 750, -250 }, 0, 0.1, WHITE);
	DrawTextureEx(ArrowTexture, (Vector2) { 2210, 300 }, -90, 0.1, WHITE);
	DrawTextureEx(ArrowTexture, (Vector2) { 2500, -260 }, 0, 0.1, WHITE);
	DrawTextureEx(ArrowTexture, (Vector2) { 2000, -200 }, -180, 0.1, WHITE);

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
	
	SetTraceLogLevel(LOG_WARNING);

	SetTargetFPS(60);
	
	stopspeed = ACCELERATION / 2.f;
	
	// Initialise the game state and bodies
	InitialiseGame();

	
	//camera
	camera.target = (Vector2){ 0,0 };
	camera.offset = (Vector2){ (float)windowWidth / 2, (float)windowHeight / 2 , };
	camera.rotation = 0.f;
	camera.zoom = 1.5f;

	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float dt = GetFrameTime();

		ArrowTexture = LoadTexture("Arrow.png");

		camera.target = (Vector2){ bodies[0].position.x, bodies[0].position.y - 100 };
		
		//printf("Texture size: %d x %d\n", ArrowTexture.width, ArrowTexture.height);
		//printf("Working directory: %s\n", GetWorkingDirectory());
		//printf("%f,%f\n", bodies[0].position.x, bodies[0].position.y); //int
		//printf("%d\n", win); //bool
		//printf("%d\n", justDied);

		printf("Grounded: %d | Coyote: %d\n",
			playerBox->isGrounded,
			coyote_counter); 
		 // Update the game state machine based on user input and game conditions
		GameStateMachine(dt);

		// 2. Apply Drag (Friction) only to the X axis. Purposly keep this out of the step physics function to show how drag is applied before physics step.
		StepPhysicsForAllBodies(bodies, dt, GRAVITY);

		UpdatePuzzles();

		UpdateJumpPads();

		// Collision resolution
		ResolveCollisionsForAllBodies(bodies);


		//testingcheatcommands
		if (IsKeyPressed(KEY_E))
		{
			playerBox->position = (Vector2){ 1600, 300.f };
		}
		if (IsKeyPressed(KEY_R))
		{
			playerBox->position = (Vector2){
			100.f, 300.f
			};

			// Drawing


		}
		
		DrawGame();
	

		// cleanup
	}
		// destroy the window and cleanup the OpenGL context
	UnloadTexture(ArrowTexture);
		CloseWindow();
		return 0;

	}