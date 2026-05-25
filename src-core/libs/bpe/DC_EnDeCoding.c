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
#include <math.h>
#include <stdlib.h>
#include "global.h"


long DeConvTwosComp(DWORD32 complement,
               short leftmost);

extern void HeaderOutput(StructCodingPara *PtrCoding);

long DeConvTwosComp(DWORD32 complement,
               short leftmost)
{
/*   To determine the integer value of a 2's complement code
Case #1            If the leftmost bit is 0:
      1.            Write down the integer value of the pure binary code
Case #2            If the leftmost bit is 1:
      1.            Complement the bits.
     2.            Add 1.
     3.            Determine the integer value of the pure binary code from step 2.
     4.            Negate
     E.g.            32-bit 2's complement code 1111 1111 1111 1111 1111 1111 1111 1011
      1.            0000 0000 0000 0000 0000 0000 0000 0100            Complement the bits.
      2.            0000 0000 0000 0000 0000 0000 0000 0101     Add 1.
        3.            5                      Determine the integer value of the pure binary code from step 2.
      4.            -5                                                                     Negate*/

   DWORD32 temp = 0;
   long Original;
   short  i = 0;
   if((leftmost >= sizeof(DWORD32) * 8) || (leftmost == 0))
      ErrorMsg(BPE_DATA_ERROR);
   if (((1 << (leftmost-1)) & complement) == 0)
      return (long)complement;
   else
   {
      temp = 0;
      for( i = 0; i < leftmost; i ++)
      {
         temp <<= 1;
         temp ++;
      }
      Original = -(long)(((~complement) & temp) + 1);
      return Original;
   }
}





DWORD32 ConvTwosComp(long Original,
               short leftmost);


DWORD32 ConvTwosComp(long Original,
               short leftmost)
{
/*      To determine the 2's complement code for a number.
      Case #1            If the number is >= 0:
         1.            Write down the number's pure binary code
      Case #2            If the number is < 0:
          1.            Write down the pure binary code for the integer without its sign
          2.            Complement the bits
         3.            Add 1

      E.g.            32-bit 2's complement of -5
         1.            0000 0000 0000 0000 0000 0000 0000 0101  32-bit pure binary of 5
         2.            1111 1111 1111 1111 1111 1111 1111 1010      complement
         3.            1111 1111 1111 1111 1111 1111 1111 1011  add 1*/

   DWORD32 temp;
   DWORD32 complement;
   short i = 0;

   if(leftmost == 1)
    return 0;

   if((leftmost >= sizeof(DWORD32) * 8) || (leftmost == 0))
   {
      ErrorMsg(BPE_DATA_ERROR);
   }

   if (Original >= 0)
      return (DWORD32) Original;
   else
   {
      complement = ~(DWORD32) (-Original);
      temp = 0;
      for ( i = 0; i < leftmost; i ++)
      {
         temp <<= 1;
         temp ++;
      }
      complement &= temp;
      complement ++;
      return complement;
   }
}



