/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "raylib.h"

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#include <stdlib.h>

#include <math.h>


// ==================== COLOUR PALETTE ====================

Color backgroundColor = { 220, 229, 209, 255 };
Color platformColor = { 107, 127, 90, 255 };
Color playerColor = { 229, 140, 58, 255 };

Color collectibleColor = { 242, 201, 76, 255 };
Color bouncePadColor = { 93, 189, 157, 255 };

Color bluePuzzleColor = { 74, 123, 190, 255 };
Color pinkPuzzleColor = { 210, 109, 140, 255 };

Color hazardColor = { 217, 71, 71, 255 };
Color textColor = { 47, 59, 46, 255 };

// ==================== AUDIO & TEXTURES ====================

Texture2D ArrowTexture;
Sound HazardSound;
Sound JumpSound;
Sound WinSound;
Sound CollectableSound;
Sound UnlockDoorSound;

bool Blue_UnlockDoorSound_Played = false;
bool Pink_UnlockDoorSound_Played = false;


// ==================== GAME / WORLD SETTINGS ====================
const float GROUND_Y = 400.0f;

const int windowWidth = 1290;
const int windowHeight = 720;
const int worldRightBound = 3300;

const float playerWidth = 30;
const float playerHeight = 40;



int score = 0.0f;
int deaths = 0.0f;

bool needsRespawn = false;
bool win = false;
bool blueUnlocked = false;
bool pinkUnlocked = false;

// ==================== PHYSICS TUNING ====================
float ACCELERATION = 2000.f;
float FRICTION = 3.f;

float JUMP_IMPLUSE = -300.f;
float JUMP_IMPLUSE_DEF = 0.f;
float JUMP_CUT = 0.8f;

float GRAVITY = 400.f;
float RISE_GRAVITY_MULT = 0.9f;
float FALL_GRAVITY_MULT = 1.5f;

float APEX_SPEED_THRES = 30.0f;
float APEX_GRAVITY_MULT = 0.7f;

float MAX_FALL_SPEED = 400.0f;
float stopspeed;

// Coyote time and jump buffering make jump input more forgiving.

int coyote_counter = 0;
int coyote_max = 10;
int buffer_counter = 0;
int buffer_max = 6;
bool jumped = true;



// ==================== PHYSICS BODY DATA ====================
// Shared data used by the player, platforms, puzzle boxes and other physics bodies.
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

// All bodies are stored in one array so they can be updated and collision-checked in loops.
#define MAX_BODIES 40	
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

// ==================== GAME STATE ====================

typedef enum GameState {
	GAME_START,
	GAME_RUNNING,
	GAME_PAUSED,
	GAME_WIN,
	GAME_LOSE
} GameState;

GameState g_currentGameState = GAME_START;


// ==================== BODY INITIALISATION ====================
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

	// Inverse mass makes static bodies (mass 0) stay fixed during collision resolution.
	if (mass > 0.f)
	{
		b->invMass = 1.f / mass;
	}
}



// ==================== COLLISION DETECTION ====================
// AABB collision: both X and Y ranges must overlap for a collision to occur.
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



// ==================== COLLISION RESOLUTION ====================
// Finds the smallest overlap and separates the bodies along that axis.
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



// ==================== PHYSICS STEP ====================
// Integrates force, gravity and velocity each frame using delta time.

