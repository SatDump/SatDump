#pragma once

#include <cstring>

/*
Aang23 :

This is a (somewhat not-so-clean) port of SOHO's
ricerecon64 to work off memory, not fail with
memory leaks on errors, etc.
*/

namespace
{
////////////////////////////////////////////
/* this is the 64 x 64 pixel block version */
#ifndef size_t
#define size_t unsigned int /*Target compiler fails to define.    */
#endif
#define uchar unsigned char
#define uword unsigned short
#define ulong unsigned long
#define MaxBlk 34 * 34 /*Maximum # of sub-image blocks in the  \
                         Image.  Note that this includes       \
                         blocks that will not be transmitted   \
                         (see UseBlock).  This was originally  \
                         set to 2020, but was reduced to 34*34 \
                         to reduce memory load.                \
                         This imposes an additional limitation \
                         on the size of the image, due to the  \
                         requirement that the block number be  \
                         less than MaxBlk.:                    \
                      (BCol2-BCol1+1)*(BRow2-BRow1+1)<=MaxBlk*/
#define BlkSz 64       /*Size (one side) of sub-image block-- \
                          power of 2.                         \
                         (Note: the DCT algorithm must match  \
                          this size.)                        */

#define nBlkBit 6                 /*log base 2 of BlkSz                 */
#define nPixBlk (BlkSz * BlkSz)   /*Pixels in one block                 */
#define MaxBlkH 64                /*Maximum # of horizontal blocks        \
                                    The maximum # of columns of pixels in \
                                     the image will be BlkSz*MaxBlkH.   */
#define MaxBlkV 64                /*Maximum # of vertical blocks       \
                                    The maximum # of rows of pixels in \
                                     the image will be BlkSz*MaxBlkV.   */
#define maxncol (MaxBlkH * BlkSz) /*maximum # of columns                */
#define maxnrow (MaxBlkV * BlkSz) /*maximum # of rows                   */
#define MaxWord 16383             /*Maximum # of words in Pack Buffer.  */
#define MaxByte (MaxWord * 2)     /* Max # of bytes / packet            */
#define nBksBit 11                /*# of bits needed to code            \
                                      MaxBlk-1+(# of other block types) \
                                    Actually we have fixed this at 11   \
                                     and defined the other block types  \
                                     accordingly                        */

#define MaxWBit 14 /*log base 2 of MaxWord-1             */
    ////////////////////////////////////////////

#define RICE_MEMORY_VERSION 1

#define ENABLE_PRINTF_OUT 0 // Aang23 : Allow making it quiet!
}

namespace soho_compression
{
    class SOHORiceDecompressor
    {
    private:
#if !RICE_MEMORY_VERSION
        FILE *fp1; /*Pointer to input file handle        */
        FILE *fp2; /*Pointer to output file handle       */
#endif
        uword *Image; /*image array                 */

    private:
        /*============================================================================*/
        /* Static versions of some of the output variables:                           */
        int ncol, nrow; /*Size of transmitted image           */
                        /*============================================================================*/
                        /*The remainder of the code is as follows:                                    */

        int Method, iblk; /*Compression method,block #          */
        long *Block;      /*Holds sub-image block               */
        long MaxPTot;     /*Total # of bytes in compressed image
                                  packets                             */
        long MaxPTo2;     /*Same, ignoring packet overhead      */
        int Ended;        /*Flag: End Packet has been called    */
        uword NextByte;   /*Next byte of Pack Buffer            */
        int CheckSum;
        int FoundHeader; /*Flags on whether or not have found
                                           image header packet and initial ADCT
                                           packet                              */

        /*Powers of two    */
        const long TwoP[30] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048,
                               4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152,
                               4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912};

