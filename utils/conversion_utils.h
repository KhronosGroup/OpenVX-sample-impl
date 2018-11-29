/*
 * Copyright (c) 2016-2017 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef UTILS_CONVERSION_UTILS_H_
#define UTILS_CONVERSION_UTILS_H_

float short2float(int16_t val);
int16_t float2short(double x);
int32_t conversion_16_8(int32_t prod);
int32_t conversion_24_8_i(int32_t acc_value);
int16_t conversion_24_8(int32_t acc_value);
vx_int16 QUANTIZE(double x);
double UNQUANTIZE(vx_int16 val);

#endif /* UTILS_CONVERSION_UTILS_H_ */
