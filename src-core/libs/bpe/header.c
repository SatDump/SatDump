#include <string.h>
#include <stdlib.h>
#include "bpe_internal.h"

void HeaderInilization(StructCodingPara *Ptr)
{
    Ptr->PtrHeader = (HeaderStruct *)calloc(1, sizeof(HeaderStruct));
    Ptr->PtrHeader->Header.Part1.StartImgFlag       = TRUE;
    Ptr->PtrHeader->Header.Part1.EngImgFlg          = FALSE;
    Ptr->PtrHeader->Header.Part1.SegmentCount_8Bits = 0;
    Ptr->PtrHeader->Header.Part1.BitDepthAC_5Bits   = 0;
    Ptr->PtrHeader->Header.Part1.BitDepthDC_5Bits   = 0;
    Ptr->PtrHeader->Header.Part1.Part2Flag          = TRUE;
    Ptr->PtrHeader->Header.Part1.Part3Flag          = TRUE;
    Ptr->PtrHeader->Header.Part1.Part4Flag          = TRUE;
    Ptr->PtrHeader->Header.Part1.PadRows_3Bits      = 0;
    Ptr->PtrHeader->Header.Part1.Reserved_5Bits     = 0;

    Ptr->PtrHeader->Header.Part2.SegByteLimit_27Bits = 0;
    Ptr->PtrHeader->Header.Part2.DCstop              = FALSE;
    Ptr->PtrHeader->Header.Part2.BitPlaneStop_5Bits  = 0;
    Ptr->PtrHeader->Header.Part2.StageStop_2Bits     = 3;
    Ptr->PtrHeader->Header.Part2.UseFill             = FALSE;
    Ptr->PtrHeader->Header.Part2.Reserved_4Bits      = 0;

    Ptr->PtrHeader->Header.Part3.S_20Bits       = 256;
    Ptr->PtrHeader->Header.Part3.OptDCSelect    = TRUE;
    Ptr->PtrHeader->Header.Part3.OptACSelect    = TRUE;
    Ptr->PtrHeader->Header.Part3.Reserved_2Bits = 0;

    Ptr->PtrHeader->Header.Part4.DWTType               = INTEGER_WAVELET;
    Ptr->PtrHeader->Header.Part4.Reserved_2Bits        = 0;
    Ptr->PtrHeader->Header.Part4.SignedPixels          = FALSE;
    Ptr->PtrHeader->Header.Part4.PixelBitDepth_4Bits   = 8;
    Ptr->PtrHeader->Header.Part4.ImageWidth_20Bits     = 2048;
    Ptr->PtrHeader->Header.Part4.TransposeImg          = NOTRANSPOSE;
    Ptr->PtrHeader->Header.Part4.CodewordLength_2Bits  = 0;
    Ptr->PtrHeader->Header.Part4.Reserved              = 0;
    Ptr->PtrHeader->Header.Part4.CustomWtFlag          = FALSE;
    Ptr->PtrHeader->Header.Part4.CustomWtHH1_2bits     = 0;
    Ptr->PtrHeader->Header.Part4.CustomWtHL1_2bits     = 1;
    Ptr->PtrHeader->Header.Part4.CustomWtLH1_2bits     = 1;
    Ptr->PtrHeader->Header.Part4.CustomWtHH2_2bits     = 1;
    Ptr->PtrHeader->Header.Part4.CustomWtHL2_2bits     = 2;
    Ptr->PtrHeader->Header.Part4.CustomWtLH2_2bits     = 2;
    Ptr->PtrHeader->Header.Part4.CustomWtHH3_2bits     = 2;
    Ptr->PtrHeader->Header.Part4.CustomWtHL3_2bits     = 3;
    Ptr->PtrHeader->Header.Part4.CustomWtLH3_2bits     = 3;
    Ptr->PtrHeader->Header.Part4.CustomWtLL3_2bits     = 3;
    Ptr->PtrHeader->Header.Part4.Reserved_11Bits       = 0;

    Ptr->ImageRows    = 0;
    Ptr->ImageWidth   = 0;
    Ptr->BitsPerPixel = 0;
    Ptr->PixelByteOrder = 0;

    Ptr->Bits = (BitStream *)calloc(1, sizeof(BitStream));
    Ptr->Bits->ByteBuffer_4Bytes     = 0;
    Ptr->Bits->CodeWordAlighmentBits = 0;
    Ptr->Bits->SegBitCounter         = 0;
    Ptr->Bits->TotalBitCounter       = 0;
    Ptr->Bits->buf_owned             = 0;
    Ptr->Bits->buf                   = NULL;
    Ptr->Bits->buf_size              = 0;
    Ptr->Bits->buf_pos               = 0;
    Ptr->SegmentFull = FALSE;
    Ptr->RateReached = FALSE;

    Ptr->DecodingStopLocations.BitPlaneStopDecoding  = -1;
    Ptr->DecodingStopLocations.BlockNoStopDecoding   = -1;
    Ptr->DecodingStopLocations.LocationFind          = FALSE;
    Ptr->DecodingStopLocations.X_LocationStopDecoding = -1;
    Ptr->DecodingStopLocations.Y_LocationStopDecoding = -1;
    Ptr->DecodingStopLocations.stoppedstage          = 10;

    Ptr->Bits->CodeWord_Length = 8; /* default: CodewordLength_2Bits == 0 */

    Ptr->altStrageStop = FALSE;
}

