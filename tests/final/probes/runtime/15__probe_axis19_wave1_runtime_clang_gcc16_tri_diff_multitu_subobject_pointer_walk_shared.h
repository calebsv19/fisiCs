#ifndef AXIS19_SUBOBJECT_POINTER_WALK_SHARED_H
#define AXIS19_SUBOBJECT_POINTER_WALK_SHARED_H

struct axis19_grid {
    unsigned rows[3][4];
};

unsigned axis19_walk(const struct axis19_grid *grid, unsigned seed);

#endif
