/*

 * Copyright (c) 2012-2017 The Khronos Group Inc.
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
#ifndef _CNN_NETWORK_H_
#define _CNN_NETWORK_H_
//#define EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT

#ifdef  __cplusplus
extern "C" {
#endif

#define Q78_FIXED_POINT_POSITION						8

#include <stdint.h>
#include <VX/vx.h>


float short2float(int16_t val);
int Overfeat(int16_t** ppdata, vx_enum format, vx_int8 fp_pos, bool raw,
		int16_t *output);
int Alexnet(int16_t** ppdata, bool raw, int16_t *output);
int Googlenet2(int16_t** ppdata, vx_enum format, vx_uint8 fp_pos, bool raw, bool immediate, int16_t *output);
//int BDS(int16_t** ppdata, bool raw, bool immediate, int16_t *output);
//int Test();

#ifdef  __cplusplus
}
#endif

#endif //_CNN_NETWORK_H_
