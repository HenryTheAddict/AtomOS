/*
 * Javier 3D Graphics Engine Implementation
 * Full software 3D renderer with lighting
 */

#include "graphics3d.h"
#include "../../kernel/include/kernel.h"
#include "../../kernel/mm/heap.h"

/* Global renderer state */
static renderer3d_t g_renderer;

/* Sin/Cos lookup tables (256 entries for 0-360 degrees) */
static fixed_t sin_table[256];
static fixed_t cos_table[256];
static bool tables_initialized = false;

/* Initialize trig tables */
static void init_trig_tables(void) {
    if (tables_initialized) return;
    
    /* Pre-computed values (would normally be calculated) */
    for (int i = 0; i < 256; i++) {
        /* Approximate sin/cos using polynomial */
        int angle = (i * 360) / 256;
        int normalized = angle % 360;
        if (normalized < 0) normalized += 360;
        
        /* Simple approximation */
        int quadrant = normalized / 90;
        int phase = normalized % 90;
        
        fixed_t val;
        if (phase <= 45) {
            val = (phase * FIXED_ONE) / 57;  /* Approximation */
        } else {
            val = FIXED_ONE - ((90 - phase) * FIXED_ONE) / 57;
        }
        
        switch (quadrant) {
            case 0: sin_table[i] = val; break;
            case 1: sin_table[i] = FIXED_ONE - val + FIXED_ONE; break;
            case 2: sin_table[i] = -val; break;
            case 3: sin_table[i] = val - FIXED_ONE; break;
        }
        
        cos_table[i] = sin_table[(i + 64) % 256];
    }
    
    tables_initialized = true;
}

/* Fixed-point trig */
fixed_t fixed_sin(fixed_t angle) {
    int idx = (FIXED_TO_INT(angle) % 360) * 256 / 360;
    if (idx < 0) idx += 256;
    return sin_table[idx % 256];
}

fixed_t fixed_cos(fixed_t angle) {
    int idx = (FIXED_TO_INT(angle) % 360) * 256 / 360;
    if (idx < 0) idx += 256;
    return cos_table[idx % 256];
}

/* Integer square root approximation */
fixed_t fixed_sqrt(fixed_t x) {
    if (x <= 0) return 0;
    
    fixed_t result = x;
    fixed_t temp;
    
    /* Newton's method */
    for (int i = 0; i < 8; i++) {
        temp = FIXED_DIV(x, result);
        result = (result + temp) >> 1;
    }
    
    return result;
}

/* Vector operations */
vec3_t vec3_add(vec3_t a, vec3_t b) {
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3_t vec3_mul(vec3_t v, fixed_t s) {
    return (vec3_t){FIXED_MUL(v.x, s), FIXED_MUL(v.y, s), FIXED_MUL(v.z, s)};
}

fixed_t vec3_dot(vec3_t a, vec3_t b) {
    return FIXED_MUL(a.x, b.x) + FIXED_MUL(a.y, b.y) + FIXED_MUL(a.z, b.z);
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    return (vec3_t){
        FIXED_MUL(a.y, b.z) - FIXED_MUL(a.z, b.y),
        FIXED_MUL(a.z, b.x) - FIXED_MUL(a.x, b.z),
        FIXED_MUL(a.x, b.y) - FIXED_MUL(a.y, b.x)
    };
}

fixed_t vec3_length(vec3_t v) {
    fixed_t len_sq = vec3_dot(v, v);
    return fixed_sqrt(len_sq);
}

vec3_t vec3_normalize(vec3_t v) {
    fixed_t len = vec3_length(v);
    if (len == 0) return (vec3_t){0, 0, 0};
    return (vec3_t){
        FIXED_DIV(v.x, len),
        FIXED_DIV(v.y, len),
        FIXED_DIV(v.z, len)
    };
}

/* Matrix operations */
mat4_t mat4_identity(void) {
    mat4_t m = {{{0}}};
    m.m[0][0] = FIXED_ONE;
    m.m[1][1] = FIXED_ONE;
    m.m[2][2] = FIXED_ONE;
    m.m[3][3] = FIXED_ONE;
    return m;
}

mat4_t mat4_multiply(mat4_t a, mat4_t b) {
    mat4_t result = {{{0}}};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i][j] += FIXED_MUL(a.m[i][k], b.m[k][j]);
            }
        }
    }
    return result;
}

