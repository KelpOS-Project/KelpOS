#ifndef KELP_GFX_MATH_H
#define KELP_GFX_MATH_H

/* Minimal <math.h> for the graphics objects (kernel/gfx/).
 * TinyGL, GLU and stb_image only need the functions declared here;
 * the implementations live in math.c and are compiled with SSE. */

#define M_PI 3.14159265358979323846

double fabs(double x);
double floor(double x);
double sqrt(double x);
double sin(double x);
double cos(double x);
double tan(double x);
double pow(double x, double y);
double ldexp(double x, int exp);

#endif
