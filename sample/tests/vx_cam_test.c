/* 

 * Copyright (c) 2011-2017 The Khronos Group Inc.
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
#include <VX/vxu.h>

#include <VX/vx_helper.h>
#if defined(OPENVX_USE_XML)
#include <VX/vx_khr_xml.h>
#endif
#include <VX/vx_lib_debug.h>
#include <VX/vx_lib_extras.h>

#include <SDL/SDL.h>

#include <assert.h>
#include <linux/videodev2.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#define MAX_PATH        (256)
#define MAX_DISPLAY     (4)
#define MAX_CAPTURE     (4)

vx_graph vxPyramidIntegral(vx_context context, vx_image image, vx_threshold thresh, vx_image output)
{
    vx_graph graph = vxCreateGraph(context);
    vx_uint32 width, height, i;
    vxQueryImage(image, VX_IMAGE_WIDTH, &width, sizeof(width));
    vxQueryImage(image, VX_IMAGE_HEIGHT, &height, sizeof(height));
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_pyramid pyr = vxCreatePyramid(context, 4, 0.5f, width, height, VX_DF_IMAGE_U8);
        vx_uint32 shift16 = 16, shift8 = 8;
        vx_scalar sshift16 = vxCreateScalar(context, VX_TYPE_UINT32, &shift16);
        vx_scalar sshift8 = vxCreateScalar(context, VX_TYPE_UINT32, &shift8);
        vx_image images[] = {
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_VIRT),
                vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16),
        };
        vx_node nodes[] = {
                vxGaussianPyramidNode(graph, image, pyr),
                vxIntegralImageNode(graph, vxGetPyramidLevel(pyr, 0), images[0]),
                vxIntegralImageNode(graph, vxGetPyramidLevel(pyr, 1), images[1]),
                vxIntegralImageNode(graph, vxGetPyramidLevel(pyr, 2), images[2]),
                vxIntegralImageNode(graph, vxGetPyramidLevel(pyr, 3), images[3]),
                vxConvertDepthNode(graph, images[0], images[4], VX_CONVERT_POLICY_WRAP, sshift16),
                vxConvertDepthNode(graph, images[4], output, VX_CONVERT_POLICY_WRAP, sshift8),
        };
        vxAddParameterToGraphByIndex(graph, nodes[0], 0);
        vxAddParameterToGraphByIndex(graph, nodes[5], 1);
        for (i = 0; i < dimof(images); i++)
            vxReleaseImage(&images[i]);
        for (i = 0; i < dimof(images); i++)
            vxReleaseNode(&nodes[i]);
        vxReleasePyramid(&pyr);
    }
    return graph;
}

vx_graph vxPseudoCannyGraph(vx_context context, vx_image input, vx_threshold thresh, vx_image output)
{
    vx_graph graph = vxCreateGraph(context);
    if (vxGetStatus((vx_reference)graph) == VX_SUCCESS)
    {
        vx_uint32 n = 0;
        vx_image virts[] = {
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_S16),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
            vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8),
        };
        vx_node nodes[] = {
            vxChannelExtractNode(graph, input, VX_CHANNEL_Y, virts[5]),
            vxSobel3x3Node(graph, virts[5], virts[0], virts[1]),
            vxMagnitudeNode(graph, virts[0], virts[1], virts[2]),
            vxPhaseNode(graph, virts[0], virts[1], virts[3]),
            vxNonMaxSuppressionNode(graph, virts[2], virts[3], virts[4]),
            vxThresholdNode(graph, virts[4], thresh, virts[6]),
            vxDilate3x3Node(graph, virts[6], virts[7]),
            vxChannelCombineNode(graph, virts[7], virts[7], virts[7], 0, output),
            vxFWriteImageNode(graph, virts[5], "luma_320x240_P400.bw"),
            vxFWriteImageNode(graph, virts[7], "canny_320x240_P400.bw"),
        };
        vxAddParameterToGraphByIndex(graph, nodes[0], 0);
        vxAddParameterToGraphByIndex(graph, nodes[5], 1);
        vxAddParameterToGraphByIndex(graph, nodes[7], 4);
        for (n = 0; n < dimof(nodes); n++)
            vxReleaseNode(&nodes[n]);
        for (n = 0; n < dimof(virts); n++)
            vxReleaseImage(&virts[n]);
    }
    return graph;
}

int32_t v4l2_start(int32_t dev) {
    uint32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int32_t ret = ioctl(dev, VIDIOC_STREAMON, &type);
    return (ret == 0?1:0);
}

int32_t v4l2_stop(int32_t dev) {
    int32_t type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int32_t ret = ioctl(dev, VIDIOC_STREAMOFF, &type);
    return (ret == 0?1:0);
}

int32_t v4l2_control_set(int32_t dev, uint32_t control, int32_t value)
{
    struct v4l2_control ctrl = {0};
    struct v4l2_queryctrl qctrl = {0};
    qctrl.id = control;
    if (ioctl(dev, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if ((qctrl.flags & V4L2_CTRL_TYPE_BOOLEAN) ||
            (qctrl.type  & V4L2_CTRL_TYPE_INTEGER))
        {
            int min = qctrl.minimum;
            int max = qctrl.maximum;
            int step = qctrl.step;
            printf("Ctrl: %d Min:%d Max:%d Step:%d\n",control,min,max,step);
            if ((min <= value) && (value <= max)) {
                if (step > 1)
                    value = value - (value % step);
            } else {
                value = qctrl.default_value;
            }
            ctrl.id = control;
            ctrl.value = value;
            if (ioctl(dev, VIDIOC_S_CTRL, &ctrl) == 0)
                return 1;
        }
    }
    return 0;
}

int32_t v4l2_control_get(int32_t dev, uint32_t control, int32_t *value)
{
    struct v4l2_control ctrl = {0};
    struct v4l2_queryctrl qctrl = {0};
    qctrl.id = control;
    if (ioctl(dev, VIDIOC_QUERYCTRL, &qctrl) == 0) {
        if ((qctrl.flags & V4L2_CTRL_TYPE_BOOLEAN) ||
            (qctrl.type  & V4L2_CTRL_TYPE_INTEGER)) {
            printf("Ctrl: %s Min:%d Max:%d Step:%d dflt:%d\n",
                    qctrl.name,
                    qctrl.minimum,
                    qctrl.maximum,
                    qctrl.step,
                    qctrl.default_value);
            ctrl.id = control;
            if (ioctl(dev, VIDIOC_G_CTRL, &ctrl) == 0) {
                *value = ctrl.value;
                printf("Ctrl: %s Value:%d\n",qctrl.name, ctrl.value);
                return 1;
            }
        }
    }
    return 0;
}

int32_t v4l2_reset_control(int32_t dev, uint32_t control)
{
    struct v4l2_control ctrl = {0};
    struct v4l2_queryctrl qctrl = {0};
    int ret = 0;

    qctrl.id = control;
    ret = ioctl(dev, VIDIOC_QUERYCTRL, &qctrl);
    if ((qctrl.flags & V4L2_CTRL_TYPE_BOOLEAN) ||
        (qctrl.type  & V4L2_CTRL_TYPE_INTEGER)) {
        ctrl.id = control;
        ctrl.value = qctrl.default_value;
        ret = ioctl(dev, VIDIOC_S_CTRL, &ctrl);
    }
    return (ret == 0 ? 1:0);
}

void v4l2_close(int32_t dev) {
    close(dev);
}

int32_t v4l2_open(uint32_t devnum, uint32_t capabilities) {
    char devname[MAX_PATH] = {0};
    struct v4l2_capability cap = {{0},{0},{0},0,0,0};
    int32_t cnt = snprintf(devname, sizeof(devname), "/dev/video%u", devnum);
    int32_t dev = open(devname, O_RDWR);
    int32_t ret = ioctl(dev, VIDIOC_QUERYCAP, &cap);
    if (cnt == 0 || ret || (capabilities & cap.capabilities) != capabilities) {
        close(dev);
        dev = -1;
    }
    return dev;
}

typedef struct _v4l2_pix_to_df_image_lut_t {
    uint32_t v4l2;
    vx_df_image df_image;
    uint32_t stride_x;
    vx_enum space;
} V4L2_to_VX_DF_IMAGE_t;

V4L2_to_VX_DF_IMAGE_t codes[] = {
    {V4L2_PIX_FMT_UYVY,  VX_DF_IMAGE_UYVY, sizeof(uint16_t), VX_COLOR_SPACE_BT601_625},
    {V4L2_PIX_FMT_YUYV,  VX_DF_IMAGE_YUYV, sizeof(uint16_t), VX_COLOR_SPACE_BT601_625}, // capture
    {V4L2_PIX_FMT_YUYV,  VX_DF_IMAGE_YUYV, sizeof(uint16_t), VX_COLOR_SPACE_BT601_625},
    {V4L2_PIX_FMT_NV12,  VX_DF_IMAGE_NV12, sizeof(uint8_t), VX_COLOR_SPACE_BT601_625},
  //{V4L2_PIX_FMT_BGR24, VX_DF_IMAGE_BGR, sizeof(uint8_t), 0}, // I don't think this is supported by capture or displays we use
    {V4L2_PIX_FMT_RGB24, VX_DF_IMAGE_RGB, sizeof(uint8_t), 0},
    {V4L2_PIX_FMT_RGB32, VX_DF_IMAGE_RGBX, sizeof(uint8_t), 0}, // we'll use a global alpha
};
uint32_t numCodes = dimof(codes);

void print_buffer(struct v4l2_buffer *desc) {
    printf("struct v4l2_buffer [%u] type:%u used:%u len:%u flags=0x%08x field:%u memory:%u offset:%u\n",
        desc->index,
        desc->type,
        desc->bytesused,
        desc->length,
        desc->flags,
        desc->field,
        desc->memory,
        desc->m.offset);
}

uint32_t v4l2_dequeue(int32_t dev) {
    struct v4l2_buffer buf = {0};
    int32_t ret;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl(dev, VIDIOC_DQBUF, &buf);
    print_buffer(&buf);
    return (ret == 0 ? buf.index : UINT32_MAX);
}

int32_t v4l2_queue(int32_t dev, uint32_t index) {
    struct v4l2_buffer buf = {0};
    int32_t ret = 0;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.index = index;
    ret = ioctl(dev, VIDIOC_QUERYBUF, &buf);
    buf.bytesused = 0;
    buf.field = V4L2_FIELD_ANY;
    ret = ioctl(dev, VIDIOC_QBUF, &buf);
    return (ret==0?1:0);
}

// C99
int32_t vxReleaseV4L2Images(vx_uint32 count, vx_image images[count], void *ptrs[count], int32_t dev) {
    uint32_t i = 0;
    int32_t ret = 0;
    struct v4l2_requestbuffers reqbuf = {0};
    for (i = 0; i < count; i++) {
        struct v4l2_buffer buf;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index = i;
        ret = ioctl(dev, VIDIOC_QUERYBUF, &buf);
        vxReleaseImage(&images[i]);
        munmap(ptrs[i], buf.length);
    }
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 0;
    ret = ioctl(dev, VIDIOC_REQBUFS, &reqbuf);
    return (ret == 0?1:0);
}
// C99
int32_t vxCreateV4L2Images(vx_context context, vx_uint32 count, vx_image images[count], vx_uint32 width, vx_uint32 height, vx_df_image format,
                            int32_t dev, void *ptrs[count]) {
    uint32_t c, i;
    struct v4l2_format fmt = {0};
    struct v4l2_requestbuffers reqbuf = {0};
    int32_t ret = 0;
    struct v4l2_buffer buffers[count];

    ret = ioctl(dev, VIDIOC_G_FMT, &fmt);
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = 0u;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    for (c = 0; c < numCodes; c++) {
        if (format == codes[c].df_image) {
            fmt.fmt.pix.pixelformat = codes[c].v4l2;
            break;
        }
    }
    if (c == numCodes) {
        return 0;
    }
    ret = ioctl(dev, VIDIOC_S_FMT, &fmt);
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = count;
    ret = ioctl(dev, VIDIOC_REQBUFS, &reqbuf);
    for (i = 0; i < count; i++) {
        buffers[i].type = reqbuf.type;
        buffers[i].memory = reqbuf.memory;
        buffers[i].index = i;
        ret = ioctl(dev, VIDIOC_QUERYBUF, &buffers[i]);
        print_buffer(&buffers[i]);
        if (buffers[i].flags != V4L2_BUF_FLAG_MAPPED) {
            ptrs[i] = mmap(NULL, buffers[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, dev, buffers[i].m.offset);
            if (ptrs[i] == MAP_FAILED) {
                // failure
                printf("Failed to map buffer!\n");
                exit(-1);
            } else {
                vx_imagepatch_addressing_t addr = {
                    .dim_x = width,
                    .dim_y = height,
                    .stride_x = codes[c].stride_x,
                    .stride_y = width*codes[c].stride_x,
                    .scale_x = VX_SCALE_UNITY,
                    .scale_y = VX_SCALE_UNITY,
                    .step_x = 1,
                    .step_y = 1,
                };
                images[i] = vxCreateImageFromHandle(context, format, &addr, &ptrs[i], VX_IMPORT_TYPE_HOST);
                vxSetImageAttribute(images[i], VX_IMAGE_SPACE, &codes[c].space, sizeof(codes[c].space));
                if (vxGetStatus((vx_reference)images[i]) != VX_SUCCESS)
                    vxReleaseImage(&images[i]);
                buffers[i].bytesused = 0;
                buffers[i].field = V4L2_FIELD_ANY;
                print_buffer(&buffers[i]);
                ret = ioctl(dev, VIDIOC_QBUF, &buffers[i]);
                ret = ioctl(dev, VIDIOC_QUERYBUF, &buffers[i]);
                if (!(buffers[i].flags & V4L2_BUF_FLAG_QUEUED)) {
                    printf("Failed to Queue Buffer [%u]!\n", i);
                    print_buffer(&buffers[i]);
                }
            }
        } else {
            printf("Buffer already mapped\n");
        }
    }
    return (ret == 0?1:0);
}

int main(int argc, char *argv[])
{
    uint32_t width = 320, height = 240, count = 10;
    vx_df_image format = VX_DF_IMAGE_YUYV;
    SDL_Surface *screen = NULL;
    SDL_Surface *backplanes[MAX_DISPLAY] = {NULL};
    SDL_Rect src, dst;
    void *captures[MAX_CAPTURE];
    vx_image images[dimof(captures) + dimof(backplanes)] = {0};
    vx_uint32 camIdx = 0, dispIdx = 0;
    vx_context context = vxCreateContext();
    vx_status status = VX_SUCCESS;
    int cam = v4l2_open(0, V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING);
    vx_graph graph = 0;
    vx_threshold thresh = vxCreateThreshold(context, VX_THRESHOLD_TYPE_RANGE, VX_TYPE_UINT8);
    vx_char xmlfile[MAX_PATH];

    vxLoadKernels(context, "openvx-debug");
    vxLoadKernels(context, "openvx-extras");
    vxRegisterHelperAsLogReader(context);

    if (cam == -1)
        exit(-1);

    if (argc > 1)
        count = atoi(argv[1]);
    if (argc > 2)
        strncpy(xmlfile, argv[2], MAX_PATH);
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        exit(-1);
    }
    screen = SDL_SetVideoMode(width, height, 24, SDL_SWSURFACE | SDL_DOUBLEBUF);
    SDL_GetClipRect(screen, &dst);
    printf("Rect: {%d, %d}, %ux%u\n", dst.x, dst.y, dst.w, dst.h);
    SDL_WM_SetCaption( "OpenVX", NULL );
    if (context && cam)
    {
        // describes YUYV
        uint32_t s,i = 0;

        if (vxCreateV4L2Images(context, MAX_CAPTURE, images, width, height, format, cam, captures))
        {
            i += MAX_CAPTURE;
            // create the display images as the backplanes
            for (s = 0; s < dimof(backplanes); s++,i++)
            {
                vx_uint32 depth = 24;
                vx_imagepatch_addressing_t map[] = {{
                        width, height,
                        (depth/8)*sizeof(vx_uint8), width*(depth/8)*sizeof(vx_uint8),
                        VX_SCALE_UNITY, VX_SCALE_UNITY,
                        1,1
                }};
                void *ptrs[1];
                backplanes[s] = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                                        width,
                                                        height,
                                                        depth,
                                                        0x000000FF,
                                                        0x0000FF00,
                                                        0x00FF0000,
                                                        0x00000000);
                assert(backplanes[s] != NULL);
                ptrs[0] = backplanes[s]->pixels;
                map[0].stride_y = backplanes[s]->pitch; // incase mapped to 2D memory
                printf("Mapping %p with stride %d to a vx_image\n", ptrs[0], map[0].stride_y);
                images[i] = vxCreateImageFromHandle(context, VX_DF_IMAGE_RGB, map, ptrs, VX_IMPORT_TYPE_HOST);
                assert(images[i] != 0);
                SDL_GetClipRect(backplanes[s], &src);
                printf("Rect: {%d, %d}, %ux%u\n", src.x, src.y, src.w, src.h);
            }
        }
        {
            uint32_t c, ctrls[][2]= {
                    {V4L2_CID_BRIGHTNESS,0},
                    {V4L2_CID_CONTRAST,0},
                    {V4L2_CID_SATURATION,0},
                    {V4L2_CID_GAIN,0},
            };
            // check the controls
            for (c = 0; c < dimof(ctrls); c++)
            {
                if (v4l2_reset_control(cam, ctrls[c][0]) == 0)
                {
                    printf("Control %u is not supported\n", ctrls[c][0]);
                }
            }
            for (c = 0; c < dimof(ctrls); c++)
            {
                if (v4l2_control_get(cam, ctrls[c][0], (int32_t *)&ctrls[c][1]) == 0)
                {
                    printf("Control %u is not supported\n", ctrls[c][0]);
                }
            }
        }
        assert(i == (dimof(backplanes)+dimof(captures)));
        {
            vx_int32 bounds[2] = {20, 180};
            vxSetThresholdAttribute(thresh, VX_THRESHOLD_THRESHOLD_LOWER, &bounds[0], sizeof(bounds[0]));
            vxSetThresholdAttribute(thresh, VX_THRESHOLD_THRESHOLD_UPPER, &bounds[1], sizeof(bounds[1]));
            // input is the YUYV image.
            // output is the RGB image
            graph = vxPseudoCannyGraph(context, images[0], thresh, images[dimof(captures)]);
            //graph = vxPyramidIntegral(context, images[0], thresh, images[dimof(captures)]);
            if (vxGetStatus((vx_reference)graph) == VX_SUCCESS) {
                status = vxVerifyGraph(graph);
                if (status != VX_SUCCESS) {
                    printf("Graph failed verification!\n");
                    vxClearLog((vx_reference)graph);
                    exit(-1);
                } else {
                    printf("Graph is verified!\n");
                }
            }
        }
        if (v4l2_start(cam)) {
            do {
                uint32_t cap = (size_t)v4l2_dequeue(cam);
                if (cap != UINT32_MAX) {
                    printf("Index : %u\n", cap);
                    camIdx = cap;
                    dispIdx = cap + dimof(captures);
                    printf("camIdx = %u, dispIdx = %u\n", camIdx, dispIdx);
                    SDL_LockSurface(backplanes[cap]);
                    status = vxSetGraphParameterByIndex(graph, 0, (vx_reference)images[camIdx]);
                    assert(status == VX_SUCCESS);
                    status = vxSetGraphParameterByIndex(graph, 2, (vx_reference)images[dispIdx]);
                    assert(status == VX_SUCCESS);
                    if (vxIsGraphVerified(graph) == vx_false_e) {
                        status = vxVerifyGraph(graph);
                        if (status != VX_SUCCESS) {
                            printf("Verify Status = %d\n", status);
                            exit(-1);
                        }
                    }
                    status = vxProcessGraph(graph);
                    if (status != VX_SUCCESS) {
                        printf("status = %d\n", status);
                        do {
                            char message[VX_MAX_LOG_MESSAGE_LEN] = {0};
                            status = vxGetLogEntry((vx_reference)graph, message);
                            if (status != VX_SUCCESS)
                                printf("Message:[%d] %s\n", status, message);
                        } while (status != VX_SUCCESS);
                    }
                    SDL_UnlockSurface(backplanes[cap]);
                    SDL_BlitSurface(backplanes[cap], &src, screen, &dst);
                    SDL_Flip(screen);
                    if (v4l2_queue(cam, cap) == 0)
                        printf("Failed to queue image!\n");
                } else {
                    printf("Failed to dequeue!\n");
                    break;
                }
            } while (count--);
            v4l2_stop(cam);
#if defined(OPENVX_USE_XML)
            status = vxExportToXML(context, xmlfile);
#endif
        } else {
            printf("Failed to start capture!\n");
        }
        vxReleaseV4L2Images(dimof(captures), images, captures, cam);
        for (s = 0; s < dimof(backplanes); s++) {
            SDL_FreeSurface(backplanes[s]);
        }
        SDL_FreeSurface(screen);
        vxReleaseThreshold(&thresh);
        vxReleaseGraph(&graph);
        v4l2_close(cam);

        vxUnloadKernels(context, "openvx-extras");
        vxUnloadKernels(context, "openvx-debug");
        vxReleaseContext(&context);
    }
    SDL_Quit();
    return 0;
}
