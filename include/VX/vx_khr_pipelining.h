/*
 * Copyright (c) 2012-2017 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
 * KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
 * SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
 *    https://www.khronos.org/registry/
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */
#ifndef _OPENVX_PIPELINING_H_
#define _OPENVX_PIPELINING_H_

/*!
 * \file
 * \brief The OpenVX Pipelining, Streaming and Batch Processing extension API.
 */

#define OPENVX_KHR_PIPELINING  "vx_khr_pipelining"

#include <VX/vx.h>

#ifdef  __cplusplus
extern "C" {
#endif

/*
 * PIPELINING API
 */

/*! \brief The Pipelining, Streaming and Batch Processing Extension Library Set
 *
 * \ingroup group_pipelining
 */
#define VX_LIBRARY_KHR_PIPELINING_EXTENSION (0x1)


/*! \brief Extra enums.
 *
 * \ingroup group_pipelining
 */
enum vx_graph_schedule_mode_enum_e
{
    VX_ENUM_GRAPH_SCHEDULE_MODE_TYPE     = 0x1E, /*!< \brief Graph schedule mode type enumeration. */
};

/*! \brief Type of graph scheduling mode
 *
 * See <tt>\ref vxSetGraphScheduleConfig</tt> and <tt>\ref vxGraphParameterEnqueueReadyRef</tt> for details about each mode.
 *
 * \ingroup group_pipelining
 */
enum vx_graph_schedule_mode_type_e {

    /*! \brief Schedule graph in non-queueing mode
     */
    VX_GRAPH_SCHEDULE_MODE_NORMAL = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_GRAPH_SCHEDULE_MODE_TYPE << 12)) + 0x0,

    /*! \brief Schedule graph in queueing mode with auto scheduling
     */
    VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_GRAPH_SCHEDULE_MODE_TYPE << 12)) + 0x1,

    /*! \brief Schedule graph in queueing mode with manual scheduling
     */
    VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_GRAPH_SCHEDULE_MODE_TYPE << 12)) + 0x2,
};

/*! \brief The graph attributes added by this extension.
 * \ingroup group_pipelining
 */
enum vx_graph_attribute_pipelining_e {
    /*! \brief Returns the schedule mode of a graph. Read-only. Use a <tt>\ref vx_enum</tt> parameter.
     * See <tt>\ref vx_graph_schedule_mode_type_e </tt> enum.
     * */
    VX_GRAPH_SCHEDULE_MODE = VX_ATTRIBUTE_BASE(VX_ID_KHRONOS, VX_TYPE_GRAPH) + 0x5,
};

/*! \brief Queueing parameters for a specific graph parameter
 *
 * See <tt>\ref vxSetGraphScheduleConfig</tt> for additional details.
 *
 *  \ingroup group_pipelining
 */
typedef struct {

    uint32_t graph_parameter_index;
    /*!< \brief Index of graph parameter to which these properties apply */

    vx_uint32 refs_list_size;
    /*!< \brief Number of elements in array 'refs_list' */

    vx_reference *refs_list;
    /*!< \brief Array of references that could be enqueued at a later point of time at this graph parameter */

} vx_graph_parameter_queue_params_t;

