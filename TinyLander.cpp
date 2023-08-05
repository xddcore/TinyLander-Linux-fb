//   >>>>>  T-I-N-Y  L-A-N-D-E-R v1.0 for ATTINY85  GPLv3 <<<<
//              Programmer: (c) Roger Buehler 2020
//              Contact EMAIL: tscha70@gmail.com
//        Official repository:  https://github.com/tscha70/
//  Tiny Lander v1.0 is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//     
//  This game uses features or part of code created by 
//  Daniel C (Electro L.I.B) https://www.tinyjoypad.com under GPLv3
//  to work with tinyjoypad game console's standard.
//             
// 2023/08/03:xddcore 1034029664@qq.com
// 修改移植以支持Linux Frame Buffer
//

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <stdio.h>
#include <math.h>

#include "spritebank.h"
#include "gameinterface.h"

extern int key_board_fb;
/*framebuffer*/
int fbfd;
char *fbp=NULL;
int screensize;

void Tiny_Flip(uint8_t mode, GAME * game, DIGITAL * score, DIGITAL * velX, DIGITAL * velY);
void fillData(long myValue, DIGITAL * data);
uint8_t getLanderSprite(uint8_t x, uint8_t y, GAME * game);
void Frame_Buffer_Clear();//FB清屏(切换界面后)

void delay(unsigned int us)
{
    usleep(us * 1000);
}


void initGame (GAME * game)
{
  SETNEXTLEVEL(game->Level, game);

  game->velocityY = 0;
  game->velocityX = 0;
  game->velXCounter = 0;
  game->velYCounter = 0;
  game->ShipExplode = 0;
  game->Toggle = true;
  game->Collision = false;
  game->HasLanded = false;
  game->EndCounter = 0;
  game->Stars = 0;
}

void showAllScoresAndBonuses(GAME *game, DIGITAL *score, DIGITAL *velX, DIGITAL *velY)
{
  //VICTORYSOUND();
  game->Level++;
  delay (1000);
  uint8_t bonusPoints = 0;

  // add bonus points
  if ((abs(game->velocityY)) <= BONUSSPEED2)
    bonusPoints++;
  if ((abs(game->velocityY)) <= BONUSSPEED1)
    bonusPoints++;
  if (game->Fuel >= game->FuelBonus)
    bonusPoints++;

  for (game->Stars = 1; game->Stars <= bonusPoints; game->Stars++)
  {
    Tiny_Flip(2, game, score, velX, velY);
    //HAPPYSOUND();
    delay(500);
  }
  game->Stars--;

  uint16_t newScore = game->Score + game->LevelScore  + (game->LevelScore * bonusPoints );
  while (game->Score < newScore)
  {
    game->Score++;
    fillData(game->Score, score);
    Tiny_Flip(2, game, score, velX, velY);
    //SOUND(129, 2);
  }
}

void changeSpeed(GAME * game)
{
  game->ThrustLEFT = JOYPAD_LEFT;
  game->ThrustRIGHT = JOYPAD_RIGHT;
  game->ThrustUP = JOYPAD_FIRE;
  game->Toggle = !game->Toggle;

  if (game->ThrustLEFT && game->Fuel > 0)
  {
    game->Fuel -= (FULLTHRUST / 2);
    game->velocityX += TrustX;
    if ((game->velocityX) > VLimit)
      game->velocityX  = VLimit;
  }
  else if (game->ThrustRIGHT && game->Fuel > 0)
  {
    game->Fuel -= (FULLTHRUST / 2);
    game->velocityX -= TrustX;
    if ((game->velocityX) < -VLimit)
      game->velocityX  = -VLimit;
  }

  if (game->ThrustUP && game->Fuel > 0)
  {
    game->Fuel -= (FULLTHRUST * 2);
    game->velocityY += TrustY;
    if ((game->velocityY) > VLimit)
      game->velocityY  = VLimit;
  }
  else
  {
    game->velocityY -= (GRAVITYDECY);
    if ((game->velocityY) < -VLimit)
      game->velocityY  = -VLimit;
  }

  if ((game->Fuel) <= 0)
    game->Fuel = 0;
}

