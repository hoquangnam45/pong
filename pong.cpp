#include <iostream>
#include <cmath>
#include <unistd.h>

#include "Libs/Gyro/SFE_LSM9DS0.h"
#include "Libs/OLED/oled/Edison_OLED.h"
#include "Libs/OLED/gpio/gpio.h"

#define G_FORCE 9.8
#define DELTA_TIME_IN_US 1e6
#define VEL_CAP_IN_MS 100
#define BALL_RADIUS 2
#define OUTER_OFFSET 64
#define PI 3.14
#define POWER_LOSS_RATIO 1

using namespace std;

float **listPos;
float **listVel;
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

void updateVelPos(float accelX, float accelY, float accelZ);
void detectCollision();

int main(){
    initOled();
    getBallCount();

    listPos = new float* [ballCount];
    listVel = new float* [ballCount];
    for (int i = 0; i < ballCount; i++){
        listPos[i] = new float[2];
        listVel[i] = new float[2]{0};
    }
    boardBuffer = new char[oled.getLCDHeight() * oled.getLCDWidth()];

    initPos();
    while(true){
        updateBoard();
        drawingBoard();
        movingBall();
        //usleep(DELTA_TIME_IN_US * 1e-3);
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
            while (freeAreaIndex[j]) j++;
        }
        freeAreaIndex[j] = 1;
        int x = j % LCDWidthByBall * (2 * BALL_RADIUS + 1) + BALL_RADIUS,
            y = j / LCDWidthByBall * (2 * BALL_RADIUS + 1) + BALL_RADIUS;
        listPos[i][0] = x;
        listPos[i][1] = y;
        freeAreaByBall--;
    }
    delete[] freeAreaIndex;
}

void updateBoard(){
    for (int i = 0; i < oled.getLCDHeight() * oled.getLCDWidth(); i++)
        boardBuffer[i] = 0;
    for(int i = 0; i < ballCount; i++){
        int8_t x0 = listPos[i][0], y0 = listPos[i][1];
        int8_t radius = BALL_RADIUS;
        int8_t f = 1 - radius;
        int8_t ddF_x = 1;
        int8_t ddF_y = -2 * radius;
        int8_t x = 0;
        int8_t y = radius;

        for (unsigned char i=y0-radius; i<=y0+radius; i++){
            boardBuffer[flatIndex(x0, i)] = i;
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
                boardBuffer[flatIndex(x0+x, i)] = i;
                boardBuffer[flatIndex(x0-x, i)] = i;
            }
            for (unsigned char i=y0-x; i<=y0+x; i++){
                boardBuffer[flatIndex(x0+y, i)] = i;
                boardBuffer[flatIndex(x0-y, i)] = i;
            }

            boardBuffer[flatIndex(x0 + x, y0 + y)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 - x, y0 + y)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 + x, y0 - y)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 - x, y0 - y)] = i + OUTER_OFFSET;

            boardBuffer[flatIndex(x0 + y, y0 + x)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 - y, y0 + x)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 + y, y0 - x)] = i + OUTER_OFFSET;
            boardBuffer[flatIndex(x0 - y, y0 - x)] = i + OUTER_OFFSET;
        }

        boardBuffer[flatIndex(x0, y0+radius)] = i + OUTER_OFFSET;
        boardBuffer[flatIndex(x0, y0-radius)] = i + OUTER_OFFSET;
        boardBuffer[flatIndex(x0+radius, y0)] = i + OUTER_OFFSET;
        boardBuffer[flatIndex(x0-radius, y0)] = i + OUTER_OFFSET;
    }
}

void drawingBoard(){
    oled.clear(PAGE);
    for (int i = 0; i < oled.getLCDHeight() * oled.getLCDWidth(); i++){
        if (boardBuffer[i] < OUTER_OFFSET) continue;
        oled.pixel(getX(i), getY(i));
    }
    oled.display();
}

void movingBall(){
    static int initFlag = 0;
    if (!initFlag){
        initFlag = 1;
        imu = new LSM9DS0(0x6B, 0x1D);
        uint16_t imuResult = imu->begin();
        cout<<hex<<"Chip ID: 0x"<<imuResult<<dec<<" (should be 0x49d4)"<<endl;
        return;
    }
    imu->newXData();
    imu->readAccel();
    float accelX = imu->calcAccel(imu->ax) * G_FORCE,
          accelY = imu->calcAccel(imu->ay) * G_FORCE,
          accelZ = imu->calcAccel(imu->ay) * G_FORCE;
    updateVelPos(accelX, accelY, accelZ);
}

