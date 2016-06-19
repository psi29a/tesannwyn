//
// Created by bcurtis on 08/03/16.
//

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>
#include <ctype.h>
#include "libtesannwyn.h"
#include "defs.h"

// TODO: not thread safe, needs to worked into tes_context
static int min_x = 32768,
        max_x = -32768,
        min_y = 32768,
        max_y = -32768;

tes_context tes_export(char *filename, int bpp) {
    tes_context context;
    context.is_import = 0;

    ltex.count = 0;
    height_stat.max = MIN_HEIGHT;
    height_stat.min = MAX_HEIGHT;

    mkdir(TA_TMP_DIR, 0777);

    Export_TES3_Land(filename);

    Create_RAW(TA_RAW_OUT, bpp);

    return context;
}


void Export_TES3_Land(char *input_esp_filename) {
    char s[40];    /* For storing the 16-byte header. */
    FILE *fpin;    /* Input File Stream (original ESP/ESM).      */

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
        unsigned int size = Bytes_To_Int(s[4], s[5], s[6], s[7]) + 16;

        /* Create some memory space to store the full original record
         * and an additional copy (nr) which may be modified.
         * A cheap hack here adds 2048 bytes to the memory allocated
         * to the new record. This is to cope with a compiler problem
         * (with Cygwin when compiling a static binary) with re-allocating
         * more space (causes problems with free(or)) later.
         * A Morrowind script can now grow by 2K without causing any
         * problems - e.g. That's several hundred position updates in one script. */

        char *or = malloc(size);    /* Pointer to the Original Record. */
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
            Process_TES3_LAND_Data(or + 16, size - 16);
        } else if (strncmp(s, "LTEX", 4) == 0) {
            Process_TES3_LTEX_Data(or + 16);
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
        Write_LTEX_data(TA_LTEX3_OUT);
    }
}

void Write_LTEX_data(char *filename) {
    int i;

    FILE *fp_lt;

    char FormID[9];

    if ((fp_lt = fopen(filename, "wb")) == 0) {
        fprintf(stderr, "Unable to create my TES3 LTEX reference file (%s): %s\n",
                filename, strerror(errno));
        exit(1);
    }

    for (i = 0; i < ltex.count; i++) {
        Form_ID_To_String(FormID, ltex.formid[i]);
        fprintf(fp_lt, "%d,%s,%s,%s\n", ltex.texnum[i], ltex.texname[i], ltex.filename[i], FormID);
    }

    fclose(fp_lt);

}

void Form_ID_To_String(char *s, char *formid) {
    sprintf(s, "%2.2X%2.2X%2.2X%2.2X",
            (unsigned char) formid[3],
            (unsigned char) formid[2],
            (unsigned char) formid[1],
            (unsigned char) formid[0]);
}

void Process_TES3_LTEX_Data(char *r) {
    short unsigned int i;
    int pos = 0;
    size_t nsize;

    int index = 0;

    char lname[128],
            tname[128];

    /*********************************************************
     * Get the (hopefully) SCTX header.
     ********************************************************/

    if (strncmp("NAME", r + pos, 4) == 0) {
        nsize = (size_t) r[pos + 4];
        strncpy(lname, r + pos + 8, nsize);
        pos += 8 + nsize;
    }

    if (strncmp("INTV", r + pos, 4) == 0) {
        nsize = (size_t) r[pos + 4];
        memcpy(&index, r + pos + 8, 4);
        pos += 8 + nsize;
    }

    if (strncmp("DATA", r + pos, 4) == 0) {
        nsize = (size_t) r[pos + 4];
        strncpy(tname, r + pos + 8, nsize);
    } else {
        printf("Couldn't find LTEX DATA subrec!\n");
        exit(1);
    }

    for (i = 0; i < ltex.count; i++) {
        if (strcmp(ltex.texname[i], lname) == 0) {
            vtex3_replace.old_values[vtex3_replace.replace_count] = vtex3_replace.myindex + (unsigned short) 1;
            vtex3_replace.new_values[vtex3_replace.replace_count] = i + (unsigned short) 1;
            vtex3_replace.replace_count++;
            vtex3_replace.count++;
            break;
        }
    }

    if (i >= ltex.count) { // Not found, so this LTEX record must be added to the list.
        if (Get_Form_ID_For_Filename(tname, ltex.texname[ltex.count], ltex.filename[ltex.count],
                                     ltex.formid[ltex.count])) {
            ltex.count++;
            ltex.texnum[ltex.count - 1] = ltex.count;
        } else {
            fprintf(stderr, "Unable to populate an ltex FormID for %s: Not in my LTEX file!!\n", tname);
            //exit(0);
        }
        vtex3_replace.old_values[vtex3_replace.replace_count] = vtex3_replace.myindex + (unsigned short) 1;
        vtex3_replace.new_values[vtex3_replace.replace_count] = i + (unsigned short) 1;
        vtex3_replace.count++;
        vtex3_replace.replace_count++;
    }
    vtex3_replace.myindex++;
}

