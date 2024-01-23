#include <math.h>

#include "vec.h"

void f64x3_cross(f64x4_t* c, f64x4_t* a, f64x4_t* b) {
    c->x = a->y * b->z - a->z * b->y;
    c->y = a->z * b->x - a->x * b->z;
    c->z = a->x * b->y - a->y * b->x;
}

void f64x3_dot_f64mat_3x3(f64x4_t* b, f64x4_t* a, f64mat_4x4_t m) {
    b->x = a->x * m[0].x + a->y * m[1].x + a->z * m[2].x;
    b->y = a->x * m[0].y + a->y * m[1].y + a->z * m[2].y;
    b->z = a->x * m[0].z + a->y * m[1].z + a->z * m[2].z;
}

void f64x3_normalize(f64x4_t* a) {
    double length = sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
    a->x /= length;
    a->y /= length;
    a->z /= length;
}