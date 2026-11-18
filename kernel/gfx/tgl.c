/* kernel/gfx/tgl.c — the TinyGL-compatible software renderer.
 *
 * Immediate-mode pipeline: glVertex* transforms into clip space with
 * the modelview and projection stacks, glEnd() rasterises the collected
 * primitive with a z-buffer and per-vertex colours. Everything is
 * rendered straight into the bound RGB32 buffer, so glFlush() has
 * nothing left to do.
 *
 * Compiled with SSE (see the GFX_OBJ rule in the Makefile); the rest of
 * the kernel stays -mno-sse so interrupt handlers never touch XMM.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tgl.h"

#define MAXV 512

struct vtx {
    float x, y, z;      /* screen space + depth in [0,1] */
    float r, g, b;      /* colours in [0,1] */
    bool ok;
};

static struct {
    uint32_t *color;
    float *depth;
    int w, h;

    float proj[16], mv[16];
    float proj_stack[8][16];
    float mv_stack[8][16];
    int proj_sp, mv_sp;
    int mode;           /* GL_MODELVIEW / GL_PROJECTION */

    int vp[4];
    float clear_r, clear_g, clear_b;
    float cur_r, cur_g, cur_b;

    bool depth_test, flat;

    GLenum prim;
    int nverts;
    float vin[MAXV][6];     /* pre-transform: xyz + rgb */

    struct vtx vout[MAXV];
} g;

/* ── small math ─────────────────────────────────────────────────────── */

static float fabsf_(float x) { return x < 0.0f ? -x : x; }

static float fsin_(float x)
{
    /* range-reduce to [-pi,pi] and use a 7th order Taylor series —
     * plenty for rotating a gear, no libm in the kernel */
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

static float fcos_(float x) { return fsin_(x + 1.57079633f); }

/* column-major 4x4 helpers (same layout as OpenGL) */
static void mat_identity(float m[16])
{
    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void mat_mul(float *dst, const float *a, const float *b)
{
    float t[16];
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++)
                s += a[k * 4 + row] * b[col * 4 + k];
            t[col * 4 + row] = s;
        }
    memcpy(dst, t, sizeof(t));
}

static float *cur_mat(void)
{
    return g.mode == GL_PROJECTION ? g.proj : g.mv;
}

static void mat_apply(const float m[16])
{
    mat_mul(cur_mat(), cur_mat(), m);
}

/* ── transform + rasteriser ─────────────────────────────────────────── */

static float pack_clamp(float c)
{
    return c < 0.0f ? 0.0f : (c > 1.0f ? 1.0f : c);
}

static void to_screen(const float in[6], int idx)
{
    struct vtx *v = &g.vout[idx];
    const float *m = g.mv, *p = g.proj;
    float ex = m[0] * in[0] + m[4] * in[1] + m[8] * in[2] + m[12];
    float ey = m[1] * in[0] + m[5] * in[1] + m[9] * in[2] + m[13];
    float ez = m[2] * in[0] + m[6] * in[1] + m[10] * in[2] + m[14];
    float ew = m[3] * in[0] + m[7] * in[1] + m[11] * in[2] + m[15];

    float cx = p[0] * ex + p[4] * ey + p[8] * ez + p[12] * ew;
    float cy = p[1] * ex + p[5] * ey + p[9] * ez + p[13] * ew;
    float cz = p[2] * ex + p[6] * ey + p[10] * ez + p[14] * ew;
    float cw = p[3] * ex + p[7] * ey + p[11] * ez + p[15] * ew;

    if (cw <= 1e-5f) {             /* behind the eye: drop the vertex */
        v->ok = false;
        return;
    }
    float inv = 1.0f / cw;
    float ndc_x = cx * inv, ndc_y = cy * inv, ndc_z = cz * inv;
    v->x = (float)g.vp[0] + (ndc_x * 0.5f + 0.5f) * (float)g.vp[2];
    v->y = (float)g.vp[1] + (0.5f - ndc_y * 0.5f) * (float)g.vp[3];
    v->z = ndc_z * 0.5f + 0.5f;
    v->r = pack_clamp(in[3]);
    v->g = pack_clamp(in[4]);
    v->b = pack_clamp(in[5]);
    v->ok = true;
}

