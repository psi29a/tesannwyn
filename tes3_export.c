/*******************************************************************************************
 ** TESAnnwyn: A TES3/TES4 height map importer/exporter (to & from RAW or BMP).
 **
 ** Paul Halliday: 31-Dec-2006
 **
 ** This is entirely my own work. No borrowed code. All reverse engineering has been
 ** researched by myself.
 **
 ** License: GNU (Copy, modify, distribute as you please. ;)
 ***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defs.h"
#include "tes3_export.h"
#include "tes3_vtex.h"
#include "common.h"

// Creates a RAW or BMP (opt_image_type) image called output_filename.
// It reads the VHGT data from files called "landx.y.tmp" where x and y are the range
// of co-ordinates matched from the ESP in the preceding function.

/* TODO: try to remove globals */
int cleanup_list_x[1048576];
int cleanup_list_y[1048576];
int cleanup_list_count = 0;
int height_stat_min = 1048576,
    height_stat_max = -1048576, // Record the minimum and maximum heights
    height_stat_max_cell_x,
    height_stat_max_cell_y,
    height_stat_min_cell_x,
    height_stat_min_cell_y;



int HumptyImage(char *output_filename, int opt_image_type, int bpp, int opt_rescale, int opt_adjust_height, int opt_grid, int opt_scale)
{
    int i, j = 0;
    int c;
    int x, y;
    float global_height_offset;
    int height = 0;
    int col_offset = 0,
        row_sum = 0;
    int ppc;	// Points per cell (64 for TES3, 32 for TES4).

    char  h8int = 0;
    short int h16int = 0;
    int   h32int = 0;
    int   Bp = bpp / 8;
    int   bmp_offset = 0;
    int   xdim, ydim;

    char land_data[32000];
    char vhgt_data[16384];
    char land_filename[256];

    FILE *fp_o,
         *fp_land;

    // Recalculate min and max x and y values.
    if ((fp_o = fopen(output_filename, "wb")) == 0) {
        fprintf(stderr, "Cannot create a new exported ESP file (%s): %s\n",
            output_filename, strerror(errno));
        exit(1);
    }

    ppc = MW_CELLSIZE;

    if (opt_image_type == BMP) {
        if (bpp == 8) {
            WriteBMPGreyScaleHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), bpp);
            bmp_offset = 1080;
        } else {
            WriteBMPHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), bpp);
            bmp_offset = 54;
        }
    } else {
        bmp_offset = 0;
    }
    Bp = bpp / 8;

    xdim = ppc*((max_x-min_x)+1);
    ydim = ppc*((max_y-min_y)+1);

    printf("Image file has dimensions: %dx%d (%d-bit)\n", xdim, ydim, bpp);

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            memset(&vhgt_data, 0, 16384);

            sprintf(land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, x, y);
            if ((fp_land = fopen(land_filename, "rb")) == 0) {

                /************************************************************
                 * If there's no land file exported, create a seabed instead.
                 ***********************************************************/

                for (c = 1; c < ppc+1; c++) {
                     fseek(fp_o, bmp_offset + ((y-min_y)*xdim*Bp*ppc) + ((x-min_x)*Bp*ppc) + (0*Bp*xdim*ppc) + ((c-1)*xdim*Bp), SEEK_SET);
                for (i = 1; i < ppc+1; i++) {
                    if (opt_rescale) {
                        height = 0;
                    } else {
                        height = (int) ((float) ((-2048 + opt_adjust_height)/8) * opt_scale);
                    }

                    if (opt_grid != -1) {
                        if (c == 1 || i == 1) {
                            height = opt_grid;
                        }
                    }

                    // For KuKulzA VVD x2:

                    /*
                    //height = 0xFFFF;

                    if (opt_grid != -1) {
                        if (
                            ((x -( 10 * (int) (x / 10)) == 0) && (i == 1 || i == 2)) ||
                            ((y -( 10 * (int) (y / 10)) == 0) && (c == 1 || c == 2))) {

                            height = opt_grid;
                        }
                    }
                    */

                    if (bpp == 32) {
                        h32int = (int) height;
                        fwrite(&h32int, 4, 1, fp_o);
                    } else if (bpp == 16) {
                        h16int = (short int) height;
                        fwrite(&h16int, 2, 1, fp_o);
                    } else if (bpp == 8) {
                        h8int = (char) height;
                        fwrite(&h8int, 1, 1, fp_o);
                    }
                }
                }
                continue;
            } else {
                fread(land_data, 16384, 1, fp_land);

                //memcpy(vhgt_data, land_data+3273+6, 1093);
                memcpy(vhgt_data, land_data, 4229);

                memcpy(&global_height_offset, &vhgt_data, 4);

                col_offset = vhgt_data[4]; // Gradient Grid point (0, 0) - usually 0, but ...
                for (i = 0; i < ppc+1; i++) {
                    if (i > 0) {
                        fseek(fp_o, bmp_offset + ((y-min_y)*xdim*Bp*ppc) + ((x-min_x)*Bp*ppc) + (0*Bp*xdim*ppc) + ((i-1)*xdim*Bp), SEEK_SET);
                        col_offset += vhgt_data[4+(i*(ppc+1))];
                    }
                    row_sum = (int) (global_height_offset) + (col_offset);

                    for (j = 0; j < ppc+1; j++) {

                        if (j > 0) {
                            row_sum += vhgt_data[4+(i*(ppc+1))+j];
                        }

                        height = (int) (((float) (row_sum + (opt_adjust_height/8))) * opt_scale);

                        if (height < height_stat_min) {
                            height_stat_min = height;
                            height_stat_min_cell_x = x;
                            height_stat_min_cell_y = y;
                        } else if (height > height_stat_max) {
                            height_stat_max = height;
                            height_stat_max_cell_x = x;
                            height_stat_max_cell_y = y;
                        }

                        if (opt_grid != -1 && (i == 1 || j == 1)) height = opt_grid;

                        if (i > 0 && j > 0) {
                            if (bpp == 32) {
                                h32int = (int) height;
                                fwrite(&h32int, 4, 1, fp_o);
                            } else if (bpp == 16) {
                                h16int = (short int) height;
                                fwrite(&h16int, 2, 1, fp_o);
                            } else if (bpp == 8) {
                                h8int = (char) height;
                                fwrite(&h8int, 1, 1, fp_o);
                            }
                        }
                    }
                }
                fclose(fp_land);
            }
        }
    }
    fclose(fp_o);

    return 0;
}

