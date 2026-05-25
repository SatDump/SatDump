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
#include <math.h>
#include "global.h"

extern long DeConvTwosComp(DWORD32 complement, short leftmost);

void AdjustOutPut(StructCodingPara * PtrCoding, BitPlaneBits * BlockCodingInfo)// need to adjust the output
{
   int i = 0;
   int m = 0;
   int n = 0;
   int b_DC= 0;
   short beta_1 = 0;
   short beta_2 = 0;
   float refinement = 0;
   DWORD32 BitPlaneCheck = 0; // to check if a bit plane has bit 1.
   int TotalBlocks = (int)PtrCoding->PtrHeader->Header.Part3.S_20Bits;

   // determine the stages to do the decoding process.


   if (PtrCoding->PtrHeader->Header.Part4.DWTType != INTEGER_WAVELET)
   {
      for ( i = 0;  i < TotalBlocks; i ++)

         for ( m = 0; m < 8; m++)
            for ( n = 0; n < 8; n ++)
               BlockCodingInfo[i].PtrBlockAddressFloating[m][n] = (float) BlockCodingInfo[i].PtrBlockAddress[m][n] ;

   }
   for ( i = 0;  i < TotalBlocks; i ++)
   {
      /* The following line is buggy: */
      /*
      *(*BlockCodingInfo[i].PtrBlockAddress) = DeConvTwosComp((DWORD32)(BlockCodingInfo[i].ShiftedDC + BlockCodingInfo[i].DecodingDCRemainder),
         PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits);
      */
      /* Here's the bug fix (Aaron Kiely, June 25,2008) */
      *(*BlockCodingInfo[i].PtrBlockAddress) = DeConvTwosComp((DWORD32)((long int)BlockCodingInfo[i].ShiftedDC + (long int)BlockCodingInfo[i].DecodingDCRemainder),
         PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits);

      *(*BlockCodingInfo[i].PtrBlockAddressFloating)  = (float) (*(*BlockCodingInfo[i].PtrBlockAddress));
   }

   if((PtrCoding->RateReached == TRUE) &&
      (PtrCoding->DecodingStopLocations.BlockNoStopDecoding != -1)
      &&(PtrCoding->DecodingStopLocations.BitPlaneStopDecoding != -1))
      {
         // adjust DC and AC componets.
               // integer and floating wavelet, we have two differenet strategies.

         // decoder up to the block.
         if (PtrCoding->DecodingStopLocations.BitPlaneStopDecoding <= PtrCoding->QuantizationFactorQ)
            b_DC = PtrCoding->DecodingStopLocations.BitPlaneStopDecoding;
         else
            b_DC = PtrCoding->QuantizationFactorQ;


         if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
                     // first adjust DC component.
         {
            if (b_DC >= 1)
               for( i = 0; i < TotalBlocks; i ++)
                  *(*BlockCodingInfo[i].PtrBlockAddress) += (1 << (b_DC - 1));

            if(PtrCoding->DecodingStopLocations.BitPlaneStopDecoding >= 1)
            {
               beta_1 = (1 << ( PtrCoding->DecodingStopLocations.BitPlaneStopDecoding - 1)) - 1;
               beta_2 = (1 << ( PtrCoding->DecodingStopLocations.BitPlaneStopDecoding )) - 1;
            }
            else
            {
               beta_1 = 0;
               beta_2 = 0;
            }
            BitPlaneCheck = 1 << PtrCoding->DecodingStopLocations.BitPlaneStopDecoding;

            if (PtrCoding->DecodingStopLocations.stoppedstage == 1) // only stopped at parents. using beta_2 for all children and grandchildren.
            {
                for (i = 0; i < TotalBlocks; i ++)
                {
                  if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_1;
                  }
                  else  if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_2;
                  }
                   for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                        continue;
                         if (m > 1  || n > 1) //grandchildren and grandchildren
                         {
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                         }
                         else  //parent
                         {
                            if (i <  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                               // else because the refinement bits have not yet decoded, add beta2
                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                               // else because the refinement bits have not yet decoded, add beta2
                               refinement = beta_2;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else   // current block
                            {
                               if((m <= PtrCoding->DecodingStopLocations.X_LocationStopDecoding) &&
                                  (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding ))
                               {
                                  if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                     refinement = beta_2;
                                  else
                                     refinement = beta_1;

                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                               }
                               else if (( PtrCoding->DecodingStopLocations.X_LocationStopDecoding == 1)
                                  && ( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding == 0))
                               {
                                  if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                     refinement = beta_2;
                                  else
                                     refinement = beta_1;

                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                               }
                               else
                               {
                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                               }
                            }
                         }
                      }
                }
            }
            else if (PtrCoding->DecodingStopLocations.stoppedstage == 2) // only stopped at children. using beta2 for all and grandchildren and beta 1
               // for all parents.
            {
                for (i = 0; i < TotalBlocks; i ++)
                {
                    for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                            continue;
                         if ((m > 3  || n >3))
                         {
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                         }
                         else if  ((m <= 1) && (n <= 1))  //parent and grandchildren //parent
                         {
                             if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                            if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                         }
                         else
                         {
                            if (i<  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {

                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                               refinement = beta_2;
                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }

                            else   // current block
                            { // children
                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                              if ((m < 4) && (n < 4))
                              {
                                 if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2) // upper right block
                                     {
                                        if  (m >= 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                                       }
                                        else
                                        {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                                 refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }
                                     }
                                 else if( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2) // lower left block
                                     {
                                        if  (m < 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -=  (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                        else  if  (n >= 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                                       }
                                        else
                                        {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                                 refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }
                                     }
                                 else // lower right block
                                     {
                                        if  (m < 2 || n < 2)
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                        else
                                        {
                                           if ((m >= PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n >= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                           else
                                           {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;
                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        }

                                     }
                               }
                              else
                               {
                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -= beta_2;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                               }

                         }
                      }
                }
                 }
            }
            else if  (PtrCoding->DecodingStopLocations.stoppedstage == 3)// parents coding stage.
            {
               for (i = 0; i < TotalBlocks; i ++)
                {
                    for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                            continue;
                         //parent and children
                         if ((m <= 3  && n <= 3))
                         {

                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                              refinement = beta_2;
                           else
                              refinement = beta_1;

                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                         }
                         else
                         {
                            if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {

                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                              refinement = beta_2;
                           else
                              refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else   if(i > PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                              refinement = beta_2;
                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                            }

                            else // i == PtrCoding->DecodingStopLocations.BlockNoStopDecoding  stopped at stage 3, gradnchildren
                            { // grandchildren

                               if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4) // stop at upper right block
                               {
                                  if (m >= 4)
                                  {
                                    refinement = beta_2;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                  else
                                     if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 2 || n >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 2) && ( n <= 5))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 2)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 2)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 6)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 2) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                              }

                              else if(PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4) // stop at upper right block
                              {
                                    if ( m < 4)
                                 {

                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                 }
                                 else if (n >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else

                                     if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2))
                                    {
                                       if (m >= 6 || n >= 2)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 6) && ( n < 2))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2)  // the 3rd 2 * 2 block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 2)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 6) || (n < 2))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                              }

                              else
                              {
                                 if  (m  < 4 || n < 4)
                                  {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                    refinement = beta_2;
                                 else
                                    refinement = beta_1;
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                     if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                     // there are four small blocks to be considered.
                                  else if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 6 || n >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 6) && ( n < 6))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 6)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 6) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                 }


                         }
                      }
                }


                 }
            }
            else // refinement process
            {
               for (i = 0; i < TotalBlocks; i ++)
               {
                  if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_1;
                  }
                  else  if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_2;
                  }

                  if (i <  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta_1 ,
                           // else because the refinement bits have not yet decoded, add beta_2
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                        }
                  }
                  else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {

                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;

                            if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                               refinement = beta_2;
                            else
                              refinement = beta_1;
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                           // else because the refinement bits have not yet decoded, add beta2
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                        }
                  }
                  else   // current block
                  {
                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;
                           if ((PtrCoding->DecodingStopLocations.X_LocationStopDecoding <= 1)// stop at parent
                              && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding <= 1)) // stop at parents
                               {
                                  if((m <= PtrCoding->DecodingStopLocations.X_LocationStopDecoding) &&
                                     (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding ))
                                  {
                                        refinement = beta_1;

                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                     if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                  else if (( PtrCoding->DecodingStopLocations.X_LocationStopDecoding == 1)
                                     && ( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding == 0))
                                  {
                                     if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                        refinement = beta_2;
                                     else
                                        refinement = beta_1;

                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;
                                      if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                         BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                      else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                         BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                  else
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                        refinement = beta_2;
                                     else
                                        refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }


                               }
                           else if ((PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4) // stop at children
                              && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4)) // stop at children
                               {
                                  if( m < 2 && n <2)
                                  {
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                    continue;
                                  }
                                  else if (m > 3 || n > 3)
                                  {
                                      if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                         refinement = beta_2;
                                      else
                                         refinement = beta_1;
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                    continue;
                                  }

                                 if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2) // upper right block
                                     {

                                        if (m > 1)
                                        {
                                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                        }
                                        else if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                     }
                                 else if( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2) // lower left block
                                     {
                                        if (m < 2)
                                        {
                                           refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else if (n >= 2)

                                        {
                                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                        }


                                         else if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                 }
                                 else // lower right block
                                     {
                                        if  (m < 2 || n < 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                        else
                                        {
                                            if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }

                                     }
                               }
                           else // stop at grandchildren
                           {                //parent and children
                              if ((m <= 3  && n <= 3))
                              {
                                 if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                    BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                 else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                    BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                 if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                    BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                 else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                    BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                 continue;
                              }
                              else if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4)  //upper right grandchildren
                              {
                                 if ( m >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 2 || n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                    {
                                       if ((m < 2) && ( n <= 5))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if( m < 2)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 2) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                 }
                              }
                              else if (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4)//lower left grandchildren
                              {
                                 if ( m < 4)
                                 {
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                 }
                                 else if (n >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2))
                                    {
                                       if (m >= 6 || n >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if ((m < 6) && ( n <= 1))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2)
                                    {
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 6) || (n < 2))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }



                                 }


                              }
                              else   // lower right
                              {
                                 if ((m < 4)  || ( n  < 4))
                                 {
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                    continue;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 6 || n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if ((m < 6) && ( n <= 6))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 6) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }

                                 }
                              }
                              }
                           }
                        }
                  }
                   }
                }
         else // floating
         {
            float beta_1,    beta_2;
            BitPlaneCheck = (1 << PtrCoding->DecodingStopLocations.BitPlaneStopDecoding);
            if (b_DC >= 1)
            {
               float temp = (float)((1 << (b_DC - 1))  - 0.5);
               for( i = 0; i < TotalBlocks; i ++)
                    *(*BlockCodingInfo[i].PtrBlockAddressFloating) += temp;
            }

            if(PtrCoding->DecodingStopLocations.BitPlaneStopDecoding >= 1)
            {
               beta_1 =(float)( (1 << ( PtrCoding->DecodingStopLocations.BitPlaneStopDecoding - 1)) - 0.5);
               beta_2 = (float)((1 << ( PtrCoding->DecodingStopLocations.BitPlaneStopDecoding )) - 0.5);
            }
            else
            {
               beta_1 = 0;
               if (PtrCoding->DecodingStopLocations.BitPlaneStopDecoding ==0)
               beta_2 = 0.5;
            }


            if (PtrCoding->DecodingStopLocations.stoppedstage == 1) // only stopped at parents. using beta_2 for all children and grandchildren.
            {
                for (i = 0; i < TotalBlocks; i ++)
                {

                  if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                     refinement = beta_1;
                  else  if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                     refinement = beta_2;

                   for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                        continue;
                         if (m > 1  || n > 1) //grandchildren and grandchildren
                         {
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short)beta_2;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                         }
                         else  //parent
                         {
                            if (i <  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                               // else because the refinement bits have not yet decoded, add beta2
                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                               // else because the refinement bits have not yet decoded, add beta2
                               refinement = beta_2;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else   // current block
                            {
                               if((m <= PtrCoding->DecodingStopLocations.X_LocationStopDecoding) &&
                                  (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding ))
                               {
                                  if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                     refinement = beta_2;
                                  else
                                     refinement = beta_1;

                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                               }
                               else if (( PtrCoding->DecodingStopLocations.X_LocationStopDecoding == 1)
                                  && ( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding == 0))
                               {
                                  if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                     refinement = beta_2;
                                  else
                                     refinement = beta_1;

                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                               }
                               else
                               {
                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += (short)beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short)beta_2;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                               }
                            }
                         }
                      }
                }
            }
            else if (PtrCoding->DecodingStopLocations.stoppedstage == 2) // only stopped at children. using beta2 for all and grandchildren and beta 1
               // for all parents.
            {
                for (i = 0; i < TotalBlocks; i ++)
                {
                    for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                            continue;
                         if ((m > 3  || n >3))
                         {
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (short)beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short)beta_2;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                         }
                         else if  ((m <= 1) && (n <= 1))  //parent and grandchildren //parent
                         {
                             if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                            if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                         }
                         else
                         {
                            if (i<  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {

                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {

                                  refinement = beta_2;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }

                            else   // current block
                            { // children
                               if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                  refinement = beta_2;
                               else
                                  refinement = beta_1;

                              if ((m < 4) && (n < 4))
                              {
                                 if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2) // upper right block
                                     {
                                        if  (m >= 2)
                                        {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short)beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short)beta_2;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                                       }
                                        else
                                        {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                                 refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }
                                     }
                                 else if( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2) // lower left block
                                     {
                                        if  (m < 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -=  (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                        else  if  (n >= 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short)beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short)beta_2;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                                       }
                                        else
                                        {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                                 refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }
                                     }
                                 else // lower right block
                                     {
                                        if  (m < 2 || n < 2)
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                        else
                                        {
                                           if ((m >= PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n >= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                           else
                                           {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;
                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        }

                                     }
                               }
                              else
                               {
                                  if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] += (short)beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddress[m][n] -=(short) beta_2;

                                  if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_2;
                                  else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                     BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_2;
                               }

                         }
                      }
                }
                 }
            }
            else if  (PtrCoding->DecodingStopLocations.stoppedstage == 3)// parents coding stage.
            {
               for (i = 0; i < TotalBlocks; i ++)
                {
                    for(m = 0; m < 8; m++)
                      for(n = 0; n < 8; n++)
                      {
                         if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                            continue;
                         //parent and children
                         if ((m <= 3  && n <= 3))
                         {

                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                              refinement = beta_2;
                           else
                              refinement = beta_1;

                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                         }
                         else
                         {
                            if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {

                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                              refinement = beta_2;
                           else
                              refinement = beta_1;

                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                           }
                            else   if(i > PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                            {
                              refinement = beta_2;
                               if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                               if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                               else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                  BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                            }

                            else // i == PtrCoding->DecodingStopLocations.BlockNoStopDecoding  stopped at stage 3, gradnchildren
                            { // grandchildren

                               if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4) // stop at upper right block
                               {
                                  if (m >= 4)
                                  {
                                    refinement = beta_2;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                  else
                                     if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 2 || n >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 2) && ( n <= 5))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 2)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 2)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 6)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 2) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                              }

                              else if(PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4) // stop at upper right block
                              {
                                    if ( m < 4)
                                 {

                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                 }
                                 else if (n >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else

                                     if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2))
                                    {
                                       if (m >= 6 || n >= 2)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 6) && ( n < 2))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2)  // the 3rd 2 * 2 block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 2)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 6) || (n < 2))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                              }

                              else
                              {
                                 if  (m  < 4 || n < 4)
                                  {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                    refinement = beta_2;
                                 else
                                    refinement = beta_1;
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                     if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                     // there are four small blocks to be considered.
                                  else if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 6 || n >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {

                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;

                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if ((m < 6) && ( n < 6))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          refinement = beta_2;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else if (n >= 6)
                                       {
                                          refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                        else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                          refinement = beta_2;
                                       else
                                          refinement = beta_1;
                                       if((m < 6) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                           }
                                          else
                                        {
                                              refinement = beta_2;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                 }


                         }
                      }
                }


                 }
            }
            else // refinement process
            {
               for (i = 0; i < TotalBlocks; i ++)
               {
                  if (i < PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_1;
                  }
                  else  if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     refinement = beta_2;
                  }

                  if (i <  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {
                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta_1 ,
                           // else because the refinement bits have not yet decoded, add beta_2
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                        }
                  }
                  else if (i >  PtrCoding->DecodingStopLocations.BlockNoStopDecoding)
                  {

                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;

                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                              refinement = beta_2;
                           else
                              refinement = beta_1;
                               // have to make a decision if the pixels is newly selected at this stage.
                               // If it is, then add the beta1,
                           // else because the refinement bits have not yet decoded, add beta2
                           if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                           if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                           else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                              BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                        }
                  }
                  else   // current block
                  {
                     for(m = 0; m < 8; m++)
                        for(n = 0; n < 8; n++)
                        {
                           if ((m == 0 && n == 0) || (BlockCodingInfo[i].PtrBlockAddress[m][n] == 0))
                              continue;
                           if ((PtrCoding->DecodingStopLocations.X_LocationStopDecoding <= 1)// stop at parent
                              && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding <= 1)) // stop at parents
                               {
                                  if((m <= PtrCoding->DecodingStopLocations.X_LocationStopDecoding) &&
                                     (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding ))
                                  {
                                        refinement = beta_1;

                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                     if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                  else if (( PtrCoding->DecodingStopLocations.X_LocationStopDecoding == 1)
                                     && ( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding == 0))
                                  {
                                     if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                        refinement = beta_2;
                                     else
                                        refinement = beta_1;

                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                     else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                        BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;
                                      if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                         BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                      else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                         BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                  }
                                  else
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                        refinement = beta_2;
                                     else
                                        refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }


                               }
                           else if ((PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4) // stop at children
                              && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4)) // stop at children
                               {
                                  if( m < 2 && n <2)
                                  {
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                    continue;
                                  }
                                  else if (m > 3 || n > 3)
                                  {
                                      if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                         refinement = beta_2;
                                      else
                                         refinement = beta_1;
                                     if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                    continue;
                                  }

                                 if( PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2) // upper right block
                                     {

                                        if (m > 1)
                                        {
                                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                        }
                                        else if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                     }
                                 else if( PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2) // lower left block
                                     {
                                        if (m < 2)
                                        {
                                           refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;

                                        }
                                        else if (n >= 2)

                                        {
                                           if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                        }


                                         else if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                 }
                                 else // lower right block
                                     {
                                        if  (m < 2 || n < 2)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                        else
                                        {
                                            if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                        }

                                     }
                               }
                           else // stop at grandchildren
                           {                //parent and children
                              if ((m <= 3  && n <= 3))
                              {
                                 if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                    BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                 else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                    BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                 if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                    BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                 else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                    BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                 continue;
                              }
                              else if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 4)  //upper right grandchildren
                              {
                                 if ( m >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 2 || n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 2)
                                    {
                                       if ((m < 2) && ( n <= 5))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if( m < 2)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 2) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                 }
                              }
                              else if (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 4)//lower left grandchildren
                              {
                                 if ( m < 4)
                                 {
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                 }
                                 else if (n >= 4)
                                 {
                                    if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                       refinement = beta_2;
                                    else
                                       refinement = beta_1;
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2))
                                    {
                                       if (m >= 6 || n >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if ((m < 6) && ( n <= 1))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 2)
                                    {
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 2)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 6) || (n < 2))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }



                                 }


                              }
                              else   // lower right
                              {
                                 if ((m < 4)  || ( n  < 4))
                                 {
                                    if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                    else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                    if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                    else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                       BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                    continue;
                                 }
                                 else
                                 {
                                    if( (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                       && (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6))
                                    {
                                       if (m >= 6 || n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          continue;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }

                                       }
                                    }
                                    else   if (PtrCoding->DecodingStopLocations.X_LocationStopDecoding < 6)
                                    {
                                       if ((m < 6) && ( n <= 6))
                                       {
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1 ;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1 ;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1 ;
                                          continue;
                                       }
                                       else if (m >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                             refinement = beta_2;
                                          else
                                             refinement = beta_1;
                                          if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                          if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                          else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                             BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                       }
                                       else
                                       {
                                          if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                 }
                                    else if  (PtrCoding->DecodingStopLocations.Y_LocationStopDecoding < 6)
                                    {
                                       if( m < 6)
                                       {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else if (n >= 6)
                                       {
                                          if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;


                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                        else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                           && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                        else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                    else // last 2x2 block in upper right grandchildren block
                                    {
                                       if((m < 6) || (n < 6))
                                       {

                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                       }
                                       else
                                       {
                                           if (m < PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                        {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;

                                        }
                                          else    if ((m == PtrCoding->DecodingStopLocations.X_LocationStopDecoding)
                                              && (n <= PtrCoding->DecodingStopLocations.Y_LocationStopDecoding))
                                           {
                                             if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (short) beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (short) beta_1;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += beta_1;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= beta_1;
                                           }
                                          else
                                        {
                                              if  ((DWORD32) abs(BlockCodingInfo[i].PtrBlockAddress[m][n])  >  BitPlaneCheck) // not newly selected.
                                                 refinement = beta_2;
                                              else
                                                 refinement = beta_1;

                                              if(BlockCodingInfo[i].PtrBlockAddress[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] += (int) refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddress[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddress[m][n] -= (int) refinement;

                                             if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] > 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] += refinement;
                                             else if(BlockCodingInfo[i].PtrBlockAddressFloating[m][n] < 0)
                                                BlockCodingInfo[i].PtrBlockAddressFloating[m][n] -= refinement;
                                          }
                                       }
                                    }
                                 }
                              }
                              }
                           }
                        }
                     }
                   }
                }
            }
      }


