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

#include "dsd.h"

void saveAmbe2450Data (dsd_opts * opts, dsd_state * state, char *ambe_d)
{
  int i, j, k;
  unsigned char b, buf[8];
  unsigned char err;

  err = (unsigned char) state->errs2;
  buf[0] = err;

  k = 0;
  for (i = 0; i < 6; i++) {
      b = 0;
      for (j = 0; j < 8; j++) {
          b = b << 1;
          b = b + ambe_d[k];
          k++;
      }
      buf[i+1] = b;
  }
  b = ambe_d[48];
  buf[7] = b;
  write(opts->mbe_out_fd, buf, 8);
}

void saveImbe4400Data (dsd_opts * opts, dsd_state * state, char *imbe_d)
{
  int i, j, k;
  unsigned char b, buf[12];
  unsigned char err;

  err = (unsigned char) state->errs2;
  buf[0] = err;

  k = 0;
  for (i = 0; i < 11; i++) {
      b = 0;
      for (j = 0; j < 8; j++) {
          b = b << 1;
          b = b + imbe_d[k];
          k++;
      }
      buf[i+1] = b;
  }
  write(opts->mbe_out_fd, buf, 12);
}

void
processAudio (dsd_opts * opts, dsd_state * state)
{
  int i, n;
  float aout_abs, max, gainfactor, gaindelta, maxbuf;

  if (opts->audio_gain == 0.0f) {
      // detect max level
      max = 0;
      for (n = 0; n < 160; n++) {
          aout_abs = fabsf (state->audio_out_temp_buf[n]);
          if (aout_abs > max)
              max = aout_abs;
      }
      state->aout_max_buf[state->aout_max_buf_idx++] = max;
      if (state->aout_max_buf_idx > 24) {
          state->aout_max_buf_idx = 0;
      }

      // lookup max history
      for (i = 0; i < 25; i++) {
          maxbuf = state->aout_max_buf[i];
          if (maxbuf > max)
              max = maxbuf;
      }

      // determine optimal gain level
      if (max > 0.0f) {
          gainfactor = (30000.0f / max);
      } else {
          gainfactor = 50.0f;
      }
      if (gainfactor < state->aout_gain) {
          state->aout_gain = gainfactor;
          gaindelta = 0.0f;
      } else {
          if (gainfactor > 50.0f) {
              gainfactor = 50.0f;
          }
          gaindelta = gainfactor - state->aout_gain;
          if (gaindelta > (0.05f * state->aout_gain)) {
              gaindelta = (0.05f * state->aout_gain);
          }
      }
      gaindelta *= 0.00625f;
  } else {
      gaindelta = 0.0f;
  }

  if(opts->audio_gain >= 0) {
      // adjust output gain
      state->audio_out_temp_buf_p = state->audio_out_temp_buf;
      for (n = 0; n < 160; n++) {
          *state->audio_out_temp_buf_p = (state->aout_gain + ((float) n * gaindelta)) * (*state->audio_out_temp_buf_p);
          state->audio_out_temp_buf_p++;
      }
      state->aout_gain += (160.0f * gaindelta);
  }
}

void
writeSynthesizedVoice (dsd_opts * opts, dsd_state * state)
{
  short aout_buf[160];
  unsigned int n;

  state->audio_out_temp_buf_p = state->audio_out_temp_buf;

  for (n = 0; n < 160; n++) {
    if (*state->audio_out_temp_buf_p > 32767.0f) {
        *state->audio_out_temp_buf_p = 32767.0f;
    } else if (*state->audio_out_temp_buf_p < -32767.0f) {
        *state->audio_out_temp_buf_p = -32767.0f;
    }
    aout_buf[n] = (short) lrintf(*state->audio_out_temp_buf_p);
    state->audio_out_temp_buf_p++;
  }

  write(opts->wav_out_fd, aout_buf, 160 * sizeof(int16_t));
}

