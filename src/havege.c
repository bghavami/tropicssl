/*
 *  HAVEGE: HArdware Volatile Entropy Gathering and Expansion
 *
 *  Copyright (C) 2006  Andre Seznec, Olivier Rochecouste
 *
 *  Contact: seznec(at)irisa_dot_fr - orocheco(at)irisa_dot_fr
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License, version 2.1 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */
/*
 *  The HAVEGE RNG was designed by Andre Seznec in 2002.
 *
 *  http://www.irisa.fr/caps/projects/hipsor/publi.php
 */

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <string.h>
#include <stdio.h>
#include <time.h>

#include "timing.h"
#include "havege.h"

/* ------------------------------------------------------------------------
 * On average, one iteration accesses two 8-word blocks in the havege WALK
 * table, and generates 16 words in the RES array.
 *
 * The data read in the WALK table is updated and permuted after each use.
 * The result of the hardware clock counter read is used for this update.
 *
 * 25 conditional tests are present.  The conditional tests are grouped in
 * two nested  groups of 12 conditional tests and 1 test that controls the
 * permutation; on average, there should be 6 tests executed and 3 of them
 * should be mispredicted.
 * ------------------------------------------------------------------------
 */

#define TST1_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;
#define TST2_ENTER if( PTEST & 1 ) { PTEST ^= 3; PTEST >>= 1;

#define TST1_LEAVE U1++; }
#define TST2_LEAVE U2++; }

#define ONE_ITERATION                                   \
                                                        \
    PTEST = PT1 >> 20;                                  \
                                                        \
    TST1_ENTER TST1_ENTER  TST1_ENTER TST1_ENTER        \
    TST1_ENTER TST1_ENTER  TST1_ENTER TST1_ENTER        \
    TST1_ENTER TST1_ENTER  TST1_ENTER TST1_ENTER        \
    TST1_LEAVE TST1_LEAVE  TST1_LEAVE TST1_LEAVE        \
    TST1_LEAVE TST1_LEAVE  TST1_LEAVE TST1_LEAVE        \
    TST1_LEAVE TST1_LEAVE  TST1_LEAVE TST1_LEAVE        \
                                                        \
    PTX = (PT1 >> 18) & 7;                              \
    PT1 &= 0x1FFF;                                      \
    PT2 &= 0x1FFF;                                      \
    CLK = (ulong) hardclock();                          \
                                                        \
    i = 0;                                              \
    A = &WALK[PT1    ]; RES[i++] ^= *A;                 \
    B = &WALK[PT2    ]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 1]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 4]; RES[i++] ^= *D;                 \
                                                        \
    IN = (*A >> (1)) ^ (*A << (31)) ^ CLK;              \
    *A = (*B >> (2)) ^ (*B << (30)) ^ CLK;              \
    *B = IN ^ U1;                                       \
    *C = (*C >> (3)) ^ (*C << (29)) ^ CLK;              \
    *D = (*D >> (4)) ^ (*D << (28)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 2]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 2]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 3]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 6]; RES[i++] ^= *D;                 \
                                                        \
    if( PTEST & 1 ) { ulong *T = A; A = C; C = T; }     \
                                                        \
    IN = (*A >> (5)) ^ (*A << (27)) ^ CLK;              \
    *A = (*B >> (6)) ^ (*B << (26)) ^ CLK;              \
    *B = IN; CLK = (ulong) hardclock();                 \
    *C = (*C >> (7)) ^ (*C << (25)) ^ CLK;              \
    *D = (*D >> (8)) ^ (*D << (24)) ^ CLK;              \
                                                        \
    A = &WALK[PT1 ^ 4];                                 \
    B = &WALK[PT2 ^ 1];                                 \
                                                        \
    PTEST = PT2 >> 1;                                   \
                                                        \
    PT2 = (RES[(i - 8) ^ PTY] ^ WALK[PT2 ^ PTY ^ 7]);   \
    PT2 = ((PT2 & 0x1FFF) & (~8)) ^ ((PT1 ^ 8) & 0x8);  \
    PTY = (PT2 >> 10) & 7;                              \
                                                        \
    TST2_ENTER TST2_ENTER  TST2_ENTER TST2_ENTER        \
    TST2_ENTER TST2_ENTER  TST2_ENTER TST2_ENTER        \
    TST2_ENTER TST2_ENTER  TST2_ENTER TST2_ENTER        \
    TST2_LEAVE TST2_LEAVE  TST2_LEAVE TST2_LEAVE        \
    TST2_LEAVE TST2_LEAVE  TST2_LEAVE TST2_LEAVE        \
    TST2_LEAVE TST2_LEAVE  TST2_LEAVE TST2_LEAVE        \
                                                        \
    C = &WALK[PT1 ^ 5];                                 \
    D = &WALK[PT2 ^ 5];                                 \
                                                        \
    RES[i++] ^= *A;                                     \
    RES[i++] ^= *B;                                     \
    RES[i++] ^= *C;                                     \
    RES[i++] ^= *D;                                     \
                                                        \
    IN = (*A >> ( 9)) ^ (*A << (23)) ^ CLK;             \
    *A = (*B >> (10)) ^ (*B << (22)) ^ CLK;             \
    *B = IN ^ U2;                                       \
    *C = (*C >> (11)) ^ (*C << (21)) ^ CLK;             \
    *D = (*D >> (12)) ^ (*D << (20)) ^ CLK;             \
                                                        \
    A = &WALK[PT1 ^ 6]; RES[i++] ^= *A;                 \
    B = &WALK[PT2 ^ 3]; RES[i++] ^= *B;                 \
    C = &WALK[PT1 ^ 7]; RES[i++] ^= *C;                 \
    D = &WALK[PT2 ^ 7]; RES[i++] ^= *D;                 \
                                                        \
    IN = (*A >> (13)) ^ (*A << (19)) ^ CLK;             \
    *A = (*B >> (14)) ^ (*B << (18)) ^ CLK;             \
    *B = IN;                                            \
    *C = (*C >> (15)) ^ (*C << (17)) ^ CLK;             \
    *D = (*D >> (16)) ^ (*D << (16)) ^ CLK;             \
                                                        \
    PT1 = ( RES[(i - 8) ^ PTX] ^                        \
            WALK[PT1 ^ PTX ^ 7] ) & (~1);               \
    PT1 ^= (PT2 ^ 0x10) & 0x10;                         \
                                                        \
    for( i = 0; i < 16; i++ )                           \
        hs->pool[n] ^= RES[i];                          \
                                                        \
    n = ( n + 1 ) % COLLECT_SIZE;

