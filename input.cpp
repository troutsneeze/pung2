/*
 * Copyright 2006-2007 Trent Gamblin
 */

#include <stdio.h>
#include <new>

#include <allegro.h>
#ifdef _WIN32
#include <winalleg.h>
#include <windows.h>
#endif

#include "pung.h"

bool Keyboard1Input::l()
{
	return key[KEY_LEFT];
}

bool Keyboard1Input::r()
{
	return key[KEY_RIGHT];
}

bool Keyboard1Input::u()
{
	return key[KEY_UP];
}

bool Keyboard1Input::d()
{
	return key[KEY_DOWN];
}

/*
bool Keyboard1Input::i()
{
	return key[KEY_LSHIFT];
}

bool Keyboard1Input::o()
{
	return key[KEY_RSHIFT];
}
*/

bool Keyboard2Input::l()
{
	return key[KEY_A];
}

bool Keyboard2Input::r()
{
	return key[KEY_D];
}

bool Keyboard2Input::u()
{
	return key[KEY_W];
}

bool Keyboard2Input::d()
{
	return key[KEY_S];
}

void AIInput::update(int step)
{
	const int EASY_RAND = 9;
	const int NORMAL_RAND = 12;
	const int HARD_RAND = 15;

	if (!foundDest) {
		if ((player == 1 && ballDir[2] > 0.0f) ||
				(player == 2 && ballDir[2] < 0.0f)) {
			movingAway = false;
			foundDest = true;
			reachedX = false;
			reachedY = false;
			reachedDest = false;
			ll = false;
			rr = false;
			uu = false;
			dd = false;
			dest[0] = nextEndzonePos[0];
			dest[1] = nextEndzonePos[1];
			dest[2] = nextEndzonePos[2];
			destX = nextEndzonePos[0];
			destY = nextEndzonePos[1];
			int r;
			if (cfg.difficulty == EASY)
				r = EASY_RAND;
			else if (cfg.difficulty == NORMAL)
				r = NORMAL_RAND;
			else
				r = HARD_RAND;
			float p = (float)misses / (float)(misses+hits) * 100.0f;
			float wanted = 100.0f / (float)r;
			int miss;
			if ((p+20.0f) < wanted) {
				miss = 1;
			}
			else if (((p-20.0f) > wanted) && ((hits+misses) > 5)) {
				miss = 0;
			}
			else
				miss = rand() % r;
			if (miss == 1) {
				misses++;
				float p = (float)misses / (float)(misses+hits)
					* 100.0f;
				float f;
				f = rand() % 9999;
				f /= 9999;
				f -= 0.5f;
				f *= 0.3f;
				f += PADDLE_W/2 + cfg.ballRadius;
				destX += f;
				f = rand() % 9999;
				f /= 9999;
				f -= 0.5f;
				f *= 0.3f;
				f += PADDLE_W/2 + cfg.ballRadius;
				destY += f;
			}
			else {
				hits++;
			}
		}
		else {
			if (player == 1) {
				if (ballDir[2] < 0.0f)
					movingAway = true;
				else
					movingAway = false;
			}
			else {
				if (ballDir[2] > 0.0f)
					movingAway = true;
				else
					movingAway = false;
			}
		}
	}
	else {
		if (!movingAway) {
			if ((nextEndzonePos[0] != dest[0]) ||
					(nextEndzonePos[1] != dest[1]) ||
					(nextEndzonePos[2] != dest[2])) {
				movingAway = true;
				dest[0] = nextEndzonePos[0];
				dest[1] = nextEndzonePos[1];
				dest[2] = nextEndzonePos[2];
			}
		}
		else {
			if ((nextEndzonePos[0] != dest[0]) ||
					(nextEndzonePos[1] != dest[1]) ||
					(nextEndzonePos[2] != dest[2])) {
				foundDest = false;
				movingAway = false;
			}
		}
	}

	float* pos;
	
	if (player == 1)
		pos = paddle1Pos;
	else
		pos = paddle2Pos;

	if (ll && pos[0] < destX)
		reachedX = true;
	else if (rr && pos[0] > destX)
		reachedX = true;
	if (uu && pos[1] > destY)
		reachedY = true;
	else if (dd && pos[1] < destY)
		reachedY = true;

	if (reachedX && reachedY) {
		reachedX = false;
		reachedY = false;
		reachedDest = true;
	}

	ll = rr = uu = dd = false;

	if (reachedDest)
		return;

	if (player == 1 && ballDir[2] > 0.0f) {
		if (!reachedX) {
			if (paddle1Pos[0] < destX)
				rr = true;
			else if (paddle1Pos[0] > destX)
				ll = true;
		}
		if (!reachedY) {
			if (paddle1Pos[1] < destY)
				uu = true;
			else if (paddle1Pos[1] > destY)
				dd = true;
		}
	}
	else if (player == 2 && ballDir[2] < 0.0f) {
		if (!reachedX) {
			if (paddle2Pos[0] < destX)
				rr = true;
			else if (paddle2Pos[0] > destX)
				ll = true;
		}
		if (!reachedY) {
			if (paddle2Pos[1] < destY)
				uu = true;
			else if (paddle2Pos[1] > destY)
				dd = true;
		}
	}
}

bool AIInput::l()
{
	return ll;
}

bool AIInput::r()
{
	return rr;
}

bool AIInput::u()
{
	return uu;
}

