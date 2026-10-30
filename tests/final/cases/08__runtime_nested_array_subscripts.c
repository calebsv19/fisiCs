#include <stdio.h>
struct Grid { double cell[12][12]; };
static int check(void) {
    struct Grid g = {0};
    unsigned map[2][4] = {{1,5,7,11},{2,4,8,10}};
    double (*matrix)[12] = g.cell;
    double expected[12][12] = {{0}};
    for (unsigned p=0;p<2;p++) for (unsigned i=0;i<4;i++) for (unsigned j=0;j<4;j++) {
        unsigned row=map[p][i], col=map[p][j];
        expected[row][col] = 100*p+10*i+j+1;
        g.cell[map[p][i]][map[p][j]] = 100*p+10*i+j+1;
    }
    for (unsigned i=0;i<12;i++) for (unsigned j=0;j<12;j++)
        if (g.cell[i][j] != expected[i][j]) return 1;
    for (unsigned p=0;p<2;p++) for (unsigned i=0;i<4;i++) for (unsigned j=0;j<4;j++) {
        unsigned row=map[p][i], col=map[p][j];
        if (matrix[map[p][i]][map[p][j]] != expected[row][col]) return 2;
        matrix[map[p][i]][map[p][j]] += 3;
        expected[row][col] += 3;
        double *address=&matrix[map[p][i]][map[p][j]];
        if (*address != expected[row][col]) return 3;
    }
    for (unsigned i=0;i<12;i++) for (unsigned j=0;j<12;j++)
        if (matrix[i][j] != expected[i][j]) return 4;
    unsigned chain[2][4]={{0,1,2,3},{3,2,1,0}};
    for (unsigned i=0;i<4;i++) {
        matrix[map[0][chain[1][i]]][map[1][chain[0][i]]] = 42+i;
        if (matrix[map[0][chain[1][i]]][map[1][chain[0][i]]] != 42+i) return 5;
    }
    return 0;
}
int main(void) { int status=check(); if(status) return status; puts("nested array subscripts PASS"); return 0; }
