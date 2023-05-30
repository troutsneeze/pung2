/*
 * Copyright 2006-2007 Trent Gamblin
 */

#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <new>

#include <allegro.h>
#ifdef _WIN32
#include <winalleg.h>
#include <windows.h>
#endif
#ifdef __linux__
#include <sys/time.h>
#endif

#include <alleggl.h>

#include <GL/glu.h>
#include <glut.h>

#include <TGUI/tgui.h>
#include <TGUI/awidgets.h>

extern "C" {
#include <logg.h>
}

#include "pung.h"

const int PERFORMANCE_OPTIONS = 3;
const int PERFORMANCE_LOW = 0;
const int PERFORMANCE_NORMAL = 1;
const int PERFORMANCE_HIGH = 2;

const int TIME_STEPS[PERFORMANCE_OPTIONS] = {
	63,
	28,
	0
};

Configuration cfg = {
	true,
	PERFORMANCE_NORMAL,
	255,
	64,
	640,
	480,
	0,
	_INPUT_MOUSE,
	INPUT_AI,
	((MAX_SPEED-MIN_SPEED)/2.0f)+MIN_SPEED,
	((MAX_SPEED-MIN_SPEED)/2.0f)+MIN_SPEED,
	0.0f,
	BALL_RADIUS_MEDIUM,
	FLASH_OFF,
	false,
	NORMAL,
	true
};

struct Color {
	float r;
	float g;
	float b;
	float a;
};

struct ColorScheme {
	Color paddleColor;
	Color wallColor;
	Color wallPointColor;
	Color ballColor;
};

const int NUM_COLOR_SCHEMES = 1;

ColorScheme colorSchemes[NUM_COLOR_SCHEMES] = {
	{
		{ 0.5f, 0.5f, 0.5f, 0.5f },
		{ 0.0f, 0.2f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 1.0f, 1.0f, 0.0f, 1.0f }
	}
};

static GLfloat paddleVerts[4][3] = {
	{ PADDLE_W/2, PADDLE_W/2, 0 },
	{ -PADDLE_W/2, PADDLE_W/2, 0 },
	{ -PADDLE_W/2, -PADDLE_W/2, 0 },
	{ PADDLE_W/2, -PADDLE_W/2, 0 }
};

GLfloat paddle1Pos[3] = { 0.0f, 0.0f, PADDLE1_Z };
GLfloat paddle2Pos[3] = { 0.0f, 0.0f, PADDLE2_Z };
GLfloat ballPos[3] = { 0.0f, 0.0f, BALL_START_Z };
GLfloat ballDir[3] = { 0.0f, 0.0f, 0.0f }; // ball direction
GLfloat nextEndzonePos[3];
GLfloat tmpPos[3];
GLfloat tmpDir[3];

int player1Points = 0;
int player2Points = 0;

int timeStep = 63;

struct Music {
	int voice;
	SAMPLE* samp;
	bool playing;
};

Music currMusic;
SAMPLE* wallSamp = 0;
SAMPLE* paddleSamp = 0;
SAMPLE* pointSamp = 0;

Input* input1 = 0;
Input* input2 = 0;

BITMAP* mouseCursor = 0;
FONT* myfont;

void setPerformanceLevel(int performance)
{
	cfg.performance = performance;
	timeStep = TIME_STEPS[performance];
}

void DeleteMusic(void)
{
	if (currMusic.samp)
		destroy_sample(currMusic.samp);
}

void PlayMusic(char* filename)
{
	currMusic.samp = logg_load(filename);
	if (!currMusic.samp)
		return;
	currMusic.voice =
		play_sample(currMusic.samp, cfg.musicVolume, 128, 1000, 1);
	currMusic.playing = true;
}

void PlaySFX(SAMPLE* s)
{
	play_sample(s, cfg.sfxVolume, 128, 1000, 0);
}

void drawPaddle()
{
	glColor4f(
		colorSchemes[cfg.colorScheme].paddleColor.r,
		colorSchemes[cfg.colorScheme].paddleColor.g,
		colorSchemes[cfg.colorScheme].paddleColor.b,
		colorSchemes[cfg.colorScheme].paddleColor.a
	);

	glBegin(GL_QUADS);
		glVertex3fv(paddleVerts[0]);
		glVertex3fv(paddleVerts[1]);
		glVertex3fv(paddleVerts[2]);
		glVertex3fv(paddleVerts[3]);
	glEnd();
}

