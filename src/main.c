#include "pd_api.h"
#include "pd_api/pd_api_gfx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int update(void *userdata);
const char *fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";
LCDFont *font = NULL;
LCDBitmap *heidi = NULL;

#ifdef _WINDLL
__declspec(dllexport)
#endif

typedef struct {
  float x;
  float y;
} Vec2;

typedef struct {
  Vec2 target;
  float targetDist;
} SoftbodySpring;

typedef struct {
  Vec2 pos;
  SoftbodySpring *springs;
  int springAmt;
} SoftbodyNode;

typedef struct {
  SoftbodyNode *nodes;
  int nodeAmt;
} Softbody;

float distance(Vec2 *a, Vec2 *b) {
  int x = a->x - b->x;
  int y = a->y - b->y;
  return sqrt(x * x + y * y);
}

Softbody createSoftbody(SoftbodyNode nodes[4], int nodeAmt) {
  for (int i = 0; i < nodeAmt; i++) {
    SoftbodyNode node = nodes[i];
    for (int j = 0; j < 4; j++) {
      SoftbodySpring spring = node.springs[j];
      float length = distance(&node.pos, &spring.target);
      spring.targetDist = length;
    }
  }
  Softbody body = {nodes, nodeAmt};
  return body;
}

Softbody createSquareSoftbody(float length) {
  Vec2 nw = {0.0, 0.0};
  Vec2 ne = {length, 0.0};
  Vec2 se = {length, length};
  Vec2 sw = {0.0, length};
  Vec2 points[4] = {nw, ne, se, sw};
  SoftbodyNode *nodes = malloc(sizeof(SoftbodyNode) * 4);
  for (int i = 0; i < 4; i++) {
    int next = i + 1;
    if (next >= 4) {
      next = 0;
    }
    SoftbodySpring spring = {points[next], 0};
    SoftbodySpring *springs = malloc(sizeof(SoftbodySpring) * 1);
    springs[0] = spring;
    SoftbodyNode node = {points[i], springs};
    nodes[i] = node;
  };

  return createSoftbody(nodes, 4);
}

Softbody square;

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

    square = createSquareSoftbody(10.0);

    pd->system->setUpdateCallback(update, pd);
  }

  return 0;
}

static int update(void *userdata) {
  PlaydateAPI *pd = userdata;
  pd->graphics->clear(kColorBlack);

  pd->graphics->setDrawMode(kDrawModeFillWhite);
  for (int i = 0; i < square.nodeAmt; i++) {
    char text[128];
    sprintf(text, "%d: (%f,%f)", i, square.nodes[i].pos.x,
            square.nodes[i].pos.y);
    pd->graphics->drawText(text, strlen(text), kASCIIEncoding, 0, i * 20);
  }

  return 1;
}
