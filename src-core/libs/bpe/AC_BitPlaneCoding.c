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

extern void BlockScanEncode(StructCodingPara *PtrCoding,     BitPlaneBits *BlockInfo);

extern void StagesEnCoding(StructCodingPara *PtrCoding,   BitPlaneBits *BlockInfo);

extern void StagesDeCoding(StructCodingPara *PtrCoding,   BitPlaneBits *BlockInfo);

void ACDepthEncoder(StructCodingPara *PtrCoding,   BitPlaneBits *BlockInfo);

short ACDepthDecoder(StructCodingPara *PtrCoding,    BitPlaneBits *BlockInfo);

void ACGaggleEncoding(StructCodingPara *PtrCoding,
                 BitPlaneBits *BlockInfo,
                 int StartIndex,
                 int gaggles,
                 int Max_k,
                 int ID_Length)
{
   int i = 0;
   int k = 0;
   int min_k = 0;

   DWORD32 min_bits = 0xFFFF;
   DWORD32 total_bits = 0;

   /* --- Begin bug fix (Kiely) --- */
   UCHAR8 uncoded_flag = ~0;   // this is the flag indicating that uncoded option used

   // determine code choice min_k to use, either via brute-force optimum calculation, or heuristic approach
   // nkeffix : typo, it should be OptACSelect see 122x0b1c3 section 4.2.4.1.2
    if(PtrCoding->PtrHeader->Header.Part3.OptACSelect == TRUE) {  // brute-force optimum code selection
      min_k = uncoded_flag;   // uncoded option used, unless we find a better option below

      for ( k = 0; k <= Max_k; k ++)
      {
         if (StartIndex==0)
            total_bits = PtrCoding->N;   // cost of uncoded first sample
         else
            total_bits = 0;

         for (i = max(StartIndex,1); i < StartIndex + gaggles; i ++)
            total_bits += ((BlockInfo[i].MappedAC >> k ) + 1) + k;  // coded sample cost

         if((total_bits < min_bits) && (total_bits < PtrCoding->N * gaggles)) {
            min_bits = total_bits;
            min_k = k;
         }
      }
   } else {  // heuristic code option selection
      int delta = 0;
      int J = gaggles;
      int offset = 0;

      if (StartIndex==0)
        {
          J = gaggles-1;
            offset = 1;
        }

      // nkeffix : As specified in 122x0b1c3 section 4.2.4.1.2 the method for
      //           CODING QUANTIZED DC COEFFICIENTS specified in 4.3.2 is used for coding BitDepthAC_Block.
      //           In eq.20 in 4.3.2.11 Delta sum for the first gaggle should not include the first uncoded value.
      for (i = StartIndex + offset; i < StartIndex + gaggles; i ++)
         delta += BlockInfo[i].MappedAC;

      if (64*delta >= 23*J*(1<<(PtrCoding->N)))
          min_k = uncoded_flag;   // indicate that uncoded option is to be used
      else if (207 * J > 128 * delta)
         min_k = 0;
      else if ((long)(J * (1<<(PtrCoding->N + 5))) <= (long)(128 * delta + 49 * J))
         min_k = PtrCoding->N - 2;
      else {
         min_k = 0;
         while ((long)(J * (1 << (min_k + 7))) <= (long)(128 * delta + 49 * J))
            min_k++;
         min_k--;
      }
   }

   // Indicate the selected coding option
   BitsOutput(PtrCoding, min_k, ID_Length);  // code choice

   // Now output coded (or uncoded) values
   for (i = StartIndex; i < StartIndex + gaggles; i ++){
       if ((min_k == uncoded_flag) || (i==0))
         BitsOutput(PtrCoding, BlockInfo[i].MappedAC, PtrCoding->N);  // uncoded
      else
         BitsOutput(PtrCoding, 1, ((BlockInfo[i].MappedAC) >> min_k)+1);    // coded (first part)
   }
   // if we have coded samples, then we also have to send the second part
   if (min_k != uncoded_flag)
      for (i = max(StartIndex,1); i < StartIndex + gaggles; i ++)
         BitsOutput(PtrCoding, BlockInfo[i].MappedAC, min_k);
   /* --- End bug fix (Kiely) --- */

}