static void put_px(int x, int y, float z, float r, float g_, float b)
{
    if (x < 0 || y < 0 || x >= g.w || y >= g.h)
        return;
    size_t i = (size_t)y * (size_t)g.w + (size_t)x;
    if (g.depth_test) {
        if (z < 0.0f || z > 1.0f || z >= g.depth[i])
            return;
        g.depth[i] = z;
    }
    uint32_t cr = (uint32_t)(pack_clamp(r) * 255.0f + 0.5f);
    uint32_t cg = (uint32_t)(pack_clamp(g_) * 255.0f + 0.5f);
    uint32_t cb = (uint32_t)(pack_clamp(b) * 255.0f + 0.5f);
    g.color[i] = 0xFF000000u | (cr << 16) | (cg << 8) | cb;
}

static void raster_tri(const struct vtx *a, const struct vtx *b,
                       const struct vtx *c)
{
    if (!a->ok || !b->ok || !c->ok)
        return;

    float area = (b->x - a->x) * (c->y - a->y) -
                 (b->y - a->y) * (c->x - a->x);
    if (fabsf_(area) < 1e-4f)
        return;
    float inv = 1.0f / area;

    int minx = (int)(a->x < b->x ? (a->x < c->x ? a->x : c->x)
                                 : (b->x < c->x ? b->x : c->x));
    int maxx = (int)(a->x > b->x ? (a->x > c->x ? a->x : c->x)
                                 : (b->x > c->x ? b->x : c->x));
    int miny = (int)(a->y < b->y ? (a->y < c->y ? a->y : c->y)
                                 : (b->y < c->y ? b->y : c->y));
    int maxy = (int)(a->y > b->y ? (a->y > c->y ? a->y : c->y)
                                 : (b->y > c->y ? b->y : c->y));
    if (minx < 0)
        minx = 0;
    if (miny < 0)
        miny = 0;
    if (maxx >= g.w)
        maxx = g.w - 1;
    if (maxy >= g.h)
        maxy = g.h - 1;
    if (minx > maxx || miny > maxy)
        return;

    /* edge functions are linear in x: step them instead of recomputing
     * both 2D cross products for every pixel */
    float dla_dx = (b->y - c->y) * inv;
    float dlb_dx = (c->y - a->y) * inv;
    bool uniform = g.flat ||
        (a->r == b->r && b->r == c->r && a->g == b->g &&
         b->g == c->g && a->b == b->b && b->b == c->b);

    for (int y = miny; y <= maxy; y++) {
        float py = (float)y + 0.5f;
        float px = (float)minx + 0.5f;
        float la = ((c->x - b->x) * (py - b->y) -
                    (c->y - b->y) * (px - b->x)) * inv;
        float lb = ((a->x - c->x) * (py - c->y) -
                    (a->y - c->y) * (px - c->x)) * inv;
        for (int x = minx; x <= maxx; x++, la += dla_dx, lb += dlb_dx) {
            float lc = 1.0f - la - lb;
            if (la < -1e-4f || lb < -1e-4f || lc < -1e-4f)
                continue;
            float z = la * a->z + lb * b->z + lc * c->z;
            float r, gg, bb;
            if (uniform) {
                r = a->r;
                gg = a->g;
                bb = a->b;
            } else {
                r = la * a->r + lb * b->r + lc * c->r;
                gg = la * a->g + lb * b->g + lc * c->g;
                bb = la * a->b + lb * b->b + lc * c->b;
            }
            put_px(x, y, z, r, gg, bb);
        }
    }
}

