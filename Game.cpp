#include "Game.h"

Game::Game() {
	window = 0;
	renderer = 0;
}

Game::~Game() {

}

bool Game::Init() {
	// Initialize SDL and the audio/video subsystem
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	// Create window
	window = SDL_CreateWindow("Air Hockey",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	if (!window) {
		// Error creating window
		return false;
	} 

	

	// Create renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer) {
		// Error creating renderer
		return false;
	}

	// Initialize timing
	lasttick = SDL_GetTicks();
	fpstick = lasttick;
	fps = 0;
	framecount = 0;

	return true;
}

void Game::Clean() {
	// Clean renderer and window
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

void Game::Run() {
	// Create board and ball
	board = new Board(renderer);
	ball = new Ball(renderer);

	// Create two paddles and put them in a std::vector (dynamic array)
	paddle.push_back(new Paddle(renderer));
	paddle.push_back(new Paddle(renderer));

	NewGame();


	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cout << "Error:" << Mix_GetError() << std::endl;
	}

	Mix_Music *bgm = Mix_LoadMUS("bgm.mp3");
	Mix_Chunk *soundEffect = Mix_LoadWAV("iflost.wav");
	Mix_Chunk *soundEffect2 = Mix_LoadWAV("ifwon.wav");
	Mix_Chunk *soundEffect3 = Mix_LoadWAV("touch.wav");
	
	
	// Main loop
	while (1) {
		// Handler events
		SDL_Event e;

		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
				case SDLK_1:
					if (!Mix_PlayingMusic())
						Mix_PlayMusic(bgm, -1);
					else if (Mix_PausedMusic())
						Mix_ResumeMusic();
					else Mix_PauseMusic();
					break;
				case SDLK_2:
					Mix_HaltMusic();
					break;
				}
			}
		}

		if (ball->x < 1) {
			Mix_PlayChannel(-1, soundEffect, 0);
		}
		else if (ball->x > board->width - 1) {
			Mix_PlayChannel(-1, soundEffect2, 0);
		}
		else if ((ball->x - ball->width / 2 < paddle[0]->x + paddle[0]->width) && (ball->x - ball->width / 2 > paddle[0]->x)) {
			Mix_PlayChannel(-1, soundEffect3, 0);
		}
		else if ((ball->x + ball->width / 2 > paddle[1]->x - paddle[0]->width) && (ball->x + ball->width / 2 < paddle[1]->x)) {
			Mix_PlayChannel(-1, soundEffect3, 0);
		}


		// Calculate delta and fps
		unsigned int curtick = SDL_GetTicks();
		float delta = (curtick - lasttick) / 1000.0f;
		if (curtick - fpstick >= FPS_DELAY) {
			fps = framecount * (1000.0f / (curtick - fpstick));
			fpstick = curtick;
			framecount = 0;
			char buf[100];
			snprintf(buf, 100, "Air Hockey(fps: %u)", fps);
			SDL_SetWindowTitle(window, buf);
		}
		else {
			framecount++;
		}
		lasttick = curtick;

		// Update and render the game
		Update(delta);
		Render(delta);
	}

	// Delete objects
	delete board;
	delete ball;

	// Delete all paddles
	for (unsigned int i = 0; i<paddle.size(); i++) {
		delete paddle[i];
	}
	paddle.clear();

	Mix_FreeMusic(bgm);
	bgm = nullptr;

	Clean();
	Mix_Quit();
	SDL_Quit();
}

void Game::NewGame() {
	// Center the paddles vertically and horizontally
	SetPaddleY(0, board->height / 2 - paddle[0]->height / 2);
	SetPaddleY(1, board->height / 2 - paddle[1]->height / 2);
	SetPaddleX(0, 1);
	SetPaddleX(1, board->width - paddle[1]->width - 1);

	// Center the ball horizontally and vertically
	ball->x = board->width / 2 - ball->width / 2;
	ball->y = board->height / 2 - ball->height / 2;

	// Set the initial direction of the ball
	ball->SetDirection(1, -1);
}

void Game::Update(float delta) {
	// Game logic

	// Get the mouse position
	int mx, my;
	SDL_GetMouseState(&mx, &my);

	// Move the paddle with the mouse
	SetPaddleY(0, my - paddle[0]->height / 2.0f);
	SetPaddleX(0, mx - paddle[0]->width / 2.0f);

	// Check collisions and perform AI
	CheckCollisions();
	AI(delta);

	// Update board and ball
	board->Update(delta);
	ball->Update(delta);                                                                                       

	// Update paddles
	for (unsigned int i = 0; i < paddle.size(); i++) {
		paddle[i]->Update(delta);
	}
}

