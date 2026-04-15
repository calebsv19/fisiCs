#include <stdio.h>
#include <string.h>

typedef struct InputState {
    float mouse_x;
    float mouse_y;
    int mouse_left_down;
    int mouse_left_pressed;
    int mouse_left_released;
} InputState;

typedef struct RenderDeriveFrame {
    InputState input;
    int frame_width;
    int frame_height;
    int wheel_y;
    unsigned frame_reasons;
} RenderDeriveFrame;

typedef struct RuntimeEnvelope {
    int guard_before;
    RenderDeriveFrame frame;
    int guard_after;
} RuntimeEnvelope;

static void derive_frame(RenderDeriveFrame *out,
                         const InputState *input,
                         int frame_width,
                         int frame_height,
                         int wheel_y,
                         unsigned frame_reasons) {
    RenderDeriveFrame tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.input = *input;
    tmp.frame_width = frame_width;
    tmp.frame_height = frame_height;
    tmp.wheel_y = wheel_y;
    tmp.frame_reasons = frame_reasons;
    *out = tmp;
}

static int checksum(const RuntimeEnvelope *env) {
    int sum = 0;
    sum += (int)(env->frame.input.mouse_x * 10.0f);
    sum += (int)(env->frame.input.mouse_y * 10.0f);
    sum += env->frame.input.mouse_left_down * 101;
    sum += env->frame.input.mouse_left_pressed * 211;
    sum += env->frame.input.mouse_left_released * 307;
    sum += env->frame.frame_width;
    sum += env->frame.frame_height;
    sum += env->frame.wheel_y;
    sum += (int)env->frame.frame_reasons;
    sum += env->guard_before;
    sum += env->guard_after;
    return sum;
}

int main(void) {
    RuntimeEnvelope env;
    InputState input = {1.5f, 2.5f, 0, 0, 0};
    int i;

    memset(&env, 0, sizeof(env));
    env.guard_before = 0x13579BDF;
    env.guard_after = 0x2468ACE0;

    for (i = 0; i < 512; ++i) {
        input.mouse_x += 0.25f;
        input.mouse_y -= 0.50f;
        input.mouse_left_down = (i & 1);
        input.mouse_left_pressed = ((i & 3) == 0);
        input.mouse_left_released = ((i & 7) == 0);

        derive_frame(&env.frame, &input, 640 + i, 360 + (i & 15), i - 256, (unsigned)(i * 3));

        if (env.guard_before != 0x13579BDF || env.guard_after != 0x2468ACE0) {
            puts("guard-corruption");
            return 1;
        }
        if (env.frame.input.mouse_left_down != input.mouse_left_down ||
            env.frame.input.mouse_left_pressed != input.mouse_left_pressed ||
            env.frame.input.mouse_left_released != input.mouse_left_released) {
            puts("input-copy-mismatch");
            return 1;
        }
    }

    printf("%d\n", checksum(&env));
    return 0;
}
