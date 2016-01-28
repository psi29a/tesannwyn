#ifndef TES3_IMPORT_H
#define TES3_IMPORT_H

/* TODO: fix globals */
static int total_land = 0;

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

#endif /* TES3_IMPORT_H */
