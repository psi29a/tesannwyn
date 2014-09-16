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
                pos += 8 +nsize;
        }

        if (strncmp("INTV", r + pos, 4) == 0) {
                nsize = (int) r[pos+4];
		memcpy(&index, r+pos+8, 4);
                pos += 8 +nsize;
        }

        if (strncmp("DATA", r + pos, 4) == 0) {
                nsize = (int) r[pos+4];
                strncpy(tname, r+pos+8, nsize);
                pos += 8 +nsize;
        } else {
                printf("Couldn't find LTEX DATA subrec!\n");
                exit(1);
        }

	for (i = 0; i < ltex.count; i++) {
		if (strcmp(ltex.texname[i], lname) == 0) {
			vtex3_replace.old[vtex3_replace.replace_count] = vtex3_replace.myindex+1;
			vtex3_replace.new[vtex3_replace.replace_count] = i+1;
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
		vtex3_replace.old[vtex3_replace.replace_count] = vtex3_replace.myindex+1;
		vtex3_replace.new[vtex3_replace.replace_count] = i+1;
		vtex3_replace.count++;
		vtex3_replace.replace_count++;
	}
	vtex3_replace.myindex++;

        return 0;
}


int ReadLTEX3(char *filename) 
{
	int i;

	int p = 0,
	    index = 0;

	char iname[128],
	     formid_ascii[64], // Only 8 should be necessary, but in case the file format is corrupt ...
	     s[512];

	FILE *fp_lt;

	if ((fp_lt = fopen(filename, "rb")) != 0) {
		while (fgets(s, 512, fp_lt) != NULL) {
			if (s[0] == '#') continue;
			p = 0;

			for (i = 0; s[i+p] != ',' && s[i+p] != '\0'; iname[i] = s[i+p], i++);
			iname[i] = '\0';
			index = atoi(iname);

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

//int GetFormIDForFilename(char *tex_filename, char *FormID)


int GetFormIDForFilename(char *tex_filename, char *ltex_name, char *ltex_filename, char *FormID) 
{
	int i;

	int p = 0;

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

