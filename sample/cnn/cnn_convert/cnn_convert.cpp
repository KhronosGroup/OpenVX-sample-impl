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
#include <fstream>
#include <string>
#include <stdint.h>
#include <cmath>
#include <math.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include "windows.h"
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

// Convert from regular decimal into Q1.7.8 fixed point decimal 
int16_t dec2bin(double x)
{
    float x_scaled = x * 256;

    int32_t maxVal = (1 << (16 - 1)) - 1; 

    if (x_scaled > maxVal)
    {
        return maxVal;
    }

    if (x_scaled < -maxVal)
    {
        return -maxVal;
    }

    float rounded_up = ceilf(x_scaled);
    float rounded_down = floorf(x_scaled);

    int16_t rounded = (int16_t)x_scaled;

    if (fabs(rounded_up - x_scaled) < fabs(x_scaled - rounded_down))
    {
        return (int16_t)rounded_up;
    }
    else
    {
        return (int16_t)rounded_down;
    }
}


int main(int argc, char **argv)
{
    FILE *file_in, *file_out;
    int result = 0;
    int counter = 0;
    if (argc < 3)
    {
        goto usage;
    }

    if (strcmp(argv[1], "-bf2short") == 0)
    {
        if (argc != 4)
        {
            printf("Invalid number of arguments");
            result = 1;
            goto exit;
        }

        // Read weights file in float format
        if ((file_in = fopen(argv[2], "rb")) == NULL)
        {
            printf("Can't open input file: %s", argv[2]);
            result = 1;
            goto exit;
        }

        // Write weights to a file in fixed point uint16 format
        if ((file_out = fopen(argv[3], "wb")) == NULL)
        {
            printf("Can't open output file: %s", argv[3]);
            fclose(file_in);
            result = 1;
            goto exit;
        }

        printf("Starting conversion...\n");

        while (!feof(file_in))
        {
            int16_t result;
            float x;
 
            size_t read = fread((void*)(&x), sizeof(x), 1, file_in);
            if (read == 1)
            {
                result = dec2bin(x);
                fwrite((void*)(&result), sizeof(result), 1, file_out);
                counter++;
                if (counter % 1000000 == 0) printf(".");
            }
        }

        printf("\nConversion done!");

        fclose(file_in);
        fclose(file_out);
        goto exit;
    }

usage:
    printf("usage: cnn_convert -bf2short <input file name> <output file name>\n\n");
    printf("input file is a CNN weights file where each weight is a 4 byte floating point\n");
    printf("Press enter to exit\n");
    getchar();

exit:
    return result;
}