/*****************************************************************
** 5. Process3LandData(): Process a LAND record.
*****************************************************************
** LAND (4 bytes) + Length (4 bytes) + X (4 bytes) + Y (4 bytes).
****************************************************************/

void Process_TES3_LAND_Data(char *r, int size) {
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
        current_x = Bytes_To_Int(r[8], r[9], r[10], r[11]);
        current_y = Bytes_To_Int(r[12], r[13], r[14], r[15]);
        pos += 16;
    } else {
        fprintf(stderr,
                "WARNING: I couldn't find the INTV header for this TES3 LAND record (got [%c%c%c%c] - ignoring record.\n",
                r[0], r[1], r[2], r[3]);
    }

    if (strncmp("DATA", r + pos, 4) == 0) {
        nsize = Bytes_To_Int(r[pos + 4], r[pos + 5], r[pos + 6], r[pos + 7]);
        pos += 8 + nsize;
    }

    if (strncmp("VNML", r + pos, 4) != 0) {

    }

    vnml_pos = pos;
    vnml_size = Bytes_To_Int(r[vnml_pos + 4], r[vnml_pos + 5], r[vnml_pos + 6], r[vnml_pos + 7]);
    vhgt_pos = vnml_pos + 8 + vnml_size;

    if (strncmp("VHGT", r + vhgt_pos, 4) != 0) {
        fprintf(stderr, "Unable to find VHGT!!!\n");

    }

    sprintf(tmp_land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, current_x, current_y);

    if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
        fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                tmp_land_filename, strerror(errno));
        exit(1);
    }

    // Write out the VHGT data (4232 bytes)
    fwrite(r + vhgt_pos + 8, 4232, 1, fp_land);

    fclose(fp_land);

    pos = vhgt_pos + 4240 + 89;

    while (pos < size) {
        memcpy(&nsize, r + pos + 4, 4);

        if (strncmp("VTEX", r + pos, 4) == 0) {
            sprintf(tmp_land_filename, "%s/vtex3.%d.%d.tmp", TA_TMP_DIR, current_x, current_y);

            if ((fp_land = fopen(tmp_land_filename, "wb")) == 0) {
                fprintf(stderr, "Unable to create a temporary cell file for writing, %s: %s\n",
                        tmp_land_filename, strerror(errno));
                exit(1);
            }
            memset(ntex, 0, sizeof(ntex));

            if (vtex3_replace.replace_count > 0) {
                Replace_VTEX3_Textures(r + pos + 8);
            }

            Standardize_TES3_VTEX((short unsigned int (*)[16]) (r + pos + 8), ntex);

            fwrite(ntex, 512, 1, fp_land);

            fclose(fp_land);
        }

        pos += 8 + nsize;
    }

    if (current_x < min_x) min_x = current_x;
    if (current_x > max_x) max_x = current_x;

    if (current_y < min_y) min_y = current_y;
    if (current_y > max_y) max_y = current_y;
}

void Replace_VTEX3_Textures(char *vtex) {
    for (int i = 0; i < 256; i++) {
        short unsigned int tex = (short unsigned int) *(vtex + (2 * i));
        for (int j = 0; j < vtex3_replace.replace_count; j++) {
            if (vtex3_replace.old_values[j] == tex) {
                *(vtex + (2 * i)) = (char) vtex3_replace.new_values[j];
            }
        }
    }
}

void Standardize_TES3_VTEX(unsigned short int vtex[16][16], unsigned short int ntex[16][16]) {
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

        if (q == 0) {
            qx = 0;
            qy = 0;
            vtpointer = 0;
        }
        else if (q == 1) {
            qx = 8;
            qy = 0;
            vtpointer = 64;
        } // 128;
        else if (q == 2) {
            qx = 0;
            qy = 8;
            vtpointer = 256;
        }
        else if (q == 3) {
            qx = 8;
            qy = 8;
            vtpointer = 320;
        }  // 384;

        for (i = 0; i < 2; i++) {
            vtpointer += (i * 64);
            for (j = 0; j < 2; j++) {
                for (k = 0; k < 4; k++) {
                    for (l = 0; l < 4; l++) {
                        memcpy(&ntex[qy + k + (4 * i)][qx + l + (4 * j)], vt + vtpointer, 2);
                        vtpointer += 2;
                    }
                }
            }
        }
    }
}

/****************************************************
** Bytes_To_Int():
**
** Convert 4-bytes in to an Integer (little endian).
** Most commonly called for converting the size of a
** record or field in to a usable integer.
***************************************************/
unsigned int Bytes_To_Int(char b1, char b2, char b3, char b4) {
    return (unsigned int) ((unsigned char) b1
                           + (256 * (unsigned char) b2)
                           + (65536 * (unsigned char) b3)
                           + (16777216 * (unsigned char) b4));
}

