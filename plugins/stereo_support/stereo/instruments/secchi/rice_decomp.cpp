/********************************************************************
 *
 *    MODULE NAME :  RECON.C
 *
 *          TITLE :  Reconstruction (De-Compression) Software for SOHO LASCO/EIT
 *
 *           CC # :  ??
 *
 *         AUTHOR :  Mitchell R Grunes (ATSC/NRL)
 *                   Karl Hoppel       (NRL)
 *
 *           DATE :  07/11/96
 *
 *        PURPOSE :  This is the software to perform image reconstruction
 *                   for the LASCO/EIT experiments on board the ESA/NASA
 *                   SOHO spacecraft.  It will be run on the ground.
 *     FUNCTIONAL
 *    DESCRIPTION :  A sample call is located in sohorecon.c.
 *
 *   CONTROL DATE :  07/11/96
 *
 *       REVISION
 *        HISTORY :  It is requsted by NRL that Lockheed/Avionics make
 *                   no revisions to this file.
 *
 *************************************************************************/

/*============================================================================
The documentation for the interface to this software is contained in file
sohorecon.c and in the final report.

NOTE: Although we have used named parameters for clarity, they cannot be easily
changed, because some other parameters here and elsewhere depend on them,
increasing values could produce arithmetic overflows, and because the DCT size
must match BlkSz.  Furthermore, the compression and reconstruction programs
must also match.*/

/*============================================================================*/
/* #define DEBUG    0 */
#define PRINTBAT 1  /*To print bit allocation table.       \
                      This might be set to 0 for the final \
                      code.                               */
#define UseCheckSum /*Don't process a packet if it contains \
                      a CheckSum.  If errors turn out to    \
                      be very common, we will have to think \
                      about this.                         */

/*The following #define's must be the same as in the calling program
  (sohorecon.c):                                                              */
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include "rice_decomp.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
namespace soho_compression
{
    /*      =======================Form of Call=======================
            void    Recon(int *ncol, int *nrow, int *SignFlag, uword Image[]);
            void    Recon(&ncol,&nrow,SignFlag,Image);
            ======================Input Variables=====================
            none--all input will come from routine ReadPack.
            =====================Output Variables=====================
            ncol,nrow   =# of columns and rows of pixels in transmitted image.
            Image       =Array holding the output image.
                         The pixel in 0-origin column i and 0-origin row j will
                          be stored at Image[i+j*(long)ncol].                     */