void DCEncoder(StructCodingPara *PtrCoding,
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
   if(PtrCoding->PtrHeader->Header.Part3.OptDCSelect == TRUE) {  // brute-force optimum code selection
      min_k = uncoded_flag;   // uncoded option used, unless we find a better option below

      for ( k = 0; k <= Max_k; k ++)
      {
         if (StartIndex==0)
            total_bits = PtrCoding->N;   // cost of uncoded first sample
         else
            total_bits = 0;

         for (i = max(StartIndex,1); i < StartIndex + gaggles; i ++)
            total_bits += ((BlockInfo[i].MappedDC >> k ) + 1) + k;  // coded sample cost

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

        // nkeffix : As specified in 122x0b1c3 section4.3.2.11
        //           in eq.20 in 4.3.2.11 Delta sum for the first gaggle should not include the first uncoded value
      for (i = StartIndex + offset; i < StartIndex + gaggles; i ++) // First Gaggle contains 15 Samples Check ME
         delta += BlockInfo[i].MappedDC;

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
   BitsOutput(PtrCoding, min_k, ID_Length);

   // Now output coded (or uncoded) values
   for (i = StartIndex; i < StartIndex + gaggles; i ++){
       if ((min_k == uncoded_flag) || (i==0))
         BitsOutput(PtrCoding, BlockInfo[i].MappedDC, PtrCoding->N);  // uncoded
      else
         BitsOutput(PtrCoding, 1, ((BlockInfo[i].MappedDC) >> min_k)+1);    // coded (first part)
   }
   // if we have coded samples, then we also have to send the second part
   if (min_k != uncoded_flag)
      for (i = max(StartIndex,1); i < StartIndex + gaggles; i ++)
         BitsOutput(PtrCoding, BlockInfo[i].MappedDC, min_k);
   /* --- End bug fix (Kiely) --- */
}



void DCEntropyEncoder(StructCodingPara* PtrCoding,
                 BitPlaneBits *BlockInfo)
{
   int Max_k = 0;
   int ID_Length = 0;
   UINT32 GaggleStartIndex = 0;
   UINT32 gaggles = 0;

   if (PtrCoding->N  == 2) {
      Max_k = 0;
      ID_Length = 1;
   } else if (PtrCoding->N <=4) {
      Max_k = 2;
      ID_Length = 2;
   } else if (PtrCoding->N <=8) {
      Max_k = 6;
      ID_Length = 3;
   } else if (PtrCoding->N <=10) {
      Max_k = 8;
      ID_Length = 4;
   }


   /* --- Begin bug fix (Kiely) --- */
   while( GaggleStartIndex < PtrCoding->PtrHeader->Header.Part3.S_20Bits ){
      gaggles = min(GAGGLE_SIZE, PtrCoding->PtrHeader->Header.Part3.S_20Bits - GaggleStartIndex);
      DCEncoder(PtrCoding, BlockInfo, GaggleStartIndex, gaggles, Max_k, ID_Length);
      GaggleStartIndex += gaggles;
   }
   /* --- End bug fix (Kiely) --- */


   if(PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits < PtrCoding->QuantizationFactorQ)
   {
      UINT32 i;
      UINT32 k;

      /* --- Begin bug fix (Kiely) --- */
      int numaddbitplanes;

      // calculate number of additional bit planes to include in DC coefficient coding (section 4.3.3 of the recommendation)
      if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
          numaddbitplanes = PtrCoding->QuantizationFactorQ -
             max( PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits, PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits );
      else
          numaddbitplanes = PtrCoding->QuantizationFactorQ - PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits;

      // loop on each bit plane in order of decreasing significance
      for (i = 0; i < numaddbitplanes; i ++)
         for( k = 0; k < PtrCoding->PtrHeader->Header.Part3.S_20Bits; k++)
            BitsOutput(PtrCoding, BlockInfo[k].DCRemainder >>(PtrCoding->QuantizationFactorQ - i - 1), 1);
      /* --- End bug fix (Kiely) --- */
   }
   return;
}


void DCGaggleDecoding(StructCodingPara *PtrCoding,
                     BitPlaneBits *BlockInfo,
                     int StartIndex,
                     int gaggles,
                     int Max_k,
                     short ID_Length)
{
   long int i = 0;
   int counter = 0;
   UCHAR8 min_k;
   DWORD32 TempWord =0;
   BOOL uncoded = FALSE;

   // makes the least ID_Length bits to ones.
   BitsRead(PtrCoding, &TempWord, ID_Length);
   min_k = (UCHAR8) TempWord;

   if( (ID_Length == 1 && min_k == 1) || (ID_Length == 2 && min_k == 3) ||
      (ID_Length == 3 && min_k == 7) || (ID_Length == 4 && min_k == 15))
       uncoded = TRUE;

   /* --- Begin bug fix (Kiely) --- */
   for( i = StartIndex; i < StartIndex + gaggles; i ++){
       if ((uncoded == TRUE)||(i==0)){  // if first value in segment, then it's an uncoded reference
         BitsRead(PtrCoding, &TempWord, PtrCoding->N);
         BlockInfo[i].MappedDC = TempWord;
      } else {
          // read first part of coded sample
         counter = 0;
         // rice decoding
         BitsRead(PtrCoding, &TempWord, 1);
         while ((TempWord == 0)&&(PtrCoding->RateReached != TRUE)){
             counter++;
            BitsRead(PtrCoding, &TempWord, 1);
         }
         if (PtrCoding->RateReached == TRUE)
            break;
         BlockInfo[i].MappedDC = counter;
         BlockInfo[i].MappedDC <<= min_k;
      }
   }
   // If we have coded samples, and we haven't reached the limit, then decode each second part
   if ((uncoded == FALSE) && (PtrCoding->RateReached != TRUE)){
      for( i = max(StartIndex,1); i < StartIndex + gaggles; i++){
         BitsRead(PtrCoding, &TempWord, min_k);
         BlockInfo[i].MappedDC += TempWord;
         if (PtrCoding->RateReached == TRUE)
            break;
      }
   }
   /* --- End bug fix (Kiely) --- */

   return;
}



short DCEntropyDecoder(StructCodingPara *PtrCoding,
                  BitPlaneBits *BlockInfo)
{
   int Max_k = 0;
   int gaggles = 15;
   int GaggleStartIndex = 0;
   UCHAR8 ID_Length = 0;

   if ( PtrCoding->N == 2) {
      Max_k = 0;
      ID_Length = 1;
   } else if ( PtrCoding->N <=4) {
      Max_k = 2;
      ID_Length = 2;
   } else if ( PtrCoding->N <=8) {
      Max_k = 6;
      ID_Length = 3;
   } else if ( PtrCoding->N <=10) {
      Max_k = 8;
      ID_Length = 4;
   } else
      ErrorMsg(BPE_DATA_ERROR);


   /* --- Begin bug fix (Kiely) --- */
   while( GaggleStartIndex < PtrCoding->PtrHeader->Header.Part3.S_20Bits ){
      gaggles = min(GAGGLE_SIZE, PtrCoding->PtrHeader->Header.Part3.S_20Bits - GaggleStartIndex);
      DCGaggleDecoding(PtrCoding, BlockInfo, GaggleStartIndex, gaggles, Max_k, ID_Length);
      GaggleStartIndex += gaggles;
   }
   /* --- End bug fix (Kiely) --- */

   return BPE_OK;
}


void DPCM_DCMapper(BitPlaneBits *BlockInfo,
               int size,
               short N)
{
   long * diff_DC = NULL;
   long i = 0;
   long theta = 0;
   long X_Min = 0;
   DWORD32 Bits1 = 0;
   DWORD32 X_Max;
   DWORD32 Max_Mapped = 0;
   X_Min = - (1<<(N-1));
   X_Max = ((1<<(N-1)) - 1);

   diff_DC = (long *)calloc(size,sizeof(long));
   BlockInfo[0].MappedDC = BlockInfo[0].ShiftedDC; // the reference DC

   for(i = 0; i < (N - 1); i ++)
      Bits1 = (Bits1<<1)+1;

   if((BlockInfo[0].ShiftedDC & (1 << (N -1))) > 0) // minus 0
      /* Bug, I believe */
      // BlockInfo[0].ShiftedDC = -(short)(((~(BlockInfo[0].ShiftedDC - 1)) & Bits1));
      BlockInfo[0].ShiftedDC = -(short)( ((BlockInfo[0].ShiftedDC^Bits1) & Bits1) + 1 );
   else
      BlockInfo[0].ShiftedDC = BlockInfo[0].ShiftedDC;  // positive.

   diff_DC[0] = BlockInfo[0].ShiftedDC;

   for(i = 1; i < size; i ++)
   {
      if((BlockInfo[i].ShiftedDC & (1 << (N -1))) > 0) // minus 0
         /*  Bug  */
         // BlockInfo[i].ShiftedDC = -(short)((~(BlockInfo[i].ShiftedDC - 1)) & Bits1);
         BlockInfo[i].ShiftedDC = -(short)( ((BlockInfo[i].ShiftedDC^Bits1) & Bits1) + 1 );
      else
         BlockInfo[i].ShiftedDC = BlockInfo[i].ShiftedDC;  // positive.
      diff_DC[i] = BlockInfo[i].ShiftedDC - BlockInfo[i - 1].ShiftedDC ;  //DCtwos[i] - DCtwos[i - 1];
   }

   for ( i = 1; i < size; i ++)
   {
      theta = min(BlockInfo[i - 1].ShiftedDC - X_Min, X_Max - BlockInfo[i - 1].ShiftedDC);
      if (diff_DC[i] >= 0 && diff_DC[i] <= theta)
         BlockInfo[i].MappedDC = 2 * diff_DC[i];
      else if(diff_DC[i] <0 && diff_DC[i] >= -theta)
         BlockInfo[i].MappedDC = - 2 * diff_DC[i] - 1;
      else
         BlockInfo[i].MappedDC = theta + abs(diff_DC[i]);

      if(BlockInfo[i].MappedDC > Max_Mapped)
         Max_Mapped = BlockInfo[i].MappedDC;
   }
   free(diff_DC);
   return;
}

void DCEncoding(StructCodingPara *PtrCoding,
             long **BlockString,
             BitPlaneBits *BlockInfo)

{
   WORD16 BitDepthDC = 0;
   WORD16 BitDepthAC = 0;
   WORD16 QuantizationFactorQ_prime;
   UINT32 i;
   UINT32 k;
   UINT32 p;

   SLONG ACBitMaxOneBlock = 1;
   SLONG Max_DC = 0;
   SLONG MaxACSegment = 0;
   DWORD32 BlockIndex;

   SLONG DC_Min = 0x10000;
   SLONG DC_Max = -0x10000;

   DWORD32 Temp_BitMaxDC = 0;

   for(BlockIndex = PtrCoding->BlockCounter;
      BlockIndex < PtrCoding->BlockCounter + PtrCoding->PtrHeader->Header.Part3.S_20Bits;
      BlockIndex++)
   {
      UINT32 IndexStart = BlockIndex - PtrCoding->BlockCounter;

      BlockInfo[IndexStart].PtrBlockAddress = (BlockString + BlockIndex * BLOCK_SIZE) ;

      *BlockInfo[IndexStart].PtrBlockAddress = *(BlockString + BlockIndex * BLOCK_SIZE);

      /******         Begin the DC coding processing, Coding stage 0      *****/

      // Begin to see if the DC value is bigger than the previous one.
      // determine BitDepthAC and Bit2sDC;

      if ((AMPLITUDE(BlockInfo[IndexStart].PtrBlockAddress[0][0])) > AMPLITUDE(Max_DC) )
         Max_DC = BlockInfo[IndexStart].PtrBlockAddress[0][0];

      if ((DWORD32)(AMPLITUDE(BlockInfo[IndexStart].PtrBlockAddress[0][0])) > Temp_BitMaxDC )
         Temp_BitMaxDC =AMPLITUDE(BlockInfo[IndexStart].PtrBlockAddress[0][0]);

      if (BlockInfo[IndexStart].PtrBlockAddress[0][0] > DC_Max)
         DC_Max = BlockInfo[IndexStart].PtrBlockAddress[0][0];

      if (BlockInfo[IndexStart].PtrBlockAddress[0][0] < DC_Min)
         DC_Min = BlockInfo[IndexStart].PtrBlockAddress[0][0];

      BlockInfo[IndexStart].BitMaxAC = 0;
      ACBitMaxOneBlock = 0;
      // search for the max bit length of AC of each block
      for (k = 0; k < BLOCK_SIZE; k ++)
         for (p = 0; p < BLOCK_SIZE; p++)
         {
            UINT32 AbsAC = 0;
            if ( k == 0 && p ==0)
               continue;

            AbsAC = AMPLITUDE(BlockInfo[IndexStart].PtrBlockAddress[k][p]);
            if((SLONG) AbsAC > ACBitMaxOneBlock)
               ACBitMaxOneBlock = AbsAC;
            if((SLONG)AbsAC > MaxACSegment)
               MaxACSegment = AbsAC;
         }
      BlockInfo[IndexStart].BitMaxAC = 0;
      while (ACBitMaxOneBlock > 0)
      {
         ACBitMaxOneBlock >>= 1;
         BlockInfo[IndexStart].BitMaxAC ++;
      }
   }

   PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits = 0;
   PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits = 0;

   while (MaxACSegment > 0) {
      MaxACSegment >>= 1;
      PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits ++;
   }

   //Max_DC can be minus

   if (DC_Min >= 0)
      Max_DC = DC_Max;
   else if( DC_Max <= 0)
      Max_DC = DC_Min;
   else if(DC_Max >= AMPLITUDE(DC_Min))
      Max_DC = DC_Max;
   else
      Max_DC = DC_Min;

   if(Max_DC >= 0)
   {
      while (Max_DC > 0)
      {
         Max_DC >>= 1;
         PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits ++;
      }
   }
   else
   {
      DWORD32 temp = -Max_DC;
      while (temp > 0)
      {
         temp >>= 1;
         PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits ++;
      }
      if( (1 << (PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits -1)) == -Max_DC)
         PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits --;
   }

   PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits++; // include the sign bit.

   // consider one bit to indicate negative or positive.
   // because BitDepthAC and BitDepthDC will be transmitted to the decoder, the decoder can get
   // exactly the same ns, which can be used for the shifting operation of DC values.
   BitDepthDC = PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits;
   BitDepthAC = PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits;

   HeaderOutput(PtrCoding);

   if (PtrCoding->SegmentFull == TRUE)
      ErrorMsg(BPE_RATE_ERROR);

   if (BitDepthDC <= 3)
      QuantizationFactorQ_prime = 0;
   else if (((BitDepthDC - (1 + (BitDepthAC >> 1))) <= 1)&&(BitDepthDC > 3))
      QuantizationFactorQ_prime = BitDepthDC - 3;
   else if (((BitDepthDC - (1 + (BitDepthAC >> 1))) > 10)&&(BitDepthDC > 3))
      QuantizationFactorQ_prime = BitDepthDC - 10;
   else
      QuantizationFactorQ_prime = 1 + (BitDepthAC>>1);

   if(PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
      // have to consider the scaling factor
      PtrCoding->QuantizationFactorQ = max(QuantizationFactorQ_prime,
          PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits);
   else
      PtrCoding->QuantizationFactorQ = (UCHAR8) QuantizationFactorQ_prime;

   /////////////////////////  Shift the DC component //////////////////////

   // k consists of PtrCoding->QuantizationFactorQ ones in the least significant bits
   k = (1<<PtrCoding->QuantizationFactorQ)-1;

   for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i ++)// shift the DC component to the right
   {
      DWORD32 NewNum = ConvTwosComp(BlockInfo[i].PtrBlockAddress[0][0], BitDepthDC);
      BlockInfo[i].ShiftedDC = (NewNum >> PtrCoding->QuantizationFactorQ);
      BlockInfo[i].DCRemainder = (WORD16)(NewNum & k);
   }

   PtrCoding->N = max(PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits
      - PtrCoding->QuantizationFactorQ, 1);

   // the maximum value of the N is 10.

   if (PtrCoding->N == 1)
   {
      for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i ++)// shift the DC component to the right
         BitsOutput(PtrCoding,BlockInfo[i].ShiftedDC, 1);
      return;
   }
   // 2.2 Sample-spit entropy for bit shifted DC's
   DPCM_DCMapper(BlockInfo, PtrCoding->PtrHeader->Header.Part3.S_20Bits, PtrCoding->N);
      //PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits - PtrCoding->N);
   DCEntropyEncoder(PtrCoding, BlockInfo);

   //ADDITIONAL BIT PLANES OF DC COEFFICIENTS
   //The coding of quantized DC coefficients described
   //in Section 4.3.1 effectively encodes the first
   //BitDepthDC-q bits of each DC coefficient.
   //When q >BitDepthAC, the next q-BitDepthAC bits
   //of each DC coefficient appear in the coded bitstream.
   //The appropriate bits of each DC coefficient are concatenated,
   //for each bit plane. That is, we have the (q-1)th most-significant
   //bit of each DC coefficient, followed by the (q-2)th most-significant bit,
   //and so on, until the BitDepthACth bit of each DC coefficient
   //(Recall that bits are indexed so that the least significant bit is
   //referred to as the 0th most significant bit.)

}


void DPCM_DCDeMapper(BitPlaneBits *BlockInfo,
                int size,
                short N)
{

   long theta = 0;
   long i;
   DWORD32 X_Max = (1 << (N-1)) - 1;
   long X_Min = - (1<< (N-1));

   long * diff_DC;

   DWORD32 Bits1 = 0;

   diff_DC = (long *)calloc(size,sizeof(long));

   BlockInfo[0].ShiftedDC = BlockInfo[0].MappedDC;
   diff_DC[0] = BlockInfo[0].ShiftedDC;

   for(i = 0; i < (N - 1); i ++)
      Bits1 = (Bits1<<1)+1;

   if((BlockInfo[0].ShiftedDC & (1 << (N -1))) > 0) // minus 0
      /* Bug, I believe */
      // BlockInfo[0].ShiftedDC = -(short)(((~(BlockInfo[0].ShiftedDC - 1)) & Bits1));
      BlockInfo[0].ShiftedDC = -(short)( ((BlockInfo[0].ShiftedDC^Bits1) & Bits1) + 1 );
   else
      BlockInfo[0].ShiftedDC = BlockInfo[0].ShiftedDC;  // positive.

   for ( i = 1; i < size; i ++)
   {
      theta = min(BlockInfo[i-1].ShiftedDC - X_Min, X_Max - BlockInfo[i-1].ShiftedDC);

      /* --- Begin bug fix (Kiely) --- */
      if (BlockInfo[i].MappedDC > 2*theta) {
         if ((long int)BlockInfo[i - 1].ShiftedDC < 0)
            diff_DC[i] = BlockInfo[i].MappedDC - theta;
         else
            diff_DC[i] = theta - BlockInfo[i].MappedDC;
      } else {
         if((float)BlockInfo[i].MappedDC / 2 == BlockInfo[i].MappedDC / 2)
            diff_DC[i] = BlockInfo[i].MappedDC / 2;
         else
            diff_DC[i] = -(long)(BlockInfo[i].MappedDC + 1) / 2 ;
      }
      BlockInfo[i].ShiftedDC = diff_DC[i] + BlockInfo[i - 1].ShiftedDC;
      /* --- End bug fix (Kiely) --- */

   }
   free(diff_DC);
   return;
}


short DCDeCoding(StructCodingPara *PtrCoding,  StructFreBlockString *StrBlocks,  BitPlaneBits *BlockInfo)
{
   UINT32 i;
   int k;
   short BitDepthDC;
   short BitDepthAC;
   WORD16 QuantizationFactorQ_prime;

   BitDepthDC = PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits;
   BitDepthAC = PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits;

   for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i++)
   {
      BlockInfo[i].PtrBlockAddress = (StrBlocks->FreqBlkString  + i * BLOCK_SIZE) ;
      *BlockInfo[i].PtrBlockAddress = *(StrBlocks->FreqBlkString  + i * BLOCK_SIZE);
      BlockInfo[i].PtrBlockAddressFloating= (StrBlocks->FloatingFreqBlk  + i * BLOCK_SIZE) ;
      *BlockInfo[i].PtrBlockAddressFloating = *(StrBlocks->FloatingFreqBlk  + i * BLOCK_SIZE);
      // clear DecodingDCRemainder (perhaps this has been done elsewhere, I'm not sure)
      BlockInfo[i].DecodingDCRemainder = 0;   // Kiely
   }

   if (BitDepthDC <= 3)
      QuantizationFactorQ_prime = 0;
   else if (((BitDepthDC - (1 + (BitDepthAC >> 1))) <= 1)&&(BitDepthDC > 3))
      QuantizationFactorQ_prime = BitDepthDC - 3;
   else if (((BitDepthDC - (1 + (BitDepthAC >> 1))) > 10)&&(BitDepthDC > 3))
      QuantizationFactorQ_prime = BitDepthDC - 10;
   else
      QuantizationFactorQ_prime = (1 + (BitDepthAC>>1));

   if(PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
      // have to consider the scaling factor
      PtrCoding->QuantizationFactorQ = max(QuantizationFactorQ_prime,
          PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits);
   else
      PtrCoding->QuantizationFactorQ = (UCHAR8) QuantizationFactorQ_prime;
   // 2.2 Sample-spit entropy for bit shifted DC's
   PtrCoding->N = max(PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits    - PtrCoding->QuantizationFactorQ, 1);

   if (PtrCoding->N ==1)
      for(i = 0; i < PtrCoding->PtrHeader->Header.Part3.S_20Bits; i ++) // shift the DC component to the right
         BitsRead(PtrCoding, &BlockInfo[i].ShiftedDC, 1);
   else
   {
      if ((k = DCEntropyDecoder(PtrCoding, BlockInfo)) != BPE_OK)
         return k;
      DPCM_DCDeMapper(BlockInfo, PtrCoding->PtrHeader->Header.Part3.S_20Bits, PtrCoding->N);
   }

   /* --- Begin bug fix (Kiely) --- */
   // Need to decode second state (section 4.3.3) DC bits here, if any
   //  (they form the beginning of DecodingDCRemainder)
   if(PtrCoding->QuantizationFactorQ > PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits)
   {
      int numaddbitplanes;
      DWORD32 TempWord;

      // calculate number of additional bit planes to include in DC coefficient coding (section 4.3.3 of the recommendation)
      if (PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
          numaddbitplanes = PtrCoding->QuantizationFactorQ -
             max( PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits, PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits );
      else
          numaddbitplanes = PtrCoding->QuantizationFactorQ - PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits;

      for (i = 0; i < numaddbitplanes; i ++)
         for( k = 0; k < PtrCoding->PtrHeader->Header.Part3.S_20Bits; k++){
            BitsRead(PtrCoding, &TempWord, 1);
            BlockInfo[k].DecodingDCRemainder += TempWord << (PtrCoding->QuantizationFactorQ - i - 1) ;
         }
   }
   /* --- End bug fix (Kiely) --- */

   PtrCoding->N = PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits - PtrCoding->QuantizationFactorQ;

   for(i = 0; i <(UINT32) PtrCoding->PtrHeader->Header.Part3.S_20Bits; i ++) {
      // shift the DC component to the right
      BlockInfo[i].ShiftedDC <<= PtrCoding->QuantizationFactorQ;
      *(*BlockInfo[i].PtrBlockAddress) = DeConvTwosComp(BlockInfo[i].ShiftedDC, PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits);
   }

   return BPE_OK;
}
