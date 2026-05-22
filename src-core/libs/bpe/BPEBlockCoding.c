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

void BlockScanEncode(StructCodingPara *PtrCoding,
                BitPlaneBits *BlockInfo)
{
   int i = 0;
   int j = 0;
   int k = 0;
   int p = 0;
   short temp_x = 0;
   short temp_y = 0;
   short counter = 0;
   UCHAR8 BitPlane = PtrCoding->BitPlane;
   UCHAR8 si = 0;
   WORD16 temp;
   DWORD32 BlockSeq;
   DWORD32 Bit_Set_Plane = (1 << (BitPlane - 1)) ;
   long ** block = NULL;

   for (BlockSeq = 0;   BlockSeq < PtrCoding->PtrHeader->Header.Part3.S_20Bits; BlockSeq++)
   {
      si = 0;
      if (BlockInfo[BlockSeq].BitMaxAC < BitPlane)
         continue;

      PtrCoding->block_index = BlockSeq;
      block = BlockInfo[BlockSeq].PtrBlockAddress;

   // check the parents.

      for (i = 0 ; i < 3; i++)
      {
         if(PtrCoding->PtrHeader->Header.Part4.DWTType == INTEGER_WAVELET)
         {
            if(((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL3_2bits >= BitPlane)) // HL3 band.
               || ((i == 1) && (PtrCoding->PtrHeader->Header.Part4.CustomWtLH3_2bits >= BitPlane)) // HL3 band.
               || ((i == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH3_2bits >= BitPlane))) // HL3 band.
               continue;
         }

         temp_x = (i >= 1 ? 1 : 0);
         temp_y = (i != 1 ? 1 : 0);
         if ((BlockInfo[BlockSeq].StrPlaneHitHistory.TypeP & (1 << (2 - i))) == 0)
         {
            BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TYPE_P;
            BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
            BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;

            if((AMPLITUDE(block[temp_x][temp_y]) >= (1 << (BitPlane - 1))) &&
               (AMPLITUDE(block[temp_x][temp_y]) < (1 << BitPlane)))
            {
               BlockInfo[BlockSeq].StrPlaneHitHistory.TypeP += (1 << (2 - i));
               BlockInfo[BlockSeq].SymbolsBlock[si].sym_val += 1;
               BlockInfo[BlockSeq].SymbolsBlock[si].sign <<= 1;
               BlockInfo[BlockSeq].SymbolsBlock[si].sign += SIGN(block[temp_x][temp_y]);
            }
         }
         else
         {
            temp = ( (AMPLITUDE(block[temp_x][temp_y])) & (Bit_Set_Plane)) > 0 ? 1 : 0 ;
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
            {
               if (((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL3_2bits < BitPlane))
                  || ((i == 1) && (PtrCoding->PtrHeader->Header.Part4.CustomWtLH3_2bits < BitPlane))
                  || ((i == 2) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHH3_2bits < BitPlane)))
               {
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentRefSymbol <<= 1;
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentRefSymbol += temp;
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentSymbolLength ++;
               }
            }
            else
            {
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentRefSymbol <<= 1;
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentRefSymbol += temp;
                  BlockInfo[BlockSeq].RefineBits.RefineParent.ParentSymbolLength ++;
            }
         }
      }

      if (BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
         si ++;
   // now determine the TranB, look into the descendent, i.e., TypeCi.

      if (BlockInfo[BlockSeq].StrPlaneHitHistory.TranB == 0)
      {
         for ( k = 0; k < 3; k ++)
         {
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
            {
               if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits >= BitPlane) &&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
                  || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits >= BitPlane)&&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
                  || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits >= BitPlane)&&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
                  continue;
            }
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
            {
               if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits >= BitPlane)) // HL3 band.
                  || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits >= BitPlane)) // HL3 band.
                  || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits >= BitPlane))) // HL3 band.
                  goto CHECK_GC_B;
            }
            temp_x = (k >= 1 ? 1 : 0);
            temp_x *= 2;
            temp_y = (k != 1 ? 1 : 0);
            temp_y *= 2;

            for ( i = temp_x ; i < temp_x + 2; i ++)
               for ( j =temp_y; j < temp_y + 2; j ++)
               {
                  // check the BitPlane to see if the bit plane is one.
                  if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                  {
                     BlockInfo[BlockSeq].StrPlaneHitHistory.TranB = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_len = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_val = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_B;
                     si++;
                     goto DS_Update;
                  }
               }
