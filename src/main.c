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


int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);


	int windowWidth = 1290;
	int windowHeight = 720;

	// Create the window and OpenGL context
	InitWindow(windowWidth, windowHeight, "2D Platformer from scrach");


	SetTargetFPS(60);

	float playerX = 100;
	float playerY = 300;
	float playerWidth = 25;

	float accel = 800.f;
	float velocityX = 0.f;
	float velocityY = 0.f;
	float friction = 1.f;
	float stopspeed = accel / 2.f;


	// Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
	SearchAndSetResourceDir("resources");

	// Load a texture from the resources directory
	Texture wabbit = LoadTexture("wabbit_alpha.png");
	
	
	// game loop
	while (!WindowShouldClose())		// run the loop until the user presses ESCAPE or presses the Close button on the window
	{
		//Make sure the frames run at the same speed for every computer
		float deltaTime = GetFrameTime();

		//Player Movement
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {

			velocityX += accel * deltaTime;

		}

		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {

			velocityX-= accel * deltaTime;

		}

		//deceleration
		velocityX -= velocityX * friction * deltaTime;
		playerX += velocityX * deltaTime;
	

		// making sure player can decelerate to 0
		if (abs(velocityX) < stopspeed * deltaTime)
		{
			velocityX = 0;
		}


		//stop player from going off screen
		if (playerX < 0) {

			playerX = 0;
			velocityX = 0;
		}
		if (playerX > windowWidth - playerWidth) {
			playerX = windowWidth - playerWidth;
			velocityX = 0;
		}

		


		// drawing
		BeginDrawing();

		// Setup the back buffer for drawing (clear color and depth buffers)
		ClearBackground(BLUE);

		// draw our texture to the screen
		DrawTexture(wabbit, playerX, playerY, WHITE);
		
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