mat4_t mat4_translate(fixed_t x, fixed_t y, fixed_t z) {
    mat4_t m = mat4_identity();
    m.m[0][3] = x;
    m.m[1][3] = y;
    m.m[2][3] = z;
    return m;
}

mat4_t mat4_rotate_x(fixed_t angle) {
    mat4_t m = mat4_identity();
    fixed_t c = fixed_cos(angle);
    fixed_t s = fixed_sin(angle);
    m.m[1][1] = c;
    m.m[1][2] = -s;
    m.m[2][1] = s;
    m.m[2][2] = c;
    return m;
}

mat4_t mat4_rotate_y(fixed_t angle) {
    mat4_t m = mat4_identity();
    fixed_t c = fixed_cos(angle);
    fixed_t s = fixed_sin(angle);
    m.m[0][0] = c;
    m.m[0][2] = s;
    m.m[2][0] = -s;
    m.m[2][2] = c;
    return m;
}

mat4_t mat4_rotate_z(fixed_t angle) {
    mat4_t m = mat4_identity();
    fixed_t c = fixed_cos(angle);
    fixed_t s = fixed_sin(angle);
    m.m[0][0] = c;
    m.m[0][1] = -s;
    m.m[1][0] = s;
    m.m[1][1] = c;
    return m;
}

mat4_t mat4_scale(fixed_t x, fixed_t y, fixed_t z) {
    mat4_t m = mat4_identity();
    m.m[0][0] = x;
    m.m[1][1] = y;
    m.m[2][2] = z;
    return m;
}

mat4_t mat4_perspective(fixed_t fov, fixed_t aspect, fixed_t near, fixed_t far) {
    mat4_t m = {{{0}}};
    fixed_t tan_half_fov = FIXED_DIV(fixed_sin(fov / 2), fixed_cos(fov / 2));
    
    m.m[0][0] = FIXED_DIV(FIXED_ONE, FIXED_MUL(aspect, tan_half_fov));
    m.m[1][1] = FIXED_DIV(FIXED_ONE, tan_half_fov);
    m.m[2][2] = -FIXED_DIV(far + near, far - near);
    m.m[2][3] = -FIXED_DIV(FIXED_MUL(INT_TO_FIXED(2), FIXED_MUL(far, near)), far - near);
    m.m[3][2] = -FIXED_ONE;
    
    return m;
}

vec4_t mat4_mul_vec4(mat4_t m, vec4_t v) {
    return (vec4_t){
        FIXED_MUL(m.m[0][0], v.x) + FIXED_MUL(m.m[0][1], v.y) + 
        FIXED_MUL(m.m[0][2], v.z) + FIXED_MUL(m.m[0][3], v.w),
        FIXED_MUL(m.m[1][0], v.x) + FIXED_MUL(m.m[1][1], v.y) + 
        FIXED_MUL(m.m[1][2], v.z) + FIXED_MUL(m.m[1][3], v.w),
        FIXED_MUL(m.m[2][0], v.x) + FIXED_MUL(m.m[2][1], v.y) + 
        FIXED_MUL(m.m[2][2], v.z) + FIXED_MUL(m.m[2][3], v.w),
        FIXED_MUL(m.m[3][0], v.x) + FIXED_MUL(m.m[3][1], v.y) + 
        FIXED_MUL(m.m[3][2], v.z) + FIXED_MUL(m.m[3][3], v.w)
    };
}

