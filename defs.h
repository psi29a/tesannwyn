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

#define APP_NAME   "TES3Annwyn"

#define TA_ESP_OUT "tesannwyn.esp"
#define TA_RAW_OUT "tesannwyn.raw"
#define TA_BMP_OUT "tesannwyn.bmp"
#define TA_CSV_OUT "tesannwyn.csv"
#define TA_VCLR_OUT "tesannwyn-vclr.bmp"
#define TA_VTEX3_OUT "tesannwyn-vtex3.bmp"
#define TA_VTEX4_OUT "tesannwyn-vtex4-%d.bmp"
#define TA_VTEX4_TMP_RAW "ta_vtex4_tmp-%d.raw"
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

#define TES4_LTEX_DATA_FILE "tes4ltex.txt"
#define TA_LTEX4_DIR	"ta4tex"
#define TA_LTEX4_OUT	"tesannwyn-ltex4.dat"

#define	TES_MORROWIND	"Morrowind"
#define	TES_OBLIVION    "Oblivion"
#define	TES_FALLOUT3    "Fallout3"
#define	TES_FALLOUTNV   "FalloutNV"
#define	TES_SKYRIM      "Skyrim"

#define LOD2_STATICS_FILE "tesannwyn_lod2_stat.esp"
#define LOD2_LOD_FILE   "tesannwyn_lod2_cs.esp"
#define LOD2_DIR        "LOD2"
#define LOD2_TEX_DIR    "Textures\\LOD2"

#define MW_CELLSIZE 64
#define OB_CELLSIZE 32

#define TES4_OB_RECORD_SIZE  20
#define TES4_FA3_RECORD_SIZE 24

#define MW_TEXSIZE 16
#define OB_TEXSIZE 34

#define MAX_LAYERS 9 // The CS only supports up to 9 texture layers in TES4 (Oblivion, Fallout 3 & Skyrim).
#define OB_TEXSIZE 34

enum { UNKNOWN, IMPORT, EXPORT };
enum { UNKNOWN_IMAGE, RAW, BMP, CSV };

int opt_mode = UNKNOWN,
    opt_tes_ver = 3,
    opt_bpp = 16,
    opt_cell_data = 0,
    opt_image_type = UNKNOWN_IMAGE,
    opt_sx = 1024,
    opt_sy = 1024,
    opt_x_cell_offset = 0,
    opt_y_cell_offset = 0,
    opt_adjust_height = 0,
    opt_world_FormID = 0,
    opt_rescale = 0,
    opt_vclr = 0,
    opt_limit = 0,
    opt_lower_limit = -2147483647,
    opt_upper_limit = 2147483647,
    opt_grid = -1,
    opt_lod = 0,
    opt_quiet = 0,
    opt_modindex00 = 0,
    opt_usertex = 0,
    opt_ignore_land_upper = -1073741824,
    opt_ignore_land_lower  = 1073741824,
    opt_vtex = 0;

char opt_ignore_land_string[64],
     opt_dimensions_string[48],
     opt_limit_string[48],
     opt_texture[32];

int  total_records = 0;

int  rec_offset = TES4_FA3_RECORD_SIZE;                 // TES4 Record offset. Oblivion uses 20. Fallout3 & Skyrim uses 24.

float opt_scale = 1.0,
      opt_v_overlap = 0.25;

char *opt_tes_mode = TES_MORROWIND;

int total_overflows = 0,
    total_underflows = 0;

int height_stat_min = 1048576,
    height_stat_max = -1048576, // Record the minimum and maximum heights
    height_stat_max_cell_x,
    height_stat_max_cell_y,
    height_stat_min_cell_x,
    height_stat_min_cell_y;

int maxlayer = 0;

char opt_worldspace[128];

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
	short unsigned int old[1024];
	short unsigned int new[1024];
} vtex3_replace;


enum { EXTERIOR, INTERIOR, TRUE, FALSE };

int cleanup_list_x[1048576];
int cleanup_list_y[1048576];
int cleanup_list_count = 0;

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

int min_x = 32768,
    max_x = -32768,
    min_y = 32768,
    max_y = -32768;

char log_message[512];
char input_esp_filename[128];
int  ro = 24;                       // TES4 Record offset. Oblivion uses 20. Fallout3 uses 24.

/***************************
* Just some running totals.
**************************/

int total_cells = 0,                /* Total CELL records found in the file. */
    total_land = 0,                 /* Total LAND records found in the file. */
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


int i = 0,                     /* A loop variable.                                                          */
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
