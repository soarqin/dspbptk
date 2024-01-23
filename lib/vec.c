#include <math.h>

#include "vec.h"

void vec3_cross(vec4 r, vec4 a, vec4 b) {
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

// void f64x3_cross(f64x4_t* c, f64x4_t* a, f64x4_t* b) {
//     c->x = a->y * b->z - a->z * b->y;
//     c->y = a->z * b->x - a->x * b->z;
//     c->z = a->x * b->y - a->y * b->x;
// }

void vec3_dot_mat3x3(vec4 r, vec4 a, mat4x4 m) {
    r[0] = a[0] * m[0][0] + a[1] * m[1][0] + a[2] * m[2][0];
    r[1] = a[0] * m[0][1] + a[1] * m[1][1] + a[2] * m[2][1];
    r[2] = a[0] * m[0][2] + a[1] * m[1][2] + a[2] * m[2][2];
}

// void f64x3_dot_f64mat_3x3(f64x4_t* b, f64x4_t* a, f64mat_4x4_t m) {
//     b->x = a->x * m[0].x + a->y * m[1].x + a->z * m[2].x;
//     b->y = a->x * m[0].y + a->y * m[1].y + a->z * m[2].y;
//     b->z = a->x * m[0].z + a->y * m[1].z + a->z * m[2].z;
// }

// void f64x3_normalize(f64x4_t* a) {
//     double length = sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
//     a->x /= length;
//     a->y /= length;
//     a->z /= length;
// }

void vec3_normalize(vec4 r) {
    double length = sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    r[0] /= length;
    r[1] /= length;
    r[2] /= length;
}