static void raster_line(const struct vtx *a, const struct vtx *b)
{
    if (!a->ok || !b->ok)
        return;
    int x0 = (int)a->x, y0 = (int)a->y;
    int x1 = (int)b->x, y1 = (int)b->y;
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        put_px(x0, y0, a->z, a->r, a->g, a->b);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* ── state ──────────────────────────────────────────────────────────── */

void tgl_bind(uint32_t *color, float *depth, int w, int h)
{
    memset(&g, 0, sizeof(g));
    g.color = color;
    g.depth = depth;
    g.w = w;
    g.h = h;
    mat_identity(g.proj);
    mat_identity(g.mv);
    g.mode = GL_MODELVIEW;
    g.vp[0] = 0;
    g.vp[1] = 0;
    g.vp[2] = w;
    g.vp[3] = h;
    g.cur_r = g.cur_g = g.cur_b = 1.0f;
    g.clear_r = g.clear_g = g.clear_b = 0.0f;
    g.depth_test = true;
    g.prim = 0;
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    g.vp[0] = x;
    g.vp[1] = y;
    g.vp[2] = w;
    g.vp[3] = h;
}

void glClearColor(GLclampf r, GLclampf gg, GLclampf b, GLclampf a)
{
    (void)a;
    g.clear_r = r;
    g.clear_g = gg;
    g.clear_b = b;
}

void glClear(GLbitfield mask)
{
    if (mask & GL_COLOR_BUFFER_BIT) {
        uint32_t cr = (uint32_t)(pack_clamp(g.clear_r) * 255.0f + 0.5f);
        uint32_t cg = (uint32_t)(pack_clamp(g.clear_g) * 255.0f + 0.5f);
        uint32_t cb = (uint32_t)(pack_clamp(g.clear_b) * 255.0f + 0.5f);
        uint32_t px = 0xFF000000u | (cr << 16) | (cg << 8) | cb;
        for (size_t i = 0; i < (size_t)g.w * (size_t)g.h; i++)
            g.color[i] = px;
    }
    if (mask & GL_DEPTH_BUFFER_BIT)
        for (size_t i = 0; i < (size_t)g.w * (size_t)g.h; i++)
            g.depth[i] = 1.0f;
}

void glFlush(void) { /* rendering is immediate */ }

void glEnable(GLenum cap)
{
    if (cap == GL_DEPTH_TEST)
        g.depth_test = true;
    /* GL_CULL_FACE is accepted but not implemented (double-sided) */
}

void glDisable(GLenum cap)
{
    if (cap == GL_DEPTH_TEST)
        g.depth_test = false;
}

void glShadeModel(GLenum mode) { g.flat = mode == GL_FLAT; }

/* ── matrices ───────────────────────────────────────────────────────── */

void glMatrixMode(GLenum mode)
{
    if (mode == GL_MODELVIEW || mode == GL_PROJECTION)
        g.mode = (int)mode;
}

void glLoadIdentity(void) { mat_identity(cur_mat()); }

void glPushMatrix(void)
{
    if (g.mode == GL_PROJECTION) {
        if (g.proj_sp < 7)
            memcpy(g.proj_stack[g.proj_sp++], g.proj, sizeof(g.proj));
    } else if (g.mv_sp < 7) {
        memcpy(g.mv_stack[g.mv_sp++], g.mv, sizeof(g.mv));
    }
}

void glPopMatrix(void)
{
    if (g.mode == GL_PROJECTION) {
        if (g.proj_sp > 0)
            memcpy(g.proj, g.proj_stack[--g.proj_sp], sizeof(g.proj));
    } else if (g.mv_sp > 0) {
        memcpy(g.mv, g.mv_stack[--g.mv_sp], sizeof(g.mv));
    }
}

void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
               GLdouble n, GLdouble f)
{
    float m[16];
    memset(m, 0, sizeof(m));
    m[0] = (float)(2.0 * n / (r - l));
    m[5] = (float)(2.0 * n / (t - b));
    m[8] = (float)((r + l) / (r - l));
    m[9] = (float)((t + b) / (t - b));
    m[10] = (float)(-(f + n) / (f - n));
    m[11] = -1.0f;
    m[14] = (float)(-2.0 * f * n / (f - n));
    mat_apply(m);
}

