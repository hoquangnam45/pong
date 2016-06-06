#include <iostream>
#include <cmath>
#include <unistd.h>

#include "Libs/Gyro/SFE_LSM9DS0.h"
#include "Libs/OLED/oled/Edison_OLED.h"
#include "Libs/OLED/gpio/gpio.h"

#define G_FORCE 9.8
#define DELTA_TIME_IN_MS 10
#define BALL_RADIUS 5
#define OUTER_OFFSET 64
#define PI 3.14

using namespace std;

int **listPos, **listVel;
char *boardBuffer;
int ballCount;
int LCDWidthByBall, LCDHeightByBall, LCDAreaByBall;

LSM9DS0 *imu;
// Define an edOLED object:
edOLED oled;

// Pin definitions:
// All buttons have pull-up resistors on-board, so just declare
// them as regular INPUT's
gpio BUTTON_UP(47, INPUT);
gpio BUTTON_DOWN(44, INPUT);
gpio BUTTON_LEFT(165, INPUT);
gpio BUTTON_RIGHT(45, INPUT);
gpio BUTTON_SELECT(48, INPUT);
gpio BUTTON_A(49, INPUT);
gpio BUTTON_B(46, INPUT);

int flatIndex(int x, int y);
int getX(int flatIndex);
int getY(int flatIndex);
void initOled();
void initPos();
void getBallCount();
void drawingBoard();
void movingBall();
void updateBoard();

int main(){
    initOled();
    getBallCount();

    listPos = new int* [ballCount];
    listVel = new int* [ballCount];
    for (int i = 0; i < ballCount; i++){
        listPos[i] = new int[2];
        listVel[i] = new int[2]{0};
    }
    boardBuffer = new char[oled.getLCDHeight() * oled.getLCDWidth()];

    initPos();
    while(true){
        updateBoard();
        drawingBoard();
        // movingBall();
        usleep(15);
    }
    return 0;
}

void initOled(){
    oled.begin();
    oled.clear(PAGE);
    oled.display();
    oled.setFontType(0);
}

int flatIndex(int x, int y){
    return x + y * oled.getLCDWidth();
}

void getBallCount(){
    oled.clear(PAGE);
    oled.setCursor(5, 48/2 - 1 - 4);
    oled.print("Xin chon");
    oled.setCursor(5, 48/2 - 1 + 4);
    oled.print("So banh");
    oled.display();
    usleep(1e6);
    LCDHeightByBall = oled.getLCDHeight() / (2*BALL_RADIUS + 1); 
    LCDWidthByBall = oled.getLCDWidth() / (2*BALL_RADIUS + 1);
    LCDAreaByBall = LCDHeightByBall * LCDWidthByBall;

    bool agreePressed = false, pressFlag = false;
    while(!agreePressed){
        if (ballCount > 0 && BUTTON_DOWN.pinRead() == LOW && !pressFlag){
            ballCount--;
            pressFlag = 1;
        }
        else if (ballCount < LCDAreaByBall && BUTTON_UP.pinRead() == LOW && !pressFlag){
            ballCount++;
            pressFlag = 1;
        }
        else if(BUTTON_DOWN.pinRead() == HIGH && BUTTON_UP.pinRead() == HIGH){
            pressFlag = 0;
        }
        if (ballCount && BUTTON_SELECT.pinRead() == LOW){
            agreePressed = true;
        }
        oled.clear(PAGE);
        oled.setCursor(0, 48/2 - 1);
        oled.print(ballCount); oled.print(" banh");
        oled.display();
    }
}
int getX(int flatIndex){
    return flatIndex % oled.getLCDWidth();
}
int getY(int flatIndex){
    return flatIndex / oled.getLCDWidth();
}
void initPos(){
    srand(time(NULL));
    int freeAreaByBall = LCDAreaByBall;
    int *freeAreaIndex = new int [freeAreaByBall]{0};
    for (int i = 0; i < ballCount && freeAreaByBall; i++){
        int pos = rand() % freeAreaByBall + 1;
        int j = -1;
        while (pos){
            j++;
            pos--;
            if (freeAreaByBall[j]) j++;
        }
        freeAreaIndex[j] = 1;
        int x = j / LCDWidthByBall * BALL_RADIUS,
            y = j % LCDWidthByBall * BALL_RADIUS;
        listPos[i][0] = x;
        listPos[i][1] = y;
        freeAreaByBall--;
    }
    delete[] freeAreaIndex;
}

void updateBoard(){
    for(int i = 0; i < ballCount; i++){
        int8_t x0 = listPos[i][0], y0 = listPos[i][1];
        int8_t radius = BALL_RADIUS;
        int8_t f = 1 - radius;
        int8_t ddF_x = 1;
        int8_t ddF_y = -2 * radius;
        int8_t x = 0;
        int8_t y = radius;

        for (unsigned char i=y0-radius; i<=y0+radius; i++){
            pixel(x0, i, color, mode);
        }

        while (x<y){
            if (f >= 0){
                y--;
                ddF_y += 2;
                f += ddF_y;
            }
            x++;
            ddF_x += 2;
            f += ddF_x;

            for (unsigned char i=y0-y; i<=y0+y; i++){
                pixel(x0+x, i, color, mode);
                pixel(x0-x, i, color, mode);
            }
            for (unsigned char i=y0-x; i<=y0+x; i++){
                pixel(x0+y, i, color, mode);
                pixel(x0-y, i, color, mode);
            }

            pixel(x0 + x, y0 + y, color, mode);
            pixel(x0 - x, y0 + y, color, mode);
            pixel(x0 + x, y0 - y, color, mode);
            pixel(x0 - x, y0 - y, color, mode);

            pixel(x0 + y, y0 + x, color, mode);
            pixel(x0 - y, y0 + x, color, mode);
            pixel(x0 + y, y0 - x, color, mode);
            pixel(x0 - y, y0 - x, color, mode);
        }

        pixel(x0, y0+radius, color, mode);
        pixel(x0, y0-radius, color, mode);
        pixel(x0+radius, y0, color, mode);
        pixel(x0-radius, y0, color, mode);
    }
}

void drawingBoard(){
    for (int i = 0; i < oled.getLCDHeight() * oled.getLCDWidth(); i++){
        if (boardBuffer[i] < OUTER_OFFSET) continue;
        oled.pixel(getX(i), getY(i));
    }
}