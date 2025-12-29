// ICE checks for sizeof/alignof on VLAs.
int f(int n) {
    int vla[n];
    int a = sizeof(vla);
    int b = _Alignof(vla);
    return a + b;
}