int RescaleGreyScale(char *output_filename, int opt_image_type, int bpp, int opt_adjust_height, int opt_scale)
{
    int i, j;
    int c;
    int x, y;
    float global_height_offset;
    int height = 0;
    int col_offset = 0,
        row_sum = 0;
    int ppc;	// Points per cell (64 for TES3, 32 for TES4).

    char tmp_int[5];

    char land_data[32000];
    char vhgt_data[16384];

    char land_filename[64];

    FILE *fp_land;

    // Recalculate min and max x and y values.

    int h_low =  65536,
        h_high = -65536,
        height_stat_max_cell_x,
        height_stat_max_cell_y,
        height_stat_min_cell_x,
        height_stat_min_cell_y;

    ppc = MW_CELLSIZE;

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            memset(&vhgt_data, 0, 16384);

            sprintf(land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, x, y);
            if ((fp_land = fopen(land_filename, "rb")) != 0) {
                fread(land_data, 16384, 1, fp_land);

                memcpy(vhgt_data, land_data, 4229);

                memcpy(&global_height_offset, &vhgt_data, 4);

                for (c = 1; c < ppc+1; c++) {
                    col_offset = vhgt_data[4]; // Gradient Grid point (0, 0) - usually 0, but ...
                    for (i = 1; i < ppc+1; i++) {
                        col_offset += vhgt_data[4+(i*(ppc+1))];
                        row_sum = (int) (global_height_offset) + (col_offset);
                        for (j = 1; j < ppc+1; j++) {
                            row_sum += vhgt_data[4+(i*(ppc+1))+j];

                            if (c == i) {
                                memcpy(tmp_int, &row_sum, 4);
                                height = ((float) (row_sum + (opt_adjust_height/8))) * opt_scale;
                                if (h_high < height) {
                                    h_high = height; 
                                    height_stat_max_cell_x = x;
                                    height_stat_max_cell_y = y;
                                }
                                if (h_low  > height) {
                                    h_low = height; 
                                    height_stat_min_cell_x = x;
                                    height_stat_min_cell_y = y;
                                }
                            }
                        }
                    }
                }
                fclose(fp_land);
            }
        }
    }

    // Now recalculate the scaling values from the range available.

    // First offset the heights so the lowest point is 0.
    if (h_low < 0) {
        opt_adjust_height = (int) ((float) (opt_adjust_height/8) - ((float) h_low / opt_scale)) * 8;
    }

    opt_scale = opt_scale / (float) ((float) (h_high - h_low) / (float) 255.01);

    printf("\nRescaling heights for 8-bit Greyscale:\n Max height detected: %d\n Min height detected: %d\n"
        " Calculated Height offset = %d\n Calculated Scale = %f\n\n",
            h_high, h_low, opt_adjust_height, opt_scale);

    return 0;
}

