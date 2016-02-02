#ifndef TES3_IMPORT_H
#define TES3_IMPORT_H

/* TODO: fix globals */
static int total_land = 0;
static int total_overflows = 0,
    total_underflows = 0;

int ImportImage(char *, int, int, int,
                int, int , int, int,
                int, int , int,
                int, int,
                int, int,
                int, char *, float);

int ReadVTEX3(char *, int, int, int, int, int, int, FILE *);
int StandardizeBMP2RAW(char *, char *, int *, int *, int *, int , int, float);
int StandardizeRAW(char *, char *, int *sx, int *, int, int, int, float);
int WriteTES3CELLRecord(int, int, FILE *);
int WriteTES3Header(FILE *);
int WriteTES3LTEX(FILE *, int *);
int DeStandardizeTES3VTEX(unsigned short int [16][16], unsigned short int [16][16]);
int WriteTES3LANDRecord(int cx, int cy, int opt_adjust_height, int image[66][66], char vclr[66][66][3], short unsigned int vtex3[16][16], FILE *fp_out, int opt_vtex, int opt_vclr, int *total_overflows, int *total_underflows, char *opt_texture);

#endif /* TES3_IMPORT_H */