void moveShip(GAME * game)
{
  if (game->ShipExplode > 0 || game->Collision || game->HasLanded) return;

  game->velXCounter += abs(game->velocityX);
  game->velYCounter += abs(game->velocityY);

  if ((game->velXCounter) >= MoveX) {
    game->velXCounter = 0;
    if ((game->velocityX) > 0)
      game->ShipPosX += 1;
    if ((game->velocityX) < 0)
      game->ShipPosX -= 1;
  }

  if (game->velYCounter >= MoveY) {
    uint8_t inc = (abs(game->velocityY) / ACCELERATOR) + 1;
    (game->velYCounter) = 0;
    if ((game->velocityY) > 0)
      game->ShipPosY -= inc;
    if (game->velocityY < 0)
      game->ShipPosY += inc;
  }

  // boundaries....
  if (game->ShipPosX > 121)
  {
    game->ShipPosX = 121;
  }
  else if (game->ShipPosX < 23)
  {
    game->ShipPosX = 23;
  }
  if (game->ShipPosY > 55)
  {
    game->ShipPosY = 55;
  }
}

void fillData(long myValue, DIGITAL * data)
{
  SPLITDIGITS(abs(myValue), data->D);
  data->IsNegative = (myValue < 0);
}

uint8_t ScoreDisplay(uint8_t x, uint8_t y, DIGITAL * score) {
  // show score within the give limits on lin 1
  if  ((y != 1) || (x < SCOREOFFSET) || (x > (SCOREOFFSET + (SCOREDIGITS * DIGITSIZE) - 1))) {
    return 0;
  }
  // show all of the file digits
  uint8_t part =  (x - SCOREOFFSET) / (DIGITSIZE);
  return DIGITS[x - SCOREOFFSET - (DIGITSIZE * part) + (score->D[(SCOREDIGITS - 1) - part] * DIGITSIZE)];
}

uint8_t VelocityDisplay(uint8_t x, uint8_t y, DIGITAL * velocity, uint8_t horizontal)
{
  // if on line 4 or 5  for horizontal(4) an vertical(4) speed
  if ((horizontal == 1 && y != 4) || (horizontal == 0 && y != 5)) {
    return 0;
  }
  // display velocity within the limits ...
  if ((x < VELOOFFSET) || (x > (VELOOFFSET + (VELODIGITS * DIGITSIZE)) - 1)) {
    return 0;
  }
  // show plus or minus sign
  if ((x >= VELOOFFSET) && (x < (VELOOFFSET + DIGITSIZE))) {
    return DIGITS[x - VELOOFFSET + ((10 + (velocity->IsNegative)) * DIGITSIZE)];
  }
  // show just 3 digits
  uint8_t part =  ((x - VELOOFFSET) / (DIGITSIZE));
  return DIGITS[x - VELOOFFSET - (DIGITSIZE * part) + (velocity->D[(VELODIGITS - 1) - part] * DIGITSIZE)];
}
uint8_t DashboardDisplay(uint8_t x, uint8_t y, GAME * game)
{
  if (x >= 0 && x <= 22) {
    return DASHBOARD[x + y * 23];
  }
  return 0x00;
}

uint8_t LanderDisplay(uint8_t x, uint8_t y, GAME * game) {
  uint8_t line = game->ShipPosY / 8;
  uint8_t offset = game->ShipPosY % 8;
  if (y == line || ((y == line + 1) && offset > 0))
  {
    if (((x - game->ShipPosX) >= 0) && ((x - game->ShipPosX) < 7)) {
      uint8_t sprite = getLanderSprite (x, y, game);
      if (offset == 0 && y == line)
        return sprite;
      if (offset > 0 && y == line)
        return sprite << offset;
      if (offset > 0 && y == (line + 1))
        return sprite >> (8 - offset);
    }
  }
  return 0x00;
}

uint8_t getLanderSprite(uint8_t x, uint8_t y, GAME * game)
{
  uint8_t sprite = 0x00;

  if (game->ShipExplode > 0)
  {
    sprite = LANDER[(x - game->ShipPosX) + ((8 - (game->ShipExplode)) * 7) ];
    //SOUND(20 * game->ShipExplode, 10);
    (game->ShipExplode)--;
    if (game->ShipExplode < 1)
      game->ShipExplode = 3;
    return sprite;
  }

  // top sprite (4 bit)
  if (game->ThrustLEFT)
    sprite = LANDER[(x - game->ShipPosX) + 21];
  else if (game->ThrustRIGHT)
    sprite = LANDER[(x - game->ShipPosX) + 28];
  else
    sprite = LANDER[(x - game->ShipPosX) ];

  // bottom spite (4 bit)
  if (game->ThrustUP && game->Toggle && game->Fuel > 0)
    return (sprite |= LANDER[(x - game->ShipPosX) + 14]);
  else
    return (sprite |= LANDER[(x - game->ShipPosX) + 7]);
}

