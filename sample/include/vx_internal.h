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

#ifndef _VX_INTERNAL_H_
#define _VX_INTERNAL_H_

/*!
 * \file vx_internal.h
 * \brief The internal implementation header.
 * \author Erik Rainey <erik.rainey@gmail.com>
 *
 * \defgroup group_internal OpenVX Sample Implementation
 * \brief The Sample OpenVX Implementation.
 * \details A free, Open Source implementation of the OpenVX implementation that
 * vendors may use as a basis of developing their own version of OpenVX.
 *
 * \defgroup group_int_types Internal Types
 * \ingroup group_internal
 * \brief Internal types, typedefs and structures.
 *
 * \defgroup group_int_macros Internal Macros
 * \ingroup group_internal
 * \brief Internal Macros, and define's
 *
 * \defgroup group_int_defines Internal Defines
 * \ingroup group_internal
 * \brief Limitations on Sizes, Ranges, Values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#endif
#if defined(__linux__) || defined(__ANDROID__) || defined(__QNX__) || defined(__APPLE__) || defined(__CYGWIN__)
#include <dlfcn.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#elif defined(_WIN32) || defined(UNDER_CE)
#include <windows.h>
#endif

#include <VX/vx.h>
/* TODO: remove vx_compatibility.h after transition period */
#include <VX/vx_compatibility.h>
#if defined(OPENVX_USE_TILING)
#define OPENVX_TILING_1_0
#include <VX/vx_khr_tiling.h>
#endif
#if defined(EXPERIMENTAL_USE_DOT)
#include <VX/vx_khr_dot.h>
#endif
#if defined(OPENVX_USE_XML)
#include <VX/vx_khr_xml.h>
#endif
#include <VX/vx_lib_extras.h>
#if defined(OPENVX_USE_IX)
#include <VX/vx_khr_ix.h>
#endif
#ifdef OPENVX_USE_OPENCL_INTEROP
#include <VX/vx_khr_opencl_interop.h>
#endif
#define VX_MAX_TENSOR_DIMENSIONS 6
#define Q78_FIXED_POINT_POSITION 8

#define VX_MAX_TENSOR_DIMENSIONS 6
#define Q78_FIXED_POINT_POSITION 8
/*! \def VX_INT_API Used to deliniate APIs which are not intended to be exported.
 * \ingroup group_int_defines
 */
#if defined(__GNUC__)
#define VX_INT_API __attribute__((visibility("hidden")))
#else
#define VX_INT_API
#endif

#ifndef dimof
/*! \brief Get the dimensionality of the array.
 * \details If not defined by the platform, this allows client to retrieve the
 * dimensionality of an array of fixed sized units.
 * \ingroup group_int_macros
 */
#define dimof(x)    (sizeof(x)/sizeof(x[0]))
#endif

/*! \brief Maximum number of characters in a path string.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_PATH     (256)

/*! \brief Defines the maximum number of characters in a target string.
 * \ingroup group_target
 */
#define VX_MAX_TARGET_NAME (64)

#ifndef VX_MAX_STRUCT_NAME
#define VX_MAX_STRUCT_NAME (64)
#endif

/*! \brief Maximum number of nodes in a graph.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_NODES    (256)

/*! \brief Maximum number of references in the context.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_REF      (4096)

/*! \brief Maximum number of user defined structs/
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_USER_STRUCTS (1024)

/*! \brief Maximum number of kernel in the context.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_KERNELS  (1024)

/*! \brief Maximum number of parameters to a kernel.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_PARAMS   (15)

/*! \brief Maximum number of loadable modules.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_MODULES  (10)

/*! \brief The largest convolution matrix the specification requires support for is 15x15.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_CONVOLUTION_DIM (15)

/*! \brief The largest nonlinear filter matrix the specification requires support for is 9x9.
* \ingroup group_int_defines
*/
#define VX_INT_MAX_NONLINEAR_DIM (9)

/*! \brief A magic value to look for and set in references.
 * \ingroup group_int_defines
 */
#define VX_MAGIC            (0xFACEC0DE)

/*! \brief Maximum queue depth.
 * \ingroup group_int_defines
 */
#define VX_INT_MAX_QUEUE_DEPTH (32)

/*! \brief The value to use in event waiting which never returns.
 * \ingroup group_int_defines
 */
#define VX_INT_FOREVER          (0xFFFFFFFF)

/*! \brief The minimum khronos number of targets.
 * \ingroup group_int_defines
 */
#define VX_INT_HOST_CORES (TARGET_NUM_CORES)

/*! \brief The largest optical flow pyr LK window.
 * \ingroup group_int_defines
 */
#define VX_OPTICALFLOWPYRLK_MAX_DIM (9)

/*! \brief Used to determine is a type is an image.
 * \ingroup group_int_macros
 */
#define VX_TYPE_IS_IMAGE(type)  (ownIsSupportedFourcc(type) == vx_true_e)

/*! \brief Used to determine if a type is a scalar.
 * \ingroup group_int_macros
 */
#define VX_TYPE_IS_SCALAR(type) (VX_TYPE_INVALID < (type) && (type) < VX_TYPE_SCALAR_MAX)

/*! \brief Used to determine if a type is a struct.
 * \ingroup group_int_macros
 */
#define VX_TYPE_IS_STRUCT(type) ((type) >= VX_TYPE_RECTANGLE && (type) < VX_TYPE_VENDOR_STRUCT_END)

/*! \brief Used to determine if a type is a data object.
 * \ingroup group_int_macros
 */
#define VX_TYPE_IS_DATA_OBJECT(type) ( (((type) >= VX_TYPE_LUT) && ((type) <= VX_TYPE_REMAP)) || \
                                       (((type) >= VX_TYPE_OBJECT_ARRAY) && ((type) <= VX_TYPE_TENSOR)) )

/*! \brief Used to determine if a type is an object.
 * \ingroup group_int_macros
 */
#define VX_TYPE_IS_OBJECT(type) ((type) >= VX_TYPE_REFERENCE && (type) < VX_TYPE_VENDOR_OBJECT_END)

/*! A parameter checker for size and alignment.
 * \ingroup group_int_macros
 */
#define VX_CHECK_PARAM(ptr, size, type, align) (size == sizeof(type) && ((vx_size)ptr & align) == 0)

/*! Convenience wrapper around calloc to cast it correctly
 * \ingroup group_int_macros
 */
#define VX_CALLOC(type) (type *)calloc(1, sizeof(type))


#define VX_STRINGERIZE(x)   x, #x