    /*============================================================================*/
    void SOHORiceDecompressor::Recon(int *ncol2, int *nrow2, int *SignFlag2, uword Image[])
    {
        /*Reconstruct an image                */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        long *Block0, Pix;
        uword *Image0;
        int i, j, iBlkVold, iblkold;
        int nBlock, nBlockGood;
        long nPixel;
        FoundHeader = 0;
        iBlkVold = -1;
        iblkold = -1;
        Valid = 1;
        nBlock = 0;
        nBlockGood = 0;
    a:
        StartPacket();

        // Aang23 : Prevent div by 0
        if ((BCol2 - BCol1 + 1) == 0)
        {
            output_was_valid = false;
            *ncol2 = 0;
            *nrow2 = 0;
            return;
        }

        iBlkV = iblk / (BCol2 - BCol1 + 1); /*Convert block # to block row and    */
        iBlkH = iblk % (BCol2 - BCol1 + 1); /* block column #                     */
        if (AtEnd == 0)
        {
            nBlock++;
            ReconPacket(); /*Reconstruct (De-compress) block     */
            if (Valid == 0)
            { /*Was not a valid packet:             */
                nBitP = 0;
                goto a;
            }
            if (Valid && iblk < MaxBlk)
            {
#if ENABLE_PRINTF_OUT
                if (iBlkV != iBlkVold)
                    printf("\nBlock Row %d Col ", iBlkV);
                printf("%d ", iBlkH);
#endif
                iBlkVold = iBlkV;
                if (iBlkH >= ncol / BlkSz || iBlkV >= nrow / BlkSz)
                {
                    Error((char *)"Out of range block # (%d)", 0, iblk, 0);
                    goto a;
                }
                if (UseBlock(iBlkH, iBlkV) == 0)
                {
                    Error((char *)"Block was not usable (occulted)", 0, 0, 0);
                    goto a;
                }
                if (iblkold >= iblk)
                    Error((char *)"Out of sequence Block #", 0, 0, 0);
                iblkold = iblk;
                for (j = 0; j < BlkSz; j++)
                { /*Copy Block into Image               */
                    Image0 = &Image[iBlkH * BlkSz + (j + iBlkV * BlkSz) * (long)ncol];
                    Block0 = &Block[j * BlkSz];
                    for (i = 0; i < BlkSz; i++)
                    {
                        Pix = Block0[i];
                        if ((Pix < (long)(ulong)MinPixAll) || (Pix > (long)(ulong)MaxPixAll))
                        {
                            if (Method >= 2)
                            { /*Lossy--just clip                    */
                                if (Pix < (long)(ulong)MinPixAll)
                                    Pix = (long)(ulong)MinPixAll;
                                else
                                    Pix = (long)(ulong)MaxPixAll;
                            }
                            else
                            { /*Lossless--should not happen         */
                                Error((char *)"Out of Bounds Pixel Value (%ld at pixel %d)",
                                      0, (int)Pix, i);
                            }
                        }
                        if (SqrtFlag)
                            Pix = Pix * Pix + Pix;
                        if (nshift)
                            Pix = (Pix << (long)nshift) + TwoP[nshift - 1];
                        if (SignFlag)
                            Pix -= 32768L;
                        Image0[i] = (uword)Pix;
                    }
                }
            }
            if (Valid)
                nBlockGood++;
            goto a;
        }
        nPixel = ncol * (long)nrow;
#if ENABLE_PRINTF_OUT
        printf("\nreconstructed image took %ld bytes.\n", nPixel * 2);
        printf("(Following #'s do not include Science Packet overhead)\n");
        printf("Compressed image took %ld bytes (%ld without packet overhead),\n",
               MaxPTot, (MaxPTo2 + 7) / 8);
        printf(" which is %lf bits/pixel.\n",
               (double)MaxPTot * 8. / (double)(nPixel));
        printf("Compression Factor=%lf from 16 bits/pixel",
               (2. * nPixel) / MaxPTot);
        printf(" (%lf from %d).\n", (nPixel * 2. * nBitPix) / (MaxPTot * 16), nBitPix);
        printf("Without packet overhead bpp=%lf CF=%lf\n",
               (MaxPTo2) / ((double)nPixel), ((double)nPixel) * 16. / (MaxPTo2));
#endif
        if (nBlock < (ncol / BlkSz) * (nrow / BlkSz))
        {
            nPixel = (long)nBlock * (long)nPixBlk;
#if ENABLE_PRINTF_OUT
            printf("If we only consider the %d transmitted blocks, we get:\n", nBlock);
            printf("  %lf bits/pixel.\n",
                   (double)MaxPTot * 8. / nPixel);
            printf(" Compression Factor=%lf from 16 bits/pixel",
                   (2. * nPixel) / MaxPTot);
            printf("  (%lf from %d).\n", (nPixel * 2. * nBitPix) / (MaxPTot * 16), nBitPix);
            printf(" Without packet overhead bpp=%lf CF=%lf\n",
                   (MaxPTo2) / ((double)nPixel), ((double)nPixel) * 16. / (MaxPTo2));
#endif
        }
#if ENABLE_PRINTF_OUT
        printf("--%d out of %d blocks were good--\n", nBlockGood, nBlock);
#endif
        *ncol2 = ncol; /*Copy statics back to caller         */
        *nrow2 = nrow;
        *SignFlag2 = SignFlag;
    }

