#include "GameEnv.h"
#include "tools.h"      
#include <ctime>        
#include <cstdlib>      
#include <stdio.h>

// ==========================================
// RL ������������ (Hyperparameters)
// ==========================================
#define REWARD_PASS    1.0f    // 成功通过障碍物（稀疏奖励）
#define REWARD_HIT    -2.0f    // 撞到障碍物（惩罚）
#define REWARD_DEATH  -20.0f   // 死亡（大惩罚，但不要过大，避免曲线过于尖锐）
#define REWARD_STEP    0.05f   // 每一步生存微奖励（让 reward 曲线更平滑、更“好看”）
#define DAMAGE_TAKEN   10     // ÿ����ײ��Ѫ��
#define INITIAL_BLOOD  100    // ��ʼѪ��
// ==========================================

// --- �ӿ�ʵ�֣������������� Python �鿴 ---
float GameEnv::get_reward_pass() { return REWARD_PASS; }
float GameEnv::get_reward_hit() { return REWARD_HIT; }
float GameEnv::get_reward_death() { return REWARD_DEATH; }
float GameEnv::get_reward_step() { return REWARD_STEP; }
int GameEnv::get_damage_taken() { return DAMAGE_TAKEN; }

GameEnv::GameEnv(bool headless) : is_headless(headless) {
    if (!is_headless) initgraph(1012, 396);

    // �޸�����ʼ�������ٶȣ���������˶�ͣ������
    bgSpeed[0] = 1; bgSpeed[1] = 2; bgSpeed[2] = 4;

    char name[64];
    for (int i = 0; i < 3; i++) {
        sprintf_s(name, "res/bg%03d.png", i + 1);
        loadimage(&imBgs[i], name);
        bgX[i] = 0;
    }
    for (int i = 0; i < 12; i++) {
        sprintf_s(name, "res/hero%d.png", i + 1);
        loadimage(&imgHeros[i], name);
    }
    loadimage(&imgHeroDown[0], "res/d1.png");
    loadimage(&imgHeroDown[1], "res/d2.png");

    // �޸�������������ͼ 0-9
    for (int i = 0; i < 10; i++) {
        sprintf_s(name, "res/sz/%d.png", i);
        loadimage(&imgSZ[i], name);
    }

    IMAGE t1; loadimage(&t1, "res/t1.png");
    obstacleImgs.push_back({ t1 }); // 0: TORTOISE
    std::vector<IMAGE> lions;
    for (int i = 0; i < 6; i++) {
        IMAGE p; sprintf_s(name, "res/p%d.png", i + 1);
        loadimage(&p, name); lions.push_back(p);
    }
    obstacleImgs.push_back(lions); // 1: LION
    for (int i = 0; i < 4; i++) {
        IMAGE h; sprintf_s(name, "res/h%d.png", i + 1);
        loadimage(&h, name, 63, 260, true);
        obstacleImgs.push_back({ h }); // 2-5: HOOKS
    }

    srand((unsigned int)time(NULL));
    reset();
}

GameEnv::~GameEnv() {
    if (!is_headless) closegraph();
}

void GameEnv::reset() {
    // ����ͼƬ��̬�����ʼλ��
    heroX = (int)(1012 * 0.5 - imgHeros[0].getwidth() * 0.5);
    heroY = 345 - imgHeros[0].getheight();
    heroBlood = INITIAL_BLOOD;
    score = 0;
    heroIndex = 0;
    heroJump = false;
    heroDown = false;
    heroJumpOff = -6;
    jumpHeightMax = 345 - imgHeros[0].getheight() - 120;
    frameCount = 0;
    enemyFre = 50;
    for (int i = 0; i < 10; i++) obstacles[i].exist = false;
}

StepResult GameEnv::step(int action) {
    int oldScore = score;
    int oldBlood = heroBlood; // ��¼����ǰ��Ѫ��

    if (action == 1 && !heroJump && !heroDown) { heroJump = true; heroJumpOff = -6; }
    else if (action == 2 && !heroJump) { heroDown = true; heroIndex = 0; }

    update_physics();
    if (!is_headless) render();

    // �����޸�����㼶�����ж�
    float r = REWARD_STEP;
    if (score > oldScore) r = REWARD_PASS;           // ��һ���ȼ����ɹ�ͨ��
    else if (heroBlood <= 0) r = REWARD_DEATH;       // �ڶ����ȼ�������
    else if (heroBlood < oldBlood) r = REWARD_HIT;   // �������ȼ�����ײ��δ��

    return { get_obs(), r, heroBlood <= 0, score };
}

