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

#ifndef UTILS_H
#define UTILS_H

#include <pebble.h>

#define DBG(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define INFO(...) APP_LOG(APP_LOG_LEVEL_INFO, __VA_ARGS__)
#define WARN(...) APP_LOG(APP_LOG_LEVEL_WARNING, __VA_ARGS__)
#define ERR(...) APP_LOG(APP_LOG_LEVEL_ERROR, __VA_ARGS__)

/** Convert the fixed point number val of precision deci to a float */
#define FIXP_FLOAT(val, deci) ((float)(val) / (deci))

/**
 * Convert a floating point value to a string
 * The full precision is always used: e.g. 0.00 for .0f at precision 2.
 * The maximal precision EPS is set to .00001
 *
 * @param buf   buffer to write the string to. To be safe the buffer must be
 *              1 (sign) + 10 (int) + 1 (.) + precision + 1 (\0) bytes long.
 *              The buffer is not checked for overflows!
 * @param val   float point value to convert
 * @param precision number of decimals to use
 */
int ftoa(char *buf, float val, int precision);

#endif /* UTILS_H */

