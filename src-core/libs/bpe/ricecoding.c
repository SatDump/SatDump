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
#include "global.h"


void RiceCoding(short InputVal,
              short BitLength,
              UCHAR8 *Option,
              StructCodingPara *PtrCoding )
{
   switch(BitLength)
   {
   case 0:    // no need to process
      return;
   case 1:
      BitsOutput(PtrCoding, InputVal, 1);
      break;
   case 2:
      //ouput FS code.
      if(Option[0] == 1)
         BitsOutput(PtrCoding, InputVal, 2);

      else if (Option[0] == 0) // nk = 0
      {
         if(InputVal <= 2)
         {
            BitsOutput(PtrCoding, 0, InputVal);
            BitsOutput(PtrCoding, 1, 1);
         }
         else
            BitsOutput(PtrCoding, 0, 3);
      }
      else
         ErrorMsg(BPE_RICE_CODING_ERROR);
      break;
   case 3:
      if (Option[1] == 0) // nk =0, no split.
      {
         if(InputVal <= 2)
         {
            BitsOutput(PtrCoding, 0, InputVal);
            BitsOutput(PtrCoding, 1, 1);
         }
         else if(InputVal <= 5)
         {
            BitsOutput(PtrCoding, 0, 3);
            BitsOutput(PtrCoding, InputVal - 3, 2);
         }
         else if(InputVal <= 7)
         {
            BitsOutput(PtrCoding, 0, 3);
            BitsOutput(PtrCoding, InputVal, 3);
         }
         else
            ErrorMsg(BPE_RICE_CODING_ERROR);
      }
      else if (Option[1] == 1) // nk =1
      {
         if(InputVal <= 1)
         {
            BitsOutput(PtrCoding, InputVal + 2, 2);
         }
         else if(InputVal <= 3)
         {
            BitsOutput(PtrCoding, InputVal, 3);
         }
         else if(InputVal <= 7)
         {
            BitsOutput(PtrCoding, 0, 2);
            if(InputVal == 4)
               BitsOutput(PtrCoding, 2, 2);
            else
               if(InputVal == 5)
                  BitsOutput(PtrCoding, 3, 2);
               else
                  if(InputVal == 6)
                     BitsOutput(PtrCoding, 0, 2);
                  else
                     if(InputVal == 7)
                        BitsOutput(PtrCoding, 1, 2);
         }
         else
            ErrorMsg(BPE_RICE_CODING_ERROR);
      }
      else if (Option[1] == 3) // uncoded .
      {
         BitsOutput(PtrCoding, InputVal, 3);
      }
      break;
   case 4: //4-bit
      if(Option[2] == 0) // no split.
      {
         if (InputVal <= 3)
         {
            BitsOutput(PtrCoding, 0, InputVal);
            BitsOutput(PtrCoding, 1, 1);
         }
         else if (InputVal <= 7)
         {
            BitsOutput(PtrCoding, 0, 5);
            BitsOutput(PtrCoding, InputVal - 4, 2);
         }
         else if (InputVal <= 15)
         {
            BitsOutput(PtrCoding, 0, 4);
            BitsOutput(PtrCoding, InputVal, 4);
         }
         else
            ErrorMsg(BPE_RICE_CODING_ERROR);
      }
      else if (Option[2] == 1) // split only one bit.
      {
         if (InputVal <= 1)
         {
            BitsOutput(PtrCoding, InputVal + 2, 2);
         }
         else if (InputVal <= 3)
         {
            BitsOutput(PtrCoding, InputVal, 3);
         }
         else if (InputVal <= 5)
         {
            BitsOutput(PtrCoding, 0, 2);
            BitsOutput(PtrCoding, InputVal - 2, 2);
         }
         else if (InputVal <= 11)
         {
            BitsOutput(PtrCoding, 0, 3);
            BitsOutput(PtrCoding, InputVal - 6, 3);
         }
         else if (InputVal <= 15)
         {
            BitsOutput(PtrCoding, 0, 3);
            BitsOutput(PtrCoding, InputVal, 4);
         }
         else
            ErrorMsg(BPE_RICE_CODING_ERROR);
      }
      else if (Option[2] == 2) // split two bits.
      {
         if (InputVal <= 3)
         {
            BitsOutput(PtrCoding, InputVal + 4, 3);
         }
         else if (InputVal <= 7)
         {
            BitsOutput(PtrCoding, InputVal, 4);
         }
         else if (InputVal <= 11)
         {
            BitsOutput(PtrCoding, 0, 2);
            BitsOutput(PtrCoding, InputVal - 4, 3);
         }
         else if (InputVal <= 15)
         {
            BitsOutput(PtrCoding, InputVal - 12, 5);
         }
         else
            ErrorMsg(BPE_RICE_CODING_ERROR);
      }

      else if (Option[2] == 3) // uncoded
      {
         BitsOutput(PtrCoding, InputVal, 4);
      }
      else
         ErrorMsg(BPE_RICE_CODING_ERROR);//"Invalid split option!\n");
      break;
   default:
      ErrorMsg(BPE_RICE_CODING_ERROR);//"Invalid bit length!\n");
   }
}