bool AIInput::d()
{
	return dd;
}

bool AIInput::i()
{
	return false;
}

bool AIInput::o()
{
	return false;
}

bool MouseInput1::useAbsolute()
{
	return true;
}

GLfloat MouseInput1::getX()
{
	int mx = mouse_x;
	float f = (float)mx / (float)SCREEN_W;
	f *= MAX_X * 2.0f;
	f -= MAX_X;
	return f;
}

GLfloat MouseInput1::getY()
{
	int my = mouse_y;
	float f = (float)my / (float)SCREEN_H;
	f *= MAX_Y * 2.0f;
	f -= MAX_Y;
	f = -f;
	return f;
}

bool MouseInput2::useAbsolute()
{
	return true;
}

GLfloat MouseInput2::getX()
{
	int mx = mouse_x;
	float f = (float)mx / (float)SCREEN_W;
	f *= MAX_X * 2.0f;
	f -= MAX_X;
	f *= 7.0f;
	if (f < MIN_X)
		f = MIN_X;
	else if (f > MAX_X)
		f = MAX_X;
	return f;
}

GLfloat MouseInput2::getY()
{
	int my = mouse_y;
	float f = (float)my / (float)SCREEN_H;
	f *= MAX_Y * 2.0f;
	f -= MAX_Y;
	f = -f;
	f *= 7.0f;
	if (f < MIN_Y)
		f = MIN_Y;
	else if (f > MAX_Y)
		f = MAX_Y;
	return f;
}

void Joy1Input::update(int step)
{
	poll_joystick();
}

bool Joy1Input::l()
{
	return joy[0].stick[0].axis[0].d1;
}

bool Joy1Input::r()
{
	return joy[0].stick[0].axis[0].d2;
}

bool Joy1Input::u()
{
	return joy[0].stick[0].axis[1].d1;
}

bool Joy1Input::d()
{
	return joy[0].stick[0].axis[1].d2;
}

void Joy2Input::update(int step)
{
	poll_joystick();
}

bool Joy2Input::l()
{
	return joy[1].stick[0].axis[0].d1;
}

bool Joy2Input::r()
{
	return joy[1].stick[0].axis[0].d2;
}

bool Joy2Input::u()
{
	return joy[1].stick[0].axis[1].d1;
}

bool Joy2Input::d()
{
	return joy[1].stick[0].axis[1].d2;
}

static Input* createInput(int player, int type)
{
	switch (type) {
		case INPUT_KB:
			if (player == 1)
				return new Keyboard1Input;
			else if (cfg.input1 == INPUT_KB)
				return new Keyboard2Input;
			else
				return new Keyboard1Input;
		case INPUT_AI:
			return new AIInput(player);
		case _INPUT_MOUSE:
			if (player == 1)
				return new MouseInput1;
			else
				return new MouseInput2;
		case INPUT_JOY:
			if (player == 1)
				return new Joy1Input;
			else if (cfg.input1 == INPUT_JOY)
				return new Joy2Input;
			else
				return new Joy1Input;
		default:
			return 0;
	}
}

void setInputs()
{
	if (input1)
		delete input1;
	if (input2)
		delete input2;
	input1 = createInput(1, cfg.input1);
	input2 = createInput(2, cfg.input2);
}

void handleInput(int step)
{
	input1->update(step);

	if (input1->useAbsolute()) {
		paddle1Pos[0] = input1->getX();
		paddle1Pos[1] = input1->getY();
	}
	else {
		float speed;
		if (input1->isAI())
			speed = 0.05;
		else
			speed = cfg.input1Speed;
		if (input1->l()) {
			paddle1Pos[0] -= speed * step;
			if (paddle1Pos[0] < MIN_X)
				paddle1Pos[0] = MIN_X;
		}
		if (input1->r()) {
			paddle1Pos[0] += speed * step;
			if (paddle1Pos[0] > MAX_X)
				paddle1Pos[0] = MAX_X;
		}
		if (input1->u()) {
			paddle1Pos[1] += speed * step;
			if (paddle1Pos[1] > MAX_Y)
				paddle1Pos[1] = MAX_Y;
		}
		if (input1->d()) {
			paddle1Pos[1] -= speed * step;
			if (paddle1Pos[1] < MIN_Y)
				paddle1Pos[1] = MIN_Y;
		}
	}

	input2->update(step);
	
	if (input2->useAbsolute()) {
		paddle2Pos[0] = input2->getX();
		paddle2Pos[1] = input2->getY();
	}
	else {
		float speed;
		if (input2->isAI())
			speed = 0.05;
		else
			speed = cfg.input2Speed;
		if (input2->l()) {
			paddle2Pos[0] -= speed * step;
			if (paddle2Pos[0] < MIN_X)
				paddle2Pos[0] = MIN_X;
		}
		if (input2->r()) {
			paddle2Pos[0] += speed * step;
			if (paddle2Pos[0] > MAX_X)
				paddle2Pos[0] = MAX_X;
		}
		if (input2->u()) {
			paddle2Pos[1] += speed * step;
			if (paddle2Pos[1] > MAX_Y)
				paddle2Pos[1] = MAX_Y;
		}
		if (input2->d()) {
			paddle2Pos[1] -= speed * step;
			if (paddle2Pos[1] < MIN_Y)
				paddle2Pos[1] = MIN_Y;
		}
	}
}