void HeaderOutput(StructCodingPara *PtrCoding)
{
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.StartImgFlag,       1);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.EngImgFlg,          1);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.SegmentCount_8Bits, 8);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits,   5);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits,   5);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.Reserved,           1);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.Part2Flag,          1);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.Part3Flag,          1);
    BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.Part4Flag,          1);

    if (PtrCoding->PtrHeader->Header.Part1.EngImgFlg == TRUE) {
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits,  3);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part1.Reserved_5Bits, 5);
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part2Flag == TRUE) {
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits, 27);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.DCstop,              1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.BitPlaneStop_5Bits,  5);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.StageStop_2Bits,     2);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.UseFill,             1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part2.Reserved_4Bits,      4);
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part3Flag == TRUE) {
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part3.S_20Bits,       20);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part3.OptDCSelect,     1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part3.OptACSelect,     1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part3.Reserved_2Bits,  2);
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part4Flag == TRUE) {
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.DWTType,               1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.Reserved_2Bits,        2);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.SignedPixels,           1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.PixelBitDepth_4Bits,   4);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.ImageWidth_20Bits,    20);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.TransposeImg,          1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits,  2);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.Reserved,              1);
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtFlag,          1);
        if (PtrCoding->PtrHeader->Header.Part4.CustomWtFlag == FALSE) {
            BitsOutput(PtrCoding, 0, 8);
            BitsOutput(PtrCoding, 0, 8);
            BitsOutput(PtrCoding, 0, 4);
        } else {
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHH3_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtHL3_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtLH3_2bits, 2);
            BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits, 2);
        }
        BitsOutput(PtrCoding, PtrCoding->PtrHeader->Header.Part4.Reserved_11Bits, 11);
    }
}