        uchar *Pack;                    /*Packed image data                   */
        int AtEnd;                      /*At end of file?                     */
        int Valid;                      /*Was this a valid packet?            */
        int OldValid;                   /*Was previous packet valid?          */
        int BCol1, BCol2, BRow1, BRow2; /*Block column and row ranges
                                                used at compression.                */
        uword MinPixAll;                /*Minimum Pixel value for Image
                                         */
        uword MaxPixAll;                /*Maximum Pixel value for Image       */
        int nBitPix;                    /*#of bits to send
                                                MaxPixAll-MinPixAll                 */
        int nByteP;                     /*Index of next byte index in
                                                Pack buffer                         */
        int nBitP;                      /*# of bits available in
                                                NextByte                            */
        int nBytePacket;                /*# of bytes in this packet           */
        int SignFlag;                   /*Was input image signed?             */
        int nshift;                     /*# of bits input right shifted
                                         */
        int SqrtFlag;                   /*Were input pixels square
                                                rooted?                             */
        int iBlkH, iBlkV;               /*Block col and row #		      */

    public:
        SOHORiceDecompressor()
        {
            Image = new uword[maxncol * (long)maxnrow];
            Pack = new uchar[MaxByte];
            Block = new long[nPixBlk];
        }

        ~SOHORiceDecompressor()
        {
            delete[] Image;
            delete[] Pack;
            delete[] Block;
        }

        bool output_was_valid = true;

    private:
        /*============================================================================*/
        /*The following routines are defined in this file:                            */
        void Recon(int *ncol, int *nrow, int *SignFlag, uword Image[]);
        int RdBit(int ibit);
        void Error(char *Mes, int iexit, int Var1, int Var2);
        int nBitNeed(ulong i);
        void EndPacket(void);
        void StartPacket(void);
        void ReconPacket(void);
        void RiceRecon(void);

        void NoRecon(void);
        void ErrorPacket(void);
        void Warning(char *Mes, long Var1, long Var2);
        long RdBitL(int ibit);

    private:
        /*============================================================================*/
        /*The following routines are defined in rice64.c:                          */
        void ReadPack(uchar Pack[], int *Valid, int *AtEnd, int *nByteP,
                      int *nBitP, int *nBytePacket);
        void ImageHeader(int *BCol1, int *BCol2, int *BRow1, int *BRow2,
                         uword *MinPixAll, uword *MaxPixAll,
                         int *Method, int *SignFlag, int *nshift, int *SqrtFlag,
                         int *ncol, int *nrow, int *Valid);
        void OtherBlock(int iblk);
        int UseBlock(int iBlkH, int iBlkV);

// Custom buffer fstream/etc
#if RICE_MEMORY_VERSION
    private:
        uint8_t *wip_buffer_ptr;
        int wip_buffer_sz;
        int wip_buffer_curr_bytepos;

        int bfseek(int pos, int mode)
        {
            if (pos >= wip_buffer_sz)
                return 1;

            wip_buffer_curr_bytepos = pos;
            return 0;
        }

        int bftell()
        {
            return wip_buffer_curr_bytepos;
        }

        bool bfeof()
        {
            return wip_buffer_curr_bytepos >= wip_buffer_sz;
        }

        int bfread(void *ptr, size_t size, size_t nmemb)
        {
            int64_t read_tot = size * nmemb;

            while (wip_buffer_curr_bytepos + read_tot > wip_buffer_sz)
                read_tot -= size;

            if (read_tot < 0)
                read_tot = 0;

            // printf("--------------------------------------------------------SIZE-- %d ---- %d\n", read_tot, size);

            memcpy(ptr, &wip_buffer_ptr[wip_buffer_curr_bytepos], read_tot);
            wip_buffer_curr_bytepos += read_tot;

            return read_tot / size;
        }
#endif

