#include "Box.h"


Box::Box(int id, float x, float y)
{
	this->_id = id;
	this->x = x;
	this->y = y;
	this->isNext = false;
	this->isLeft = true;

	this->loadResources();
	this->setState(BOX_STATE_DEFAULT);
}

Box::~Box()
{
}

void Box::update(DWORD dt, unordered_map<int, GameObject*>* staticObjects, unordered_map<int, GameObject*>* dynamicObjects)
{
	this->dt = dt;
	if (_death) {
		generatePosition();
		return;
	}

	if (isLeft) {
		if (y >= 720 + 59) {
			x += speed.vx * dt;
		}

		y += speed.vy * dt;

		if (!isNext) {
			if (x >= 4944 - 6 && x < 4985 + 25 - 6) {
				speed.vy = 0;
			}

			if (x >= 4985 + 25 - 6) {
				speed.vx = 0;
				speed.vy = BOX_SPEED_Y;
			}
		}
		else
		{
			if (x >= 4944 - 6 + 59 && x < 4985 + 25 - 6 + 59) {
				speed.vy = 0;
			}

			if (x >= 4985 + 25 - 6 + 59) {
				speed.vx = 0;
				speed.vy = BOX_SPEED_Y;
			}
		}


		if (y >= 820 + 59) {
			y = 820 + 59;
			_death = true;
		}
	}
	else
	{
		if (y >= 720 + 59) {
			x -= speed.vx * dt;
		}

		y += speed.vy * dt;

		if (!isNext) {
			if (x <= 4985 + 25 -6) {
				speed.vx = 0;
				speed.vy = BOX_SPEED_Y;
			}
		}
		else
		{
			if (x <= 4985 + 25 - 6 + 59) {
				speed.vx = 0;
				speed.vy = BOX_SPEED_Y;
			}
		}


		if (y >= 820 + 59) {
			y = 820 + 59;
			_death = true;
		}
	}
}

void Box::render(DWORD dt, D3DCOLOR colorBrush)
{
	if (_death) {
		renderDie(dt);
		return;
	}

	auto center = cameraGlobal->transform(x, y);
	_animations[state]->render(center.x, center.y);
}

void Box::renderDie(DWORD dt, D3DCOLOR colorBrush)
{
	for (int i = 0; i < 4; i++) {
		auto center = cameraGlobal->transform(die[i].x, die[i].y);
		_animations[BOX_STATE_DIE + i]->render(center.x, center.y);
	}
}

void Box::generatePosition()
{
	die[0] = { x, y }; //* important
	die[1] = { x + 24, y };
	die[2] = { x, y + 24};
	die[3] = { x + 48, y + 24};

	speed.vy += 0.012f * dt;
	speed.vx += 0.01f * dt;

	dx = speed.vx * dt;
	dy = speed.vy * dt;

	die[0].x -= dx;
	die[1].x += dx * 1.5;
	die[2].x -= dx;
	die[3].x += dx * 2;

	die[0].y += dy * 2;
	die[1].y += dy;
	die[2].y += dy * 2;
	die[3].y += dy;
}

void Box::getBoundingBox(float & left, float & top, float & right, float & bottom)
{
}

Box * Box::clone(int id, int x, int y)
{
	return nullptr;
}

void Box::setState(int state)
{
	switch (state)
	{
	case BOX_STATE_DEFAULT:
		speed.vx = BOX_SPEED_X;
		speed.vy = BOX_SPEED_Y;
		break;
	case BOX_STATE_HIGHTLIGHT:
		speed.vx = 0;
		speed.vy = 0;
	default:
		break;
	}
	this->state = state;
	_animations[state]->reset();
}

void Box::loadResources()
{
	CTextures * textures = CTextures::getInstance();
	if (!textures->getTexture(BOX_ID_TEXTURE))
		textures->add(BOX_ID_TEXTURE, L"Resource\\Textures\\subboss.png", 0, 0, D3DCOLOR_XRGB(255, 0, 255));

	CSprites * sprites = CSprites::getInstance();
	CAnimations * animations = CAnimations::getInstance();

	LPANIMATION ani;

	// default
	sprites->addSprite(20001, BOX_ID_TEXTURE, 284, 29, 48, 48);

	ani = new CAnimation(100);
	ani->add(20001);
	animations->add(BOX_STATE_DEFAULT, ani);

	// hightlight
	sprites->addSprite(20011, BOX_ID_TEXTURE, 284, 82, 48, 48);

	ani = new CAnimation(100);
	ani->add(20011);
	animations->add(BOX_STATE_HIGHTLIGHT, ani);

	// die
	sprites->addSprite(20021, BOX_ID_TEXTURE, 271, 19, 8, 8);
	sprites->addSprite(20022, BOX_ID_TEXTURE, 337, 19, 8, 8);
	sprites->addSprite(20023, BOX_ID_TEXTURE, 337, 79, 8, 8);
	sprites->addSprite(20024, BOX_ID_TEXTURE, 271, 79, 8, 8);

	for (int i = 0; i < 4; i++)
	{
		ani = new CAnimation(100);
		ani->add(20021 + i);
		animations->add(BOX_STATE_DIE + i, ani);
		this->addAnimation(BOX_STATE_DIE + i);
	}

	this->addAnimation(BOX_STATE_DEFAULT);
	this->addAnimation(BOX_STATE_HIGHTLIGHT);
}

