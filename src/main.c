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

const float GRAVITY = 9.8;

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
  Vec2 velocity;
  SoftbodySpring *springs;
  int springAmt;
} SoftbodyNode;

typedef struct {
  SoftbodyNode *nodes;
  int nodeAmt;
} Softbody;

float distance(Vec2 a, Vec2 b) {
  int x = a.x - b.x;
  int y = a.y - b.y;
  return sqrt(x * x + y * y);
}
Vec2 add(Vec2 a, Vec2 b) {
  a.x += b.x;
  a.y += b.y;
  return a;
}
Vec2 mul(Vec2 a, Vec2 b) {
  a.x *= b.x;
  a.y *= b.y;
  return a;
}
Vec2 mulScalar(Vec2 a, float b) {
  a.x *= b;
  a.y *= b;
  return a;
}

Softbody createSoftbody(SoftbodyNode nodes[4], int nodeAmt) {
  for (int i = 0; i < nodeAmt; i++) {
    SoftbodyNode node = nodes[i];
    for (int j = 0; j < 4; j++) {
      SoftbodySpring spring = node.springs[j];
      float length = distance(node.pos, spring.target);
      spring.targetDist = length;
    }
  }
  Softbody body = {nodes, nodeAmt};
  return body;
}

Softbody createSquareSoftbody(float length, Vec2 origin) {
  Vec2 nw = {origin.x, origin.y};
  Vec2 ne = {length + origin.x, origin.y};
  Vec2 se = {length + origin.x, length + origin.y};
  Vec2 sw = {origin.x, length + origin.y};
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
    SoftbodyNode node = {points[i], {0}, springs, 1};
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

    float width = 50.0f;
    Vec2 origin = {(400.0f - width) / 2, 16.0f};
    square = createSquareSoftbody(width, origin);

    pd->system->setUpdateCallback(update, pd);
  }

  return 0;
}

static int update(void *userdata) {
  PlaydateAPI *pd = userdata;
  pd->graphics->clear(kColorBlack);

  pd->graphics->setDrawMode(kDrawModeFillWhite);
  float delta_time = 0.1;

  // update loop
  for (int i = 0; i < square.nodeAmt; i++) {
    SoftbodyNode *node = &square.nodes[i];
    node->velocity.y += GRAVITY * delta_time;

    for (int j = 0; j < node->springAmt; j++) {
      SoftbodySpring target = node->springs[j];
    }

    node->pos = add(node->pos, mulScalar(node->velocity, delta_time));
  }

  // draw loop
  for (int i = 0; i < square.nodeAmt; i++) {
    SoftbodyNode *node = &square.nodes[i];
    // char text[128];
    // sprintf(text, "%d: (%f,%f)", i, node.pos.x, node.pos.y);
    // pd->graphics->drawText(text, strlen(text), kASCIIEncoding, 0, i * 20);

    pd->graphics->drawRect(node->pos.x - 4, node->pos.y - 4, 8, 8, kColorWhite);

    for (int j = 0; j < node->springAmt; j++) {
      SoftbodySpring *target = &node->springs[j];
      pd->graphics->drawLine(node->pos.x, node->pos.y, target->target.x,
                             target->target.y, 2, kColorWhite);
    }
  }

  return 1;
}