int HumptyVCLR(char *output_filename, int opt_grid)
{
    int i, j;
    int c;
    int x, y;
    int rgb = 0xFFFFFFFF;
    char bgr[4],
         rgb_c[4];
    int ppc;	// Points per cell (64 for TES3, 32 for TES4).

    int   xdim;

    char land_data[32000];
    char vclr_data[16384];

    char land_filename[64];

    FILE *fp_o,
         *fp_land;

    // Recalculate min and max x and y values.

    if ((fp_o = fopen(output_filename, "wb")) == 0) {
        fprintf(stderr, "Cannot create a new exported ESP file (%s): %s\n",
            output_filename, strerror(errno));
        exit(1);
    }

    ppc = MW_CELLSIZE;

    xdim = ppc*((max_x-min_x)+1);

    WriteBMPHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 24);

    printf("Vertex Colour Image file has dimensions: %dx%d (24-bit)\n", ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1));

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            sprintf(land_filename, "%s/vclr.%d.%d.tmp", TA_TMP_DIR, x, y);
            if ((fp_land = fopen(land_filename, "rb")) == 0) {

                /************************************************************
                 * If there's no land file exported, create a seabed instead.
                 ***********************************************************/

                for (c = 1; c < ppc+1; c++) {
                     fseek(fp_o, 54+ ((y-min_y)*xdim*3*ppc) + ((x-min_x)*3*ppc) + ((c-1)*xdim*3), SEEK_SET);
                for (i = 1; i < ppc+1; i++) {
                    rgb = 0xFFFFFFFF;
                    if (opt_grid != -1) {
                        if (c == 1 || i == 1) rgb = opt_grid;
                    }
                    fwrite(&rgb, 3, 1, fp_o);
                }
                }
                continue;
            } else {
                memset(&vclr_data, 0, 12675);
                fread(land_data, 16384, 1, fp_land);

                memcpy(vclr_data, land_data, 12675);

                for (c = 1; c < ppc+1; c++) {
                     fseek(fp_o, 54+ ((y-min_y)*xdim*3*ppc) + ((x-min_x)*3*ppc) + ((c-1)*xdim*3), SEEK_SET);
                for (i = 1; i < ppc+1; i++) {
                    for (j = 1; j < ppc+1; j++) {
                        if (c == i) {
                            rgb = 0xFFFFFFFF;
                            //memset(bgr, 255, 4);
                            memcpy(&bgr, vclr_data + (i*3*(ppc+1)) + (3*j), 3);
                            if (opt_grid != -1) {
                                if (c == 1 || j == 1) memcpy(bgr, &opt_grid, 3);
                            }
                            rgb_c[2] = bgr[0];
                            rgb_c[1] = bgr[1];
                            rgb_c[0] = bgr[2];
                            fwrite(&rgb_c, 3, 1, fp_o);
                        }
                    }
                }
                }
                fclose(fp_land);
            }
        }
    }
    fclose(fp_o);

    return 0;
}

