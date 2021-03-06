﻿#include "MegamanX.h"
#include "Elevator.h"
#include "ConstGlobals.h"
#include "Bee.h"
#include <vector>
#include "DeadPoint.h"
#include "BlastHornet.h"
#include "ITemHP.h"
void MegamanX::collisionStatic(unordered_map<int, GameObject*>* staticObjects)
{
	vector<CollisionEvent*> coEvents;
	vector<CollisionEvent*> coEventsResult;
	
	collision->findCollisions(dt, this, *staticObjects, coEvents);
	UINT size = coEvents.size();
	
	if (size == 0)
	{
		x += dx;
		y += dy;
	}
	else
	{
		float min_tx, min_ty, nx = 0, ny;
		collision->filterCollision(coEvents, coEventsResult, min_tx, min_ty, nx, ny);

		float reY = y;
		x += min_tx * dx + nx * 1.f;
		y += min_ty * dy + ny * 0.4f;
		
		if (nx != 0) speed.vx = 0.0f;
		if (ny != 0) speed.vy = 0.0f;
		
		for (UINT i = 0; i < coEventsResult.size(); ++i)
		{
			auto e = coEventsResult[i];
			if (dynamic_cast<DeadPoint *>(e->obj))
			{
				receiveDamage(9999.9f);
				break;
			}
			
			StaticObject* obj = dynamic_cast<StaticObject *>(e->obj);
			if (obj)
			{
				if (e->ny < 0)
				{
					keyController->stopFallOrSlide();
					keyController->setFloor(obj);

					elevator = dynamic_cast<Elevator *>(obj);

					if (elevator)
					{
						if (!elevator->getIsRun())
						{
							elevator->run();
						}
						speed.vy = elevator->speed.vy;
						y = reY + elevator->speed.vy * dt;
						//y += elevator->speed.vy * dt;
					}
				}
				else if(e->ny > 0)
				{
					keyController->stopJump();
					keyController->setFloor(obj);

					//speed.vy = 0.005*dt;
				}
				else if (e->nx !=0)
				{
					keyController->stopDash();
					keyController->setNearWall(e->nx > 0, obj);
				}
			}
		}
	}
	keyController->update();
	
	for (UINT i = 0; i < size; ++i) delete coEvents[i];
}

void MegamanX::collisionDynamic(unordered_map<int, GameObject*>* dynamicObjects)
{
	bulletCollisionDynamic(dynamicObjects);//like main bullet collision dynamic
	if (isProtect) return;
	dynamicCollisionThis(dynamicObjects); //like dynamic obejct bullet collision this
}

void MegamanX::setHurt()
{
	isHurt = true;
	isProtect = true;
	keyController->setHurt(true);
	timeHurt.start();
	timeProtect.start();
	delay = 3;
}

MegamanX::MegamanX(UINT id, float x, float y, float vx, float vy) :DynamicObject(id, x, y, vx, vy)
{
	weapon = new MegamanWeapon(&_weapons);
	effect = new MegamanEffectFactory();
	keyController = new KeyController(this, effect, weapon, false);
	width = Stand_Shoot_Width;
	height = Stand_Shoot_Height;
	initHP = _hp = 24.0f;
	_death = true;
}

MegamanX::~MegamanX()
{
}


void MegamanX::update(DWORD dt, unordered_map<int, GameObject*>* staticObjects, unordered_map<int, GameObject*>* dynamicObjects)
{
	weapon->update(dt);

	GameObject::update(dt);
	speed.vy += 0.0012f * dt;

	if (_death)
	{
		timeRevival.update();
		if (timeRevival.isStop())
		{
			this->revival();
		}
		return;
	}
	if (this->isRevivaling)
	{
		updateRevivaling(dt, staticObjects);
		return;
	}else
	if (!enable) return;




	collisionStatic(staticObjects);
	collisionDynamic(dynamicObjects);

	if (isProtect)
	{
		timeProtect.update();
		
		if (timeHurt.isRunning())
		{
			timeHurt.update();
		}
		else isHurt = false, keyController->setHurt(false);

		delay -= 1;
		if (timeProtect.isStop()) isProtect = false;
	}
}

