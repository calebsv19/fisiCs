#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Rect rect = {0, 0, 10, 10};
    int x1 = -5;
    int y1 = 5;
    int x2 = 5;
    int y2 = 5;
    int x3 = 2;
    int y3 = -4;
    int x4 = 2;
    int y4 = 4;
    int x5 = -3;
    int y5 = -2;
    int x6 = -1;
    int y6 = -2;
    int miss_ok;

    if (!SDL_IntersectRectAndLine(&rect, &x1, &y1, &x2, &y2)) {
        return 1;
    }
    if (x1 != 0 || y1 != 5 || x2 != 5 || y2 != 5) {
        return 2;
    }

    if (!SDL_IntersectRectAndLine(&rect, &x3, &y3, &x4, &y4)) {
        return 3;
    }
    if (x3 != 2 || y3 != 0 || x4 != 2 || y4 != 4) {
        return 4;
    }

    miss_ok = SDL_IntersectRectAndLine(&rect, &x5, &y5, &x6, &y6) ? 0 : 1;
    if (!miss_ok) {
        return 5;
    }

    printf("sdl_rect_line_clip_matrix_ok h=%d,%d,%d,%d v=%d,%d,%d,%d miss=%d\n",
           x1, y1, x2, y2,
           x3, y3, x4, y4, miss_ok);
    return 0;
}