const GLfloat GRID_Z = 2.0f;
const GLfloat GRID_X = 0.8f;
const GLfloat GRID_Y = 0.6f;

void drawWalls()
{
	glLineWidth(1.0f);

	int wallColor = makecol(
		colorSchemes[cfg.colorScheme].wallColor.r,
		colorSchemes[cfg.colorScheme].wallColor.g,
		colorSchemes[cfg.colorScheme].wallColor.b);
	int wallPointColor = makecol(
		colorSchemes[cfg.colorScheme].wallPointColor.r,
		colorSchemes[cfg.colorScheme].wallPointColor.g,
		colorSchemes[cfg.colorScheme].wallPointColor.b);
	int color = aWgtInterpolateColor(cfg.wallColorRatio,
		wallColor, wallPointColor);

	glColor3ub(getr(color), getg(color), getb(color));

	bool last = false;
	GLfloat z = PADDLE1_Z;

	for (;;) {
		glBegin(GL_LINES);
			glVertex3f(MIN_X-PADDLE_W/2, MIN_Y-PADDLE_W/2, z); // left
			glVertex3f(MIN_X-PADDLE_W/2, MAX_Y+PADDLE_W/2, z);
			glVertex3f(MAX_X+PADDLE_W/2, MIN_Y-PADDLE_W/2, z); // right
			glVertex3f(MAX_X+PADDLE_W/2, MAX_Y+PADDLE_W/2, z);
			glVertex3f(MIN_X-PADDLE_W/2, MIN_Y-PADDLE_W/2, z); // bottom
			glVertex3f(MAX_X+PADDLE_W/2, MIN_Y-PADDLE_W/2, z);
			glVertex3f(MIN_X-PADDLE_W/2, MAX_Y+PADDLE_W/2, z); // top
			glVertex3f(MAX_X+PADDLE_W/2, MAX_Y+PADDLE_W/2, z);
		glEnd();
		if (last)
			break;
		z -= GRID_Z;
		if (z < PADDLE2_Z) {
			z = PADDLE2_Z;
			last = true;
		}
	}

	GLfloat x = MIN_X-PADDLE_W/2;
	last = false;

	for (;;) {
		glBegin(GL_LINES);
			glVertex3f(x, MAX_Y+PADDLE_W/2, PADDLE1_Z); // top
			glVertex3f(x, MAX_Y+PADDLE_W/2, PADDLE2_Z);
			glVertex3f(x, MIN_Y-PADDLE_W/2, PADDLE1_Z); // bottom
			glVertex3f(x, MIN_Y-PADDLE_W/2, PADDLE2_Z);
		glEnd();
		if (last)
			break;
		x += GRID_X;
		if (x > (MAX_X+PADDLE_W/2)) {
			x = MAX_X+PADDLE_W/2;
			last = true;
		}
	}

	GLfloat y = MIN_Y-PADDLE_W/2;
	last = false;

	for (;;) {
		glBegin(GL_LINES);
			glVertex3f(MIN_X-PADDLE_W/2, y, PADDLE1_Z); // left
			glVertex3f(MIN_X-PADDLE_W/2, y, PADDLE2_Z);
			glVertex3f(MAX_X+PADDLE_W/2, y, PADDLE1_Z); // right
			glVertex3f(MAX_X+PADDLE_W/2, y, PADDLE2_Z);
		glEnd();
		if (last)
			break;
		y += GRID_Y;
		if (y > (MAX_Y+PADDLE_W/2)) {
			y = MAX_Y+PADDLE_W/2;
			last = true;
		}
	}
}

bool ballHittingPaddle(GLfloat pos[3])
{
	if (
		(ballPos[0] < (pos[0]-PADDLE_W/2-cfg.ballRadius)) ||
		(ballPos[0] > (pos[0]+PADDLE_W/2+cfg.ballRadius)) ||
		(ballPos[1] < (pos[1]-PADDLE_W/2-cfg.ballRadius)) ||
		(ballPos[1] > (pos[1]+PADDLE_W/2+cfg.ballRadius))
	   )
		return false;
	return true;
}

void saveBall()
{
	tmpPos[0] = ballPos[0];
	tmpPos[1] = ballPos[1];
	tmpPos[2] = ballPos[2];
	tmpDir[0] = ballDir[0];
	tmpDir[1] = ballDir[1];
	tmpDir[2] = ballDir[2];
}