#if defined(_WIN32) && !defined(__GNUC__)
#define VX_INLINE _inline
//#define VX_FMT_TIME   "%I64d"    // Show the perf stats in seconds.
#define VX_FMT_TIME   "%.3Lf"    // Show the perf stats in milliseconds.
#else
#define VX_INLINE inline
#if (defined(__x86_64) || defined(__amd64)) && !defined(__APPLE__) // 64 bit
//#define VX_FMT_TIME   "%lu"      // Show the perf stats in seconds.
#define VX_FMT_TIME   "%.3f"     // Show the perf stats in milliseconds.
#else
//#define VX_FMT_TIME    "%llu"    // Show the perf stats in seconds.
#define VX_FMT_TIME   "%.3f"     // Show the perf stats in milliseconds.
#endif
#endif

#define VX_MIN(a, b) ((a) < (b) ? (a) : (b))
#define VX_MAX(a, b) ((a) > (b) ? (a) : (b))

/*! A convenience typedef for void pointers.
 * \ingroup group_int_types
 */
typedef void *vx_ptr_t;

/*! \brief Used to print out the value of a value. */
#define VX_FMT_VALUE VX_FMT_SIZE

/*! A thread return value.
 * \ingroup group_int_osal
 */
typedef vx_size vx_value_t;

/*! A thread function pointer.
 * \ingroup group_int_osal
 */
typedef vx_value_t (*vx_thread_f)(void *arg);

#if defined(__linux__) || defined(__ANDROID__) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__QNX__)
/*! A POSIX module handle.
 * \ingroup group_int_osal
 */
typedef void *vx_module_handle_t;
/*! A POSIX symbol handle.
 * \ingroup group_int_osal
 */
typedef void *vx_symbol_t;

/*! An initial value for a module */
#define VX_MODULE_INIT (NULL)

#if defined(__APPLE__)
/*! \brief The module name macro.
 * \ingroup group_int_osal
 */
#define VX_MODULE_NAME(name)    "lib"name".dylib"
#elif defined(__CYGWIN__)
/*! \brief The module name macro.
 * \ingroup group_int_osal
 */
#define VX_MODULE_NAME(name)   "lib"name".dll.a"
#elif defined(__QNX__) || defined(__linux__) || defined(__ANDROID__)
/*! \brief The module name macro.
 * \ingroup group_int_osal
 */
#define VX_MODULE_NAME(name)   "lib"name".so"
#endif
#if defined(__APPLE__)
#define VX_PTHREAD_SEMAPHORE
/*! A MacOSX semaphore wrapper.
 * \ingroup group_int_osal
 */
typedef struct _vx_sem_t
{
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    int count;
} vx_sem_t;
#else
/*! A POSIX semaphore wrapper
 * \ingroup group_int_osal
 */
typedef sem_t vx_sem_t;
#endif
/*! A POSIX thread
 * \ingroup group_int_osal
 */
typedef pthread_t vx_thread_t;
/*! A POSIX thread function def
 * \ingroup group_int_osal
 */
typedef void * (*pthread_f )(void *);
/*! A POSIX event type
 * \ingroup group_int_osal
 */
typedef struct _vx_event_t {
    vx_bool             autoreset;   /*!< Indicates whether the event will auto-reset after signalling */
    vx_bool             set;         /*!< The current event value */
    pthread_cond_t     cond;        /*!< The PThread Condition */
    pthread_condattr_t attr;        /*!< The PThread Condition Attribute */
    pthread_mutex_t    mutex;       /*!< The PThread Mutex */
} vx_event_t;

#define FILE_JOINER "/"
#elif defined(_WIN32) || defined(UNDER_CE)
/*! A Windows specific module handle.
 * \ingroup group_int_osal
 */
typedef HMODULE vx_module_handle_t;
/*! A Windows specific symbol handle.
 * \ingroup group_int_osal
 */
typedef HANDLE  vx_symbol_t;

/*! An initial value for a module */
#define VX_MODULE_INIT (0)

#define VX_MODULE_NAME(name)   ""name".dll"
/*! A Windows specific semaphore wrapper.
 * \ingroup group_int_osal
 */
typedef HANDLE vx_sem_t;
/*! A Windows specific thread handle.
 * \ingroup group_int_osal
 */
typedef HANDLE vx_thread_t;
/*! A Windows specific event handle.
 * \ingroup group_int_osal
 */
typedef HANDLE vx_event_t;

#define FILE_JOINER "\\"
#endif

/*! \brief The data object for queues.
 * \ingroup group_int_osal
 */
typedef struct _vx_value_set_t {
    vx_value_t v1;
    vx_value_t v2;
    vx_value_t v3;
} vx_value_set_t;

/*! \brief The queue object.
 * \ingroup group_int_osal
 */
typedef struct _vx_queue_t {
    vx_value_set_t *data[VX_INT_MAX_QUEUE_DEPTH];
    vx_int32 start_index;
    vx_int32 end_index;
    vx_sem_t lock;
    vx_event_t readEvent;
    vx_event_t writeEvent;
    vx_bool popped;
} vx_queue_t;

/*! \brief The processor structure which contains the graph queue.
 * \ingroup group_int_context
 */
typedef struct _vx_processor_t {
    vx_queue_t input;
    vx_queue_t output;
    vx_thread_t thread;
    vx_bool running;
} vx_processor_t;

// forward declarations
struct _vx_threadpool_t;
struct _vx_threadpool_worker_t;
typedef struct _vx_target *vx_target;

/*! \brief The function pointer to the worker function.
 * \param [in] worker The per-thread worker data structure.
 * \retval false_e Indicates that the worker failed to process data or had some other
 * error.
 * \ingroup group_threadpools
 */
typedef vx_bool (*vx_threadpool_f)(struct _vx_threadpool_worker_t *worker);

/*! \brief The structure given to each threadpool worker during execution.
 * \ingroup group_int_osal
 */
typedef struct _vx_threadpool_worker_t {
    /*! \brief The work queue */
    vx_queue_t *queue;
    /*! \brief The handle to the worker thread */
    vx_thread_t handle;
    /*! \brief The index of this worker in the pool */
    uint32_t index;
    /*! \brief Indicates whether this worker is currently operating. */
    vx_bool active;
    /*! \brief The worker function */
    vx_threadpool_f function;
    /*! \brief The user argument to the thread */
    void *arg;
    /*! \brief The data information from the client */
    vx_value_set_t *data;
    /*! \brief Pointer to the top level structure. */
    struct _vx_threadpool_t *pool;
    /*! \brief Performance capture variable. */
    vx_perf_t perf;
} vx_threadpool_worker_t;

/*! \brief The threadpool tracking structure
 * \ingroup group_int_osal
 */
typedef struct _vx_threadpool_t {
    /*! \brief The number of threads in the pool */
    uint32_t numWorkers;
    /*! \brief The maximum number of threads in the queue */
    uint32_t numWorkItems;
    /*! \brief Unit size of a work item */
    uint32_t sizeWorkItem;
    /*! \brief The number of corrent items in the queue */
     int32_t numCurrentItems;
    /*! \brief The array of workers */
    vx_threadpool_worker_t *workers;
    /*! \brief The next index to submit work to */
    uint32_t nextWorkerIndex;
    /*! \brief The semaphore which protect access to the work queues */
    vx_sem_t sem;
    /*! \brief The event which indicates that all work is completed */
    vx_event_t completed;
} vx_threadpool_t;

