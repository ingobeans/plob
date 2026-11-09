#include "pd_api.h"

static int update(void *userdata);
const char *fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";
LCDFont *font = NULL;

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

    // Note: If you set an update callback in the kEventInit handler, the system
    // assumes the game is pure C and doesn't run any Lua code in the game
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

  pd->graphics->clear(kColorWhite);
  pd->graphics->setFont(font);
  char *text = "hi there !! <3";
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

  pd->system->drawFPS(0, 0);

  return 1;
}