    /*============================================================================*/
    void SOHORiceDecompressor::StartPacket()
    { /*Start a Compression Packet          */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        // static unsigned long totalnWdP = 0l;
        int nWdP;
        OldValid = Valid;
        ReadPack(Pack, &Valid, &AtEnd, &nByteP, &nBitP, &nBytePacket);
        if (AtEnd)
            return;
        nByteP = nBitP = Ended = CheckSum = 0;
        nBytePacket = 4;
        iblk = RdBit(nBksBit);
#ifdef DEBUG
        printf("StartPacket: iblk = %u\n", iblk);
#endif
        if (FoundHeader == 0 && iblk != 2045 && iblk != 2047)
        {
#if ENABLE_PRINTF_OUT
            Error("Have not yet read image header packet!", 0, 0, 0);
#endif
        }
        else
        {
            nWdP = RdBit(MaxWBit);
            CheckSum -= nWdP; /*Was 0 at time checksum written      */
            nBytePacket = nWdP * 2;
            MaxPTot += nBytePacket;
            MaxPTo2 += nBytePacket * 8 - nBksBit - MaxWBit - 4;
#ifdef DEBUG
            printf("StartPacket(): nWdP = %u total_bytes %lu  Start CheckSum = %d\n",
                   nWdP, MaxPTot, CheckSum);
#endif
        }
    }

    /*============================================================================*/
    void SOHORiceDecompressor::EndPacket()
    { /*End a Compression Packet            */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        int CheckSumAct, idummy;
        if (AtEnd != 0 || Ended != 0)
            return;
        Ended = 1;
        CheckSumAct = 15 & ((CheckSum) + (CheckSum >> 4) +
                            (CheckSum >> 8) + (CheckSum >> 12));
        CheckSum = RdBit(4);

#ifdef DEBUG
        printf("EndPacket(): Calc checksum = %u    Block Checksum = %u\n", CheckSumAct, CheckSum);
#endif

        if ((nByteP + 1) / 2 * 2 != nBytePacket)
            Error((char *)"Wrong # of words in Packet", 0, 0, 0);
        if (CheckSumAct != CheckSum)
        {
#ifdef UseCheckSum
            Error((char *)"Error:Invalid CheckSum: (Nominal, Actual)", 0, CheckSum, CheckSumAct);
#endif
#ifndef UseCheckSum
            Warning("Invalid CheckSum: Nominal %d Actual %d", CheckSum, CheckSumAct);
#endif
        }
        /*Flush to start of next word         */
        while (((nBitP != 0) || nByteP % 2 != 0) && AtEnd == 0)
            idummy = RdBit(1);
    }

