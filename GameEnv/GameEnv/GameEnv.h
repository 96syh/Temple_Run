#ifndef GAME_ENV_H
#define GAME_ENV_H

#include <vector>
#include <graphics.h>

// --- �ṹ�嶨�� ---

// ÿһ���������ظ� Python �Ľ���ṹ
struct StepResult {
    std::vector<float> observation;
    float reward;
    bool done;
    int score;
};

// �ϰ������Խṹ
struct Obstacle {
    int type;      // 0: �ڹ�, 1: ʨ��, 2-5: �ҹ�
    int imgindex;  // ��ǰ����֡����
    int x, y;
    int speed;
    int power;     // ��ײ���� (��Ѫ��)
    bool exist;
    bool hited;    // �Ƿ��ѷ�����ײ (��ֹ�����ظ���Ѫ)
    bool passed;   // �Ƿ��ѱ��ɹ�ͨ�� (���ڼƷ�)
};

// --- �����ඨ�� ---

class GameEnv {
public:
    // ����������
    GameEnv(bool headless = true);
    ~GameEnv();

    // Python ���õ���Ҫ�ӿ�
    void reset();               // ���û���
    StepResult step(int action); // ִ��һ������
    std::vector<float> get_obs(); // ��ȡ��ǰ�۲�״̬

    // ���������������ӿ� (�� Python �� print)
    float get_reward_pass();
    float get_reward_death();
    int get_damage_taken();
    float get_reward_hit();
    float get_reward_step();

private:
    // �ڲ��������߼�����
    void update_physics();  // ���������붯������
    void create_obstacle(); // ��������ϰ���
    void check_hit();       // ��ײ�����Ѫ������
    void render();          // EasyX ͼ����Ⱦ (����ͷģʽ)

    // ������������
    bool is_headless;
    int heroX, heroY;
    int heroIndex;          // Ӣ�۵�ǰ����֡����
    int heroBlood;          // Ӣ�۵�ǰѪ��
    int score;              // ��Ϸ�÷�

    // Ӣ��״̬����
    bool heroJump;
    bool heroDown;
    int heroJumpOff;        // ��Ծƫ����
    int jumpHeightMax;      // �����Ծ�߶�
    int frameCount;         // �ϰ������ɵ�֡������
    int enemyFre;           // �ϰ�������Ƶ��

    // ��Դ���� (�ϸ�ƥ�� main.cpp)
    IMAGE imBgs[3];         // Զ���С������㱳��
    int bgX[3];             // ���㱳���ĺ�����
    int bgSpeed[3];         // ���㱳���Ĺ����ٶ�
    IMAGE imgHeros[12];     // Ӣ���ܲ���������
    IMAGE imgHeroDown[2];   // Ӣ���¶׶�������
    IMAGE imgSZ[10];        // 0-9 ���ַ���ֵ��ͼ

    // �ϰ�����ͼ���� (Ƕ�������洢����֡)
    std::vector<std::vector<IMAGE>> obstacleImgs;
    Obstacle obstacles[10]; // ������ͬʱ���ڵ��ϰ�������

    // ������ײ�ж���������
    bool rectIntersect(int a1x, int a1y, int a2x, int a2y, int b1x, int b1y, int b2x, int b2y) {
        return (a1x < b2x && a2x > b1x && a1y < b2y && a2y > b1y);
    }
};

#endif