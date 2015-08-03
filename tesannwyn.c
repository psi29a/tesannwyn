/*******************************************************************************************
 ** TESAnnwyn: A TES3/TES4 height map importer/exporter (to & from RAW or BMP).
 **
 ** Paul Halliday: 31-Dec-2006 
 **
 ** This is entirely my own work. No borrowed code. All reverse engineering has been
 ** researched by myself.
 **
 ** License: GNU (Copy, modify, distribute as you like. 
 ** Please just credit me if you borrow code. ;)
 ***************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

// Unistd is not usually included on Windows so you may need to comment this out:
#include <unistd.h>
#include <getopt.h>

//#include "defs.h"
#include "funcs.h"

#include "common.c"
#include "tes3_import.c"
#include "tes3_export.c"
#include "tes3_vtex.c"

int main(int argc, char *argv[])
{
    int argn = 0;

    char c,
         s[256];

    FILE *fp_in;

    printf("********************************************************\n"
           "* TESAnnwyn: A RAW/BMP heightmap importer/exporter     *\n"
               "* for TES3/TES4 Morrowind/Oblivion/Fallout3/Skyrim.    *\n"
           "********************************************************\n\n");

        /***********************************
         * Parse the command line arguments.
         **********************************/

    argn = argc;

        while ((c = getopt(argc, argv, "i:e:d:x:y:m:n:s:h:p:b:o:w:t:u:F:E:T:l:0LzcCgrvVq")) != EOF) {
                switch (c) {
            case 'i':
                opt_mode = IMPORT;
                                if (optarg) {
                    if (isdigit(optarg[0])) {
                                            opt_tes_ver = atoi(optarg);
                        if (opt_tes_ver == 3) {
                            opt_tes_mode = TES_MORROWIND;
                        } else {
                            opt_tes_mode = TES_OBLIVION;
                        }
                                            argn -= 2;
                    } else if (strcasecmp(optarg, TES_MORROWIND) == 0) {
                        opt_tes_ver = 3;
                        opt_tes_mode = TES_MORROWIND;
                                            argn -= 2;
                    } else if (strcasecmp(optarg, TES_OBLIVION) == 0) {
                        opt_tes_ver = 4;
                        opt_tes_mode = TES_OBLIVION;
                                            argn -= 2;
                    } else if (strcasecmp(optarg, TES_FALLOUT3) == 0) {
                        opt_tes_ver = 5;
                        opt_tes_mode = TES_FALLOUT3;
                                            argn -= 2;
                    } else if (strcasecmp(optarg, TES_FALLOUTNV) == 0) {
                        opt_tes_ver = 5;
                        opt_tes_mode = TES_FALLOUTNV;
                                            argn -= 2;
                    } else if (strcasecmp(optarg, TES_SKYRIM) == 0) {
                        opt_tes_ver = 5;
                        opt_tes_mode = TES_SKYRIM;
                                            argn -= 2;
                                    } else
                                            ShowUsageExit(argv[0]);
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'e':
                opt_mode = EXPORT;
                                if (optarg) {
                                        opt_tes_ver = atoi(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'c':
                opt_vclr = 1;
                break;
            case 'C':
                opt_cell_data = 1;
                break;
            case '0':
                opt_modindex00 = 1;
                break;
            case 'L':
                opt_lod = 1;
                break;
            case 'g':
                opt_grid = 0;
                break;
            case 'F':
                                if (optarg) {
                                        opt_world_FormID = atoi(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'T':
                                if (optarg) {
                                        opt_vtex = atoi(optarg);
                    if (opt_vtex <= 3) {
                        opt_vtex = 3;
                    } else if (opt_vtex >= 5) {
                        opt_vtex = 5;
                        rec_offset = TES4_FA3_RECORD_SIZE;
                    } else {
                        rec_offset = TES4_OB_RECORD_SIZE;
                    }
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'E':
                                if (optarg) {
                                        opt_v_overlap = atof(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'b':
                                if (optarg) {
                                        opt_bpp = atoi(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'p':
                opt_mode = EXPORT;
                                if (optarg) {
                                        opt_image_type = atoi(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
            case 'q':
                opt_quiet = 1;
                break;
            case 'w':
                opt_mode = EXPORT;
                                if (optarg) {
                                        strcpy(opt_worldspace, optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                break;
                        case 'x':
                                if (optarg) {
                                        opt_x_cell_offset = atoi(optarg);
                                        argn -= 2;
                                } else
                                        ShowUsageExit(argv[0]);
                                break;
                        case 'y':
                                if (optarg) {
                                        opt_y_cell_offset = atoi(optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'z':
                opt_vtex = 3; //atof(optarg);
                                break;
            case 't':
                                if (optarg) {
                                        strcpy(opt_texture, optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
            case 'd':
                                if (optarg) {
                                        strcpy(opt_dimensions_string, optarg);
                    DecodeDimensions(opt_dimensions_string, &opt_sx, &opt_sy);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'm':
                fprintf(stderr, "\nPlease note: The '-m' and '-n' options have been replaced by a single '-d' option.\ne.g. To specify dimensions of 1280x1024, please use: '-d 1280x1024' instead.\n\n");
                exit(1);
                                if (optarg) {
                                        opt_sx = atoi(optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'n':
                fprintf(stderr, "\nPlease note: The '-m' and '-n' options have been replaced by a single '-d' option.\ne.g. To specify dimensions of 1280x1024, please use: '-d 1280x1024' instead.\n\n");
                exit(1);
                                if (optarg) {
                                        opt_sy = atoi(optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 's':
                                if (optarg) {
                                        opt_scale = (float) atof(optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'h':
                                if (optarg) {
                                        opt_adjust_height = atoi(optarg);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'l':
                                if (optarg) {
                                        strcpy(opt_limit_string, optarg);
                    DecodeLimits(opt_limit_string, &opt_lower_limit, &opt_upper_limit);
                                        argn -= 2;
                    opt_limit = 1;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                break;
                        case 'o':
                                if (optarg) {
                                        strcpy(opt_ignore_land_string, optarg);
                    DecodeOptIgnoreLand(opt_ignore_land_string, &opt_ignore_land_lower, &opt_ignore_land_upper);
                                        argn -= 2;
                                } else {
                                        ShowUsageExit(argv[0]);
                                }
                                break;
                        case 'u':
                                if (optarg) {
                    opt_usertex = 1;
                                        strcpy(opt_limit_string, optarg);
                    DecodeUserTexture(opt_limit_string, usertex.name, usertex.fname);
                                        argn -= 2;
                                } else {
                                    ShowUsageExit(argv[0]);
                                }
                break;
                        case 'r':
                opt_rescale = 1;
                break;
                        case 'v':
                        case 'V':
                                printf("Program Name: %s\n"
                                       "Version:      0.23\n"
                                       "Date:         Dec-2006 -> Nov-2011\n"
                                       "Author:       Paul Halliday (Ocean_Lightwave@yahoo.co.uk)\n"
                    "Website: oceanlightwave.com\n",
                                        APP_NAME);
                                exit(0);
                        default:
                                ShowUsageExit(argv[0]);
                }
        }

    if (argn < 2) {
        ShowUsageExit(argv[0]);
    }

    DecodeFilenames(argv[argc-1]);

    // Double check the option file details: Is the input file a TES3, TES4, BMP, etc.

    /* ************************************
     * File type check (BMP, TES3/TES4) ...
     * ************************************
     */

    if ((fp_in = fopen(input_files.filename[0], "rb")) == 0) {
        fprintf(stderr, "Cannot open %s for reading: %s\n",
            input_files.filename[0], strerror(errno));
        exit(1);
    }
    fread(s, 4, 1, fp_in);
    if (strncmp("TES3", s, 4) == 0) {
        printf("Looks like a TES3 file. Assuming want to export this to an image.\n");
        opt_tes_ver = 3;
        opt_mode = EXPORT;
    } else if (strncmp("TES4", s, 4) == 0) {
        opt_mode = EXPORT;
        fseek(fp_in, TES4_FA3_RECORD_SIZE, SEEK_SET);
        fread(s, 4, 1, fp_in);
        if (isalpha(s[0]) && isalpha(s[1]) && isalpha(s[2]) && isalpha(s[3])) {
            printf("Looks like a TES4 (Fallout 3 NV / Skyrim) file. Assuming you want this exported to an image.\n");
            opt_tes_ver = 5;
        } else {
            printf("Looks like a TES4 (Oblivion / Fallout 3) file. Assuming you want this exported to an image.\n");
            opt_tes_ver = 4;
        }
    } else if (strncmp("BM", s, 2) == 0) {
        printf("This is a BMP. You must want to import this image.\n");
        opt_mode = IMPORT;
        opt_image_type = BMP;
    } else {
        opt_mode = IMPORT;
        opt_image_type = RAW;
    }

    if      (opt_tes_ver == 4) rec_offset = TES4_OB_RECORD_SIZE;
        else if (opt_tes_ver == 5) rec_offset = TES4_FA3_RECORD_SIZE;

    fclose(fp_in);

    printf("Using a height scale factor of: %f.\n", opt_scale);
    printf("Using a height offset value of: %d.\n", opt_adjust_height);

    if (opt_limit && opt_mode == IMPORT) {
        printf("Imported land below %d game units and above %d game units will be flattened.\n",
            opt_lower_limit, opt_upper_limit);
        opt_upper_limit /= 8;
        opt_lower_limit /= 8;
    }

    if (opt_texture[0] != '\0') {
        if (opt_tes_ver <= 3) {
            printf("Landscape will be textured with Morrowind texture number '%s'\n\n", opt_texture);
        } else {
            printf("Landscape will be textured with Oblivion/Fallout3/Skyrim texture FormID '%s'\n\n", opt_texture);
        }
    }
    if (opt_usertex) {
        if (opt_tes_ver >= 4) {
            printf("Landscape will be textured with the texture '%s'."
            " Make sure you put this file in your Oblivion or Fallout3 or Skyrim \"Data\\Textures\\Landscape folder\".\n\n",
                usertex.fname);
        }
    }

    if (opt_mode == IMPORT) {
        if (opt_ignore_land_lower < 1073741824 && opt_ignore_land_upper > -1073741824) {
            printf("Landscape only containing heights between %d and %d will not be imported.\n",
                opt_ignore_land_lower, opt_ignore_land_upper);
        }
        ImportImage(input_files.filename[0]);
    } else {
        ExportImages();
    }

    return 0;
}


int DecodeOptIgnoreLand(char *s, int *opt_ignore_land_lower, int *opt_ignore_land_upper)
{
    int i;
    int p = 0;
    char tmp[64];

    if (s[0] == '-') p++;

    for (i = 0; s[i+p] != '-' && s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    *opt_ignore_land_lower = atoi(tmp);
    if (s[0] == '-') *opt_ignore_land_lower *= -1;

    p += i+1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    if (tmp[0] == '\0') {
        //printf("Fixing.\n");
        *opt_ignore_land_upper = *opt_ignore_land_lower;
    } else {
        *opt_ignore_land_upper = atoi(tmp);
    }

    return 0;
}

int DecodeLimits(char *s, int *opt_lower_limit, int *opt_upper_limit)
{
    int i;
    int p = 0;
    char tmp[64];

    for (i = 0; s[i+p] != ',' && s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    *opt_lower_limit = atoi(tmp);

    p += i+1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    if (tmp[0] == '\0') {
        //printf("Fixing.\n");
        *opt_upper_limit = 2147483647;
    } else {
        *opt_upper_limit = atoi(tmp);
    }

    return 0;
}

int DecodeDimensions(char *s, int *opt_sx, int *opt_sy)
{
    int i;
    int p = 0;
    char tmp[64];

    for (i = 0; s[i+p] != 'x' && s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    *opt_sx = atoi(tmp);

    p += i+1;
    for (i = 0; s[i] != '\0'; tmp[i] = s[i+p], i++);
    tmp[i] = '\0';

    *opt_sy = atoi(tmp);

    return 0;
}

int DecodeFilenames(char *s)
{
    int i = 0,
        p = 0;

    input_files.count = 0;

    while (s[p] != '\0') {
        for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; input_files.filename[input_files.count][i] = s[i+p], i++);
        input_files.filename[input_files.count][i] = '\0';
        input_files.count++;

        if (s[i+p] == '\0') break;
        p += i + 1;
    }

    return 0;
}

int DecodeUserTexture(char *s, char *name, char *fname)
{
    int i = 0,
        p = 0;


    for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; name[i] = s[i+p], i++);

    name[i+p] = '\0';
    p+= i + 1;

    for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; fname[i] = s[i+p], i++);
    fname[i+p] = '\0';

    return 0;
}