    /*============================================================================*/
    void SOHORiceDecompressor::ReconPacket()
    { /*Reconstruct one block of pixels.
        --OUTPUTS--
        Block= BlkSz*BlkSz element block of
        image pixels, stored in BlkSz*BlkSz*2
        contiguous bytes.
        --ALSO MODIFIED OR USED--
        nBitP,Pack,CheckSum,Method          */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        if (AtEnd)
            return;
        if (iblk < MaxBlk)
        { /*Valid packet type                   */
            if (Method == 0)
            { /*Uncompressed data                   */
                NoRecon();
            }
            else if (Method == 1)
            { /*Rice Reconstruction                 */
                RiceRecon();
            }
            else
            {
#if ENABLE_PRINTF_OUT
                Error("Invalid reconstruction method", 0, 0, 0);
#endif
            }
        }
        else if (iblk == 2047)
        { /*Error Packet                        */
            ErrorPacket();
        }
        else if (iblk == 2045)
        { /*Image Header Packet                 */
            if (FoundHeader)
            {
                Error((char *)"Header has already been read!", 0, 0, 0);
            }
            else
            {
                FoundHeader = 1;
                ImageHeader(&BCol1, &BCol2, &BRow1, &BRow2,
                            &MinPixAll, &MaxPixAll,
                            &Method, &SignFlag, &nshift, &SqrtFlag,
                            &ncol, &nrow, &Valid);
                if (Method < 0 || Method > 3 || MinPixAll > MaxPixAll ||
                    SignFlag < 0 || SignFlag > 1 ||
                    nshift < 0 || nshift > 15 || SqrtFlag < 0 || SqrtFlag > 1 ||
                    ncol < 0 || ncol > maxncol || BCol1 < 0 || BCol1 > BCol2 ||
                    BCol2 >= MaxBlkH || BRow1 < 0 || BRow1 > BRow2 || BRow2 >= MaxBlkV)
                    Error((char *)"Incorrect header parameter", 0, 0, 0);
                nBitPix = nBitNeed((ulong)(MaxPixAll - MinPixAll));
                if (Valid == 0)
                    FoundHeader = 0;
            }
        }
        else
        {
            if (Valid)
                OtherBlock(iblk);
        }
        EndPacket();
    }

/*============================================================================*/
#define RiceBlk 16 /*Rice block size (factor of BlkSz), \
                     one side                           */
#define RiceBlks (BlkSz / RiceBlk)
#define RiceBlks2 (RiceBlks * RiceBlks)
#define nRiceBit 4  /*log base 2 of RiceBlk               */
#define nRiceBits 2 /*log base 2 of RiceBlks              */
    /*============================================================================*/
    void SOHORiceDecompressor::RiceRecon()
    {                           /*Called by ReconPacket to Reconstruct
                                  1 block using Rice Algorithm        */
                                /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        uword Predict[nPixBlk]; /*Predict values for pixels           */
        uword MaxPix;           /*Maximum pixel value                 */
        uword MaxPixBit;        /*# of bits needed to send MaxPix     */
        int nSum, Bit, i, ii, jj;
        int RiceAdapt[RiceBlks2], MinAdapt;
        uword MSB, MaxMSB, Adapt;
        long Sum, Diff, Theta, Pix;
        MaxPix = (uword)RdBit(nBitPix) + MinPixAll;
        MaxPixBit = nBitNeed((ulong)MaxPix);
        Pix = (ulong)((uword)RdBit(MaxPixBit) + MinPixAll);
        Predict[0] = (uword)Pix;
        Block[0] = Pix;
        MinAdapt = RdBit(4);
        Adapt = (uword)RdBit(3);
        for (i = 0; i < RiceBlks2; i++)
            RiceAdapt[i] = RdBit((int)Adapt) + MinAdapt;
        for (i = 1; i < nPixBlk; i++)
        {
            jj = i >> nBlkBit;
            ii = i - (jj << nBlkBit);
            Sum = nSum = 0;
            if (jj > 0)
            {
                if (ii > 0)
                {
                    Sum += (long)(ulong)Predict[i - BlkSz - 1];
                    nSum++;
                }
                Sum += (long)(ulong)Predict[i - BlkSz];
                nSum++;
                if (ii < BlkSz - 1)
                {
                    Sum += (long)(ulong)Predict[i - BlkSz + 1];
                    nSum++;
                }
            }
            if (ii > 0)
            {
                Sum += (long)(ulong)Predict[i - 1];
                nSum++;
            }
            Predict[i] = (uword)((Sum + (nSum >> 1)) / nSum);
            jj = jj >> nRiceBit;
            ii = ii >> nRiceBit;
            Adapt = RiceAdapt[ii + (jj << nRiceBits)];
            if (Adapt == 15u)
            {
                Pix = (ulong)((uword)RdBit(MaxPixBit) + MinPixAll);
            }
            else if (Adapt > 0u)
            {
                MSB = 0;
                MaxMSB = MaxPix >> Adapt;
                while ((Bit = RdBit(1)) == 0 && AtEnd == 0 && MSB < (uword)(MaxMSB - 1u))
                    MSB++;
                if (Bit == 0)
                    MSB++;
                Diff = (ulong)((uword)(MSB << Adapt) | (uword)RdBit(Adapt));
                Theta = (long)(ulong)MaxPix - (long)(ulong)Predict[i];
                if (Theta > (long)(ulong)Predict[i])
                    Theta = (long)(ulong)Predict[i];
                if (Diff <= Theta + Theta)
                {
                    if (Diff & 1)
                        Diff = -(Diff + 1) / 2;
                    else
                        Diff = Diff / 2;
                }
                else
                {
                    Diff = Diff - Theta;
                    if (Theta != (long)(ulong)Predict[i])
                        Diff = -Diff;
                }
                Pix = (long)(ulong)Predict[i] + Diff;
            }
            else
            {
                Pix = (long)(ulong)Predict[i];
            }
            if (Pix > (long)(ulong)MaxPix)
            {
                Error((char *)"Out of bounds predict value (%d at pixel %d)", 0, (short)Pix, i);
                return;
            }
            Predict[i] = (uword)Pix;
            Block[i] = Pix;
        }
    }

