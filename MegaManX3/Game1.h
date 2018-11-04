#pragma once
#include "Game.h"
#include "KeyEvent.h"

#include "Camera.h"
#include "Graphics.h"
class Game1 :
	public Game
{

public:
	void loadResource() override;
	void initOption() override;
	void update(DWORD) override;
	void render(DWORD) override;
	
	~Game1();
};