void DPCM_ACMapper(BitPlaneBits *BlockInfo,
               int size,
               short N)
{
   short *diff_AC;
   short theta = 0;
   long i;
   int X_Min = 0;
   int X_Max = ((1 << N) - 1) ;

   diff_AC = (short *)calloc(size,sizeof(short));
   diff_AC[0] = BlockInfo[0].BitMaxAC;
   BlockInfo[0].MappedAC = BlockInfo[0].BitMaxAC;
   for(i = 1; i < size; i ++)
      diff_AC[i] = BlockInfo[i].BitMaxAC - BlockInfo[i-1].BitMaxAC;

   for ( i = 1; i < size; i ++) {
      theta = min(BlockInfo[i-1].BitMaxAC - X_Min, X_Max - BlockInfo[i-1].BitMaxAC);
      if (diff_AC[i] >= 0 && diff_AC[i] <= theta)
         BlockInfo[i].MappedAC = 2 * diff_AC[i];
      else if(diff_AC[i] < 0 && diff_AC[i] >= -theta)
         BlockInfo[i].MappedAC = - 2 * diff_AC[i] - 1;
      else
         BlockInfo[i].MappedAC = theta + abs(diff_AC[i]);
   }
   free(diff_AC);
   return;
}



void ACDepthEncoder(StructCodingPara *PtrCoding,
               BitPlaneBits *BlockInfo)
{
   SINT GaggleStartIndex;
   UCHAR8 Max_k;
   UCHAR8 ID_Length;
   UCHAR8 gaggles ; // initial value is 15.

   PtrCoding->N = 0;
   while( PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits>>PtrCoding->N > 0)
      PtrCoding->N++;

   DPCM_ACMapper(BlockInfo, PtrCoding->PtrHeader->Header.Part3.S_20Bits, PtrCoding->N);

   if ( PtrCoding->N == 2) {
      Max_k = 0;
      ID_Length = 1;
   }
   else if (PtrCoding->N <=4) {
      Max_k = 2;
      ID_Length = 2;
   }
   else if (PtrCoding->N <=5) {
      Max_k = 6;
      ID_Length = 3;
   }
   else {
      ErrorMsg(BPE_DATA_ERROR);
      // added the following two lines to eliminate a compiler warning message
      Max_k = 0;
      ID_Length = 0;
   }

   /* --- Begin bug fix (Kiely) --- */
   GaggleStartIndex = 0;
   while( GaggleStartIndex < PtrCoding->PtrHeader->Header.Part3.S_20Bits ){
      gaggles = min(GAGGLE_SIZE, PtrCoding->PtrHeader->Header.Part3.S_20Bits - GaggleStartIndex);
      ACGaggleEncoding(PtrCoding, BlockInfo, GaggleStartIndex, gaggles, Max_k, ID_Length);
      if(PtrCoding->SegmentFull == TRUE)
         return;
      GaggleStartIndex += gaggles;
   }
   /* --- End bug fix (Kiely) --- */

   return;
}



void ACBpeEncoding(StructCodingPara *PtrCoding,    BitPlaneBits *BlockInfo)
{
   DWORD32 i = 0;
   UCHAR8 BitPlane = 0;

////////////////////////////////////////////////////////////////////////////////////////
   // The link nodes PlaneSym are used to coding of the bit planes. Contains a
   // Link data structure. It encodes more than 16 blocks at this time.
   // Need to modify it to encode up to 16 blocks only. May 20 2004
   if (PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits != 0)
      // bit plane encoder is not necessary if BitDepthAC = 0.
   {
      if (PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits == 1)
         //only the lowest bit plane has valud AC component manitude.
         for ( i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i++)
            BitsOutput(PtrCoding, BlockInfo[i].BitMaxAC, 1);
      else
         // returned is the mapped ac value.
         ACDepthEncoder(PtrCoding, BlockInfo);

         // 3.2 Family tree scaning sequence in a segment.

      for ( BitPlane = PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits ;
            BitPlane > 0; BitPlane --)
      {
         PtrCoding->BitPlane = BitPlane;
         if((PtrCoding->PtrHeader->Header.Part2.BitPlaneStop_5Bits == BitPlane)
            // nkeffix : No need to check again BitPlaneStop_5Bits is either the default value 0 or
            //           or the specified value in the header
            //&&(PtrCoding->PtrHeader->Header.Part1.Part2Flag == TRUE))
            )
            return;

/////////////////////////////////////////////////////////////////////////
//////////////////////////  Stage 0   ///////////////////////////////////
// encode the DC component (single bit only)
         // if ( B && ( A || (C && D) ) ){}
         if ( (BitPlane <= PtrCoding->QuantizationFactorQ) &&
             ((PtrCoding->PtrHeader->Header.Part4.DWTType != INTEGER_WAVELET) ||
              ((PtrCoding->QuantizationFactorQ >
               PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits ) &&
               (PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits < BitPlane)) )
            )
         {
            // output the DC bits that have been shifted out
            for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i++)
               BitsOutput(PtrCoding, ((BlockInfo[i].DCRemainder >> (BitPlane - 1)) & 0x01), 1);
         }

/////////////////////////////////////////////////////////////////////////

         if(PtrCoding->SegmentFull == TRUE)
            return;

         BlockScanEncode(PtrCoding, BlockInfo);

         if(PtrCoding->SegmentFull == TRUE)
            return;

         // ***************************  4. Stages coding *********************************//
         StagesEnCoding(PtrCoding, BlockInfo);

         if(PtrCoding->SegmentFull == TRUE)
            return;
      }
   }
}