int HumptyVTEX3(char *output_filename, int layer)
{
    int i, j;
    int x, y;
    int bsize = 0;
    int bmp_offset = 0;
    int ppc,	// Points per cell (64 for TES3, 32 for TES4).
        Bp,
        ipad,
        opad;

    int xdim;

    unsigned int tex = 0;

    char vtex3_data[4096];

    char land_filename[64];

    FILE *fp_o,
         *fp_land;

    // Recalculate min and max x and y values.

    if ((fp_o = fopen(output_filename, "wb")) == 0) {
        fprintf(stderr, "Cannot create a new TES3 VTEX placement BMP image file! (%s): %s\n",
            output_filename, strerror(errno));
        exit(1);
    }

    putchar('\n');
    ppc = 16;
    Bp = 2;
    ipad = 0;
    bsize = (ppc+ipad)*ppc*Bp;
    bmp_offset = 0;
    WriteBMPHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 16);
    printf("TES3 VTEX placement image has dimensions: %dx%d (%d-bit)\n", ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 8*Bp);

    xdim = ppc*((max_x-min_x)+1);

    opad = (int) (4*( ((float) (xdim*Bp) /4) - (float) ((int) ((xdim)*Bp/4)) )) ;

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            sprintf(land_filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, x, y);

            if ((fp_land = fopen(land_filename, "rb")) == 0) {

                /************************************************************
                 * If there's no land file exported, create a seabed instead.
                 ***********************************************************/

                for (i = 0; i < ppc; i++) {
                    fseek(fp_o, 54 + ((y-min_y)*(xdim*Bp*ppc+(ppc*opad))) + ((x-min_x)*Bp*ppc) + ((i)*xdim*Bp) + (i*opad), SEEK_SET);
                    for (j = 0; j < ppc; j++) {
                        fwrite(&tex, 3, 1, fp_o);
                    }
                    fwrite(&tex, opad, 1, fp_o);
                }
                continue;
            } else {
                memset(&vtex3_data, 0, bsize);
                fseek(fp_land, bmp_offset, SEEK_SET);
                fread(vtex3_data, bsize, 1, fp_land);

                for (i = 0; i < ppc; i++) {
                    fseek(fp_o, 54 + ((y-min_y)*(xdim*Bp*ppc+(ppc*opad))) + ((x-min_x)*Bp*ppc) + ((i)*xdim*Bp) + (i*opad), SEEK_SET);
                    fwrite(vtex3_data+i*(ipad+ppc*Bp), ppc*Bp, 1, fp_o);
                    fwrite(&tex, opad, 1, fp_o);
                }
                fclose(fp_land);
            }
        }
    }
    fclose(fp_o);

    return 0;
}


