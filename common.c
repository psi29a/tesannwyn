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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "defs.h"

int ExportImages();
int Process4CELLData(char *r, int size);
int Process4LANDData(char *r, int size);
int ExportTES4Land(char *input_esp_filename, char *worldspace, int bpp);
int Process4LTEXData(char *r, int size);
int Process4WRLDData(char *r, int size);
int bytes_to_int(char b1, char b2, char b3, char b4);
int DecompressZLIBStream(char *input, int input_size, char output[], int *output_size);
int CompressZLIBStream(char *input, int input_size, char output[], int *output_size, int compress_level);
int CleanUp();
int WriteBMPHeader(FILE *fp_out, int sx, int sy, int bpp);
int WriteBMPGreyScaleHeader(FILE *fp_out, int sx, int sy, int bpp);
int HumptyImage(char *output_filename, int opt_image_type, int opt_tes_ver, int bpp);
int RescaleGreyScale(char *output_filename, int opt_image_type, int opt_tes_ver, int bpp);
int FixTES3HeaderSize(char *filename, int num_records) ;
int HumptyVCLR(char *output_filename, int opt_tes_ver);
int HumptyVTEX3(char *output_filename, int opt_tes_ver, int layer);
int bytes_to_int(char b1, char b2, char b3, char b4);