/*! \brief The work item to distribute across the threadpools
 * \ingroup group_int_osal
 */
typedef struct _vx_work_t {
    /*! \brief The target to execute on */
    vx_target target;
    /*! \brief The node to execute */
    vx_node node;
    /*! \brief The resulting action */
    vx_enum action;
} vx_work_t;

/*! \brief An internal enum for notating which sort of reference type we need.
 * \ingroup group_int_type
 */
typedef enum _vx_reftype_e {
    VX_INTERNAL = 1,
    VX_EXTERNAL = 2,
    VX_BOTH = 3,
} vx_reftype_e;

/*! \brief The most basic type in the OpenVX system. Any type that inherits
 *  from vx_reference_t must have a vx_reference_t as its first memeber
 *  to allow casting to this type.
 * \ingroup group_int_refererence
 */
typedef struct _vx_reference {
#if !DISABLE_ICD_COMPATIBILITY
    /*! \brief Platform for ICD compatibility. */
    struct _vx_platform * platform;
#endif
    /*! \brief Used to validate references, must be set to VX_MAGIC. */
    vx_uint32 magic;
    /*! \brief Set to an enum value in \ref vx_type_e. */
    vx_enum type;
    /*! \brief Pointer to the top level context.
     * If this reference is the context, this will be NULL.
     */
    vx_context context;
    /*! \brief The pointer to the object's scope parent. When virtual objects
     * are scoped within a graph, this will point to that parent graph. This is
     * left generic to allow future scoping variations. By default scope should
     * be the same as context.
     */
    vx_reference scope;
    /*! \brief The count of the number of users with this reference. When
     * greater than 0, this can not be freed. When zero, the value can be
     * considered inaccessible.
     */
    vx_uint32 external_count;
    /*! \brief The count of the number of framework references. When
     * greater than 0, this can not be freed.
     */
    vx_uint32 internal_count;
    /*! \brief The number of times the object has been read (in some portion) */
    vx_uint32 read_count;
    /*! \brief The number of times the object has been written to (in some portion) */
    vx_uint32 write_count;
    /*! \brief The reference lock which is used to protect access to "in-fly" data. */
    vx_sem_t lock;
    /*! \brief A reserved field which can be used to store anonymous data */
    void *reserved;
    /*! \brief A field which can be used to store a temporary, per-graph index. */
    vx_uint32 index;
    /*! \brief This indicates if the object was extracted from another object */
    vx_bool extracted;
    /*! \brief This indicates if the object is virtual or not */
    vx_bool is_virtual;
    /* \brief This indicates if the object belongs to a delay */
    struct _vx_delay *delay;
    /* \brief This indicates the original delay slot index when the object belongs to a delay */
    vx_int32 delay_slot_index;
    /*! \brief This indicates that if the object is virtual whether it is accessible at the moment or not */
    vx_bool is_accessible;
    /*! \brief The reference name */
    char name[VX_MAX_REFERENCE_NAME];
} vx_reference_t;


/*! \brief The internal representation of the error object.
 * \ingroup group_int_error
 */
typedef struct _vx_error {
    /*! \brief The "base" reference object. */
    vx_reference_t base;
    /*! \brief The specific error code contained in this object. */
    vx_status status;
} vx_error_t;

/*! \brief The internal representation of a scalar value.
 * \ingroup group_int_scalar
 */
typedef struct _vx_scalar {
    /*! \brief The internal reference object. */
    vx_reference_t        base;
    /*! \brief The atomic type of the scalar */
    vx_enum               data_type;
    /*! \brief The value contained in the reference for a scalar type */
    union {
        /*! \brief A character */
        vx_char   chr;
        /*! \brief Signed 8 bit */
        vx_int8   s08;
        /*! \brief Unsigned 8 bit */
        vx_uint8  u08;
        /*! \brief Signed 16 bit */
        vx_int16  s16;
        /*! \brief Unsigned 16 bit */
        vx_uint16 u16;
        /*! \brief Signed 32 bit */
        vx_int32  s32;
        /*! \brief Unsigned 32 bit */
        vx_uint32 u32;
        /*! \brief Signed 64 bit */
        vx_int64  s64;
        /*! \brief Unsigned 64 bit */
        vx_int64  u64;
#if defined(EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT)
        /*! \brief 16 bit float */
        vx_float16 f16;
#endif
        /*! \brief 32 bit float */
        vx_float32 f32;
        /*! \brief 64 bit float */
        vx_float64 f64;
        /*! \brief 32 bit image format code */
        vx_df_image  fcc;
        /*! \brief Signed 32 bit*/
        vx_enum    enm;
        /*! \brief Architecture depth unsigned value */
        vx_size    size;
        /*! \brief Boolean Values */
        vx_bool    boolean;
    } data;
} vx_scalar_t;

typedef struct _vx_tensor_t {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The memory layout definition */
    void *addr;
    /*! \brief Number of dimensions */
    vx_uint32 number_of_dimensions;
    /*! \brief Size of all dimensions */
    vx_size dimensions[VX_MAX_TENSOR_DIMENSIONS];
    /*! \brief Stride of all dimensions */
    vx_size stride[VX_MAX_TENSOR_DIMENSIONS];
    /*! \brief Type of data element */
    vx_enum data_type;
    /*! \brief Fixed point position */
    vx_int8 fixed_point_position;
    struct _vx_tensor_t *subtensors[VX_INT_MAX_REF];
    /*! \brief A pointer to a parent md data object. */
    //vx_tensor  parent;
    struct _vx_tensor_t* parent;
	vx_image  subimages[VX_INT_MAX_REF];
} vx_tensor_t;
/*! \brief The internal representation of the attributes associated with a run-time parameter.
 * \ingroup group_int_kernel
 */
typedef struct _vx_signature_t {
    /*! \brief The array of directions */
    vx_enum        directions[VX_INT_MAX_PARAMS];
    /*! \brief The array of types */
    vx_enum        types[VX_INT_MAX_PARAMS];
    /*! \brief The array of states */
    vx_enum        states[VX_INT_MAX_PARAMS];
    /*! \brief The number of items in both \ref vx_signature_t::directions and \ref vx_signature_t::types. */
    vx_uint32      num_parameters;
} vx_signature_t;

/*! \brief The internal representation of a parameter.
 * \ingroup group_int_parameter
 */
typedef struct _vx_parameter {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief Index at which this parameter is tracked in both the node references and kernel signatures */
    vx_uint32      index;
    /*! \brief Pointer to the node which this parameter is associated with */
    vx_node        node;
    /*! \brief Pointer to the kernel which this parameter is associated with, if retreived from
     * \ref vxGetKernelParameterByIndex.
     */
    vx_kernel      kernel;
} vx_parameter_t;

