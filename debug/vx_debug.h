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

#ifndef _OPENVX_INT_DEBUG_H_
#define _OPENVX_INT_DEBUG_H_

#ifdef __cplusplus
#include <csignal>
#else
#include <signal.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <execinfo.h>
#endif

/*!
 * \file
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_int_debug Internal Debugging API
 * \ingroup group_internal
 * \brief The Internal Debugging API
 */

/*! \brief These are the bit flags for debugging.
 * \ingroup group_int_debug
 */
enum vx_debug_zone_e {
    VX_ZONE_ERROR       = 0,    /*!< Used for most errors */
    VX_ZONE_WARNING     = 1,    /*!< Used to warning developers of possible issues */
    VX_ZONE_API         = 2,    /*!< Used to trace API calls and return values */
    VX_ZONE_INFO        = 3,    /*!< Used to show run-time processing debug */

    VX_ZONE_PERF        = 4,    /*!< Used to show performance information */
    VX_ZONE_CONTEXT     = 5,
    VX_ZONE_OSAL        = 6,
    VX_ZONE_REFERENCE   = 7,

    VX_ZONE_ARRAY       = 8,
    VX_ZONE_IMAGE       = 9,
    VX_ZONE_SCALAR      = 10,
    VX_ZONE_KERNEL      = 11,

    VX_ZONE_GRAPH       = 12,
    VX_ZONE_NODE        = 13,
    VX_ZONE_PARAMETER   = 14,
    VX_ZONE_DELAY       = 15,

    VX_ZONE_TARGET      = 16,
    VX_ZONE_LOG         = 17,

    VX_ZONE_MAX         = 32
};

#if defined(_WIN32) && !defined(__GNUC__)
#define VX_PRINT(zone, message, ...) do { vx_print(zone, "[%s:%u] "message, __FUNCTION__, __LINE__, __VA_ARGS__); } while (0)
#else
#define VX_PRINT(zone, message, ...) do { vx_print(zone, "[%s:%u] "message, __FUNCTION__, __LINE__, ## __VA_ARGS__); } while (0)
#endif

/*! \def VX_PRINT
 * \brief The OpenVX Debugging Facility.
 * \ingroup group_int_debug
 */

/*! \brief A debugging macro for entering kernels.
 * \ingroup group_int_debug
 */
#define VX_KERNEL_ENTRY(params, num) { \
    vx_uint32 p; \
    VX_PRINT(VX_ZONE_API, "Entered Kernel! Parameters:\n"); \
    for (p = 0; p < num; p++) { \
        VX_PRINT(VX_ZONE_API, "\tparameter[%u]="VX_FMT_REF"\n", p, params[p]); \
    }\
}

/*! \brief A debugging macro for leaving kernels
 * \ingroup group_int_debug
 */
#define VX_KERNEL_RETURN(status) VX_PRINT(VX_ZONE_API, "returning %d\n", status);

#ifndef DEBUG_BREAK
#if defined(_WIN32) && !defined(__CYGWIN__)
#define DEBUG_BREAK()  do{ *((int *) NULL) = 0;exit(3); }while(0)
#else
#define DEBUG_BREAK()  raise(SIGABRT)
#endif
#endif

#if (defined(__linux__) || defined(__QNX__)) && !defined(__ANDROID__)

#define VX_BACKTRACE(zone) { \
    void *stack[50]; \
    int i, cnt = backtrace(stack, dimof(stack)); \
    char **symbols = backtrace_symbols(stack, cnt); \
    vx_print(zone, "Backtrace[%d]: (%p)\n", cnt, symbols); \
    for (i = 0; i < cnt; i++) { \
        vx_print(zone, "\t[%p] %s\n", stack[i], (symbols ? symbols[i] : NULL)); \
    } \
    free(symbols);\
}

#elif defined(_WIN32) && !defined(__MINGW32__)

#define VX_BACKTRACE(zone) { \
    PVOID stack[50]; \
    USHORT i, cnt = CaptureStackBackTrace(0, dimof(stack), stack, NULL); \
    for (i = 0; i < cnt; i++) { \
        vx_print(zone, "\t[%p]\n", stack[i]); \
    } \
}

#else

#define VX_BACKTRACE(zone)

#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \brief Internal Printing Function.
 * \param [in] zone The debug zone from \ref vx_debug_zone_e.
 * \param [in] format The format string to print.
 * \param [in] ... The variable list of arguments.
 * \ingroup group_int_debug
 */
void vx_print(vx_enum zone, char *format, ...);

/*! \brief Sets a zone bit in the debug mask
 * \param [in] zone The debug zone from \ref vx_debug_zone_e.
 * \ingroup group_int_debug
 */
void vx_set_debug_zone(vx_enum zone);

/*! \brief Clears the zone bit in the mask.
 * \param [in] zone The debug zone from \ref vx_debug_zone_e.
 * \ingroup group_int_debug
 */
void vx_clr_debug_zone(vx_enum zone);

/*! \brief Returns true or false if the zone bit is set or cleared.
 * \param [in] zone The debug zone from \ref vx_debug_zone_e.
 * \ingroup group_int_debug
 */
vx_bool vx_get_debug_zone(vx_enum zone);

/*! \brief Pulls the debug zone mask from the environment variables.
 * \ingroup group_int_debug
 */
void vx_set_debug_zone_from_env(void);

/*!
 * \brief Prints the value of an addressing structure.
 * \param [in] addr
 * \ingroup group_int_image
 */
void ownPrintImageAddressing(const vx_imagepatch_addressing_t *addr);

/*! \brief Prints the values of the images.
 * \ingroup group_int_image
 */
void ownPrintImage(vx_image image);

/*! \brief Prints the name of an object type.
 * \ingroup group_int_debug
 */
const char *ownGetObjectTypeName(vx_enum type);

#ifdef __cplusplus
}
#endif

#endif