/* Initialize 3D renderer */
void g3d_init(int width, int height) {
    init_trig_tables();
    
    g_renderer.width = width;
    g_renderer.height = height;
    g_renderer.framebuffer = fb_get_back_buffer();
    g_renderer.zbuffer = (fixed_t *)kmalloc(width * height * sizeof(fixed_t));
    g_renderer.mode = RENDER_FLAT;
    g_renderer.depth_test = true;
    g_renderer.backface_culling = true;
    g_renderer.clear_color = COLOR_BG_DARK;
    g_renderer.ambient_light = RGB(30, 30, 30);
    g_renderer.light_count = 0;
    
    /* Default camera */
    g_renderer.camera.position = (vec3_t){0, 0, INT_TO_FIXED(-5)};
    g_renderer.camera.rotation = (vec3_t){0, 0, 0};
    g_renderer.camera.fov = INT_TO_FIXED(60);
    g_renderer.camera.near_plane = INT_TO_FIXED(1) / 10;
    g_renderer.camera.far_plane = INT_TO_FIXED(1000);
    g_renderer.camera.aspect = FIXED_DIV(INT_TO_FIXED(width), INT_TO_FIXED(height));
    
    kprintf("3D Graphics engine initialized (%dx%d)\n", width, height);
}

void g3d_shutdown(void) {
    if (g_renderer.zbuffer) {
        kfree(g_renderer.zbuffer);
        g_renderer.zbuffer = NULL;
    }
}

void g3d_begin_frame(void) {
    /* Calculate view and projection matrices */
    g_renderer.proj_matrix = mat4_perspective(
        g_renderer.camera.fov,
        g_renderer.camera.aspect,
        g_renderer.camera.near_plane,
        g_renderer.camera.far_plane
    );
    
    /* Simple view matrix from camera position/rotation */
    mat4_t rot_x = mat4_rotate_x(-g_renderer.camera.rotation.x);
    mat4_t rot_y = mat4_rotate_y(-g_renderer.camera.rotation.y);
    mat4_t trans = mat4_translate(
        -g_renderer.camera.position.x,
        -g_renderer.camera.position.y,
        -g_renderer.camera.position.z
    );
    g_renderer.view_matrix = mat4_multiply(rot_x, mat4_multiply(rot_y, trans));
}

void g3d_end_frame(void) {
    /* Frame complete */
}

void g3d_clear(color_t color) {
    for (int i = 0; i < g_renderer.width * g_renderer.height; i++) {
        g_renderer.framebuffer[i] = color;
    }
}

void g3d_clear_depth(void) {
    for (int i = 0; i < g_renderer.width * g_renderer.height; i++) {
        g_renderer.zbuffer[i] = INT_TO_FIXED(10000);
    }
}

void g3d_set_camera(camera_t *camera) {
    g_renderer.camera = *camera;
}

void g3d_set_render_mode(render_mode_t mode) {
    g_renderer.mode = mode;
}

void g3d_enable_depth_test(bool enable) {
    g_renderer.depth_test = enable;
}

void g3d_enable_backface_culling(bool enable) {
    g_renderer.backface_culling = enable;
}

void g3d_add_light(light_t *light) {
    if (g_renderer.light_count < 8) {
        g_renderer.lights[g_renderer.light_count++] = *light;
    }
}

void g3d_clear_lights(void) {
    g_renderer.light_count = 0;
}

void g3d_set_ambient(color_t color) {
    g_renderer.ambient_light = color;
}

/* Project 3D point to 2D screen coordinates */
static bool project_vertex(vec3_t world, vec2i_t *screen, fixed_t *depth) {
    /* Apply view-projection matrix */
    vec4_t clip = mat4_mul_vec4(g_renderer.view_matrix, 
                                 (vec4_t){world.x, world.y, world.z, FIXED_ONE});
    clip = mat4_mul_vec4(g_renderer.proj_matrix, clip);
    
    /* Perspective divide */
    if (clip.w == 0) return false;
    
    fixed_t inv_w = FIXED_DIV(FIXED_ONE, clip.w);
    fixed_t ndc_x = FIXED_MUL(clip.x, inv_w);
    fixed_t ndc_y = FIXED_MUL(clip.y, inv_w);
    fixed_t ndc_z = FIXED_MUL(clip.z, inv_w);
    
    /* Check if behind camera */
    if (ndc_z < -FIXED_ONE || ndc_z > FIXED_ONE) return false;
    
    /* Convert to screen coordinates */
    screen->x = FIXED_TO_INT((ndc_x + FIXED_ONE) * g_renderer.width / 2);
    screen->y = FIXED_TO_INT((FIXED_ONE - ndc_y) * g_renderer.height / 2);
    *depth = ndc_z;
    
    return true;
}

