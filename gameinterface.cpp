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
// the code works at 16MHZ internal
// and use ssd1306xled Library for SSD1306 oled display 128x64

#include "gameinterface.h"

int key_board_fb = -1;
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

/*触摸屏事件接口*/
int check_touch() {
    int fd = open(Touch_Screen_Event, O_RDONLY | O_NONBLOCK);
    if (fd == -1) {
        perror("Error opening touch device");
        return -1;
    }

    struct input_event ev;
    ssize_t n = read(fd, &ev, sizeof(ev));
    close(fd);

    if (n == sizeof(ev)) {
        if (ev.type == EV_ABS) {
            return 1; // 触摸事件
        }
    }

    return 0; // 没有触摸事件
}

int Keyboard_Init()
{
    int fd = open(KEYBOARD_DEVICE, O_RDONLY);
    if (fd == -1) {
        perror("Error opening keyboard device");
        return -1;
    }
    return fd;
}

void Keyboard_Event()
{
    struct input_event ev;

    // Change file descriptor to non-blocking
    int flags = fcntl(key_board_fb, F_GETFL, 0);
    fcntl(key_board_fb, F_SETFL, flags | O_NONBLOCK);

    ssize_t n = read(key_board_fb, &ev, sizeof(struct input_event));

    if (n == sizeof(struct input_event)) {
            if (ev.type == EV_KEY) {
                switch (ev.code) {
                    case KEY_A:
                        A_key_pressed = ev.value;  // Set A_key_pressed to 1 if A key pressed, 0 otherwise
                        if(ev.value == 1) {
                            printf("A key pressed\n");
                        }
                        break;
                    case KEY_D:
                        D_key_pressed = ev.value;  // Set D_key_pressed to 1 if D key pressed, 0 otherwise
                        if(ev.value == 1) {
                            printf("D key pressed\n");
                        }
                        break;
                    case KEY_SPACE:
                        Space_key_pressed = ev.value;  // Set Space_key_pressed to 1 if Space key pressed, 0 otherwise
                        if(ev.value == 1) {
                            printf("Space key pressed\n");
                        }
                        break;
                    default:  // Other key pressed
                      A_key_pressed=0;
                      D_key_pressed=0;
                      Space_key_pressed=0;
                        break;
                }
            }
        } 
    else{
      A_key_pressed=0;
      D_key_pressed=0;
      Space_key_pressed=0;
    }


}

int isSpaceKeyPressed()
{
    struct input_event ev;

    // Make sure file descriptor is blocking
    int flags = fcntl(key_board_fb, F_GETFL, 0);
    fcntl(key_board_fb, F_SETFL, flags & ~O_NONBLOCK);

    ssize_t n = read(key_board_fb, &ev, sizeof(struct input_event));

    if (n == sizeof(struct input_event)) {
        if (ev.type == EV_KEY && ev.code == KEY_SPACE) {
            printf("Space key pressed\n");
            return ev.value == 1 ? 1 : 0;  // Return 1 if space key pressed, 0 otherwise
        }
    }

    return 0;
}