    /*============================================================================*/
    void SOHORiceDecompressor::NoRecon()
    { /*Called by ReconPacket to use
        uncompressed data                   */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        int i;
        for (i = 0; i < nPixBlk; i++)
            Block[i] = (long)(ulong)RdBit(16);
    }

    /*============================================================================*/
    void SOHORiceDecompressor::ErrorPacket()
    { /*Read an error packet                */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        long iblk2, error, Method, Phase, Process;
        iblk2 = RdBitL(32);
        error = RdBitL(32);
        Method = RdBitL(32);
        Phase = RdBitL(32);
        Process = RdBitL(32);
#if ENABLE_PRINTF_OUT
        Error("===Encountered Error Packet Error Code %d===", 0, (int)error, 0);
        printf("iblk=%ld error=%ld Method=%ld Phase=%ld Process=%ld\n",
               iblk2, error, Method, Phase, Process);
#endif
        if (error == 1)
            printf("Incorrect Method or Phase detected by InitComp\n");
        if (error == 2)
            printf("Incorrect Method, Phase or iblk detected by Comp\n");
        if (error == 3)
            printf("Out of range nByteP\n");
        if (error == 4)
            printf("ADCT Binary search for # of words did not converge\n");
        if (error == 5)
            printf("Invalid Scaling factor on initial packet\n");
        if (error == 6)
            printf("Invalid ADCT decision table scaling\n");
        if (error == 7)
            printf("Invalid ADCT coefficient scaling\n");
        if (error == 8)
            printf("Invalid ADCT Block Class\n");
        if (error == 9)
            printf("Invalid ADCT intermediate scaling\n");
        if (error == 10)
            printf("Incorrect block detected by Compress\n");
        if (error == 11)
            printf("Incorrect input parameters detected by Compress\n");
        if (error == 12)
            printf("Pixel value has more than nPixBit bits\n");
        if (error == 13)
            printf("Maximum pixel value has more than nPixBit bits\n");
        if (error == 14)
            printf("Average pixel value has too many bits\n");
    }

    /*============================================================================*/
    void SOHORiceDecompressor::Error(char *Mes, int iexit, int Var1, int Var2)
    { /*Process error codes  */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        if (Valid != 0 && OldValid != 0)
        { /*Don't keep giving errors if
         This packet or the previous
         one has already been flagged*/
            printf("\n=ERROR=");
            printf("%s %d %d", Mes, Var1, Var2);
            printf("\n");
        }
        Valid = 0;
        if (iexit != 0)
            logger->critical("Wanted to exit!"); //    exit(0);
    }

    /*============================================================================*/
    void SOHORiceDecompressor::Warning(char *Mes, long Var1, long Var2)
    { /*Process warning codes       */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        printf("\n\a====================WARNING====================\n");
        printf(Mes, Var1, Var2);
        printf("\n");
    }

    /*============================================================================*/
    int SOHORiceDecompressor::RdBit(int ibit)
    { /*Read ival from Pack buffer in ibit
         bits                               */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        uword nDiscard, ival, MSB;
        if (ibit < 0 || ibit > 32)
            Error((char *)"Invalid RdBit %d \n", 0, ibit, 0);
        if (AtEnd)
            return 0;
        CheckSum += ibit;
        ival = 0;
        while (ibit > 0)
        {
            if (nBitP <= 0)
            {
                if (nByteP >= nBytePacket || nByteP < 0)
                {
                    AtEnd = 1; /*End of file flag                    */
                    Error((char *)"Packet extended past end of file", 0, nByteP, nBytePacket);
                }
                NextByte = Pack[nByteP++];
                nBitP = 8;
            }
            if (ibit <= nBitP)
            {
                nDiscard = nBitP - ibit;
                MSB = NextByte >> nDiscard;
                ival = (ival << (uword)ibit) | MSB;
                NextByte -= MSB << nDiscard;
                nBitP -= ibit;
                ibit = 0;
            }
            else
            {
                ival = (ival << nBitP) | NextByte;
                ibit -= nBitP;
                nBitP = 0;
            }
        }
        CheckSum += ival;
        return (int)(unsigned)ival;
    }