int CleanUp()
{
    int i;

    char filename[128];

    for (i = 0; i < cleanup_list_count; i++) {
        sprintf(filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
        sprintf(filename, "%s/vclr.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
        sprintf(filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
        unlink(filename);
        if (opt_lod) {
            sprintf(filename, "%s/normals.%d.%d.tmp", TA_TMP_DIR, cleanup_list_x[i], cleanup_list_y[i]);
            unlink(filename);
        }
    }

    if (opt_vtex && opt_tes_ver >= 4) {
                for (i = 0; i < 9; i++) {
            sprintf(filename, "%s/%d", TA_TMP_DIR, i);
            //rmdir(filename, 0777);
                }
    }

    rmdir(TA_TMP_DIR);

    return 0;
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
    int i;

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
            0xFD, 0x00, 0xFE, 0xFE, 0xFE, 0x00, 0xFF, 0xFF, 0xFF, 0x00
    };

    i = (sx*sy*4)+1078;

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

int HumptyImage(char *output_filename, int opt_image_type, int opt_tes_ver, int bpp)
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

    if (opt_lod) {
        sprintf(land_filename, "%s", LOD2_DIR);
        mkdir(land_filename, 0777);
        sprintf(land_filename, "%s/%s", LOD2_DIR, opt_worldspace);
        mkdir(land_filename, 0777);
        for (i = 1; i < 5; i++) {
            sprintf(land_filename, "%s/%s/%d", LOD2_DIR, opt_worldspace, i);
            mkdir(land_filename, 0777);
        }
    }


    // Recalculate min and max x and y values.

    if ((fp_o = fopen(output_filename, "wb")) == 0) {
        fprintf(stderr, "Cannot create a new exported ESP file (%s): %s\n",
            output_filename, strerror(errno));
        exit(1);
    }

    if (opt_tes_ver <= 3) {
        ppc = MW_CELLSIZE;
    } else {
        ppc = OB_CELLSIZE;
    }

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
                if (opt_tes_ver == 3) {
                    memcpy(vhgt_data, land_data, 4229);
                } else {
                    memcpy(vhgt_data, land_data, 1093);
                }

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

int RescaleGreyScale(char *output_filename, int opt_image_type, int opt_tes_ver, int bpp)
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

    if (opt_tes_ver == 3) {
        ppc = MW_CELLSIZE;
    } else {
        ppc = OB_CELLSIZE;
    }

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            memset(&vhgt_data, 0, 16384);

            sprintf(land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, x, y);
            if ((fp_land = fopen(land_filename, "rb")) != 0) {
                fread(land_data, 16384, 1, fp_land);

                if (opt_tes_ver == 3) {
                    memcpy(vhgt_data, land_data, 4229);
                } else {
                    memcpy(vhgt_data, land_data, 1093);
                }

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
                                    h_high = height; height_stat_max_cell_x = x;
                                    height_stat_max_cell_y = y;
                                }
                                if (h_low  > height) {
                                    h_low = height; height_stat_min_cell_x = x;
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

int FixTES3HeaderSize(char *filename, int num_records)
{
    char csize[4];
    struct stat fs;

    FILE *fp;

    if (stat(filename, &fs) == -1) {
        fprintf(stderr, "Unable to stat output file %s to fix the header record count. Weird!: %s\n",
            filename, strerror(errno));
        return -1;
    }

    printf("Writing %d records\n", num_records);
    memcpy(csize, &num_records, 4);

    if ((fp = fopen(filename, "ab")) == 0) {
        fprintf(stderr, "Unable to re-open %s to fix the header record count. Weird!: %s\n",
            filename, strerror(errno));
        return -1;
    } else {
        fseek(fp, 320, SEEK_SET);
        fprintf(fp, "%c%c%c%c", csize[0], csize[1], csize[2], csize[3]);
        fclose(fp);
    }

    return 0;
}

int HumptyVCLR(char *output_filename, int opt_tes_ver)
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

    if (opt_tes_ver <= 3) {
        ppc = MW_CELLSIZE;
    } else {
        ppc = OB_CELLSIZE;
    }

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

                if (opt_tes_ver == 3) {
                    memcpy(vclr_data, land_data, 12675);
                } else {
                    memcpy(vclr_data, land_data, 3267);
                }

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

int HumptyVTEX3(char *output_filename, int opt_tes_ver, int layer)
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

    if (opt_tes_ver <= 3) {
        ppc = 16;
        Bp = 2;
        ipad = 0;
        bsize = (ppc+ipad)*ppc*Bp;
        bmp_offset = 0;
        WriteBMPHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 16);
        printf("TES3 VTEX placement image has dimensions: %dx%d (%d-bit)\n", ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 8*Bp);
    } else {
        ppc = 34;
        Bp = 3;
        bmp_offset = 54;
//		ipad = (int) (4.0f*( ((float) (ppc*Bp/4)) - (float)((int) (ppc*Bp/4)) ));
        ipad = 2;
        bsize = ppc*(ppc*Bp+ipad);
        WriteBMPHeader(fp_o, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 24);
        printf("TES4 VTEX placement image called %s (layer %d) has dimensions: %dx%d (%d-bit)\n", output_filename, layer, ppc*((max_x-min_x)+1), ppc*((max_y-min_y)+1), 8*Bp);
    }

    xdim = ppc*((max_x-min_x)+1);

    opad = (int) (4*( ((float) (xdim*Bp) /4) - (float) ((int) ((xdim)*Bp/4)) )) ;

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            if (opt_tes_ver == 3) {
                sprintf(land_filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, x, y);
            } else {
                sprintf(land_filename, "%s/%d/tex.%d.%d.bmp", TA_TMP_DIR, layer, x, y);
            }

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

    if (opt_tes_ver >= 4) {

        if ((fp_o = fopen(TA_LTEX4_OUT, "wb")) == 0) {
            fprintf(stderr, "Unable to create TES4 Texture Placement<->FormID Lookup file (%s) for writing: %s\n",
                TA_LTEX4_OUT, strerror(errno));
        } else {
            for (i = 0; i < ltex.count; i++) {
                fprintf(fp_o, "%d,%2.2X%2.2X%2.2X%2.2X\n", i,
                    (unsigned char) ltex.formid[i][3],
                    (unsigned char) ltex.formid[i][2],
                    (unsigned char) ltex.formid[i][1],
                    (unsigned char) ltex.formid[i][0]);
            }
            fclose(fp_o);
        }
    }

    return 0;
}

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

