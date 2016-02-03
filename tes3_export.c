/* TESAnnwyn: A TES3 height map importer/exporter (to & from RAW or BMP).
 *
 * Paul Halliday: 31-Dec-2006
 * Bret Curtis: 2015
 *
 * This is entirely my own work. No borrowed code.
 * All reverse engineering has been researched by myself.
 *
 * License: GNU (Copy, modify, distribute as you please. ;)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defs.h"
#include "tes3_export.h"

static int min_x = 32768,
    max_x = -32768,
    min_y = 32768,
    max_y = -32768;

/****************************************************
** bytes_to_int():
**
** Convert 4-bytes in to an Integer (little endian).
** Most commonly called for converting the size of a
** record or field in to a usable integer.
***************************************************/
int bytes_to_int(char b1, char b2, char b3, char b4)
{
    return (unsigned int) ((unsigned char) b1
            + (256 * (unsigned char) b2)
            + (65536 * (unsigned char) b3)
            + (16777216 * (unsigned char) b4));
}

int WriteBMPHeader(FILE *fp_out, int sx, int sy, int bpp)
{
    int i;

    char bmp_head[54] = {
            0x42, 0x4D, 0x98, 0xEA, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
            0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x64, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x0B,
            0x00, 0x00, 0x12, 0x0B, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    i = (sx*sy*4)+54;

    memcpy(bmp_head+2, &i, 4);
    memcpy(bmp_head+18, &sx, 4);
    memcpy(bmp_head+22, &sy, 4);
    bmp_head[28] = bpp;
    memcpy(bmp_head+28, &bpp, 1);  // Correct the header for bits per pixel.

    i = (sx*sy*4);
    memcpy(bmp_head+34, &i, 4);

    fwrite(bmp_head, 54, 1, fp_out);

    return 0;
}

int WriteBMPGreyScaleHeader(FILE *fp_out, int sx, int sy, int bpp)
{
    char bmp_head[1080] = {
            0x42, 0x4D, 0x38, 0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x04,
            0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x75, 0x0A, 0x00, 0x00, 0x75, 0x0A, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
            0x01, 0x00, 0x02, 0x02, 0x02, 0x00, 0x03, 0x03, 0x03, 0x00, 0x04, 0x04,
            0x04, 0x00, 0x05, 0x05, 0x05, 0x00, 0x06, 0x06, 0x06, 0x00, 0x07, 0x07,
            0x07, 0x00, 0x08, 0x08, 0x08, 0x00, 0x09, 0x09, 0x09, 0x00, 0x0A, 0x0A,
            0x0A, 0x00, 0x0B, 0x0B, 0x0B, 0x00, 0x0C, 0x0C, 0x0C, 0x00, 0x0D, 0x0D,
            0x0D, 0x00, 0x0E, 0x0E, 0x0E, 0x00, 0x0F, 0x0F, 0x0F, 0x00, 0x10, 0x10,
            0x10, 0x00, 0x11, 0x11, 0x11, 0x00, 0x12, 0x12, 0x12, 0x00, 0x13, 0x13,
            0x13, 0x00, 0x14, 0x14, 0x14, 0x00, 0x15, 0x15, 0x15, 0x00, 0x16, 0x16,
            0x16, 0x00, 0x17, 0x17, 0x17, 0x00, 0x18, 0x18, 0x18, 0x00, 0x19, 0x19,
            0x19, 0x00, 0x1A, 0x1A, 0x1A, 0x00, 0x1B, 0x1B, 0x1B, 0x00, 0x1C, 0x1C,
            0x1C, 0x00, 0x1D, 0x1D, 0x1D, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x1F, 0x1F,
            0x1F, 0x00, 0x20, 0x20, 0x20, 0x00, 0x21, 0x21, 0x21, 0x00, 0x22, 0x22,
            0x22, 0x00, 0x23, 0x23, 0x23, 0x00, 0x24, 0x24, 0x24, 0x00, 0x25, 0x25,
            0x25, 0x00, 0x26, 0x26, 0x26, 0x00, 0x27, 0x27, 0x27, 0x00, 0x28, 0x28,
            0x28, 0x00, 0x29, 0x29, 0x29, 0x00, 0x2A, 0x2A, 0x2A, 0x00, 0x2B, 0x2B,
            0x2B, 0x00, 0x2C, 0x2C, 0x2C, 0x00, 0x2D, 0x2D, 0x2D, 0x00, 0x2E, 0x2E,
            0x2E, 0x00, 0x2F, 0x2F, 0x2F, 0x00, 0x30, 0x30, 0x30, 0x00, 0x31, 0x31,
            0x31, 0x00, 0x32, 0x32, 0x32, 0x00, 0x33, 0x33, 0x33, 0x00, 0x34, 0x34,
            0x34, 0x00, 0x35, 0x35, 0x35, 0x00, 0x36, 0x36, 0x36, 0x00, 0x37, 0x37,
            0x37, 0x00, 0x38, 0x38, 0x38, 0x00, 0x39, 0x39, 0x39, 0x00, 0x3A, 0x3A,
            0x3A, 0x00, 0x3B, 0x3B, 0x3B, 0x00, 0x3C, 0x3C, 0x3C, 0x00, 0x3D, 0x3D,
            0x3D, 0x00, 0x3E, 0x3E, 0x3E, 0x00, 0x3F, 0x3F, 0x3F, 0x00, 0x40, 0x40,
            0x40, 0x00, 0x41, 0x41, 0x41, 0x00, 0x42, 0x42, 0x42, 0x00, 0x43, 0x43,
            0x43, 0x00, 0x44, 0x44, 0x44, 0x00, 0x45, 0x45, 0x45, 0x00, 0x46, 0x46,
            0x46, 0x00, 0x47, 0x47, 0x47, 0x00, 0x48, 0x48, 0x48, 0x00, 0x49, 0x49,
            0x49, 0x00, 0x4A, 0x4A, 0x4A, 0x00, 0x4B, 0x4B, 0x4B, 0x00, 0x4C, 0x4C,
            0x4C, 0x00, 0x4D, 0x4D, 0x4D, 0x00, 0x4E, 0x4E, 0x4E, 0x00, 0x4F, 0x4F,
            0x4F, 0x00, 0x50, 0x50, 0x50, 0x00, 0x51, 0x51, 0x51, 0x00, 0x52, 0x52,
            0x52, 0x00, 0x53, 0x53, 0x53, 0x00, 0x54, 0x54, 0x54, 0x00, 0x55, 0x55,
            0x55, 0x00, 0x56, 0x56, 0x56, 0x00, 0x57, 0x57, 0x57, 0x00, 0x58, 0x58,
            0x58, 0x00, 0x59, 0x59, 0x59, 0x00, 0x5A, 0x5A, 0x5A, 0x00, 0x5B, 0x5B,
            0x5B, 0x00, 0x5C, 0x5C, 0x5C, 0x00, 0x5D, 0x5D, 0x5D, 0x00, 0x5E, 0x5E,
            0x5E, 0x00, 0x5F, 0x5F, 0x5F, 0x00, 0x60, 0x60, 0x60, 0x00, 0x61, 0x61,
            0x61, 0x00, 0x62, 0x62, 0x62, 0x00, 0x63, 0x63, 0x63, 0x00, 0x64, 0x64,
            0x64, 0x00, 0x65, 0x65, 0x65, 0x00, 0x66, 0x66, 0x66, 0x00, 0x67, 0x67,
            0x67, 0x00, 0x68, 0x68, 0x68, 0x00, 0x69, 0x69, 0x69, 0x00, 0x6A, 0x6A,
            0x6A, 0x00, 0x6B, 0x6B, 0x6B, 0x00, 0x6C, 0x6C, 0x6C, 0x00, 0x6D, 0x6D,
            0x6D, 0x00, 0x6E, 0x6E, 0x6E, 0x00, 0x6F, 0x6F, 0x6F, 0x00, 0x70, 0x70,
            0x70, 0x00, 0x71, 0x71, 0x71, 0x00, 0x72, 0x72, 0x72, 0x00, 0x73, 0x73,
            0x73, 0x00, 0x74, 0x74, 0x74, 0x00, 0x75, 0x75, 0x75, 0x00, 0x76, 0x76,
            0x76, 0x00, 0x77, 0x77, 0x77, 0x00, 0x78, 0x78, 0x78, 0x00, 0x79, 0x79,
            0x79, 0x00, 0x7A, 0x7A, 0x7A, 0x00, 0x7B, 0x7B, 0x7B, 0x00, 0x7C, 0x7C,
            0x7C, 0x00, 0x7D, 0x7D, 0x7D, 0x00, 0x7E, 0x7E, 0x7E, 0x00, 0x7F, 0x7F,
            0x7F, 0x00, 0x80, 0x80, 0x80, 0x00, 0x81, 0x81, 0x81, 0x00, 0x82, 0x82,
            0x82, 0x00, 0x83, 0x83, 0x83, 0x00, 0x84, 0x84, 0x84, 0x00, 0x85, 0x85,
            0x85, 0x00, 0x86, 0x86, 0x86, 0x00, 0x87, 0x87, 0x87, 0x00, 0x88, 0x88,
            0x88, 0x00, 0x89, 0x89, 0x89, 0x00, 0x8A, 0x8A, 0x8A, 0x00, 0x8B, 0x8B,
            0x8B, 0x00, 0x8C, 0x8C, 0x8C, 0x00, 0x8D, 0x8D, 0x8D, 0x00, 0x8E, 0x8E,
            0x8E, 0x00, 0x8F, 0x8F, 0x8F, 0x00, 0x90, 0x90, 0x90, 0x00, 0x91, 0x91,
            0x91, 0x00, 0x92, 0x92, 0x92, 0x00, 0x93, 0x93, 0x93, 0x00, 0x94, 0x94,
            0x94, 0x00, 0x95, 0x95, 0x95, 0x00, 0x96, 0x96, 0x96, 0x00, 0x97, 0x97,
            0x97, 0x00, 0x98, 0x98, 0x98, 0x00, 0x99, 0x99, 0x99, 0x00, 0x9A, 0x9A,
            0x9A, 0x00, 0x9B, 0x9B, 0x9B, 0x00, 0x9C, 0x9C, 0x9C, 0x00, 0x9D, 0x9D,
            0x9D, 0x00, 0x9E, 0x9E, 0x9E, 0x00, 0x9F, 0x9F, 0x9F, 0x00, 0xA0, 0xA0,
            0xA0, 0x00, 0xA1, 0xA1, 0xA1, 0x00, 0xA2, 0xA2, 0xA2, 0x00, 0xA3, 0xA3,
            0xA3, 0x00, 0xA4, 0xA4, 0xA4, 0x00, 0xA5, 0xA5, 0xA5, 0x00, 0xA6, 0xA6,
            0xA6, 0x00, 0xA7, 0xA7, 0xA7, 0x00, 0xA8, 0xA8, 0xA8, 0x00, 0xA9, 0xA9,
            0xA9, 0x00, 0xAA, 0xAA, 0xAA, 0x00, 0xAB, 0xAB, 0xAB, 0x00, 0xAC, 0xAC,
            0xAC, 0x00, 0xAD, 0xAD, 0xAD, 0x00, 0xAE, 0xAE, 0xAE, 0x00, 0xAF, 0xAF,
            0xAF, 0x00, 0xB0, 0xB0, 0xB0, 0x00, 0xB1, 0xB1, 0xB1, 0x00, 0xB2, 0xB2,
            0xB2, 0x00, 0xB3, 0xB3, 0xB3, 0x00, 0xB4, 0xB4, 0xB4, 0x00, 0xB5, 0xB5,
            0xB5, 0x00, 0xB6, 0xB6, 0xB6, 0x00, 0xB7, 0xB7, 0xB7, 0x00, 0xB8, 0xB8,
            0xB8, 0x00, 0xB9, 0xB9, 0xB9, 0x00, 0xBA, 0xBA, 0xBA, 0x00, 0xBB, 0xBB,
            0xBB, 0x00, 0xBC, 0xBC, 0xBC, 0x00, 0xBD, 0xBD, 0xBD, 0x00, 0xBE, 0xBE,
            0xBE, 0x00, 0xBF, 0xBF, 0xBF, 0x00, 0xC0, 0xC0, 0xC0, 0x00, 0xC1, 0xC1,
            0xC1, 0x00, 0xC2, 0xC2, 0xC2, 0x00, 0xC3, 0xC3, 0xC3, 0x00, 0xC4, 0xC4,
            0xC4, 0x00, 0xC5, 0xC5, 0xC5, 0x00, 0xC6, 0xC6, 0xC6, 0x00, 0xC7, 0xC7,
            0xC7, 0x00, 0xC8, 0xC8, 0xC8, 0x00, 0xC9, 0xC9, 0xC9, 0x00, 0xCA, 0xCA,
            0xCA, 0x00, 0xCB, 0xCB, 0xCB, 0x00, 0xCC, 0xCC, 0xCC, 0x00, 0xCD, 0xCD,
            0xCD, 0x00, 0xCE, 0xCE, 0xCE, 0x00, 0xCF, 0xCF, 0xCF, 0x00, 0xD0, 0xD0,
            0xD0, 0x00, 0xD1, 0xD1, 0xD1, 0x00, 0xD2, 0xD2, 0xD2, 0x00, 0xD3, 0xD3,
            0xD3, 0x00, 0xD4, 0xD4, 0xD4, 0x00, 0xD5, 0xD5, 0xD5, 0x00, 0xD6, 0xD6,
            0xD6, 0x00, 0xD7, 0xD7, 0xD7, 0x00, 0xD8, 0xD8, 0xD8, 0x00, 0xD9, 0xD9,
            0xD9, 0x00, 0xDA, 0xDA, 0xDA, 0x00, 0xDB, 0xDB, 0xDB, 0x00, 0xDC, 0xDC,
            0xDC, 0x00, 0xDD, 0xDD, 0xDD, 0x00, 0xDE, 0xDE, 0xDE, 0x00, 0xDF, 0xDF,
            0xDF, 0x00, 0xE0, 0xE0, 0xE0, 0x00, 0xE1, 0xE1, 0xE1, 0x00, 0xE2, 0xE2,
            0xE2, 0x00, 0xE3, 0xE3, 0xE3, 0x00, 0xE4, 0xE4, 0xE4, 0x00, 0xE5, 0xE5,
            0xE5, 0x00, 0xE6, 0xE6, 0xE6, 0x00, 0xE7, 0xE7, 0xE7, 0x00, 0xE8, 0xE8,
            0xE8, 0x00, 0xE9, 0xE9, 0xE9, 0x00, 0xEA, 0xEA, 0xEA, 0x00, 0xEB, 0xEB,
            0xEB, 0x00, 0xEC, 0xEC, 0xEC, 0x00, 0xED, 0xED, 0xED, 0x00, 0xEE, 0xEE,
            0xEE, 0x00, 0xEF, 0xEF, 0xEF, 0x00, 0xF0, 0xF0, 0xF0, 0x00, 0xF1, 0xF1,
            0xF1, 0x00, 0xF2, 0xF2, 0xF2, 0x00, 0xF3, 0xF3, 0xF3, 0x00, 0xF4, 0xF4,
            0xF4, 0x00, 0xF5, 0xF5, 0xF5, 0x00, 0xF6, 0xF6, 0xF6, 0x00, 0xF7, 0xF7,
            0xF7, 0x00, 0xF8, 0xF8, 0xF8, 0x00, 0xF9, 0xF9, 0xF9, 0x00, 0xFA, 0xFA,
            0xFA, 0x00, 0xFB, 0xFB, 0xFB, 0x00, 0xFC, 0xFC, 0xFC, 0x00, 0xFD, 0xFD,
            0xFD, 0x00, 0xFE, 0xFE, 0xFE, 0x00, 0xFF, 0xFF, 0xFF, 0x00};

    int i = (sx*sy*4)+1078;
    memcpy(bmp_head+2, &i, 4);
    memcpy(bmp_head+18, &sx, 4);
    memcpy(bmp_head+22, &sy, 4);
    bmp_head[28] = bpp;
    memcpy(bmp_head+28, &bpp, 1);  // Correct the header for bits per pixel.

    i = (sx*sy*4);
    memcpy(bmp_head+34, &i, 4);

    fwrite(bmp_head, 1078, 1, fp_out);

    return 0;
}

// Creates a RAW or BMP (opt_image_type) image called output_filename.
// It reads the VHGT data from files called "landx.y.tmp" where x and y are the range
// of co-ordinates matched from the ESP in the preceding function.
int HumptyImage(char *output_filename, int opt_image_type, int bpp, int opt_adjust_height, int opt_grid, int opt_rescale, float opt_scale)
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

                        if (height < height_stat.min) {
                            height_stat.min = height;
                            height_stat.cell_min_x = x;
                            height_stat.cell_min_y = y;
                        } else if (height > height_stat.max) {
                            height_stat.max = height;
                            height_stat.cell_max_x = x;
                            height_stat.cell_max_y = y;
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

int RescaleGreyScale(char *output_filename, int opt_image_type, int bpp, int opt_adjust_height, float opt_scale)
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
        h_high = -65536;

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
                                    height_stat.cell_max_x = x;
                                    height_stat.cell_max_y = y;
                                }
                                if (h_low  > height) {
                                    h_low = height;
                                    height_stat.cell_min_x = x;
                                    height_stat.cell_min_y = y;
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


int CleanUp(int cleanup_list_x[], int cleanup_list_y[], int *cleanup_list_count)
{
    int i;

    char filename[128];

    for (i = 0; i < *cleanup_list_count; i++) {
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

int ExportImages(int opt_image_type, int opt_bpp, int opt_vclr, int opt_grid, int opt_vtex, int opt_adjust_height, int opt_rescale, float opt_scale)
{
    int i;
    ltex.count = 0;

    static int cleanup_list_x[1048576];
    static int cleanup_list_y[1048576];
    static int cleanup_list_count = 0;

    mkdir(TA_TMP_DIR, 0777);

    // A sanity check.
    if (opt_image_type != RAW && opt_image_type != BMP) {
        opt_image_type = BMP;
        printf("You didn't specify an image type (-p 1 or 2). I'll choose BMP for you ...\n");
    }

    for (i = 0; i < input_files.count; i++) {
        printf("\nRunning TES3 exporter:\n");
        ExportTES3Land(input_files.filename[i], opt_bpp, opt_vclr, opt_vtex, cleanup_list_x, cleanup_list_y, &cleanup_list_count);
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
        HumptyImage(TA_RAW_OUT, opt_image_type, opt_bpp, opt_adjust_height, opt_grid, opt_rescale, opt_scale);
    } else if (opt_image_type == BMP) {
        printf("\n\nGenerating new BMP output file: %s ...\n", TA_BMP_OUT);
        if (opt_rescale) {
            RescaleGreyScale(TA_BMP_OUT, opt_image_type, opt_bpp, opt_adjust_height, opt_scale);
        }
        HumptyImage(TA_BMP_OUT, opt_image_type, opt_bpp, opt_adjust_height, opt_grid, opt_rescale, opt_scale);
    }

    CleanUp(cleanup_list_x, cleanup_list_y, &cleanup_list_count);

    printf("\n\nHighest point was %d THU (%d Game Units) = %f metres    [Cell (%d,%d)]\n",
            height_stat.max, height_stat.max * 8, ((float) height_stat.max * 0.114), height_stat.cell_max_x, height_stat.cell_max_y);
    printf("Lowest  point was %d THU (%d Game Units) = %f metres    [Cell (%d,%d)]\n",
            height_stat.min, height_stat.min * 8, ((float) height_stat.min * 0.114), height_stat.cell_min_x, height_stat.cell_min_y);
    printf("\nBottom left of image corresponds to cell (%d, %d). This could be useful if you want to re-import.\n", min_x, min_y );

    if (opt_image_type == RAW) {
        printf("Completed. The output file is called: %s\n", TA_RAW_OUT);
    } else if (opt_image_type == BMP) {
        printf("Completed. The output file is called: %s\n", TA_BMP_OUT);
    }

    return 0;
}

int ExportTES3Land(char *input_esp_filename, int opt_bpp, int opt_vclr, int opt_vtex, int cleanup_list_x[], int cleanup_list_y[], int *cleanup_list_count)
{
    char s[40];	/* For storing the 16-byte header. */
    FILE *fpin;	/* Input File Stream (original ESP/ESM).      */

    /* Reset texture replacement list. */
    vtex3_replace.myindex = 0;
    vtex3_replace.replace_count = 0;

    /* Open the ESP (input file) for reading. */
    if ((fpin = fopen(input_esp_filename, "rb")) == NULL) {
        fprintf(stderr, "Unable to open %s for reading: %s\n",
            input_esp_filename, strerror(errno));
        exit(1);
    }

    /* TES3 records follows the format of TYPE (4 bytes) then
     * the record length (4 bytes) - which we put in to the variable, size. */
    while (fread(s, 1, 16, fpin) > 0) {

        /* Size of current record. */
        int size = bytes_to_int(s[4], s[5], s[6], s[7]) + 16;

        /* Create some memory space to store the full orignal record
         * and an additional copy (nr) which may be modified.
         * A cheap hack here adds 2048 bytes to the memory allocated
         * to the new record. This is to cope with a compiler problem
         * (with Cygwin when compiling a static binary) with reallocing
         * more space (causes problems with free(or)) later.
         * A Morrowind script can now grow by 2K without causing any
         * problems - e.g. That's several hundred position updates in one script. */

        char *or = (void *) malloc(size);	/* Pointer to the Original Record. */
        if (or == NULL) {
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
            //putchar(s[0]); // TODO: verbose output
            Process3LANDData(or + 16, size-16, opt_vclr, opt_vtex, cleanup_list_x, cleanup_list_y, cleanup_list_count);
        }  else if (strncmp(s, "LTEX", 4) == 0) {
            //putchar(s[0]); // TODO: verbose output
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
        WriteLTEXdata(TA_LTEX3_OUT, cleanup_list_x, cleanup_list_y, cleanup_list_count);
    }

    return 0;
}

/*****************************************************************
** 5. Process3LandData(): Process a LAND record.
*****************************************************************
** LAND (4 bytes) + Length (4 bytes) + X (4 bytes) + Y (4 bytes).
****************************************************************/

int Process3LANDData(char *r, int size, int opt_vclr, int opt_vtex, int cleanup_list_x[], int cleanup_list_y[], int *cleanup_list_count)
{
    int pos = 0,
        nsize = 0,
        vnml_size = 0,
        vnml_pos = 0,
        vhgt_pos = 0,
        current_x = 0,
        current_y = 0;

    unsigned short int ntex[16][16];

    char tmp_land_filename[64];

    FILE *fp_land;

    /*********************************************************
     * Get the (hopefully) INTV  header and X, Y co-ordinates.
     ********************************************************/
    if (strncmp("INTV", r, 4) == 0) {
        current_x = bytes_to_int(r[8], r[9], r[10], r[11]);
        current_y = bytes_to_int(r[12], r[13], r[14], r[15]);
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

        sprintf(tmp_land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, current_x, current_y);

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
                sprintf(tmp_land_filename, "%s/vclr.%d.%d.tmp", TA_TMP_DIR, current_x, current_y);

                if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                        fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                                tmp_land_filename, strerror(errno));
                        exit(1);
            }

            fwrite(r+vhgt_pos+ 4329 + 8, 12675, 1, fp_land);

            fclose(fp_land);
        } else if (strncmp("VTEX", r + pos, 4) == 0 && opt_vtex) {
                sprintf(tmp_land_filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, current_x, current_y);

                if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                        fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                                tmp_land_filename, strerror(errno));
                        exit(1);
            }
            memset(ntex, 0, sizeof(ntex));

            if (vtex3_replace.replace_count > 0) {
                ReplaceVTEX3Textures(r + pos + 8);
            }

            StandardizeTES3VTEX((short unsigned int(*)[16])(r + pos + 8), ntex);

            fwrite(ntex, 512, 1, fp_land);

            fclose(fp_land);
        }

        pos += 8 + nsize;
    }

        if (current_x < min_x) min_x = current_x;
        if (current_x > max_x) max_x = current_x;

        if (current_y < min_y) min_y = current_y;
        if (current_y > max_y) max_y = current_y;

        cleanup_list_x[*cleanup_list_count] = current_x;
        cleanup_list_y[(*cleanup_list_count)++] = current_y;

    return 0;
}

