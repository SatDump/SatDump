#include <stdlib.h>
#include <string.h>
#include "bpe_internal.h"

/* Write one byte to the encoder output buffer, growing it as needed. */
static void bpe_buf_putbyte(BitStream *bs, UCHAR8 byte)
{
    if (bs->buf_pos >= bs->buf_size) {
        size_t new_size = bs->buf_size ? bs->buf_size * 2 : 65536;
        UCHAR8 *new_buf = (UCHAR8 *)realloc(bs->buf, new_size);
        if (!new_buf)
            ErrorMsg(BPE_MEM_ERROR);
        bs->buf      = new_buf;
        bs->buf_size = new_size;
    }
    bs->buf[bs->buf_pos++] = byte;
}

/* Read one byte from the decoder input buffer.
 * Returns 0 (not EOF) when the stream is exhausted – the rate-control logic
 * in BitsRead should prevent reads past valid data, but returning 0 is
 * safer than returning -1 which would corrupt ByteBuffer_4Bytes. */
static int bpe_buf_getbyte(BitStream *bs)
{
    if (bs->buf_pos >= bs->buf_size)
        return 0;
    return (unsigned char)bs->buf[bs->buf_pos++];
}

static void OutputCodeWord(StructCodingPara *Ptr)
{
    Ptr->Bits->CodeWordAlighmentBits++;
    Ptr->Bits->SegBitCounter++;
    Ptr->Bits->TotalBitCounter++;

    if (Ptr->Bits->CodeWordAlighmentBits == Ptr->Bits->CodeWord_Length) {
        if (Ptr->Bits->CodeWord_Length == 8) {
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)Ptr->Bits->ByteBuffer_4Bytes);
        } else if (Ptr->Bits->CodeWord_Length == 16) {
            WORD16 temp = (WORD16)Ptr->Bits->ByteBuffer_4Bytes;
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)( temp        & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((temp >> 8)  & 0xFF));
        } else if (Ptr->Bits->CodeWord_Length == 24) {
            /* Replicate original byte-order: byte 0 (LSB), byte 1, byte 2 */
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)( Ptr->Bits->ByteBuffer_4Bytes        & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((Ptr->Bits->ByteBuffer_4Bytes >>  8) & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((Ptr->Bits->ByteBuffer_4Bytes >> 16) & 0xFF));
        } else if (Ptr->Bits->CodeWord_Length == 32) {
            UINT32 temp = Ptr->Bits->ByteBuffer_4Bytes;
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)( temp        & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((temp >>  8) & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((temp >> 16) & 0xFF));
            bpe_buf_putbyte(Ptr->Bits, (UCHAR8)((temp >> 24) & 0xFF));
        }
        Ptr->Bits->CodeWordAlighmentBits = 0;
    }
}

void BitsOutput(StructCodingPara *Ptr, DWORD32 bit, int length)
{
    short  i;
    UCHAR8 temp_bits;

    if (length == 0)
        return;
    if (Ptr->SegmentFull == TRUE)
        return;

    if (Ptr->PtrHeader->Header.Part2.SegByteLimit_27Bits != 0) {
        if (Ptr->Bits->SegBitCounter + (UINT32)length >=
                Ptr->PtrHeader->Header.Part2.SegByteLimit_27Bits * 8) {
            short RemainderBits = (short)(
                Ptr->PtrHeader->Header.Part2.SegByteLimit_27Bits * 8 -
                Ptr->Bits->SegBitCounter);
            Ptr->SegmentFull = TRUE;
            for (i = RemainderBits - 1; i >= 0; i--) {
                temp_bits = (UCHAR8)(0x01 & (bit >> (length - 1)));
                length--;
                Ptr->Bits->ByteBuffer_4Bytes <<= 1;
                Ptr->Bits->ByteBuffer_4Bytes  += temp_bits;
                OutputCodeWord(Ptr);
            }
            return;
        }
    }

    if (length > 32) {
        /* Pad high bits with zeros then encode the low 32 bits. */
        int tt = length - 32;
        Ptr->Bits->ByteBuffer_4Bytes <<= tt;
        OutputCodeWord(Ptr);
        length = 32;
    }

    for (i = length - 1; i >= 0; i--) {
        temp_bits = (UCHAR8)(0x01 & (bit >> i));
        Ptr->Bits->ByteBuffer_4Bytes <<= 1;
        Ptr->Bits->ByteBuffer_4Bytes  += temp_bits;
        OutputCodeWord(Ptr);
    }
}

short BitsRead(StructCodingPara *Ptr, DWORD32 *bit, short length)
{
    UCHAR8 i;
    *bit = 0;

    if (length == 0 || Ptr->SegmentFull == TRUE) {
        *bit = 0;
        return BPE_OK;
    }

    if (Ptr->RateReached == TRUE) {
        *bit = 0;
        return BPE_OK;
    }

    for (i = 0; i < length; i++) {
        if (Ptr->Bits->CodeWordAlighmentBits == 0) {
            if (Ptr->Bits->buf_pos >= Ptr->Bits->buf_size)
                ErrorMsg(BPE_STREAM_ERROR);
            Ptr->Bits->ByteBuffer_4Bytes      = (UINT32)bpe_buf_getbyte(Ptr->Bits);
            Ptr->Bits->CodeWordAlighmentBits   = 8;
        }
        (*bit) <<= 1;
        (*bit) += (UCHAR8)((Ptr->Bits->ByteBuffer_4Bytes >>
                            (Ptr->Bits->CodeWordAlighmentBits - 1)) & 0x01);
        Ptr->Bits->CodeWordAlighmentBits--;
        Ptr->Bits->SegBitCounter++;
        Ptr->Bits->TotalBitCounter++;

        if ((Ptr->Bits->SegBitCounter >= Ptr->DecodingAllowedBitsSizeInSegment)
                && Ptr->DecodingAllowedBitsSizeInSegment != 0) {
            UINT32 CurrentTotalBytes = (Ptr->Bits->SegBitCounter +
                                        Ptr->Bits->CodeWordAlighmentBits) / 8;

            Ptr->RateReached = TRUE;
            Ptr->DecodingStopLocations.BitPlaneStopDecoding   = Ptr->BitPlane - 1;
            Ptr->DecodingStopLocations.TotalBitsReadThisTime  = i + 1;
            (*bit) <<= (length - i - 1);

            while (CurrentTotalBytes < Ptr->PtrHeader->Header.Part2.SegByteLimit_27Bits) {
                bpe_buf_getbyte(Ptr->Bits);
                CurrentTotalBytes++;
            }
            Ptr->SegmentFull = TRUE;
            return BPE_OK;
        }
    }
    return BPE_OK;
}