/*! \brief Sets the graph scheduler config
 *
 * This API is used to set the graph scheduler config to
 * allow user to schedule multiple instances of a graph for execution.
 *
 * For legacy applications that don't need graph pipelining or batch processing,
 * this API need not be used.
 *
 * Using this API, the application specifies the graph schedule mode, as well as
 * queueing parameters for all graph parameters that need to allow enqueueing of references.
 * A single monolithic API is provided instead of discrete APIs, since this allows
 * the implementation to get all information related to scheduling in one shot and
 * then optimize the subsequent graph scheduling based on this information.
 * <b> This API MUST be called before graph verify </b> since
 * in this case it allows implementations the opportunity to optimize resources based on
 * information provided by the application.
 *
 * 'graph_schedule_mode' selects how input and output references are provided to a graph and
 * how the next graph schedule is triggered by an implementation.
 *
 * Below scheduling modes are supported:
 *
 * When graph schedule mode is <tt>\ref VX_GRAPH_SCHEDULE_MODE_QUEUE_AUTO</tt>:
 * - Application needs to explicitly call <tt>\ref vxVerifyGraph</tt> before enqueing data references
 * - Application should not call <tt>\ref vxScheduleGraph</tt> or <tt>\ref vxProcessGraph</tt>
 * - When enough references are enqueued at various graph parameters, the implementation
 *   could trigger the next graph schedule.
 * - Here, not all graph parameters need to have enqueued references for a graph schedule to begin.
 *   An implementation is expected to execute the graph as much as possible until a enqueued reference
 *   is not available at which time it will stall the graph until the reference becomes available.
 *   This allows application to schedule a graph even when all parameters references are
 *   not yet available, i.e do a 'late' enqueue. However, exact behaviour is implementation specific.
 *
 * When graph schedule mode is <tt>\ref VX_GRAPH_SCHEDULE_MODE_QUEUE_MANUAL</tt>:
 * - Application needs to explicitly call <tt>\ref vxScheduleGraph</tt>
 * - Application should not call <tt>\ref vxProcessGraph</tt>
 * - References for all graph parameters of the graph needs to enqueued before <tt>\ref vxScheduleGraph</tt>
 *   is called on the graph else an error is returned by <tt>\ref vxScheduleGraph</tt>
 * - Application can enqueue multiple references at the same graph parameter.
 *   When <tt>\ref vxScheduleGraph</tt> is called, all enqueued references get processed in a 'batch'.
 * - User can use <tt>\ref vxWaitGraph</tt> to wait for the previous <tt>\ref vxScheduleGraph</tt>
 *   to complete.
 *
 * When graph schedule mode is <tt>\ref VX_GRAPH_SCHEDULE_MODE_NORMAL</tt>:
 * - 'graph_parameters_list_size' MUST be 0 and
 * - 'graph_parameters_queue_params_list' MUST be NULL
 * - This mode is equivalent to non-queueing scheduling mode as defined by OpenVX v1.2 and earlier.
 *
 * By default all graphs are in VX_GRAPH_SCHEDULE_MODE_NORMAL mode until this API is called.
 *
 * 'graph_parameters_queue_params_list' allows to specify below information:
 * - For the graph parameter index that is specified, it enables queueing mode of operation
 * - Further it allows the application to specify the list of references that it could later
 *   enqueue at this graph parameter.
 *
 * For graph parameters listed in 'graph_parameters_queue_params_list',
 * application MUST use <tt>\ref vxGraphParameterEnqueueReadyRef</tt> to
 * set references at the graph parameter.  Using other data access API's on
 * these parameters or corresponding data objects will return an error.
 * For graph parameters not listed in 'graph_parameters_queue_params_list'
 * application MUST use the <tt>\ref vxSetGraphParameterByIndex</tt> to set the reference
 * at the graph parameter.  Using other data access API's on these parameters or
 * corresponding data objects will return an error.
 *
 * This API also allows application to provide a list of references which could be later
 * enqueued at the graph parameter. This allows implementation to do meta-data checking
 * up front rather than during each reference enqueue.
 *
 * When this API is called before <tt>\ref vxVerifyGraph</tt>, the 'refs_list' field
 * can be NULL, if the reference handles are not available yet at the application.
 * However 'refs_list_size' MUST always be specified by the application.
 * Application can call <tt>\ref vxSetGraphScheduleConfig</tt> again after verify graph
 * with all parameters remaining the same except with 'refs_list' field providing
 * the list of references that can be enqueued at the graph parameter.
 *
 * \param [in] graph Graph reference
 * \param [in] graph_schedule_mode Graph schedule mode. See <tt>\ref vx_graph_schedule_mode_type_e</tt>
 * \param [in] graph_parameters_list_size Number of elements in graph_parameters_queue_params_list
 * \param [in] graph_parameters_queue_params_list Array containing queuing properties at graph parameters that need to support queueing.
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid reference
 * \retval VX_ERROR_INVALID_PARAMETERS Invalid graph parameter queueing parameters
 * \retval VX_FAILURE Any other failure.
 *
 * \ingroup group_pipelining
 */
VX_API_ENTRY vx_status vxSetGraphScheduleConfig(
    vx_graph graph,
    vx_enum graph_schedule_mode,
    uint32_t graph_parameters_list_size,
    const vx_graph_parameter_queue_params_t graph_parameters_queue_params_list[]
    );