/* Draw 3D line */
void g3d_draw_line_3d(vec3_t p1, vec3_t p2, color_t color) {
    vec2i_t s1, s2;
    fixed_t d1, d2;
    
    if (!project_vertex(p1, &s1, &d1)) return;
    if (!project_vertex(p2, &s2, &d2)) return;
    
    fb_draw_line(s1.x, s1.y, s2.x, s2.y, color);
}

/* Draw filled triangle with flat shading */
static void draw_flat_triangle(vec2i_t v0, vec2i_t v1, vec2i_t v2,
                               fixed_t z0, fixed_t z1, fixed_t z2,
                               color_t color) {
    /* Sort vertices by y coordinate */
    if (v0.y > v1.y) { vec2i_t t = v0; v0 = v1; v1 = t; fixed_t tz = z0; z0 = z1; z1 = tz; }
    if (v1.y > v2.y) { vec2i_t t = v1; v1 = v2; v2 = t; fixed_t tz = z1; z1 = z2; z2 = tz; }
    if (v0.y > v1.y) { vec2i_t t = v0; v0 = v1; v1 = t; fixed_t tz = z0; z0 = z1; z1 = tz; }
    
    int total_height = v2.y - v0.y;
    if (total_height == 0) return;
    
    for (int y = v0.y; y <= v2.y; y++) {
        bool second_half = y > v1.y || v1.y == v0.y;
        int segment_height = second_half ? v2.y - v1.y : v1.y - v0.y;
        if (segment_height == 0) continue;
        
        int alpha = (y - v0.y);
        int beta = second_half ? (y - v1.y) : (y - v0.y);
        
        int ax = v0.x + (v2.x - v0.x) * alpha / total_height;
        int bx;
        if (second_half) {
            bx = v1.x + (v2.x - v1.x) * beta / segment_height;
        } else {
            bx = v0.x + (v1.x - v0.x) * beta / segment_height;
        }
        
        if (ax > bx) { int t = ax; ax = bx; bx = t; }
        
        for (int x = ax; x <= bx; x++) {
            if (x >= 0 && x < g_renderer.width && y >= 0 && y < g_renderer.height) {
                int idx = y * g_renderer.width + x;
                
                /* Approximate depth */
                fixed_t z = (z0 + z1 + z2) / 3;
                
                if (!g_renderer.depth_test || z < g_renderer.zbuffer[idx]) {
                    g_renderer.framebuffer[idx] = color;
                    g_renderer.zbuffer[idx] = z;
                }
            }
        }
    }
}

/* Calculate lighting for a surface */
static color_t calculate_lighting(vec3_t normal, vec3_t position, color_t base_color) {
    int r = GET_R(g_renderer.ambient_light);
    int g = GET_G(g_renderer.ambient_light);
    int b = GET_B(g_renderer.ambient_light);
    
    for (int i = 0; i < g_renderer.light_count; i++) {
        light_t *light = &g_renderer.lights[i];
        
        vec3_t light_dir;
        fixed_t attenuation = FIXED_ONE;
        
        if (light->type == LIGHT_DIRECTIONAL) {
            light_dir = vec3_normalize(light->position);
        } else if (light->type == LIGHT_POINT) {
            vec3_t to_light = vec3_sub(light->position, position);
            fixed_t dist = vec3_length(to_light);
            light_dir = vec3_normalize(to_light);
            attenuation = FIXED_DIV(light->range, light->range + dist);
        } else {
            continue;
        }
        
        /* Diffuse lighting */
        fixed_t diff = vec3_dot(normal, light_dir);
        if (diff < 0) diff = 0;
        diff = FIXED_MUL(diff, FIXED_MUL(light->intensity, attenuation));
        
        r += FIXED_TO_INT(FIXED_MUL(INT_TO_FIXED(GET_R(light->color)), diff));
        g += FIXED_TO_INT(FIXED_MUL(INT_TO_FIXED(GET_G(light->color)), diff));
        b += FIXED_TO_INT(FIXED_MUL(INT_TO_FIXED(GET_B(light->color)), diff));
    }
    
    /* Apply to base color */
    r = (r * GET_R(base_color)) / 255;
    g = (g * GET_G(base_color)) / 255;
    b = (b * GET_B(base_color)) / 255;
    
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    
    return RGB(r, g, b);
}

