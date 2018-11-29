/*
 ============================================================================
 Description : Bug 13518 - Reverse order dependence
               generates VX_ERROR_INVALID_DIMENSION for vxConvertDepthNode
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <VX/vx.h>

int main(void)
{
   vx_context context = vxCreateContext();
   vx_pixel_value_t value = {{ 8 }};
   vx_graph graph = vxCreateGraph(context);
   vx_image images[] = {
     vxCreateUniformImage(context, 640, 480, VX_DF_IMAGE_U8, &value),
     vxCreateImage(context, 640, 480, VX_DF_IMAGE_U8)
   };
   vx_image intermediate_images[] = {
     vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16)
   };

   /*Create the depth conversion nodes*/
   vx_uint32 uint_value2 = 0;
   vx_scalar vx_value2 = vxCreateScalar(context, VX_TYPE_INT32, &uint_value2);
   vx_uint32 uint_value1 = 0;
   vx_scalar vx_value1 = vxCreateScalar(context, VX_TYPE_INT32, &uint_value1);

   /* The order in which these two nodes are created should not matter. */
   vxConvertDepthNode(graph, intermediate_images[0], images[1],
                      VX_CONVERT_POLICY_SATURATE, vx_value2);

   vxConvertDepthNode(graph, images[0], intermediate_images[0],
                      VX_CONVERT_POLICY_SATURATE, vx_value1);

   vx_status status = vxVerifyGraph(graph);
   if (status == VX_SUCCESS) {
       status = vxProcessGraph(graph);
   }

   if (status != VX_SUCCESS) {
       fprintf(stderr, "badness\n");
       abort ();
   }

   exit (0);
}