void MegamanX::updateStage(DWORD dt, unordered_map<int, GameObject*>* dynamicObjects)
{
	if (!enable || _death) return;
	GameObject::update(dt);
	collisionDynamic(dynamicObjects);
}

void MegamanX::addHP(float hp)
{
	if (_hp + hp > initHP)
		_hp = initHP;
	else
		_hp += hp;
	soundsGlobal->play(sound_MX_heal);
}

void MegamanX::updateState(DWORD dt) 
{
	int statePre = keyController->getState(isFlipX);
	if (state != statePre && (statePre == dash || statePre == run || statePre == jump || statePre == fall || statePre == run_shoot))
	{
		_animations[statePre]->reset();
	}
	state = statePre;
}

void MegamanX::render(DWORD dt, D3DCOLOR colorBrush)
{
	if (_death)
	{
		if (timePreDie.isRunning())
		{
			timePreDie.update();
			auto pos = cameraGlobal->transform(x, y);
			_animations[preDie]->render(pos.x, pos.y);
			return;
		}
		countDissapear.update();
		if (countDissapear.isStop())
		{
			resetPoint();
			countDissapear.start();
		}
		dissapear(dt, colorBrush);
		return;
	}

	if (this->isRevivaling)
	{
		auto pos = &cameraGlobal->transform(x, y);
		_animations[state]->render(pos->x + width / 2, pos->y + 20, true);
		if (state == appear + 1 && _animations[state]->isLastFrame())
		{
			_animations[state]->reset();
			isRevivaling = false;
			setEnable(true);
			keyController->setToLeft(false);
		}
	}
	else
	{
		if (enable)
			updateState(dt);

		if (!isHurt && isProtect)
		{
			colorBrush = showblur ? WHITE(128) : WHITE(255);
			if (delay < 0) delay = 3, showblur = !showblur; //delay two frames 
		}


		Point center = { 0, 0 };
		config(center);

		if (isFlipX)
			_animations[state]->renderFlipX(center.x, center.y, false, colorBrush);
		else
			_animations[state]->render(center.x, center.y, false, colorBrush);
	}
	effect->render(dt, x, y, width, height);
	weapon->render(dt);
}

