/*
Bit plane encoder
Please note:
(1)   Before you download and use the program, you must read and agree the license agreement carefully.
(2)   We supply the source code and program WITHOUT ANY WARRANTIES. The users will be responsible
        for any loses or damages caused by the use of the source code and the program.

Author:
Hongqiang Wang
Department of Electrical Engineering
University of Nebraska-Lincoln
Email: hqwang@bigred.unl.edu, hqwang@eecomm.unl.edu

Your comment and suggestions are welcome. Please report bugs to me via email and I would greatly appreciate it.
Nov. 3, 2006
*/

#include <stdlib.h>
#include <stdio.h>

void CoeffDegroup(int **img_wav,
          int rows,
          int cols)
{
   long ** temp = NULL;
   int i = 0;
   int j = 0;
   int x = 0;
   int y = 0;
   int p = 0;
   int k = 0;

   /* Read the input image */
   temp = (long **)calloc((long)rows,sizeof(long *));
   for(i = 0; i < rows; i++)
         temp[i] = (long *)calloc(cols,sizeof(long));
// HH1 band.
   for (i = (rows >> 1); i < rows; i += 4)
      for(j = (cols >> 1); j < cols; j += 4)
      {
         x = ((i - (rows >> 1)) << 1);
         y = ((j - (cols >> 1)) << 1);
         for ( p = 4; p < 8; p ++)
            for ( k = 4; k < 8; k ++)
               temp[i + p - 4][j + k - 4] = img_wav[x + p][y + k];
      }
// HL1
   for (i = 0; i < (rows>>1); i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = (i << 1) ;
         y = ((j - (cols >> 1)) << 1);
         for ( p = 0; p < 4; p ++)
            for ( k = 4; k < 8; k ++)
               temp[i + p][j + k - 4] = img_wav[x + p][y + k];
      }
// LH1
   for (i = (rows >> 1); i < rows; i += 4)
      for(j = 0; j < (cols >> 1); j+=4)
      {
         x = ((i - (rows >> 1)) << 1);
         y = (j << 1);
         for ( p = 4; p < 8; p ++)
            for ( k = 0; k < 4; k ++)
               temp[i + p - 4][j + k] = img_wav[x + p][y + k];
      }

// HH2 band.
   for (i = (rows>>2); i < (rows>>1); i+=2)
      for(j = (cols>>2); j < (cols>>1); j+=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = ((j - (cols>>2)) << 2);
         temp[i][j] =  img_wav[x + 2][y + 2];
         temp[i][j + 1] =  img_wav[x + 2][y + 3];
         temp[i + 1][j] =  img_wav[x + 3][y + 2];
         temp[i + 1][j + 1] =  img_wav[x + 3][y + 3];
      }
// HL2
   for (i = 0; i < (rows>>2); i += 2)
      for(j = (cols>>2); j < (cols>>1); j += 2)
      {
         x = (i << 2);
         y = ((j - (cols >> 2)) << 2);
         temp[i][j] =  img_wav[x][y + 2];
         temp[i][j+ 1] =  img_wav[x][y + 3];
         temp[i + 1][j] =  img_wav[x + 1][y + 2];
         temp[i + 1][j + 1] =  img_wav[x + 1][y + 3];
      }
// LH2
   for (i = (rows>>2); i < (rows>>1); i+= 2)
      for(j = 0; j < (cols>>2); j +=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = (j << 2) ;
         temp[i][j] = img_wav[x + 2][y];
         temp[i][j + 1] = img_wav[x + 2][y + 1];
         temp[i + 1][j] = img_wav[x + 3][y] ;
         temp[i + 1][j + 1] = img_wav[x + 3][y + 1];
      }
// HH3 band.
   x = (rows >> 3);
   y = (cols >> 2);
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i][j] = img_wav[((i - x) <<3) + 1][((j - (cols>>3)) <<3) + 1];
// HL3
   for (i = 0; i < (rows>>3); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i][j] = img_wav[i << 3][((j - (cols>>3)) <<3) + 1];

