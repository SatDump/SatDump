#include "common.h"
#include "defines.h"

namespace hinode
{
    void HinodeDepacketizer::fill_gap(const ccsds::CCSDSPacket &pkt, int data_length, int packetpos, int rst2ndNo)
    {
        /* defintiion of file internal variable refferences */

        struct rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // modified

        /* */

        int packet_first_rstNo;
        int buff_end_rstNo;
        int markpos;
        int i;

        packet_first_rstNo = rst2ndNo; // CCSDS�p�P�b�g�̍ŏ��Ɍ�������
        // RST�ԍ�
        if (num_RST > 0)
        {
            /* �o�b�t�@����RSTn������ꍇ */
            buff_end_rstNo = rst_tbl[num_RST - 1].No; // �o�b�t�@���̍Ō��RST�̔ԍ�
            buff_pos = rst_tbl[num_RST - 1].pos;      // �o�b�t�@���̍Ō��RST�̈ʒu
            buff_pos += 2;
        }
        else
        {
            /* �o�b�t�@����RSTn���Ȃ��ꍇ */
            buff_end_rstNo = -1;
            buff_pos = ECS0_start;
        }
#if 0
  if (packet_first_rstNo < buff_end_rstNo) {
#else
        // KM notice *4 from here
        if (packet_first_rstNo <= buff_end_rstNo)
        {
            // KM notice *4 until here
#endif
            packet_first_rstNo += 8;
        }
        /* RST���o�b�t�@�ɒǉ� */

        const int num_RST1 = num_RST + packet_first_rstNo - buff_end_rstNo;
        add_rst(buff_end_rstNo, num_RST, num_RST1);
        // Modified: rst_tbl, buff_pos, buffer, Output: restored_flag

        num_RST = num_RST1; // [KM53][KM56] KM note *5)

        /*
         *********************************************
         *
         *�@CCSDS�p�P�b�g���̍ŏ���RST������̃f�[�^
         *�@���o�b�t�@�ɒǉ�
         *
         *********************************************
         */

        markpos = -1;
        for (i = packetpos + 2; i < data_length; i++)
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
                }
                markpos = -1;
            }
            buff_pos++;
        }
    }

    void HinodeDepacketizer::clear_chktbl()
    {
        memset(_chk_tbl, 0, MAX_PIXELS);
    }

    void HinodeDepacketizer::fill_chktbl()
    {
        /* defintiion of file internal variable refferences */
        const struct rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // input

        /* */

        const int restart_pixel = sci_hdr.restart_pixel;

        /* */

        int dpcm_col = 0, dpcm_row = 0;
        int i;

        int tmp = 0; //////

        const int partx = sci_hdr.PartImageSizeX;
        const int party = sci_hdr.PartImageSizeY;

        switch (sci_hdr.ImageCompMode)
        {
        case COMP_MODE_DCT12: // DPCM���k
            dpcm_col = 8 * 64;
            dpcm_row = 8;
            break;
        case COMP_MODE_DPCM12:
            dpcm_col = partx;
            dpcm_row = restart_pixel / partx;
            break;
        }

        // dbg("head = %d %d %d", _sci_head.PartImageSizeX, restart_pixel, num_RST);

        for (i = 0; i < num_RST + 1; i++)
        {

            int ii, jj;
            int wk;

            // dbg("rst = %d", i);

            int OffsetX = i * dpcm_col;
            int OffsetY = 0;

            OffsetY += (OffsetX / partx) * dpcm_row;
            OffsetX %= partx;

            if (rst_tbl[i].flg == 1)
            {
                wk = 1;
            }
            else
            {
                wk = 0;
            }

            for (ii = 0; ii < dpcm_row; ii++)
            {
                for (jj = 0; jj < dpcm_col; jj++)
                {

                    int x = OffsetX + jj;
                    int y = OffsetY + ii;

                    y += (x / partx) * dpcm_row;
                    x %= partx;

                    if (y < party)
                    {
                        const int k = y * partx + x;
                        _chk_tbl[k] = wk;

                        tmp += (1 - wk); //////
                    }
                }
            }
        }
        // dbg("pixel = %d\n", tmp);
    }

    int HinodeDepacketizer::trunc_recovered()
    {
        /* defintiion of file internal variable refferences */
        struct rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // modified

        /* */

        const int num_segment = sci_hdr.num_segment;

        /* */

        int i, wk;

        /* �ǉ������ꏊ�̌��� */
        wk = -1;
        for (i = 0; i < num_RST; i++)
        {
            if (rst_tbl[i].flg == 1)
            {
                wk = i;
                break;
            }
        }

        if (wk == -1)
        {
            return -1;
        }

        /* �ǉ������ꏊ�ȍ~���폜 */
        buff_pos = rst_tbl[wk].pos;

        add_rst(rst_tbl[wk].No - 1, wk, num_segment - 1);
        // Modified: rst_tbl, buff_pos, buffer, Output: restored_flag

        num_RST = num_segment - 1;

        add_eoi();
        // Input: num_RST, Modified: rst_tbl, buff_pos, buffer

        return 0;
    }

    void HinodeDepacketizer::fill_tail()
    {
        /* defintiion of file internal variable refferences */
        rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // modified
        /* */
        const int num_segment = sci_hdr.num_segment;
        /* */
        // dbg("%d %d %d\n", rst_tbl[num_RST - 1].No, num_RST, num_segment - 1);
        add_rst(rst_tbl[num_RST - 1].No, num_RST, num_segment - 1);
        // Modified: rst_tbl, buff_pos, buffer, Output: restored_flag
        if (num_RST < num_segment - 1)
            num_RST = num_segment - 1; // [KM51][KM54][KM55]
        add_eoi();
        // Input: num_RST, Modified: rst_tbl, buff_pos, buffer
    }

    void HinodeDepacketizer::add_rst(int p1, int p2, int p3)
    {
        /* defintiion of file internal variable refferences */
        rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // modified

        /* */

        int i, j, n;

        j = p1;
        for (i = p2; i < p3; i++)
        {
            j++;
            n = j % 8;
            rst_tbl[i].pos = buff_pos;
            rst_tbl[i].No = n;
            rst_tbl[i].flg = 1; // recovery flag for RSTn
            buffer[buff_pos++] = 0xFF;
            buffer[buff_pos++] = 0xD0 | n;
        }
        //  (*num_RST) = i;

        restored_flag = 1;
    }

    void HinodeDepacketizer::add_eoi()
    {
        /* defintiion of file internal variable refferences */
        struct rst_tbl_t *rst_tbl = &rst_tbl_array[0]; // modified

        /* */

        buffer[buff_pos] = 0xFF;
        buffer[buff_pos] = 0xD9;
        rst_tbl[num_RST].flg = 1; // recovery flag for EOI

        restored_flag = 1;
    }
}