/* 
 * Entropy gathering phase
 */
void havege_collect_rand( havege_state *hs )
{
    time_t t;
    long int i, n = 0;
    ulong U1,  U2,  *A, *B, *C, *D;
    ulong PT1, PT2, *WALK, RES[16];
    ulong PTX, PTY, CLK, PTEST, IN;

    memset( RES, 0, sizeof( RES ) );

    WALK = hs->WALK;
    PT1  = hs->PT1;
    PT2  = hs->PT2;
    PTX  = U1 = 0;
    PTY  = U2 = 0;

    t = time( NULL );

    while( time( NULL ) - t < COLLECT_TIME )
    {
        /*
         * make sure the internal loop size fits the
         * CPU instruction cache
         */
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION /* 6 */

#if defined HAVE_CPU_UIII || defined HAVE_CPU_PIII || \
    defined HAVE_CPU_PIV  || defined HAVE_CPU_G4   || \
    defined HAVE_CPU_ATHLON

        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION                /* 11 */

#endif
#if defined HAVE_CPU_PIII || defined HAVE_CPU_PIV || \
    defined HAVE_CPU_G4   || defined HAVE_CPU_ATHLON

        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION /* 14 */

#endif
#if defined HAVE_CPU_PIV || defined HAVE_CPU_G4 || \
    defined HAVE_CPU_ATHLON

        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION /* 17 */

#endif
#if defined HAVE_CPU_G4 || defined HAVE_CPU_ATHLON

        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION                /* 22 */

#endif
#if defined HAVE_CPU_ATHLON

        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION
        ONE_ITERATION  ONE_ITERATION  ONE_ITERATION /* 58 */

#endif
    }

    hs->offset = 0;
    hs->PT1 = PT1;
    hs->PT2 = PT2;
}

/*
 * HAVEGE initialization phase
 */
void havege_init( havege_state *hs )
{
    memset( hs, 0, sizeof( havege_state ) );
    havege_collect_rand( hs );
}

/*
 * Returns a random unsigned long.
 */
ulong havege_rand( void *rng_state )
{
    havege_state *hs = rng_state;

    if( hs->offset >= COLLECT_SIZE )
        havege_collect_rand( hs );

    return( hs->pool[hs->offset++] );
}

#ifdef RAND_TEST

#include <time.h>

int main( int argc, char *argv[] )
{
    FILE *f;
    time_t t;
    int i, j, k;
    ulong buf[1024];
    havege_state hs;

    if( argc < 2 )
    {
        fprintf( stderr, "usage: %s <output filename>\n", argv[0] );
        return( 1 );
    }

    if( ( f = fopen( argv[1], "wb+" ) ) == NULL )
    {
        fprintf( stderr, "failed to open '%s' for writing.\n", argv[0] );
        return( 1 );
    }

    havege_init( &hs );

    t = time( NULL );

    for( i = 0, k = 32768 / sizeof( ulong ); i < k; i++ )
    {
        for( j = 0; j < 1024; j++ )
            buf[j] = havege_rand( &hs );

        fwrite( buf, sizeof( buf ), 1, f );

        printf( "Generating 32Mb of data in file '%s'... %04.1f" \
                "%% done\r", argv[1], (100 * (float) (i + 1)) / k );
        fflush( stdout );
    }

    if( t == time( NULL ) )
        t--;

    printf( "Generating 32Mb of data in file '%s'... done (%ld bytes/" \
            "sec)\n", argv[1], ( 32 * 1048576 ) / ( time( NULL ) - t ) );

    fclose( f );
    return( 0 );
}
#endif