/* Draw a single triangle */
void g3d_draw_triangle(triangle_t *tri) {
    vec2i_t screen[3];
    fixed_t depth[3];
    
    /* Project all vertices */
    for (int i = 0; i < 3; i++) {
        if (!project_vertex(tri->v[i].position, &screen[i], &depth[i])) {
            return;  /* Triangle not visible */
        }
    }
    
    /* Backface culling */
    if (g_renderer.backface_culling) {
        int cross = (screen[1].x - screen[0].x) * (screen[2].y - screen[0].y) -
                    (screen[1].y - screen[0].y) * (screen[2].x - screen[0].x);
        if (cross < 0) return;
    }
    
    /* Calculate face normal for lighting */
    vec3_t edge1 = vec3_sub(tri->v[1].position, tri->v[0].position);
    vec3_t edge2 = vec3_sub(tri->v[2].position, tri->v[0].position);
    vec3_t normal = vec3_normalize(vec3_cross(edge1, edge2));
    
    /* Average position for lighting */
    vec3_t center = {
        (tri->v[0].position.x + tri->v[1].position.x + tri->v[2].position.x) / 3,
        (tri->v[0].position.y + tri->v[1].position.y + tri->v[2].position.y) / 3,
        (tri->v[0].position.z + tri->v[1].position.z + tri->v[2].position.z) / 3
    };
    
    color_t color = calculate_lighting(normal, center, tri->v[0].color);
    
    switch (g_renderer.mode) {
        case RENDER_WIREFRAME:
            fb_draw_line(screen[0].x, screen[0].y, screen[1].x, screen[1].y, color);
            fb_draw_line(screen[1].x, screen[1].y, screen[2].x, screen[2].y, color);
            fb_draw_line(screen[2].x, screen[2].y, screen[0].x, screen[0].y, color);
            break;
            
        case RENDER_FLAT:
        case RENDER_GOURAUD:
        case RENDER_TEXTURED:
            draw_flat_triangle(screen[0], screen[1], screen[2],
                              depth[0], depth[1], depth[2], color);
            break;
    }
}

/* Draw a mesh */
void g3d_draw_mesh(mesh_t *mesh, material_t *material) {
    /* Build model matrix */
    mat4_t trans = mat4_translate(mesh->position.x, mesh->position.y, mesh->position.z);
    mat4_t rot_x = mat4_rotate_x(mesh->rotation.x);
    mat4_t rot_y = mat4_rotate_y(mesh->rotation.y);
    mat4_t rot_z = mat4_rotate_z(mesh->rotation.z);
    mat4_t scale = mat4_scale(mesh->scale.x, mesh->scale.y, mesh->scale.z);
    
    mat4_t model = mat4_multiply(trans, mat4_multiply(rot_z, 
                   mat4_multiply(rot_y, mat4_multiply(rot_x, scale))));
    
    /* Transform and draw each triangle */
    for (int i = 0; i < mesh->triangle_count; i++) {
        triangle_t transformed = mesh->triangles[i];
        
        for (int j = 0; j < 3; j++) {
            vec4_t pos = mat4_mul_vec4(model, (vec4_t){
                transformed.v[j].position.x,
                transformed.v[j].position.y,
                transformed.v[j].position.z,
                FIXED_ONE
            });
            transformed.v[j].position = (vec3_t){pos.x, pos.y, pos.z};
            
            if (material) {
                transformed.v[j].color = material->diffuse;
            }
        }
        
        g3d_draw_triangle(&transformed);
    }
}

