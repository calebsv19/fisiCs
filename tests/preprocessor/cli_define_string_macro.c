#ifndef VK_RENDERER_SHADER_ROOT
#error "VK_RENDERER_SHADER_ROOT must be defined"
#endif

const char* g_shader_root = VK_RENDERER_SHADER_ROOT;

int main(void) {
    return g_shader_root[0] == '\0';
}