void updateVelPos(float accelX, float accelY, float accelZ){
    float total_accel = sqrt(pow(accelX,2) + pow(accelY,2) + pow(accelZ,2));
    float total_accelXY =  sqrt(pow(accelX,2) + pow(accelY,2));
    float inducedAccel = 1 - total_accel;
    float inducedAccelXY = inducedAccel / total_accel * total_accelXY;
    float inducedAccelX = inducedAccelXY / total_accelXY * accelX;
    float inducedAccelY = inducedAccelXY / total_accelXY * accelY; 
    accelX += inducedAccelX;
    accelY += inducedAccelY;
    //accelX = 10;
    //accelY = 0;
    for (int i = 0; i < ballCount; i++){
        //if (!i) cout << accelX * listPos[i][0] + 1/2 * pow(listVel[i][0], 2) << endl;
        float temp_vel_x = listVel[i][0] + accelX * DELTA_TIME_IN_US * 1e-6;
        float temp_vel_y = listVel[i][1] + accelY * DELTA_TIME_IN_US * 1e-6;
        float vel = sqrt(pow(temp_vel_x, 2) + pow(temp_vel_y, 2));
        //cout << "Vel1: " << temp_vel_x << " " << temp_vel_y << endl;
        if (vel > VEL_CAP_IN_MS){
            temp_vel_x = VEL_CAP_IN_MS * temp_vel_x / vel;
            temp_vel_y = VEL_CAP_IN_MS * temp_vel_y / vel;
        }
        // v0 * (t1 - t0) + 1/2 * (t1^2 - t0^2) = v0 * (t1 - t0) + 1/2 * (t1 - t0) * (t1 + t0)
        float temp_pos_x = listPos[i][0] - (listVel[i][0] * DELTA_TIME_IN_US * 1e-6 + 1./2 * accelX * pow(DELTA_TIME_IN_US, 2) * 1e-12);
        float temp_pos_y = listPos[i][1] + (listVel[i][1] * DELTA_TIME_IN_US * 1e-6 + 1./2 * accelY * pow(DELTA_TIME_IN_US, 2) * 1e-12);
        float boundLeft = temp_pos_x - BALL_RADIUS;
        float boundRight = temp_pos_x + BALL_RADIUS;
        float boundTop = temp_pos_y - BALL_RADIUS;
        float boundDown = temp_pos_y + BALL_RADIUS;
        if (boundLeft < 0){
            temp_vel_x = sqrt(POWER_LOSS_RATIO) * -sqrt(pow(temp_vel_x,2) - 2 * accelX * -boundLeft);
            boundLeft = 0;
            temp_pos_x = boundLeft + BALL_RADIUS;
        }
        else if (boundRight > oled.getLCDWidth() - 1){
            temp_vel_x = sqrt(POWER_LOSS_RATIO) * sqrt(pow(temp_vel_x, 2) - 2 * accelX * (oled.getLCDWidth() - 1 - boundRight));
            boundRight = oled.getLCDWidth() - 1;
            temp_pos_x = boundRight - BALL_RADIUS;
        }
        if (boundTop < 0){
            temp_vel_y = sqrt(POWER_LOSS_RATIO) * sqrt(pow(temp_vel_y, 2) - 2 * accelY * boundTop);
            boundTop = 0;
            temp_pos_y = boundTop + BALL_RADIUS;
        }
        else if (boundDown > oled.getLCDHeight() - 1){
            temp_vel_y = sqrt(POWER_LOSS_RATIO) * -sqrt(pow(temp_vel_y, 2) - 2 * accelY * -(oled.getLCDHeight() - 1 - boundDown));
            boundDown = oled.getLCDHeight() - 1;
            temp_pos_y = boundDown - BALL_RADIUS;
        }
        /*
        cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
        cout << "Accel: " << accelX << " " << accelY << endl;
        cout << "Pos0: "  << listPos[i][0] << " " << listPos[i][1] << endl;
        cout << "Vel0: " << listVel[i][0] << " " << listVel[i][1] << endl;
        cout << "Pos1: " << temp_pos_x << " " << temp_pos_y << endl;
        cout << "Vel1: " << temp_vel_x << " " << temp_vel_y << endl;
        cout << endl;*/
        listPos[i][0] = temp_pos_x;
        listPos[i][1] = temp_pos_y;
        listVel[i][0] = temp_vel_x;
        listVel[i][1] = temp_vel_y;
    }
}
    