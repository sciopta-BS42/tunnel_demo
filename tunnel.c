/*SOC
**********************************************************************
**  _______  _______ _________ _______  _______ _________ _______   **
** (  ____ \(  ____ \\__   __/(  ___  )(  ____ )\__   __/(  ___  )  **
** | (    \/| (    \/   ) (   | (   ) || (    )|   ) (   | (   ) |  **
** | (_____ | |         | |   | |   | || (____)|   | |   | (___) |  **
** (_____  )| |         | |   | |   | ||  _____)   | |   |  ___  |  **
**       ) || |         | |   | |   | || (         | |   | (   ) |  **
** /\____) || (____/\___) (___| (___) || )         | |   | )   ( |  **
** \_______)(_______/\_______/(_______)|/          )_(   |/     \|  **
**                                                                  **
**                   (c) 2021 SCIOPTA Systems GmbH                  **
**                                                                  **
**********************************************************************
** ID: S21147BS1                                                    **
** $Revision$                                             **
** $Date$                                     **
** Simple GFX test process                                          **
**********************************************************************
EOC*/

#include <sciopta.h>
#include <math.h>
#include <gfx/simple.h>
#include "fsl_elcdif.h"

void initWaitVBL();

extern const gfx_screenInfo_t * screen;

#define PI    3.141529f
#define MAX_Y 272
#define MAX_X 480

uint8_t dist[MAX_Y][MAX_X];
uint8_t light[MAX_Y][MAX_X];
uint8_t angle[MAX_Y][MAX_X];

void gfx_string(const gfx_screenInfo_t * screen, char *s)
{
  while( *s ){
    gfx_putchar(screen, *s++);
  }
}

void initLut()
{
  uint32_t i;
  uint8_t r;
  uint8_t b;
  uint8_t g;

  LCDIF->LUT0_ADDR = 0;
  for(g = 0, r = 0, b = 0, i = 0; i < 256; ++i){
    b = i;
    if ( b > 31 ){
      b = 31;
    }
    if ( i > 31 ){
      r = (i - 32);
      if ( r > 31 ) r = 31;
    }
    if ( i > 63 ){
      g = (i - 63);
      if ( g > 63 ) g = 63;
    }
    LCDIF->LUT0_DATA = (r<<11) | b | (g<<5);
  }
}

float distance(int x, int y)
{
  x -= (MAX_X)/2;
  y -= (MAX_Y)/2;
  return sqrtf((float)(x*x+y*y));
}

SC_PROCESS(tunnel)
{
  int x,y;
  float d;
  int di;

  initLut();

  for(x = 0; x < 129; ++x){
    gfx_colorSet(screen, 0, x);
    gfx_rectangle(screen, x, x, (480-2*x), (272-2*x));
    sc_sleep(2);
  }

  gfx_fontSet(screen,BIG_FNT);
  gfx_colorSet(screen, 0, 255);
  ((gfx_screenInfo_t *)screen)->char_x = 153;
  ((gfx_screenInfo_t *)screen)->char_y = 130;
  gfx_string(screen, "SCIOPTA on mimxrt1064-evk");

  for(y = 0; y < MAX_Y/2; ++y){
    for(x = 0; x < MAX_X/2; ++x){
      d = distance(x,y);
      di = (int)(10000.f/d);
      dist[y][x] = di;
      dist[y][(MAX_X-1)-x] = di;
      dist[(MAX_Y-1)-y][x] = di;
      dist[(MAX_Y-1)-y][(MAX_X-1)-x] = di;

      di = (int)(d/1.5f);
      if ( di > 255 ){
        di = 255;
      }
      if ( di < 0 ){
        di = 0;
      }
      light[y][x] = di;
      light[y][(MAX_X-1)-x] = di;
      light[(MAX_Y-1)-y][x] = di;
      light[(MAX_Y-1)-y][(MAX_X-1)-x] = di;
    }
  }

  for(y = 0; y < MAX_Y; ++y){
    int _y = MAX_Y/2-y;
    for(int x = 0; x < MAX_X; ++x){
      int _x = MAX_X/2-x;
      float a = 0;

      if ( _y == 0 ){
        if ( _x < 0 ){
          a = PI/2.f;
        } else {
          a = -PI/2.f;
        }
      } else {
        a = atanf((float)_x/(float)_y);
      }
      if ( _y > 0 ){
        a += PI;
      }
      angle[y][x] = (int)(a*256.f/(2.f*PI)+128.f);
    }
  }

  sc_sleep(2000);
  for(x = 129; x >= 0; --x){
    gfx_colorSet(screen, 0, 129-x);
    gfx_rectangle(screen, x, x, (480-2*x), (272-2*x));
    sc_sleep(2);
  }

  initWaitVBL();
  sc_triggerWait(2, SC_ENDLESS_TMO);

  int ox = 0;
  int oy = 0;
  int depth = 0;
  for(;;){
    oy = sinf((float)(depth)/512.f*PI)*255.f+128.f;
    --ox;
    ox &= 0xff;
    oy &= 0xff;
    ++depth;
    depth &= 1023;

    sc_triggerWait(2, SC_ENDLESS_TMO);

    uint8_t *scr = (uint8_t *)screen->rawscreen->screen;

    for(y = 0; y < MAX_Y; y += 1){
      for(x = 0; x < MAX_X; x += 1){
        int tx = dist[y][x]-ox;
        int ty = (angle[y][x])-oy;
        int col = light[y][x];
        if ( col < 15 ) col = 0;
        if ( ((tx ^ ty) & 0x10)  ){
          col /=2;
        }
        *scr ++ = col;
      }
    }
  }
}