void StepPhysics(Body* b, float dt, float gravity)
{
	if (b->invMass == 0.0f)
	{
		return;
	}

	// Apply horizontal force using inverse mass.

	b->velocity.x += dt * (b->force.x * b->invMass);

	// Choose a gravity multiplier based on whether the body is rising or falling.
	float gravityMult = 1.0f;

	if (b->velocity.y < 0.f)
	{
		//Rising
		gravityMult = RISE_GRAVITY_MULT;

	}
	else
	{
		// Falling
		gravityMult = FALL_GRAVITY_MULT;
	}


	// Apex hang: reduce gravity briefly when vertical velocity is close to zero.
	if (fabs(b->velocity.y) < APEX_SPEED_THRES)
	{
		gravityMult *= APEX_GRAVITY_MULT;
	}


	// Apply the customised gravity calculation.
	b->velocity.y += gravity * gravityMult * dt;

	// Apply any additional vertical force.
	b->velocity.y += dt * (b->force.y * b->invMass);


	// Limit maximum fall speed.
	if (b->velocity.y > MAX_FALL_SPEED)
	{
		b->velocity.y = MAX_FALL_SPEED;
	}


	// Integrate velocity into position using delta time.
	b->position.x += b->velocity.x * dt;
	b->position.y += b->velocity.y * dt;

	b->force = (Vector2){ 0.0f, 0.0f };

}

// ==================== PLAYER INPUT ====================
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

	// -------------------- JUMP ASSISTS --------------------

	// Coyote time: keep jump available briefly after walking off a platform.
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

	// Jump buffer: remember a jump press briefly before the player is able to jump.
	if (IsKeyPressed(KEY_UP) || (IsKeyPressed(KEY_SPACE)))
	{
		buffer_counter = buffer_max;
	}
	

	// Perform a normal or coyote-time jump when a buffered input is available.

	if (!jumped)
	{
		if (b->isGrounded || coyote_counter > 0)
		{

			


			if (buffer_counter > 0)
			{
				b->velocity.y += JUMP_IMPLUSE * b->invMass;
				PlaySound(JumpSound);
				

				b->isGrounded = false; // Reset grounded state after jump
				jumped = true;

				buffer_counter = 0;
			}
			
		}
		
	}

	// Variable jump height: releasing jump while rising cuts the remaining upward velocity.
	if (IsKeyReleased(KEY_UP) || (IsKeyReleased(KEY_SPACE)) && playerBox->velocity.y < 0)
	{
		playerBox->velocity.y *= JUMP_CUT;
	}

	// Count down unused buffered input.
	if (buffer_counter > 0)
	{
		buffer_counter -= 1;
	}
	

	// Snap very small horizontal speeds to zero to avoid endless sliding.
	if (abs(b->velocity.x) < stopspeed * dt)
	{
		b->velocity.x = 0;
	}


	// Keep the player within the horizontal world bounds.
	if (bodies[0].position.x < -0) {

		bodies[0].position.x = -0;
		bodies[0].position.x = 0;
	}
	if (bodies[0].position.x > worldRightBound - playerWidth) {
		bodies[0].position.x = worldRightBound - playerWidth;
		bodies[0].velocity.x = 0;
	}
}

// ==================== PHYSICS UPDATE ====================

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