// LH3
   for (i = (rows>>3); i <( rows>>2); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i][j] = img_wav[((i - x) <<3) + 1][j<<3];

// LL3 band
   for(i = 0 ; i <(rows>>3); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i][j] =  img_wav[i<<3][j<<3];

   for (i = 0; i < rows; i++)
      for(j = 0; j < cols; j++)
         img_wav[i][j] = temp[i][j];
   // nkeffix : release memory allocations
    for (i = 0; i < rows; i++)
       free(temp[i]);
    free(temp);

   return;
}



void CoeffDegroupFloating(float **img_wav,
          int rows,
          int cols)
{
   float ** temp = NULL;
   int i = 0;
   int j = 0;
   int x = 0;
   int y = 0;
   int p = 0;
   int k = 0;

   /* Read the input image */
   temp = (float **)calloc(rows,sizeof(float *));
   for(i = 0; i < rows; i++)
         temp[i] = (float *)calloc(cols,sizeof(float));
// HH1 band.
   for (i = (rows>>1); i < rows; i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y =( (j - (cols>>1)) << 1);
         for ( p = 4; p < 8; p ++)
            for ( k = 4; k < 8; k ++)
               temp[i + p - 4][j + k - 4] = img_wav[x + p][y + k];
      }
// HL1
   for (i = 0; i < (rows>>1); i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = (i << 1) ;
         y = ((j - (cols>>1)) << 1);
         for ( p = 0; p < 4; p ++)
            for ( k = 4; k < 8; k ++)
               temp[i + p][j + k - 4] = img_wav[x + p][y + k];
      }
// LH1
   for (i = (rows>>1); i < rows; i+=4)
      for(j = 0; j < (cols>>1); j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y = (j << 1) ;
         for ( p = 4; p < 8; p ++)
            for ( k = 0; k < 4; k ++)
               temp[i + p - 4][j + k] = img_wav[x + p][y + k];
      }

// HH2 band.
   for (i = (rows>>2); i < (rows>>1); i+=2)
      for(j = (cols>>2); j < (cols>>1); j+=2)
      {
         x = ((i - (rows >>2 )) << 2);
         y = ((j - (cols >> 2)) << 2);
         temp[i][j] =  img_wav[x + 2][y + 2];
         temp[i][j + 1] =  img_wav[x + 2][y + 3];
         temp[i + 1][j] =  img_wav[x + 3][y + 2];
         temp[i + 1][j + 1] =  img_wav[x + 3][y + 3];
      }
// HL2
   for (i = 0; i < (rows>>2); i += 2)
      for(j = (cols>>2); j < (cols>>1); j += 2)
      {
         x = (i << 2);
         y = ((j - (cols>>2)) << 2);
         temp[i][j] =  img_wav[x][y + 2];
         temp[i][j+ 1] =  img_wav[x][y + 3];
         temp[i + 1][j] =  img_wav[x + 1][y + 2];
         temp[i + 1][j + 1] =  img_wav[x + 1][y + 3];
      }
// LH2
   for (i = (rows>>2); i < (rows>>1); i+= 2)
      for(j = 0; j < (cols>>2); j +=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = (j << 2);
         temp[i][j] = img_wav[x + 2][y];
         temp[i][j + 1] = img_wav[x + 2][y + 1];
         temp[i + 1][j] = img_wav[x + 3][y] ;
         temp[i + 1][j + 1] = img_wav[x + 3][y + 1];
      }
// HH3 band.
   x = (rows>>3);
   y = (cols>>2);
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i][j] = img_wav[((i - x) <<3) + 1][((j - (cols>>3)) <<3) + 1];
// HL3
   for (i = 0; i < (rows>>3); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i][j] = img_wav[i << 3][((j - (cols>>3)) <<3) + 1];

// LH3
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i][j] = img_wav[((i - x) <<3) + 1][j<<3];

