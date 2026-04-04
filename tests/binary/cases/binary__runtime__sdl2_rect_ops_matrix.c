#include <SDL2/SDL.h>
#include <stdio.h>

int main(void) {
    SDL_Rect a = {0, 0, 10, 8};
    SDL_Rect b = {6, 3, 10, 6};
    SDL_Rect inter = {0, 0, 0, 0};
    SDL_Rect uni = {0, 0, 0, 0};
    SDL_bool has;
    int ok_inter;
    int ok_union;

    has = SDL_HasIntersection(&a, &b);
    if (!has || !SDL_IntersectRect(&a, &b, &inter)) {
        return 1;
    }
    SDL_UnionRect(&a, &b, &uni);

    ok_inter = (inter.x == 6 && inter.y == 3 && inter.w == 4 && inter.h == 5) ? 1 : 0;
    ok_union = (uni.x == 0 && uni.y == 0 && uni.w == 16 && uni.h == 9) ? 1 : 0;
    if (!ok_inter || !ok_union) {
        return 2;
    }

    printf("sdl_rect_ops_matrix_ok inter=%d,%d,%d,%d union=%d,%d,%d,%d\n",
           inter.x, inter.y, inter.w, inter.h,
           uni.x, uni.y, uni.w, uni.h);
    return 0;
}
