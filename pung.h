#ifndef PUNG_H
#define PUNG_H

#include <new>

#include <GL/gl.h>

const float MIN_X = -2.3f;
const float MAX_X = 2.3f;
const float MIN_Y = -1.6f;
const float MAX_Y = 1.6f;

const float MAX_SPEED = 0.01f;
const float MIN_SPEED = 0.001f;

const float PADDLE_W = 1.0f;
const float PADDLE1_Z = -5.0f;
const float PADDLE2_Z = -30.0f;
const float BALL_START_Z = -17.5f;
const float BALL_START_SPEED = 0.01f;
const float BALL_RADIUS_SMALL = 0.2f;
const float BALL_RADIUS_MEDIUM = 0.35f;
const float BALL_RADIUS_BIG = 0.5f;
const float EDGE_DIST = 0.01f;
const float BALL_SPEED_INC = 1.1f;

const float FLASH_SPEED = 0.002f;

extern GLfloat paddle1Pos[3];
extern GLfloat paddle2Pos[3];
extern GLfloat ballDir[3];
extern GLfloat nextEndzonePos[3];
extern FONT* myfont;

const int EASY = 0;
const int NORMAL = 1;
const int HARD = 2;

struct Configuration {
	bool fullscreen;
	int performance;
	int sfxVolume;
	int musicVolume;
	int screenWidth;
	int screenHeight;
	int colorScheme;
	int input1;
	int input2;
	float input1Speed;
	float input2Speed;
	float wallColorRatio;
	float ballRadius;
	int flashing;
	bool soundInstalled;
	int difficulty;
	bool showTime;
};

extern Configuration cfg;

const int _INPUT_MOUSE = 0; // these appears to be defined on MinGW, hence the leading _
const int INPUT_KB = 1;
const int INPUT_JOY = 2;
const int INPUT_AI = 3;

class Input {
public:
	virtual void update(int step) {}
	virtual bool isAI() { return false; } // is not a human player
	virtual bool useAbsolute() { return false; } // returns absolute X/Y
	virtual bool l() { return false; }
	virtual bool r() { return false; }
	virtual bool u() { return false; }
	virtual bool d() { return false; }
	virtual bool i() { return false; }
	virtual bool o() { return false; }
	virtual GLfloat getX() { return 0; }
	virtual GLfloat getY() { return 0; }
};

extern Input* input1;
extern Input* input2;

class Keyboard1Input : public Input {
public:
	bool l();
	bool r();
	bool u();
	bool d();
};

class Keyboard2Input : public Input {
public:
	bool l();
	bool r();
	bool u();
	bool d();
};

class AIInput : public Input {
public:
	void update(int step);
	bool l();
	bool r();
	bool u();
	bool d();
	bool i();
	bool o();
	AIInput(int player) {
		this->player = player;
		foundDest = false;
		movingAway = false;
		reachedDest = false;
		reachedX = false;
		reachedY = false;
		ll = rr = uu = dd = false;
		hits = 0;
		misses = 0;
	}
protected:
	int player;
	bool ll;
	bool rr;
	bool uu;
	bool dd;
	GLfloat destX;
	GLfloat destY;
	bool foundDest;
	GLfloat dest[3];
	bool movingAway;
	bool reachedDest;
	bool reachedX;
	bool reachedY;
	int hits;
	int misses;
};

class MouseInput1 : public Input {
public:
	bool useAbsolute();
	GLfloat getX();
	GLfloat getY();
};

class MouseInput2 : public Input {
public:
	bool useAbsolute();
	GLfloat getX();
	GLfloat getY();
};

class Joy1Input : public Input {
public:
	void update(int step);
	bool l();
	bool r();
	bool u();
	bool d();
};

class Joy2Input : public Input {
public:
	void update(int step);
	bool l();
	bool r();
	bool u();
	bool d();
};

extern void setVolumes(int sfxVolume, int musicVolume);

extern void setInputs();
extern void handleInput(int step);

const int FLASH_OFF = 0;
const int FLASH_UP = 1;
const int FLASH_DOWN = 2;

extern int getSensitivityStop(float speed);
extern float getSensitivity(int stop);
extern bool config();
extern void load_config();

#endif // PUNG_H
