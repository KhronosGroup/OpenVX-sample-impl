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

#include <VX/vx.h>
#include <vx_debug.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef dimof
#define dimof(x) (sizeof(x)/sizeof(x[0]))
#endif

void vx_print(vx_enum zone, char *format, ...);

static vx_uint32 vx_zone_mask;

#undef  ZONE_BIT
#define ZONE_BIT(zone)  (1 << zone)

void vx_set_debug_zone(vx_enum zone)
{
    if (0 <= zone && zone < VX_ZONE_MAX) {
        vx_zone_mask |= ZONE_BIT(zone);
        vx_print(zone, "Enabled\n");
        //printf("vx_debug_mask=0x%08x\n", vx_debug_mask);
    }
}

void vx_clr_debug_zone(vx_enum zone)
{
    if (0 <= zone && zone < VX_ZONE_MAX) {
        vx_print(zone, "Disabled\n");
        vx_zone_mask &= ~(ZONE_BIT(zone));
        //printf("vx_debug_mask=0x%08x\n", vx_debug_mask);
    }
}

vx_bool vx_get_debug_zone(vx_enum zone)
{
    if (0 <= zone && zone < VX_ZONE_MAX)
        return ((vx_zone_mask & zone)?vx_true_e:vx_false_e);
    else
        return vx_false_e;
}

#define _STR2(x) {#x, x}

struct vx_string_and_enum_e {
    vx_char name[20];
    vx_enum value;
};

struct vx_string_and_enum_e enumnames[] = {
    _STR2(VX_ZONE_ERROR),
    _STR2(VX_ZONE_WARNING),
    _STR2(VX_ZONE_API),
    _STR2(VX_ZONE_INFO),
    _STR2(VX_ZONE_PERF),
    _STR2(VX_ZONE_CONTEXT),
    _STR2(VX_ZONE_OSAL),
    _STR2(VX_ZONE_REFERENCE),
    _STR2(VX_ZONE_ARRAY),
    _STR2(VX_ZONE_IMAGE),
    _STR2(VX_ZONE_SCALAR),
    _STR2(VX_ZONE_KERNEL),
    _STR2(VX_ZONE_GRAPH),
    _STR2(VX_ZONE_NODE),
    _STR2(VX_ZONE_PARAMETER),
    _STR2(VX_ZONE_DELAY),
    _STR2(VX_ZONE_TARGET),
    _STR2(VX_ZONE_LOG),
    {"UNKNOWN", -1}, // if the zone is not found, this will be returned.
};

vx_char *find_zone_name(vx_enum zone)
{
    vx_uint32 i;
    for (i = 0; i < dimof(enumnames); i++)
    {
        if (enumnames[i].value == zone)
        {
            break;
        }
    }
    return enumnames[i].name;
}

static int get_vx_zone_index(const char *name)
{
    vx_uint32 i;
    for (i = 0; i < dimof(enumnames)-1; i++)
    {
        if (strcmp(enumnames[i].name, name) == 0)
        {
            break;
        }
    }
    return enumnames[i].value;
}

void vx_set_debug_zone_from_env(void)
{
    char *str = getenv("VX_ZONE_MASK");
    if (str)
    {
        sscanf(str, "%x", &vx_zone_mask);
    }
    else
    {
        str = getenv("VX_ZONE_LIST");
        if (str)
        {
            vx_char *name = NULL;
#if defined(_WIN32)
            char *buf = _strdup(str);
#else
            char *buf = strdup(str);
#endif
            str = buf;
            do {
                name = strtok(str, ",");
                str = NULL;

                if (name)
                {
                        int zone_id = get_vx_zone_index(name);
                        if (zone_id < 0)
                        {
                            VX_PRINT(VX_ZONE_ERROR, "Invalid log zone name %s", name);
                        }
                        else
                        {
                            vx_set_debug_zone(zone_id);
                        }
                }
            } while (name != NULL);
            free(buf);
        }
    }
    //printf("vx_zone_mask = 0x%08x\n", vx_zone_mask);
}