/*! \brief The kernel attributes structure.
 * \ingroup group_int_kernel
 */
typedef struct _vx_kernel_attr_t {
    /*! \brief The local data size for this kernel */
    vx_size       localDataSize;
    /*! \brief The local data pointer for this kernel */
    vx_ptr_t      localDataPtr;
    /*! \brief The global data size for the kernel */
    vx_size       globalDataSize;
    /*! \brief The global data pointer for this kernel */
    vx_ptr_t      globalDataPtr;
    /*! \brief The border mode of this node */
    vx_border_t   borders;
    /*! \brief The border mode of this node */
#ifdef OPENVX_KHR_TILING
    /*! \brief The block size information */
    vx_tile_block_size_t blockinfo;
    /*! \brief The neighborhood size */
    vx_neighborhood_size_t nhbdinfo;
    /*! \brief The tile memory size. */
    vx_size       tileDataSize;
    /*! \brief The tile memory pointer */
    vx_ptr_t      tileDataPtr;
#endif
    /*! \brief The reset valid rectangle flag */
    vx_bool       valid_rect_reset;

#ifdef OPENVX_USE_OPENCL_INTEROP
    vx_bool opencl_access;
#endif
} vx_kernel_attr_t;

/*! \brief The internal representation of an abstract kernel.
 * \ingroup group_int_kernel
 */
typedef struct _vx_kernel {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief */
    vx_char        name[VX_MAX_KERNEL_NAME];
    /*! \brief */
    vx_enum        enumeration;
    /*! \brief */
    vx_kernel_f    function;
    /*! \brief */
    vx_signature_t signature;
    /*! Indicates that the kernel is not yet enabled. */
    vx_bool        enabled;
    /*! Indicates that this kernel is added by user. */
    vx_bool        user_kernel;
    /*! \brief */
    vx_kernel_validate_f validate;
    /*! \brief */
    vx_kernel_input_validate_f validate_input;
    /*! \brief */
    vx_kernel_output_validate_f validate_output;
    /*! \brief */
    vx_kernel_initialize_f initialize;
    /*! \brief */
    vx_kernel_deinitialize_f deinitialize;
    /*! \brief The collection of attributes of a kernel */
    vx_kernel_attr_t attributes;
    /*! \brief Target Index, back reference for the later nodes to inherit affinity */
    vx_uint32 affinity;
#ifdef OPENVX_KHR_TILING
    /*! \brief The tiling function pointer interface */
    vx_tiling_kernel_f tiling_function;
#endif
} vx_kernel_t;

/*! \brief The function which initializes the target
 * \param [in] target The pointer to the target context.
 * \note The target interface function must be exported as "vxTargetInit"
 * \ingroup group_int_target
 */
typedef vx_status (*vx_target_init_f)(vx_target target);

/*! \brief The function which deinitializes the target.
 * \param [in] target The pointer to the target context.
 * \note The target interface function must be exported as "vxTargetDeinit"
 * \ingroup group_int_target
 */
typedef vx_status (*vx_target_deinit_f)(vx_target target);

/*! \brief Allows OpenVX to query a target to see if it supports an additional
 * abstract target type like "khronos.automatic" or "khronos.low_power" on a
 * specific kernel.
 * \param [in] target The pointer to the target context.
 * \param [in] targetName The name of the abstract target.
 * \param [in] kernelName The name of the kernel.
 * \param [in] variantName The variant name of the kernel.
 * \param [out] pIndex The pointer to the index of the kernel in the target's list
 * if the kernel is supported (the function will return VX_SUCCESS).
 * \note The target interface function must be exported as "vxTargetSupports"
 * \return A <tt>\ref vx_status_e</tt> enumeration
 * \retval VX_SUCCESS The kernel is supported and the pIndex has been set.
 * \retval VX_ERROR_NOT_SUPPORTED the kernel is not supported as stated.
 * \ingroup group_int_target
 */
typedef vx_status (*vx_target_supports_f)(vx_target target,
                                          vx_char targetName[VX_MAX_TARGET_NAME],
                                          vx_char kernelName[VX_MAX_TARGET_NAME],
                                          vx_uint32 *pIndex);

/*! \brief Processes the array of nodes supplied.
 * \param [in] target The pointer to the target context.
 * \param [in] nodes The array of nodes pointers.
 * \param [in] startIndex The beginning index to process
 * \param [in] numNodes The number of nodes to process from startIndex.
 * \note The target interface function must be exported as "vxTargetProcess"
 * \ingroup group_int_target
 */
typedef vx_action (*vx_target_process_f)(vx_target target, vx_node nodes[], vx_size startIndex, vx_size numNodes);

/*! \brief Verifies the array of nodes supplied for target specific information.
 * \param [in] target The pointer to the target context.
 * \param [in] node The node to verify.
 * \note The target interface function must be exported as "vxTargetVerify"
 * \ingroup group_int_target
 */
typedef vx_status (*vx_target_verify_f)(vx_target target, vx_node node);

/*! \brief Adds a kernel to a target.
 * \param [in] target The target object.
 * \param [in] name
 * \param [in] enumeration
 * \param [in] func_ptr
 * \param [in] numParams
 * \param [in] validate
 * \param [in] input
 * \param [in] output
 * \param [in] initialize
 * \param [in] deinitialize
 * \ingroup group_int_target
 */
typedef vx_kernel (*vx_target_addkernel_f)(vx_target target,
                                           const vx_char name[VX_MAX_KERNEL_NAME],
                                           vx_enum enumeration,
                                           vx_kernel_f func_ptr,
                                           vx_uint32 num_parameters,
                                           vx_kernel_validate_f validate,
                                           vx_kernel_input_validate_f input,
                                           vx_kernel_output_validate_f output,
                                           vx_kernel_initialize_f initialize,
                                           vx_kernel_deinitialize_f deinitialize);

#ifdef OPENVX_KHR_TILING
/*! \brief Adds a tiling kernel to the target.
 * \ingroup group_int_target
 */
typedef vx_kernel (*vx_target_addtilingkernel_f)(vx_target target,
                                                  const vx_char name[VX_MAX_KERNEL_NAME],
                                                  vx_enum enumeration,
                                                  vx_tiling_kernel_f flexible_func_ptr,
                                                  vx_tiling_kernel_f fast_func_ptr,
                                                  vx_uint32 num_parameters,
                                                  vx_kernel_input_validate_f input,
                                                  vx_kernel_output_validate_f output);
#endif

/*! \brief The structure which holds all the target interface function pointers.
 * \ingroup group_int_target
 */
typedef struct _vx_target_funcs_t {
    /*! \brief Target initialization function */
    vx_target_init_f     init;
    /*! \brief Target deinitialization function */
    vx_target_deinit_f   deinit;
    /*! \brief Target query function */
    vx_target_supports_f supports;
    /*! \brief Target processing function */
    vx_target_process_f  process;
    /*! \brief Target verification function */
    vx_target_verify_f   verify;
    /*! \brief Target function to add a kernel */
    vx_target_addkernel_f addkernel;
#ifdef OPENVX_KHR_TILING
    /*! \brief Target function to add a tiling kernel */
    vx_target_addtilingkernel_f addtilingkernel;
#endif
} vx_target_funcs_t;

