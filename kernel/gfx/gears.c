/* kernel/gfx/gears.c — the classic GLGears demo on top of TinyGL.
 *
 * Three interlocking gears rotate in front of a dark background, one
 * frame per call. This is the desktop's proof that the GL subset works
 * end to end: matrices, immediate mode, z-buffer, per-face shading.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "tgl.h"
#include "gears.h"

#define GEARS_MAX_W 800
#define GEARS_MAX_H 600
#define OUTLINE_MAX 160

static uint32_t *gears_color;
static float *gears_depth;
static int gears_w, gears_h;

int gears_init(int w, int h)
{
    if (w <= 0 || h <= 0 || w > GEARS_MAX_W || h > GEARS_MAX_H)
        return -1;
    if (gears_color && w == gears_w && h == gears_h)
        return 0;
    /* the kernel heap has no free(): allocate once per window size */
    uint32_t *c = malloc((size_t)w * (size_t)h * sizeof(uint32_t));
    float *d = malloc((size_t)w * (size_t)h * sizeof(float));
    if (!c || !d)
        return -1;
    gears_color = c;
    gears_depth = d;
    gears_w = w;
    gears_h = h;
    return 0;
}

const uint32_t *gears_pixels(void) { return gears_color; }
int gears_width(void) { return gears_w; }
int gears_height(void) { return gears_h; }

/* sin/cos without libm (same series as the renderer) */
static float gsin(float x)
{
    const float pi = 3.14159265f;
    const float two_pi = 6.28318531f;
    int k = (int)(x / two_pi + (x < 0.0f ? -0.5f : 0.5f));
    x -= (float)k * two_pi;
    if (x > pi)
        x -= two_pi;
    else if (x < -pi)
        x += two_pi;
    float x2 = x * x;
    return x * (1.0f - x2 * (1.0f / 6.0f -
                x2 * (1.0f / 120.0f - x2 * (1.0f / 5040.0f))));
}

static float gcos(float x) { return gsin(x + 1.57079633f); }

/* one extruded gear: solid disc of radius r_root with trapezoid teeth
 * reaching r_tip, thickness depth, all flat-shaded by face direction
 * (front bright, back dark, rims shaded by their angle) */
static void draw_gear(float r_root, float r_tip, float depth, int teeth,
                     float br, float bg, float bb)
{
    float ox[OUTLINE_MAX], oy[OUTLINE_MAX];
    int n = 0;
    float step = 6.28318531f / (float)teeth;
    float da = step / 4.0f;

    for (int i = 0; i < teeth; i++) {
        float a = (float)i * step;
        /* root start, tooth rise, tooth top end, tooth fall */
        ox[n] = r_root * gcos(a);
        oy[n++] = r_root * gsin(a);
        ox[n] = r_root * gcos(a + da);
        oy[n++] = r_root * gsin(a + da);
        ox[n] = r_tip * gcos(a + da);
        oy[n++] = r_tip * gsin(a + da);
        ox[n] = r_tip * gcos(a + 2.0f * da);
        oy[n++] = r_tip * gsin(a + 2.0f * da);
        ox[n] = r_root * gcos(a + 2.0f * da);
        oy[n++] = r_root * gsin(a + 2.0f * da);
    }

    float front = depth * 0.5f, back = -depth * 0.5f;
    (void)back;

    /* front face: triangle fan around the centre (the outline is
     * star-shaped, so a fan is a valid triangulation) */
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(br, bg, bb);
    glVertex3f(0.0f, 0.0f, front);
    for (int i = 0; i < n; i++)
        glVertex3f(ox[i], oy[i], front);
    glVertex3f(ox[0], oy[0], front);
    glEnd();

    /* no back faces: the gears are viewed from the front, and the
     * z-buffer would reject every one of their pixels anyway */

    /* rim walls, shaded by direction so the gear reads as 3D */
    glBegin(GL_QUADS);
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        float mx = (ox[i] + ox[j]) * 0.5f;
        float my = (oy[i] + oy[j]) * 0.5f;
        float len = mx < 0.0f ? -mx : mx;
        float l2 = my < 0.0f ? -my : my;
        len += l2;
        if (len < 1e-6f)
            len = 1.0f;
        float shade = 0.45f + 0.45f * (mx < 0.0f ? -mx : mx) / len;
        glColor3f(br * shade, bg * shade, bb * shade);
        glVertex3f(ox[i], oy[i], front);
        glVertex3f(ox[j], oy[j], front);
        glVertex3f(ox[j], oy[j], back);
        glVertex3f(ox[i], oy[i], back);
    }
    glEnd();

    /* hub on the front face — suggests the classic axle hole */
    glBegin(GL_TRIANGLE_FAN);
    glColor3f(0.12f, 0.15f, 0.20f);
    glVertex3f(0.0f, 0.0f, front + 0.02f);
    for (int i = 0; i <= n; i += 4) {
        int k = i % n;
        glVertex3f(ox[k] * 0.22f, oy[k] * 0.22f, front + 0.02f);
    }
    glEnd();
}

void gears_render(float t)
{
    if (!gears_color || !gears_depth || gears_w <= 0 || gears_h <= 0)
        return;

    tgl_bind(gears_color, gears_depth, gears_w, gears_h);

    glViewport(0, 0, gears_w, gears_h);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    /* dark sea-blue background, same tone as the window panel */
    glClearColor(0.07f, 0.11f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)gears_w / (float)gears_h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-aspect, aspect, -1.0, 1.0, 5.0, 80.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0f, 0.0f, -40.0f);
    glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(25.0f, 0.0f, 1.0f, 0.0f);

    float angle = t * 55.0f; /* degrees per second */

    /* the three classic gears, interlocking via opposite rotation */
    glPushMatrix();
    glTranslatef(-3.0f, -2.0f, 0.0f);
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    draw_gear(3.2f, 4.0f, 1.0f, 20, 0.45f, 0.85f, 0.40f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(3.1f, -2.0f, 0.0f);
    glRotatef(-2.0f * angle - 9.0f, 0.0f, 0.0f, 1.0f);
    draw_gear(1.5f, 2.1f, 2.0f, 10, 0.35f, 0.65f, 0.95f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-3.1f, 4.2f, 0.0f);
    glRotatef(-2.0f * angle - 25.0f, 0.0f, 0.0f, 1.0f);
    draw_gear(1.3f, 2.0f, 1.0f, 10, 0.95f, 0.82f, 0.35f);
    glPopMatrix();

    glFlush();
}
