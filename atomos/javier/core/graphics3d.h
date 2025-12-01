/*
 * Javier 3D Graphics Engine
 * Software-based 3D rendering with z-buffering
 */

#ifndef _JAVIER_GRAPHICS3D_H
#define _JAVIER_GRAPHICS3D_H

#include "../../kernel/include/types.h"
#include "../../kernel/drivers/video/framebuffer.h"

/* Math constants */
#define PI          3.14159265358979323846f
#define DEG_TO_RAD  (PI / 180.0f)
#define RAD_TO_DEG  (180.0f / PI)

/* Fixed-point math for systems without FPU */
typedef int32_t fixed_t;
#define FIXED_SHIFT     16
#define FIXED_ONE       (1 << FIXED_SHIFT)
#define INT_TO_FIXED(x) ((fixed_t)((x) * FIXED_ONE))
#define FIXED_TO_INT(x) ((int)((x) / FIXED_ONE))
#define FLOAT_TO_FIXED(x) ((fixed_t)((x) * FIXED_ONE))
#define FIXED_TO_FLOAT(x) ((float)(x) / FIXED_ONE)
#define FIXED_MUL(a, b) ((fixed_t)(((int64_t)(a) * (b)) >> FIXED_SHIFT))
#define FIXED_DIV(a, b) ((fixed_t)(((int64_t)(a) << FIXED_SHIFT) / (b)))

/* 3D Vector */
typedef struct {
    fixed_t x, y, z;
} vec3_t;

/* 4D Vector (homogeneous coordinates) */
typedef struct {
    fixed_t x, y, z, w;
} vec4_t;

/* 2D Vector (screen space) */
typedef struct {
    int x, y;
} vec2i_t;

/* 4x4 Matrix */
typedef struct {
    fixed_t m[4][4];
} mat4_t;

/* Vertex with attributes */
typedef struct {
    vec3_t position;
    vec3_t normal;
    fixed_t u, v;       /* Texture coordinates */
    color_t color;
} vertex_t;

/* Triangle */
typedef struct {
    vertex_t v[3];
} triangle_t;

/* Mesh (collection of triangles) */
typedef struct {
    triangle_t *triangles;
    int triangle_count;
    vec3_t position;
    vec3_t rotation;
    vec3_t scale;
} mesh_t;

/* Camera */
typedef struct {
    vec3_t position;
    vec3_t rotation;    /* Euler angles */
    fixed_t fov;        /* Field of view */
    fixed_t near_plane;
    fixed_t far_plane;
    fixed_t aspect;
} camera_t;

/* Light types */
typedef enum {
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
    LIGHT_AMBIENT
} light_type_t;

/* Light source */
typedef struct {
    light_type_t type;
    vec3_t position;    /* Or direction for directional */
    color_t color;
    fixed_t intensity;
    fixed_t range;      /* For point lights */
} light_t;

/* Texture */
typedef struct {
    uint32_t *pixels;
    int width;
    int height;
} texture_t;

/* Material */
typedef struct {
    color_t diffuse;
    color_t specular;
    fixed_t shininess;
    texture_t *texture;
} material_t;

/* Render mode */
typedef enum {
    RENDER_WIREFRAME,
    RENDER_FLAT,
    RENDER_GOURAUD,
    RENDER_TEXTURED
} render_mode_t;

/* 3D Renderer state */
typedef struct {
    int width, height;
    uint32_t *framebuffer;
    fixed_t *zbuffer;
    camera_t camera;
    light_t lights[8];
    int light_count;
    mat4_t view_matrix;
    mat4_t proj_matrix;
    render_mode_t mode;
    color_t clear_color;
    color_t ambient_light;
    bool depth_test;
    bool backface_culling;
} renderer3d_t;

/* Initialization */
void g3d_init(int width, int height);
void g3d_shutdown(void);

/* Frame management */
void g3d_begin_frame(void);
void g3d_end_frame(void);
void g3d_clear(color_t color);
void g3d_clear_depth(void);

/* Camera */
void g3d_set_camera(camera_t *camera);
void g3d_camera_look_at(camera_t *camera, vec3_t target);

/* Rendering settings */
void g3d_set_render_mode(render_mode_t mode);
void g3d_enable_depth_test(bool enable);
void g3d_enable_backface_culling(bool enable);

/* Lighting */
void g3d_add_light(light_t *light);
void g3d_clear_lights(void);
void g3d_set_ambient(color_t color);

/* Drawing primitives */
void g3d_draw_line_3d(vec3_t p1, vec3_t p2, color_t color);
void g3d_draw_triangle(triangle_t *tri);
void g3d_draw_mesh(mesh_t *mesh, material_t *material);

/* Mesh creation */
mesh_t *g3d_create_cube(fixed_t size);
mesh_t *g3d_create_sphere(fixed_t radius, int segments);
mesh_t *g3d_create_plane(fixed_t width, fixed_t height);
mesh_t *g3d_create_pyramid(fixed_t base, fixed_t height);
void g3d_destroy_mesh(mesh_t *mesh);

/* Matrix operations */
mat4_t mat4_identity(void);
mat4_t mat4_multiply(mat4_t a, mat4_t b);
mat4_t mat4_translate(fixed_t x, fixed_t y, fixed_t z);
mat4_t mat4_rotate_x(fixed_t angle);
mat4_t mat4_rotate_y(fixed_t angle);
mat4_t mat4_rotate_z(fixed_t angle);
mat4_t mat4_scale(fixed_t x, fixed_t y, fixed_t z);
mat4_t mat4_perspective(fixed_t fov, fixed_t aspect, fixed_t near, fixed_t far);
mat4_t mat4_look_at(vec3_t eye, vec3_t target, vec3_t up);

/* Vector operations */
vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_mul(vec3_t v, fixed_t s);
fixed_t vec3_dot(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);
fixed_t vec3_length(vec3_t v);
vec3_t vec3_normalize(vec3_t v);
vec4_t mat4_mul_vec4(mat4_t m, vec4_t v);

/* Fixed-point trig functions */
fixed_t fixed_sin(fixed_t angle);
fixed_t fixed_cos(fixed_t angle);
fixed_t fixed_sqrt(fixed_t x);

#endif /* _JAVIER_GRAPHICS3D_H */