/*! \brief Enqueues new references into a graph parameter for processing
 *
 * This new reference will take effect on the next graph schedule.
 *
 * In case of a graph parameter which is input to a graph, this function provides
 * a data reference with new input data to the graph.
 * In case of a graph parameter which is not input to a graph, this function provides
 * a 'empty' reference into which a graph execution can write new data into.
 *
 * This function essentially transfers ownership of the reference from the application to the graph.
 *
 * User MUST use <tt>vxGraphParameterDequeueDoneRef</tt> to get back the
 * processed or consumed references.
 *
 * The references that are enqueued MUST be the references listed during
 * <tt>\ref vxSetGraphScheduleConfig</tt>. If a reference outside this list is provided then
 * behaviour is undefined.
 *
 * \param [in] graph Graph reference
 * \param [in] graph_parameter_index Graph parameter index
 * \param [in] refs The array of references to enqueue into the graph parameter
 * \param [in] num_refs Number of references to enqueue
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid reference OR reference is not a valid reference
 * \retval VX_ERROR_INVALID_PARAMETERS graph_parameter_index is NOT a valid graph parameter index
 * \retval VX_FAILURE Reference could not be enqueued.
 *
 * \ingroup group_pipelining
 */
VX_API_ENTRY vx_status VX_API_CALL vxGraphParameterEnqueueReadyRef(vx_graph graph,
                vx_uint32 graph_parameter_index,
                vx_reference *refs,
                vx_uint32 num_refs);



/*! \brief Dequeues 'consumed' references from a graph parameter
 *
 * This function dequeues references from a graph parameter of a graph.
 * The reference that is dequeued is a reference that had been previously enqueued into a graph,
 * and after subsequent graph execution is considered as processed or consumed by the graph.
 * This function essentially transfers ownership of the reference from the graph to the application.
 *
 * <b> IMPORTANT </b> : This API will block until at least one reference is dequeued.
 *
 * In case of a graph parameter which is input to a graph, this function provides
 * a 'consumed' buffer to the application so that new input data can filled
 * and later enqueued to the graph.
 * In case of a graph parameter which is not input to a graph, this function provides
 * a reference filled with new data based on graph execution. User can then use this
 * newly generated data with their application. Typically when this new data is
 * consumed by the application the 'empty' reference is again enqueued to the graph.
 *
 * This API returns an array of references up to a maximum of 'max_refs'. Application MUST ensure
 * the array pointer ('refs') passed as input can hold 'max_refs'.
 * 'num_refs' is actual number of references returned and will be <= 'max_refs'.
 *
 *
 * \param [in] graph Graph reference
 * \param [in] graph_parameter_index Graph parameter index
 * \param [in] refs Pointer to an array of max elements 'max_refs'
 * \param [out] refs Dequeued references filled in the array
 * \param [in] max_refs Max number of references to dequeue
 * \param [out] num_refs Actual number of references dequeued.
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid reference
 * \retval VX_ERROR_INVALID_PARAMETERS graph_parameter_index is NOT a valid graph parameter index
 * \retval VX_FAILURE Reference could not be dequeued.
 *
 * \ingroup group_pipelining
 */
VX_API_ENTRY vx_status VX_API_CALL vxGraphParameterDequeueDoneRef(vx_graph graph,
            vx_uint32 graph_parameter_index,
            vx_reference *refs,
            vx_uint32 max_refs,
            vx_uint32 *num_refs);

/*! \brief Checks and returns the number of references that are ready for dequeue
 *
 * This function checks the number of references that can be dequeued and
 * returns the value to the application.
 *
 * See also <tt>\ref vxGraphParameterDequeueDoneRef</tt>.
 *
 * \param [in] graph Graph reference
 * \param [in] graph_parameter_index Graph parameter index
 * \param [out] num_refs Number of references that can be dequeued using
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid reference
 * \retval VX_ERROR_INVALID_PARAMETERS graph_parameter_index is NOT a valid graph parameter index
 * \retval VX_FAILURE Any other failure.
 *
 * \ingroup group_pipelining
 */
VX_API_ENTRY vx_status VX_API_CALL vxGraphParameterCheckDoneRef(vx_graph graph,
            vx_uint32 graph_parameter_index,
            vx_uint32 *num_refs);

/*
 * EVENT QUEUE API
 */

/*! \brief Extra enums.
 *
 * \ingroup group_event
 */
enum vx_event_enum_e
{
    VX_ENUM_EVENT_TYPE     = 0x1D, /*!< \brief Event Type enumeration. */
};

/*! \brief Type of event that can be generated during system execution
 *
 * \ingroup group_event
 */
enum vx_event_type_e {