int CleanUp(int cleanup_list_x[], int cleanup_list_y[])
{
    int i,
        cleanup_list_count = 0;

    char filename[128];

    for (i = 0; i < cleanup_list_count; i++) {
        sprintf(filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
        sprintf(filename, "%s/vclr.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
        sprintf(filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
    }

    rmdir(TA_TMP_DIR);

    return 0;
}

int ExportImages(int opt_image_type, int opt_bpp, int opt_vclr, int opt_grid, int opt_vtex, int opt_adjust_height, int opt_rescale)
{
    int i,
        cleanup_list_count = 0;
    char dir_name[128];
    char vtex_layer_filename[128];

    ltex.count = 0;

        mkdir(TA_TMP_DIR, 0777);

    // A sanity check.

    if (opt_image_type != RAW && opt_image_type != BMP) {
        opt_image_type = BMP;
        printf("You didn't specify an image type (-p 1 or 2). I'll choose BMP for you ...\n");
    }

    for (i = 0; i < input_files.count; i++) {
        printf("\nRunning TES3 exporter:\n");
        ExportTES3Land(input_files.filename[i], opt_bpp, opt_vclr, opt_vtex);
    }

    if (opt_vclr) {
        printf("\n\nGenerating BMP of VCLR data: %s ... \n", TA_VCLR_OUT);
        HumptyVCLR(TA_VCLR_OUT, opt_grid);
    }

    if (opt_vtex) {
        printf("\n\nGenerating BMP of VTEX placement data: %s ... \n", TA_VTEX3_OUT);
        HumptyVTEX3(TA_VTEX3_OUT, 1);
    }

    if (opt_image_type == RAW) {
        printf("\n\nGenerating new RAW output file: %s ...\n", TA_RAW_OUT);
        HumptyImage(TA_RAW_OUT, opt_image_type, opt_bpp, opt_rescale, opt_adjust_height, opt_grid, opt_rescale);
    } else if (opt_image_type == BMP) {
        printf("\n\nGenerating new BMP output file: %s ...\n", TA_BMP_OUT);
        if (opt_rescale) {
            RescaleGreyScale(TA_BMP_OUT, opt_image_type, opt_bpp, opt_adjust_height, opt_rescale);
        }
        HumptyImage(TA_BMP_OUT, opt_image_type, opt_bpp, opt_rescale, opt_adjust_height, opt_grid, opt_rescale);
    }

    CleanUp(cleanup_list_x, cleanup_list_y);

        printf("\n\nHighest point was %d THU (%d Game Units) = %f metres    [Cell (%d,%d)]\n",
                height_stat_max, height_stat_max * 8, ((float) height_stat_max * 0.114), height_stat_max_cell_x, height_stat_max_cell_y);
        printf("Lowest  point was %d THU (%d Game Units) = %f metres    [Cell (%d,%d)]\n",
                height_stat_min, height_stat_min * 8, ((float) height_stat_min * 0.114), height_stat_min_cell_x, height_stat_min_cell_y);
    printf("\nBottom left of image corresponds to cell (%d, %d). This could be useful if you want to re-import.\n", min_x, min_y );

    if (opt_image_type == RAW) {
        printf("Completed. The output file is called: %s\n", TA_RAW_OUT);
    } else if (opt_image_type == BMP) {
        printf("Completed. The output file is called: %s\n", TA_BMP_OUT);
    }


    return 0;
}

int ExportTES3Land(char *input_esp_filename, int bpp, int opt_vclr, int opt_vtex)
{
    int size;	/* Size of current record.             */

    char s[40];	/* For storing the 16-byte header. */
    char *or;	/* Pointer to the Original Record. */

    FILE *fpin;	/* Input File Stream (original ESP/ESM).      */

    /*
     * Reset texture replacement list.
     */

    vtex3_replace.myindex = 0;
    vtex3_replace.replace_count = 0;

    /*
     * Open the ESP (input file) for reading.
     */

    if ((fpin = fopen(input_esp_filename, "rb")) == NULL) {
        fprintf(stderr, "Unable to open %s for reading: %s\n",
            input_esp_filename, strerror(errno));
        exit(1);
    }

    /* TES3 records follows the format of TYPE (4 bytes) then
     * the record length (4 bytes) - which we put in to the variable, size.
     */

    while (fread(s, 1, 16, fpin) > 0) {

        cell.current_x = 0;
        cell.current_y = 0;

        size = bytes_to_int(s[4], s[5], s[6], s[7]) + 16;

        /* Create some memory space to store the full orignal record
         * and an additional copy (nr) which may be modified.
         * A cheap hack here adds 2048 bytes to the memory allocated
         * to the new record. This is to cope with a compiler problem
         * (with Cygwin when compiling a static binary) with reallocing
         * more space (causes problems with free(or)) later.
         * A Morrowind script can now grow by 2K without causing any
         * problems - e.g. That's several hundred position updates in one script.
         */

        if ((or = (void *) malloc(size)) == NULL) {
            fprintf(stderr, "Unable to allocate %d bytes of"
                " memory to store TES file record: %s\n",
                size, strerror(errno));
            exit(1);
        }

        fseek(fpin, -16, SEEK_CUR);

        if (fread(or, 1, size, fpin) < size) {
            fprintf(stderr, "Unable to read entire marker record"
                " (%d bytes) from %s into memory: %s\n",
                size, input_esp_filename, strerror(errno));
            exit(1);
        }

        /******************************************************
         ** If it's a CELL or a LAND record, then hand it on to
         ** a procedure that will handle the format, including
         ** determining if any modifications should be made.
         *****************************************************/

        if (strncmp(s, "LAND", 4) == 0) {
            putchar(s[0]);
            Process3LANDData(or + 16, size-16, opt_vclr, opt_vtex);
        }  else if (strncmp(s, "LTEX", 4) == 0) {
            putchar(s[0]);
            Process3LTEXData(or + 16, size-16);
        }

        /************************************************************
         * If this record is marked as save, then save the original
         * unmodified record (*or) to the new file.
         * If generate_specific_esp is set, only save the TES3 record.
         ***********************************************************/

        free(or);
    }
    fclose(fpin);

    if (ltex.count > 0) {
        unlink(TA_LTEX3_OUT);
        WriteLTEXdata(TA_LTEX3_OUT);
    }

    return 0;
}

/*****************************************************************
** 5. Process3LandData(): Process a LAND record.
*****************************************************************
** LAND (4 bytes) + Length (4 bytes) + X (4 bytes) + Y (4 bytes).
****************************************************************/

int Process3LANDData(char *r, int size, int opt_vclr, int opt_vtex)
{
    int pos = 0,
        nsize = 0,
        vnml_size = 0,
        vnml_pos = 0,
        vhgt_pos = 0;

    unsigned short int ntex[16][16];

    char tmp_land_filename[64];

    FILE *fp_land;

    /*********************************************************
     * Get the (hopefully) INTV  header and X, Y co-ordinates.
     ********************************************************/
    if (strncmp("INTV", r, 4) == 0) {
        cell.current_x = bytes_to_int(r[8], r[9], r[10], r[11]);
        cell.current_y = bytes_to_int(r[12], r[13], r[14], r[15]);
        pos += 16;
    } else {
        fprintf(stderr, "WARNING: I couldn't find the INTV header for this TES3 LAND record (got [%c%c%c%c] - ignoring record.\n",
            r[0], r[1], r[2], r[3]);
        return 1;
    }

    if (strncmp("DATA", r + pos, 4) == 0) {
        nsize = bytes_to_int(r[pos+4], r[pos+5], r[pos+6], r[pos+7]);
        pos += 8 + nsize;
    }

    if (strncmp("VNML", r + pos, 4) != 0) {
        return -1;
    }

    vnml_pos = pos;
    vnml_size = bytes_to_int(r[vnml_pos+4], r[vnml_pos+5], r[vnml_pos+6], r[vnml_pos+7]);
    vhgt_pos = vnml_pos + 8 + vnml_size;

    if (strncmp("VHGT", r + vhgt_pos, 4) != 0) {
        fprintf(stderr, "Unable to find VHGT!!!\n");
        return -1;
    }

        sprintf(tmp_land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, cell.current_x, cell.current_y);

        if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                        tmp_land_filename, strerror(errno));
                exit(1);
        }

        // Write out the VHGT data (4232 bytes)

    fwrite(r+vhgt_pos+8, 4232, 1, fp_land);

    fclose(fp_land);

    pos = vhgt_pos + 4240+89;

    while (pos < size) {
        memcpy(&nsize, r + pos + 4, 4);

        if (opt_vclr && strncmp("VCLR", r + pos, 4) == 0) {
                sprintf(tmp_land_filename, "%s/vclr.%d.%d.tmp", TA_TMP_DIR, cell.current_x, cell.current_y);

                if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                        fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                                tmp_land_filename, strerror(errno));
                        exit(1);
            }

            fwrite(r+vhgt_pos+ 4329 + 8, 12675, 1, fp_land);

            fclose(fp_land);
        } else if (strncmp("VTEX", r + pos, 4) == 0 && opt_vtex) {
                sprintf(tmp_land_filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, cell.current_x, cell.current_y);

                if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                        fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                                tmp_land_filename, strerror(errno));
                        exit(1);
            }
            memset(ntex, 0, sizeof(ntex));

            if (vtex3_replace.replace_count > 0) {
                ReplaceVTEX3Textures(r + pos + 8);
            }

            StandardizeTES3VTEX(r + pos + 8, ntex);

            fwrite(ntex, 512, 1, fp_land);

            fclose(fp_land);
        }

        pos += 8 + nsize;
    }

        if (cell.current_x < min_x) min_x = cell.current_x;
        if (cell.current_x > max_x) max_x = cell.current_x;

        if (cell.current_y < min_y) min_y = cell.current_y;
        if (cell.current_y > max_y) max_y = cell.current_y;

        cleanup_list_x[cleanup_list_count] = cell.current_x;
        cleanup_list_y[cleanup_list_count++] = cell.current_y;

    total_land_copied++;

    return 0;
}

