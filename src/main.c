#include "pd_api.h"
#include "pd_api/pd_api_gfx.h"
#include "pd_api/pd_api_sys.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

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

struct SoftbodyNode;

typedef struct {
  struct SoftbodyNode *target;
  float targetLength;
} SoftbodySpring;

typedef struct SoftbodyNode {
  Vec2 pos;
  Vec2 velocity;
  SoftbodySpring *springs;
  int springAmt;
  float distFromCenter;
} SoftbodyNode;

typedef struct {
  SoftbodyNode *nodes;
  int nodeAmt;
  Vec2 center;
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
Vec2 sub(Vec2 a, Vec2 b) {
  a.x -= b.x;
  a.y -= b.y;
  return a;
}
Vec2 mulVec2(Vec2 a, Vec2 b) {
  a.x *= b.x;
  a.y *= b.y;
  return a;
}
Vec2 mulScalar(Vec2 a, float b) {
  a.x *= b;
  a.y *= b;
  return a;
}
float lerp(float a, float b, float t) { return a + (b - a) * t; }
Vec2 lerpVec2(Vec2 a, Vec2 b, float t) {
  a.x = lerp(a.x, b.x, t);
  a.y = lerp(a.y, b.y, t);
  return a;
}

float dot(Vec2 a, Vec2 b) {
  float result = 0.0;
  result += a.x * b.x;
  result += a.y * b.y;
  return result;
}
float absFloat(float a) {
  if (a < 0.0f) {
    return a * -1.0f;
  }
  return a;
}

Vec2 divVec2(Vec2 a, Vec2 b) {
  a.x /= b.x;
  a.y /= b.y;
  return a;
}
Vec2 divScalar(Vec2 a, float b) {
  a.x /= b;
  a.y /= b;
  return a;
}

bool pointInside(Softbody *body, Vec2 point) {
  // body's nodes must be defined in clockwise order!

  for (int i = 0; i < body->nodeAmt; i++) {
    int next = i + 1;
    if (next >= body->nodeAmt) {
      next = 0;
    }
    SoftbodyNode *node = &body->nodes[i];
    SoftbodyNode *nextNode = &body->nodes[next];

    // slope = ((d-b)/(c-a))
    // f(x)  = slope*x+d-slope*c
    float slope =
        (nextNode->pos.y - node->pos.y) / (nextNode->pos.x - node->pos.x);

    float lineVal = slope * point.x + nextNode->pos.y - slope * nextNode->pos.x;

    bool onInside = point.y > lineVal;
    if (slope < 0.0f ^ nextNode->pos.y < node->pos.y) {
      onInside = !onInside;
    }

    if (!onInside) {
      return false;
    }
  }
  return true;
}

Vec2 getBodyCenter(Softbody *body) {
  Vec2 center = {0.0, 0.0};
  for (int i = 0; i < body->nodeAmt; i++) {
    SoftbodyNode *node = &body->nodes[i];
    center = add(center, node->pos);
  }
  center = divScalar(center, body->nodeAmt);
  return center;
}

Softbody createSoftbody(SoftbodyNode nodes[4], int nodeAmt) {
  Vec2 center = {0.0, 0.0};
  for (int i = 0; i < nodeAmt; i++) {
    SoftbodyNode *node = &nodes[i];
    center = add(center, node->pos);
    for (int j = 0; j < node->springAmt; j++) {
      SoftbodySpring *spring = &node->springs[j];
      float length = distance(node->pos, spring->target->pos);
      spring->targetLength = length;
    }
  }
  center = divScalar(center, nodeAmt);

  for (int i = 0; i < nodeAmt; i++) {
    SoftbodyNode *node = &nodes[i];
    node->distFromCenter = distance(node->pos, center);
  }

  Softbody body = {nodes, nodeAmt, center};
  return body;
}

void freeSoftbody(Softbody *body) {
  free(body->nodes->springs);
  free(body->nodes);
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

    int springAmt = 1;
    if (i <= 1) {
      springAmt = 2;
    }

    SoftbodySpring spring = {&nodes[next], 0};
    SoftbodySpring *springs = malloc(sizeof(SoftbodySpring) * springAmt);
    springs[0] = spring;

    if (springAmt > 1) {
      SoftbodySpring diagonal = {&nodes[i + 2], 0};
      springs[1] = diagonal;
    }

    SoftbodyNode node = {points[i], {0}, springs, springAmt, 0.0};
    nodes[i] = node;
  };

  return createSoftbody(nodes, 4);
}

float lengthVec2(Vec2 vec) {
  Vec2 origin = {0.0, 0.0};
  return distance(vec, origin);
}

Vec2 rotateAround(Vec2 point, Vec2 anchor, float theta, float distFromAnchor) {
  point = sub(point, anchor);
  float angle = atan2f(point.y, point.x);
  angle += theta;
  point.x = cosf(angle) * distFromAnchor;
  point.y = sinf(angle) * distFromAnchor;
  point = add(point, anchor);
  return point;
}

void rotateBody(Softbody *body, float theta) {
  for (int i = 0; i < body->nodeAmt; i++) {
    SoftbodyNode *node = &body->nodes[i];
    node->pos =
        rotateAround(node->pos, body->center, theta, node->distFromCenter);
  }
}

Vec2 origin = {(400.0f - 50.0f) / 2, 48.0f};

Softbody *softbodies = NULL;
int softbodyCount = 0;
int allocatedBodies = 0;