uint8_t FuelDisplay(uint8_t x, uint8_t y, GAME * game)
{
  if (y != 6) return 0x00;
  if (x > 4 && x <= 19)
  {
    // max fuel = 15.000 Liter - each liter = 1 fuel-bar we have 15 bars
    if ((game->Fuel / 1000) + 1 > x - 4 || ((x - 4 == 1) && game->Fuel > 0))
      return 0xF8;
    else
      return 0x00;
  }
  return 0x00;
}

uint8_t GameDisplay(uint8_t x, uint8_t y, GAME * game)
{
  const uint8_t offset = 23;
  if (x >= offset)
  {
    uint8_t frame;
    if (x == offset || x == 127)
      // left and right border-line
      frame = 0xFF;
    else
      // draw the map from the coordinates given by the GAMEMAP
      frame = GETLANDSCAPE(x - offset, y, ((game->Level - 1) * 2), game);

    uint8_t ship = LanderDisplay(x, y, game);

    if (y == 7 && x >= (game->LandingPadLEFT + offset) && x <= (game->LandingPadRIGHT + offset))
    {
      if (ship != 0 && (0xFC | ship) != (0xFC + ship))
      {
        if (abs(game->velocityY) <= LANDINGSPEED && (game->ShipPosX >= game->LandingPadLEFT + offset) && (game->ShipPosX + 7 <= game->LandingPadRIGHT + offset) )
        {
          game->HasLanded = true;
          return frame | ship;
        }
        else
        {
          if (!game->Collision)
            game->Lives--;
          game->ShipExplode = 3;
          game->Collision = true;

          return frame | LanderDisplay(x, y, game);
        }
      }
    }
    else if  ((frame != 0 && ship != 0) && (frame | ship) != (frame + ship))
    {
      if (!game->Collision)
        game->Lives--;
      game->ShipExplode = 3;
      game->Collision = true;
      return frame | LanderDisplay(x, y, game);
    }

    return frame | ship;
  }
  return 0x00;
}

uint8_t StarsDisplay(uint8_t x, uint8_t y, GAME * game)
{
  const uint8_t o1 = 23;
  uint8_t bg = 0x00;
  if (y == 0 && x > o1)
  {
    bg |= 0x01;
  }
  if (x == o1)
  {
    bg |= 0xFF;
  }
  if (x == 127)
  {
    bg |= 0xFF;
  }
  if (y == 7 && x > o1)
  {
    bg |= 0x80;
  }

  const uint8_t offset = 40;
  if (y > 1 && y < 5)
  {
    if (x > offset &&  x < (offset + 72))
    {
      if (game->Stars > (x - offset) / 24)
      {
        return STARFULL[((x - offset) % 24) + ((y - 2) * 24)];
      }
      else
      {
        return STAROUTLINE[((x - offset) % 24) + ((y - 2) * 24)];
      }
    }
  }
  return bg;
}

uint8_t LivesDisplay(uint8_t x, uint8_t y, GAME * game)
{
  const uint8_t offset = 1;
  if (y == 7 && x >= offset && x < (4 * 5) + offset)
  {
    if (game->Lives > (x - offset) / 5)
      return LIVE[(x - offset) % 5];
  }
  return 0x00;
}

void Frame_Buffer_Clear()
{
    // Clear the screen in RGB565 format
    for (int y = 0; y < 128; y++) {
        for (int x = 0; x < 256; x++) {
            long location = (x * 2) + (y * 640); // 2 bytes per pixel, 640 bytes per line
            *((unsigned short *)(fbp + location)) = 0x0000;
        }
    }
}