    /*============================================================================*/
    long SOHORiceDecompressor::RdBitL(int ibit)
    { /*Same as RdBit, for longs             */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        long ival;
        ival = 0;
        if (ibit > 16)
        {
            ival = (long)RdBit(ibit - 16);
            ibit = 16;
        }
        ival = (ival << 16) | (long)RdBit(ibit);
        return ival;
    }

    /*============================================================================*/
    int SOHORiceDecompressor::nBitNeed(ulong i)
    { /*# of bits needed to represent i
        (i must be >= 0)                    */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        ulong j;
        int need;
        j = i;
        need = 0;
        while (j > 0)
        {
            j = j >> 1u;
            need++;
        }
        return need;
    }

    ///////////////////////////////////////////////////////// rice64.c

    /*============================================================================*/
    void SOHORiceDecompressor::ReadPack(uchar Pack[], int *Valid, int *AtEnd, int *nByteP,
                                        int *nBitP, int *nBytePacket)
    {                               /*Read 1 packet into Pack.
                                      Set AtEnd=0 if there was a packet,
                                                1 if end of image reached.
                                      For right now we ignore Science
                                      super-packet structure, and we assume
                                      no transmission errors.             */
                                    /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        static int FirstPacket = 1; /*First packet?                       */
        static int byterev = 0;     /*Need to byte reverse file?          */
        static long PrevStart;      /*Previous packet start position      */
        int iblk, nWdP;
        uchar c;
        int i;
#ifndef SEEK_SET
#define SEEK_SET 0 /*Should be part of C language--but \
                     isn't for gcc.                      */
#endif
        if (*Valid == 0)
        { /*Handle invalid packets              */
            *Valid = 1;
            PrevStart += 2;
/*&&&printf("Re-trying at file position %ld\n",PrevStart);                */
#if RICE_MEMORY_VERSION
            if (bfseek(PrevStart, SEEK_SET))
#else
            if (fseek(fp1, PrevStart, SEEK_SET))
#endif
                Error((char *)"Bad fseek call", 1, 0, 0);
        }
        else
        {
            *Valid = 1;
        }
#if RICE_MEMORY_VERSION
        if ((PrevStart = bftell()) < 0)
#else
        if ((PrevStart = ftell(fp1)) < 0)
#endif
            Error((char *)"Bad ftell call", 1, 0, 0);

        *AtEnd = 0; /*Initialize                          */
#if RICE_MEMORY_VERSION
        if (bfeof() != 0 || bfread(Pack, (size_t)2, (size_t)2) != (size_t)2)
#else
        if (feof(fp1) != 0 || fread(Pack, (size_t)2, (size_t)2, fp1) != (size_t)2)
#endif
        {
            *AtEnd = 1;
            return;
        }
        if (byterev)
        {
            for (i = 0; i < 4; i += 2)
            {
                c = Pack[i];
                Pack[i] = Pack[i + 1];
                Pack[i + 1] = c;
            }
        }
        *nByteP = *nBitP = 0;
        *nBytePacket = 4;      /*So we can use RdBit to get length  */
        iblk = RdBit(nBksBit); /*Block #                             */
        if (FirstPacket)
        { /*Check for byte reversal             */
            if (iblk == 2045 || iblk == 2047)
            { /*Found first packet or error packet! */
                FirstPacket = 0;
            }
            else
            { /*If not, try byte reversal           */
                for (i = 0; i < 4; i += 2)
                {
                    c = Pack[i];
                    Pack[i] = Pack[i + 1];
                    Pack[i + 1] = c;
                }
                *nByteP = *nBitP = 0;
                iblk = RdBit(nBksBit);
                if (iblk == 2045 || iblk == 2047)
                {
                    FirstPacket = 0;
                    byterev = 1;
                    /*printf("Words being byte reversed!\n");*/
                }
                else
                {
                    Error((char *)"First block not image header--", 0, 0, 0);
                }
            }
        }
        nWdP = RdBit(MaxWBit);
        if (nWdP > 2)
        {
#if RICE_MEMORY_VERSION
            if (bfread(&Pack[4], (size_t)(nWdP - 2), (size_t)2) != (size_t)2)
#else
            if (fread(&Pack[4], (size_t)(nWdP - 2), (size_t)2, fp1) != (size_t)2)
#endif
            {
                Error((char *)"Packet extended past end of file", 0, 0, 0);
            }
        }
        if (byterev)
        {
            *nBytePacket = nWdP * 2;
            for (i = 4; i < *nBytePacket; i += 2)
            {
                c = Pack[i];
                Pack[i] = Pack[i + 1];
                Pack[i + 1] = c;
            }
        }
    }

