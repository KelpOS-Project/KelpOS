#ifndef KELP_TGL_H
#define KELP_TGL_H

#include <stdint.h>

/* tgl.h — TinyGL-compatible OpenGL 1.x subset for KelpOS.
 *
 * A small software renderer in the spirit of Fabrice Bellard's TinyGL:
 * fixed-function immediate mode, 4x4 float matrices, z-buffered
 * triangle rasterisation with Gouraud (or flat) colours.
 *
 * Not implemented (yet): textures, lighting, display lists, blending,
 * alpha test. The surface below is the subset the desktop GLGears demo
 * needs and the natural place to grow from.
 *
 * Rendering goes to a caller-provided RGB32 colour buffer plus a float
 * depth buffer (tgl_bind(), a TinyGL-style zbuffer extension). This
 * header is for the SSE-enabled graphics objects (kernel/gfx/); the
 * rest of the kernel must not call it directly.
 */

typedef float          GLfloat;
typedef double         GLdouble;
typedef int            GLint;
typedef unsigned int   GLenum;
typedef unsigned char  GLboolean;
typedef unsigned int   GLbitfield;
typedef int            GLsizei;
typedef float          GLclampf;
typedef unsigned char  GLubyte;

#define GL_FALSE 0
#define GL_TRUE  1

/* primitives */
#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_LINE_LOOP      0x0002
#define GL_LINE_STRIP     0x0003
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN   0x0006
#define GL_QUADS          0x0007
#define GL_QUAD_STRIP     0x0008
#define GL_POLYGON        0x0009

/* matrices */
#define GL_MODELVIEW  0x1700
#define GL_PROJECTION 0x1701

/* clear bits */
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100

/* caps and modes */
#define GL_DEPTH_TEST 0x0B71
#define GL_CULL_FACE  0x0B44
#define GL_SMOOTH     0x1D01
#define GL_FLAT       0x1D00

/* ── render target binding (TinyGL zbuffer-style extension) ─────────── */

/* bind an RGB32 colour buffer (0xFFRRGGBB) and a float depth buffer of
 * w*h entries; both must stay alive while bound */
void tgl_bind(uint32_t *color, float *depth, int w, int h);

/* ── clear and frame ────────────────────────────────────────────────── */

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h);
void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a);
void glClear(GLbitfield mask);
void glFlush(void);

/* ── state ──────────────────────────────────────────────────────────── */

void glEnable(GLenum cap);
void glDisable(GLenum cap);
void glShadeModel(GLenum mode);

/* ── matrix stack ───────────────────────────────────────────────────── */

void glMatrixMode(GLenum mode);
void glLoadIdentity(void);
void glPushMatrix(void);
void glPopMatrix(void);
void glFrustum(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
               GLdouble n, GLdouble f);
void glOrtho(GLdouble l, GLdouble r, GLdouble b, GLdouble t,
             GLdouble n, GLdouble f);
void glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void glScalef(GLfloat x, GLfloat y, GLfloat z);
void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

/* ── immediate mode ─────────────────────────────────────────────────── */

void glBegin(GLenum mode);
void glEnd(void);
void glVertex2f(GLfloat x, GLfloat y);
void glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void glColor3f(GLfloat r, GLfloat g, GLfloat b);
void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void glNormal3f(GLfloat x, GLfloat y, GLfloat z); /* stored, unused */

#endif