enum vx_ext_target_type_e {
    VX_TYPE_TARGET = 0x816,/*!< \brief A <tt>\ref vx_target</tt> */
};

/*! \brief The priority list of targets.
 * \ingroup group_int_target
 */
enum vx_target_priority_e {
    /*! \brief Defines the priority of the C model target */
    VX_TARGET_PRIORITY_C_MODEL,
    /*! \brief Defines the maximum priority */
    VX_TARGET_PRIORITY_MAX,
};

/*! \brief Defines the number of targets in the sample implementation.
 * \ingroup group_int_target
 */
#define VX_INT_MAX_NUM_TARGETS  (VX_TARGET_PRIORITY_MAX)

/*! \brief The tracking structure for a module.
 * \ingroup group_int_context
 */
typedef struct _vx_module_t {
    /*! \brief The name of the module. */
    vx_char             name[VX_INT_MAX_PATH];
    /*! \brief The module handle */
    vx_module_handle_t  handle;
    /*! \brief The reference counter */
    vx_uint32 ref_count;
    /*! \brief The module lock which is used to protect access to "in-fly" data. */
    vx_sem_t lock;
} vx_module_t;

/*! \brief The internal representation of a target.
 * \ingroup group_int_target
 */
typedef struct _vx_target {
    /*! \brief The internal reference object */
    vx_reference_t      base;
    /*! \brief A quick checking method to see if the target is usable. */
    vx_bool             enabled;
    /*! \brief The name of the target */
    vx_char             name[VX_MAX_TARGET_NAME];
    /*! \brief The handle to the module which contains the target interface */
    vx_module_t         module;
    /*! \brief The table of function pointers to target */
    vx_target_funcs_t   funcs;
    /*! \brief Used to determine precidence when more than once core supports a kernel */
    vx_uint32           priority;
    /*! \brief The number of supported kernels on this target */
    vx_uint32           num_kernels;
    /*! \brief The supported kernels on this target */
    vx_kernel_t         kernels[VX_INT_MAX_KERNELS];
    /*! \brief Target Specific Private Data */
    void               *reserved;
} vx_target_t;

/*! \brief The framework's internal-external memory tracking structure.
 * \ingroup group_int_context.
 */
typedef struct _vx_external_t {
    /*! \brief The pointer associated with the reference. */
    void *ptr;
    /*! \brief The reference being accessed */
    vx_reference ref;
    /*! \brief The usage model of the pointer */
    vx_enum usage;
    /*! \brief The allocated state of the pointer, if true, the framework can free the memory. */
    vx_bool allocated;
    /*! \brief Indicates if this entry is being used */
    vx_bool used;
    /*! \brief Extra data attached to the accessor */
    void *extra_data;
} vx_external_t;

typedef union _vx_memory_map_extra
{
    struct
    {
        /*! \brief The rectangle to map in case of image */
        vx_rectangle_t rect;
        vx_uint32 plane_index;
    } image_data;
    struct
    {
        vx_size start;
        vx_size end;
    } array_data;
} vx_memory_map_extra;

/*! \brief The framework's mapping memory tracking structure.
 * \ingroup group_int_context.
 */
typedef struct _vx_memory_map_t
{
    /*! \brief Indicates if this entry is being used */
    vx_bool used;
    /*! \brief The reference of data object being mapped */
    vx_reference ref;
    /*! \brief The extra data of mapped object */
    vx_memory_map_extra extra;
    /*! \brief The usage model of the pointer */
    vx_enum usage;
    /*! \brief The memory type */
    vx_enum mem_type;
    /*! \brief The options to map operation */
    vx_uint32 flags;
    /*! \brief The mapping buffer pointer associated with the reference. */
    void* ptr;
#ifdef OPENVX_USE_OPENCL_INTEROP
    cl_mem         opencl_buf;
    vx_uint32      opencl_offset;
#endif
} vx_memory_map_t;

/*! \brief The top level context data for the entire OpenVX instance
 * \ingroup group_int_context
 */
typedef struct _vx_context {
    /*! \brief The base reference object */
    vx_reference_t      base;
    /*! \brief The pointer to process global lock */
    vx_sem_t*           p_global_lock;
    /*! \brief The reference table which contains the handle for later garage collection if needed */
    vx_reference        reftable[VX_INT_MAX_REF];
    /*! \brief The number of references in the table. */
    vx_uint32           num_references;
    /*! \brief The array of kernel modules. */
    vx_module_t         modules[VX_INT_MAX_MODULES];
    /*! \brief The number of kernel libraries loaded */
    vx_uint32           num_modules;
    /*! \brief The graph queue processor */
    vx_processor_t      proc;
    /*! \brief The combined number of unique kernels in the system */
    vx_uint32           num_kernels;
    /*! \brief The number of unique kernels */
    vx_uint32           num_unique_kernels;
    /*! \brief The number of available targets in the implementation */
    vx_uint32           num_targets;
    /*! \brief The list of implemented targets */
    vx_target_t         targets[VX_INT_MAX_NUM_TARGETS];
    /*! \brief The list of priority sorted target indexes */
    vx_uint32           priority_targets[VX_INT_MAX_NUM_TARGETS];
    /*! \brief The log callback for errors */
    vx_log_callback_f   log_callback;
    /*! \brief The log semaphore */
    vx_sem_t            log_lock;
    /*! \brief The log enable toggle. */
    vx_bool             log_enabled;
    /*! \brief If true the log callback is reentrant and doesn't need to be locked. */
    vx_bool             log_reentrant;
    /*! \brief The performance counter enable toggle. */
    vx_bool             perf_enabled;
    /*! \brief The list of externally accessed references */
    vx_external_t       accessors[VX_INT_MAX_REF];
    /*! \brief The memory mapping table lock */
    vx_sem_t            memory_maps_lock;
    /*! \brief The list of memory maps */
    vx_memory_map_t     memory_maps[VX_INT_MAX_REF];
    /*! \brief The list of user defined structs. */
    struct {
        /*! \brief Type constant */
        vx_enum type;
        /*! \brief Size in bytes */
        vx_size size;
        /*! \brief Name */
        vx_char name[VX_MAX_STRUCT_NAME];
    } user_structs[VX_INT_MAX_USER_STRUCTS];
    /*! \brief The worker pool used to parallelize the graph*/
    vx_threadpool_t    *workers;
    /*! \brief The immediate mode border */
    vx_border_t         imm_border;
    /*! \brief The unsupported border mode policy for immediate mode functions */
    vx_enum             imm_border_policy;
    /*! \brief The next available dynamic user kernel ID */
    vx_uint32           next_dynamic_user_kernel_id;
    /*! \brief The next available dynamic user library ID */
    vx_uint32           next_dynamic_user_library_id;
    /*! \brief The immediate mode enumeration */
    vx_enum             imm_target_enum;
    /*! \brief The immediate mode target string */
    vx_char             imm_target_string[VX_MAX_TARGET_NAME];
#ifdef OPENVX_USE_OPENCL_INTEROP
    cl_context opencl_context;
    cl_command_queue opencl_command_queue;
#endif

} vx_context_t;