// Checks collision pairs for all active bodies and resolves each pair once.

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


						// Hazard bodies trigger a player respawn on collision.
						if (TestAABB(playerBox, voidBody1))
						{
							needsRespawn = true;
							PlaySound(HazardSound);

						}
						if (TestAABB(playerBox, voidBody2))
						{
							needsRespawn = true;
							PlaySound(HazardSound);

						}
						if (TestAABB(playerBox, voidBody3))
						{
							needsRespawn = true;
							PlaySound(HazardSound);

						}


						// Collecting the gold body increases score once, then disables the collectable.
						if (TestAABB(playerBox, collectableBody))
						{


							if (collectableBody->isAlive)
							{
								score += 1;
								PlaySound(CollectableSound);

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

						// Respawn the player once and increment the death counter.
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

// ==================== PUZZLE LOGIC ====================
// Pressure plates disable their matching doors while the correct push box overlaps them.
void UpdatePuzzles()
{
	if (TestAABB(pushBoxBlue, plateBlueBody))
	{
		doorBlueBody->isAlive = false;
		if (!Blue_UnlockDoorSound_Played)
		{
			PlaySound(UnlockDoorSound);
			Blue_UnlockDoorSound_Played = true;
		}
	}
	else
	{
		doorBlueBody->isAlive = true;
		Blue_UnlockDoorSound_Played = false;
	}


	if (TestAABB(pushBoxPink, platePinkBody))
	{
		doorPinkBody->isAlive = false;
		if (!Pink_UnlockDoorSound_Played)
		{
			PlaySound(UnlockDoorSound);
			Pink_UnlockDoorSound_Played = true;
		}
	}
	else
	{
		doorPinkBody->isAlive = true;
		Pink_UnlockDoorSound_Played = false;
	}
	
}



// Jump pad temporarily increases the jump impulse while the player overlaps it.
void UpdateJumpPads()
{

	if (TestAABB(playerBox, JumpPad))
	{
		JUMP_IMPLUSE = -600.f;
	}
	else
	{
		JUMP_IMPLUSE = JUMP_IMPLUSE_DEF;
	}

}

// ==================== DRAWING ====================
// Draw all currently active physics bodies.
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

// Draw UI appropriate to the current game state.
void DrawGameUI()
{
	// return state
	switch (g_currentGameState) {
	case GAME_START:
		DrawRectangle(bodies[0].position.x - 500, bodies[0].position.y - 400, 1500, 1000, backgroundColor);
		win = false;
		DrawText("Puzzle Platformer", bodies[0].position.x-400, bodies[0].position.y-200, 50, textColor);
		DrawText("Press Enter to Start", bodies[0].position.x - 350, bodies[0].position.y - 140, 30, textColor);
		DrawText("CONTROLS : Arrows / WASD: Move | Space / Up Arrow: Jump \n Enter: Reset Box Position", bodies[0].position.x - 300, bodies[0].position.y - 80, 20, textColor);
		DrawText("OBJECTIVE : Push Boxes to their corresponding plates \n to progress, collect the gold cube and reach the end of the level!", bodies[0].position.x - 300, bodies[0].position.y -10, 20, textColor);
		DrawText("Be careful not to touch any red!", bodies[0].position.x - 300, bodies[0].position.y +50, 20, textColor);


		break;
	case GAME_RUNNING:
		
		if (!win)
		{

		DrawRectangle(bodies[0].position.x - 420, bodies[0].position.y - 205, 150, 50, WHITE);
		DrawText(TextFormat("Score: %d \nDeaths: %d", score, deaths), bodies[0].position.x - 400, bodies[0].position.y - 200, 20, textColor);
		
		
		}
		
		break;
	case GAME_PAUSED:
		DrawRectangle(bodies[0].position.x - 500, bodies[0].position.y - 400, 1500, 1000, backgroundColor);
		DrawText("Game Paused!", bodies[0].position.x - 200, bodies[0].position.y - 200, 50, textColor);
		DrawText("Press Enter to continue \nor E to go to Menu", bodies[0].position.x - 200, bodies[0].position.y - 100, 30, textColor);
		


		break;
	case GAME_WIN:
		DrawRectangle(bodies[0].position.x - 500, bodies[0].position.y - 400, 1500, 1000, backgroundColor);
		DrawText("YOU WON!\nPress Enter to Restart", bodies[0].position.x - 200, bodies[0].position.y - 200, 30, textColor);
		break;

	case GAME_LOSE:
		DrawText("        You Lose!\nPress Enter to Restart", bodies[0].position.x - 200, bodies[0].position.y - 200, 20, textColor);
		break;
	}
}

// Reset a movable body to a known position with zero velocity.
void ResetBox(Body* box, Vector2 position)
{
	if (!box) {
		return;
	}

	box->position = position;
	box->velocity = (Vector2){ 0.0f, 0.0f };
}


// ==================== GAME FLOW ====================
// Handles start, running, pause and end states plus state-specific input.
void GameStateMachine(float dt) {

	//reset positions of moveable boxes and player
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

		if (IsKeyPressed(KEY_ENTER)) 
		{
			ResetBox(pushBoxBlue, resetpushBoxBluePosition);
			ResetBox(pushBoxPink, resetpushBoxPinkPosition);
		}
		HandleInput(playerBox, dt);

		//win condition
		if (win)
		{
			g_currentGameState = GAME_WIN;
			PlaySound(WinSound);
		}

		if (IsKeyPressed(KEY_P))
		{
			g_currentGameState = GAME_PAUSED;
			
		}
		break;
		
	case GAME_PAUSED:
		if (IsKeyPressed(KEY_ENTER)) {
		g_currentGameState = GAME_RUNNING;
		}

		if (IsKeyPressed(KEY_E)) {
			g_currentGameState = GAME_START;
		}

		break;



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

// ==================== LEVEL SETUP ====================
// Assign body pointers and initialise all level objects.
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

	 InitBody(maingroundBody, (Vector2) { 0.f, 400.f }, (Vector2) { 2300.f, 400.f }, 0.f, platformColor);

	 InitBody(platformBody1, (Vector2) { 500.0f, 300.0f }, (Vector2) { 150.0f, 20.0f }, 0.0f, platformColor);
	 InitBody(platformBody2, (Vector2) { 900.f, 190.f }, (Vector2) { 150.0f, 20.0f }, 0.f, platformColor);
	 InitBody(platformBody3, (Vector2) { 300.f, 150.f }, (Vector2) { 150.0f, 20.0f }, 0.f, platformColor);
	 InitBody(platformBody4, (Vector2) { 800.f, 50.f }, (Vector2) { 150.0f, 20.0f }, 0.f, platformColor);
	 InitBody(platformBody5, (Vector2) { 700.f, -50.f }, (Vector2) { 150.0f, 20.0f }, 0.f, platformColor);
	 InitBody(platformBody6, (Vector2) { 1000.f, -150.f }, (Vector2) { 150.0f, 20.0f }, 0.f, platformColor);

	 InitBody(platformBody7, (Vector2) { 1550.f, -50.f }, (Vector2) { 500.0f, 100.0f }, 0.f, platformColor);
	 InitBody(platformBody8, (Vector2) { 3000.f, 0.f }, (Vector2) { 300.0f, 50.0f }, 0.f, platformColor);

	 InitBody(dividingwallBody1, (Vector2) { 1100.f, -700.f }, (Vector2) { 300.0f, 975.0f }, 0.f, platformColor);
	 InitBody(dividingwallBody2, (Vector2) { 3000.f, -700.f }, (Vector2) { 500.0f, 500.0f }, 0.f, platformColor);


	//Initalise Moving bodies
	
	InitBody(playerBox, (Vector2) { 100.f, 300.f }, (Vector2) { playerWidth, playerHeight }, 1.f, playerColor);
	playerBox->isPlayer = true;


	// init puzzle bodies
	InitBody(pushBoxBlue, (Vector2) { 850, 0 }, (Vector2) { 50.0f, 50.0f }, 1.0f, bluePuzzleColor);
	pushBoxBlue->isAlive = true;
	InitBody(plateBlueBody, (Vector2) { 960.f, 400.f }, (Vector2) { 70.0f, 30.0f }, 0.f, bluePuzzleColor);
	InitBody(doorBlueBody, (Vector2) { 1225.f, 200.f }, (Vector2) { 50.0f, 200.0f }, 0.f, bluePuzzleColor);

	InitBody(pushBoxPink, (Vector2) { 5000, -5000 }, (Vector2) { 50.0f, 50.0f }, 1.0f, pinkPuzzleColor);
	pushBoxPink->isAlive = true;
	InitBody(platePinkBody, (Vector2) { 1700.f, 400.f }, (Vector2) { 70.0f, 30.0f }, 0.f, pinkPuzzleColor);
	InitBody(doorPinkBody, (Vector2) {3100.f, -200.f }, (Vector2) { 50.0f, 200.0f }, 0.f, pinkPuzzleColor);
	

	//init jump pad
	InitBody(JumpPad, (Vector2) { 2200.f, 380.f }, (Vector2) { 80.0f, 20.0f }, 0.f, bouncePadColor);


	//init hazard bodies
	InitBody(voidBody1, (Vector2) { 2300.f, 450.f }, (Vector2) { 1000.f, 100.f }, 0.f, hazardColor);
	InitBody(voidBody2, (Vector2) { 490.f, 370.f }, (Vector2) { 170.f, 30.f }, 0.f, hazardColor);
	InitBody(voidBody3, (Vector2) { 1750.f, -80.f }, (Vector2) { 150.f, 30.f }, 0.f, hazardColor);
	

	//init win body
	InitBody(winBody, (Vector2) { 3250.f, -200.f }, (Vector2) { 50.f, 200.f }, 0.f, collectibleColor);
	


	//intialise collectable body
	InitBody(collectableBody, (Vector2) { 1040.0f, -300.0f }, (Vector2) { 20.0f, 20.0f }, 0.0f, collectibleColor);
	
		
	


}

// ==================== CAMERA / RENDERING ====================

Camera2D camera = { 0 };

void DrawGame()
{
	BeginDrawing();

	BeginMode2D(camera);

	ClearBackground(backgroundColor);

	//bounds
	DrawRectangle(-500, -2000, 500, 3000, platformColor);
	DrawRectangle(worldRightBound, -2000, 500, 3000, platformColor);

	//arrows
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

// ==================== MAIN ====================

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");
	
	// Create the window and set FPS
	InitWindow(windowWidth, windowHeight, "2D Platformer from scrach");
	
	//remove unneeded info in the console
	SetTraceLogLevel(LOG_WARNING);

	SetTargetFPS(60);
	
	//setting variables as other variables
	stopspeed = ACCELERATION / 2.f;
	JUMP_IMPLUSE_DEF = JUMP_IMPLUSE;
	
	// Initialise the game state and bodies
	InitialiseGame();

	InitAudioDevice();

	
	// Set Camera to follow the player with a slight offset.

	camera.target = (Vector2){ 0,0 };
	camera.offset = (Vector2){ (float)windowWidth / 2, (float)windowHeight / 2 , };
	camera.rotation = 0.f;
	camera.zoom = 1.5f;

	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float dt = GetFrameTime();


		//Setting Sounds and Textures
		ArrowTexture = LoadTexture("Arrow.png");
		HazardSound = LoadSound("error.WAV");
		JumpSound = LoadSound("jump.WAV");
		SetSoundVolume(JumpSound, 0.1f);
		WinSound = LoadSound("win.WAV");
		SetSoundVolume(WinSound, 0.2f);
		CollectableSound = LoadSound("aquire.WAV");
		UnlockDoorSound = LoadSound("completed.WAV");

		camera.target = (Vector2){ bodies[0].position.x, bodies[0].position.y - 100 };

		// Update gameplay state and input.
		GameStateMachine(dt);

		// Apply horizontal friction, gravity and movement to all dynamic bodies.
		StepPhysicsForAllBodies(bodies, dt, GRAVITY);

		UpdatePuzzles();

		UpdateJumpPads();

		// Detect and resolve body collisions after physics movement.
		ResolveCollisionsForAllBodies(bodies);


		// Development shortcuts used to quickly test different parts of the level.
		if (IsKeyPressed(KEY_E))
		{
			playerBox->position = (Vector2){ 1600, 300.f };
		}
		if (IsKeyPressed(KEY_R))
		{
			playerBox->position = (Vector2){
			100.f, 300.f
			};
		}
		// Render the current frame.
		DrawGame();
	

	}
	// Release loaded resources and close the window.
		UnloadTexture(ArrowTexture);
		UnloadSound(HazardSound);
		UnloadSound(JumpSound);
		UnloadSound(WinSound);
		UnloadSound(CollectableSound);
		UnloadSound(UnlockDoorSound);
		CloseAudioDevice;
		CloseWindow();
		return 0;

	}