CHECK_GC_B:;
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
               {
                  if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
                     || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
                     || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
                     continue;
               }
            temp_x = (k >= 1 ? 1 : 0);
            temp_x *= 4;
            temp_y = (k != 1 ? 1 : 0);
             temp_y *= 4;
            for ( i = temp_x; i < temp_x + 4; i ++)
               for ( j = temp_y; j < temp_y + 4; j++)
                  if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                  {
                     BlockInfo[BlockSeq].StrPlaneHitHistory.TranB = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_len = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_val = 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_B;
                     si++;
                     goto DS_Update;
                  }
         }
   DS_Update:;
      }


      if (BlockInfo[BlockSeq].StrPlaneHitHistory.TranB == 0)
      {
         BlockInfo[BlockSeq].SymbolsBlock[si].sym_len = 1;
         BlockInfo[BlockSeq].SymbolsBlock[si].sym_val = 0;
         BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_B;
         continue;
      }
      // TranB bits end

      if (BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
         si ++;

      if (BlockInfo[BlockSeq].StrPlaneHitHistory.TranB == 1) // continue to scan
      {
         // determine if to scan the descendent.

         for ( k = 0; k < 3; k ++)
         {
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
            {
               if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits >= BitPlane)&&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
                  || ((k == 1) &&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits >= BitPlane)&&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
                  || ((k == 2) &&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits >= BitPlane)&&
                  (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
                  continue;
            }

            if ((BlockInfo[BlockSeq].StrPlaneHitHistory.TranD & ( 1 << ( 2 - k))) == 0)
            {
                if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
               {
                  if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits >= BitPlane)) // HL3 band.
                     || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits >= BitPlane)) // HL3 band.
                     || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits >= BitPlane))) // HL3 band.
                     goto CHECK_GC;
               }

               temp_x = (k >= 1 ? 1 : 0);
               temp_x *= 2;
               temp_y = (k != 1 ? 1 : 0);
               temp_y *= 2;

               for ( i = temp_x ; i < temp_x + 2; i ++)
                  for ( j = temp_y; j < temp_y + 2; j ++)
                  {
                     // check the BitPlane to see if the bit plane is one.
                     if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                     {
                        BlockInfo[BlockSeq].StrPlaneHitHistory.TranD += (1 << ( 2 - k));
                        BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_D;
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                        goto NEW_B;
                     }
                  }

   CHECK_GC:;
                  if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
                  {
                     if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
                        || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
                        || ((k == 2) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
                        continue;
                  }

                  temp_x = (k >= 1 ? 1 : 0);
                  temp_x *= 4;
                  temp_y = (k != 1 ? 1 : 0);
                  temp_y *= 4;

                  for (i = temp_x; i < temp_x + 4; i ++)
                     for ( j = temp_y; j < temp_y + 4; j++)
                     {
                        if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                        {
                           BlockInfo[BlockSeq].StrPlaneHitHistory.TranD += (1 << ( 2 - k));
                           BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_D;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                           goto NEW_B;
                        }
                     }
                  // otherwise the transition bit D is 0.
                  if((BlockInfo[BlockSeq].StrPlaneHitHistory.TranD & (1 << ( 2 - k))) == 0)
                  {
                     BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_D;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                  }
            }
