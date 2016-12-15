/*
 * Copyright (c) 2016, Sensirion AG
 * Author: Andreas Brauchli <andreas.brauchli@sensirion.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "utils.h"

#define EPS .00001

int ftoa(char *buf, float val, int precision) {
  char *start = buf;
  // start with positive/negative
  if (val < 0) {
    *(buf++) = '-';
    val = -val;
  }
  // integer value
  float round = .5f;
  for (int i = 0; i < precision; ++i)
    round *= 0.1;
  val += round;
  buf += snprintf(buf, 10, "%d", (int)val);
  // decimals
  if (precision > 0) {
    val -= (int) val;
    *(buf++) = '.';
    for (int i = 0; i < precision; ++i) {
      if (val > EPS) {
        val *= 10;
        *(buf++) = '0' + (int) val;
        val -= (int) val;
      } else {
        *(buf++) = '0';
      }
    }
  }
  *buf = '\0';
  return buf - start;
}

