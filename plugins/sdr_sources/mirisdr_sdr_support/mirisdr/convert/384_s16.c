
/*
 * 10+2 bitový formát
 * 1024 bajtů odpovídá 768 hodnotám
 * struktura:
 *   16b hlavička
 *  984b 6 bloků o velikosti 164b
 *      160b = 128x 10b hodnot
 *        4b = posun vlevo 2b
 *   24b kontrolní součet
 */
static int mirisdr_samples_convert_384_s16 (mirisdr_dev_t *p, unsigned char* buf, uint8_t *dst8, int cnt) {
    int i, i_max, j, k, ret = 0;
    uint32_t addr = 0, shift;
    uint8_t *src = buf;
    int16_t *dst = (int16_t*) dst8;

    /* dostáváme 1-3 1024 bytů dlouhé bloky, poslední část je 24 bitů, tu nezpracováváme */
    for (i_max = cnt >> 10, i = 0; i < i_max; i++, src+= 24) {
        /* pozice hlavičky */
        addr = src[3] << 24 | src[2] << 16 | src[1] << 8 | src[0] << 0;

        /* potenciálně ztracená data */
        if ((i == 0) && (addr != p->addr)) {
            fprintf(stderr, "%u samples lost, %d, %08x:%08x\n", addr - p->addr, cnt, p->addr, addr);
        }

        /* přeskočíme hlavičku 16 bitů, 6 bloků, poslední 4 bajtový posuvný blok zpracujeme */
        for (src+= 16, j = 0; j < 6; j++, src+= 4) {
            /* 2 bity pro každou hodnotu - určení posunu */
            shift = src[160 + 3] << 24 | src[160 + 2] << 16 | src[160 + 1] << 8 | src[160 + 0] << 0;

            /* 16x 10 bajtů */
            for (k = 0; k < 16; k++, src+= 10, ret+= 8) {
                /* 10 bajtů na 8 vzorků, plný rozsah zaručí správné znaménko */
                dst[ret + 0] = ((src[0] & 0xff) << 6) | ((src[1] & 0x03) << 14);
                dst[ret + 1] = ((src[1] & 0xfc) << 4) | ((src[2] & 0x0f) << 12);
                dst[ret + 2] = ((src[2] & 0xf0) << 2) | ((src[3] & 0x3f) << 10);
                dst[ret + 3] = ((src[3] & 0xc0) << 0) | ((src[4] & 0xff) <<  8);
                dst[ret + 4] = ((src[5] & 0xff) << 6) | ((src[6] & 0x03) << 14);
                dst[ret + 5] = ((src[6] & 0xfc) << 4) | ((src[7] & 0x0f) << 12);
                dst[ret + 6] = ((src[7] & 0xf0) << 2) | ((src[8] & 0x3f) << 10);
                dst[ret + 7] = ((src[8] & 0xc0) << 0) | ((src[9] & 0xff) <<  8);

                /* posun vpravo respektuje signed bit */
                switch ((shift >> (2 * k)) & 0x3) {
                case 0:
                    dst[ret + 0]>>= 2;
                    dst[ret + 1]>>= 2;
                    dst[ret + 2]>>= 2;
                    dst[ret + 3]>>= 2;
                    dst[ret + 4]>>= 2;
                    dst[ret + 5]>>= 2;
                    dst[ret + 6]>>= 2;
                    dst[ret + 7]>>= 2;
                    break;
                case 1:
                    dst[ret + 0]>>= 1;
                    dst[ret + 1]>>= 1;
                    dst[ret + 2]>>= 1;
                    dst[ret + 3]>>= 1;
                    dst[ret + 4]>>= 1;
                    dst[ret + 5]>>= 1;
                    dst[ret + 6]>>= 1;
                    dst[ret + 7]>>= 1;
                    break;
                /* 2 = 3 */
                }
            }
        }
    }

    p->addr = addr + 384;

    /* total used bytes */
    return ret * 2;
}
