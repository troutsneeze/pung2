/*
 * Copyright 2006-2007 Trent Gamblin
 */

#include <allegro.h>
#ifdef _WIN32
#include <winalleg.h>
#include <windows.h>
#endif
#include <alleggl.h>

#include <TGUI/tgui.h>
#include <TGUI/awidgets.h>

#include <cstdio>

#include "pung.h"

const int MENU_MAIN = 0;
const int MENU_GAME = 1;
const int MENU_INPUT = 2;
const int MENU_GFX = 3;
const int MENU_SOUND = 4;

const int BRIGHTEN_AMOUNT = 40;

static void brighten(BITMAP* bmp)
{
	for (int y = 0; y < bmp->h; y++) {
		for (int x = 0; x < bmp->w; x++) {
			int p = getpixel(bmp, x, y);
			if (p == makecol(255, 0, 255))
				continue;
			int r = getr(p);
			int g = getg(p);
			int b = getb(p);
			r = MIN(255, r + BRIGHTEN_AMOUNT);
			g = MIN(255, g + BRIGHTEN_AMOUNT);
			b = MIN(255, b + BRIGHTEN_AMOUNT);
			int n = makecol(r, g, b);
			putpixel(bmp, x, y, n);
		}
	}
}

static void darken(BITMAP* bmp)
{
	for (int y = 0; y < bmp->h; y++) {
		for (int x = 0; x < bmp->w; x++) {
			int p = getpixel(bmp, x, y);
			if (p == makecol(255, 0, 255))
				continue;
			int r = getr(p);
			int g = getg(p);
			int b = getb(p);
			r = MAX(0, r - BRIGHTEN_AMOUNT);
			g = MAX(0, g - BRIGHTEN_AMOUNT);
			b = MAX(0, b - BRIGHTEN_AMOUNT);
			int n = makecol(r, g, b);
			putpixel(bmp, x, y, n);
		}
	}
}

class WriteError {};
class ReadError {};

static void iputl(long l, FILE *f)
{
	if (fputc(l & 0xFF, f) == EOF) {
		throw new WriteError();
	}
	if (fputc((l >> 8) & 0xFF, f) == EOF) {
		throw new WriteError();
	}
	if (fputc((l >> 16) & 0xFF, f) == EOF) {
		throw new WriteError();
	}
	if (fputc((l >> 24) & 0xFF, f) == EOF) {
		throw new WriteError();
	}
}

static long igetl(FILE *f)
{
	int c1 = fgetc(f);
	if (c1 == EOF) throw new ReadError();
	int c2 = fgetc(f);
	if (c2 == EOF) throw new ReadError();
	int c3 = fgetc(f);
	if (c3 == EOF) throw new ReadError();
	int c4 = fgetc(f);
	if (c4 == EOF) throw new ReadError();
	return (long)c1 | ((long)c2 << 8) | ((long)c3 << 16) | ((long)c4 << 24);
}

int getSensitivityStop(float speed)
{
	float diff = MAX_SPEED - MIN_SPEED;
	speed -= MIN_SPEED;
	speed /= diff;
	speed *= 100.0f;
	return (int)speed;
}

float getSensitivity(int stop)
{
	float diff = MAX_SPEED - MIN_SPEED;
	diff *= (float)stop / 100.0f;
	diff += MIN_SPEED;
	return diff;
}

void load_config()
{
	FILE* f = fopen("pung.cfg", "rb");
	if (!f)
		return;

	try {
		cfg.fullscreen = (bool)igetl(f);
		cfg.performance = igetl(f);
		cfg.sfxVolume = igetl(f);
		cfg.musicVolume = igetl(f);
		cfg.screenWidth = igetl(f);
		cfg.screenHeight = igetl(f);
		cfg.input1 = igetl(f);
		cfg.input2 = igetl(f);
		cfg.input1Speed = getSensitivity(igetl(f));
		cfg.input2Speed = getSensitivity(igetl(f));
		int rad = igetl(f);
		if (rad == 0)
			cfg.ballRadius = BALL_RADIUS_SMALL;
		else if (rad == 1)
			cfg.ballRadius = BALL_RADIUS_MEDIUM;
		else
			cfg.ballRadius = BALL_RADIUS_BIG;
		cfg.difficulty = igetl(f);
		cfg.showTime = (bool)igetl(f);
	}
	catch (ReadError e) {
	}

	fclose(f);
}