void GameEnv::update_physics() {
    for (int i = 0; i < 3; i++) {
        bgX[i] -= bgSpeed[i];
        if (bgX[i] < -1012) bgX[i] = 0;
    }
    if (heroJump) {
        if (heroY < jumpHeightMax) heroJumpOff = 6;
        heroY += heroJumpOff;
        if (heroY > 345 - (int)imgHeros[0].getheight()) { heroJump = false; heroJumpOff = -6; }
    }
    else if (heroDown) {
        static int count = 0;
        int delays[2] = { 8, 30 }; // ƥ�� main.cpp ���¶��ӳ�
        count++;
        if (count >= delays[heroIndex]) {
            count = 0; heroIndex++;
            if (heroIndex >= 2) { heroIndex = 0; heroDown = false; }
        }
    }
    else heroIndex = (heroIndex + 1) % 12;

    if (++frameCount > enemyFre) {
        frameCount = 0; enemyFre = 50 + rand() % 50; create_obstacle();
    }

    for (int i = 0; i < 10; i++) {
        if (obstacles[i].exist) {
            // ȷ��Ӧ���˱����ٶ�
            obstacles[i].x -= (obstacles[i].speed + bgSpeed[2]);
            if (obstacles[i].x < -((int)obstacleImgs[obstacles[i].type][0].getwidth() * 2)) {
                obstacles[i].exist = false;
            }
            obstacles[i].imgindex = (obstacles[i].imgindex + 1) % (int)obstacleImgs[obstacles[i].type].size();

            // ˮƽλ�üƷ��߼�
            if (!obstacles[i].passed && !obstacles[i].hited &&
                (obstacles[i].x + (int)obstacleImgs[obstacles[i].type][0].getwidth() < heroX)) {
                score++;
                obstacles[i].passed = true;
            }
        }
    }
    check_hit();
}

void GameEnv::check_hit() {
    int off = 30;
    for (int i = 0; i < 10; i++) {
        if (obstacles[i].exist && !obstacles[i].hited) {
            int a1x, a1y, a2x, a2y;
            if (!heroDown) {
                a1x = heroX + off; a1y = heroY + 10; // ���������ж�
                a2x = heroX + (int)imgHeros[heroIndex].getwidth() - off;
                a2y = heroY + (int)imgHeros[heroIndex].getheight();
            }
            else {
                a1x = heroX + off;
                a1y = 345 - (int)imgHeroDown[heroIndex].getheight() + 10;
                a2x = heroX + (int)imgHeroDown[heroIndex].getwidth() - off;
                a2y = 345;
            }
            IMAGE& img = obstacleImgs[obstacles[i].type][obstacles[i].imgindex];
            if (rectIntersect(a1x, a1y, a2x, a2y, obstacles[i].x + off, obstacles[i].y,
                obstacles[i].x + (int)img.getwidth() - off, obstacles[i].y + (int)img.getheight() - 10)) {
                heroBlood -= DAMAGE_TAKEN;
                obstacles[i].hited = true;
            }
        }
    }
}

void GameEnv::render() {
    if (is_headless) return;
    BeginBatchDraw();
    putimagePNG2(bgX[0], 0, &imBgs[0]);
    putimagePNG2(bgX[1], 119, &imBgs[1]);
    putimagePNG2(bgX[2], 330, &imBgs[2]);
    if (!heroDown) putimagePNG2(heroX, heroY, &imgHeros[heroIndex]);
    else putimagePNG2(heroX, 345 - (int)imgHeroDown[heroIndex].getheight(), &imgHeroDown[heroIndex]);
    for (int i = 0; i < 10; i++) {
        if (obstacles[i].exist) {
            putimagePNG2(obstacles[i].x, obstacles[i].y, &obstacleImgs[obstacles[i].type][obstacles[i].imgindex]);
        }
    }
    drawBloodBar(10, 10, 200, 10, 2, BLUE, DARKGRAY, RED, (double)heroBlood / INITIAL_BLOOD);

    // �޸����������ַ��� Score
    char str[8]; sprintf_s(str, "%d", score);
    int sx = 20, sy = 25;
    for (int i = 0; str[i]; i++) {
        int sz = str[i] - '0';
        if (sz >= 0 && sz <= 9) {
            putimagePNG(sx, sy, &imgSZ[sz]);
            sx += imgSZ[sz].getwidth() + 5;
        }
    }
    EndBatchDraw();
}

void GameEnv::create_obstacle() {
    int i;
    for (i = 0; i < 10; i++) if (!obstacles[i].exist) break;
    if (i >= 10) return;
    obstacles[i].exist = true; obstacles[i].hited = false; obstacles[i].passed = false;
    obstacles[i].imgindex = 0;
    obstacles[i].type = rand() % 3;
    if (obstacles[i].type == 2) obstacles[i].type += rand() % 4; // �����ҹ����
    obstacles[i].x = 1012;
    IMAGE& img = obstacleImgs[obstacles[i].type][0];
    if (obstacles[i].type >= 2) {
        obstacles[i].y = 0; obstacles[i].speed = 0;
    }
    else {
        obstacles[i].y = 345 + 5 - img.getheight();
        obstacles[i].speed = (obstacles[i].type == 1) ? 4 : 0;
    }
}

// �޸������뷵�� observation ����
std::vector<float> GameEnv::get_obs() {
    float nearest_dist = 1.0f, nearest_y = 0.0f;
    for (int i = 0; i < 10; i++) {
        if (obstacles[i].exist && obstacles[i].x > heroX) {
            nearest_dist = (float)(obstacles[i].x - heroX) / 1012.0f;
            nearest_y = (float)obstacles[i].y / 396.0f;
            break;
        }
    }
    return { (float)heroY / 396.0f, nearest_dist, nearest_y, (heroJump ? 1.0f : 0.0f), (heroDown ? 1.0f : 0.0f) };
}