void restoreBall()
{
	ballPos[0] = tmpPos[0];
	ballPos[1] = tmpPos[1];
	ballPos[2] = tmpPos[2];
	ballDir[0] = tmpDir[0];
	ballDir[1] = tmpDir[1];
	ballDir[2] = tmpDir[2];
}

bool moveBall(int step, bool forPoints);

void findNextEndzonePos()
{
	while (!moveBall(1, false))
		;
	nextEndzonePos[0] = ballPos[0];
	nextEndzonePos[1] = ballPos[1];
	nextEndzonePos[2] = ballPos[2];
}

// returns true when it hits an end zone
bool moveBall(int step, bool forPoints)
{
	bool ret = false;

	ballPos[0] += ballDir[0] * step;
	if (ballPos[0] < (MIN_X-PADDLE_W/2+cfg.ballRadius)) {
		if (forPoints)
			PlaySFX(wallSamp);
		ballPos[0] = MIN_X-PADDLE_W/2+cfg.ballRadius+EDGE_DIST;
		ballDir[0] = -ballDir[0];
	}
	else if (ballPos[0] > (MAX_X+PADDLE_W/2-cfg.ballRadius)) {
		if (forPoints)
			PlaySFX(wallSamp);
		ballPos[0] = MAX_X+PADDLE_W/2-cfg.ballRadius-EDGE_DIST;
		ballDir[0] = -ballDir[0];
	}
	ballPos[1] += ballDir[1] * step;
	if (ballPos[1] < (MIN_Y-PADDLE_W/2+cfg.ballRadius)) {
		if (forPoints)
			PlaySFX(wallSamp);
		ballPos[1] = MIN_Y-PADDLE_W/2+cfg.ballRadius+EDGE_DIST;
		ballDir[1] = -ballDir[1];
	}
	else if (ballPos[1] > (MAX_Y+PADDLE_W/2-cfg.ballRadius)) {
		if (forPoints)
			PlaySFX(wallSamp);
		ballPos[1] = MAX_Y+PADDLE_W/2-cfg.ballRadius-EDGE_DIST;
		ballDir[1] = -ballDir[1];
	}
	ballPos[2] += ballDir[2] * step;
	if (ballPos[2] < (PADDLE2_Z+(forPoints ? cfg.ballRadius : 0.0f))) {
		if (!ballHittingPaddle(paddle2Pos)) {
			if (forPoints) {
				PlaySFX(pointSamp);
				player1Points++;
				cfg.flashing = FLASH_UP;
				if (ballDir[2] < 0)
					ballDir[2] = -BALL_START_SPEED;
				else
					ballDir[2] = BALL_START_SPEED;
			}
		}
		else if (forPoints)
			PlaySFX(paddleSamp);
		ballPos[2] = PADDLE2_Z+cfg.ballRadius+EDGE_DIST;
		ballDir[2] = -ballDir[2];
		ballDir[2] *= BALL_SPEED_INC;
		if (forPoints) {
			saveBall();
			findNextEndzonePos();
			restoreBall();
		}
		cfg.wallColorRatio = 0.00f;
		ret = true;
	}
	else if (ballPos[2] > (PADDLE1_Z-(forPoints ? cfg.ballRadius : 0.0f))) {
		if (!ballHittingPaddle(paddle1Pos)) {
			if (forPoints) {
				PlaySFX(pointSamp);
				player2Points++;
				cfg.flashing = FLASH_UP;
				if (ballDir[2] < 0)
					ballDir[2] = -BALL_START_SPEED;
				else
					ballDir[2] = BALL_START_SPEED;
			}
		}
		else if (forPoints)
			PlaySFX(paddleSamp);
		ballPos[2] = PADDLE1_Z-cfg.ballRadius-EDGE_DIST;
		ballDir[2] = -ballDir[2];
		ballDir[2] *= BALL_SPEED_INC;
		if (forPoints) {
			saveBall();
			findNextEndzonePos();
			restoreBall();
		}
		cfg.wallColorRatio = 0.0f;
		ret = true;
	}

	return ret;
}

const int GRANULARITY = 10000;

void setRandomBallDirection()
{
	int r = rand() % GRANULARITY;
	float f = (float)r / (float)GRANULARITY;
	if (f > 0.3f)
		f = 0.3f;
	else if (f < 0.9f)
		f = 0.9f;
	f = f - 0.5f;
	f /= 100.0f;
	ballDir[0] = f;
	r = rand() % GRANULARITY;
	f = (float)r / (float)GRANULARITY;
	if (f > 0.3f)
		f = 0.3f;
	else if (f < 0.7f)
		f = 0.7f;
	f = f - 0.5f;
	f /= 100.0f;
	ballDir[0] = f;
	ballDir[1] = f;
	ballDir[2] = (rand() % 2) ? BALL_START_SPEED : -BALL_START_SPEED;
	ballPos[0] = 0.0f;
	ballPos[1] = 0.0f;
	ballPos[2] = BALL_START_Z;
}