/*! \brief A data structure used to track the various costs which could being optimized.
 *
 */
typedef struct _vx_cost_factors_t {
    /*! \brief [computed] A measure of the bandwidth due to processing data */
    vx_size  bandwidth;
    /*! \brief [estimate] The power factor */
    vx_float32 power;
    /*! \brief [constant] The cycle count per unit data */
    vx_float32 cycles_per_unit;
    /*! \brief [estimate] The overhead latency due to IPC, etc. */
    vx_uint64 overhead;
} vx_cost_factors_t;

/*! \brief The internal representation of a node.
 * \ingroup group_int_node
 */
typedef struct _vx_node {
    /*! \brief The internal reference object. */
    vx_reference_t      base;
    /*! \brief The pointer to the kernel structure */
    vx_kernel           kernel;
    /*! \brief The list of references which are the values to pass to the kernels */
    vx_reference        parameters[VX_INT_MAX_PARAMS];
    /*! \brief Status code returned from the last execution of the kernel. */
    vx_status           status;
    /*! \brief The performance logging variable */
    vx_perf_t           perf;
    /*! \brief A callback to call when the node is complete */
    vx_nodecomplete_f   callback;
    /*! \brief Enable to change read-only attributes (VX_NODE_LOCAL_DATA_SIZE, VX_NODE_LOCAL_DATA_PTR) for user-kernel initialization callback needs. */
    vx_bool             local_data_change_is_enabled;
    /*! \brief Indicates that attributes (VX_NODE_LOCAL_DATA_SIZE, VX_NODE_LOCAL_DATA_PTR) were set by implementation, but not user-kernel initialization callback. */
    vx_bool             local_data_set_by_implementation;
    /*! \brief A back reference to the parent graph. */
    vx_graph            graph;
    /*! \brief Used to keep track of visitation during execution and verification. */
    vx_bool             visited;
    /*! \brief This is used to determine if the node has executed. */
    vx_bool             executed;
    /*! \brief The instance version of the attributes of the kernel */
    vx_kernel_attr_t    attributes;
    /*! \brief The index of the target to execute the kernel on */
    vx_uint32           affinity;
    /*! \brief The child graph of the node. */
    vx_graph            child;
    /*! \brief The node cost factors */
    vx_cost_factors_t   costs;
    /*! \brief The node replica flag */
    vx_bool             is_replicated;
    /*! \brief The replicated parameters flags */
    vx_bool             replicated_flags[VX_INT_MAX_PARAMS];
} vx_node_t;

/*! \brief The internal representation of a graph.
 * \ingroup group_int_graph
 */
typedef struct _vx_graph {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The array of all nodes in this graph */
    vx_node        nodes[VX_INT_MAX_REF];
    /*! \brief The performance logging variable. */
    vx_perf_t      perf;
    /*! \brief The number of nodes actively allocated in this graph. */
    vx_uint32      numNodes;
    /*! \brief The array of all starting node indexes in the graph. */
    vx_uint32      heads[VX_INT_MAX_REF];
    /*! \brief The number of all nodes in heads list */
    vx_uint32      numHeads;
    /*! \brief The state of the graph (vx_graph_state_e) */
    vx_enum        state;
    /*! \brief This indicates that the graph has been verified. */
    vx_bool        verified;
    /*! \brief This indicates that the graph has been verified earlier, but invalidated latter and is need for verification again. */
    vx_bool        reverify;
    /*! \brief This lock is used to prevent multiple schedulings (data overwrite) */
    vx_sem_t       lock;
    /*! \brief The list of graph parameters. */
    struct {
        /*! \brief The reference to the node which has the parameter */
        vx_node_t *node;
        /*! \brief The index to the parameter on the node. */
        vx_uint32  index;
    } parameters[VX_INT_MAX_PARAMS];
    /*! \brief The number of graph parameters. */
    vx_uint32      numParams;
    /*! \brief A switch to turn off SMP mode */
    vx_bool        should_serialize;
    /*! \brief [hidden] If non-NULL, the parent graph, for scope handling. */
    vx_graph       parentGraph;
    /*! \brief The array of all delays in this graph */
    vx_delay       delays[VX_INT_MAX_REF];
} vx_graph_t;

/*! \brief The dimensions enumeration, also stride enumerations.
 * \ingroup group_int_image
 */
enum vx_dim_e {
    /*! \brief Channels dimension, stride */
    VX_DIM_C = 0,
    /*! \brief Width (dimension) or x stride */
    VX_DIM_X,
    /*! \brief Height (dimension) or y stride */
    VX_DIM_Y,
    /*! \brief [hidden] The maximum number of dimensions */
    VX_DIM_MAX,
};

/*! \brief The bounds enumeration.
 * \ingroup group_int_image
 */
enum vx_bounds_e {
    /*! \brief The starting inclusive bound */
    VX_BOUND_START,
    /*! \brief The ending exclusive bound */
    VX_BOUND_END,
    /*! \brief [hidden] The maximum bound dimension */
    VX_BOUND_MAX,
};

/*! \brief The maximum number of 2d planes an image may have.
 * \ingroup group_int_image
 */
#define VX_PLANE_MAX    (4)

/*! \brief The raw definition of memory layout.
 * \ingroup group_int_memory
 */
typedef struct _vx_memory_t {
    /*! \brief Determines if this memory was allocated by the system */
    vx_bool        allocated;
    /*! \brief The number of pointers in the array */
    vx_uint32      nptrs;
    /*! \brief The array of ROI offsets (one per plane for images) */
    vx_uint32      offset[VX_PLANE_MAX];
    /*! \brief The array of pointers (one per plane for images) */
    vx_uint8*      ptrs[VX_PLANE_MAX];
#ifdef OPENVX_USE_OPENCL_INTEROP
    cl_mem         opencl_buf[VX_PLANE_MAX];
    vx_uint32      opencl_offset[VX_PLANE_MAX];
#endif
    /*! \brief The number of dimensions per ptr */
    vx_uint32       ndims;
    /*! \brief The dimensional values per ptr */
    vx_uint32       dims[VX_PLANE_MAX][VX_DIM_MAX];
    /*! \brief The per ptr stride values per dimension */
    vx_int32       strides[VX_PLANE_MAX][VX_DIM_MAX];
    /*! \brief The write locks. Used by Access/Commit pairs on usages which have
     * VX_WRITE_ONLY or VX_READ_AND_WRITE flag parts. Only single writers are permitted.
     */
    vx_sem_t locks[VX_PLANE_MAX];
} vx_memory_t;