void HeaderReadin(StructCodingPara *PtrCoding)
{
    DWORD32 Byte;

    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.StartImgFlag = (BOOL)Byte;
    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.EngImgFlg = (BOOL)Byte;
    BitsRead(PtrCoding, &Byte, 8);
    PtrCoding->PtrHeader->Header.Part1.SegmentCount_8Bits = (UCHAR8)Byte;
    BitsRead(PtrCoding, &Byte, 5);
    PtrCoding->PtrHeader->Header.Part1.BitDepthDC_5Bits = (UCHAR8)Byte;
    BitsRead(PtrCoding, &Byte, 5);
    PtrCoding->PtrHeader->Header.Part1.BitDepthAC_5Bits = (UCHAR8)Byte;
    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.Reserved = (UCHAR8)Byte;
    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.Part2Flag = (BOOL)Byte;
    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.Part3Flag = (BOOL)Byte;
    BitsRead(PtrCoding, &Byte, 1);
    PtrCoding->PtrHeader->Header.Part1.Part4Flag = (BOOL)Byte;

    if (PtrCoding->PtrHeader->Header.Part1.EngImgFlg == TRUE) {
        BitsRead(PtrCoding, &Byte, 3);
        PtrCoding->PtrHeader->Header.Part1.PadRows_3Bits = (UCHAR8)Byte;
        BitsRead(PtrCoding, &Byte, 5);
        PtrCoding->PtrHeader->Header.Part1.Reserved_5Bits = (UCHAR8)Byte;
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part2Flag == TRUE) {
        BitsRead(PtrCoding, &Byte, 27);
        PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits = Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part2.DCstop = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 5);
        PtrCoding->PtrHeader->Header.Part2.BitPlaneStop_5Bits = (UCHAR8)Byte;
        BitsRead(PtrCoding, &Byte, 2);
        PtrCoding->PtrHeader->Header.Part2.StageStop_2Bits = (UCHAR8)Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part2.UseFill = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 4);
        PtrCoding->PtrHeader->Header.Part2.Reserved_4Bits = (UCHAR8)Byte;
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part3Flag == TRUE) {
        BitsRead(PtrCoding, &Byte, 20);
        PtrCoding->PtrHeader->Header.Part3.S_20Bits = Byte;

        if (PtrCoding->BitsPerPixel != 0) {
            PtrCoding->DecodingAllowedBitsSizeInSegment = (DWORD32)(
                PtrCoding->BitsPerPixel *
                PtrCoding->PtrHeader->Header.Part3.S_20Bits * 64);
            if (PtrCoding->DecodingAllowedBitsSizeInSegment >
                    ((DWORD32)PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits << 3))
                PtrCoding->DecodingAllowedBitsSizeInSegment =
                    ((DWORD32)PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits << 3);
        } else {
            PtrCoding->DecodingAllowedBitsSizeInSegment = 0;
            if (PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits != 0)
                PtrCoding->DecodingAllowedBitsSizeInSegment =
                    ((DWORD32)PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits << 3);
        }

        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part3.OptDCSelect = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part3.OptACSelect = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 2);
        PtrCoding->PtrHeader->Header.Part3.Reserved_2Bits = (UCHAR8)Byte;
    } else {
        PtrCoding->PtrHeader->Header.Part2.SegByteLimit_27Bits = 0;
    }

    if (PtrCoding->PtrHeader->Header.Part1.Part4Flag == TRUE) {
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part4.DWTType = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 2);
        PtrCoding->PtrHeader->Header.Part4.Reserved_2Bits = (UCHAR8)Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part4.SignedPixels = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 4);
        PtrCoding->PtrHeader->Header.Part4.PixelBitDepth_4Bits = (UCHAR8)Byte;
        BitsRead(PtrCoding, &Byte, 20);
        PtrCoding->PtrHeader->Header.Part4.ImageWidth_20Bits = (DWORD32)Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part4.TransposeImg = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 2);
        PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits = (UCHAR8)Byte;

        if      (PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits == 0)
            PtrCoding->Bits->CodeWord_Length = 8;
        else if (PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits == 1)
            PtrCoding->Bits->CodeWord_Length = 16;
        else if (PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits == 2)
            PtrCoding->Bits->CodeWord_Length = 24;
        else if (PtrCoding->PtrHeader->Header.Part4.CodewordLength_2Bits == 3)
            PtrCoding->Bits->CodeWord_Length = 32;

        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part4.Reserved = (BOOL)Byte;
        BitsRead(PtrCoding, &Byte, 1);
        PtrCoding->PtrHeader->Header.Part4.CustomWtFlag = (BOOL)Byte;
        if (Byte == TRUE) {
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHH1_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHL1_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtLH1_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHH2_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHL2_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtLH2_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHH3_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtHL3_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtLH3_2bits = (UCHAR8)Byte;
            BitsRead(PtrCoding, &Byte, 2);
            PtrCoding->PtrHeader->Header.Part4.CustomWtLL3_2bits = (UCHAR8)Byte;
        } else {
            BitsRead(PtrCoding, &Byte, 20);
        }
        BitsRead(PtrCoding, &Byte, 11);
        PtrCoding->PtrHeader->Header.Part4.Reserved_11Bits = (WORD16)Byte;
    }

    PtrCoding->DecodingStopLocations.BitPlaneStopDecoding  = -1;
    PtrCoding->DecodingStopLocations.BlockNoStopDecoding   = -1;
    PtrCoding->DecodingStopLocations.LocationFind          = FALSE;
    PtrCoding->DecodingStopLocations.X_LocationStopDecoding = -1;
    PtrCoding->DecodingStopLocations.Y_LocationStopDecoding = -1;
}

void HeaderUpdate(HeaderStruct *HeaderStr)
{
    if (HeaderStr->Header.Part1.StartImgFlag == TRUE)
        HeaderStr->Header.Part1.StartImgFlag = FALSE;
    HeaderStr->Header.Part1.BitDepthAC_5Bits    = 0;
    HeaderStr->Header.Part1.BitDepthDC_5Bits    = 0;
    HeaderStr->Header.Part1.SegmentCount_8Bits++;

    HeaderStr->Header.Part1.Part2Flag = FALSE;
    HeaderStr->Header.Part1.Part3Flag = FALSE;
    HeaderStr->Header.Part1.Part4Flag = FALSE;

    if (HeaderStr->Header.Part3.S_20Bits < 16)
        ErrorMsg(BPE_INVALID_HEADER);
}
