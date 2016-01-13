#ifndef DSD_H
#define DSD_H
/*
 * Copyright (C) 2010 DSD Author
 * GPG Key ID: 0x3F1D7FD0 (74EF 430D F7F2 0A48 FCE6  F630 FAA2 635D 3F1D 7FD0)
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#define __USE_XOPEN
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <math.h>
#include <mbelib.h>
#include "fec.h"
#define SAMPLE_RATE_IN 48000
#define SAMPLE_RATE_OUT 8000
#define FSK4_NTAPS  8
#define FSK4_NSTEPS 128
#define RRC_NZEROS 80
#define RRC_NXZEROS 134

/*
 * global variables
 */
extern int exitflag;

#ifndef FFTCOMPLEX_T_DEFINED
typedef struct FFTComplex {
    float re, im;
} FFTComplex;
#define FFTCOMPLEX_T_DEFINED
#endif

typedef struct _WAVHeader {
    uint32_t pad0;
    uint32_t pad1; 
    uint32_t hdr_len;
    uint16_t wav_id;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t bitrate;
    uint32_t block_align;
    uint32_t data_elem;
    uint32_t data_len;
} __attribute__((packed)) WAVHeader;

typedef struct
{
  int errorbars;
  unsigned int verbose;
  unsigned int datascope;
  int audio_in_fd;
  unsigned char audio_in_type; // 0 for device, 1 for file, 2 for portaudio
  unsigned char audio_in_format; // 0 -> s16, 1 -> float, 2 -> flac, 3 -> u8 i/q pairs at device samplerate, 4 -> same as 3, but also needing a Hilbert transform before lpf/quadrature demod steps.
  time_t mbe_out_last_timeval;
  char mbe_out_dir[1024];
  char mbe_out_path[1024];
  int mbe_out_fd;
  unsigned int serial_baud;
  int serial_fd;
  unsigned char agc_enable;
  float audio_gain;
  unsigned int wav_out_samplerate;
  int wav_out_fd;
  unsigned int uvquality;
  unsigned char frame_types; // 0 -> DMR, 1 -> DStar, 2 -> NXDN48, 3 -> NXDN96, 4 -> P25P1
  unsigned char inverted_dmr;
  unsigned char inverted_x2tdma;
  unsigned int msize;
} dsd_opts;

typedef struct
{
  float xv[RRC_NXZEROS+1];

  float d_history[FSK4_NTAPS+1];
  unsigned int d_history_last;
  float d_symbol_clock;
  float d_symbol_spread;
  float d_symbol_time;
  float fine_frequency_correction;
  float input_gain;

  unsigned char dibit_buf[48000];
  unsigned char *dibit_buf_p;
  FFTComplex prev;
  float inbuffer[4608];
  unsigned int inbuf_pos;

  unsigned int inbuf_size;
  float audio_out_temp_buf[160];
  float *audio_out_temp_buf_p;
  int synctype, lastsynctype;
  int center;
  int min;
  int max;
  int lmid;
  int umid;
  int sbuf[128];
  unsigned int sidx;
  unsigned int ssize;
  int maxbuf[1024];
  int minbuf[1024];
  int midx;
  char ftype[16];
  unsigned int symbolcnt;
  unsigned char rf_mod;
  unsigned char lastp25type;
  int offset;
  unsigned int talkgroup;
  unsigned int radio_id;
  unsigned short nac;
  unsigned char duid;
  int errs2;
  unsigned char p25enc;
  unsigned char firstframe;
  unsigned char dmrMsMode;
  char slot0light[8];
  char slot1light[8];
  float aout_gain;
  float aout_max_buf[25];
  unsigned int aout_max_buf_idx;
  unsigned int samplesPerSymbol;
  unsigned char currentslot;
  mbe_parms cur_mp;
  mbe_parms prev_mp;
  mbe_parms prev_mp_enhanced;

  unsigned int debug_audio_errors;
  unsigned int debug_header_errors;
  unsigned int debug_data_errors;

  unsigned int p25_bit_count;
  unsigned int p25_bit_error_count;

  ReedSolomon ReedSolomon_12_09_04;

  ReedSolomon ReedSolomon_24_12_13;
  ReedSolomon ReedSolomon_36_20_17;
} dsd_state;

/*
 * Frame sync patterns
 */
#define INV_P25P1_SYNC "333331331133111131311111"
#define P25P1_SYNC     "111113113311333313133333"
#define P25P1_TUNING_INVERSE_SYNC 0xFFDF5F55DD55ULL
#define P25P1_TUNING_SYNC 0x5575F5FF77FFULL
#define P25P1_TUNING_M1200 0x001050551155ULL
#define P25P1_TUNING_P1200 0xFFEFAFAAEEAAULL
#define P25P1_TUNING_PM2400 0xAA8A0A008800ULL