/*! \brief The internal representation of a \ref vx_image
 * \ingroup group_int_image
 */
typedef struct _vx_image {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The memory layout definition */
    vx_memory_t    memory;
    /*! \brief Width of the Image in Pixels */
    vx_uint32      width;
    /*! \brief Height of the Image in Pixels */
    vx_uint32      height;
    /*! \brief Format of the Image in VX_DF_IMAGE codes */
    vx_df_image    format;
    /*! \brief The number of active planes */
    vx_uint32      planes;
    /*! \brief The constants space (BT601 or BT709) */
    vx_enum        space;
    /*! \brief The desired color range */
    vx_enum        range;
    /*! \brief The sub-channel scaling for each plane */
    vx_uint32      scale[VX_PLANE_MAX][VX_DIM_MAX];
    /*! \brief The per-plane, per-dimension bounds (start, end). */
    vx_uint32      bounds[VX_PLANE_MAX][VX_DIM_MAX][VX_BOUND_MAX];
    /*! \brief A pointer to a parent image object. */
    vx_image       parent;
    /*! \brief The array of ROIs from this image */
    vx_image       subimages[VX_INT_MAX_REF];
    /*! \brief Indicates if the image is constant. */
    vx_bool        constant;
    /*! \brief The valid region */
    vx_rectangle_t region;
    /*! \brief The memory type */
    vx_enum        memory_type;
} vx_image_t;

/*! \brief The internal representation of a \ref vx_array
 * \ingroup group_int_array
 */
typedef struct _vx_array {
    /*! \brief The base reference object */
    vx_reference_t base;
    /*! \brief The memory layout definition */
    vx_memory_t memory;
    /*! \brief The item type of the array. */
    vx_enum item_type;
    /*! \brief The size of array item in bytes */
    vx_size item_size;
    /*! \brief The number of items in the array */
    vx_size num_items;
    /*! \brief The array capacity */
    vx_size capacity;
    /*! \brief Offset attribute value. Used internally by LUT implementation */
    vx_uint32 offset;
} vx_array_t;

/*! \brief The internal representation of a \ref vx_object_array
 * \ingroup group_int_object_array
 */
typedef struct _vx_object_array {
    /*! \brief The base reference object */
    vx_reference_t base;
    /*! \brief The reference table of array items */
    vx_reference items[VX_INT_MAX_REF];
    /*! \brief The number of items in the array */
    vx_size num_items;
    /*! \brief The item type of the array. */
    vx_enum item_type;
} vx_object_array_t;

/*! \brief The internal representation of the delay parameters as a list.
 * \ingroup group_int_delay
 */
typedef struct _vx_delay_param_t {
    struct _vx_delay_param_t *next;
    vx_node node;
    vx_uint32 index;
} vx_delay_param_t;

/*! \brief The internal representation of any delay object.
 * \ingroup group_int_delay
 */
typedef struct _vx_delay {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The number of objects in the delay. */
    vx_size count;
    /*! \brief The current index which is '0' */
    vx_uint32 index;
    /*! \brief Object Type in the Delay. */
    vx_enum type;
    /*! \brief The set of Nodes each object is associated with in the delay. */
    vx_delay_param_t *set;
    /*! \brief The set of objects in the delay. */
    vx_reference *refs;
    /*! \brief The set of delays for pyramid levels. */
    vx_delay *pyr;
} vx_delay_t;

/*! \brief A LUT is a specific type of array.
 * \ingroup group_int_lut
 */
typedef vx_array_t vx_lut_t;

/*! \brief A remap is a 2D image of float32 pairs.
 * \ingroup group_int_remap
 */
typedef struct _vx_remap {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The memory layout */
    vx_memory_t memory;
    /*! \brief Input Width */
    vx_uint32 src_width;
    /*! \brief Input Height */
    vx_uint32 src_height;
    /*! \brief Output Width */
    vx_uint32 dst_width;
    /*! \brief Output Height */
    vx_uint32 dst_height;
} vx_remap_t;

/*! \brief A histogram.
 * \ingroup group_int_histogram
 */
typedef struct _vx_distribution {
    /*! \brief Base object */
    vx_reference_t base;
    /*! \brief Memory layout */
    vx_memory_t memory;
    /*! \brief The total number of the values in the active X dimension of the distribution. */
    vx_uint32 range_x;
    /*! \brief The total number of the values in the active Y dimension of the distribution. */
    vx_uint32 range_y;
    /*! \brief The number of inactive elements from zero in the X dimension */
    vx_int32 offset_x;
    /*! \brief The number of inactive elements from zero in the Y dimension */
    vx_int32 offset_y;
} vx_distribution_t;


#define VX_DEFAULT_THRESHOLD_FALSE_VALUE 0
#define VX_DEFAULT_THRESHOLD_TRUE_VALUE  255

#define VX_S16_THRESHOLD_FALSE_VALUE 0
#define VX_S16_THRESHOLD_TRUE_VALUE  (-1)
#define VX_U16_THRESHOLD_FALSE_VALUE 0
#define VX_U16_THRESHOLD_TRUE_VALUE  0xFFFF
#define VX_S32_THRESHOLD_FALSE_VALUE 0
#define VX_S32_THRESHOLD_TRUE_VALUE  (-1)
#define VX_U32_THRESHOLD_FALSE_VALUE 0
#define VX_U32_THRESHOLD_TRUE_VALUE  0xFFFFFFFF

/*! \brief The internal threshold structure.
 * \ingroup group_int_threshold
 */
typedef struct _vx_threshold {
    /*! \brief Base object */
    vx_reference_t base;
    /*! \brief From \ref vx_threshold_type_e */
    vx_enum thresh_type;
    /*! \brief From \ref vx_type_e */
    vx_enum data_type;
    /*! \brief The binary threshold value */
    vx_pixel_value_t value;
    /*! \brief Lower bound for range threshold */
    vx_pixel_value_t lower;
    /*! \brief Upper bound for range threshold */
    vx_pixel_value_t upper;
    /*! \brief True value for output */
    vx_pixel_value_t true_value;
    /*! \brief False value for output */
    vx_pixel_value_t false_value;
    /*! \brief The input image format */
    vx_df_image input_format;
    /*! \brief The output image format  */
    vx_df_image output_format;
} vx_threshold_t;

/*! \brief The internal matrix structure.
 * \ingroup group_int_matrix
 */
typedef struct _vx_matrix {
    /*! \brief Base object */
    vx_reference_t base;
    /*! \brief Memory Layout */
    vx_memory_t memory;
    /*! \brief From \ref vx_type_e */
    vx_enum data_type;
    /*! \brief Number of columns */
    vx_size columns;
    /*! \brief Number of rows */
    vx_size rows;
    /*! \brief Origin */
    vx_coordinates2d_t origin;
    /*! \brief Pattern */
    vx_enum pattern;
} vx_matrix_t;

