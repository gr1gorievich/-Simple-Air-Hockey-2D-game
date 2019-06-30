#pragma once

#ifndef BOARD_H_
#define BOARD_H_

#include "Entity.h"
#include <stdlib.h>
#include <time.h>

class Board : public Entity {
public:
	Board(SDL_Renderer* renderer);
	~Board();

	void Update(float delta);
	void Render(float delta);

	float boundtop, boundbottom;

private:
	SDL_Texture* sidetexture;
};

#endif