void Frame_Buffer_Flip(uint8_t x , uint8_t y, uint8_t data)
{
    //一次给一竖列8bit数据
     for (uint8_t y_pixel = 0; y_pixel < 8; y_pixel++)//解析为单个y_pixel
    {
        uint8_t data_pixel = (data>>y_pixel)&0x01;
        long location1 = (x * 2 * 2) + ((( (y * 8) + y_pixel) * 2 ) * 640);
        long location2 = (x * 2 * 2 + 1) + ((((y * 8)+y_pixel) * 2 + 1 ) * 640);
        if(data_pixel==1)//此像素点应该被点亮
        //等比例放大2倍
        {
            *((unsigned short *)(fbp + location1))= 0xff;//rgb565 fb
            *((unsigned short *)(fbp + location2))= 0xff;//rgb565 fb
        }
    } 
}
void Tiny_Flip(uint8_t mode, GAME * game, DIGITAL * score, DIGITAL * velX, DIGITAL * velY) {
  uint8_t y, x;
  for (y = 0; y < 8; y++)
  {
    for (x = 0; x < 128; x++)
    {
      if (mode == 0) {
        Frame_Buffer_Flip(x, y, GameDisplay(x, y, game) | LivesDisplay(x, y, game) | DashboardDisplay(x, y, game) | ScoreDisplay(x, y, score) | VelocityDisplay(x, y, velX, 1) | VelocityDisplay(x, y, velY, 0) | FuelDisplay(x, y, game));
      } else if (mode == 1) {
        Frame_Buffer_Flip(x, y, INTRO[x + (y * 128)]);
      }
      else if (mode == 2)
      {
        Frame_Buffer_Flip(x, y, StarsDisplay ( x, y, game) | LivesDisplay(x, y, game) | DashboardDisplay(x, y, game) | ScoreDisplay(x, y, score) | VelocityDisplay(x, y, velX, 1) | VelocityDisplay(x, y, velY, 0) | FuelDisplay(x, y, game));
      }
    }
    if (mode == 0 || mode == 2) {
        //munmap(fbp, screensize);
        //close(fbfd);
    }
  }
}

/**************************/
void setup() {
  //SSD1306.ssd1306_init();
  //SSD1306.ssd1306_fillscreen(0x00);
  //TINYJOYPAD_INIT();
}

void loop() {
  DIGITAL score;
  DIGITAL velX;
  DIGITAL velY;
  GAME game;

BEGIN:
  Frame_Buffer_Clear();//进入界面的时候刷新一下界面
  game.Level = 1;
  game.Score = 0;
  game.Lives = 4;
  while (1) {
    Tiny_Flip(1, &game, &score, &velX, &velY);
    if (Space_key_pressed) {//开始游戏按钮 digitalRead(1) == 0
      if (JOYPAD_UP){ 
        game.Level = 10;
        //ALERTSOUND();
      }
      else if (JOYPAD_DOWN) {
        game.Lives = 255;
        //ALERTSOUND();
      }
      else {
        //SOUND(100, 125);
        //SOUND(50, 125);
      }

      goto START;
    }
  }

START:
  Frame_Buffer_Clear();//进入界面的时候刷新一下界面
  initGame(&game);
  //INTROSOUND();
  while (1) {
    Keyboard_Event();
    //Frame_Buffer_Clear();//每次移动飞船后都要刷新一下界面
    fillData(game.Score, &score);
    fillData(game.velocityX, &velX);
    fillData(game.velocityY, &velY);
    moveShip(&game);
    changeSpeed(&game);

    Tiny_Flip(0, &game, &score, &velX, &velY);
    if (game.EndCounter > 8) {
      if (game.HasLanded)
      {
        showAllScoresAndBonuses(&game, &score, &velX, &velY);
        delay(500);
        goto START;
      }
      else
      {
        delay (2000);
        if (game.Lives > 0)
          goto START;
        goto BEGIN;
      }

    }
    if (game.ShipExplode > 0 || game.Collision)
      game.EndCounter++;
    if (game.HasLanded)
      game.EndCounter = 10;
  }
}

int main()
{
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error opening framebuffer device");
        return 1;
    }

    // Get the framebuffer fixed information
    struct fb_fix_screeninfo finfo;
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        perror("Error reading fixed information");
        close(fbfd);
        return 1;
    }

    // Check if the line length matches what we expect (640 bytes)
    if (finfo.line_length != 640) {
        fprintf(stderr, "Unexpected line length: %d\n", finfo.line_length);
        close(fbfd);
        return 1;
    }

    screensize = finfo.line_length * 240; // 640 bytes per line, 240 lines

    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((intptr_t)fbp == -1) {
        perror("Error mapping framebuffer device to memory");
        close(fbfd);
        return 1;
    }

    key_board_fb = Keyboard_Init();

    //TINYJOYPAD_INIT();//此处改为电容触摸屏/外接键盘/板载MPU6050/外接按钮驱动
    while(1)
    {
        loop();
    }
    return 0;
}