#define DMR_BS_DATA_SYNC  "313333111331131131331131"
#define DMR_MS_DATA_SYNC  "311131133313133331131113"
#define DMR_BS_VOICE_SYNC "131111333113313313113313"
#define DMR_MS_VOICE_SYNC "133313311131311113313331"
#define DMR_RC_SYNC "131331111133133133311313"

#define X2TDMA_BS_DATA_SYNC  "331313111113131113331133"
#define X2TDMA_MS_DATA_SYNC  "313113333111111133333313"
#define X2TDMA_BS_VOICE_SYNC "113131333331313331113311"
#define X2TDMA_MS_VOICE_SYNC "131331111333333311111131"

#define DMR_TUNING_BS_DATA_SYNC  0xDFF57D75DF5DULL
#define DMR_TUNING_MS_DATA_SYNC  0xD5D7F77FD757ULL
#define DMR_TUNING_BS_VOICE_SYNC 0x755FD7DF75F7ULL
#define DMR_TUNING_MS_VOICE_SYNC 0x7F7D5DD57DFDULL
#define DMR_TUNING_RC_SYNC       0x77D55F7DFD77ULL

#define X2TDMA_TUNING_BS_DATA_SYNC 0xF77557757F5FULL
#define X2TDMA_TUNING_MS_DATA_SYNC 0xDD7FD555FFF7ULL
#define X2TDMA_TUNING_BS_VOICE_SYNC 0x5DDFFDDFD5F5ULL
#define X2TDMA_TUNING_MS_VOICE_SYNC 0x77D57FFF555DULL

// Conventional
#define NXDN_BS_DATA_SYNC      "313133113131111313"
#define NXDN_MS_DATA_SYNC      "313133113131111333"
#define NXDN_BS_VOICE_SYNC     "313133113131113113"
#define NXDN_MS_VOICE_SYNC     "313133113131113133"
#define INV_NXDN_BS_DATA_SYNC  "131311331313333131"
#define INV_NXDN_MS_DATA_SYNC  "131311331313333111"
#define INV_NXDN_BS_VOICE_SYNC "131311331313331331"
#define INV_NXDN_MS_VOICE_SYNC "131311331313331311"

// Trunked
#define NXDN_TC_VOICE_SYNC     "313133113113113113"
#define NXDN_TD_VOICE_SYNC     "313133113133113111"
#define NXDN_TC_CC_SYNC     "313133113111311313"
#define NXDN_TD_CC_SYNC     "313133113133311113"
#define INV_NXDN_TC_VOICE_SYNC "131311331331331331"
#define INV_NXDN_TD_VOICE_SYNC "131311331311331333"
#define INV_NXDN_TC_CC_SYNC "131311331333133131"
#define INV_NXDN_TD_CC_SYNC "131311331311133331"

#define NXDN_TUNING_BS_DATA_SYNC  0xDDF5DD577
#define NXDN_TUNING_MS_DATA_SYNC  0xDDF5DD57F
#define NXDN_TUNING_TC_CC_SYNC    0xDDF5D5D77
#define NXDN_TUNING_TD_CC_SYNC    0xDDF5DFD57
#define NXDN_TUNING_BS_VOICE_SYNC 0xDDF5DD5D7
#define NXDN_TUNING_MS_VOICE_SYNC 0xDDF5DD5DF
#define NXDN_TUNING_TC_VOICE_SYNC 0xDDF5D75D7
#define NXDN_TUNING_TD_VOICE_SYNC 0xDDF5DF5D5
#define INV_NXDN_TUNING_BS_DATA_SYNC  0x775F77FDD
#define INV_NXDN_TUNING_MS_DATA_SYNC  0x775F77FD5
#define INV_NXDN_TUNING_TC_CC_SYNC    0x775F7F7DD
#define INV_NXDN_TUNING_TD_CC_SYNC    0x775F757FD
#define INV_NXDN_TUNING_BS_VOICE_SYNC 0x775F77F7D
#define INV_NXDN_TUNING_MS_VOICE_SYNC 0x775F77F75
#define INV_NXDN_TUNING_TC_VOICE_SYNC 0x775F7DF7D
#define INV_NXDN_TUNING_TD_VOICE_SYNC 0x775F75F7F