    public:
#if RICE_MEMORY_VERSION
        int run_main_buff(uint8_t *input_buffer, int input_size, uint16_t **output_image, int *img_rows, int *img_cols)
        {
            /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
            int i, ncol, nrow, SignFlag;
#if ENABLE_PRINTF_OUT
            printf("Rice Decompression 07 Feb 2003 version\n");
#endif

            // if ((fp1 = fopen(input_file, "rb")) == NULL)
            //     Error("Could not open compressed file", 1, 0, 0);
            wip_buffer_ptr = input_buffer;
            wip_buffer_sz = input_size;
            wip_buffer_curr_bytepos = 0;

            // if ((fp2 = fopen(output_file, "wb")) == NULL)
            //     Error("Bad Output File1", 1, 0, 0);
            *output_image = nullptr; // new uint16_t[maxncol * maxnrow];
            *img_rows = 0;
            *img_cols = 0;

            // if (argc == 4)
            //     RdHdIGSE(fp1); /*Read past IGSE header       */

            Recon(&ncol, &nrow, &SignFlag, Image);

            // for (i = 0; i < nrow; i++)
            //{
            //     if (fwrite(&Image[i * (long)ncol], (size_t)2, (size_t)ncol, fp2) != (size_t)ncol)
            //         Error("Bad Output Write2", 1, 0, 0);
            // }
            //  if (fclose(fp2))
            //      Error("Bad Close1", 1, 0, 0); /*Program Showim needs to
            //                                      be able to read the number
            //                                      of cols, rows, and the
            //                                      signed/unsigned state
            //                                      from file test.siz.         */
            //  if ((fp2 = fopen("test.siz", "w")) == NULL)
            //      Error("Bad Output File2 - Can't open test.siz", 1, 0, 0);
            //  if (fprintf(fp2, "%d %d %d\n", ncol, nrow, SignFlag) <= 1)
            //      Error("Bad Output Write1", 1, 0, 0);
            //  if (fclose(fp2))
            //      Error("Bad Close2", 1, 0, 0);

            if (nrow > maxnrow || ncol > maxncol)
                return 1;

            *img_rows = nrow;
            *img_cols = ncol;

            *output_image = Image;

#if ENABLE_PRINTF_OUT
            printf("\nDone!\n");
#endif
            return 0;
        }
#else
        int run_main(char *input_file, uint16_t **output_image, int *img_rows, int *img_cols)
        {
            /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
            int i, ncol, nrow, SignFlag;
#if ENABLE_PRINTF_OUT
            printf("Rice Decompression 07 Feb 2003 version\n");
#endif

            if ((fp1 = fopen(input_file, "rb")) == NULL)
                Error("Could not open compressed file", 1, 0, 0);

            // if ((fp2 = fopen(output_file, "wb")) == NULL)
            //     Error("Bad Output File1", 1, 0, 0);
            *output_image = nullptr; // new uint16_t[maxncol * maxnrow];
            *img_rows = 0;
            *img_cols = 0;

            // if (argc == 4)
            //     RdHdIGSE(fp1); /*Read past IGSE header       */

            Recon(&ncol, &nrow, &SignFlag, Image);

            // for (i = 0; i < nrow; i++)
            // {
            //     if (fwrite(&Image[i * (long)ncol], (size_t)2, (size_t)ncol, fp2) != (size_t)ncol)
            //         Error("Bad Output Write2", 1, 0, 0);
            // }
            // if (fclose(fp2))
            //     Error("Bad Close1", 1, 0, 0); /*Program Showim needs to
            //                                     be able to read the number
            //                                     of cols, rows, and the
            //                                     signed/unsigned state
            //                                     from file test.siz.         */
            // if ((fp2 = fopen("test.siz", "w")) == NULL)
            //     Error("Bad Output File2 - Can't open test.siz", 1, 0, 0);
            // if (fprintf(fp2, "%d %d %d\n", ncol, nrow, SignFlag) <= 1)
            //     Error("Bad Output Write1", 1, 0, 0);
            // if (fclose(fp2))
            //     Error("Bad Close2", 1, 0, 0);*/

            if (nrow > maxnrow || ncol > maxncol)
                return 1;

            *img_rows = nrow;
            *img_cols = ncol;

            *output_image = Image;

            printf("\nDone!\n");
            return 0;
        }
#endif
    };
}