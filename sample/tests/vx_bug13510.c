/*
 Description : Bug 13510 - Infinite recursion and child graphs on nodes
 */

#include <stdio.h>
#include <stdlib.h>
#include <VX/vx.h>

static vx_status execute_and_change_int32_parameter(vx_graph graph, vx_node node, int paramno,
						    vx_int32 newval)
{
    /* Verify and process the graph */
    vx_context context = vxGetContext((vx_reference)graph);
    vx_status status = vxVerifyGraph(graph);
    if (status == VX_SUCCESS) {
        status = vxProcessGraph(graph);
    }

    /* Set new parameter on the function node */
    vx_int32 uint_value = newval;
    vx_scalar vx_value = vxCreateScalar(context, VX_TYPE_INT32, &uint_value);
    vxSetParameterByIndex(node, paramno, (vx_reference)vx_value);

    /* Verify and process the graph a second time */
    status = vxVerifyGraph(graph);
    if (status == VX_SUCCESS) {
        status = vxProcessGraph(graph);
    }

    /* Also verify that we get the correct value when reading it back. */
    vx_scalar scalar_out;
    vx_parameter par_out = vxGetParameterByIndex(node, paramno);
    vxQueryParameter(par_out, VX_PARAMETER_REF, &scalar_out, sizeof(scalar_out));

    vx_int32 value_out;
    vxCopyScalar(scalar_out, &value_out, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);

    if (value_out != newval) {
        fprintf(stderr, "got %d, expected %d\n", value_out, newval);
        abort ();
    }

    vxReleaseScalar(&scalar_out);
    vxReleaseParameter(&par_out);
    return status;
}

int main(void) {
    /* Create the graph */
    vx_context context = vxCreateContext();
    vx_pixel_value_t value = {{ 8 }};
    vx_pixel_value_t value2 = {{ 16 }};
    vx_status status;

    vx_image images[] = {
      vxCreateUniformImage(context, 640, 480, VX_DF_IMAGE_U8, &value),
      vxCreateUniformImage(context, 320, 240, VX_DF_IMAGE_U8, &value),
      vxCreateUniformImage(context, 640, 480, VX_DF_IMAGE_S16, &value2),
      vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8),
    };

    vx_graph graph_ok = vxCreateGraph(context);

    /* First, correct behavior using a correctly-working node (that has
       a scalar parameter). */
    vx_uint32 shift_value = 3;
    vx_scalar shift_scalar = vxCreateScalar(context, VX_TYPE_INT32, &shift_value);
    vx_node NodeOk = vxConvertDepthNode(graph_ok, images[2], images[3], VX_CONVERT_POLICY_WRAP, shift_scalar);

    status = execute_and_change_int32_parameter(graph_ok, NodeOk, 3, 4);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "badness 1\n");
        abort ();
    }

    vx_graph graph_bug = vxCreateGraph(context);

    /* Then, a second run, showing the incorrect behavior for vxHalfScaleGaussianNode. */
    vx_node NodeBug = vxHalfScaleGaussianNode(graph_bug, images[0], images[1], 3);

    status = execute_and_change_int32_parameter(graph_bug, NodeBug, 2, 5);
    if (status != VX_SUCCESS) {
        fprintf(stderr, "badness 2\n");
        abort ();
    }

    exit (0);
 }