// LL3 band
   for(i = 0 ; i <(rows>>3); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i][j] =  img_wav[i<<3][j<<3];

   for (i = 0; i < rows; i++)
      for(j = 0; j < cols; j++)
         img_wav[i][j] = temp[i][j];
   // nkeffix : release memory allocations
    for (i = 0; i < rows; i++)
       free(temp[i]);
    free(temp);

   return;
}




void CoeffRegroupF97(float **TransformedImage,
              int rows,
              int cols)
{
   float ** temp;
   int i;
   int j;
   int x;
   int y;
   int p;
   int k;

   temp = (float **)calloc(rows,sizeof(float *));
   for(i = 0; i < rows; i++)
         temp[i] = (float *)calloc(cols, sizeof(float));   /* Read the input image */
// HH1 band.
   for (i = (rows>>1); i < rows; i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y = ((j - (cols>>1)) << 1);
         for ( p = 4; p < 8; p ++)
            for ( k = 4; k < 8; k ++)
               temp[x + p][y + k] = TransformedImage[i + p - 4][j + k - 4];
      }
// HL1
   for (i = 0; i < (rows>>1); i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = (i<<1) ;
         y = ((j - (cols>>1)) << 1);
         for ( p = 0; p < 4; p ++)
            for ( k = 4; k < 8; k ++)
               temp[x + p][y + k] = TransformedImage[i + p][j + k - 4];
      }
// LH1
   for (i = (rows>>1); i < rows; i+=4)
      for(j = 0; j < (cols>>1); j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y = (j << 1) ;
         for ( p = 4; p < 8; p ++)
            for ( k = 0; k < 4; k ++)
               temp[x + p][y + k] = TransformedImage[i + p - 4][j + k];
      }

// HH2 band.
   for (i = (rows>>2); i < (rows>>1); i+=2)
      for(j = (cols>>2); j < (cols>>1); j+=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = ((j - (cols>>2)) << 2);
         temp[x + 2][y + 2] =  TransformedImage[i][j];
         temp[x + 2][y + 3] =  TransformedImage[i][j + 1];
         temp[x + 3][y + 2] =  TransformedImage[i + 1][j];
         temp[x + 3][y + 3] =  TransformedImage[i + 1][j + 1];
      }
// HL2
   for (i = 0; i < (rows>>2); i += 2)
      for(j = (cols>>2); j < (cols>>1); j += 2)
      {
         x = (i << 2);
         y = ((j - (cols>>2)) << 2);
         temp[x][y + 2] =  TransformedImage[i][j];
         temp[x][y + 3] =  TransformedImage[i][j+ 1];
         temp[x + 1][y + 2] =  TransformedImage[i + 1][j];
         temp[x + 1][y + 3] =  TransformedImage[i + 1][j + 1];
      }
// LH2
   for (i = (rows>>2); i < (rows>>1); i+= 2)
      for(j = 0; j < (cols>>2); j +=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = (j << 2);
         temp[x + 2][y] = TransformedImage[i][j];
         temp[x + 2][y + 1] = TransformedImage[i][j + 1];
         temp[x + 3][y] = TransformedImage[i + 1][j];
         temp[x + 3][y + 1] = TransformedImage[i + 1][j + 1];
      }

// HH3 band.
   x = (rows>>3);
   y = (cols>>2);
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[((i - x) <<3) + 1][((j - (cols>>3)) <<3) + 1] = TransformedImage[i][j];
// HL3
   for (i = 0; i <(rows>>3); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i << 3][((j - (cols>>3)) <<3) + 1] = TransformedImage[i][j];

// LH3
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[((i - x) <<3) + 1][j<<3] = TransformedImage[i][j];

// LL3 band
   for(i = 0 ; i < (rows>>3); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i<<3][j<<3] =  TransformedImage[i][j];

   for (i = 0; i < rows; i++)
      for(j = 0; j < cols; j++)
         TransformedImage[i][j] = temp[i][j];
   return;
}




