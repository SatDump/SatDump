#include "common.h"
#include <cmath>
#include "defines.h"
#include <cstring>
#include "common/image/jpeg12_utils.h"

namespace hinode
{
    ScienceHeader HinodeDepacketizer::parse_science_header(ccsds::CCSDSPacket &pkt)
    {
        ScienceHeader wk;

        uint8_t *p = &pkt.payload[0];
        wk.DataType = p[12];                                                // DataType
        wk.PacketSize = p[13] << 16 | p[14] << 8 | p[15];                   // PacketSize
        wk.SerialPacketNo = p[16] << 24 | p[17] << 16 | p[18] << 8 | p[19]; // SerialPacketNo
        wk.MainID = p[20] << 8 | p[21];                                     // MainID
        wk.MainSQFlag = (p[22] & 0xc0) >> 6;                                // MainSequenceFlag
        wk.MainSQCount = (p[22] & 0x3f) << 8 | p[23] << 0;                  // MainSequenceCount
                                                                            // p[30] ̏2 bit  reserved
        wk.NumOfPacket = (p[24] & 0x3f) << 2 | (p[25] & 0xc0) >> 6;         // NumberOfPacket
        wk.NumOfFrame = p[25] & 0x3f;                                       // NumberOfFrame
        wk.SubID = p[26] << 8 | p[27];                                      // SubID
        wk.SubSQFlag = (p[28] & 0xc0) >> 6;                                 // SequenceFlag
        wk.SubSQCount = (p[28] & 0x3f) << 8 | p[29];                        // SequenceCount
        wk.FullImageSizeX = p[30] << 8 | p[31];                             // FullImageSizeX
        wk.FullImageSizeY = p[32] << 8 | p[33];                             // FullImageSizeY
        wk.BasePointCoorX = p[34] << 8 | p[35];                             // BasePointCoorX
        wk.BasePointCoorY = p[36] << 8 | p[37];                             // BasePointCoorY
        wk.PartImageSizeX = p[38] << 8 | p[39];                             // PartImageSizeX
        wk.PartImageSizeY = p[40] << 8 | p[41];                             // PartImageSizeY
                                                                            // p[48] 1 bit  reserved
        wk.BitCompMode = (p[42] & 0x78) >> 3;                               // k[hibit)
        wk.ImageCompMode = p[42] & 0x07;                                    // k[hiImage)
                                                                            // p[49] ̏1 bit  reserved
        wk.HTACNo = (p[43] & 0x60) >> 5;                                    // nt}ACNo
        wk.HTDCNo = (p[43] & 0x18) >> 3;                                    // nt}DCNo
        wk.QTNo = p[43] & 0x07;                                             // ʎqe[uNo

        {
            int restart_pixel = 99; // invalid
            int num_segment = 99;   // invalid

            switch (wk.ImageCompMode)
            {
            case COMP_MODE_DPCM12: // DPCM���k
                for (restart_pixel = wk.PartImageSizeX; restart_pixel % 64 != 0;)
                { // ���X�^�[�g�Ԋu
                    restart_pixel = restart_pixel * 2;
                }
                num_segment = (int)ceil(1.0 * wk.PartImageSizeX * wk.PartImageSizeY / restart_pixel);
                break;

            case COMP_MODE_DCT12:           // DCT���k
                restart_pixel = 8 * 8 * 64; // ���X�^�[�g�Ԋu
                num_segment = (int)ceil(1.0 * wk.PartImageSizeX * wk.PartImageSizeY / restart_pixel);
                break;

            default:
                wk.ImageCompMode = 0;
            }

            wk.restart_pixel = restart_pixel;
            wk.num_segment = num_segment;
        }

        // uint32_t time = p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
        // double ttime = time + 946684800.0;
        // printf("TIME = %s\n", timestamp_to_string(ttime).c_str());

        num_RST = 0;
        restored_flag = 0;
        buff_pos = 0;
        ECS0_start = 0;

        return wk;
    }