    /*! \brief Graph parameter consumed event
     *
     * This event is generated when a data reference at a graph parameter
     * is consumed during a graph execution.
     * It is used to indicate that a given data reference is no longer used by the graph and can be
     * dequeued and accessed by the application.
     *
     * \note Graph execution could still be "in progress" for rest of the graph that does not use
     * this data reference.
     */
    VX_EVENT_GRAPH_PARAMETER_CONSUMED = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_EVENT_TYPE << 12)) + 0x0,

    /*! \brief Graph completion event
     *
     * This event is generated every time a graph execution completes.
     * Graph completion event is generated for both successful execution of a graph
     * or abandoned execution of a graph.
     */
    VX_EVENT_GRAPH_COMPLETED = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_EVENT_TYPE << 12)) + 0x1,

    /*! \brief Node completion event
     *
     * This event is generated every time a node within a graph completes execution.
     */
    VX_EVENT_NODE_COMPLETED = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_EVENT_TYPE << 12)) + 0x2,

    /*! \brief User defined event
     *
     * This event is generated by user application outside of OpenVX framework using the \ref vxSendUserEvent API.
     * User events allow application to have single centralized 'wait-for' loop to handle
     * both framework generated events as well as user generated events.
     */
    VX_EVENT_USER = ((( VX_ID_KHRONOS ) << 20) | ( VX_ENUM_EVENT_TYPE << 12)) + 0x3
};

/*! \brief Parameter structure returned with event of type VX_EVENT_GRAPH_PARAMETER_CONSUMED
 *
 * \ingroup group_event
 */
struct _vx_event_graph_parameter_consumed {

    vx_graph graph;
    /*!< \brief graph which generated this event */

    vx_uint32 graph_parameter_index;
    /*!< \brief graph parameter index which generated this event */
};

/*! \brief Parameter structure returned with event of type VX_EVENT_GRAPH_COMPLETED
 *
 * \ingroup group_event
 */
struct _vx_event_graph_completed {

    vx_graph graph;
    /*!< \brief graph which generated this event */
};

/*! \brief Parameter structure returned with event of type VX_EVENT_NODE_COMPLETED
 *
 * \ingroup group_event
 */
struct _vx_event_node_completed {

    vx_graph graph;
    /*!< \brief graph which generated this event */

    vx_node node;
    /*!< \brief node which generated this event */
};

/*! \brief Parameter structure returned with event of type VX_EVENT_USER_EVENT
 *
 * \ingroup group_event
 */
struct _vx_event_user_event {

    vx_uint32 user_event_id;
    /*!< \brief user event ID associated with this event */

    void *user_event_parameter;
    /*!< \brief User defined parameter value. This is used to pass additional user defined parameters with a user event.
     */
};

/*! \brief Data structure which holds event information
 *
 * \ingroup group_event
 */
typedef struct _vx_event {

    vx_enum type;
    /*!< \brief see event type \ref vx_event_type_e */

    vx_uint64 timestamp;
    /*!< time at which this event was generated, in units of nano-secs */

    union {
        struct _vx_event_graph_parameter_consumed graph_parameter_consumed; /*!< event information for type: \ref VX_EVENT_GRAPH_PARAMETER_CONSUMED */
        struct _vx_event_graph_completed graph_completed; /*!< event information for type: \ref VX_EVENT_GRAPH_COMPLETED */
        struct _vx_event_node_completed node_completed; /*!< event information for type: \ref VX_EVENT_NODE_COMPLETED */
        struct _vx_event_user_event user_event; /*!< event information for type: \ref VX_EVENT_USER */
    } event_info;
    /*!< parameter structure associated with a event. Depends on type of the event */

} vx_event_t;

/*! \brief Wait for a single event
 *
 * After <tt> \ref vxDisableEvents </tt> is called, if <tt> \ref vxWaitEvent(.. ,.. , vx_false_e) </tt> is called,
 * <tt> \ref vxWaitEvent </tt> will remain blocked until events are re-enabled using <tt> \ref vxEnableEvents </tt>
 * and a new event is received.
 *
 * If <tt> \ref vxReleaseContext </tt> is called while a application is blocked on <tt> \ref vxWaitEvent </tt>, the
 * behavior is not defined by OpenVX.
 *
 * If <tt> \ref vxWaitEvent </tt> is called simultaneously from multiple thread/task contexts
 * then its behaviour is not defined by OpenVX.
 *
 * \param context [in] OpenVX context
 * \param event [out] Data structure which holds information about a received event
 * \param do_not_block [in] When value is vx_true_e API does not block and only checks for the condition
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS Event received and event information available in 'event'
 * \retval VX_FAILURE No event is received
 *
 * \ingroup group_event
 */
