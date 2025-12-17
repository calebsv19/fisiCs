%:define DIG 3
??=define TRI 4
#pragma STDC FP_CONTRACT ON

int main(void) {
    int arr<:2:> = { DIG, TRI };
    return DIG + TRI;
}