NEW_B:;
         }
      }

      if(BlockInfo[BlockSeq].StrPlaneHitHistory.TranD == 0)
         ErrorMsg(BPE_BLOCKSCAN_CODING_ERROR);//" Error encoding of TranD! Cannot be all Zero!\n");

   // Now we determine if we need TypeCi.
      for ( k = 0; k < 3; k ++)
      {
         // TypeCi will be always scanned unless all its mementrs have been hit.
         if ((BlockInfo[BlockSeq].StrPlaneHitHistory.TranD & (1 << (2 - k))) != 0)
         {
            if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
            {
               if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits >= BitPlane)) // HL3 band.
                  || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits >= BitPlane)) // HL3 band.
                  || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits >= BitPlane))) // HL3 band.
                  continue;
            }

            temp_x = (k >= 1 ? 1 : 0);
            temp_x *= 2;
            temp_y = (k != 1 ? 1 : 0);
            temp_y *= 2;

            p = 1;
            for ( i = 0 ; i < 4; i ++)
               if ((BlockInfo[BlockSeq].StrPlaneHitHistory.TypeCi[k].TypeC << (1 << (3 - i))) != 1)
               {
                  p = 0;
                  break;
               }

            if (p == 0) // not all TypeCi hitted so far. scan TypeCi
            {
               counter = 0;
               if (BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
                  si++;
               BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TYPE_CI;
               for ( i = temp_x ; i < temp_x + 2; i ++)
                  for ( j = temp_y; j < temp_y + 2; j ++)
                  {
                     // check the BitPlane to see if the bit plane is one.
                     if((BlockInfo[BlockSeq].StrPlaneHitHistory.TypeCi[k].TypeC & ( 1 << (3 - counter))) == 0)
                     {
                        if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                        {
                           BlockInfo[BlockSeq].StrPlaneHitHistory.TypeCi[k].TypeC += (1 << ( 3 - counter));
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sign <<= 1;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sign += SIGN(block[i][j]);
                        }
                        else
                        {
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                        }
                     }
                     else
                     {
                        temp = ((AMPLITUDE(block[i][j]) & Bit_Set_Plane) > 0 ? 1 : 0);

                        if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
                        {
                           if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits < BitPlane)) // HL3 band.
                              || ((k == 1) && (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits < BitPlane )) // HL3 band.
                              || ((k == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits < BitPlane))) // HL3 band.
                           {

                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenSymbolLength ++;

                           }
                        }
                        else
                        {
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenSymbolLength ++;
                        }
                     }
                     counter ++;
                  }
            }
            else
            {   // refinement bits
               for ( i = temp_x ; i < temp_x + 2; i ++)
                  for ( j = temp_y; j < temp_y + 2; j ++)
                  {
                        temp = ((AMPLITUDE(block[i][j]) & Bit_Set_Plane) > 0 ? 1 : 0);
                     if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
                     {
                        if(((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits < BitPlane )) // HL3 band.
                           || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits < BitPlane )) // HL3 band.
                           || ((k == 2) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits < BitPlane )))// HL3 band.
                        {
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenSymbolLength ++;
                        }
                     }
                     else
                     {
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineChildren.ChildrenSymbolLength ++;

                     }


                  }
            }
         }
      }
   // determine if we need TranGi, which determine if
   // we need to scan the grand children.

   // TranGi is needed whenver TranD is 1 unless it has been set to in the
   // previous bit planes.
      if(BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
         si ++;
      for(k = 0; k < 3; k++)
      {

         if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
         {
            if((((k == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
               || ((k == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
               || ((k == 2) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane)))) // HL3 band.
               continue;
         }

         if (((BlockInfo[BlockSeq].StrPlaneHitHistory.TranD & (1 << (2 - k)))
            != 0) && ((BlockInfo[BlockSeq].StrPlaneHitHistory.TranGi & (1 << (2 - k)))  == 0))
         {
            // TranGi is needed.
            BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_GI;
            temp_x = (k >= 1 ? 1 : 0);
            temp_x *= 4;
            temp_y = (k != 1 ? 1 : 0);
             temp_y *= 4;

            for ( i= temp_x; i < temp_x + 4; i ++)
               for ( j = temp_y; j < temp_y + 4; j++)
               {
                  if(   (Bit_Set_Plane & AMPLITUDE(block[i][j])) > 0)
                  {
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                     BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                     BlockInfo[BlockSeq].StrPlaneHitHistory.TranGi += ( 1 << (2 - k));
                     goto NEW_Gi;
                  }
               }
            BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
            BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
         }
   NEW_Gi:;
      }

      if(BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
         si ++;

   // now based TranHi.
      for (i = 0; i < 3; i ++)
      {
         if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
         {
            if(((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
               || ((i == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
               || ((i == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
               continue;
         }

         if(BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
            si ++;

         for ( j = 0; j < 4; j ++)
         {
            temp_x = (i >= 1 ? 1 : 0) * 4 + (j >= 2 ? 1 : 0) * 2;
            temp_y = (i != 1 ? 1 : 0) * 4 + (j % 2) * 2;

            if (((BlockInfo[BlockSeq].StrPlaneHitHistory.TranGi & (1 << ( 2 - i))) != 0)
               && ((BlockInfo[BlockSeq].StrPlaneHitHistory.TranHi[i].TranH & (1 << ( 3 - j))) == 0))
            {
               BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TRAN_HI;
               for ( k = temp_x; k < temp_x + 2; k ++)
                  for ( p = temp_y; p < temp_y + 2; p ++)
                  {
                     if(   (Bit_Set_Plane & AMPLITUDE(block[k][p])) > 0)
                     {
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                        BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                        BlockInfo[BlockSeq].StrPlaneHitHistory.TranHi[i].TranH += (1 << ( 3 - j));
                        goto NEW_Gij;
                     }
                  }
               BlockInfo[BlockSeq].SymbolsBlock[si].sym_len++;
               BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
            }
NEW_Gij:;
         }
      }

      for (i = 0; i < 3; i ++)
      {
         if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
         {
            if(((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits >= BitPlane)) // HL3 band.
               || ((i == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits >= BitPlane)) // HL3 band.
               || ((i == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits >= BitPlane))) // HL3 band.
               continue;
         }

         for ( j = 0; j < 4; j ++)
         {
            temp_x = (i >= 1 ? 1 : 0) * 4 + (j >= 2 ? 1 : 0) * 2;
            temp_y = (i != 1 ? 1 : 0) * 4 + (j % 2) * 2;

            if((BlockInfo[BlockSeq].StrPlaneHitHistory.TranHi[i].TranH & (1 << (3 - j))) != 0)
            {
               short t = 0;
               counter = 0;
               for ( k = 0; k < 4; k ++)
                  if ((BlockInfo[BlockSeq].StrPlaneHitHistory.TypeHij[i].TypeHij[j].TranH & ( 1 << (3 - k))) == 0)
                     counter ++;

               if (counter == 0) // refinement bits. all bits have been hit before.
               {
                  for ( k = temp_x; k < temp_x + 2; k ++)
                     for ( p = temp_y; p < temp_y + 2; p ++)
                     {
                        temp = ((AMPLITUDE(block[k][p]) & Bit_Set_Plane) > 0 ? 1 : 0);

                        if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
                        {
                           if(((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits < BitPlane )) // HL3 band.
                              || ((i == 1) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits < BitPlane )) // HL3 band.
                              || ((i == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits < BitPlane ))) // HL3 band.
                           {
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenSymbolLength ++;
                           }
                        }
                        else
                        {
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenSymbolLength ++;
                        }
                     }
                     continue;
               }

            // if TranHi == 1, then four grand children TypeHij will be scanned/


               if(BlockInfo[BlockSeq].SymbolsBlock[si].sym_len != 0)
                  si ++;

               BlockInfo[BlockSeq].SymbolsBlock[si].type = ENUM_TYPE_HIJ;

               t = 0;
               for ( k = temp_x; k < temp_x + 2; k ++)
                  for ( p = temp_y; p < temp_y + 2; p ++)
                  {
                     if((BlockInfo[BlockSeq].StrPlaneHitHistory.TypeHij[i].TypeHij[j].TranH & (1 << (3 - t))) == 0)
                     {
                        if((Bit_Set_Plane & AMPLITUDE(block[k][p])) > 0)
                        {
                           BlockInfo[BlockSeq].StrPlaneHitHistory.TypeHij[i].TypeHij[j].TranH += (1 << (3 - t));
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sign <<= 1;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sign += SIGN(block[k][p]);
                        }
                        else
                        {
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_len ++;
                           BlockInfo[BlockSeq].SymbolsBlock[si].sym_val <<= 1;
                        }
                     }
                     else
                     {   //refinement.
                        temp = ((AMPLITUDE(block[k][p]) & Bit_Set_Plane) > 0 ? 1 : 0);
                        if((PtrCoding->PtrHeader->Header.Part4).DWTType == INTEGER_WAVELET)
                        {
                           if(((i == 0) && (PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits < BitPlane )) // HL3 band.
                              || ((i == 1) && (PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits < BitPlane ))
                              || ((i == 2) &&   (PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits < BitPlane )))
                           {
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenSymbolLength ++;
                           }
                        }
                        else
                        {
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol <<= 1;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenRefSymbol += temp;
                              BlockInfo[BlockSeq].RefineBits.RefineGrandChildren[i].GrandChildrenSymbolLength ++;
                        }
                     }
                     t++;
                  }
            }
         }
      }
   }

return;
}