static void save_config()
{
	FILE* f = fopen("pung.cfg", "wb");

	try {
		iputl((long)cfg.fullscreen, f);
		iputl(cfg.performance, f);
		iputl(cfg.sfxVolume, f);
		iputl(cfg.musicVolume, f);
		iputl(cfg.screenWidth, f);
		iputl(cfg.screenHeight, f);
		iputl(cfg.input1, f);
		iputl(cfg.input2, f);
		iputl(getSensitivityStop(cfg.input1Speed), f);
		iputl(getSensitivityStop(cfg.input2Speed), f);
		if (cfg.ballRadius == BALL_RADIUS_SMALL)
			iputl(0, f);
		else if (cfg.ballRadius == BALL_RADIUS_MEDIUM)
			iputl(1, f);
		else
			iputl(2, f);
		iputl(cfg.difficulty, f);
		iputl((long)cfg.showTime, f);
	}
	catch (WriteError e) {
	}

	fclose(f);
}

struct Resolution {
	char* name;
	int width;
	int height;
} resolutions[] = {
	{ "640x480", 640, 480 },
	{ "800x600", 800, 600 },
	{ "1024x768", 1024, 768 },
	{ 0, 0, 0 }
};

int currResolution()
{
	for (int i = 0; resolutions[i].name != 0; i++) {
		if (resolutions[i].width == cfg.screenWidth &&
				resolutions[i].height == cfg.screenHeight)
			return i;
	}
	return -1;
}

