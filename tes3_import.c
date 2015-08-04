/*******************************************************************************************
 ** TES3Annwyn: A TES3/TES4 height map importer/exporter (to & from RAW or BMP).
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
#include <unistd.h>
#include <math.h>
#include <string.h>

#include "defs.h"
#include "tes3_import.h"
#include "tes3_export.h"

int ImportImage(char *input_filename, int opt_bpp, int opt_vclr, int opt_sx,
                int opt_sy, int opt_image_type, int opt_vtex, int opt_rescale,
                int opt_adjust_height, int opt_limit, int opt_lower_limit,
                int opt_upper_limit, int opt_x_cell_offset,
                int opt_y_cell_offset, int opt_ignore_land_upper,
                int opt_ignore_land_lower, char opt_texture[], int opt_scale)
{
    int i,
        x,
        y,
        cx,
        cy;

    int Bp = 2; 	 // Bytes per Pixel (i.e. bpp / 8)!

    int x_range = 0,
        x_cell_range = 0,
        y_cell_range = 0;

    struct {
        int sx;
        int sy;
        int x_range;
        int Bp;
    } vclr;

    int vclr_sx = 0,
        vclr_sy = 0;

    int flag_ignore_land = 0;

    char  h8int = 0;
    short int h16int = 0;
    int   h32int = 0;

    char s[4096],
         s_vclr[4096],
         s_vtex[8192];

    int image[66][66];

    char vclr_image[66][66][3];
    unsigned short int vtex_image[16][16];
    int tex_size = MW_TEXSIZE;

    FILE *fp_in,
         *fp_out,
         *fp_vclr,
         *fp_vtex[9];

    int height_stat_min = 1048576,
        height_stat_max = -1048576, // Record the minimum and maximum heights
        height_stat_max_cell_x,
        height_stat_max_cell_y,
        height_stat_min_cell_x,
        height_stat_min_cell_y;

    int cellsize = 0;

    int total_overflows = 0,
        total_underflows = 0;

    int total_records = 0;

    Bp = opt_bpp / 8; // (wince at the capital) Bytes per pixel (opt_bpp is bits per pixel!)

    total_land = 0;

    if (opt_vclr) {
        vclr.sx = opt_sx;
        vclr.sy = opt_sy;

        vclr_sx = opt_sx;
        vclr_sy = opt_sy;

        StandardizeBMP2RAW(TA_VCLR_IN, TA_TMP_VCLR_RAW, &vclr_sx, &vclr_sy, &vclr.Bp, MW_CELLSIZE, opt_adjust_height, opt_scale);
        printf("Import VCLR BMP has dimensions %dx%d (%d-bit)\n", vclr_sx, vclr_sy, 8*vclr.Bp);
    }

    if (opt_image_type == RAW) {
        StandardizeRAW(input_filename, TA_TMP_RAW, &opt_sx, &opt_sy, Bp, MW_CELLSIZE, opt_adjust_height, opt_scale);
    } else {
        StandardizeBMP2RAW(input_filename, TA_TMP_RAW, &opt_sx, &opt_sy, &Bp, MW_CELLSIZE, opt_adjust_height, opt_scale);
        printf("Imported BMP has dimensions %dx%d (%d-bit)\n", opt_sx, opt_sy, Bp*8);
    }

    printf("Reading file: %s (dimensions %dx%d, bit-rate %d)\n", input_filename, opt_sx, opt_sy, Bp*8);

    if ((fp_out = fopen(TA_ESP_OUT, "wb")) == 0) {
        fprintf(stderr, "Unable to open %s for writing: %s\n",
            TA_ESP_OUT, strerror(errno));
        exit(1);
    }

    WriteTES3Header(fp_out);
    if (opt_texture[0] != '\0') {
        WriteTES3LTEX(fp_out, &total_records);
    } else if (opt_vtex > 0) {
        WriteTES3LTEX(fp_out, &total_records);
    }
    if (opt_vtex > 0) {
        tex_size = MW_TEXSIZE;
    }

    if ((fp_in = fopen(TA_TMP_RAW, "rb")) == NULL) {
        fprintf(stderr, "Unable to open the temporary RAW file I just created (%s) for reading!: %s\n",
            input_filename, strerror(errno));
        exit(1);
    }

    if (opt_vclr) {
        if ((fp_vclr = fopen(TA_TMP_VCLR_RAW, "rb")) == NULL) {
            fprintf(stderr, "You specified that you wanted to import a VCLR image with this "
                "heightmap, but I cannot find the RAW version I just created! It should be called: %s: %s\n",
                TA_TMP_VCLR_RAW, strerror(errno));
            exit(1);
        }
    }

    if (opt_vtex == 3) {
        if ((fp_vtex[0] = fopen(TA_VTEX3_OUT, "rb")) == NULL) {
            fprintf(stderr, "You specified that you wanted to import a VTEX3 image with this heightmap, but I cannot find it - it should be called: %s - but %s\n",
                TA_VTEX3_OUT, strerror(errno));
            exit(1);
        }
    }

    putchar('\n');

    // A loop to cycle through all the files, copying one row at a time.

    x_range = (opt_sx)* Bp;
    vclr.x_range = (vclr_sx) * vclr.Bp;

    cellsize = MW_CELLSIZE;
    x_cell_range = opt_sx / MW_CELLSIZE;
    y_cell_range = opt_sy / MW_CELLSIZE;

    for (cy = 0; cy < y_cell_range; cy++) {
        for (cx = 0; cx < x_cell_range; cx++) {
            memset(&image, 0, sizeof(image));
            memset(&vclr_image, 0, sizeof(vclr_image));
            for (y = 0; y < cellsize+2; y++) {
                fseek(fp_in, cx*cellsize*Bp + (cy*(cellsize)*(x_range+(2*Bp))) + (y*(x_range+(2*Bp))), SEEK_SET);
                fread(s, (cellsize+2)*Bp, 1, fp_in);

                if (opt_vclr) {
                    fseek(fp_vclr, cx*cellsize*vclr.Bp + (cy*(cellsize)*(vclr.x_range+(2*vclr.Bp))) + (y*(vclr.x_range+(2*vclr.Bp))), SEEK_SET);
                    fread(&s_vclr, (cellsize+2)*vclr.Bp, 1, fp_vclr);
                }

                for (x = 0; x < (cellsize+2); x++) {
                    image[y][x] = 0;
                    if (Bp == 1) {
                        memcpy(&h8int, s+(Bp*x), Bp);
                        image[y][x] = (unsigned char) h8int;
                    } else if (Bp == 2) {
                        memcpy(&h16int, s+(Bp*x), Bp);
                        image[y][x] = h16int;
                    } else if (Bp == 4) {
                        memcpy(&h32int, s+(Bp*x), Bp);
                        image[y][x] = h32int;
                    }

                    if (opt_vclr) {
                        memcpy(&vclr_image[y][x][0], s_vclr + (3*x), 3);

                    }
                }
            }

            for (y = 0; y < tex_size; y++) {
                ReadVTEX3(s_vtex, tex_size, cx, cy, y, opt_sx, opt_sy, fp_vtex[0]);
                for (x = 0; x < tex_size; x++) {
                    memcpy(&vtex_image[y][x], s_vtex + (2*x), 2);
                }
            }
            for (x = 0; x < cellsize+2; x++) {
                for (y = 0; y < cellsize+2; y++) {

                    image[y][x] = (int) (((float) image[y][x] * (float) opt_scale) + (float) opt_adjust_height / 8.0f);

                    if (opt_rescale) {
                        if (image[y][x] >= 2500)  {
                            image[y][x] *= 1.0 - (float) ((0.32 * (float) (8*image[y][x] - 20000)) / 28000);
                        }
                    }


                    if (opt_limit) {
                        if (image[y][x] < opt_lower_limit) image[y][x] = opt_lower_limit;
                        if (image[y][x] > opt_upper_limit) image[y][x] = opt_upper_limit;
                    }

                    if (image[y][x] < height_stat_min) {
                        height_stat_min = image[y][x];
                        height_stat_min_cell_x = cx + opt_x_cell_offset;
                        height_stat_min_cell_y = cy + opt_y_cell_offset;
                    } else if (image[y][x] > height_stat_max) {
                        height_stat_max = image[y][x];
                        height_stat_max_cell_x = cx + opt_x_cell_offset;
                        height_stat_max_cell_y = cy + opt_y_cell_offset;
                    }
                }
            }
            if (opt_ignore_land_upper || opt_ignore_land_lower) {
                flag_ignore_land = 1;
                for (x = 0; x < cellsize+1; x++) {
                    for (y = 0; y < cellsize+1; y++) {
                        if (image[x][y] < opt_ignore_land_lower/8 ||
                            image[x][y] > opt_ignore_land_upper/8) {
                            flag_ignore_land = 0;
                        }
                    }
                }
            }
            if (flag_ignore_land > 0) {
                flag_ignore_land = 0;
            } else {
                WriteTES3CELLRecord(cx + opt_x_cell_offset, cy + opt_y_cell_offset, fp_out);
                WriteTES3LANDRecord(cx + opt_x_cell_offset, cy + opt_y_cell_offset, opt_adjust_height, &image, vclr_image, vtex_image, fp_out, opt_vtex, opt_vclr, &total_overflows, &total_underflows, &opt_texture);
            }
        }
    }

    // Be nice and close everything.

    if (opt_vclr) fclose(fp_vclr);

    if (opt_vtex) {
        for (i = 0; i < MAX_LAYERS; i++) {
            if (fp_vtex[i] != NULL) fclose(fp_vtex[i]);
        }
    }

    fclose(fp_in);

    fclose(fp_out);
    unlink(TA_TMP_RAW);
    unlink(TA_TMP_VCLR_RAW);

    if (total_overflows > 0 || total_underflows > 0) {
        printf("\n\nSome gradient overflows/underflows have been caught and blocked:\n"
            "Total Overflows:  %d\n"
            "Total Underflows: %d\n",
            total_overflows, total_underflows);
    }
    printf("\n\nHighest point is %d THU (%d Game Units) = %f metres  [Cell (%d,%d)]\n",
        height_stat_max, height_stat_max * 8, ((float) height_stat_max * 0.114),
        height_stat_max_cell_x, height_stat_max_cell_y);
    printf("Lowest  point is %d THU (%d Game Units) = %f metres  [Cell (%d,%d)]\n",
        height_stat_min, height_stat_min * 8, ((float) height_stat_min * 0.114),
        height_stat_min_cell_x, height_stat_min_cell_y);
    printf("\nTotal number of cells imported: %d\n", total_land);
    printf("\nFinished! The imported ESP is called %s and is %d cells by %d cells.\n", TA_ESP_OUT, x_cell_range, y_cell_range);

    return 0;
}

int CatchGradientOverflows(int *gradient, int *total_overflows, int *total_underflows)
{
    if (*gradient > 127) {
        //printf("Gradient corrected: %d\n", *gradient);
        *gradient = 127;
        *total_overflows++;
        return 1;
    } else if (*gradient < -128) {
        //printf("Gradient corrected: %d\n", *gradient);
        *gradient = -128;
        *total_underflows--;
        return 1;
    }

    return 0;
}

int WriteTES3LANDRecord(int cx, int cy, int opt_adjust_height, int image[66][66], char vclr[66][66][3], short unsigned int vtex3[16][16], FILE *fp_out, int opt_vtex, int opt_vclr, int *total_overflows, int *total_underflows, char opt_texture[])
{
    int i, x, y;

    short int tex_num = 0;
    int size = 0;

    float height_offset;
    float v1[3],
          v2[3],
          normal[3],
          hyp;

    int igrad[65][65]; // An integer version of the gradients, useful for identifying over/underflows.
    char grad[65][65]; // 8-bit gradients, as required in the ESP format.

    short unsigned int ntex[16][16];

    memset(grad, 0, sizeof(grad));
    memset(igrad, 0, sizeof(igrad));

    for (y = 1; y < 65; y++) {
        for (x = 1; x < 65; x++) {
            igrad[y][x] = image[y][x] - image[y][x-1];
            CatchGradientOverflows(&igrad[y][x], total_overflows, total_underflows);
        }
    }

    // Calculate Column 0.
    for (y = 1; y < 65 ; y++) {
        igrad[y][0] = image[y][0] - image[y-1][0];
    }

    // Calculate Row 0.
    for (x = 1; x < 65 ; x++) {
        igrad[0][x] = image[0][x] - image[0][x-1];
        CatchGradientOverflows(&igrad[0][x], total_overflows, total_underflows);
    }

    for (y = 0; y < 65; y++) {
        for (x = 0; x < 65; x++) {
            grad[y][x] = (unsigned char) igrad[y][x];
        }
    }

    size = 17040; // 16959 - was 17040; // 12727+vhgt = 16967+wnam =
    if (opt_texture[0] != '\0' || opt_vtex > 0) {
        size += 520;
    }
    if (opt_vclr) {
        size += 12683;
    }

    fprintf(fp_out, "LAND");
    fwrite(&size, 4, 1, fp_out);

    for (i = 0; i < 8; i++) {
        fputc(0, fp_out);
    }
    fprintf(fp_out, "INTV%c%c%c%c", 8, 0, 0, 0);
    fwrite(&cx, 4, 1, fp_out);
    fwrite(&cy, 4, 1, fp_out);

    fprintf(fp_out, "DATA%c%c%c%c", 4, 0, 0, 0);
    if (opt_vclr == 0 && opt_texture[0] == '\0' && opt_vtex <= 0) {
        fprintf(fp_out, "%c%c%c%c", 9, 0, 0, 0);
    } else if (!opt_vclr) {
        fprintf(fp_out, "%c%c%c%c", 13, 0, 0, 0);
    } else {
        fprintf(fp_out, "%c%c%c%c", 7, 0, 0, 0);
    }

    fprintf(fp_out, "VNML%c%c%c%c", 131, 49, 0, 0);

    for (y = 0; y < 65; y++) {
        for (x = 0; x < 65; x++) {
            v1[0] = 16; 	// = 128/8
            v1[1] = 0;
            v1[2] = image[y][x+1] - image[y][x];

            v2[0] = 0;
            v2[1] = 16;
            v2[2] = image[y+1][x] - image[y][x];

            normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
            normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
            normal[2] = v1[0]*v2[1] - v1[1]*v2[0];

            hyp = sqrt(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]) / 127.0f;

            normal[0] /= hyp; normal[1] /= hyp; normal[2] /= hyp;

            fprintf(fp_out, "%c%c%c",
                (unsigned char) normal[0],
                (unsigned char) normal[1],
                (unsigned char) normal[2]);
        }
    }

    fprintf(fp_out, "VHGT%c%c%c%c", 136, 16, 0, 0);

    printf("(%d,%d) ", cx, cy);

    height_offset = (float) image[0][0];
    fwrite(&height_offset, 4, 1, fp_out);

    for (y = 0; y < 65; y++) {
        for (x = 0; x < 65; x++) {
            fputc(grad[y][x], fp_out);
        }
    }

    fprintf(fp_out, "%c%c%c", 0, 0, 0);

    fprintf(fp_out, "WNAM%c%c%c%c", 81, 0, 0, 0);
    for (i = 0; i < 81; i++) {
        fputc(128, fp_out);
    }

    if (opt_vclr != 0) {
        fprintf(fp_out, "VCLR%c%c%c%c", 131, 49, 0, 0);
        for (y = 0; y < 65; y++) {
            for (x = 0; x < 65; x++) {
                fputc(vclr[y][x][2], fp_out);
                fputc(vclr[y][x][1], fp_out);
                fputc(vclr[y][x][0], fp_out);
            }
        }
    }

    if (opt_texture[0] != '\0') {
        tex_num = atoi(opt_texture);
        fprintf(fp_out, "VTEX%c%c%c%c", 0, 2, 0, 0);
        for (i = 0; i < 256; i++) {
            fwrite(&tex_num, 2, 1, fp_out);
        }
    } else if (opt_vtex > 0) {
        fprintf(fp_out, "VTEX%c%c%c%c", 0, 2, 0, 0);
        DeStandardizeTES3VTEX(ntex, vtex3);
        fwrite(ntex, 512, 1, fp_out);
    }

    total_land++;

    return 0;
}

int WriteTES3CELLRecord(int cx, int cy, FILE *fp_out)
{
    int i;

    fprintf(fp_out, "CELL%c%c%c%c", 29, 0, 0, 0);
    for (i = 0; i < 8; i++) {
        fputc(0, fp_out);
    }
    fprintf(fp_out, "NAME%c%c%c%c%c", 1, 0, 0, 0, 0);
    fprintf(fp_out, "DATA%c%c%c%c", 12, 0, 0, 0);

    for (i = 0; i < 4; i++) {
        fputc(0, fp_out);
    }
    fwrite(&cx, 4, 1, fp_out);
    fwrite(&cy, 4, 1, fp_out);

    return 0;
}

// Not generally used, this version reads a file called cells.txt" contained cell co-ordinates in names,
// useful if relocating a land or importing, say, a Fallout3/Oblivion/Skyrim landscape to Morrowind.

int WriteTES3CELLRecordN(int cx, int cy, FILE *fp_out)
{
    int i;
    char name[256];

    char s[256];
    char s_x[64], s_y[64], s_name[256];
    int fx = -99999999, fy = -99999999; // No chance of an accidental match with these defaults. :)
    int ox, oy;
    int p;

    FILE *fp;

//	ox = (int) ((float) cx / 2)-1;
//	oy = (int) ((float) cy / 2)+4;
    ox = (int) cx;
    oy = (int) cy;

    if ((fp = fopen("cells.txt", "r")) != 0) {
        while (fgets(s, 256, fp) != 0) {
            p = 0;

            for (i = 0; s[i+p] != ':' && s[i+p] != '\0'; s_x[i] = s[i+p], i++);
            s_x[i] = '\0';

            fx = atoi(s_x);

            p += i+1;
            for (i = 0; s[i+p] != ':' && s[i+p] != '\0'; s_y[i] = s[i+p], i++);
            s_y[i] = '\0';

            fy = atoi(s_y);

            p += i+1;
            for (i = 0; s[i+p] != ':' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; s_name[i] = s[i+p], i++);
            s_name[i] = '\0';

            //printf("Got %d (%s),%d (%s),%s<<\n", fx, s_x, fy, s_y, s_name);

            if (fx == ox && fy == oy) break;
        }
        fclose(fp);
    } else {
        fprintf(stderr, "Cannot open list of cell names %s: %s\n", "cells.txt", strerror(errno));
    }

    if (fx == ox && fy == oy) {
        strcpy(name, s_name);
        printf("Matched name: %s\n", name);
    } else {
        name[0] = '\0';
    }

    fprintf(fp_out, "CELL%c%c%c%c", 29+ (int) strlen(name), 0, 0, 0);
    for (i = 0; i < 8; i++) {
        fputc(0, fp_out);
    }
    fprintf(fp_out, "NAME%c%c%c%c%s%c", (int) strlen(name)+1, 0, 0, 0, name, 0);
    fprintf(fp_out, "DATA%c%c%c%c", 12, 0, 0, 0);

    for (i = 0; i < 4; i++) {
        fputc(0, fp_out);
    }
    fwrite(&cx, 4, 1, fp_out);
    fwrite(&cy, 4, 1, fp_out);

    return 0;
}


int WriteTES3Header(FILE *fp_out)
{
        char header_dat[324] = {
                0x54, 0x45, 0x53, 0x33, 0x34, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x48, 0x45, 0x44, 0x52, 0x2C, 0x01, 0x00, 0x00, 0x66, 0x66,
        0xA6, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x06, 0x00, 0x00
        };

        int i;

        for (i = 0; i < sizeof(header_dat); i++) {
                fputc(header_dat[i], fp_out);
        }

        return 0;

}

int StandardizeRAW(char *input_filename, char *output_filename, int *sx, int *sy, int Bp, int std, int opt_adjust_height, int opt_scale)
{
    int i, j;
    int sxx = 0, syx = 0;
    int pad_height = -256,
        pdh32 = -256;
    short int pdh16 = -256;
    char pdh8 = 0;

    FILE *fp_in,
         *fp_out;

    char s[65536];

    printf("Standardizing RAW file to %dx%d (%d-bit)\n", *sx, *sy, Bp*8);

    if ((fp_in = fopen(input_filename, "rb")) == NULL) {
        fprintf(stderr, "Unable to open input file (%s) for reading: %s\n",
            input_filename, strerror(errno));
        exit(1);
    }
    if ((fp_out = fopen(output_filename, "wb")) == NULL) {
        fprintf(stderr, "Unable to open input file (%s) for reading: %s\n",
            output_filename, strerror(errno));
        exit(1);
    }

    // If the input image is not a multiple of 64 in either dimension, we'll have to pad it out, or the
    // end rows/columns will fall out of sync when read back in.
    // Work out how much we might need to add.

    sxx = (*sx - std * (int) (*sx / std));
    syx = (*sy - std * (int) (*sy / std));
    if (sxx > 0) sxx = std - sxx;
    if (syx > 0) syx = std - syx;

    pad_height = 0;
    if (Bp == 1) {
        pdh8 = (int) ((float) (pdh8 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh8, 1);
    } else if (Bp == 2) {
        pdh16 = (int) ((float) (pdh16 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh16, 2);
    } else if (Bp == 4) {
        pdh32 = (int) ((float) (pdh32 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh32, 4);
    }
    pad_height = 0;

    for (i = 0; i < *sy; i++) {
        fread (s, Bp * (*sx), 1, fp_in);
        fwrite(s, Bp * (*sx), 1, fp_out);

        for (j = 0; j < sxx+2; j++) {
            fwrite(&pad_height, Bp, 1, fp_out);
        }
    }

    // Double up data in first row (including the first column twice).

    for (i = 0; i < syx+2; i++) {
        fwrite(&pad_height, Bp, 1, fp_out);
        for (j = 0; j < (*sx+sxx+2); j++) {
            fwrite(&pad_height, Bp, 1, fp_out);
        }
    }

    *sx = *sx + sxx;
    *sy = *sy + syx;

    if (sxx > 0 || syx > 0) {
        printf("RAW file data has been padded from %dx%d to %dx%d (%d-bit) for importing.\n",
            *sx - sxx, *sy -syx, *sx, *sy, Bp*8);
    }

    fclose(fp_out);
    fclose(fp_in);

    return 0;
}

// Turn a BMP file in to a 32-bit RAW file of the same dimensions.

int StandardizeBMP2RAW(char *input_filename, char *output_filename, int *sx, int *sy, int *Bp, int std, int opt_adjust_height, int opt_scale)
{
    int i, j;

    int sxx = 0, syx = 0;
    int pad_height = -256,
        pdh32 = -256;
    short int pdh16 = -256;
    char pdh8 = 0;
    int odd_row = 0;

    FILE *fp_in,
         *fp_out;

    char s[65536];
    char pad[4];

    int bmp_head_size = 54;

    if ((fp_in = fopen(input_filename, "rb")) == NULL) {
        fprintf(stderr, "Unable to open input file (%s) for reading: %s\n",
            input_filename, strerror(errno));
        exit(1);
    }

    // Gather data from BMP header.
    fread(s, 54, 1, fp_in);
    bmp_head_size = s[10] + 256*s[11];

    fseek(fp_in, 0, SEEK_SET);

    fread(s, bmp_head_size, 1, fp_in);

    memcpy(sx, s+18, 4);
    memcpy(sy, s+22, 4);
    memcpy(Bp, s+28, 1);

    *Bp = *Bp / 8;

    pad_height = 0;
    if (*Bp == 1) {
        pdh8 = (int) ((float) (pdh8 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh8, 1);
    } else if (*Bp == 2) {
        pdh16 = (int) ((float) (pdh16 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh16, 2);
    } else if (*Bp == 3) { // Only used for 24-bit VCLR images. So we set it to white.
        pdh32 = 0xFFFFFFFF;
        memcpy(&pad_height, &pdh32, 4);
    } else if (*Bp == 4) {
        pdh32 = (int) ((float) (pdh32 + opt_adjust_height) * opt_scale);
        memcpy(&pad_height, &pdh32, 4);
    }

    if ((fp_out = fopen(output_filename, "wb")) == NULL) {
        fprintf(stderr, "Unable to open input file (%s) for reading: %s\n",
            output_filename, strerror(errno));
        exit(1);
    }
    fseek(fp_in, bmp_head_size, SEEK_SET);

    // If the input image is not a multiple of 64 in either dimension, we'll have to pad it out, or the
    // end rows/columns will fall out of sync when read back in.
    // Work out how much we might need to add.

    sxx = (*sx - std * (int) (*sx / std));
    syx = (*sy - std * (int) (*sy / std));
    if (sxx > 0) sxx = std - sxx;
    if (syx > 0) syx = std - syx;

    // If the row numbers are odd here, this BMP has already been padded out.

    odd_row =  (*sx* *Bp) - (4 * (int) (*sx * *Bp/ 4));
    if (odd_row > 0) odd_row = 4 - odd_row;

    for (i = 0; i < *sy; i++) {
        fread(s,  *Bp * (*sx), 1, fp_in);
        fwrite(s, *Bp * (*sx), 1, fp_out);

        fread(pad, odd_row, 1, fp_in);

        for (j = 0; j < sxx+2; j++) {
            fwrite(&pad_height, *Bp, 1, fp_out);
        }
    }

    // Double up data in first row (including the first column twice).

    for (i = 0; i < syx+2; i++) {
        fwrite(&pad_height, *Bp, 1, fp_out);
        for (j = 0; j < (*sx+sxx+2); j++) {
            fwrite(&pad_height, *Bp, 1, fp_out);
        }
    }

    *sx = *sx + sxx;
    *sy = *sy + syx;

    if (sxx > 0 || syx > 0) {
        printf("BMP has been padded from %dx%d to %dx%d (%d-bit) for importing.\n",
            *sx - sxx, *sy -syx, *sx, *sy, *Bp*8);
    }

    fclose(fp_out);
    fclose(fp_in);

    return 0;
}

int WriteTES3LTEX(FILE *fp_out, int *total_records)
{
        int i = 0,
            p = 0,
            index = 0,
            i1, i2;

        char lname[128],
             tname[128],
             iname[128],
             s[1024];

        FILE *fp_lt;

        if ((fp_lt = fopen(TES3_LTEX_DATA_FILE, "rb")) != 0) {
                while (fgets(s, 1024, fp_lt) != NULL) {
                        if (s[0] == '#') continue; // Ignore lines beginning with #

                        //sscanf(s, "%d,%s,%s\n", &index, lname, tname);
                        for (i = 0; s[i] != ',' && s[i] != '\0'; iname[i] = s[i], i++);
                        iname[i] = '\0';

                        p = i+1;
                        for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; lname[i] = s[i+p], i++);
                        lname[i] = '\0';

                        p += i+1;
                        for (i = 0; s[i+p] != ',' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; tname[i]
 = s[i+p], i++);
                        tname[i] = '\0';

                        index = atoi(iname);

                        i2 = (int) index / 256;
                        i1 = (int) index - (256*i2);

                        fprintf(fp_out, "LTEX%c%c%c%c", 30 + (int) strlen(lname) + (int) strlen(tname), 0, 0, 0);
                        for (i = 0; i < 8; i++) {
                                fputc(0, fp_out);
                        }

                        fprintf(fp_out, "NAME%c%c%c%c%s%c", (int) strlen(lname)+1, 0, 0, 0, lname, 0);
                        fprintf(fp_out, "INTV%c%c%c%c%c%c%c%c", 4, 0, 0, 0, i1, i2 , 0, 0);
                        fprintf(fp_out, "DATA%c%c%c%c%s%c", (int) strlen(tname)+1, 0, 0, 0, tname, 0);
                        total_records++;
                }
                fclose(fp_lt);
        } else {
                fprintf(stderr, "Warning: Cannot open your TES3 land textures data (%s) file for reading: %s\n",
                        TES3_LTEX_DATA_FILE, strerror(errno));
                return 1;
        }

        return 0;
}

int ReadVTEX3(char *s_vtex, int tex_size, int cx, int cy, int y, int sx, int sy, FILE *fp_vtex)
{
    char s[4096];

    int  Bp = 2;

    int x_range;
    int x_size;
    int y_size;

    x_size = 16;
    y_size = 16;

    x_range = sx / 4; // 672 across

    fseek(fp_vtex, 54 +
        (((int) ((float) cx * x_size))*Bp)
        + ((int) ((float) cy / 1.0f) * x_range * y_size * Bp)
        + ((int) ((float) y / 1.0f) * x_range * Bp)
        ,SEEK_SET);

    fread(s, (int) x_size*Bp, 1, fp_vtex);

    for (i = 0; i < tex_size; i++) {
        memcpy(s_vtex + (Bp*i), s + (i * Bp), 2);
    }

    return 0;
}