/*! \brief A convolution is a special type of matrix (MxM)
 * \ingroup group_int_convolution
 */
typedef struct _vx_convolution {
    vx_matrix_t base;   /*!< \brief Inherits everything from \ref vx_matrix_t. */
    vx_uint32 scale;    /*!< \brief The Scale Factor. */
} vx_convolution_t;

/*! \brief A pyramid object. Contains a set of scaled images.
 * \ingroup group_int_pyramid
 */
typedef struct _vx_pyramid {
    /*! \brief Base object */
    vx_reference_t base;
    /*! \brief Number of levels in the pyramid */
    vx_size numLevels;
    /*! \brief Array of images */
    vx_image *levels;
    /*! \brief Scaling factor between levels of the pyramid. */
    vx_float32 scale;
    /*! \brief Level 0 width */
    vx_uint32 width;
    /*! \brief Level 0 height */
    vx_uint32 height;
    /*! \brief Format for all levels */
    vx_df_image format;
} vx_pyramid_t;

/*! \brief The internal representation of any import object.
 * \ingroup group_int_import
 */
typedef struct _vx_import {
    /*! \brief The internal reference object. */
    vx_reference_t base;
    /*! \brief The type of import */
    vx_enum type;
    /*! \brief The number of references in the import. */
    vx_uint32 count;
    /*! \brief The set of references in the import. */
    vx_reference *refs;
} vx_import_t;

/*!
 * \brief This structure is used to extract meta data from a validation
 * function. If the data object between nodes is virtual, this will allow the
 * framework to automatically create the data object, if needed.
 * \ingroup group_int_meta_format
 */
typedef struct _vx_meta_format
{
    vx_reference_t base;        /*!< \brief The base reference */
    vx_size size;               /*!< \brief The size of struct. */
    vx_enum type;               /*!< \brief The <tt>\ref vx_type_e</tt> or <tt>\ref vx_df_image_e</tt> code */
    /*! \brief A struct of the configuration types needed by all object types. */
    struct dim {
        /*! \brief When a VX_TYPE_IMAGE */
        struct image {
            vx_uint32 width;    /*!< \brief The width of the image in pixels */
            vx_uint32 height;   /*!< \brief The height of the image in pixels */
            vx_df_image format;   /*!< \brief The format of the image. */
            vx_rectangle_t delta; /*!< \brief The delta rectangle applied to this image */
        } image;
        /*! \brief When a VX_TYPE_PYRAMID is specified */
        struct pyramid {
            vx_uint32 width;    /*!< \brief The width of the 0th image in pixels. */
            vx_uint32 height;   /*!< \brief The height of the 0th image in pixels. */
            vx_df_image format;   /*!< \brief The <tt>\ref vx_df_image_e</tt> format of the image. */
            vx_size levels;     /*!< \brief The number of scale levels */
            vx_float32 scale;   /*!< \brief The ratio between each level */
        } pyramid;
        /*! \brief When a VX_TYPE_SCALAR is specified */
        struct scalar {
            vx_enum type;       /*!< \brief The type of the scalar */
        } scalar;
        /*! \brief When a VX_TYPE_ARRAY is specified */
        struct array {
            vx_enum item_type;  /*!< \brief The type of the Array items */
            vx_size capacity;   /*!< \brief The capacity of the Array */
        } array;
        /*! \brief When a VX_TYPE_MATRIX is specified */
        struct matrix {
            vx_enum type;       /*!< \brief The value type of the matrix*/
            vx_size rows;       /*!< \brief The M dimension of the matrix*/
            vx_size cols;       /*!< \brief The N dimension of the matrix*/
        } matrix;
        /*! \brief When a VX_TYPE_DISTRIBUTION is specified */
        struct distribution {
            vx_size bins;       /*!< \brief Indicates the number of bins*/
            vx_int32 offset;    /*!< \brief Indicates the start of the values to use (inclusive)*/
            vx_uint32 range;    /*!< \brief Indicates the total number of the consecutive values of the distribution interval*/
        } distribution;
        /*! \brief When a VX_TYPE_REMAP is specified */
        struct remap {
            vx_uint32 src_width; /*!< \brief The source width*/
            vx_uint32 src_height;/*!< \brief The source height*/
            vx_uint32 dst_width; /*!< \brief The destination width*/
            vx_uint32 dst_height;/*!< \brief The destination width*/
        } remap;
        /*! \brief When a VX_TYPE_LUT is specified */
        struct lut {
            vx_enum type;        /*!< \brief Indicates the value type of the LUT*/
            vx_size count;       /*!< \brief Indicates the number of elements in the LUT*/
        } lut;
        /*! \brief When a VX_TYPE_THRESHOLD is specified */
        struct threshold {
            vx_enum type; /*!< \brief The value type of the threshold*/
        } threshold;
        /*! \brief When a VX_TYPE_OBJECT_ARRAY is specified */
        struct object_array {
            vx_enum item_type;  /*!< \brief The type of the ObjectArray items */
            vx_size num_items;  /*!< \brief The number of ObjectArray items */
        } object_array;
        struct tensor {
            vx_size number_of_dimensions;                 /*! \brief Number of dimensions */
            vx_size dimensions[VX_MAX_TENSOR_DIMENSIONS];   /*! \brief Size of all dimensions */
            vx_enum data_type;                              /*! \brief Type of data element */
            vx_int8 fixed_point_position;                  /*! \brief Fixed point position */
        } tensor;
    } dim;

    vx_kernel_image_valid_rectangle_f set_valid_rectangle_callback;
} vx_meta_format_t;

#include <VX/vx_helper.h>
#include "vx_debug.h"

// PROTOTYPES FOR INTERNAL FUNCTIONS
#include "vx_context.h"
#include "vx_delay.h"
#include "vx_graph.h"
#include "vx_image.h"
#include "vx_tensor.h"
#include "vx_kernel.h"
#include "vx_log.h"
#include "vx_node.h"
#include "vx_osal.h"
#include "vx_parameter.h"
#include "vx_reference.h"
#include "vx_scalar.h"
#include "vx_target.h"
#include "vx_memory.h"
#include "vx_convolution.h"
#include "vx_distribution.h"
#include "vx_lut.h"
#include "vx_matrix.h"
#include "vx_pyramid.h"
#include "vx_threshold.h"
#include "vx_remap.h"
#include "vx_array.h"
#include "vx_object_array.h"
#include "vx_error.h"
#include "vx_meta_format.h"
#include "vx_import.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <vx_inlines.c>

#if !DISABLE_ICD_COMPATIBILITY
	VX_API_ENTRY vx_context VX_API_CALL vxCreateContextFromPlatform(struct _vx_platform * platform);
#endif

#ifdef __cplusplus
}
#endif

#endif
