// Regression: a function declared with an array-typedef parameter is
// compatible with a callback typedef using the same array typedef.  Function
// parameter adjustment makes both first parameters pointers to int.
typedef int JumpBuffer[8];
typedef void (*JumpCallback)(JumpBuffer, int);

void jump_out(JumpBuffer environment, int value);
void install_jump_callback(JumpCallback callback);

void configure_jump_callback(void) {
    install_jump_callback(jump_out);
}
