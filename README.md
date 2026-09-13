## Puzzle Platformer

A 2D puzzle platformer built from scratch in C using Raylib, focused on custom physics, responsive player movement, and physics-based puzzle mechanics.

Originally developed as a university project for *Games Programming: AI and Physics*, the project involved building and extending a custom physics system rather than relying on an existing game engine.


<img width="1290" height="600" alt="GIF of platformer" src="https://github.com/user-attachments/assets/dc961eec-af67-4854-b7a0-a62601e8a971" />

## About

Puzzle Platformer is a short 2D platforming game where the player navigates obstacles and solves physics-based puzzles using pushable boxes and pressure plates.

The project was primarily an exploration of implementing game physics from scratch, including custom collision detection and resolution, movement physics, and additional platforming mechanics designed to improve player responsiveness.

## Features

- Custom physics-based movement and jumping
- AABB collision detection and resolution
- Pushable box and pressure plate puzzles
- Bounce pads
- Collectables and scoring
- Hazards, death and respawning
- Coyote time and jump buffering
- Variable jump height
- Asymmetric gravity and apex hang
- Follow camera
- Win, lose and pause states
- Sound effects and visual feedback

## Controls

| Input | Action |
|---|---|
| A / D or Arrow Keys | Move |
| Space / Up Arrow | Jump |
| Enter | Reset push boxes |
| P | Pause |

## Technical Highlights

The game was developed without a traditional game engine, with gameplay and physics systems implemented directly in C using Raylib.

Some of the main systems I implemented include custom AABB collision detection and resolution, mass-based interactions between physics bodies, and a customised jump system using different gravity values while rising, falling and reaching the apex of a jump.

Additional movement assists such as coyote time, jump buffering and variable jump height were added and tuned through playtesting to make the platforming more responsive.

## Built With

- C
- Raylib
- Visual Studio

## Development

This project was created for the Griffith University course *Games Programming: AI and Physics*.

Course materials provided the foundation for the physics implementation, which I expanded through experimentation and additional research into collision systems and platforming movement..