    /*The user must supply an ImageHeader routine to read the image header packet:

            =======================Form of Call=======================

            void ImageHeader(int *BCol1, int *BCol2, int *BRow1, int *BRow2,
              uword *MinPixAll, uword *MaxPixAll,
              int *Method, int *SignFlag, int *nshift, int *SqrtFlag,
              int *ncol, int *nrow, int *Valid);
            ImageHeader(&BCol1,&BCol2,&BRow1,&BRow2,
             &MinPixAll,&MaxPixAll,
             &Method,&SignFlag,&nshift,&SqrtFlag)
             &ncol,&nrow,&Valid);

            ======================Input Variables=====================

            Input is through RdBit routine
            Valid   =non-zero if everything in block was valid--may be
                     changed by RdBit mid-way through.

            ============================Output=============================

            BCol1   =Starting 0-Origin block column number.
            BCol2   =Ending   0-Origin block column number.
                     Means software will transmit 0-origin columns
                            BCol1*BlkSz to BCol2*BlkSz+BlkSz-1

            BRow1   =Starting 0-Origin block row    number.
            BRow2   =Ending   0-Origin block row    number.
                     Means software will transmit 0-origin rows
                     BRow1*BlkSz to BRow2*BlkSz+BlkSz-1
            MaxPixAll=Maximum pixel value
            MinPixAll=Minimum pixel value:
            Method  =Compression Method.
                     0=Uncompressed (used only to debug system)
                     1=(Rice Algorithm) Lossless Compression
                     2=(ADCT Algorithm) Lossy Compression
                     3-7 Reserved
            SignFlag=0 if image pixels unsigned, non-zero if signed
            nshift  =# of bits input should be is to be right shifted
            SqrtFlag=non-zero if square root is to be taken
            ncol    =# of columns in output image&&&
            nrow    =# of rows    in output image


    Our example follows.  The user may change ImageHeader to match a particular
    format.  If that is done, the ImageHeader routine in the compression program
    must also be changed.  This program, if modified, must still set all of the
    variables included in this example:                                           */

    /*============================================================================*/