void CheckUsefill(StructCodingPara * PtrCoding)
{

   if(PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits != 0
      && PtrCoding->PtrHeader->Header.Part2.UseFill == TRUE)
   {
      if((PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits << 3) <
         PtrCoding->Bits->SegBitCounter)
      {
         int RemainderBits = 0;
         DWORD32 TempWord = 0;
         RemainderBits = PtrCoding->Bits->SegBitCounter
            - (PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits <<3);

         while(RemainderBits > 0)
         {
            BitsRead(PtrCoding, &TempWord, 1);
            if(PtrCoding->SegmentFull == TRUE)
               return;
         }
      }
   }
   return;

}


void ACGaggleDecoding(StructCodingPara *PtrCoding,
                     BitPlaneBits *BlockInfo,
                     int StartIndex,
                     int gaggles,
                     int Max_k,
                     short ID_Length)
{
   long int i;
   UCHAR8 min_k;
   DWORD32 TempWord =0;
   int counter = 0;
   UCHAR8 ones = ~0;
   BOOL uncoded = FALSE;

   // makes the least ID_Length bits to ones.
   ones = ((ones << (8 - ID_Length)) >> ((8 - ID_Length)));

   BitsRead(PtrCoding, &TempWord, ID_Length);

   min_k = (UCHAR8) TempWord;

   if((ID_Length == 1 && min_k == 1) || (ID_Length == 2 && min_k == 3) ||
      (ID_Length == 3 && min_k == 7) || (ID_Length == 4 && min_k == 15))
      uncoded = TRUE;

   /* --- Begin bug fix (Kiely) --- */
   for( i = StartIndex; i < StartIndex + gaggles; i ++){
       if ((uncoded == TRUE)||(i==0)){  // if first value in segment, then it's an uncoded reference
         BitsRead(PtrCoding, &TempWord, PtrCoding->N);
         BlockInfo[i].MappedAC = (WORD16) TempWord;
         if(PtrCoding->RateReached == TRUE)
            return;
      } else {
          // read first part
         counter = 0;
         // rice decoding
         BitsRead(PtrCoding, &TempWord, 1);
         while ((TempWord == 0)&&(PtrCoding->RateReached != TRUE)){
             counter++;
            BitsRead(PtrCoding, &TempWord, 1);
         }
         if (PtrCoding->RateReached == TRUE)
            break;
         BlockInfo[i].MappedAC = counter;
         BlockInfo[i].MappedAC <<= min_k;
      }
   }
   if(PtrCoding->RateReached == TRUE)
      return;
   // If we have coded samples, and we haven't reached the limit, then decode each second part
   if ((uncoded == FALSE) && (PtrCoding->RateReached != TRUE)){
      // i==0 indicates a reference sample, which was already decoded above
      for( i = max(StartIndex,1); i < StartIndex + gaggles; i ++){
         BitsRead(PtrCoding, &TempWord, min_k);
         BlockInfo[i].MappedAC += (WORD16)TempWord;
         if (PtrCoding->RateReached == TRUE)
            break;
      }
   }
   /* --- End bug fix (Kiely) --- */

   return;
}





void DPCM_ACDeMapper(BitPlaneBits *BlockCodingInfo, //UCHAR8 *ACmax_blocks,
                int size,
                short N)
{
   short * diff_AC;
   short theta = 0;
   long i;
   int X_Min = 0;
   int X_Max = ((1 << N) - 1) ;

   diff_AC = (short *)calloc(size,sizeof(short));

   BlockCodingInfo[0].BitMaxAC = (UCHAR8)BlockCodingInfo[0].MappedAC;

   for ( i = 1; i < size; i ++)
   {
      theta = min(BlockCodingInfo[i-1].BitMaxAC  - X_Min, X_Max - BlockCodingInfo[i-1].BitMaxAC );
      if((float)BlockCodingInfo[i].MappedAC / 2 == BlockCodingInfo[i].MappedAC / 2)
      {
         diff_AC[i] = (short)(BlockCodingInfo[i].MappedAC / 2);
         if(diff_AC[i] >= 0 && diff_AC[i] <= theta)
         {
            BlockCodingInfo[i].BitMaxAC = diff_AC[i] + BlockCodingInfo[i - 1].BitMaxAC;
            continue;
         }
      }
      else
      {
         diff_AC[i] = - (short)((BlockCodingInfo[i].MappedAC + 1) / 2) ;
         if(diff_AC[i] <= 0 && diff_AC[i] >= -theta)
         {
            BlockCodingInfo[i].BitMaxAC = diff_AC[i] + BlockCodingInfo[i - 1].BitMaxAC;
            continue;
         }
      }
      diff_AC[i] = (short)(BlockCodingInfo[i].MappedAC  - theta);

      BlockCodingInfo[i].BitMaxAC = diff_AC[i] +  BlockCodingInfo[i - 1].BitMaxAC;

      if(    BlockCodingInfo[i].BitMaxAC < X_Min ||  BlockCodingInfo[i].BitMaxAC > X_Max)
      {
         diff_AC[i] = -diff_AC[i];
          BlockCodingInfo[i].BitMaxAC = diff_AC[i] +  BlockCodingInfo[i - 1].BitMaxAC;
      }
   }
   free(diff_AC);
   return;
}