void RiceDecoding(DWORD32 *decoded,
            short BitLength,
            UCHAR8 *splitOption,
            StructCodingPara *Ptr)
{
   DWORD32 WordReadin = 0;
   DWORD32 i = 0;

   switch(BitLength)
   {
   case 0:
      return; // no need to process
   case 1:
       BitsRead(Ptr, decoded, 1);
      break;
   case 2:
      //ouput FS code.
      switch (splitOption[0])
      {
      case 1:
         BitsRead(Ptr, &WordReadin, 2);
         if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return;
         }

         *decoded = WordReadin;
         return ;
      case 0:
         for(i = 0; i < 3; i ++)
         {
            BitsRead(Ptr, &WordReadin, 1);

            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return ;
            }
            if(WordReadin == 1)   break;
         }
         *decoded = i;
         break;
      default:
         ErrorMsg(BPE_RICE_CODING_ERROR);//"Decoding: ErrorMsg split options!\n");
      }
      break;
   case 3:
      switch (splitOption[1])
      {
      case 3:
         BitsRead(Ptr, &WordReadin, 3); //uncoded
         if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return ;
         }
         *decoded = WordReadin;
         return  ;

      case 1: // nk = 1, max bits value is 11b. i.e., decoded can take values form 0 to 3.

         BitsRead(Ptr, &WordReadin, 2);
               if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return ;
         }
         if((WordReadin & 0x2) > 0)
         {
            if((WordReadin & 0x1) == 0)
               *decoded = 0;
            else
               *decoded = 1;
         }
         else if((WordReadin & 0x1) > 0)
         {
            BitsRead(Ptr, &WordReadin, 1);
            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return ;
            }

            if( WordReadin > 0)
               *decoded = 3;
            else
               *decoded = 2;
         }
         else
         {
            BitsRead(Ptr, &WordReadin, 2);
            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return  ;
            }

            switch (WordReadin)
            {
            case 0x2:
               *decoded = 4;
               break;
            case 0x3:
               *decoded = 5;
               break;
            case 0x0:
               *decoded = 6;
               break;
            case 0x1:
               *decoded = 7;
               break;
            }
         }
         return  ;

      case 0:
         WordReadin = 0;
         for ( i = 0; i < 3; i++)
         {
            BitsRead(Ptr, &WordReadin, 1);
            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return  ;
            }
            if(WordReadin == 1)
               break;
         }
         if (WordReadin == 1)  // the third letter is zero.
            *decoded = i;
         else
         {
            BitsRead(Ptr, &WordReadin, 2);

            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return  ;
            }

            if(WordReadin != 3)
            {
               *decoded = WordReadin + 3;
            }
            else
            {
               BitsRead(Ptr, &WordReadin, 1);

               if (WordReadin == 0)
                  *decoded = 6;
               else
                  *decoded = 7;
            }

         }
         return  ;
      }

   case 4:
      switch (splitOption[2])
      {
      case 3:
         BitsRead(Ptr, &WordReadin,4); //uncoded

         if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return  ;
         }
         *decoded = WordReadin;
         return  ;
      case 2:

         BitsRead(Ptr, &WordReadin, 3);
         if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return  ;
         }

         if((WordReadin & 0x4) > 0)
         {
            switch (WordReadin & 0x3)
            {
            case 0x0:
               *decoded = 0;
               break;
            case 0x1:
               *decoded = 1;
               break;
            case 0x2:
               *decoded = 2;
               break;
            case 0x3:
               *decoded = 3;
               break;
            }
         }
         else if((WordReadin & 0x2) > 0)
         {
            if((WordReadin & 0x1) == 0)
            {
               BitsRead(Ptr, &WordReadin, 1);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }
               if( WordReadin == 0)
                  *decoded = 4;
               else
                  *decoded = 5;
            }
            else
            {
               BitsRead(Ptr, &WordReadin, 1);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }

               if( WordReadin == 0)
                  *decoded = 6;
               else
                  *decoded = 7;
            }
         }
         else
         {
            if( (WordReadin & 0x1) == 1)
            {
               BitsRead(Ptr, &WordReadin, 2);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }
               switch (WordReadin)
               {
               case 0x0:
                  *decoded = 8;
                  break;
               case 0x1:
                  *decoded = 9;
                  break;
               case 0x2:
                  *decoded = 10;
                  break;
               case 0x3:
                  *decoded = 11;
                  break;
               }
            }
            else
            {
               BitsRead(Ptr, &WordReadin, 2);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }
               switch (WordReadin)
               {
               case 0x0:
                  *decoded = 12;
                  break;
               case 0x1:
                  *decoded = 13;
                  break;
               case 0x2:
                  *decoded = 14;
                  break;
               case 0x3:
                  *decoded = 15;
                  break;
               }
            }
         }
         return  ;

      case 1:
         BitsRead(Ptr, &WordReadin, 2);

         if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return  ;
         }
         if(WordReadin >= 2)
         {
            if (WordReadin == 2)
               *decoded = 0;
            else
               *decoded = 1;
         }
         else if ((WordReadin & 1) == 1)
         {
            BitsRead(Ptr, &WordReadin, 1);
               if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return  ;
         }
            if (WordReadin == 0)
               *decoded = 2;
            else
               *decoded = 3;
         }
         else
         {
            BitsRead(Ptr, &WordReadin, 2);

            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return  ;
            }
            if((WordReadin & 0x2) > 0)
            {
               if ((WordReadin &0x1) == 0)
                  *decoded = 4;
               else
                  *decoded = 5;
            }
            else if((WordReadin & 0x1) == 0)
            {
               BitsRead(Ptr, &WordReadin, 2);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }
               switch (WordReadin)
               {
               case 0x0:
                  *decoded = 6;
                  break;
               case 0x1:
                  *decoded = 7;
                  break;
               case 0x2:
                  *decoded = 8;
                  break;
               case 0x3:
                  *decoded = 9;
                  break;
               }
            }
            else
            {
               BitsRead(Ptr, &WordReadin, 2);

               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }

               if(WordReadin == 0)
                  *decoded = 10;
               else if(WordReadin == 1)
                  *decoded = 11;
               else if(WordReadin == 2)
               {
                  BitsRead(Ptr, &WordReadin, 1);

                   if (Ptr->RateReached == TRUE)
                  {
                     *decoded = 0;
                      return  ;
                  }
                  if( WordReadin == 0)
                     *decoded = 12;
                  else
                     *decoded = 13;
               }
               else
               {
                  BitsRead(Ptr, &WordReadin, 1);

                  if (Ptr->RateReached == TRUE)
                  {
                     *decoded = 0;
                     return  ;
                  }
                  if( WordReadin == 0)
                     *decoded = 14;
                  else
                     *decoded = 15;
               }
            }
         }
         return  ;
      case 0:
         for ( i = 0; i < 4; i++)
         {
            BitsRead(Ptr, &WordReadin, 1);
               if (Ptr->RateReached == TRUE)
         {
            *decoded = 0;
            return  ;
         }
            if(WordReadin == 1)   break;
         }
         if (i != 4)
            *decoded = i;
         else
         {
            BitsRead(Ptr, &WordReadin, 3);

            if (Ptr->RateReached == TRUE)
            {
               *decoded = 0;
               return ;
            }
            if((WordReadin & 0x4) == 0)
            {
               switch (WordReadin)
               {
               case 0x0:
                  *decoded = 4;
                  break;
               case 0x1:
                  *decoded = 5;
                  break;
               case 0x2:
                  *decoded = 6;
                  break;
               case 0x3:
                  *decoded = 7;
                  break;
               }
            }
            else
            {
               *decoded = WordReadin;
               *decoded <<= 1;
               BitsRead(Ptr, &WordReadin, 1);
               if (Ptr->RateReached == TRUE)
               {
                  *decoded = 0;
                  return  ;
               }
               *decoded += WordReadin;
            }
         }
         return  ;
      }
   default:
      ErrorMsg(BPE_RICE_CODING_ERROR);
   }
   return  ;
}





