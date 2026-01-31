#include <stdio.h>
#include <graphics.h>
#include "tools.h"
#include <mmsystem.h>
#include <conio.h>
#include <vector>
#include <time.h>

#pragma comment(lib,"winmm.lib")

using namespace std; 

#define WIN_SCORE 66 
#define WIN_WIDTH 1012 
#define WIN_HEIGHT 396 
#define OBSTACLE_COUNT 10 


typedef enum {
	TORTOISE,
	LION,	  
	HOOK1,
	HOOK2,
	HOOK3,
	HOOK4,
	OBSTACLE_TYPE_COUNT  
} obstacle_type;


vector<vector<IMAGE>> obstacleImgs;



typedef struct  obstacle{
	int type; 
	int imgindex;
	int x, y; 
	int speed; 
	int power; 
	bool exist; 
	bool hited; 
	bool passed;   
}obstacle_t;

IMAGE imBgs[3]; 
int bgX[3]; 
int bgSpeed[3] = { 1 , 2, 4 };
IMAGE imgHeros[12];

int heroX; 
int heroY;
int heroIndex; 

bool heroJump; 
int jumpHeightMax; 
int heroJumpOff; 
int update; 
bool heroDown; 
int heroBlood; 

int score; 


obstacle_t obstacles[OBSTACLE_COUNT];

int lastObsIndex; 

IMAGE imgHeroDown[2];

IMAGE imgSZ[10];



void init() {
	initgraph(WIN_WIDTH,WIN_HEIGHT);
	
	char name[64];
	for (int i = 0; i < 3; i++) {
		sprintf_s(name, "res/bg%03d.png",i + 1);
		loadimage(&imBgs[i],name);	
		bgX[i] = 0;
	}
	for (int i = 0; i < 12; i++) {
		sprintf_s(name, "res/hero%d.png", i + 1);
		loadimage(&imgHeros[i], name);
	}
	heroX = WIN_WIDTH * 0.5 - imgHeros[0].getwidth() * 0.5; 
	heroY = 345 - imgHeros[0].getheight();
	heroIndex = 0;
	heroJump = false;
	jumpHeightMax = 345 - imgHeros[0].getheight() - 120;
	heroJumpOff = -6;

	update = true;

	heroBlood = 100;
	IMAGE imgTort;
	loadimage(&imgTort, "res/t1.png");
	vector<IMAGE> imgTortArray;
	imgTortArray.push_back(imgTort); 
	obstacleImgs.push_back(imgTortArray);  

	IMAGE imgLion;
	vector<IMAGE> imgLionArray;
	for (int i = 0; i < 6; i++) {
		sprintf_s(name, "res/p%d.png", i + 1);
		loadimage(&imgLion, name);
		imgLionArray.push_back(imgLion);
	}
	obstacleImgs.push_back(imgLionArray);

	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		obstacles[i].exist = false;

	}

	loadimage(&imgHeroDown[0], "res/d1.png");
	loadimage(&imgHeroDown[1], "res/d2.png");
	heroDown = false;

	IMAGE imgHook;
	
	for (int i = 0; i < 4; i++) {
		vector<IMAGE> imgHookArray;
		sprintf_s(name, "res/h%d.png", i + 1);
		loadimage(&imgHook, name,63,260,true);
		imgHookArray.push_back(imgHook);
		obstacleImgs.push_back(imgHookArray);
	}

	preLoadSound("res/hit.mp3");
	mciSendString("play res/bg.mp3 repeat",0,0,0);

	lastObsIndex = -1;
	score = 0;

	for (int i = 0; i < 10; i++) {
		sprintf(name, "res/sz/%d.png", i);
		loadimage(&imgSZ[i],name);
	}


}


void updateBg() {

	putimagePNG2(bgX[0], 0, &imBgs[0]);
	putimagePNG2(bgX[1], 119, &imBgs[1]);
	putimagePNG2(bgX[2], 330, &imBgs[2]);
}


void creatObstacle() {
	int i;
	for (i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist == false){
			break;
		}
	}
	if (i >= OBSTACLE_COUNT) {
		return;
	}

	obstacles[i].exist = true;
	obstacles[i].hited = false;
	obstacles[i].imgindex = 0;

	obstacles[i].type = (obstacle_type)(rand() % 3);
	if (lastObsIndex >= 0 &&
			obstacles[lastObsIndex].type >= HOOK1 &&
			obstacles[lastObsIndex].type <= HOOK4 &&
			obstacles[i].type == LION &&
			obstacles[lastObsIndex].x > (WIN_WIDTH - 500)) {
		obstacles[i].type = TORTOISE;
	}
	lastObsIndex = i;
	if (obstacles[i].type == HOOK1) {
		obstacles[i].type += rand() % 4;
	}
	obstacles[i].x = WIN_WIDTH;
	obstacles[i].y = 345 + 5 - obstacleImgs[obstacles[i].type][0].getheight();
	if (obstacles[i].type == TORTOISE) {
		obstacles[i].speed = 0;
		obstacles[i].power = 5;
	}
	else if (obstacles[i].type == LION){
		obstacles[i].speed = 4;
		obstacles[i].power = 5;
	}
	else if(obstacles[i].type >= HOOK1 && obstacles[i].type <= HOOK4){
		obstacles[i].speed = 0;
		obstacles[i].power = 5;
		obstacles[i].y = 0;
	}
	obstacles[i].passed = false;
}