#define DSTAR_HD_SYNC        "131313131333133113131111"
#define DSTAR_SYNC           "313131313133131113313111"
#define INV_DSTAR_HD_SYNC    "313131313111311331313333"
#define INV_DSTAR_SYNC       "131313131311313331131333"

#define PROVOICE_SYNC        "13131333111311311133113311331133"
#define PROVOICE_EA_SYNC     "31131311331331111133131311311133"
#define INV_PROVOICE_SYNC    "31313111333133133311331133113311"
#define INV_PROVOICE_EA_SYNC "13313133113113333311313133133311"

#define DSTAR_TUNING_HD_SYNC 0x77777F7D7755ULL
#define DSTAR_TUNING_SYNC 0xDDDDDF757DD5ULL
#define INV_DSTAR_TUNING_HD_SYNC 0xDDDDD5D7DDFFULL
#define INV_DSTAR_TUNING_SYNC 0x777775DFD77FULL

/*
 * Inline functions
 */
static inline unsigned int
get_uint(unsigned char *payload, unsigned int bits)
{
    unsigned int i, v = 0;
    for(i = 0; i < bits; i++) {
        v <<= 1;
        v |= payload[i];
    }
    return v;
}

static inline unsigned int Golay23_CorrectAndGetErrCount(unsigned int *block)
{
  unsigned int i, errs = 0, in = *block;

  Golay23_Correct (block);

  in >>= 11;
  in ^= *block;
  for (i = 0; i < 12; i++) {
      if ((in >> i) & 1) {
          errs++;
      }
  }

  return (errs);
}

/*
 * Function prototypes 
 */
unsigned int getDibit (dsd_opts * opts, dsd_state * state);
void skipDibit (dsd_opts * opts, dsd_state * state, int count);
void Shellsort_int(int *in, unsigned int n);

int openAudioInDevice (dsd_opts *opts, const char *audio_in_dev);
void demodAmbe3600x24x0Data (int *errs2, char ambe_fr[4][24], char ambe_d[49]);
void saveAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d);
void closeMbeOutFile (dsd_opts * opts, dsd_state * state);
void openMbeOutFile (dsd_opts * opts, dsd_state * state);
void openWavOutFile (dsd_opts *opts, const char *wav_out_file);
void closeWavOutFile (dsd_opts *opts);
void processFrame (dsd_opts * opts, dsd_state * state);
void printFrameSync (dsd_opts * opts, dsd_state * state, char *frametype, int offset);
int getFrameSync (dsd_opts * opts, dsd_state * state);
void print_datascope(dsd_state *state, int *lbuf2, unsigned int lsize);
void noCarrier (dsd_opts * opts, dsd_state * state);
void cleanupAndExit (dsd_opts * opts, dsd_state * state);
void sigfun (int sig);
void processAMBEFrame (dsd_opts * opts, dsd_state * state, unsigned char ambe_dibits[36]);
void processIMBEFrame (dsd_opts * opts, dsd_state * state, char imbe_d[88]);
void process_IMBE (dsd_opts* opts, dsd_state* state, unsigned char imbe_dibits[72]);
int getSymbol (dsd_opts * opts, dsd_state * state, int have_sync);
float dsd_gen_root_raised_cosine(float sampling_freq, float symbol_rate, float alpha);
void processEmb (dsd_state *state, unsigned char lcss, unsigned char emb_fr[4][32]);
void processDMRdata (dsd_opts * opts, dsd_state * state);
unsigned int processDMRvoice (dsd_opts * opts, dsd_state * state);
unsigned int processNXDNVoice (dsd_opts * opts, dsd_state * state);
unsigned int processX2TDMAvoice (dsd_opts * opts, dsd_state * state);
unsigned int processDSTAR (dsd_opts * opts, dsd_state * state);
unsigned int processDSTAR_HD (dsd_opts * opts, dsd_state * state);
int bchDec(uint64_t cw, uint16_t *cw_decoded);
uint64_t bchEnc(uint16_t in_word);
void process_p25_frame(dsd_opts *opts, dsd_state *state, char *tmpStr, unsigned int tmpLen);
float get_p25_ber_estimate (dsd_state* state);
float dmr_filter(dsd_state *state, float sample);
unsigned int decode_p25_lcf(unsigned int lcinfo[3], char *strbuf);
unsigned int dsd_div32(unsigned int num, unsigned int den, unsigned int *rem);

void ReedSolomon_36_20_17_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity);
int ReedSolomon_36_20_17_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity);
void ReedSolomon_24_12_13_encode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* out_hex_parity);
int ReedSolomon_24_12_13_decode(ReedSolomon *rs, unsigned char* hex_data, unsigned char* hex_parity);

#endif // DSD_H
