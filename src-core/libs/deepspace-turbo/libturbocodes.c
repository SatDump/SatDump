//
// Created by gianluca on 22/02/17.
//

#include "libturbocodes.h"
//#include "utilities.h"
#include <stdlib.h>
#include <math.h>


static int *turbo_interleave(int *packet, t_turbocode code)
{
    int *interleaved_packet = malloc(code.packet_length * sizeof(int));// {{{
    for (int j = 0; j < code.packet_length; ++j) {
        interleaved_packet[j] = packet[code.interleaver[j]];
    }

    return interleaved_packet;// }}}
}

static int *turbo_deinterleave(int *packet, t_turbocode code)
{
    int *local = malloc(code.packet_length*sizeof(int));// {{{
    for (int i = 0; i < code.packet_length; ++i) {
        local[code.interleaver[i]] = packet[i];
    }

    return local;// }}}
}

static void message_interleave(double ***messages, t_turbocode code)
{
    // local array// {{{
    double **local = malloc(2*sizeof(double*));
    local[0] = malloc(code.packet_length * sizeof(double));
    local[1] = malloc(code.packet_length * sizeof(double));

    for (int i = 0; i < code.packet_length; ++i) {
        local[0][i] = (*messages)[0][code.interleaver[i]];
        local[1][i] = (*messages)[1][code.interleaver[i]];
    }

    for (int i = 0; i < code.packet_length; ++i) {
        (*messages)[0][i] = local[0][i];
        (*messages)[1][i] = local[1][i];
    }

    free(local[0]);
    free(local[1]);
    free(local);// }}}
}

static void message_deinterleave(double ***messages, t_turbocode code)
{
     // local array// {{{
    double **local = malloc(2*sizeof(double*));
    local[0] = malloc(code.packet_length * sizeof(double));
    local[1] = malloc(code.packet_length * sizeof(double));

    for (int i = 0; i < code.packet_length; ++i) {
        local[0][code.interleaver[i]] = (*messages)[0][i];
        local[1][code.interleaver[i]] = (*messages)[1][i];
    }

    for (int i = 0; i < code.packet_length; ++i) {
        (*messages)[0][i] = local[0][i];
        (*messages)[1][i] = local[1][i];
    }

    free(local[0]);
    free(local[1]);
    free(local);// }}}
}


t_turbocode turbo_initialize(t_convcode upper, t_convcode lower, int *interleaver, int packet_length)
{
    t_turbocode code;/*{{{*/
    code.upper_code = upper;
    code.lower_code = lower;

    code.packet_length = packet_length;
    code.interleaver = interleaver;

    // compute encoded length
    int turbo_length = 0;
    turbo_length += upper.components * (code.packet_length + upper.memory);
    turbo_length += lower.components * (code.packet_length + lower.memory);

    code.encoded_length = turbo_length;
    return code;/*}}}*/
}

int *turbo_encode(int *packet, t_turbocode code)
{
    int *interleaved_packet = turbo_interleave(packet, code);/*{{{*/

    // reference to encoded messages
    int **conv_encoded = malloc(2 * sizeof(int*));
    int turbo_length = code.encoded_length;
    conv_encoded[0] = convcode_encode(packet, code.packet_length, code.upper_code);
    conv_encoded[1] = convcode_encode(interleaved_packet, code.packet_length, code.lower_code);

    int *turbo_encoded = malloc(turbo_length * sizeof *turbo_encoded);

    t_convcode codes[2] = {code.upper_code, code.lower_code};
    // parallel to serial

    int k = 0, c = 0, cw = 0;/*{{{*/
    while (k < turbo_length) {
        t_convcode cc = codes[c];

        // number of components of cc
        int comps = cc.components;

        // copy bits from cc output to turbo_encoded
        for (int i = 0; i < comps; i++) {
            int bit = conv_encoded[c][cw*comps + i];
            turbo_encoded[k++] = bit;
        }

        c = (c + 1) % 2;
        // when c = 0 the first codeword is complete
        cw = !c  ? cw+1 : cw;
    }/*}}}*/

    free(conv_encoded[0]);
    free(conv_encoded[1]);
    free(conv_encoded);

    free(interleaved_packet);
    

    return turbo_encoded;/*}}}*/
}

int* turbo_decode(double *received, int iterations, double noise_variance, t_turbocode code)
{
    // serial to parallel/*{{{*/
    int *lengths = malloc(2 * sizeof  *lengths);/*{{{*/
    double **streams = malloc(2 * sizeof(double*));
    t_convcode codes[2] = {code.upper_code, code.lower_code};
    for (int i = 0; i < 2; i++) {
        t_convcode cc = codes[i];
        lengths[i] = cc.components * (code.packet_length + cc.memory);
        streams[i] = malloc(lengths[i] * sizeof(double));
    }

    int k = 0, c = 0, cw = 0;
    while (k < code.encoded_length) {
        t_convcode cc = codes[c];

        for (int i = 0; i < cc.components; i++)
           streams[c][cw*cc.components + i] = received[k++];

        c = (c + 1) % 2;
        cw = !c ? cw + 1 : cw;
    }/*}}}*/

    // initial messages
    double **messages = malloc(2 * sizeof(double *));
    for (int i = 0; i < 2; i++) {
        messages[i] = malloc(code.packet_length * sizeof(double));
        for (int j = 0; j < code.packet_length; j++) {
            messages[i][j] = log(0.5);
        }
    }

    int *turbo_decoded   = NULL;
    int *turbo_decoded_1 = NULL;
    for (int i = 0; i < iterations; i++) {

        // run BCJR on upper code
        turbo_decoded_1 = convcode_extrinsic(streams[0], lengths[0], &messages, code.upper_code, noise_variance, 0);

        // apply interleaver
        message_interleave(&messages, code);

        // run BCJR on lower code
        turbo_decoded = convcode_extrinsic(streams[1], lengths[1], &messages, code.lower_code, noise_variance, i == (iterations - 1));

        // deinterleave
        message_deinterleave(&messages, code);
    }

    int *decoded_deinterleaved = turbo_deinterleave(turbo_decoded, code);
    //decoded_deinterleaved = turbo_deinterleave(turbo_decoded, code);

    for (int i = 0; i < 2; i++)
        free(streams[i]);
    free(streams);
    free(turbo_decoded);
    free(turbo_decoded_1);
    free(lengths);
    free(messages[0]);
    free(messages[1]);
    free(messages);

    //length of the 
    return decoded_deinterleaved; /*}}}*/
}