int ReplaceVTEX3Textures(char *vtex)
{
    for (int i = 0; i < 256; i++) {
        short unsigned int tex = (short unsigned int) *(vtex+(2*i));
        for (int j = 0; j < vtex3_replace.replace_count; j++) {
            if (vtex3_replace.old_values[j] == tex) {
                *(vtex+(2*i)) = vtex3_replace.new_values[j];
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


int WriteLTEXdata(char *filename, int cleanup_list_x[], int cleanup_list_y[], int *cleanup_list_count)
{
    int i;

    FILE *fp_lt;

    char FormID[9];

    if ((fp_lt = fopen(filename, "wb")) == 0) {
        fprintf(stderr, "Unable to create my TES3 LTEX reference file (%s): %s\n",
            filename, strerror(errno));
        CleanUp(cleanup_list_x, cleanup_list_y, cleanup_list_count);
        exit(1);
    }

    for (i = 0 ; i < ltex.count; i++) {
        FormIDToString(FormID, ltex.formid[i]);
        fprintf(fp_lt, "%d,%s,%s,%s\n", ltex.texnum[i], ltex.texname[i], ltex.filename[i], FormID);
    }

    fclose(fp_lt);

    return 0;
}

int Process3LTEXData(char *r, int size)
{
    short unsigned int i;
    int pos = 0;
    int nsize;

    int index = 0;

    char lname[128],
         tname[128];

    /*********************************************************
     * Get the (hopefully) SCTX header.
     ********************************************************/

    if (strncmp("NAME", r + pos, 4) == 0) {
        nsize = (int) r[pos+4];
        strncpy(lname, r+pos+8, nsize);
        pos += 8 + nsize;
    }

    if (strncmp("INTV", r + pos, 4) == 0) {
        nsize = (int) r[pos+4];
        memcpy(&index, r+pos+8, 4);
        pos += 8 + nsize;
    }

    if (strncmp("DATA", r + pos, 4) == 0) {
        nsize = (int) r[pos+4];
        strncpy(tname, r+pos+8, nsize);
        // pos += 8 + nsize;
    } else {
        printf("Couldn't find LTEX DATA subrec!\n");
        exit(1);
    }

    for (i = 0; i < ltex.count; i++) {
        if (strcmp(ltex.texname[i], lname) == 0) {
            vtex3_replace.old_values[vtex3_replace.replace_count] = vtex3_replace.myindex+1;
            vtex3_replace.new_values[vtex3_replace.replace_count] = i+1;
            vtex3_replace.replace_count++;
            vtex3_replace.count++;
            break;
        }
    }

    if (i >= ltex.count) { // Not found, so this LTEX record must be added to the list.
        if (GetFormIDForFilename(tname, ltex.texname[ltex.count], ltex.filename[ltex.count], ltex.formid[ltex.count])) {
            ltex.count++;
            ltex.texnum[ltex.count-1] = ltex.count;
        } else {
            fprintf(stderr, "Unable to populate an ltex FormID for %s: Not in my LTEX file!!\n", tname);
            //exit(0);
        }
        vtex3_replace.old_values[vtex3_replace.replace_count] = vtex3_replace.myindex+1;
        vtex3_replace.new_values[vtex3_replace.replace_count] = i+1;
        vtex3_replace.count++;
        vtex3_replace.replace_count++;
    }
    vtex3_replace.myindex++;

        return 0;
}


int ReadLTEX3(char *filename)
{
    char  s[512];
    FILE *fp_lt;

    if ((fp_lt = fopen(filename, "rb")) != 0) {
        while (fgets(s, 512, fp_lt) != NULL) {
            if (s[0] == '#') continue;
            char iname[128],
                 formid_ascii[64]; /* Only 8 should be necessary, but in case the file format is corrupt ... */
            int i = 0,
                p = 0;

            for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; iname[i] = s[i+p], i++);
            iname[i] = '\0';
            int index = atoi(iname);

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; i++);

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; i++);

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; formid_ascii[i] = s[i+p], i++);
            formid_ascii[i] = '\0';

            StringToReverseFormID(formid_ascii, ltex.formid[ltex.count]);
            ltex.texnum[ltex.count] = index;
            ltex.count++;
        }
    } else {
        fprintf(stderr, "\nUnable to open the TES3 LTEX texture to texture this landscape (%s) - this file can be optionally generated when you perform a heightmap export. But, %s.\n",
            filename, strerror(errno));
        fprintf(stderr, "This landscape will NOT be textured!\n\n");
        return 1;
    }

    fclose(fp_lt);

    return 0;
}