void MegamanX::dissapear(DWORD dt, D3DCOLOR colorBrush)
{
	
	speed.vx = 0.2f;
	speed.vy = -0.2f;
	p1.x += speed.vx*dt;
	p1.y += speed.vy*dt;
	auto pos = cameraGlobal->transform(p1.x, p1.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = 0.2f;
	speed.vy = 0.2f;
	p2.x += speed.vx*dt;
	p2.y += speed.vy*dt;
	pos = cameraGlobal->transform(p2.x, p2.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);
	
	speed.vx = -0.2f;
	speed.vy = 0.2f;
	p3.x += speed.vx*dt;
	p3.y += speed.vy*dt;
	pos = cameraGlobal->transform(p3.x, p3.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = -0.2f;
	speed.vy = -0.2f;
	p4.x += speed.vx*dt;
	p4.y += speed.vy*dt;
	pos = cameraGlobal->transform(p4.x, p4.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = 0.2f;
	speed.vy = 0.f;
	p5.x += speed.vx*dt;
	p5.y += speed.vy*dt;
	pos = cameraGlobal->transform(p5.x, p5.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = 0;
	speed.vy = 0.2f;
	p6.x += speed.vx*dt;
	p6.y += speed.vy*dt;
	pos = cameraGlobal->transform(p6.x, p6.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = -0.2f;
	speed.vy = 0;
	p7.x += speed.vx*dt;
	p7.y += speed.vy*dt;
	pos = cameraGlobal->transform(p7.x, p7.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);

	speed.vx = 0;
	speed.vy = -0.2f;
	p8.x += speed.vx*dt;
	p8.y += speed.vy*dt;
	pos = cameraGlobal->transform(p8.x, p8.y);
	_animations[die]->render(pos.x, pos.y, colorBrush);
}

void MegamanX::resetPoint()
{
	p1 = { x, y - 10 };
	p2 = { x + 10, y };
	p3 = { x, y + 10 };
	p4 = { x - 10, y };
	p5 = { x + 10, y - 10 };
	p6 = { x + 10 , y + 10 };
	p7 = { x - 10 , y + 10 };
	p8 = { x - 10, y - 10 };
}

void MegamanX::config(Point & center)
{
	switch (state)
	{
	case dash:
	case dash_shoot:
		center = cameraGlobal->transform(x, y + 9);////
		break;
	case slide:
	case cling:
		int deta;
		(isFlipX) ? deta = -1 : deta = 6;
		center = cameraGlobal->transform(x + deta, y);////
		break;
	case slide_shoot:
	case cling_shoot:
		(isFlipX) ? deta = -1 : deta = 1;
		center = cameraGlobal->transform(x + deta, y);
		break;
	default:
		center = cameraGlobal->transform(x, y);
		break;
	}
}

void MegamanX::onKeyDown(int keyCode)
{
	switch (keyCode)
	{
	case DIK_LEFT:
		keyController->addKeyArrow(true);
		break;
	case DIK_RIGHT:
		keyController->addKeyArrow(false);
		break;
	case DIK_Z:		
		keyController->addKeyZ();
		
		break;
	case DIK_X:
		keyController->addKeyX();
		
		break;
	case DIK_C:
		keyController->addKeyC();
		
		break;
	default:
		break;
	}
}

void MegamanX::onKeyUp(int keyCode)
{
	switch (keyCode)
	{
	case DIK_LEFT:
		keyController->removeKeyArrow(true);
		break;
	case DIK_RIGHT:
		keyController->removeKeyArrow(false);
		break;
	case DIK_Z:
		keyController->removeKeyZ();
		break;
	case DIK_X:
		keyController->removeKeyX();
		break;
	case DIK_C:
		keyController->removeKeyC();
		break;
	default:
		break;
	}
}

void MegamanX::keyState(BYTE *_state)
{
	keyController->update();
}

void MegamanX::getBoundingBox(float & left, float & top, float & right, float & bottom)
{
	left = x;
	top = y;
	if (isRevivaling)
	{
		right = x + width;
		bottom = y + height;
	}
	else
	{
		keyController->getSize(width, height);
		right = x + width;
		bottom = height + y;
	}
}

void MegamanX::receiveDamage(float damage)
{
	if (_hp - damage > 0)
	{
		_hp -= damage;
		_attacked = true;
		timeAttacked.start();
		setHurt();
	}
	else
	{
		_hp -= damage;
		timeRevival.start();
		setAnimationDie();
		timeHide.start();
		_death = true;
		soundsGlobal->play(sound_MX_die);
	}
}

void MegamanX::setEnable(bool value)
{
	if (value)
	{
		enable = true;
		keyController->stopDashRunning();
		keyController->stopJump();
		keyController->stopRun();
	}
}

void MegamanX::dynamicCollisionThis(unordered_map<int, GameObject*>* dynamicObjects)
{
	auto megamanBox = this->getBoundBox();
	for (auto kv : *dynamicObjects)
	{
		// object collision main (include bullet) (use single & swept aabb)
		DynamicObject* obj = dynamic_cast<DynamicObject*>(kv.second);
		if (!obj || obj->isDeath()) continue;

		if (obj->getBoundBox().intersectsWith(megamanBox) //single
			||
			collisionGameObject(obj, this)) // swpet
		{

			this->receiveDamage(obj->getDamage());
			return;
		}

		auto bullets = obj->getWeapons();
		for (auto bullet = bullets->begin(); bullet != bullets->end(); )
		{
			if (bullet[0]->getBoundBox().intersectsWith(megamanBox) //single
				||
				collisionBullet(*bullet, this)) //swept
			{
				this->receiveDamage(bullet[0]->getDamage());
				if (dynamic_cast<Bee*>(bullet[0]))
				{
					bullet[0]->setAnimationEnd();
				}
				else
				{
					if (bullet[0]->toLeft)
						obj->createExplosion(bullet[0]->x + 10, bullet[0]->y);
					else
						obj->createExplosion(bullet[0]->x - 10, bullet[0]->y);
				}

				delete *bullet;
				bullet = bullets->erase(bullet);
				return;
			}
			else ++bullet;
		}
	}
}

void MegamanX::bulletCollisionDynamic(unordered_map<int, GameObject*>* dynamicObjects)
{
	for(auto it = dynamicObjects->begin(); it!= dynamicObjects->end(); )
	{
		DynamicObject* obj = dynamic_cast<DynamicObject*>((*it).second);
		if (!obj)
		{
			++it;
			continue;
		}
		// bullet main collision enemies or boss (use single & swept aabb)
		auto objBox = obj->getBoundBox();
		for (auto bullet = _weapons.begin(); bullet != _weapons.end();)
		{
			if (obj->isDeath()) break;
			auto box = bullet[0]->getBoundBox();
			if (box.intersectsWith(objBox) //single aabb collision
				||
				collisionGameObject(*bullet, obj)) //colision bullet with dynamic with swept aabb
			{
				obj->receiveDamage(bullet[0]->getDamage());
				if (obj->isDeath())
				{
					createItems(obj);
					if (dynamic_cast<BusterShot*>(*bullet)) // don't cross delete bullet
					{
						delete *bullet;
						bullet = _weapons.erase(bullet);
					}
				}
				else
				{
					bullet[0]->setAnimationEnd();
					delete *bullet;
					bullet = _weapons.erase(bullet);
				}
				break;// out for bullet
			}
			else // bullet main collision bullet enemies or boss (use swept aabb)
			{
				auto dynamicBullets = obj->getWeapons();
				bool noDelete = true;
				for (auto bulletDynamic = dynamicBullets->begin(); bulletDynamic != dynamicBullets->end();)
				{
					if (box.intersectsWith(bulletDynamic[0]->getBoundBox())
						||
						collisionBullet(*bulletDynamic, *bullet))
					{
						if (dynamic_cast<Bee*>(bulletDynamic[0]))
						{
							bulletDynamic[0]->createExplosion(bulletDynamic[0]->x, bulletDynamic[0]->y);
						}
						else
						{
							if (bulletDynamic[0]->toLeft)
								obj->createExplosion(bulletDynamic[0]->x + 10, bulletDynamic[0]->y);
							else
								obj->createExplosion(bulletDynamic[0]->x - 10, bulletDynamic[0]->y);
						}

						delete bulletDynamic[0];
						bulletDynamic = dynamicBullets->erase(bulletDynamic);
						noDelete = false;
						if (dynamic_cast<BusterShot*>(*bullet)) // don't cross delete bullet
						{
							delete *bullet,
							bullet = _weapons.erase(bullet);
						}
						else ++bullet;
						break;
					}
					else ++bulletDynamic;
				}
				if (noDelete) ++bullet;
			}
		}
		++it;
	}
}

void MegamanX::revival()
{
	this->reset();
	isRevivaling = true;
	soundsGlobal->play(sound_MX_revival);
}

void MegamanX::reset()
{
	width = Stand_Shoot_Width;
	height = Stand_Shoot_Height;
	_hp = initHP;
	state = appear;
	enable = false;
	_death = false;

}

void MegamanX::updateRevivaling(DWORD dt, unordered_map<int, GameObject*>* staticObjects)
{
	
	vector<CollisionEvent*> coEvents;
	vector<CollisionEvent*> coEventsResult;

	collision->findCollisions(dt, this, *staticObjects, coEvents);
	UINT size = coEvents.size();

	if (size == 0)
	{
		x += dx;
		y += dy;
		if (state != appear + 1)
		{
			state = appear;
		}
	}
	else
	{
		float min_tx, min_ty, nx = 0, ny;
		collision->filterCollision(coEvents, coEventsResult, min_tx, min_ty, nx, ny);
		y += min_ty * dy + ny * 0.4f;

		if (nx != 0) speed.vx = 0.0f;
		if (ny != 0) speed.vy = 0.0f;

		if (state != appear + 1)
		{
			state = appear + 1;
			//height = 40;
		}

	}

	for (UINT i = 0; i < size; ++i) delete coEvents[i];
}

void MegamanX::createItems(DynamicObject* obj)
{
	if (dynamic_cast<BlastHornet*>(obj)) return;
	ITemHP* item = ITemHP::tryCreateITemHP(obj->x, obj->y);
	if (item)
		items->emplace_back(item);
}