#if defined(_WIN32) && !defined(__CYGWIN__)

void vx_print(vx_enum zone, char *string, ...)
{
    if (vx_zone_mask & ZONE_BIT(zone))
    {
        char format[1024];
        va_list ap;
        _snprintf(format, sizeof(format), "%20s: %08x: %s", find_zone_name(zone), GetCurrentThreadId(), string);
        format[sizeof(format)-1] = 0; // for MSVC which is not C99 compliant
        va_start(ap, string);
        vprintf(format, ap);
        va_end(ap);
    }
}

#elif defined(__ANDROID__)

#include <android/log.h>

void vx_print(vx_enum zone, char *format, ...)
{
    if (vx_zone_mask & ZONE_BIT(zone))
    {
        char string[1024];
        va_list ap;
        snprintf(string, sizeof(string), "%20s:%s", find_zone_name(zone), format);
        va_start(ap, format);
        __android_log_vprint(ANDROID_LOG_DEBUG, "OpenVX", string, ap);
        va_end(ap);
    }
}

#elif defined(__linux__) || defined(__QNX__) || defined(__APPLE__) || defined(__CYGWIN__)

void vx_print(vx_enum zone, char *format, ...)
{
    if (vx_zone_mask & ZONE_BIT(zone))
    {
        char string[1024];
        va_list ap;
        snprintf(string, sizeof(string), "%20s:%s", find_zone_name(zone), format);
        va_start(ap, format);
        vprintf(string, ap);
        va_end(ap);
    }
}

#endif

void ownPrintImageAddressing(const vx_imagepatch_addressing_t *addr)
{
    if (addr)
    {
        VX_PRINT(VX_ZONE_IMAGE, "addr:%p dim={%u,%u} stride={%d,%d} scale={%u,%u} step={%u,%u}\n",
                addr,
                addr->dim_x, addr->dim_y,
                addr->stride_x, addr->stride_y,
                addr->scale_x, addr->scale_y,
                addr->step_x, addr->step_y);
    }
}

const char *ownGetObjectTypeName(vx_enum type) {
    const char *name = "";

    switch(type) {
    case VX_TYPE_CONTEXT:
        name = "CONTEXT"; break;
    case VX_TYPE_GRAPH:
        name = "GRAPH"; break;
    case VX_TYPE_NODE:
        name = "NODE"; break;
    case VX_TYPE_KERNEL:
        name = "KERNEL"; break;
    case VX_TYPE_PARAMETER:
        name = "PARAMETER"; break;
    case VX_TYPE_DELAY:
        name = "DELAY"; break;
    case VX_TYPE_LUT:
        name = "LUT"; break;
    case VX_TYPE_DISTRIBUTION:
        name = "DISTRIBUTION"; break;
    case VX_TYPE_PYRAMID:
        name = "PYRAMID"; break;
    case VX_TYPE_THRESHOLD:
        name = "THRESHOLD"; break;
    case VX_TYPE_MATRIX:
        name = "MATRIX"; break;
    case VX_TYPE_CONVOLUTION:
        name = "CONVOLUTION"; break;
    case VX_TYPE_SCALAR:
        name = "SCALAR"; break;
    case VX_TYPE_ARRAY:
        name = "ARRAY"; break;
    case VX_TYPE_IMAGE:
        name = "IMAGE"; break;
    case VX_TYPE_REMAP:
        name = "REMAP"; break;
    case VX_TYPE_ERROR:
        name = "<ERROR OBJECT>"; break;
    case VX_TYPE_META_FORMAT:
        name = "META_FORMAT"; break;
    case VX_TYPE_OBJECT_ARRAY:
        name = "OBJECT_ARRAY"; break;
    case VX_TYPE_TENSOR:
        name = "TENSOR"; break;

    default:
        name = "<UNKNOWN TYPE>";
    }

    return name;
}



