#ifndef TES3_EXPORT_H
#define TES3_EXPORT_H

/* TODO: try to remove globals */

static int min_x = 32768,
    max_x = -32768,
    min_y = 32768,
    max_y = -32768;

static int height_stat_min = 1048576,
    height_stat_max = -1048576, // Record the minimum and maximum heights
    height_stat_max_cell_x,
    height_stat_max_cell_y,
    height_stat_min_cell_x,
    height_stat_min_cell_y;

int HumptyImage(char *, int, int, int, int, int, float);
int HumptyVCLR(char *, int);
int HumptyVTEX3(char *, int);
int RescaleGreyScale(char *, int, int, int, float);
int WriteBMPHeader(FILE *, int, int, int);
int WriteBMPGreyScaleHeader(FILE *, int, int, int);
int bytes_to_int(char, char, char, char);
int CleanUp(int [], int [], int *);

int ExportImages(int, int, int, int, int, int, int, float);

int ExportTES3Land(char *, int, int, int, int [], int [], int *);
int Process3LANDData(char *, int, int, int, int [], int [], int *);
int ReplaceVTEX3Textures(char *);
int StandardizeTES3VTEX(unsigned short int [16][16], unsigned short int [16][16]);
int WriteLTEXdata(char *, int [], int [], int *);

int GetFormIDFromTEXNum(unsigned short int, char *);
int GetFormIDForFilename(char *, char *, char *, char *);
int ReadLTEX3(char *);
int StringToReverseFormID(char *, char *);
int Process3LTEXData(char *, int);
int FormIDToString(char *, char *);


#endif /* TES3_EXPORT_H */


