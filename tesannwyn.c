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

#include "defs.h"
#include "funcs.h"

#include "common.c"
#include "tes3_import.c"
#include "tes3_export.c"
#include "tes3_vtex.c"

#include "main.c"