void demodAmbe3600x24x0Data (int *errs2, char ambe_fr[4][24], char *ambe_d)
{
  int i, j, k;
  unsigned int block = 0, c0_gout = 0, gout = 0, errs = 0;
  unsigned short pr[115], foo;
  char *ambe = ambe_d;

  for (j = 22; j >= 0; j--) {
      block <<= 1;
      block |= ambe_fr[0][j+1];
  }

  Golay23_Correct (&block);
  c0_gout = block;

  block >>= 11;
  block ^= c0_gout;
  for (j = 0; j < 12; j++) {
      if ((block >> j) & 1) {
          errs++;
      }
  }

  // create pseudo-random modulator
  foo = c0_gout;
  pr[0] = (16 * foo);
  for (i = 1; i < 24; i++) {
      pr[i] = (173 * pr[i - 1]) + 13849 - (65536 * (((173 * pr[i - 1]) + 13849) >> 16));
  }
  for (i = 1; i < 24; i++) {
      pr[i] >>= 15;
  }

  // just copy C0
  for (j = 11; j >= 0; j--) {
      *ambe++ = ((c0_gout >> j) & 1);
  }

  // demodulate C1 with pr
  // then, ecc and copy C1
  k = 1;
  for (i = 22; i >= 0; i--) {
      block <<= 1;
      block |= (ambe_fr[1][i] ^ pr[k++]);
  }

  Golay23_Correct (&block);
  gout = block;

  block >>= 11;
  block ^= gout;
  for (j = 0; j < 12; j++) {
      if ((block >> j) & 1) {
          errs++;
      }
  }

  for (j = 11; j >= 0; j--) {
      *ambe++ = ((gout >> j) & 1);
  }

  // just copy C2
  for (j = 10; j >= 0; j--) {
      *ambe++ = ambe_fr[2][j];
  }

  // just copy C3
  for (j = 13; j >= 0; j--) {
      *ambe++ = ambe_fr[3][j];
  }

  *errs2 = errs;
}

void
processAMBEFrame (dsd_opts * opts, dsd_state * state, char ambe_fr[4][24])
{
  char ambe_d[49];

  //state->errs2 = mbe_eccAmbe3600x2450C0 (ambe_fr);
  //mbe_demodulateAmbe3600x2450Data (ambe_fr);
  //state->errs2 += mbe_eccAmbe3600x2450Data (ambe_fr, ambe_d);
  demodAmbe3600x24x0Data (&state->errs2, ambe_fr, ambe_d);
  if (opts->mbe_out_fd != -1) {
      saveAmbe2450Data (opts, state, ambe_d);
  }
  state->debug_audio_errors += state->errs2;

  if (opts->wav_out_fd != -1) {
      if ((state->synctype == 6) || (state->synctype == 7)) {
          mbe_processAmbe2400Dataf (state->audio_out_temp_buf, &state->errs2, state->err_str, ambe_d,
                                    &state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced, opts->uvquality);
      } else {
          mbe_processAmbe2450Dataf (state->audio_out_temp_buf, &state->errs2, state->err_str, ambe_d,
                                    &state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced, opts->uvquality);
      }

      processAudio (opts, state);
      writeSynthesizedVoice (opts, state);
  }
}

void
processIMBEFrame (dsd_opts * opts, dsd_state * state, char imbe_d[88])
{
  if (opts->mbe_out_fd != -1) {
      saveImbe4400Data (opts, state, imbe_d);
  }
  state->debug_audio_errors += state->errs2;

  if (opts->wav_out_fd != -1) {
      mbe_processImbe4400Dataf (state->audio_out_temp_buf, &state->errs2, state->err_str, imbe_d,
                                &state->cur_mp, &state->prev_mp, &state->prev_mp_enhanced, opts->uvquality);
      processAudio (opts, state);
      writeSynthesizedVoice (opts, state);
  }
}

void
closeMbeOutFile (dsd_opts * opts, dsd_state * state)
{
  unsigned int is_imbe = !(state->synctype & ~1U); // Write IMBE magic if synctype == 0 or 1
  char new_path[1024];
  time_t tv_sec;
  struct tm timep;
  int result;

  if (opts->mbe_out_fd != -1) {
      tv_sec = opts->mbe_out_last_timeval;

      close(opts->mbe_out_fd);
      opts->mbe_out_fd = -1;
      gmtime_r(&tv_sec, &timep);
      snprintf (new_path, 1023, "%s/nac0-%04u-%02u-%02u-%02u:%02u:%02u-tg%u-src%u.%cmb", opts->mbe_out_dir,
                timep.tm_year + 1900, timep.tm_mon + 1, timep.tm_mday,
                timep.tm_hour, timep.tm_min, timep.tm_sec, state->talkgroup, state->radio_id, (is_imbe ? 'i' : 'a'));
      result = rename (opts->mbe_out_path, new_path);
  }
}

void
openMbeOutFile (dsd_opts * opts, dsd_state * state)
{
  struct timeval tv;
  unsigned int is_imbe = !(state->synctype & ~1U); // Write IMBE magic if synctype == 0 or 1

  gettimeofday (&tv, NULL);
  opts->mbe_out_last_timeval = tv.tv_sec;
  snprintf (opts->mbe_out_path, 1023, "%s/%ld.%cmb", opts->mbe_out_dir, tv.tv_sec, (is_imbe ? 'i' : 'a'));

  if ((opts->mbe_out_fd = open (opts->mbe_out_path, O_WRONLY | O_CREAT, 0644)) < 0) {
      printf ("Error, couldn't open %s\n", opts->mbe_out_path);
      return;
  }

  // write magic
  if ((state->synctype == 0) || (state->synctype == 1)) {
      write (opts->mbe_out_fd, ".imb", 4);
  } else {
      write (opts->mbe_out_fd, ".amb", 4);
  }
}