void GetVTEX34Cell(unsigned short int vtex[16][16], int ntex[34][34], int cell)
{
    int x, y;
    int xs = 0, ys = 0;

    char formid[4];

    if (cell == 2 || cell == 4) xs = 8;
    if (cell == 3 || cell == 4) ys = 8;

    for (y = 0; y < 34; y++) {
        for (x = 0; x < 34; x++) {
            //ntex[y][x] = vtex[((int) ((float) (y+1) * 0.26))-1+ys][((int) ((float) (x+1) * 0.26))-1+xs];

            if (GetFormIDFromTEXNum(vtex[(int) (((float) y * 0.242))+ys][(int) (((float) x * 0.242))+xs], formid)) {
                memcpy(&ntex[y][x], formid, 4);
            } else {
                memset(&ntex[y][x], 0, 4);
            }
        }
    }
}

int Match34TexturesQuad(int vtex4[34][34], char *vtex_record, int *vtex_size, int quad)
{
    int i, x, y;
    int xs = 0, ys = 0;

    char found = 0,
         vtxt_record[2362]; // Maximum length = 8 x 17 x 17 = 2312 bytes.

    unsigned short int vtxt_pos = 0;
    unsigned short int grid_pos = 0;

    float opacity = 1.0; // Texture opacity, we'll always have it full on.

    struct {
        int count;
        unsigned short int texnum;
        char formid[64][4];
    } tex_list;

    *vtex_size = 0;

    if (quad == 1 || quad == 3) xs = 17;
    if (quad == 2 || quad == 3) ys = 17;

    // Make a list of all the FormIDs contained in the grid (maximum from 8x8 is 64).

    tex_list.count = 0;

    for (y = 0; y < 17; y++) {
        for (x = 0; x < 17; x++) {
            found = 0;
            for (i = 0; i < tex_list.count; i++) {
                if (memcmp(&vtex4[y+ys][x+xs], &tex_list.formid[i], 4) == 0) {
                    found = 1;
                }
            }

            if (!found && vtex4[y+ys][x+xs] != 0) {
                memcpy(tex_list.formid[tex_list.count], &vtex4[y+ys][x+xs], 4);
                tex_list.count++;
                found = 0;
            }
        }
    }

    for (i = 0; i < tex_list.count; i++) {
        if (tex_list.formid[i][0] == 0 &&
            tex_list.formid[i][1] == 0 &&
            tex_list.formid[i][2] == 0 &&
            tex_list.formid[i][3] == 0)
            continue;
        sprintf(vtex_record + *vtex_size, "ATXT%c%c", 8, 0);
        *vtex_size += 6;
        memcpy(vtex_record + *vtex_size, &tex_list.formid[i], 4);
        *vtex_size += 4;
        sprintf(vtex_record + *vtex_size, "%c%c%c%c", quad, 121, i, 0);
        *vtex_size += 4;

        vtxt_pos = 0;

        for (y = 0; y < 17; y++) {
            for (x = 0; x < 17; x++) {
                if (memcmp(&vtex4[y+ys][x+xs], &tex_list.formid[i], 4) == 0) {
                    grid_pos = y*17 + x;
                    memcpy(vtxt_record + vtxt_pos, &grid_pos, 2);
                    sprintf(vtxt_record + vtxt_pos + 2, "%c%c", 0, 0);
                    memcpy(vtxt_record + vtxt_pos + 4, &opacity, 4);
                    vtxt_pos += 8;
                }
            }
        }
        sprintf(vtex_record + *vtex_size, "VTXT");
        memcpy(vtex_record + *vtex_size + 4, &vtxt_pos, 2);
        *vtex_size += 6;

        memcpy(vtex_record + *vtex_size, vtxt_record, vtxt_pos);

        *vtex_size += vtxt_pos;
    }

    return 0;
}