void CoeffRegroup(int **TransformedImage,
              int rows,
              int cols)
{
   long ** temp;
   int i;
   int j;
   int x;
   int y;
   int p;
   int k;

   temp = (long **)calloc(rows,sizeof(long *));
   for(i = 0; i < rows; i++)
         temp[i] = (long *)calloc(cols, sizeof(long));   /* Read the input image */
// HH1 band.
   for (i = (rows>>1); i < rows; i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y = ((j - (cols>>1)) << 1);
         for ( p = 4; p < 8; p ++)
            for ( k = 4; k < 8; k ++)
               temp[x + p][y + k] = TransformedImage[i + p - 4][j + k - 4];
      }
// HL1
   for (i = 0; i < (rows>>1); i+=4)
      for(j = (cols>>1); j < cols; j+=4)
      {
         x = (i<<1) ;
         y = ((j - (cols>>1)) << 1);
         for ( p = 0; p < 4; p ++)
            for ( k = 4; k < 8; k ++)
               temp[x + p][y + k] = TransformedImage[i + p][j + k - 4];
      }
// LH1
   for (i = (rows>>1); i < rows; i+=4)
      for(j = 0; j < (cols>>1); j+=4)
      {
         x = ((i - (rows>>1)) << 1);
         y = (j<<1) ;
         for ( p = 4; p < 8; p ++)
            for ( k = 0; k < 4; k ++)
               temp[x + p][y + k] = TransformedImage[i + p - 4][j + k];
      }

// HH2 band.
   for (i = (rows>>2); i < (rows>>1); i+=2)
      for(j = (cols>>2); j < (cols>>1); j+=2)
      {
         x = ((i - (rows>>2)) <<2);
         y = ((j - (cols>>2)) <<2);
         temp[x + 2][y + 2] =  TransformedImage[i][j];
         temp[x + 2][y + 3] =  TransformedImage[i][j + 1];
         temp[x + 3][y + 2] =  TransformedImage[i + 1][j];
         temp[x + 3][y + 3] =  TransformedImage[i + 1][j + 1];
      }
// HL2
   for (i = 0; i < (rows>>2); i += 2)
      for(j = (cols>>2); j < (cols>>1); j += 2)
      {
         x = (i << 2);
         y = ((j - (cols>>2)) << 2);
         temp[x][y + 2] =  TransformedImage[i][j];
         temp[x][y + 3] =  TransformedImage[i][j+ 1];
         temp[x + 1][y + 2] =  TransformedImage[i + 1][j];
         temp[x + 1][y + 3] =  TransformedImage[i + 1][j + 1];
      }
// LH2
   for (i = (rows>>2); i < (rows>>1); i+= 2)
      for(j = 0; j < (cols>>2); j +=2)
      {
         x = ((i - (rows>>2)) << 2);
         y = (j << 2);
         temp[x + 2][y] = TransformedImage[i][j];
         temp[x + 2][y + 1] = TransformedImage[i][j + 1];
         temp[x + 3][y] = TransformedImage[i + 1][j];
         temp[x + 3][y + 1] = TransformedImage[i + 1][j + 1];
      }

// HH3 band.
   x = (rows>>3);
   y = (cols>>2);
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[((i - x) <<3) + 1][((j - (cols>>3)) <<3) + 1] = TransformedImage[i][j];
// HL3
   for (i = 0; i < (rows>>3); i++)
      for(j = (cols>>3); j < (cols>>2); j++)
         temp[i << 3][((j - (cols>>3)) <<3) + 1] = TransformedImage[i][j];

// LH3
   for (i = (rows>>3); i < (rows>>2); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[((i - x) <<3) + 1][j<<3] = TransformedImage[i][j];

// LL3 band
   for(i = 0 ; i <(rows>>3); i++)
      for(j = 0; j < (cols>>3); j++)
         temp[i<<3][j<<3] =  TransformedImage[i][j];

   for (i = 0; i < rows; i++)
      for(j = 0; j < cols; j++)
         TransformedImage[i][j] = temp[i][j];
   return;
}





