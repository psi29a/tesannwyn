//
// Created by bcurtis on 08/03/16.
//

#ifndef TESANNWYN_LIBTESANNWYN_H
#define TESANNWYN_LIBTESANNWYN_H

// Record the minimum and maximum heights
typedef struct {
    int min;
    int max;
    int cell_max_x;
    int cell_max_y;
    int cell_min_x;
    int cell_min_y;
} height_stats;

typedef struct {
    char *data;
    unsigned short int is_import;
    height_stats statistics;

} tes_context;

tes_context tes_export(int image_type, int bpp, int vclr, int grid, int vtex, int adjust_height, float scale);

#endif //TESANNWYN_LIBTESANNWYN_H