int GetFormIDFromTEXNum(unsigned short int texnum, char *FormID)
{
    int i;

    for (i = 0; i < ltex.count; i++) {
        if (ltex.texnum[i] == texnum) break;
    }

    if (ltex.texnum[i] == texnum) {
        memcpy(FormID, ltex.formid[i], 4);
        return 1;
    } else {
        memset(FormID, 0, 4);
        return 0;
    }
}

int FormIDToString(char *s, char *formid)
{
    sprintf(s, "%2.2X%2.2X%2.2X%2.2X",
        (unsigned char) formid[3],
        (unsigned char) formid[2],
        (unsigned char) formid[1],
        (unsigned char) formid[0]);
    return 0;
}

int StringToReverseFormID(char *s, char *formid)
{
    int j;
    char htmp[3];

    for (j = 0; j < 4; j++) {
        htmp[0] = s[(2*j)];
        htmp[1] = s[(2*j)+1];
        htmp[2] = '\0';

        if (islower(htmp[0])) htmp[0] = toupper(htmp[0]);
        if (islower(htmp[1])) htmp[1] = toupper(htmp[1]);

    formid[3-j] = strtol(htmp, NULL, 16);
    }

    return 0;
}

int GetFormIDForFilename(char *tex_filename, char *ltex_name, char *ltex_filename, char *FormID)
{
    char lname[128],
         tname[128],
         formid_ascii[64], // Only 8 should be necessary, but in case the file format is corrupt ...
         s[512];

    char filename[128];

    FILE *fp_lt;

    sprintf(filename, "%s", TES3_LTEX_DATA_FILE);
    if ((fp_lt = fopen(filename, "rb")) != 0) {
        while (fgets(s, 512, fp_lt) != NULL) {
            if (s[0] == '#') continue;
            int i = 0,
                p = 0;

            for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; i++);

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; lname[i] = s[i+p], i++);
            lname[i] = '\0';

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; tname[i] = s[i+p], i++);
            tname[i] = '\0';

            p += i+1;
            for (i = 0; s[i+p] != ',' && s[i+p] != '\0' && s[i+p] != '\n' && s[i+p] != '\r'; formid_ascii[i] = s[i+p], i++);
            formid_ascii[i] = '\0';

            if (strcmp(tex_filename, tname) == 0) break;
        }
    } else {
        fprintf(stderr, "Unable to open the LTEX texture lookup file (%s): %s\n",
            filename, strerror(errno));
    }

    fclose(fp_lt);

    if (strcmp(tex_filename, tname) == 0) {
        StringToReverseFormID(formid_ascii, FormID);
        strcpy(ltex_name, lname);
        strcpy(ltex_filename, tname);
        return 1;
    } else {
        return 0;
    }
}
