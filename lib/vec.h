typedef float f32_t;
typedef double f64_t;

typedef struct {
    f64_t x;
    f64_t y;
    f64_t z;
    f64_t w;
} f64x4_t;

typedef f64x4_t f64mat_4x4_t[4];

void f64x3_cross(f64x4_t* c, f64x4_t* a, f64x4_t* b);
void f64x3_dot_f64mat_3x3(f64x4_t* b, f64x4_t* a, f64mat_4x4_t m);
void f64x3_normalize(f64x4_t* a);