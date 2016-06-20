//
// Created by bcurtis on 08/03/16.
//

#ifndef TESANNWYN_LIBTESANNWYN_H
#define TESANNWYN_LIBTESANNWYN_H

// Record the minimum and maximum heights
typedef struct {
    int min;
    int max;
    int cell_max_x;
    int cell_max_y;
    int cell_min_x;
    int cell_min_y;
} height_stats;

typedef struct {
    unsigned short int is_import;
    height_stats statistics;

} tes_context;

tes_context tes_export(char *filename, int bpp);


// declarations for tes3_export
void Write_LTEX_data(char *);
int Get_Form_ID_For_Filename(char *, char *, char *, char *);
void String_To_Reverse_FormID(char *, char *);
void Process_TES3_LTEX_Data(char *);
void Form_ID_To_String(char *, char *);
unsigned int Bytes_To_Int(char, char, char, char);
void Replace_VTEX3_Textures(char *);
void Standardize_TES3_VTEX(unsigned short int [16][16], unsigned short int [16][16]);
void Process_TES3_LAND_Data(char *, int);
void Export_TES3_Land(char *);
void Create_RAW(char *, int);

#endif //TESANNWYN_LIBTESANNWYN_H