    void SOHORiceDecompressor::ImageHeader(int *BCol1, int *BCol2, int *BRow1, int *BRow2,
                                           uword *MinPixAll, uword *MaxPixAll,
                                           int *Method, int *SignFlag, int *nshift, int *SqrtFlag,
                                           int *ncol, int *nrow, int * /*Valid*/)
    { /*Reconstruct Image Header Packet     */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96 */
        long index;
        uword *Image0;
        *Method = RdBit(3);
        *BCol1 = RdBit(nBitNeed((ulong)(MaxBlkH - 1)));
        *BCol2 = RdBit(nBitNeed((ulong)(MaxBlkH - 1)));
        *BRow1 = RdBit(nBitNeed((ulong)(MaxBlkV - 1)));
        *BRow2 = RdBit(nBitNeed((ulong)(MaxBlkV - 1)));
        *ncol = (*BCol2 - *BCol1 + 1) * BlkSz;
        *nrow = (*BRow2 - *BRow1 + 1) * BlkSz;
        *MaxPixAll = (uword)RdBit(16);
        *MinPixAll = (uword)RdBit(nBitNeed((ulong)*MaxPixAll));
        *SignFlag = RdBit(1);
        *nshift = RdBit(4);
        *SqrtFlag = RdBit(1);
#if ENABLE_PRINTF_OUT
        printf("Method=%d BCol1=%d BCol2=%d BRow1=%d BRow2=%d ncol=%d nrow=%d\n",
               *Method, *BCol1, *BCol2, *BRow1, *BRow2, *ncol, *nrow);
        printf("MinPixAll=%hu MaxPixAll=%hu SignFlag=%d nshift=%d SqrtFlag=%d\n",
               *MinPixAll, *MaxPixAll, *SignFlag, *nshift, *SqrtFlag);
#endif

#ifdef DEBUG2
        /* test pattern */
        printf("Checking DEBUG2 ImageHeader\n");
        uword icount = 0, power[12];
        uword power2[12] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
        for (icount = 0; icount < 12; icount++)
        {
            power[icount] = RdBit(nBitNeed(power2[icount]));
            if (power[icount] != power2[icount])
            {
                printf("Imageheader() : DEBUG2 error\n");
            }
        }
#endif

        // Aang23 : FIX OVERFLOW
        if (maxncol < *ncol || maxnrow < *nrow || *nrow < 0 || *ncol < 0)
        {
            output_was_valid = false;
            *ncol = maxncol;
            *nrow = maxnrow;
        }

        Image0 = Image; /*Zero fill image.                    */
        for (index = *ncol * (long)*nrow; index; index--)
        {
            *Image0 = 0;
            Image0++;
        }
        EndPacket();
#if ENABLE_PRINTF_OUT
        if (*Valid)
            printf("(Successful Read of Image Header Packet)--\n");
        printf("--------------------------------------------------------\n");
#endif
    }
    /*The user must supply an OtherBlock routine to handle block types (as stated
    earlier, code 7666 is free to assign other block types 2021-2040) not specified
    by code 7230.  Our example simply prints a message:                           */

    /*============================================================================*/
    void SOHORiceDecompressor::OtherBlock(int /*iblk*/)
    { /*Interpret other types of block      */
        /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        static unsigned int firstOther = 0;
#if ENABLE_PRINTF_OUT
        if (firstOther == 0)
            printf("\n--Undefined Block type %d- ", iblk);
        else
            printf("%d-");
#endif
        firstOther++;
    }
    /*The user must supply a UseBlock routine to specify which blocks may have been
    sent.  (Certain blocks in the images will be occulted, and so do not need to be
    sent.)  All blocks in the ranges specified by BCol, BCol2, BRow1 and BRow2 will
    have been sent unless the UseBlock routine returned zero on board the
    spacecraft; ideally UseBlock should return the same values on the ground so that
    invalid block numbers will be detectable.  However, if no transmission errors
    occur, that will not be necessary.

            =======================Form of Call=======================

            result=UseBlock(iBlkH,iBlkV)

            ======================Input Variables=====================

            iBlkH   =(int) 0-origin sub-image block column #
            iBlkV   =(int) 0-origin sub-image block row    #
      (The sub image block referred to by iBlkH,iBlkV includes the following pixels:
      Image[iBlkH*BlkSz to (iBlkH+1)*BlkSz-1, iBlkV*BlkSz to (iBlkV+1)*BlkSz-1]


            =====================Output Variables=====================

            result  =(int)0 if block is not to be transmitted.
                     non-zero if block is to be sent

    In our example, UseBlock will always indicate that the block number was valid:*/

    /*============================================================================*/
    int SOHORiceDecompressor::UseBlock(int /*iBlkH*/, int /*iBlkV*/)
    {             /*Return 0 if sub-image block is to be
                    sent, else return non-zero.         */
                  /*  Written by Mitchell R Grunes, ATSC/NRL, 07/11/96                          */
        return 1; /*For this example we always send.    */
    }
}
#pragma GCC diagnostic pop
