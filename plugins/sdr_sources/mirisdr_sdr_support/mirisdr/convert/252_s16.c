
/*
 * 14 bitový formát
 * 1024 bajtů odpovídá 504 hodnotám
 * struktura:
 *   16b hlavička
 *  1008b bloků obsahujících 504 14b hodnot tvářících se jako 16b
 */
static int mirisdr_samples_convert_252_s16 (mirisdr_dev_t *p, unsigned char* buf, uint8_t *dst8, int cnt) {
    int i, i_max, j, ret = 0;
    uint32_t addr = 0;
    uint8_t *src = buf;
    int16_t *dst = (int16_t*) dst8;

    /* dostáváme 1-3 1024 bytů dlouhé bloky */
    for (i_max = cnt >> 10, i = 0; i < i_max; i++, src+= 1008) {
        /* pozice hlavičky */
        addr = src[3] << 24 | src[2] << 16 | src[1] << 8 | src[0] << 0;

        /* potenciálně ztracená data */
        if ((i == 0) && (addr != p->addr)) {
            fprintf(stderr, "%u samples lost, %d, %08x:%08x\n", addr - p->addr, cnt, p->addr, addr);
        }

        /* přeskočíme hlavičku 16 bitů, 252 I+Q párů */
        for (src+= 16, j = 0; j < 1008; j+= 4, ret+= 2) {
            /* maximální rozsah */
            dst[ret + 0] = (src[j + 0] << 2) | (src[j + 1] << 10);
            dst[ret + 1] = (src[j + 2] << 2) | (src[j + 3] << 10);
        }
    }

    p->addr = addr + 252;

    /* total used bytes */
    return ret * 2;
}