short ACDepthDecoder(StructCodingPara *PtrCoding,
                BitPlaneBits *BlockInfo)
{
   SINT Max_k = 0;
   UINT32 GaggleStartIndex = 0;
   UCHAR8 ID_Length = 0;
   WORD16 gaggles = 0;

   PtrCoding->N = 0;
   while( PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits >> PtrCoding->N > 0)
      PtrCoding->N++;

   if ( PtrCoding->N == 2) {
      Max_k = 0;
      ID_Length = 1;
   }
   else if (PtrCoding->N <=4) {
      Max_k = 2;
      ID_Length = 2;
   }
   else if (PtrCoding->N <=5) {
      Max_k = 6;
      ID_Length = 3;
   }
   else
      ErrorMsg(BPE_DATA_ERROR);


   /* --- Begin bug fix (Kiely) --- */
   while( GaggleStartIndex < PtrCoding->PtrHeader->Header.Part3.S_20Bits ){
      gaggles = min(GAGGLE_SIZE, PtrCoding->PtrHeader->Header.Part3.S_20Bits - GaggleStartIndex);
      ACGaggleDecoding(PtrCoding, BlockInfo, GaggleStartIndex, gaggles, Max_k, ID_Length);
      GaggleStartIndex += gaggles;
   }
   /* --- End bug fix (Kiely) --- */

   DPCM_ACDeMapper(BlockInfo, PtrCoding->PtrHeader->Header.Part3.S_20Bits, PtrCoding->N);

   return BPE_OK;
}


void  ACBpeDecoding(StructCodingPara *PtrCoding,
                BitPlaneBits *BlockCodingInfo)
{
   UINT32 i = 0;
   DWORD32 TempWord = 0;
   UCHAR8 BitPlane = 0;

   ///////////  3. Organize the AC component/////////////////////////////
   // 3.1 Specify the number of bit planes in each block.
   if(PtrCoding->RateReached == TRUE)
      return;
   if (PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits != 0) // bit plane encoder is not necessary if PtrHeader->Header.Part1.BitDepthAC_5Bits = 0.
   {
      if (PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits == 1)
      { //only the lowest bit plane has valud AC component manitude.
         DWORD32 TempWord;
         for ( i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i++)
         {
            BitsRead(PtrCoding, &TempWord, 1);
            BlockCodingInfo[i].BitMaxAC = (UCHAR8)TempWord;
         }
      }
      else
         ACDepthDecoder(PtrCoding, BlockCodingInfo);
   }
   if(PtrCoding->SegmentFull == TRUE)
      return ;

   // 3.2 Family tree scaning sequence in a segment.

   for ( BitPlane = PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits; BitPlane > 0; BitPlane --)
   {
      // nkeffix : No need to check again BitPlaneStop_5Bits is either the default value 0 or
      //           or the specified value in the header
      if(PtrCoding->PtrHeader->Header.Part2.BitPlaneStop_5Bits == BitPlane)
         return;

      if(PtrCoding->SegmentFull == TRUE ||  PtrCoding->RateReached == TRUE)
         return;

      PtrCoding->BitPlane = BitPlane;

      // if ( B && ( A || (C && D) ) ){}
      if ( (BitPlane <= PtrCoding->QuantizationFactorQ) &&
           ((PtrCoding->PtrHeader->Header.Part4.DWTType != INTEGER_WAVELET) ||
           ((PtrCoding->QuantizationFactorQ >
            PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits ) &&
            (PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits < BitPlane)) )
         )
      {
         // read DC bits that have been shifted out
         for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i++)
         {
            if(PtrCoding->SegmentFull == TRUE)
               break;
            BitsRead(PtrCoding, &TempWord, 1);
            BlockCodingInfo[i].DecodingDCRemainder += (WORD16)(TempWord << (BitPlane - 1)) ;
         }
      }

      if(PtrCoding->SegmentFull != TRUE)
         StagesDeCoding(PtrCoding, BlockCodingInfo);
   }
   CheckUsefill(PtrCoding);
   return;
}