int Get_Form_ID_For_Filename(char *tex_filename, char *ltex_name, char *ltex_filename, char *FormID) {
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

            for (i = 0; s[i + p] != ',' && s[i + p] != '\0'; i++);

            p += i + 1;
            for (i = 0; s[i + p] != ',' && s[i + p] != '\0'; lname[i] = s[i + p], i++);
            lname[i] = '\0';

            p += i + 1;
            for (i = 0;
                 s[i + p] != ',' && s[i + p] != '\0' && s[i + p] != '\n' && s[i + p] != '\r'; tname[i] = s[i + p], i++);
            tname[i] = '\0';

            p += i + 1;
            for (i = 0;
                 s[i + p] != ',' && s[i + p] != '\0' && s[i + p] != '\n' && s[i + p] != '\r'; formid_ascii[i] = s[i +
                                                                                                                  p], i++);
            formid_ascii[i] = '\0';

            if (strcmp(tex_filename, tname) == 0) break;
        }
    } else {
        fprintf(stderr, "Unable to open the LTEX texture lookup file (%s): %s\n",
                filename, strerror(errno));
    }

    fclose(fp_lt);

    if (strcmp(tex_filename, tname) == 0) {
        String_To_Reverse_FormID(formid_ascii, FormID);
        strcpy(ltex_name, lname);
        strcpy(ltex_filename, tname);
        return 1;
    } else {
        return 0;
    }
}

void String_To_Reverse_FormID(char *s, char *form_id) {
    unsigned short j;
    char htmp[3];

    for (j = 0; j < 4; j++) {
        htmp[0] = s[(2 * j)];
        htmp[1] = s[(2 * j) + 1];
        htmp[2] = '\0';

        if (islower(htmp[0])) htmp[0] = (char) toupper(htmp[0]);
        if (islower(htmp[1])) htmp[1] = (char) toupper(htmp[1]);

        form_id[3 - j] = (char) strtol(htmp, NULL, 16);
    }
}


// Creates a RAW or BMP (opt_image_type) image called output_filename.
// It reads the VHGT data from files called "landx.y.tmp" where x and y are the range
// of co-ordinates matched from the ESP in the preceding function.
void Create_RAW(char *output_filename, int bpp) {
    int i, j = 0;
    int c;
    int x, y;
    float global_height_offset;
    int height = 0;
    int col_offset = 0,
            row_sum = 0;
    int ppc;    // Points per cell (64 for TES3, 32 for TES4).

    char h8int = 0;
    short int h16int = 0;
    int h32int = 0;
    int bmp_offset = 0;
    int xdim, ydim;

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

    bmp_offset = 0;

    int Bp = bpp / 8;

    xdim = ppc * ((max_x - min_x) + 1);
    ydim = ppc * ((max_y - min_y) + 1);

    printf("Image file has dimensions: %dx%d (%d-bit)\n", xdim, ydim, bpp);

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {

            memset(&vhgt_data, 0, 16384);

            sprintf(land_filename, "%s/land.%d.%d.tmp", TA_TMP_DIR, x, y);
            if ((fp_land = fopen(land_filename, "rb")) == 0) {

                /************************************************************
                 * If there's no land file exported, create a seabed instead.
                 ***********************************************************/

                for (c = 1; c < ppc + 1; c++) {
                    fseek(fp_o, bmp_offset + ((y - min_y) * xdim * Bp * ppc) + ((x - min_x) * Bp * ppc) +
                                (0 * Bp * xdim * ppc) + ((c - 1) * xdim * Bp), SEEK_SET);
                    for (i = 1; i < ppc + 1; i++) {
                        height = -256;

                        if (bpp == 32) {
                            h32int = height;
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

                if (!fread(land_data, 16384, 1, fp_land) && !feof(fp_land)) {
                    fprintf(stderr, "%s\n", strerror(errno));
                    exit(1);
                }

                memcpy(vhgt_data, land_data, 4229);

                memcpy(&global_height_offset, &vhgt_data, 4);

                col_offset = vhgt_data[4]; // Gradient Grid point (0, 0) - usually 0, but ...
                for (i = 0; i < ppc + 1; i++) {
                    if (i > 0) {
                        fseek(fp_o, bmp_offset + ((y - min_y) * xdim * Bp * ppc) + ((x - min_x) * Bp * ppc) +
                                    (0 * Bp * xdim * ppc) + ((i - 1) * xdim * Bp), SEEK_SET);
                        col_offset += vhgt_data[4 + (i * (ppc + 1))];
                    }
                    row_sum = (int) (global_height_offset) + (col_offset);

                    for (j = 0; j < ppc + 1; j++) {

                        if (j > 0) {
                            row_sum += vhgt_data[4 + (i * (ppc + 1)) + j];
                        }

                        height = row_sum;

                        if (height < height_stat.min) {
                            height_stat.min = height;
                            height_stat.cell_min_x = x;
                            height_stat.cell_min_y = y;
                        } else if (height > height_stat.max) {
                            height_stat.max = height;
                            height_stat.cell_max_x = x;
                            height_stat.cell_max_y = y;
                        }

                        if (i > 0 && j > 0) {
                            if (bpp == 32) {
                                h32int = height;
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
}