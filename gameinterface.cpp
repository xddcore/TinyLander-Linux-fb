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
// 修改移植以支持Linux Frame Buffer，以及USB键盘或MPU6050&电容触摸屏的体感控制。
// 更多详情见README.md

#include "gameinterface.h"

/*Frame Buffer*/
int frame_buffer_fb = -1;
char *fbp=NULL;
int screensize = 0;

/*USB Keyboard*/
int key_board_fb = -1;
//0:没按下 1:按下 2:长按
int A_key_pressed = 0;
int D_key_pressed = 0;
int Space_key_pressed = 0;

void TINYJOYPAD_INIT(void) {

}


void SetLandingMap(uint8_t level, GAME *game)
{
  uint8_t i;
  uint8_t prev;
  game->LandingPadLEFT = 0;
  game->LandingPadRIGHT = 255;
  for (i = 0; i < 27; i++)
  {
    uint8_t val = GAMEMAP[(level - 1) * 2][i];

    if ((prev == 0 && (val != 0 || i == 26)) && game->LandingPadRIGHT == 0)
    {
      game->LandingPadRIGHT  = i * 4;
    }

    if (val == 0 && game->LandingPadLEFT == 0)
    {
      game->LandingPadLEFT = i * 4;
      game->LandingPadRIGHT = 0;
    }

    prev = val;
  }
}

void SETNEXTLEVEL(uint8_t level, GAME *game)
{
  if ( level > NUMOFGAMES)
    level = 1;
  game->Level = level;
  SetLandingMap(level, game);
  game->ShipPosX = GAMELEVEL[level - 1][0];
  game->ShipPosY = GAMELEVEL[level - 1][1];
  game->Fuel = 100 * GAMELEVEL[level - 1][2];
  game->LevelScore = GAMELEVEL[level - 1][3];
  game->FuelBonus = 100 * GAMELEVEL[level - 1][4];
}

uint8_t GETLANDSCAPE(uint8_t x, uint8_t y, uint8_t level, GAME *game)
{
  const uint8_t height = 63;
  uint8_t frame = 0x00;
  uint8_t t =  x % 4;
  uint8_t ind = x / 4;
  uint8_t val = height - GAMEMAP[level][ind];
  uint8_t valT = height - GAMEMAP[level + 1][ind];
  if (x > 0 && t != 0)
  {
    if ( (ind + 1) < 27)
    {
      if (val < height)
      { uint8_t val2 = height - GAMEMAP[level][ind + 1];
        val += ((val2 - val) / 4) * ( t);
      }
      uint8_t valT2 = height - GAMEMAP[level + 1][ind + 1];
      valT += ((valT2 - valT) / 4) * ( t);
    }
  }

  uint8_t b = val / 8;
  uint8_t bT = valT / 8;
  if (b == y)
  {
    // draw the landing-platform
    if (val == height)
      if (x % 2 == 0)
        frame |= 0xB8;
      else
        frame |= 0x58;
    else
      // draw pixel on the correct height
      frame |= (0xFF << (val - (b * 8)) ) ;
  }
  if (bT == y)
    frame |= (0xFF >>  7 - (valT - (bT * 8)));
  if (y > b || y < bT )
    frame |= 0xFF;

  return frame;
}

// splits each digit in it's own byte
void SPLITDIGITS(uint16_t val, uint8_t *d)
{
  d[4] = val / 10000;
  d[3] = (val - (d[4] * 10000)) / 1000;
  d[2] = (val - (d[3] * 1000) - (d[4] * 10000)) / 100;
  d[1] = (val - (d[2] * 100) - (d[3] * 1000) - (d[4] * 10000)) / 10;
  d[0] = val - (d[1] * 10) - (d[2] * 100) - (d[3] * 1000) - (d[4] * 10000);
}

void SOUND(uint8_t freq, uint8_t dur) {
  //
}

void INTROSOUND()
{
  //SOUND(80, 55); _delay_ms(20); SOUND(90, 55); _delay_ms(20); SOUND(100, 55); SOUND(115, 255); SOUND(115, 255);
}
void VICTORYSOUND()
{
  //SOUND(111, 100); _delay_ms(20); SOUND(111, 90); _delay_ms(20); SOUND(144, 255); SOUND(144, 255); SOUND(144, 255);
}

void ALERTSOUND()
{
  //SOUND(150, 100); _delay_ms(100); SOUND(150, 90); _delay_ms(100); SOUND(150, 100);
}

void HAPPYSOUND()
{
  //SOUND(75, 90); _delay_ms(10); SOUND(114, 90); SOUND(121, 90);
}

/*********************************以下为Linux接口**********************************/
/*Frame Buffer初始化*/
int Frame_Buffer_Init()
{
    int fbfd;
    fbfd = open(Frame_Buffer_Device, O_RDWR);
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
}
//全屏清除，速度慢，会闪屏
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

//静态部分保留，动态部分清除
void Frame_Buffer_Clear_Part(uint8_t x , uint8_t y, uint8_t data)
{
  //待实现
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
/*USB键盘初始化*/
int Keyboard_Init()
{
    int fd = open(KEYBOARD_DEVICE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening keyboard device");
        return -1;
    }
    return fd;
}
/*USB键盘事件处理*/
void Keyboard_Event()
{
    struct input_event ev;
    // Change file descriptor to non-blocking
    int flags = fcntl(key_board_fb, F_GETFL, 0);
    fcntl(key_board_fb, F_SETFL, flags | O_NONBLOCK);
    ssize_t n;
    //确保实时性，每次读空event缓冲区。
    do {
        n = read(key_board_fb, &ev, sizeof(struct input_event));
        //ev.value： 0-按键没按下 1-按键按下 2-按键长按
        if (n == sizeof(struct input_event)) {
            if (ev.type == EV_KEY) {
                switch (ev.code) {
                    case KEY_A:
                        A_key_pressed = ev.value;  // Set A_key_pressed to 1 if A key pressed, 0 otherwise
                        //printf("A key state: %d\n", A_key_pressed);
                        break;
                    case KEY_D:
                        D_key_pressed = ev.value;  // Set D_key_pressed to 1 if D key pressed, 0 otherwise
                        //printf("D key state: %d\n", D_key_pressed);
                        break;
                    case KEY_SPACE:
                        Space_key_pressed = ev.value;  // Set Space_key_pressed to 1 if Space key pressed, 0 otherwise
                        //printf("Space key state: %d\n", Space_key_pressed);
                        break;
                    default:  // Other key pressed
                        break;
                }
            }
        } 
    } while (n > 0);
}