void drawBall()
{
	glColor4f(
		colorSchemes[cfg.colorScheme].ballColor.r,
		colorSchemes[cfg.colorScheme].ballColor.g,
		colorSchemes[cfg.colorScheme].ballColor.b,
		colorSchemes[cfg.colorScheme].ballColor.a
	);

	glutSolidSphere(cfg.ballRadius, 16, 16);
}

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	drawWalls();
	glLoadIdentity();
	glTranslatef(paddle2Pos[0], paddle2Pos[1], paddle2Pos[2]);
	drawPaddle();
	glLoadIdentity();
	glTranslatef(ballPos[0], ballPos[1], ballPos[2]);
	drawBall();
	glLoadIdentity();
	glTranslatef(paddle1Pos[0], paddle1Pos[1], paddle1Pos[2]);
	drawPaddle();

	glFlush();
}

void resetScore()
{
	player1Points = 0;
	player2Points = 0;
}

void updateFlash(int step)
{
	if (cfg.flashing == FLASH_UP) {
		cfg.wallColorRatio += FLASH_SPEED * step;
		if (cfg.wallColorRatio > 1.0f) {
			cfg.wallColorRatio = 1.0f;
			cfg.flashing = FLASH_DOWN;
		}
	}
	else if (cfg.flashing == FLASH_DOWN) {
		cfg.wallColorRatio -= FLASH_SPEED * step;
		if (cfg.wallColorRatio < 0.0f) {
			cfg.wallColorRatio = 0.0f;
			cfg.flashing = FLASH_OFF;
		}
	}
}

int colorDepths[] = { 32, 24, 16, 15, -1 };