VX_API_ENTRY vx_status VX_API_CALL vxWaitEvent(vx_context context, vx_event_t *event, vx_bool do_not_block);

/*! \brief Enable event generation
 *
 * \param context [in] OpenVX context
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 *
 * \ingroup group_event
 */
VX_API_ENTRY vx_status VX_API_CALL vxEnableEvents(vx_context context);

/*! \brief Disable event generation
 *
 * When events are disabled, any event generated before this API is
 * called will still be returned via \ref vxWaitEvent API.
 * However no additional events would be returned via \ref vxWaitEvent API
 * until events are enabled again.
 *
 * \param context [in] OpenVX context
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 *
 * \ingroup group_event
 */
VX_API_ENTRY vx_status VX_API_CALL vxDisableEvents(vx_context context);

/*! \brief Generate user defined event
 *
 * \param context [in] OpenVX context
 * \param user_event_id [in] User defined event ID
 * \param user_event_parameter [in] User defined event parameter. NOT used by implementation.
 *                                   Returned to user as part \ref vx_event_t.user_event_parameter field
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 *
 * \ingroup group_event
 */
VX_API_ENTRY vx_status VX_API_CALL vxSendUserEvent(vx_context context, vx_uint32 id, void *parameter);


/*! \brief Register an event to be generated
 *
 * Generation of event may need additional resources and overheads for an implementation.
 * Hence events should be registered for references only when really required by an application.
 *
 * This API can be called on graph, node or graph parameter.
 * This API MUST be called before doing \ref vxVerifyGraph for that graph.
 *
 * \param ref [in] Reference which will generate the event
 * \param type [in] Type or condition on which the event is generated
 * \param param [in] Specifies the graph parameter index when type is VX_EVENT_GRAPH_PARAMETER_CONSUMED
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 * \retval VX_ERROR_INVALID_REFERENCE ref is not a valid <tt>\ref vx_reference</tt> reference.
 * \retval VX_ERROR_NOT_SUPPORTED type is not valid for the provided reference.
 *
 * \ingroup group_event
 */
VX_API_ENTRY vx_status VX_API_CALL vxRegisterEvent(vx_reference ref, enum vx_event_type_e type, vx_uint32 param);

/*
 * STREAMING API
 */

/*! \brief Start streaming mode of graph execution
 *
 * In streaming mode of graph execution, once a application starts graph execution
 * further intervention of the application is not needed to re-schedule a graph;
 * i.e. a graph re-schedules itself and executes continuously until streaming mode of execution is stopped.
 *
 * When this API is called, the framework schedules the graph via <tt>\ref vxScheduleGraph</tt> and
 * returns.
 * This graph gets re-scheduled continuously until <tt>\ref vxStopGraphStreaming</tt> is called by the user
 * or any of the graph nodes return error during execution.
 *
 * The graph MUST be verified via \ref vxVerifyGraph before calling this API.
 * Also user application MUST ensure no previous executions of the graph are scheduled before calling this API.
 *
 * After streaming mode of a graph has been started, the following APIs should \a \b not be used on that graph by an application:
 * <tt>\ref vxScheduleGraph</tt>, <tt>\ref vxWaitScheduleGraphDone</tt>, and
 * <tt>\ref vxIsScheduleGraphAllowed</tt>.
 *
 * <tt>\ref vxWaitGraph</tt> can be used as before to wait for all pending graph executions
 * to complete.
 *
 * \param graph [in] Reference to the graph to start streaming mode of execution.
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid <tt>\ref vx_graph</tt> reference.
 *
 * \ingroup group_streaming
 */
VX_API_ENTRY vx_status vxStartGraphStreaming(vx_graph graph);


/*! \brief Stop streaming mode of graph execution
 *
 * This function blocks until graph execution is gracefully stopped at a logical boundary, for example,
 * when all internally scheduled graph executions are completed.
 *
 * \param graph [in] Reference to the graph to stop streaming mode of execution.
 *
 * \return A <tt>\ref vx_status_e</tt> enumeration.
 * \retval VX_SUCCESS No errors; any other value indicates failure.
 * \retval VX_FAILURE Graph is not started in streaming execution mode.
 * \retval VX_ERROR_INVALID_REFERENCE graph is not a valid reference.
 *
 * \ingroup group_streaming
 */
VX_API_ENTRY vx_status vxStopGraphStreaming(vx_graph graph);

#ifdef  __cplusplus
}
#endif

#endif
