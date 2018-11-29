/*
 ============================================================================
 Description : Bug 13517 - VX_ERROR_INVALID_SCOPE error when using virtual images.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <VX/vx.h>

int main(void) {
    vx_context context = vxCreateContext();
    vx_pixel_value_t value = {{ 8 }};
    vx_image images[] = {
      vxCreateUniformImage(context, 320, 240, VX_DF_IMAGE_U8, &value),
      vxCreateImage(context, 80, 60, VX_DF_IMAGE_U8),
      vxCreateUniformImage(context, 320, 240, VX_DF_IMAGE_U8, &value),
      vxCreateImage(context, 80, 60, VX_DF_IMAGE_U8)
    };
    vx_graph graph = vxCreateGraph(context);
    vx_graph graph_this = vxCreateGraph(context);
    vx_graph graph_other = vxCreateGraph(context);
    vx_image virts[] = {
      vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
      vxCreateVirtualImage(graph_other, 0, 0, VX_DF_IMAGE_VIRT)
    };

    vxHalfScaleGaussianNode(graph, images[0], virts[0], 3);
    vxHalfScaleGaussianNode(graph, virts[0], images[1], 3);

    vx_status status = vxVerifyGraph(graph);
    if (status == VX_SUCCESS) {
        status = vxProcessGraph(graph);
    }

    if (status != VX_SUCCESS) {
       fprintf(stderr, "badness\n");
       abort();
    }

    /* Now do exactly the same (with "fresh" images and another graph),
       except using a virtual image with the scope of a different
       (non-ancestor-)graph and expect a scope-error during the
       verification phase. */
    vxHalfScaleGaussianNode(graph_this, images[2+0], virts[1], 3);
    vxHalfScaleGaussianNode(graph_this, virts[1], images[2+1], 3);

    status = vxVerifyGraph(graph_this);
    if (status != VX_ERROR_INVALID_SCOPE) {
       fprintf(stderr, "badness, error expected\n");
       abort();
    }
    /* (We can't process the graph due to that error.) */

    return 0;
}
