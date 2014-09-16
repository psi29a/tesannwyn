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

int ExportImages()
{
	int i;
	char dir_name[128];
	char vtex_layer_filename[128];

	cleanup_list_count = 0;

	ltex.count = 0;

        mkdir(TA_TMP_DIR, 0777);

	// A sanity check.

	if (opt_image_type != RAW && opt_image_type != BMP) {
		opt_image_type = BMP;
		printf("You didn't specify an image type (-p 1 or 2). I'll choose BMP for you ...\n");
	}

	if (opt_vtex && opt_tes_ver >= 4) {
		ltex.count = 1;	// 0 will be used when there's no texture.
		maxlayer = 0;

		if (opt_vtex >= 4) {
     			// Delete any old VTEX4 filenames to avoid confusion when doing a re-import.
	                for (i = 0; i < 9; i++) {
	                        sprintf(vtex_layer_filename, TA_VTEX4_OUT, i);
				printf("Deleting %s\n", vtex_layer_filename);
	                        unlink(vtex_layer_filename);
	                }
		}

                for (i = 0; i < 9; i++) {
			sprintf(dir_name, "%s/%d", TA_TMP_DIR, i);
			mkdir(dir_name, 0777);
                }
	}

	if (opt_cell_data && opt_tes_ver >= 4) {
		printf("Deleting %s\n", TA_CELL_IN);
		printf("Deleting %s\n", TA_CELL_BMP);
		unlink(TA_CELL_IN);
		unlink(TA_CELL_BMP);
		sprintf(dir_name, "%s/%s", TA_TMP_DIR, TA_CELL_TMP);
		unlink(dir_name);
	}

	for (i = 0; i < input_files.count; i++) {
		if (opt_tes_ver == 3) {
			printf("\nRunning TES3 exporter:\n");
			ExportTES3Land(input_files.filename[i], opt_bpp);
		} else { // TES4
			printf("\nRunning TES4 exporter:\n");
			// ExportTES4Land(input_files.filename[i], opt_worldspace, opt_bpp);
		}
	}

	if (opt_vclr) { 
		printf("\n\nGenerating BMP of VCLR data: %s ... \n", TA_VCLR_OUT);
		HumptyVCLR(TA_VCLR_OUT, opt_tes_ver);
	}

	if (opt_vtex) {  // == 3
		if (opt_tes_ver == 3) {
			printf("\n\nGenerating BMP of VTEX placement data: %s ... \n", TA_VTEX3_OUT);
			HumptyVTEX3(TA_VTEX3_OUT, opt_tes_ver, 1);
		} else {
			for (i = 0; i < maxlayer; i++) {
				sprintf(vtex_layer_filename, "tesannwyn-vtex4-%d.bmp", i);
				HumptyVTEX3(vtex_layer_filename, opt_tes_ver, i);
			}
		}
	}

	if (opt_lod) {
		// HumptyLOD2();
	}

	if (opt_image_type == RAW) {
		printf("\n\nGenerating new RAW output file: %s ...\n", TA_RAW_OUT);
		HumptyImage(TA_RAW_OUT, opt_image_type, opt_tes_ver, opt_bpp);
	} else if (opt_image_type == BMP) {
		printf("\n\nGenerating new BMP output file: %s ...\n", TA_BMP_OUT);
		if (opt_rescale) {
			RescaleGreyScale(TA_BMP_OUT, opt_image_type, opt_tes_ver, opt_bpp);
		}
		HumptyImage(TA_BMP_OUT, opt_image_type, opt_tes_ver, opt_bpp);
	}

	CleanUp();

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

int ExportTES3Land(char *input_esp_filename, int bpp)
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
			Process3LANDData(or + 16, size-16);
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

int Process3LANDData(char *r, int size)
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
		CleanUp();
		exit(1);
	}

	for (i = 0 ; i < ltex.count; i++) {
		FormIDToString(FormID, ltex.formid[i]);
		fprintf(fp_lt, "%d,%s,%s,%s\n", ltex.texnum[i], ltex.texname[i], ltex.filename[i], FormID);
	}

	fclose(fp_lt);

	return 0;
}

