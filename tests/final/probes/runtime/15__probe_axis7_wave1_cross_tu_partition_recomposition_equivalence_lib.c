typedef struct Cell { unsigned int lane, payload; } Cell;

Cell transform_cell(Cell in) {
    Cell out;
    out.lane = (in.lane + 1u) % 4u;
    out.payload = in.payload * 11u + in.lane * 7u;
    return out;
}

unsigned int fold_cells(const Cell *cells, unsigned int count) {
    unsigned int total = 0u;
    for (unsigned int i = 0u; i < count; ++i) total += cells[i].payload * (cells[i].lane + 3u);
    return total;
}