void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
             GLdouble n, GLdouble f)
{
    float m[16];
    memset(m, 0, sizeof(m));
    m[0] = (float)(2.0 / (r - l));
    m[5] = (float)(2.0 / (t - b));
    m[10] = (float)(-2.0 / (f - n));
    m[12] = (float)(-(r + l) / (r - l));
    m[13] = (float)(-(t + b) / (t - b));
    m[14] = (float)(-(f + n) / (f - n));
    m[15] = 1.0f;
    mat_apply(m);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    float m[16];
    mat_identity(m);
    m[12] = x;
    m[13] = y;
    m[14] = z;
    mat_apply(m);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    float m[16];
    mat_identity(m);
    m[0] = x;
    m[5] = y;
    m[10] = z;
    mat_apply(m);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    float len = fabsf_(x) + fabsf_(y) + fabsf_(z);
    if (len < 1e-6f)
        return;
    len = 1.0f / len; /* cheap normalisation, fine for axis-aligned */
    x *= len;
    y *= len;
    z *= len;
    float a = angle * (3.14159265f / 180.0f);
    float c = fcos_(a), s = fsin_(a), ic = 1.0f - c;

    float m[16];
    m[0] = c + x * x * ic;
    m[1] = y * x * ic + z * s;
    m[2] = z * x * ic - y * s;
    m[3] = 0.0f;
    m[4] = x * y * ic - z * s;
    m[5] = c + y * y * ic;
    m[6] = z * y * ic + x * s;
    m[7] = 0.0f;
    m[8] = x * z * ic + y * s;
    m[9] = y * z * ic - x * s;
    m[10] = c + z * z * ic;
    m[11] = 0.0f;
    m[12] = m[13] = m[14] = 0.0f;
    m[15] = 1.0f;
    mat_apply(m);
}

/* ── immediate mode ─────────────────────────────────────────────────── */

void glBegin(GLenum mode)
{
    g.prim = mode;
    g.nverts = 0;
}

void glEnd(void)
{
    int n = g.nverts;
    for (int i = 0; i < n; i++)
        to_screen(g.vin[i], i);

    switch (g.prim) {
    case GL_TRIANGLES:
        for (int i = 0; i + 2 < n; i += 3)
            raster_tri(&g.vout[i], &g.vout[i + 1], &g.vout[i + 2]);
        break;
    case GL_TRIANGLE_STRIP:
        for (int i = 2; i < n; i++)
            raster_tri(&g.vout[i - 2], &g.vout[i - 1], &g.vout[i]);
        break;
    case GL_TRIANGLE_FAN:
    case GL_POLYGON:
        for (int i = 1; i + 1 < n; i++)
            raster_tri(&g.vout[0], &g.vout[i], &g.vout[i + 1]);
        break;
    case GL_QUADS:
        for (int i = 0; i + 3 < n; i += 4) {
            raster_tri(&g.vout[i], &g.vout[i + 1], &g.vout[i + 2]);
            raster_tri(&g.vout[i], &g.vout[i + 2], &g.vout[i + 3]);
        }
        break;
    case GL_QUAD_STRIP:
        for (int i = 0; i + 3 < n; i += 2) {
            raster_tri(&g.vout[i], &g.vout[i + 1], &g.vout[i + 3]);
            raster_tri(&g.vout[i], &g.vout[i + 3], &g.vout[i + 2]);
        }
        break;
    case GL_LINES:
        for (int i = 0; i + 1 < n; i += 2)
            raster_line(&g.vout[i], &g.vout[i + 1]);
        break;
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
        for (int i = 0; i + 1 < n; i++)
            raster_line(&g.vout[i], &g.vout[i + 1]);
        if (g.prim == GL_LINE_LOOP && n > 2)
            raster_line(&g.vout[n - 1], &g.vout[0]);
        break;
    case GL_POINTS:
        for (int i = 0; i < n; i++)
            if (g.vout[i].ok)
                put_px((int)g.vout[i].x, (int)g.vout[i].y, g.vout[i].z,
                       g.vout[i].r, g.vout[i].g, g.vout[i].b);
        break;
    default:
        break;
    }
    g.prim = 0;
    g.nverts = 0;
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    if (g.nverts >= MAXV)
        return;
    g.vin[g.nverts][0] = x;
    g.vin[g.nverts][1] = y;
    g.vin[g.nverts][2] = z;
    g.vin[g.nverts][3] = g.cur_r;
    g.vin[g.nverts][4] = g.cur_g;
    g.vin[g.nverts][5] = g.cur_b;
    g.nverts++;
}

void glVertex2f(GLfloat x, GLfloat y) { glVertex3f(x, y, 0.0f); }

void glColor3f(GLfloat r, GLfloat gg, GLfloat b)
{
    g.cur_r = r;
    g.cur_g = gg;
    g.cur_b = b;
}

void glColor4f(GLfloat r, GLfloat gg, GLfloat b, GLfloat a)
{
    (void)a;
    glColor3f(r, gg, b);
}

void glNormal3f(GLfloat x, GLfloat y, GLfloat z)
{
    (void)x;
    (void)y;
    (void)z; /* no lighting yet */
}