void checkHit() {
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist && obstacles[i].hited == false) {
			int a1x, a1y, a2x, a2y;
			int off = 30;
			if (!heroDown) { 
				a1x = heroX + off;
				a1y = heroY + off;
				a2x = heroX + imgHeros[heroIndex].getwidth() - off;
				a2y = heroY + imgHeros[heroIndex].getheight();
			}else{
				a1x = heroX + off;
				a1y = 345 - imgHeroDown[heroIndex].getheight();
				a2x = heroX + imgHeroDown[heroIndex].getwidth() - off;
				a2y = 345;
			}
			IMAGE img = obstacleImgs[obstacles[i].type][obstacles[i].imgindex];
			int b1x = obstacles[i].x + off;
			int b1y = obstacles[i].y + off;
			int b2x = obstacles[i].x + img.getwidth() - off;
			int b2y = obstacles[i].y + img.getheight() - 10;   
			if (rectIntersect(a1x,a1y,a2x,a2y,b1x,b1y,b2x,b2y)) {
				heroBlood -= obstacles[i].power;
				playSound("res/hit.mp3");
				obstacles[i].hited = true;
			}
		}
	}
}



void flyBg() {
	for (int i = 0; i < 3; i++) {
		bgX[i] -= bgSpeed[i];
		if (bgX[i] < -WIN_WIDTH) {
			bgX[i] = 0;
		}
	}
	if (heroJump) {
		if (heroY < jumpHeightMax) {
			heroJumpOff = 6;
		}
		heroY += heroJumpOff;
		if (heroY > 345 - imgHeros[0].getheight()) {
			heroJump = false;
			heroJumpOff = -6;
		}
	}
	else if (heroDown) { 
		static int count = 0;
		int delays[2] = { 8, 30 };
		count++;
		if (count >= delays[heroIndex]){
			count = 0;
			heroIndex++;
			if (heroIndex >= 2) {
				heroIndex = 0;
				heroDown = false;
			}
		}
	}else{
		heroIndex = (heroIndex + 1) % 12;
	}

	static int frameCount = 0;
	static int enemyFre = 50;
	frameCount++;
	if (frameCount > enemyFre) {
		frameCount = 0;
		enemyFre = 50 + rand() % 50;
		creatObstacle();
	}

	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist) {
			obstacles[i].x -= obstacles[i].speed + bgSpeed[2];
			if (obstacles[i].x < -obstacleImgs[obstacles[i].type][0].getwidth() * 2) {
				obstacles[i].exist = false;
			}
			
			int len = obstacleImgs[obstacles[i].type].size();
			obstacles[i].imgindex = (obstacles[i].imgindex + 1) % len;
		}
	}


	checkHit();

}

void jump() {
	heroJump = true;
	update = true;
}

void down() {
	heroDown = true;
	update = true;
	heroIndex = 0;
}


void keyEvent() {
	char ch;
	if (_kbhit()) { 
		ch = _getch();  
		if (ch == 'w') {
			jump();
		}
		else if(ch == 's') {
			down();
		}
	}

}

void updateEnemy() {

	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist) {
			putimagePNG2(obstacles[i].x, obstacles[i].y, WIN_WIDTH,
				&obstacleImgs[obstacles[i].type][obstacles[i].imgindex]);
		}
	}
}

void updateHero() {
	if (!heroDown) {
		putimagePNG2(heroX, heroY, &imgHeros[heroIndex]);
	}
	else {
		int y = 345 - imgHeroDown[heroIndex].getheight();
		putimagePNG2(heroX,y,&imgHeroDown[heroIndex]);
	}
	
}

void updateBloodBar() {
	drawBloodBar(10, 10, 200, 10, 2, BLUE, DARKGRAY, RED,heroBlood / 100.0 );
}


void checkOver(){
	if (heroBlood <= 0) {
		loadimage(0,"res/over.png");
		FlushBatchDraw();
		mciSendString("stop res/bg.mp3",0,0,0);
		system("pause");
		heroBlood = 100;
		score = 0;
		mciSendString("play res/bg.mp3 repeat", 0, 0, 0);
	}

}


void checkScore() {
	for (int i = 0; i < OBSTACLE_COUNT; i++) {
		if (obstacles[i].exist &&
				obstacles[i].passed == false &&
				obstacles[i].hited == false &&
				obstacles[i].x + obstacleImgs[obstacles[i].type][0].getwidth() < heroX) {
			score++;
			obstacles[i].passed = true;
		 printf("score:%d\n",score);
		}
	}
}


void updateScore() {
	char str[8];
	sprintf(str, "%d", score);
	int x = 20;
	int y = 25;
	for (int i = 0; str[i]; i++) {
		int sz = str[i] - '0';
		putimagePNG(x, y, &imgSZ[sz]);
		x += imgSZ[sz].getwidth() + 5;
	}
}


void checkWin() {
	if (score >= WIN_SCORE) {
		mciSendString("play res/win.mp3",0,0,0);
		Sleep(2000);
		loadimage(0, "res/win.png");
		FlushBatchDraw();
		mciSendString("stop res/win.mp3",0,0,0);
		system("pause");
		heroBlood = 100;
		score = 0;
		mciSendString("play res/bg.mp3 repeat", 0, 0, 0);
	}
}




int main(void) {
	init();
	loadimage(0, "res/over.png");
	system("pause");

	int timer = 0;
	while (true)
	{
		keyEvent();
		timer += getDelay();
		if (timer > 15) {
			timer = 0;
			update = true;
			}
		if (update) {
			update = false;
			BeginBatchDraw();
			updateBg();
			updateHero();
			updateEnemy();
			
			updateBloodBar();
			updateScore();		
			EndBatchDraw();
			checkWin();
			checkOver();
			checkScore();		
			flyBg();
		}
	}
	

	system("pause");
	return 0;
}