bool config()
{
	bool draw_cursor = false;

	if (!cfg.fullscreen) {
		if (show_os_cursor(MOUSE_CURSOR_ARROW) != 0)
			draw_cursor = true;
	}
	else
		draw_cursor = true;

	load_config();

	BITMAP* bg = create_bitmap(SCREEN_W, SCREEN_H);
	if (!bg) {
		printf("Error creating background bitmap.\n");
		return false;
	}

	BITMAP* buffer = create_bitmap(SCREEN_W, SCREEN_H);
	if (!buffer) {
		destroy_bitmap(bg);
		printf("Error creating back buffer.\n");
		return false;
	}

	clear(bg);
	clear(buffer);

	BITMAP* gameIcon = load_bitmap("data/gameicon.pcx", 0);
	if (!gameIcon) {
		destroy_bitmap(bg);
		destroy_bitmap(buffer);
		return false;
	}
	BITMAP* gfxIcon = load_bitmap("data/gfxicon.pcx", 0);
	if (!gfxIcon) {
		destroy_bitmap(bg);
		destroy_bitmap(buffer);
		destroy_bitmap(gameIcon);
		return false;
	}
	BITMAP* soundIcon = load_bitmap("data/soundicon.pcx", 0);
	if (!soundIcon) {
		destroy_bitmap(bg);
		destroy_bitmap(buffer);
		destroy_bitmap(gameIcon);
		destroy_bitmap(gfxIcon);
		return false;
	}

	BITMAP* inputIcon = 0;
	BITMAP* inputIconHover = 0;
	BITMAP* inputIconDown = 0;
	BITMAP* gameIconHover = 0;
	BITMAP* gfxIconHover = 0;
	BITMAP* soundIconHover = 0;
	BITMAP* gameIconDown = 0;
	BITMAP* gfxIconDown = 0;
	BITMAP* soundIconDown = 0;
	BITMAP* larrow = 0;
	BITMAP* rarrow = 0;
	BITMAP* larrowd = 0;
	BITMAP* rarrowd = 0;
	BITMAP* closeBmp = 0;
	BITMAP* tabBmp = 0;
	BITMAP* checkedBmp = 0;
	BITMAP* uncheckedBmp = 0;

	inputIcon = load_bitmap("data/inputicon.pcx", 0);
	inputIconHover = create_bitmap(inputIcon->w, inputIcon->h);
	inputIconDown = create_bitmap(inputIcon->w, inputIcon->h);
	gameIconHover = create_bitmap(gameIcon->w, gameIcon->h);
	gfxIconHover = create_bitmap(gfxIcon->w, gfxIcon->h);
	soundIconHover = create_bitmap(soundIcon->w, soundIcon->h);
	gameIconDown = create_bitmap(gameIcon->w, gameIcon->h);
	gfxIconDown = create_bitmap(gfxIcon->w, gfxIcon->h);
	soundIconDown = create_bitmap(soundIcon->w, soundIcon->h);
	larrow = load_bitmap("data/larrow.pcx", 0);
	rarrow = load_bitmap("data/rarrow.pcx", 0);
	larrowd = load_bitmap("data/larrowd.pcx", 0);
	rarrowd = load_bitmap("data/rarrowd.pcx", 0);
	closeBmp = load_bitmap("data/close.pcx", 0);
	tabBmp = load_bitmap("data/tab.pcx", 0);
	checkedBmp = load_bitmap("data/checked.pcx", 0);
	uncheckedBmp = load_bitmap("data/unchecked.pcx", 0);

	if (!gameIconHover || !gfxIconHover || !soundIconHover ||
			!gameIconDown || !gfxIconDown || !soundIconDown ||
			!larrow || !rarrow || !larrowd || !rarrowd ||
			!closeBmp || !tabBmp || !inputIcon) {
		destroy_bitmap(bg);
		destroy_bitmap(buffer);
		destroy_bitmap(gameIcon);
		destroy_bitmap(gfxIcon);
		destroy_bitmap(soundIcon);
		if (gameIconHover)
			destroy_bitmap(gameIconHover);
		if (gfxIconHover)
			destroy_bitmap(gfxIconHover);
		if (soundIconHover)
			destroy_bitmap(soundIconHover);
		if (gameIconDown)
			destroy_bitmap(gameIconDown);
		if (gfxIconDown)
			destroy_bitmap(gfxIconDown);
		if (soundIconDown)
			destroy_bitmap(soundIconDown);
		if (larrow)
			destroy_bitmap(larrow);
		if (rarrow)
			destroy_bitmap(rarrow);
		if (larrowd)
			destroy_bitmap(larrowd);
		if (rarrow)
			destroy_bitmap(rarrowd);
		if (closeBmp)
			destroy_bitmap(closeBmp);
		if (tabBmp)
			destroy_bitmap(tabBmp);
		if (inputIcon)
			destroy_bitmap(inputIcon);
		if (inputIconHover)
			destroy_bitmap(inputIconHover);
		if (inputIconDown)
			destroy_bitmap(inputIconDown);
		if (checkedBmp)
			destroy_bitmap(checkedBmp);
		if (uncheckedBmp)
			destroy_bitmap(uncheckedBmp);
		return false;
	}

	blit(gameIcon, gameIconHover, 0, 0, 0, 0, gameIcon->w, gameIcon->h);
	blit(inputIcon, inputIconHover, 0, 0, 0, 0, inputIcon->w, inputIcon->h);
	blit(gfxIcon, gfxIconHover, 0, 0, 0, 0, gfxIcon->w, gfxIcon->h);
	blit(soundIcon, soundIconHover, 0, 0, 0, 0, soundIcon->w, soundIcon->h);
	blit(gameIcon, gameIconDown, 0, 0, 0, 0, gameIcon->w, gameIcon->h);
	blit(inputIcon, inputIconDown, 0, 0, 0, 0, inputIcon->w, inputIcon->h);
	blit(gfxIcon, gfxIconDown, 0, 0, 0, 0, gfxIcon->w, gfxIcon->h);
	blit(soundIcon, soundIconDown, 0, 0, 0, 0, soundIcon->w, soundIcon->h);

	brighten(gameIconHover);
	brighten(inputIconHover);
	brighten(gfxIconHover);
	brighten(soundIconHover);
	darken(gameIconDown);
	darken(inputIconDown);
	darken(gfxIconDown);
	darken(soundIconDown);

	BITMAP* logo = load_bitmap("data/logo.pcx", 0);
	if (logo)
		masked_blit(logo, bg, 0, 0, bg->w/2-logo->w/2, bg->h/2-logo->h/2,
				logo->w, logo->h);

	int menu = MENU_MAIN;
	tguiInit(buffer);

	aWgtSet(AWGT_FONT, myfont);
	aWgtSet(AWGT_OPTION_LEFT_BMP, larrow);
	aWgtSet(AWGT_OPTION_RIGHT_BMP, rarrow);
	aWgtSet(AWGT_OPTION_LEFT_DOWN_BMP, larrowd);
	aWgtSet(AWGT_OPTION_RIGHT_DOWN_BMP, rarrowd);
	aWgtSet(AWGT_FRAME_CLOSE_BMP, closeBmp);
	aWgtSet(AWGT_SLIDER_TAB_BMP, tabBmp);
	aWgtSet(AWGT_CHECKBOX_CHECKED_BMP, checkedBmp);
	aWgtSet(AWGT_CHECKBOX_UNCHECKED_BMP, uncheckedBmp);

	AWgtFrame* mainFrame = new AWgtFrame(0, 0, 400, 470, "Pung: Configuration",
		true,	makecol(0, 200, 0), makecol(0, 0, 0),
		makecol(150, 150, 150), makecol(255, 255, 0), 0, 
		AWGT_TEXT_SQUARE_BORDER, 128, true, true, false);
	AWgtText* mainText1 = new AWgtText(70, 85, "Game:", makecol(0, 255, 0),
			0, AWGT_TEXT_SQUARE_BORDER);
	tguiCenterWidget(mainText1, 150, 90);
	AWgtText* mainText2 = new AWgtText(70, 160, "Input:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	tguiCenterWidget(mainText2, 150, 165);
	AWgtText* mainText3 = new AWgtText(70, 225, "Graphics:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	tguiCenterWidget(mainText3, 150, 230);
	AWgtText* mainText4 = new AWgtText(70, 300, "Sound:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	tguiCenterWidget(mainText4, 150, 305);
	AWgtIcon* mainIcon1 = new AWgtIcon(260, 65, true,
			gameIcon, gameIconHover, gameIconDown, 0);
	AWgtIcon* mainIcon2 = new AWgtIcon(260, 140, true,
			inputIcon, inputIconHover, inputIconDown, 0);
	AWgtIcon* mainIcon3 = new AWgtIcon(260, 205, true,
			gfxIcon, gfxIconHover, gfxIconDown, 0);
	AWgtIcon* mainIcon4 = new AWgtIcon(260, 280, true,
			soundIcon, soundIconHover, soundIconDown, 0);
	AWgtButton* mainButton1 = new AWgtButton(50, 385, 100, 25, 0,
			"Play!", makecol(200, 200, 200),
			makecol(50, 50, 50), 0, makecol(255, 255, 0), 0,
			AWGT_TEXT_DROP_SHADOW);
	AWgtButton* mainButton2 = new AWgtButton(250, 385, 100, 25, 0,
			"Exit", makecol(200, 200, 200),
			makecol(50, 50, 50), 0, makecol(255, 255, 0), 0,
			AWGT_TEXT_DROP_SHADOW);
	tguiSetParent(0);
	tguiAddWidget(mainFrame);
	tguiSetParent(mainFrame);
	tguiAddWidget(mainText1);
	tguiAddWidget(mainText2);
	tguiAddWidget(mainText3);
	tguiAddWidget(mainText4);
	tguiAddWidget(mainIcon1);
	tguiAddWidget(mainIcon2);
	tguiAddWidget(mainIcon3);
	tguiAddWidget(mainIcon4);
	tguiAddWidget(mainButton1);
	tguiAddWidget(mainButton2);
	tguiTranslateWidget(mainFrame, SCREEN_W/2-200, SCREEN_H/2-235);

	AWgtFrame* gfxFrame = new AWgtFrame(0, 0, 512, 256, "Graphics",
			true, makecol(0, 200, 0), 0, makecol(150, 150, 150),
			makecol(255, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER,
			128, true, true, false);
	AWgtCheckbox* gfxCheckbox = new AWgtCheckbox(0, 0, 0, "Fullscreen",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, false);
	AWgtText* gfxText = new AWgtText(0, 0, "Resolution:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	std::vector<char*> resOptions;
	for (int i = 0; resolutions[i].width > 0; i++)
		resOptions.push_back(resolutions[i].name);
	AWgtOption* gfxOption = new AWgtOption(0, 0, -1, &resOptions,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER,
			0);
	gfxOption->setSelected(currResolution());
	AWgtText* gfxText2 = new AWgtText(0, 0, "Changes require a restart",
			makecol(255, 255, 255), 0, AWGT_TEXT_NORMAL);
	tguiCenterWidget(gfxText2, SCREEN_W/2, SCREEN_H-50);
	AWgtButton* gfxButton = new AWgtButton(0, 0, 50, 25, 0, "OK",
			makecol(200, 200, 200), makecol(50, 50, 50),
			0, makecol(255, 255, 0), 0, AWGT_TEXT_DROP_SHADOW);

	AWgtFrame* soundFrame = new AWgtFrame(0, 0, 400, 256, "Sound", true,
			makecol(0, 200, 0), 0, makecol(150, 150, 150),
			makecol(255, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER,
			128, true, true, false);
	AWgtText* soundText1 = new AWgtText(0, 0, "SFX Volume:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* soundText2 = new AWgtText(0, 0, "Music Volume:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtSlider* soundSlider1 = new AWgtSlider(0, 0, 300, 100, 0, 
			makecol(255, 255, 255), 0, 1);
	AWgtSlider* soundSlider2 = new AWgtSlider(0, 0, 300, 100, 0, 
			makecol(255, 255, 255), 0, 1);
	AWgtButton* soundButton = new AWgtButton(0, 0, 50, 25, 0, "OK",
			makecol(200, 200, 200), makecol(50, 50, 50),
			0, makecol(255, 255, 0), 0, AWGT_TEXT_DROP_SHADOW);

	AWgtFrame* gameFrame = new AWgtFrame(0, 0, 512, 256, "Game", true,
			makecol(0, 200, 0), 0, makecol(150, 150, 150),
			makecol(255, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER,
			128, true, true, false);
	AWgtText* gameText1 = new AWgtText(0, 0, "CPU Usage:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* gameText2 = new AWgtText(0, 0, "Ball Size:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* gameText3 = new AWgtText(0, 0, "Difficulty:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	std::vector<char*> gameOptions1;
	gameOptions1.push_back("Low");
	gameOptions1.push_back("Normal");
	gameOptions1.push_back("High");
	std::vector<char*> gameOptions2;
	gameOptions2.push_back("Small");
	gameOptions2.push_back("Medium");
	gameOptions2.push_back("Big");
	std::vector<char*> gameOptions3;
	gameOptions3.push_back("Easy");
	gameOptions3.push_back("Normal");
	gameOptions3.push_back("Hard");
	AWgtOption* gameOption1 = new AWgtOption(0, 0, 150, &gameOptions1,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, 0);
	AWgtOption* gameOption2 = new AWgtOption(0, 0, 150, &gameOptions2,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, 0);
	AWgtOption* gameOption3 = new AWgtOption(0, 0, 150, &gameOptions3,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, 0);
	AWgtCheckbox* gameCheckbox = new AWgtCheckbox(0, 0, 0, "Show Time",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, false);
	AWgtButton* gameButton = new AWgtButton(0, 0, 50, 25, 0, "OK",
			makecol(200, 200, 200),
			makecol(50, 50, 50), 0, makecol(255, 255, 0), 0,
			AWGT_TEXT_DROP_SHADOW);

	AWgtFrame* inputFrame = new AWgtFrame(0, 0, 512, 256, "Input",
			true, makecol(0, 200, 0), 0, makecol(150, 150, 150),
			makecol(255, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER,
			128, true, true, false);
	AWgtText* inputText1 = new AWgtText(0, 0, "Player 1:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* inputText2 = new AWgtText(0, 0, "Sensitivity:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* inputText3 = new AWgtText(0, 0, "Player 2:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	AWgtText* inputText4 = new AWgtText(0, 0, "Sensitivity:",
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER);
	std::vector<char*> inputOptions;
	inputOptions.push_back("Mouse");
	inputOptions.push_back("Keyboard");
	inputOptions.push_back("Joystick");
	inputOptions.push_back("CPU");
	AWgtOption* inputOption1 = new AWgtOption(0, 0, 200, &inputOptions,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, 0);
	AWgtOption* inputOption2 = new AWgtOption(0, 0, 200, &inputOptions,
			makecol(0, 255, 0), 0, AWGT_TEXT_SQUARE_BORDER, 0);
	AWgtSlider* inputSlider1 = new AWgtSlider(0, 0, 200, 100, 0,
			makecol(255, 255, 255), 0, 1);
	AWgtSlider* inputSlider2 = new AWgtSlider(0, 0, 200, 100, 0,
			makecol(255, 255, 255), 0, 1);
	AWgtButton* inputButton = new AWgtButton(0, 0, 50, 25, 0, "OK",
			makecol(200, 200, 200), makecol(50, 50, 50), 0,
			makecol(255, 255, 0), 0, AWGT_TEXT_DROP_SHADOW);

	blit(bg, buffer, 0, 0, 0, 0, bg->w, bg->h);	

	int timeStep = 63;

	bool quit = false;
	bool ret = true;

	for (;;) {
		long start = tguiCurrentTimeMillis();
		blit(bg, buffer, 0, 0, 0, 0, bg->w, bg->h);
		tguiUpdateDirtyRectangle(0, 0, SCREEN_W, SCREEN_H);
		tguiClearDirtyRectangle();
		TGUIWidget* widget = tguiUpdate();
		switch (menu) {
			case MENU_MAIN:
				if (widget == mainButton1) {
					ret = true;
					quit = true;
				}
				else if (widget == mainButton2 ||
						widget == mainFrame) {
					ret = false;
					quit = true;
				}
				else if (widget == mainIcon1) {
					menu = MENU_GAME;
					gameOption1->setSelected(
							cfg.performance);
					if (cfg.ballRadius == BALL_RADIUS_SMALL)
						gameOption2->setSelected(0);
					else if (cfg.ballRadius == BALL_RADIUS_MEDIUM)
						gameOption2->setSelected(1);
					else
						gameOption2->setSelected(2);
					gameOption3->setSelected(
							cfg.difficulty);
					gameCheckbox->setChecked(
							cfg.showTime);
					tguiPush();
					tguiSetParent(0);
					tguiAddWidget(gameFrame);
					tguiSetParent(gameFrame);
					tguiCenterWidget(gameText1,
							128, 55);
					tguiAddWidget(gameText1);
					tguiCenterWidget(gameText2,
							128, 95);
					tguiAddWidget(gameText2);
					tguiCenterWidget(gameText3,
							128, 135);
					tguiAddWidget(gameText3);
					tguiCenterWidget(gameOption1,
							384, 55);
					tguiAddWidget(gameOption1);
					tguiCenterWidget(gameOption2,
							384, 95);
					tguiAddWidget(gameOption2);
					tguiCenterWidget(gameOption3,
							384, 135);
					tguiAddWidget(gameOption3);
					tguiCenterWidget(gameCheckbox,
							256, 175);
					tguiAddWidget(gameCheckbox);
					tguiCenterWidget(gameButton,
							256, 215);
					tguiAddWidget(gameButton);
					tguiTranslateWidget(gameFrame,
							(SCREEN_W/2)-256,
							(SCREEN_H/2)-128);
				}
				else if (widget == mainIcon2) {
					menu = MENU_INPUT;
					inputOption1->setSelected(cfg.input1);
					inputOption2->setSelected(cfg.input2);
					inputSlider1->setStop(
							getSensitivityStop(cfg.input1Speed));
					inputSlider2->setStop(
							getSensitivityStop(cfg.input2Speed));
					tguiPush();
					tguiSetParent(0);
					tguiAddWidget(inputFrame);
					tguiSetParent(inputFrame);
					tguiCenterWidget(inputText1,
							128, 55);
					tguiAddWidget(inputText1);
					tguiCenterWidget(inputText2,
							128, 95);
					tguiAddWidget(inputText2);
					tguiCenterWidget(inputText3,
							128, 135);
					tguiAddWidget(inputText3);
					tguiCenterWidget(inputText4,
							128, 175);
					tguiAddWidget(inputText4);
					tguiCenterWidget(inputOption1,
							384, 55);
					tguiCenterWidget(inputOption2,
							384, 135);
					tguiCenterWidget(inputSlider1,
							384, 95);
					tguiCenterWidget(inputSlider2,
							384, 175);
					tguiAddWidget(inputOption1);
					tguiAddWidget(inputOption2);
					tguiAddWidget(inputSlider1);
					tguiAddWidget(inputSlider2);
					tguiCenterWidget(inputButton,
							256, 215);
					tguiAddWidget(inputButton);
					tguiTranslateWidget(inputFrame,
							(SCREEN_W/2)-256,
							(SCREEN_H/2)-128);
				}
				else if (widget == mainIcon3) {
					menu = MENU_GFX;
					tguiPush();
					tguiSetParent(0);
					tguiAddWidget(gfxFrame);
					tguiAddWidget(gfxText2);
					tguiSetParent(gfxFrame);
					tguiCenterWidget(gfxCheckbox,
							256, 80);
					tguiAddWidget(gfxCheckbox);
					gfxCheckbox->setChecked(cfg.fullscreen);
					tguiCenterWidget(gfxText,
							256, 110);
					tguiAddWidget(gfxText);
					tguiCenterWidget(gfxOption,
							256, 140);
					tguiAddWidget(gfxOption);
					tguiCenterWidget(gfxButton,
							256, 200);
					tguiAddWidget(gfxButton);
					tguiTranslateWidget(gfxFrame,
							SCREEN_W/2-256,
							SCREEN_H/2-128);
				}
				else if (widget == mainIcon4) {
					menu = MENU_SOUND;
					tguiPush();
					tguiSetParent(0);
					tguiAddWidget(soundFrame);
					tguiSetParent(soundFrame);
					tguiCenterWidget(soundText1,
							200, 70);
					tguiAddWidget(soundText1);
					tguiCenterWidget(soundText2,
							200, 140);
					tguiAddWidget(soundText2);
					int stop = cfg.sfxVolume * 100 /
						255;
					soundSlider1->setStop(stop);
					tguiCenterWidget(soundSlider1,
							200, 95);
					tguiAddWidget(soundSlider1);
					stop = cfg.musicVolume * 100 /
						255;
					soundSlider2->setStop(stop);
					tguiCenterWidget(soundSlider2,
							200, 165);
					tguiAddWidget(soundSlider2);
					tguiCenterWidget(soundButton,
							200, 210);
					tguiAddWidget(soundButton);
					tguiTranslateWidget(soundFrame,
							SCREEN_W/2-200,
							SCREEN_H/2-128);
				}
				break;
			case MENU_GAME:
				if (widget == gameFrame ||
						widget == gameButton) {
					menu = MENU_MAIN;
					tguiTranslateWidget(gameFrame,
							-gameFrame->getX(),
							-gameFrame->getY());
					cfg.performance =
						gameOption1->getSelected();
					switch (gameOption2->getSelected()) {
					case 0:
						cfg.ballRadius =
							BALL_RADIUS_SMALL;
						break;
					case 1:
						cfg.ballRadius =
							BALL_RADIUS_MEDIUM;
						break;
					case 2:
						cfg.ballRadius =
							BALL_RADIUS_BIG;
						break;
					};
					cfg.difficulty =
						gameOption3->getSelected();
					cfg.showTime =
						gameCheckbox->isChecked();
					tguiDeleteActive();
					tguiPop();
					tguiTranslateWidget(mainFrame,
							-mainFrame->getX(),
							-mainFrame->getY());
					tguiTranslateWidget(mainFrame, SCREEN_W/2-200, SCREEN_H/2-235);
				}
				break;
			case MENU_INPUT:
				if (widget == inputFrame ||
						widget == inputButton) {
					menu = MENU_MAIN;
					tguiTranslateWidget(inputFrame,
							-inputFrame->getX(),
							-inputFrame->getY());
					cfg.input1 = inputOption1->getSelected();
					cfg.input2 = inputOption2->getSelected();
					cfg.input1Speed =
						getSensitivity(inputSlider1->getStop());
					cfg.input2Speed =
						getSensitivity(inputSlider2->getStop());
					tguiDeleteActive();
					tguiPop();
					tguiTranslateWidget(mainFrame,
							-mainFrame->getX(),
							-mainFrame->getY());
					tguiTranslateWidget(mainFrame, SCREEN_W/2-200, SCREEN_H/2-235);
				}
				break;
			case MENU_GFX:
				if (widget == gfxFrame ||
						widget == gfxButton) {
					menu = MENU_MAIN;
					tguiTranslateWidget(gfxFrame,
							-gfxFrame->getX(),
							-gfxFrame->getY());
					int oldWidth = cfg.screenWidth;
					int oldHeight = cfg.screenHeight;
					int res = gfxOption->getSelected();
					cfg.screenWidth =
						resolutions[res].width;
					cfg.screenHeight =
						resolutions[res].height;
					bool oldFullscreen = cfg.fullscreen;
					cfg.fullscreen =
						gfxCheckbox->isChecked();
					if (oldFullscreen != cfg.fullscreen ||
							oldWidth != cfg.screenWidth ||
							oldHeight != cfg.screenHeight) {
						ret = false;
						quit = true;
					}
					tguiDeleteActive();
					tguiPop();
					tguiTranslateWidget(mainFrame,
							-mainFrame->getX(),
							-mainFrame->getY());
					tguiTranslateWidget(mainFrame, SCREEN_W/2-200, SCREEN_H/2-235);
				}
				break;
			case MENU_SOUND:
				if (widget == soundFrame ||
						widget == soundButton) {
					menu = MENU_MAIN;
					cfg.sfxVolume =
						soundSlider1->getStop() *
						255 /	100;
					cfg.musicVolume =
						soundSlider2->getStop() *
						255 / 100;
					tguiTranslateWidget(soundFrame,
							-soundFrame->getX(),
							-soundFrame->getY());
					tguiDeleteActive();
					tguiPop();
					tguiTranslateWidget(mainFrame,
							-mainFrame->getX(),
							-mainFrame->getY());
					tguiTranslateWidget(mainFrame, SCREEN_W/2-200, SCREEN_H/2-235);
				}
				break;
		}
		allegro_gl_set_allegro_mode();
		if (draw_cursor)
			draw_sprite(buffer, mouse_sprite, mouse_x, mouse_y);
		blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
		allegro_gl_unset_allegro_mode();
		allegro_gl_flip();
		long end = tguiCurrentTimeMillis();
		long duration = end - start;
		if (duration < timeStep)
			rest(timeStep-duration);
		if (keypressed()) {
			int k = readkey() >> 8;
			if ((menu == MENU_MAIN) && (k == KEY_ESC))
				return false;
		}
		if (quit) {
			break;
		}
	}

	destroy_bitmap(bg);
	destroy_bitmap(buffer);
	destroy_bitmap(gameIcon);
	destroy_bitmap(gfxIcon);
	destroy_bitmap(inputIcon);
	destroy_bitmap(soundIcon);
	destroy_bitmap(gameIconHover);
	destroy_bitmap(gfxIconHover);
	destroy_bitmap(inputIconHover);
	destroy_bitmap(soundIconHover);
	destroy_bitmap(gameIconDown);
	destroy_bitmap(inputIconDown);
	destroy_bitmap(gfxIconDown);
	destroy_bitmap(soundIconDown);
	destroy_bitmap(larrow);
	destroy_bitmap(rarrow);
	destroy_bitmap(larrowd);
	destroy_bitmap(rarrowd);
	destroy_bitmap(closeBmp);
	destroy_bitmap(tabBmp);
	if (logo)
		destroy_bitmap(logo);
	destroy_bitmap(checkedBmp);
	destroy_bitmap(uncheckedBmp);

	tguiDeleteActive();

	delete mainFrame;
	delete mainText1;
	delete mainText2;
	delete mainText3;
	delete mainText4;
	delete mainIcon1;
	delete mainIcon2;
	delete mainIcon3;
	delete mainIcon4;
	delete mainButton1;
	delete mainButton2;

	resOptions.clear();
	delete gfxFrame;
	delete gfxCheckbox;
	delete gfxText;
	delete gfxText2;
	delete gfxOption;
	delete gfxButton;

	delete soundFrame;
	delete soundText1;
	delete soundText2;
	delete soundSlider1;
	delete soundSlider2;
	delete soundButton;

	gameOptions1.clear();
	gameOptions2.clear();
	gameOptions3.clear();
	delete gameFrame;
	delete gameText1;
	delete gameText2;
	delete gameText3;
	delete gameOption1;
	delete gameOption2;
	delete gameOption3;
	delete gameCheckbox;
	delete gameButton;

	inputOptions.clear();
	delete inputFrame;
	delete inputText1;
	delete inputText2;
	delete inputText3;
	delete inputText4;
	delete inputOption1;
	delete inputOption2;
	delete inputSlider1;
	delete inputSlider2;
	delete inputButton;
	
	save_config();

	return ret;
}
