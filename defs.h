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
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define APP_NAME   "TESAnnwyn"

#define TA_ESP_OUT "tesannwyn.esp"
#define TA_RAW_OUT "tesannwyn.raw"
#define TA_BMP_OUT "tesannwyn.bmp"
#define TA_CSV_OUT "tesannwyn.csv"
#define TA_VCLR_OUT "tesannwyn-vclr.bmp"
#define TA_VTEX3_OUT "tesannwyn-vtex3.bmp"
#define TA_VCLR_IN  "tesannwyn-vclr.bmp"
#define TA_CELL_BMP "tesannwyn-cells.bmp"
#define TA_CELL_TMP "tesannwyn-cells.tmp"
#define TA_CELL_IN  "tesannwyn-cells.dat"

#define TA_DEFAULT_EXPORT_WORLDSPACE "TESAnnwyn"
#define TA_DEFAULT_IMPORT_WORLDSPACE "TESAnnwyn"
#define TA_TMP_RAW "ta_tmp.raw"
#define TA_TMP_VCLR_RAW "ta_vclr_tmp.raw"
#define TA_TMP_BMP "ta_tmp2.raw"
#define TA_TMP_DIR "annwyn.tmp"

#define TES3_LTEX_DATA_FILE "tes3ltex.txt"
#define TA_LTEX3_OUT	"tesannwyn-ltex3.dat"

#define MW_CELLSIZE 64

#define MW_TEXSIZE 16

#define MAX_LAYERS 9 // The CS only supports up to 9 texture layers in TES4 (Oblivion, Fallout 3 & Skyrim).

enum { UNKNOWN, IMPORT, EXPORT };
enum { UNKNOWN_IMAGE, RAW, BMP, CSV };

#define	TES_MORROWIND	"Morrowind"

struct {
    char name[256];
    char fname[256];
} usertex;

struct {
    int count;
    char filename[256][128];
} input_files;

struct {
    int count;
    int replace_count;
    short unsigned int myindex;
    short unsigned int old_values[1024];
    short unsigned int new_values[1024];
} vtex3_replace;


enum { EXTERIOR, INTERIOR, TRUE, FALSE };

struct cell_data {
        int size;
        char name[1024];
        char region_name[1024];
        int current_x;
        int current_y;
        int new_x;
        int new_y;
        int type;
        int save;
        int copy;
} cell;

/***************************************************************
* The rules array is made global, only because it saves sending
* pointers between the ReadRules and ParseRules functions.
**************************************************************/

char rule[5][1024];

static int min_x = 32768,
    max_x = -32768,
    min_y = 32768,
    max_y = -32768;

char log_message[512];
char input_esp_filename[128];

/***************************
* Just some running totals.
**************************/

static int total_cells = 0,                /* Total CELL records found in the file. */
    total_records_changed = 0,      /* LAND or CELL records changed/copied.  */
    total_cells_copied = 0,
    total_land_copied = 0,
    total_worlds = 0,
    total_objects = 0,              /* Total objects found in the file.      */
    total_objects_changed = 0,      /* Total objects changed/copied.         */
    total_scripts = 0,              /* Total SCPT records found in the file. */
    total_scripts_changed = 0,      /* Total SCPT records modified.          */
    total_dialogs = 0,              /* Total INFO scripts found in the file. */
    total_dialogs_changed = 0;      /* Total INFO records modified.          */

char *last_cell_data;


static int i = 0,                     /* A loop variable.                                                          */
    verbose_mode = 0,          /* Command line option: Option to produce an extensive log file.             */
    list_cell_mode = 0,        /* Command line option: Option to produce a file listing the plugin's cells. */
    show_rules_mode = 0,       /* Command line option: Option to display verbose form of rules then exit.   */
    modify_script_mode = 1,    /* Command line option: Option to modify scripts if necessary: Default on.   */
    generate_specific_esp = 0, /* Command line option: Option to create a new ESP only containing changes.  */
    pass = 0,                  /* Current file pass. Tesfaith runs through the ESP/ESM in 2 passes.         */
    num_scripts_to_recompile=0,/* Record the number of modified scripts that'll need recompiling in TESCS.  */
    last_cell_size = 0;

#define TMP_CELL_FILENAME               "cell%d.tmp"

struct {
        unsigned short int count;
        unsigned short int texnum[2048];
        char texname[2048][64];
        char filename[2048][64];
        char formid[2048][4];
} ltex;

char t4layer[9][34][34][3]; // 9 Layers of 34x34 pixels, 3 bytes per pixel.

#endif /* DEFINITIONS_H */
