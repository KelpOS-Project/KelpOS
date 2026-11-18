/* kernel/gfx/compat/math.c — the handful of libm functions TinyGL,
 * GLU and stb_image use. Compiled with SSE like the rest of kernel/gfx.
 *
 * Accuracy is far better than a 320x240 software renderer needs; the
 * point is to stay freestanding and avoid a libm dependency.
 */
#include <stdint.h>
#include "math.h"

double fabs(double x)
{
    union {
        double d;
        uint64_t u;
    } v;
    v.d = x;
    v.u &= 0x7FFFFFFFFFFFFFFFULL;
    return v.d;
}

double floor(double x)
{
    if (x != x) /* NaN */
        return x;
    if (x >= 9223372036854775807.0 || x <= -9223372036854775807.0)
        return x;
    int64_t i = (int64_t)x;
    double d = (double)i;
    return d > x ? d - 1.0 : d;
}

/* x87 square root: exact, and the FPU is enabled in kmain() */
double sqrt(double x)
{
    if (x <= 0.0)
        return 0.0;
    double r;
    __asm__("fsqrt" : "=t"(r) : "0"(x));
    return r;
}

static double reduce_2pi(double x)
{
    /* range reduction without fmod: angles here are small (degrees
     * converted to radians, a few hundred at most) */
    const double two_pi = 6.28318530717958647692;
    int64_t k = (int64_t)(x / two_pi + (x < 0.0 ? -0.5 : 0.5));
    return x - (double)k * two_pi;
}

double sin(double x)
{
    x = reduce_2pi(x);
    double x2 = x * x;
    /* Taylor series to x^15 */
    return x * (1.0 + x2 * (-1.0 / 6.0 + x2 * (1.0 / 120.0 +
           x2 * (-1.0 / 5040.0 + x2 * (1.0 / 362880.0 +
           x2 * (-1.0 / 39916800.0 + x2 * (1.0 / 6227020800.0)))))));
}

double cos(double x)
{
    return sin(x + 1.57079632679489661923);
}

double tan(double x)
{
    return sin(x) / cos(x);
}

static double exp_(double x)
{
    /* x = k*ln2 + r, exp(x) = 2^k * exp(r) */
    const double ln2 = 0.69314718055994530942;
    int64_t k = (int64_t)(x / ln2 + (x < 0.0 ? -0.5 : 0.5));
    double r = x - (double)k * ln2;
    double term = 1.0, sum = 1.0;
    for (int i = 1; i < 20; i++) {
        term *= r / (double)i;
        sum += term;
    }
    return ldexp(sum, (int)k);
}

static double log_(double x)
{
    if (x <= 0.0)
        return -1e30;
    /* scale into [1,2) so the atanh series converges quickly */
    int e = 0;
    while (x >= 2.0) {
        x *= 0.5;
        e++;
    }
    while (x < 1.0) {
        x *= 2.0;
        e--;
    }
    double z = (x - 1.0) / (x + 1.0);
    double z2 = z * z;
    double term = z, sum = 0.0;
    for (int i = 1; i < 40; i += 2) {
        sum += term / (double)i;
        term *= z2;
    }
    return 2.0 * sum + (double)e * 0.69314718055994530942;
}

double pow(double x, double y)
{
    if (x <= 0.0)
        return 0.0; /* the renderer only uses pow on [0,1] intensities */
    return exp_(y * log_(x));
}

double ldexp(double x, int e)
{
    while (e > 0) {
        x *= 2.0;
        e--;
    }
    while (e < 0) {
        x *= 0.5;
        e++;
    }
    return x;
}
