TES3Annwyn Source Code: A TES3 (Morrowind) only version of TESAnnwyn
====================================================================

This is the source code to TES3Annwyn, a slightly cut down version of TESAnnwyn which does not include the TES4 import/exporter and some other legacy stuff (like the 'LOD2' NIF generator). There are no 3rd party libraries involved, this is all my own work and should compile easily on any system (it was developed on Linux in 2006, then I would compile it on Windows when releasing for others to use):

It's cut down primarily for people who want to use it for Morrowind specific work.

The source code is far from my most elegant program, which is why I've always refrained from releasing the code because the layout was such a mess. It evolved as I decoded the various TES formats (documentation on the land format was non existent back then) so there have been tweaks and hacks for various features. Many common variables are global rather than passed between functions, which makes things much faster to write and the code runs faster too, but it means there's a lot of variables in "defs.h"! Some could probably be sanitized.

This is the original TESAnnwyn program with the TES4 modules not included so the size is smaller (no need for ZLIB for example). There are various hooks to the TES4 bits which I've commented out. On the plus side, I often left comments in various places as I coded so you're not completely left blind.

TESAnnwyn has a tiny memory footprint, it can import and export monumental size heightmaps that could not be loaded in to memory and it does this by creating lots of little files instead; each file is one cell in size. On Linux and Windows XP it hurtles along very quickly, but on Windows 7 (not sure about Vista an Windows 8) it seems much slower at creating the little files, maybe because it's a console program, or a 32-bit program, maybe virus scanner hooks, I don't know, but Windows 7 (64 bit at least) runs TESAnnwyn much slower than XP or Linux did.

===========
Source Code
===========

These are the files:

1. defs.h        : Global variable and structure definitions (includes all the TES4 function names).
2. funcs.h       : Function protoype definitions (includes all the TES4 function names).
3. tesannwyn.c   : Includes all the header and C source files.
4. main.c        : The main() routine which processes all the command line arguments then either calls 
                   the ImportImage or ExportImages functions. You can replace this routine with one of 
                   your own with fixed arguments if you wish or add an API library interface to your 
                   own software that sets up the right variables.
5. common.c      : Various common routines.
6. tes3_import.c : The TES3 heightmap Importer (reads heightmap, VCLR and VTEX RAW/BMPs to create a ESP file).
7. tes4_import.c : The TES3 heightmap Exporter (reads ESM/ESP files and creates the heightmap, VCLR and VTEX BMP/RAW images).
8. ta-vtex3.c    : TES3 VTEX processing routines. Still includes a lot of TES4 functions, to convert TES4 textures to TES3 etc.

9. Makefile      : Simple makefile to compile the code (e.g. just type 'make' in a Unix environment).
10. tes3ltex.txt : The Standard TES3 -> TES4 texture lookup file that's distributed with TESAnnwyn.


======================
Creating your own hook
======================
In tesannwyn.c you can replace the following line:

#include "main.c"

with your own filename which contains an API function to call with the arguments you require. 

Or for example you could create a stand-alone executable which does a specific task such as export the heightmaps for Morrowind, Firemoth and Bloodmoon into a single heightmap, vertex colour map and texture placement map if main() was just:

int main()
{
        opt_mode	= EXPORT;
        opt_tes_mode	= TES_MORROWIND;
	opt_image_type	= 2; // BMP. Set to 1 if you want to export RAW like the Oblivion CS uses.
        opt_vtex	= 3;
        opt_vclr	= 1;

        strcpy(input_files.filename[0], "Morrowind.esm");
        strcpy(input_files.filename[1], "Firemoth.esp");
        strcpy(input_files.filename[2], "Bloodmoon.esm");
        input_files.count = 3;

        ExportImages();
}

Or to import a BMP heightmap called "tesannwyn.bmp", VCLR (it always expects "tesannwyn-vclr.bmp" and textures "tesannwyn-vtex.bmp" with a cell offset of (-28,-16):

int main()
{
	opt_mode	= IMPORT;
	opt_tes_mode	= TES_MORROWIND;
	opt_image_type	= 2; // BMP
	opt_vtex	= 3;
	opt_vclr	= 1;
	opt_x_cell_offset = -28;
	opt_y_cell_offset = -16;
	ImportImage("tesannwyn.bmp");
}


More options can be found in main.c, for example to export a RAW instead of a BMP, use:

	opt_image_type = 1;


If you don't have an aversion to the TESAnnwyn console window popping up inside your own program when you need to do heightmap imports/exports then you can use the standard TESAnnwyn program compiled for your environment, and potentially benefit from future options added to the original program.

--
Author:  Paul Halliday (aka Lightwave)
Email:   ocean_lightwave@yahoo.co.uk
Website: http://www.oceanlightwave.com

Last official development: January 2011.
This version:              13-Sep-2014 (TES3 source code version).