/* Create a cube mesh */
mesh_t *g3d_create_cube(fixed_t size) {
    mesh_t *mesh = (mesh_t *)kmalloc(sizeof(mesh_t));
    mesh->triangle_count = 12;  /* 6 faces * 2 triangles */
    mesh->triangles = (triangle_t *)kmalloc(12 * sizeof(triangle_t));
    mesh->position = (vec3_t){0, 0, 0};
    mesh->rotation = (vec3_t){0, 0, 0};
    mesh->scale = (vec3_t){FIXED_ONE, FIXED_ONE, FIXED_ONE};
    
    fixed_t h = size / 2;
    
    /* Define cube vertices */
    vec3_t verts[8] = {
        {-h, -h, -h}, {h, -h, -h}, {h, h, -h}, {-h, h, -h},
        {-h, -h, h}, {h, -h, h}, {h, h, h}, {-h, h, h}
    };
    
    color_t colors[6] = {
        RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255),
        RGB(255, 255, 0), RGB(255, 0, 255), RGB(0, 255, 255)
    };
    
    /* Define faces (2 triangles each) */
    int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {4, 5, 1, 0}, {3, 2, 6, 7}
    };
    
    int tri = 0;
    for (int f = 0; f < 6; f++) {
        /* First triangle */
        mesh->triangles[tri].v[0].position = verts[faces[f][0]];
        mesh->triangles[tri].v[1].position = verts[faces[f][1]];
        mesh->triangles[tri].v[2].position = verts[faces[f][2]];
        for (int v = 0; v < 3; v++) mesh->triangles[tri].v[v].color = colors[f];
        tri++;
        
        /* Second triangle */
        mesh->triangles[tri].v[0].position = verts[faces[f][0]];
        mesh->triangles[tri].v[1].position = verts[faces[f][2]];
        mesh->triangles[tri].v[2].position = verts[faces[f][3]];
        for (int v = 0; v < 3; v++) mesh->triangles[tri].v[v].color = colors[f];
        tri++;
    }
    
    return mesh;
}

/* Create pyramid mesh */
mesh_t *g3d_create_pyramid(fixed_t base, fixed_t height) {
    mesh_t *mesh = (mesh_t *)kmalloc(sizeof(mesh_t));
    mesh->triangle_count = 6;  /* 4 sides + 2 for base */
    mesh->triangles = (triangle_t *)kmalloc(6 * sizeof(triangle_t));
    mesh->position = (vec3_t){0, 0, 0};
    mesh->rotation = (vec3_t){0, 0, 0};
    mesh->scale = (vec3_t){FIXED_ONE, FIXED_ONE, FIXED_ONE};
    
    fixed_t h = base / 2;
    
    vec3_t apex = {0, height, 0};
    vec3_t base_verts[4] = {
        {-h, 0, -h}, {h, 0, -h}, {h, 0, h}, {-h, 0, h}
    };
    
    /* Side faces */
    for (int i = 0; i < 4; i++) {
        mesh->triangles[i].v[0].position = apex;
        mesh->triangles[i].v[1].position = base_verts[i];
        mesh->triangles[i].v[2].position = base_verts[(i + 1) % 4];
        
        color_t c = RGB(200 - i * 30, 100 + i * 20, 50 + i * 40);
        for (int v = 0; v < 3; v++) mesh->triangles[i].v[v].color = c;
    }
    
    /* Base */
    mesh->triangles[4].v[0].position = base_verts[0];
    mesh->triangles[4].v[1].position = base_verts[2];
    mesh->triangles[4].v[2].position = base_verts[1];
    mesh->triangles[5].v[0].position = base_verts[0];
    mesh->triangles[5].v[1].position = base_verts[3];
    mesh->triangles[5].v[2].position = base_verts[2];
    for (int v = 0; v < 3; v++) {
        mesh->triangles[4].v[v].color = RGB(100, 100, 100);
        mesh->triangles[5].v[v].color = RGB(100, 100, 100);
    }
    
    return mesh;
}

void g3d_destroy_mesh(mesh_t *mesh) {
    if (mesh) {
        if (mesh->triangles) kfree(mesh->triangles);
        kfree(mesh);
    }
}
