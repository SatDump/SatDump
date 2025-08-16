

#include <cstdint>

typedef struct preludeStructure
{
    unsigned char id[5];      /* Should be ascii CWIF\0                    */
    unsigned char type;       /* 0 = LAC/GAC, 1 = HRPT                     */
    unsigned char fill1[310]; /* Not used                                  */
    int32_t numbits;          /* Number of sync bits checked by formatter  */
    int32_t errbits;          /* Number of error bits counted by formatter */
    unsigned char fill2[8];   /* Not used                                  */
    int32_t startTime;        /* Time of AOS in seconds since 1/1/70 00:00 */
    int32_t stopTime;         /* Time of LOS in seconds since 1/1/70 00:00 */
    int32_t numlines;         /* Number of frames captured by formatter    */
    unsigned char fill3[168]; /* Not used                                  */
} FilePrelude;