int ReplaceVTEX3Textures(char *vtex)
{
    int i, j;
    short unsigned int tex;

    for (i = 0; i < 256; i++) {
        tex = (short unsigned int) *(vtex+(2*i));
        for (j = 0; j < vtex3_replace.replace_count; j++) {
            if (vtex3_replace.old[j] == tex) {
                *(vtex+(2*i)) = vtex3_replace.new[j];
            }
        }

    }

    return 0;
}

int StandardizeTES3VTEX(unsigned short int vtex[16][16], unsigned short int ntex[16][16])
{
        int i, j, k, l;
        int q = 0;
        int qx = 0, qy = 0;
        int vtpointer = 0;
        char *vt;

        /************************************************************************
         ** Stage 1: Create a sensible (X by Y) array from the strange MW format:
         ** 4x4 groups, bottom-left, bottom-right, top-left, top-right,
         ** then those blocks repear in a similar 4x4 pattern!
         ***********************************************************************/

        vt = (void *) &vtex[0][0];
        for (q = 0; q < 4; q++) {

                if (q == 0)      { qx = 0; qy = 0; vtpointer = 0;   }
                else if (q == 1) { qx = 8; qy = 0; vtpointer = 64;  } // 128;
                else if (q == 2) { qx = 0; qy = 8; vtpointer = 256; }
                else if (q == 3) { qx = 8; qy = 8; vtpointer = 320; }  // 384;

                for (i = 0; i < 2; i++) {
                        vtpointer += (i*64);
                        for (j = 0; j < 2; j++) {
                                for (k = 0; k < 4; k++) {
                                        for (l = 0; l < 4; l++) {
                                                memcpy(&ntex[qy+k+(4*i)][qx+l+(4*j)], vt + vtpointer, 2); // = *(vt + vtpointer);
                                                vtpointer += 2;
                                        }
                                }
                        }
                }
        }

    return 0;
}

