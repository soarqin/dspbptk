typedef float f32_t;
typedef double f64_t;

typedef f64_t vec4[4];
typedef f64_t mat4x4[4][4];

void vec3_cross(vec4 c, vec4 a, vec4 b);
void vec3_dot_mat3x3(vec4 b, vec4 a, mat4x4 m);
void vec3_normalize(vec4 a);