#include <math.h>

#include "vec.h"

void vec3_cross(vec4 r, vec4 a, vec4 b) {
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

void vec3_dot_mat3x3(vec4 r, vec4 a, mat4x4 m) {
    r[0] = a[0] * m[0][0] + a[1] * m[1][0] + a[2] * m[2][0];
    r[1] = a[0] * m[0][1] + a[1] * m[1][1] + a[2] * m[2][1];
    r[2] = a[0] * m[0][2] + a[1] * m[1][2] + a[2] * m[2][2];
}

double vec3_length(vec4 a) {
    return sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
}

void vec3_normalize(vec4 r) {
    double length = vec3_length(r);
    r[0] /= length;
    r[1] /= length;
    r[2] /= length;
}