int DeStandardizeTES3VTEX(unsigned short int vtex[16][16], unsigned short int ntex[16][16])
{
    int i, j, k, l, q;
    int qx, qy;

    int vtpointer;
    char *vtpos;

    memset(vtex, 0, 512);

    vtpos = (void *) &vtex[0][0];

    for (q = 0; q < 4; q++) {
                if (q == 0)      { qx = 0; qy = 0; vtpointer = 0;   }
                else if (q == 1) { qx = 8; qy = 0; vtpointer = 64;  }
                else if (q == 2) { qx = 0; qy = 8; vtpointer = 256; }
                else if (q == 3) { qx = 8; qy = 8; vtpointer = 320; }

        for (i = 0; i < 2; i++) {
            vtpointer += (i*64);
            for (j = 0; j < 2; j++) {
                for (k = 0; k < 4; k++) {
                    for (l = 0; l < 4; l++) {
                        memcpy((vtpos + vtpointer), &ntex[qy+k+(4*i)][qx+l+(4*j)], 2);
                        vtpointer += 2;
                    }
                }
            }
        }
    }

    return 0;
}

int WriteLTEXdata(char *filename)
{
    int i;

    FILE *fp_lt;

    char FormID[9];

    if ((fp_lt = fopen(filename, "wb")) == 0) {
        fprintf(stderr, "Unable to create my TES3 LTEX reference file (%s): %s\n",
            filename, strerror(errno));
        CleanUp(cleanup_list_x, cleanup_list_y);
        exit(1);
    }

    for (i = 0 ; i < ltex.count; i++) {
        FormIDToString(FormID, ltex.formid[i]);
        fprintf(fp_lt, "%d,%s,%s,%s\n", ltex.texnum[i], ltex.texname[i], ltex.filename[i], FormID);
    }

    fclose(fp_lt);

    return 0;
}