int main(int argc, char **argv)
{
	srand(time(NULL));
	load_config();

	if (allegro_init() != 0) {
		printf("Error: Couldn't initialize Allegro.\n");
		return 1;
	}

	if (install_timer() < 0) {
		allegro_message("Error: Couldn't install timer.");
		return 1;
	}

	rest(1000);	// double click fix for gnome

	if (install_allegro_gl() == -1) {
		allegro_message("Error: Couldn't initialize AllegroGL.");
		return 1;
	}

	glutInit(&argc, argv);
	
	if (install_mouse() == -1) {
		allegro_message("Error: Couldn't install mouse driver.");
		return 1;
	}

	if (!install_sound(DIGI_AUTODETECT, MIDI_NONE, 0))
		cfg.soundInstalled = true;

	if (install_keyboard() < 0) {
		allegro_message("Error: Couldn't install keyboard driver.");
		return 1;
	}

	install_joystick(JOY_TYPE_AUTODETECT);

	allegro_gl_clear_settings();
	allegro_gl_set(AGL_DOUBLEBUFFER, 1);
	allegro_gl_set(AGL_RENDERMETHOD, 1);
	allegro_gl_set(AGL_SUGGEST, AGL_RENDERMETHOD);

	if (!cfg.fullscreen) {
		allegro_gl_set(AGL_WINDOWED, TRUE);
		allegro_gl_set(AGL_SUGGEST, AGL_WINDOWED);
	}

	int i;

	for (i = 0; colorDepths[i] > 0; i++) {
		set_color_depth(colorDepths[i]);
		allegro_gl_set(AGL_COLOR_DEPTH, colorDepths[i]);
		int driver;
		if (cfg.fullscreen)
			driver = GFX_OPENGL;
		else
			driver = GFX_OPENGL_WINDOWED;
		if (set_gfx_mode(driver, cfg.screenWidth,
					cfg.screenHeight, 0, 0) == 0)
			break;
	}

	if (colorDepths[i] < 0) {
		if (set_gfx_mode(GFX_OPENGL, 640, 480, 0, 0) == 0) {
			cfg.screenWidth = 640;
			cfg.screenHeight = 480;
		}
		else {
			allegro_message("Error: Couldn't find a supported graphics mode.");
			return 1;
		}
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_2D);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Depth buffer setup */
	glClearDepth(40.0f);

	/* The Type Of Depth Test To Do */
	glDepthFunc(GL_LEQUAL);

	/* Really Nice Perspective Calculations */
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glViewport(0, 0, (GLsizei)cfg.screenWidth,
			(GLsizei)cfg.screenHeight);

	/* change to the projection matrix and set our viewing volume. */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	/* Set our perspective */
	GLfloat ratio = (GLfloat)cfg.screenWidth /
		(GLfloat)cfg.screenHeight;
	gluPerspective(45.0f, ratio, 0.1f, 100.0f);

	/* Make sure we're chaning the model view and not the projection */
	glMatrixMode(GL_MODELVIEW);

	/* Reset The View */
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	aWgtInit();

	mouseCursor = load_bitmap("data/mouse.pcx", 0);
	set_mouse_sprite(mouseCursor);	

	BITMAP* closeButtonBmp = load_bitmap("data/frameClose.pcx", 0);
	aWgtSet(AWGT_FRAME_CLOSE_BMP, closeButtonBmp);

	int frameAlpha;

	myfont = load_font("data/font.pcx", 0, 0);

	wallSamp = logg_load("data/wall.ogg");
	paddleSamp = logg_load("data/paddle.ogg");
	pointSamp = logg_load("data/point.ogg");

	allegro_gl_set_texture_format(GL_RGB8);

	while (config()) {
		try {
			setInputs();
		}
		catch (std::bad_alloc) {
			allegro_message("Error creating input drivers.");
			return 1;
		}
		int stop = 40;
		if (cfg.input1 == _INPUT_MOUSE) {
			stop = getSensitivityStop(cfg.input1Speed);
		}
		else if (cfg.input2 == _INPUT_MOUSE) {
			stop = getSensitivityStop(cfg.input2Speed);
		}
		stop /= 20;
		stop = 5 - stop;
		set_mouse_speed(stop, stop);
		setPerformanceLevel(cfg.performance);
		PlayMusic("data/music.ogg");
		setRandomBallDirection();
		resetScore();
		cfg.flashing = FLASH_OFF;

		saveBall();
		findNextEndzonePos();
		restoreBall();
		int frames = 0;
		long startTime = tguiCurrentTimeMillis();
		for (;;) {
			long start = tguiCurrentTimeMillis();
			display();
			allegro_gl_set_allegro_mode();
			char s[100];
			sprintf(s, "%d", player1Points);
			aWgtTextout(screen, myfont, s, 0, 0, makecol(255, 255, 255), 0,
					AWGT_TEXT_NORMAL);
			sprintf(s, "%d", player2Points);
			int x = screen->w - text_length(myfont, s);
			aWgtTextout(screen, myfont, s, x, 0, makecol(255, 255, 255), 0,
					AWGT_TEXT_NORMAL);
			if (cfg.showTime) {
				long t = tguiCurrentTimeMillis() -
					startTime;
				int h = t / 1000 / 60 / 60;
				int m = (t / 1000 / 60) % 60;
				int s = (t / 1000) % 60;
				char st[100];
				if (h > 0) {
					sprintf(st, "%02d:%02d:%02d",
							h, m, s);
				}
				else if (m > 9) {
					sprintf(st, "%02d:%02d", m, s);
				}
				else if (m > 0) {
					sprintf(st, "%d:%02d", m, s);
				}
				else {
					sprintf(st, "%d", s);
				}
				int x = (SCREEN_W/2) -
					(text_length(myfont, st) / 2);
				aWgtTextout(screen, myfont, st, x, 0,
					makecol(255, 255, 255), 0,
					AWGT_TEXT_NORMAL);

			}
			allegro_gl_unset_allegro_mode();
			allegro_gl_flip();
			frames++;
			unsigned long now = tguiCurrentTimeMillis();
			unsigned long duration = now - start;
			int step;
			if (duration < timeStep) {
				rest(timeStep - duration);
				step = timeStep;
			}
			else {
				rest(0);
				step = duration;
			}
			if (keypressed()) {
				int k = readkey() >> 8;
				if (k == KEY_ESC)
					break;
			}
			int st;
			for (st = 0; st < step; st++) {
				moveBall(1, true);
			}
			handleInput(step);
			updateFlash(step);
		}
		long duration = tguiCurrentTimeMillis() - startTime;;
		if (duration > 1000)	
			printf("%d FPS\n", frames / (duration / 1000));
		DeleteMusic();
		set_mouse_speed(2, 2);
	}
	
	set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

	return 0;
}
END_OF_MAIN()
