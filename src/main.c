#include "pd_api.h"
#include "pd_api/pd_api_gfx.h"
#include <stdio.h>

static int update(void *userdata);
const char *fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";
LCDFont *font = NULL;
LCDBitmap *heidi = NULL;

#ifdef _WINDLL
__declspec(dllexport)
#endif

int eventHandler(PlaydateAPI *pd, PDSystemEvent event, uint32_t arg) {
  if (event == kEventInit) {
    const char *err;
    font = pd->graphics->loadFont(fontpath, &err);

    if (font == NULL)
      pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__,
                        fontpath, err);

    const char *err2;
    heidi = pd->graphics->loadBitmap("sprites/heidi.png", &err2);
    if (heidi == NULL)
      pd->system->error("Couldn't load heidi!! %s", err2);

    pd->system->setUpdateCallback(update, pd);
  }

  return 0;
}

#define TEXT_WIDTH 86
#define TEXT_HEIGHT 16

float x = (400 - TEXT_WIDTH) / 2.0f;
float y = (240 - TEXT_HEIGHT) / 2.0f;
int dx = 1;
int dy = 2;

static int update(void *userdata) {
  PlaydateAPI *pd = userdata;

  pd->graphics->clear(kColorBlack);
  pd->graphics->setFont(font);
  char *text = "heidi game heidi game";
  pd->graphics->setDrawMode(kDrawModeCopy);
  pd->graphics->drawBitmap(heidi, 64, 64, false);
  pd->graphics->setDrawMode(kDrawModeFillWhite);
  pd->graphics->drawText(text, strlen(text), kASCIIEncoding, x, y);

  float change = pd->system->getCrankChange();

  x += dx * (1.0f + change / 2.0f);
  y += dy * (1.0f + change / 2.0f);

  if (x < 0 || x > LCD_COLUMNS - TEXT_WIDTH) {
    if (x < 0)
      x = 0;
    if (x > LCD_COLUMNS - TEXT_WIDTH)
      x = LCD_COLUMNS - TEXT_WIDTH;
    dx = -dx;
  }

  if (y < 0 || y > LCD_ROWS - TEXT_HEIGHT) {
    if (y < 0)
      y = 0;
    if (y > LCD_ROWS - TEXT_HEIGHT)
      y = LCD_ROWS - TEXT_HEIGHT;

    dy = -dy;
  }

  return 1;
}