    HinodeDepacketizer::HinodeDepacketizer()
    {
        buffer = new uint8_t[MAX_COMP_LEN];
        _chk_tbl = new uint8_t[MAX_PIXELS];
        rst_tbl_array = new rst_tbl_t[MAX_RST];
        status = JPEG_INIT;
    }

    HinodeDepacketizer::~HinodeDepacketizer()
    {
        delete[] buffer;
        delete[] _chk_tbl;
        delete[] rst_tbl_array;
    }

    int HinodeDepacketizer::work(ccsds::CCSDSPacket &pkt, DecodedImage *result)
    {
        // if (pkt.payload.size() == pkt.header.packet_length + 1)
        //     return status;
        result->apid = -1;

        int wk = 0;
        int rst2ndNo = 0;

        if (status != BUFF_NOW)
        { /* 1. waiting for start of next image */
            if (pkt.header.sequence_flag == 0b01)
            {
                sci_hdr = parse_science_header(pkt);

                if (sci_hdr.ImageCompMode != 0)
                { // JPEG data
                    status = BUFF_NOW;
                    before_sq_count = pkt.header.packet_sequence_count;
                    before_sq_flag = pkt.header.sequence_flag;
                }
                else
                { // not JPEG data
                    status = NOT_JPEG_DATA;
                }
            }

            if (status == JPEG_INIT)
                return JPEG_IRRETRIEVABLE; // KM note *12

            return status;
        }
        else
        { /* 2. Processing of image */

            rst_tbl_t *rst_tbl = &rst_tbl_array[0];

            if (pkt.header.sequence_flag == 0b01)
            { /* 2.1. ���̓p�P�b�g����P�p�P�b�g�̏ꍇ */
                /* finalize process of accumulated image */

                int iret = 99; // invalid, determined soon

                if (before_sq_flag == 0b01)
                {
                    /* 2.1.1. Processing of designed pattern 8 */
                    iret = JPEG_IRRETRIEVABLE; // discard accumlated image
                }
                else
                {
                    /* 2.1.2. Processing of designed pattern 4, 10 */
                    fill_tail(); // recover RSTn, EOI [KM51]
                    // Input: _sci_head.num_segment, Modified: num_RST, rst_tbl, buff_pos, buffer

                    if (num_RST != sci_hdr.num_segment - 1)
                    { // check number of RSTn
                        /* recovery failed, retry recovery */
                        wk = trunc_recovered(); // [KM52], KM note *9)
                        // Input: _sci_head.num_segment, Modified: num_RST, rst_tbl, buff_pos, buffer

                        //  dbg("KM52 wk=%d num_RST=%d num_segment=%d\n", wk, *num_RST, _sci_head->num_segment);

                        if (wk != 0)
                        {
                            // dbg("KM72");

                            return status = JPEG_IRRETRIEVABLE; // discard accumlated image
                                                                // KM: why return here ??
                        }
                    }

                    /* recovery succeeded */

                    clear_chktbl(); //////
                    fill_chktbl();
                    // Input: _sci_head, num_RST, rst_tbl, output: _chk_tbl

                    {
                        result->apid = pkt.header.apid;
                        result->sci = sci_hdr;
                        result->img = image::decompress_jpeg12(buffer, buff_pos);
                        img_cnt++;
                    }

                    // dbg("KM61");

                    iret = JPEG_RESTORED;

                    /* end of 2.1.2. */
                }

                sci_hdr = parse_science_header(pkt);

                if (sci_hdr.ImageCompMode != 0)
                { // JPEG data
                    status = BUFF_NOW;
                    before_sq_count = pkt.header.packet_sequence_count;
                    before_sq_flag = pkt.header.sequence_flag;
                }
                else
                { // not JPEG data
                    status = NOT_JPEG_DATA;
                }

                return iret;
                /* end of 2.1. */
            }
            else
            {
                /* 2.2. ���̓p�P�b�g����P�p�P�b�g�ȊO�̏ꍇ */
                int istatus = BUFF_NOW;
                int istart_flag = 1;
                const int data_length = pkt.header.packet_length + 1 - 4;

                const int sq_count_diff = (pkt.header.packet_sequence_count - before_sq_count + ALIGN) % ALIGN;

                if (sq_count_diff >= MAX_SQDIFF)
                {
                    /* 2.2.1. too large counter skip */

                    if (before_sq_flag == 0b01)
                    {
                        // dbg("KM73");

                        return status = JPEG_IRRETRIEVABLE; // discard accumulated image
                    }
                    else
                    {
                        fill_tail(); // recover RSTn, EOI [KM54] KM note *10)
                        // Input: _sci_head.num_segment, Modified: num_RST, rst_tbl, buff_pos, buffer
                        // dbg("KM73a");

                        istatus = RESTORED_IRRETRIEVABLE; // finish accumulation of image
                        istart_flag = 0;
                    }

                    /* end of 2.2.1. */
                }
                else if (sq_count_diff > 1)
                {
                    /* 2.2.2. not too large counter skip */

                    /* ���͂����p�P�b�g����RSTn�̌��� */
                    int packetpos = -1;
#if 0
 	for (i=0; i<data_length; i++) {
#else
                    // KM notice *7 from here
                    for (int i = 0; i < data_length - 1; i++)
                    {
                        // KM notice *7 until here
#endif
                        if (pkt.payload[i + 4] == 0xFF)
                        {
                            const int rst2nd = pkt.payload[i + 5] & RST2ND_MASK;
                            rst2ndNo = pkt.payload[i + 5] & RSTNO_MASK;
                            if (rst2nd == 0xD0)
                            {
                                packetpos = i; // CCSDS�p�P�b�g����RST�̍ŏ��̈ʒu
                                break;
                            }
                        }
                    }

                    if (packetpos == -1)
                    {
                        /* 2.2.2.1. no RSTn found */

                        if (pkt.header.sequence_flag != 0b10)
                        {
                            return status = BUFF_NOW; // discard packet, continue accumulation
                                                      // do not update before_sq_count, before_sq_flag
                        }
                        else
                        {

                            if (before_sq_flag == 0b01)
                            {

                                // dbg("KM74");

                                return status = JPEG_IRRETRIEVABLE; // discard accumulated image of image
                            }
                            else
                            {
                                fill_tail(); // recover RSTn, EOI [KM55] KM note *10)
                                // Input: _sci_head.num_segment, Modified: num_RST, rst_tbl, buff_pos, buffer

                                // dbg("KM62");

                                istatus = JPEG_RESTORED; // finish accumulation of image
                                istart_flag = 0;
                            }
                        }

                        /* end of 2.2.2.1. */
                    }
                    else
                    {
                        /* 2.2.2.2. RSTn found */

                        if (before_sq_flag == 0b01)
                        {

                            if (sci_hdr.ImageCompMode == COMP_MODE_DCT12)
                            {
                                insert_dct_header();
                            }
                            else if (sci_hdr.ImageCompMode == COMP_MODE_DPCM12)
                            {
                                insert_dpcm_header();
                            }

                            ECS0_start = buff_pos; // �o�b�t�@�̍ŏI�|�C���^����
                        }

                        fill_gap(pkt, data_length, packetpos, rst2ndNo); // recover RSTn, add packet [KM53, KM56]
                        // Input: ECS0_start, Modified: num_RST, rst_tbl, buff_pos, buffer

                        if (pkt.header.sequence_flag == 0b10)
                        {
                            // dbg("KM63");

                            istatus = JPEG_RESTORED; // finsish accumulation of image
                            istart_flag = 0;
                        }
                        else
                        {
                            // istatus = BUFF_NOW; // continue accumlation of image
                        }

                        /* end of 2.2.2.2. */
                    }

                    /* end of 2.2.2. */
                }
                else
                {
                    /* 2.2.3. no counter skip */

                    int markpos = -1;
                    // KM notice *8 from here
                    if (buffer[buff_pos - 1] == 0xFF)
                    {
                        markpos = buff_pos - 1;
                    }
                    // KM notice *8 until here
                    for (int i = 0; i < data_length; i++)
                    {
                        const unsigned char octet = pkt.payload[i + 4];
                        buffer[buff_pos] = octet;
                        if (octet == 0xFF)
                        {
                            markpos = buff_pos;
                        }
                        else
                        {
                            if (markpos >= 0)
                            {
                                const int rst2nd = octet & RST2ND_MASK;
                                rst2ndNo = octet & RSTNO_MASK;
                                if (rst2nd == 0xD0)
                                { // RSTn����������
                                    rst_tbl[num_RST].pos = markpos;
                                    rst_tbl[num_RST].No = rst2ndNo;
                                    rst_tbl[num_RST].flg = 0;
                                    num_RST++;
                                }
                                else if (octet == 0xDA)
                                {                              // SOS ����������
                                    ECS0_start = buff_pos + 9; // �摜�f�[�^�̊J�n�ʒu�́ASOS �}�[�J�̂Q�o�C�g�ڂ���A�X�o�C�g��ɂ���B
                                                               // KM notice *3 from here
                                }
                                else if (octet == 0xD9)
                                { // EOI
                                    rst_tbl[num_RST].flg = 0;
                                    // KM notice *3 until here
                                }
                            }
                            markpos = -1;
                        }
                        buff_pos++;
                    }

                    if (pkt.header.sequence_flag == 0b10)
                    {
                        //// dbg("KM64");

                        istatus = JPEG_RESTORED; // finsish accumulation of image
                        istart_flag = 0;
                    }
                    else
                    {
                        // istatus = BUFF_NOW; // continue accumlation of image
                    }

                    /* end of 2.2.3. */
                }

                if (istatus != BUFF_NOW)
                {
                    clear_chktbl();
                    // Output: _chk_tbl
                }

                if (istatus == JPEG_RESTORED && istart_flag == 0)
                {

                    if (restored_flag == 0)
                    {
                        istatus = JPEG_EXTRACT;
                    }
                    else
                    {

                        if (num_RST != sci_hdr.num_segment - 1)
                        { // check number of RSTn

                            /* recovery failed, retry recovery */
                            wk = trunc_recovered(); // [KM57]
                            // Input: _sci_head.num_segment, Modified: num_RST, rst_tbl, buff_pos, buffer
                            if (wk != 0)
                            {
                                // dbg("KM75");

                                return status = JPEG_IRRETRIEVABLE; // discard accumlated image
                            }
                            // dbg("KM75a");

                            istatus = RESTORED_IRRETRIEVABLE; // finish accumulation of image
                        }

                        /* recovery succeeded */

                        clear_chktbl(); //////
                        fill_chktbl();
                        // Input: _sci_head, num_RST, rst_tbl, output: _chk_tbl
                    }
                }

                if (istatus != BUFF_NOW)
                {
                    /* JPEG_EXTRACT or JPEG_RESTORED or RESTORED_IRRETRIEVABLE */
                    /* JPEG �f�[�^���i�[�����o�b�t�@�̃|�C���^��Ԃ� */
                    {
                        result->apid = pkt.header.apid;
                        result->sci = sci_hdr;
                        result->img = image::decompress_jpeg12(buffer, buff_pos);
                        img_cnt++;
                    }
                }
                else
                {
                    /* BUFF_NOW */
                    /* NULL ��Ԃ� */

                    before_sq_count = pkt.header.packet_sequence_count;
                    before_sq_flag = pkt.header.sequence_flag;
                }

                switch (istatus)
                {
                case JPEG_EXTRACT:
                case JPEG_RESTORED:
                    status = (istart_flag == 0) ? NOT_JPEG_DATA : BUFF_NOW;
                    break;
                case RESTORED_IRRETRIEVABLE:
                    status = (istart_flag == 0) ? JPEG_IRRETRIEVABLE : BUFF_NOW;
                    break;
                case BUFF_NOW:
                    status = BUFF_NOW;
                    break;
                default:
                    fprintf(stderr, "DePacketizeBody(): fatal logic error (1266)\n");
                }

                return istatus;
                /* end of 2.2. */
            }
            /* end of 2. */
        }
    }
}