void addBody(Softbody body) {
  int toAllocate = sizeof(Softbody) * (softbodyCount + 1);
  if (toAllocate > allocatedBodies) {
    softbodies = realloc(softbodies, toAllocate);
    allocatedBodies = toAllocate;
    printf("allocated %i\n", allocatedBodies);
  }
  softbodies[softbodyCount] = body;
  softbodyCount++;
}
void clearBodies(void) {
  for (int i = 0; i < softbodyCount; i++) {
    freeSoftbody(&softbodies[i]);
  }
  softbodyCount = 0;
}

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
    pd->system->setPeripheralsEnabled(kAccelerometer);
    addBody(createSquareSoftbody(50.0, origin));
  }

  return 0;
}

Vec2 gravity = {0.0, 0.0};
bool freezeMovement = false;
Vec2 testPoint = {0.0, 0.0};

static int update(void *userdata) {
  PlaydateAPI *pd = userdata;
  pd->graphics->clear(kColorBlack);

  pd->graphics->setDrawMode(kDrawModeFillWhite);
  float delta_time = 0.1;
  // delta_time = pd->system->getCrankChange() / 100.0f;
  float crankChange = pd->system->getCrankChange();
  if (crankChange != 0.0f) {
    rotateBody(&softbodies[0], crankChange / 180.0f * 3.14f);
  }

  float accX = 0;
  float accY = 0;
  float accZ = 0;
  pd->system->getAccelerometer(&accX, &accY, &accZ);

  pd->graphics->drawRect(testPoint.x - 2.0f, testPoint.y - 2.0f, 4.0, 4.0,
                         kColorWhite);
  gravity.x = accX * 58.0f;
  gravity.y = accY * 58.0f;

  // update loop
  for (int s = 0; s < softbodyCount; s++) {
    Softbody *body = &softbodies[s];

    if (!freezeMovement) {
      for (int i = 0; i < body->nodeAmt; i++) {
        SoftbodyNode *node = &body->nodes[i];
        node->velocity = add(node->velocity, mulScalar(gravity, delta_time));

        if (node->pos.y >= 240) {
          float delta = node->pos.y - 240;
          node->velocity.y -= delta * 100.0f * delta_time;
        }
        if (node->pos.x >= 400) {
          float delta = node->pos.x - 400;
          node->velocity.x -= delta * 100.0f * delta_time;
        }
        if (node->pos.y < 0) {
          float delta = node->pos.y;
          node->velocity.y -= delta * 100.0f * delta_time;
        }
        if (node->pos.x < 0) {
          float delta = node->pos.x;
          node->velocity.x -= delta * 100.0f * delta_time;
        }

        // check collisions
        for (int os = 0; os < softbodyCount; os++) {
          if (os == s)
            continue;

          bool inside = pointInside(&softbodies[os], node->pos);
          if (inside) {
            Vec2 dirToCenter = sub(body->center, softbodies[os].center);
            // float length = lengthVec2(dirToCenter);
            // dirToCenter = divScalar(dirToCenter, length);
            node->velocity =
                add(node->velocity, mulScalar(dirToCenter, 26.0f * delta_time));
          }
        }

        for (int j = 0; j < node->springAmt; j++) {
          SoftbodySpring *spring = &node->springs[j];
          float dist = distance(node->pos, spring->target->pos);

          float stiffness = 24.0;
          float damping = 2.2;

          float springForce = (spring->targetLength - dist) * stiffness;

          Vec2 velocityDelta = sub(spring->target->velocity, node->velocity);
          Vec2 direction = divScalar(sub(spring->target->pos, node->pos), dist);
          float dampForce = -damping * dot(velocityDelta, direction);

          Vec2 totalForce = mulScalar(direction, springForce + dampForce);

          spring->target->velocity =
              add(spring->target->velocity, mulScalar(totalForce, delta_time));

          node->velocity =
              add(node->velocity, mulScalar(totalForce, -delta_time));
        }

        node->pos = add(node->pos, mulScalar(node->velocity, delta_time));
      }
    }

    body->center = getBodyCenter(body);

    // draw loop
    for (int i = 0; i < body->nodeAmt; i++) {
      SoftbodyNode *node = &body->nodes[i];
      // char text[128];
      // sprintf(text, "%d: (%f,%f)", i, node.pos.x, node.pos.y);
      // pd->graphics->drawText(text, strlen(text), kASCIIEncoding, 0, i * 20);

      pd->graphics->drawRect(node->pos.x - 4, node->pos.y - 4, 8, 8,
                             kColorWhite);

      for (int j = 0; j < node->springAmt; j++) {
        SoftbodySpring *spring = &node->springs[j];
        pd->graphics->drawLine(node->pos.x, node->pos.y, spring->target->pos.x,
                               spring->target->pos.y, 2, kColorWhite);
      }
    }
  }

  PDButtons current;
  PDButtons pushed;
  PDButtons released;
  pd->system->getButtonState(&current, &pushed, &released);
  if (current & kButtonRight) {
    testPoint.x += 60.0f * delta_time;
  }
  if (current & kButtonLeft) {
    testPoint.x -= 60.0f * delta_time;
  }
  if (current & kButtonUp) {
    testPoint.y -= 60.0f * delta_time;
  }
  if (current & kButtonDown) {
    testPoint.y += 60.0f * delta_time;
  }

  if (pushed & kButtonA) {
    if (freezeMovement) {
      clearBodies();
    }
    addBody(createSquareSoftbody(50.0, origin));
  }
  if (pushed & kButtonB) {
    freezeMovement = !freezeMovement;
  }

  pd->system->drawFPS(0, 0);

  return 1;
}
