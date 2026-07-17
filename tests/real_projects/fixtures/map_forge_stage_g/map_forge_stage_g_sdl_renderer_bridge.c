#include "app/headless/app_headless_render_internal.h"
#include "render/renderer.h"

#include <string.h>

void renderer_set_backend(Renderer *renderer, RendererBackend backend) {
    if (renderer) renderer->backend = backend;
}

RendererBackend renderer_get_backend(const Renderer *renderer) {
    return renderer ? renderer->backend : RENDERER_BACKEND_SDL;
}

const char *renderer_backend_name(RendererBackend backend) {
    return backend == RENDERER_BACKEND_VULKAN ? "vulkan" : "sdl";
}

bool renderer_init(Renderer *renderer, SDL_Window *window, int width, int height) {
    (void)window;
    if (!renderer || width <= 0 || height <= 0) return false;
    renderer->width = width;
    renderer->height = height;
    renderer->backend = RENDERER_BACKEND_SDL;
    return true;
}

bool renderer_resize(Renderer *renderer, int width, int height) {
    if (!renderer || width <= 0 || height <= 0) return false;
    renderer->width = width;
    renderer->height = height;
    return true;
}

bool renderer_set_viewport(Renderer *renderer, int x, int y, int width, int height) {
    SDL_Rect rect;
    if (!renderer || width <= 0 || height <= 0) return false;
    renderer->viewport_enabled = true;
    renderer->viewport_x = x;
    renderer->viewport_y = y;
    renderer->width = width;
    renderer->height = height;
    rect = (SDL_Rect){x, y, width, height};
    if (renderer->sdl) SDL_RenderSetClipRect(renderer->sdl, &rect);
    return true;
}

void renderer_reset_viewport(Renderer *renderer) {
    if (!renderer) return;
    renderer->viewport_enabled = false;
    renderer->viewport_x = 0;
    renderer->viewport_y = 0;
    if (renderer->sdl) SDL_RenderSetClipRect(renderer->sdl, NULL);
}

void renderer_shutdown(Renderer *renderer) {
    if (!renderer) return;
    if (renderer->sdl) SDL_DestroyRenderer(renderer->sdl);
    memset(renderer, 0, sizeof(*renderer));
}

void renderer_begin_frame(Renderer *renderer) { (void)renderer; }

void renderer_clear(Renderer *renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!renderer || !renderer->sdl) return;
    SDL_SetRenderDrawColor(renderer->sdl, r, g, b, a);
    SDL_RenderClear(renderer->sdl);
}

void renderer_end_frame(Renderer *renderer) {
    if (renderer && renderer->sdl) SDL_RenderPresent(renderer->sdl);
}

void renderer_set_draw_color(Renderer *renderer, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (renderer && renderer->sdl) SDL_SetRenderDrawColor(renderer->sdl, r, g, b, a);
}

void renderer_draw_line(Renderer *renderer, float x0, float y0, float x1, float y1) {
    if (renderer && renderer->sdl) SDL_RenderDrawLineF(renderer->sdl, x0, y0, x1, y1);
}

void renderer_draw_lines(Renderer *renderer, const SDL_FPoint *points, int count) {
    if (renderer && renderer->sdl && points && count > 1) SDL_RenderDrawLinesF(renderer->sdl, points, count);
}

void renderer_draw_rect(Renderer *renderer, const SDL_FRect *rect) {
    if (renderer && renderer->sdl && rect) SDL_RenderDrawRectF(renderer->sdl, rect);
}

void renderer_fill_rect(Renderer *renderer, const SDL_FRect *rect) {
    if (renderer && renderer->sdl && rect) SDL_RenderFillRectF(renderer->sdl, rect);
}

void renderer_draw_geometry(Renderer *renderer,
                            const SDL_Vertex *vertices,
                            int num_vertices,
                            const int *indices,
                            int num_indices) {
    if (renderer && renderer->sdl && vertices && num_vertices > 0) {
        SDL_RenderGeometry(renderer->sdl, NULL, vertices, num_vertices, indices, num_indices);
    }
}

bool map_forge_headless_map_layers_init(AppState *app,
                                        const Renderer *renderer,
                                        const RegionInfo *region,
                                        int width,
                                        int height,
                                        const MapForgeHeadlessOutputConfig *output) {
    (void)app; (void)renderer; (void)region; (void)width; (void)height; (void)output;
    return false;
}

void map_forge_headless_map_layers_shutdown(AppState *app) { (void)app; }

bool map_forge_headless_map_layers_prepare_frame(AppState *app,
                                                 const Renderer *renderer,
                                                 const Camera *camera,
                                                 const MapForgeHeadlessOutputConfig *output) {
    (void)app; (void)renderer; (void)camera; (void)output;
    return false;
}

void map_forge_headless_map_layers_draw(AppState *app, AppVisibleTileRenderStats *out_stats) {
    (void)app;
    if (out_stats) memset(out_stats, 0, sizeof(*out_stats));
}

bool app_layer_runtime_enabled(const AppState *app, TileLayerKind kind) {
    (void)app; (void)kind;
    return false;
}

bool app_layer_active_runtime(const AppState *app, TileLayerKind kind) {
    (void)app; (void)kind;
    return false;
}

const char *layer_policy_band_label(TileZoomBand band) {
    (void)band;
    return "disabled";
}
