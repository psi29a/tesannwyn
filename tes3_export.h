#ifndef TES3_EXPORT_H
#define TES3_EXPORT_H

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

int GetFormIDForFilename(char *, char *, char *, char *);
int StringToReverseFormID(char *, char *);
int Process3LTEXData(char *, int);
int FormIDToString(char *, char *);


#endif /* TES3_EXPORT_H */


