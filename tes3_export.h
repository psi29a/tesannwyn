#ifndef TES3_EXPORT_H
#define TES3_EXPORT_H

/* TODO: try to remove globals */
static int cleanup_list_x[1048576];
static int cleanup_list_y[1048576];
static int cleanup_list_count = 0;
static int height_stat_min = 1048576,
    height_stat_max = -1048576, // Record the minimum and maximum heights
    height_stat_max_cell_x,
    height_stat_max_cell_y,
    height_stat_min_cell_x,
    height_stat_min_cell_y;

int HumptyImage(char *, int, int, int, int, int, float);
int RescaleGreyScale(char *, int, int, int, float);

#endif /* TES3_EXPORT_H */