void Game::SetPaddleX(int index, float x) {
	float newx;
	if (index == 0) {
		if (x > board->width / 2 - paddle[0]->width) {
			newx = board->width / 2 - paddle[0]->width;
		}
		else if (x < 1) {
			newx = 1;
		}
		else {
			newx = x;
		}
		paddle[index]->x = newx;
	}
	if (index == 1) {
		if (x < board->width / 2 + paddle[1]->width) {
			newx = board->width / 2 + paddle[1]->width;
		}
		else if (x > board->width - 1) {
			newx = board->width - 1;
		}
		else {
			newx = x;
		}
		paddle[index]->x = newx;
	}
}

void Game::SetPaddleY(int index, float y) {
	float newy;
	if (y < board->boundtop) {
		// Top bound
		newy = board->boundtop;
	}
	else if (y + paddle[index]->height > board->boundbottom) {
		// Bottom bound
		newy = board->boundbottom - paddle[index]->height;
	}
	else {
		// Within bounds
		newy = y;
	}

	// Set the new y
	paddle[index]->y = newy;
}

void Game::CheckCollisions() {
	// Top and bottom
	if (ball->y < board->boundtop) {
		ball->y = board->boundtop;
		ball->SetDirection(ball->dirx, -1 * ball->diry);
	}
	else if (ball->y + ball->height > board->boundbottom) {
		ball->y = board->boundbottom - ball->height;
		ball->SetDirection(ball->dirx, -1 * ball->diry);
	}

	// Left paddle collisions
	if (ball->Collides(paddle[0])) {
		ball->x = paddle[0]->x + paddle[0]->width;
		ball->SetDirection(1, GetReflection(0, (ball->y + ball->height / 2) - paddle[0]->y));
	}

	// Right paddle collisions
	if (ball->Collides(paddle[1])) {
		ball->x = paddle[1]->x - ball->width;
		ball->SetDirection(-1, GetReflection(1, (ball->y + ball->height / 2) - paddle[1]->y));
	}

	// Check if ball is off the screen
	if (ball->x + ball->width <= board->x) {

		// Center the ball vertically on the side of player 2
		ball->x = board->width / 2 - ball->width / 2 + 250;
		ball->y = board->height / 2 - ball->height / 2;

		ball->SetDirection(-1, -1);
	}
	else if (ball->x >= board->x + board->width) {

		// Center the ball vertically on the side of player 1
		ball->x = board->width / 2 - ball->width / 2 - 250;
		ball->y = board->height / 2 - ball->height / 2;

		ball->SetDirection(1, -1);
	}
}

float Game::GetReflection(int index, float hity) {
	// Make sure the hity variable is within the height of the paddle
	if (hity < 0) {
		hity = 0;
	}
	else if (hity > paddle[index]->height) {
		hity = paddle[index]->height;
	}

	// Everything above the center of the paddle is reflected upward
	// while everything below the center is reflected downward
	hity -= paddle[index]->height / 2.0f;

	// Scale the reflection, making it fall in the range -2.0f to 2.0f
	return 2.0f * (hity / (paddle[index]->height / 2.0f));
}

void Game::AI(float delta) {

	// Calculate how much the paddle can move in this frame
	float dy = AI_SPEED * delta;
	float dx = AI_SPEED * delta;

	// Imperfect ai
	if (ball->x > board->width / 2 && ball->dirx > 0) {
		// Lag behind the ball, when ball is in sight
		if (paddle[1]->y + paddle[1]->height / 2 < ball->y + ball->height / 2 - dy) {
			SetPaddleY(1, paddle[1]->y + dy);
			SetPaddleX(1, paddle[1]->x - dx);
		}
		else if (paddle[1]->y + paddle[1]->height / 2 > ball->y + ball->height / 2 + dy) {
			SetPaddleY(1, paddle[1]->y - dy);
			SetPaddleX(1, paddle[1]->x - dx);
		}
	}
	else {
		// Move to the center, when ball is far away (for SetPaddleY)
		if (paddle[1]->y + paddle[1]->height / 2 < board->height / 2 - dy) {
			SetPaddleY(1, paddle[1]->y + dy);
			SetPaddleX(1, board->width - paddle[1]->width + dx);
		}
		else if (paddle[1]->y + paddle[1]->height / 2 > board->height / 2 + dy) {
			SetPaddleY(1, paddle[1]->y - dy);
			SetPaddleX(1, board->width - paddle[1]->width + dx);
		}
	}
}

void Game::Render(float delta) {
	// Clear the screen to white
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
	SDL_RenderClear(renderer);

	// Render board and ball
	board->Render(delta);
	ball->Render(delta);

	// Render paddles
	for (unsigned int i = 0; i<paddle.size(); i++) {
		paddle[i]->Render(delta);
	}

	// Update the screen
	SDL_RenderPresent(renderer);
}
