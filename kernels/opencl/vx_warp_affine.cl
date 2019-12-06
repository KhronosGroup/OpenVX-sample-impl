
#define VX_ID_KHRONOS    0x000
#define VX_ENUM_INTERPOLATION   0x04
#define VX_ENUM_BASE(vendor, id)            (((vendor) << 20) | (id << 12))
#define VX_INTERPOLATION_NEAREST_NEIGHBOR  VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_INTERPOLATION) + 0x0
#define VX_INTERPOLATION_BILINEAR          VX_ENUM_BASE(VX_ID_KHRONOS, VX_ENUM_INTERPOLATION) + 0x1

#define VEC_DATA_TYPE_STR(type, size) type##size
#define VEC_DATA_TYPE(type, size) VEC_DATA_TYPE_STR(type, size)

#define CONVERT_STR(x, type) (convert_##type((x)))
#define CONVERT(x, type) CONVERT_STR(x, type)

#define IMAGE_DECLARATION(name)      \
    __global uchar *name##_ptr,      \
    uint        name##_stride_x, \
    uint        name##_step_x,   \
    uint        name##_stride_y, \
    uint        name##_step_y,   \
    uint        name##_offset_first_element_in_bytes
	
#define CONVERT_TO_IMAGE_STRUCT(name) \
    update_image_workitem_ptr(name##_ptr, name##_offset_first_element_in_bytes, name##_stride_x, name##_step_x, name##_stride_y, name##_step_y)

#define CONVERT_TO_IMAGE_STRUCT_NO_STEP(name) \
    update_image_workitem_ptr(name##_ptr, name##_offset_first_element_in_bytes, name##_stride_x, 0, name##_stride_y, 0)
	
/** Structure to hold Image information */
typedef struct Image
{
    __global uchar *ptr;                           /**< Pointer to the starting postion of the buffer */
    int             offset_first_element_in_bytes; /**< The offset of the first element in the source image */
    int             stride_x;                      /**< Stride of the image in X dimension (in bytes) */
    int             stride_y;                      /**< Stride of the image in Y dimension (in bytes) */
} Image;

/** Wrap image information into an Image structure, and make the pointer point at this workitem's data.
*
* @param[in] ptr                           Pointer to the starting postion of the buffer
* @param[in] offset_first_element_in_bytes The offset of the first element in the source image
* @param[in] stride_x                      Stride of the image in X dimension (in bytes)
* @param[in] step_x                        stride_x * number of elements along X processed per workitem(in bytes)
* @param[in] stride_y                      Stride of the image in Y dimension (in bytes)
* @param[in] step_y                        stride_y * number of elements along Y processed per workitem(in bytes)
*
* @return An image object
*/
inline Image update_image_workitem_ptr(__global uchar *ptr, uint offset_first_element_in_bytes, uint stride_x, uint step_x, uint stride_y, uint step_y)
{
    Image img =
    {
        .ptr = ptr,
        .offset_first_element_in_bytes = offset_first_element_in_bytes,
        .stride_x = stride_x,
        .stride_y = stride_y
    };
    img.ptr += img.offset_first_element_in_bytes + get_global_id(0) * step_x + get_global_id(1) * step_y;
    return img;
}

/** Get the pointer position of a Image
*
* @param[in] img Pointer to the starting position of the buffer
* @param[in] x   Relative X position
* @param[in] y   Relative Y position
*/
inline __global uchar *offset(const Image *img, int x, int y)
{
    return img->ptr + x * img->stride_x + y * img->stride_y;
}

/** Clamps the given coordinates to the borders according to the border size.
 *
 * @param[in] coords      Vector of 2D coordinates to clamp. Even positions are X coords, odd positions are Y coords.
 * @param[in] width       Width of the image
 * @param[in] height      Height of the image
 * @param[in] border_size Border size of the image
 *
 */
inline const float8 clamp_to_border_with_size(float8 coords, const float width, const float height, const float border_size)
{
	const float4 clamped_x = clamp(coords.even, 0.0f - border_size, width - 1 + border_size);
    const float4 clamped_y = clamp(coords.odd, 0.0f - border_size, height - 1 + border_size);
	
    return (float8)(clamped_x.s0, clamped_y.s0, clamped_x.s1, clamped_y.s1, clamped_x.s2, clamped_y.s2, clamped_x.s3, clamped_y.s3);
}

/* FIXME(COMPMID-682): Clamp border properly in UNDEFINED border mode in Warp, Scale, Remap */
/** Clamps the given coordinates to the borders.
 *
 * @param[in] coords Vector of 2D coordinates to clamp. Even positions are X coords, odd positions are Y coords.
 * @param[in] width  Width of the image
 * @param[in] height Height of the image
 *
 */
inline const float8 clamp_to_border(float8 coords, const float width, const float height)
{
    return clamp_to_border_with_size(coords, width, height, 0);
}

/** Reads four texels from the input image. The coords vector is used to determine which texels to be read.
 *
 * @param[in] in     Pointer to the source image.
 * @param[in] coords Vector of coordinates to be read from the image.
 */
inline const VEC_DATA_TYPE(uchar, 4) read_texels4(const Image *in, const int8 coords)
{ 
    return (VEC_DATA_TYPE(uchar, 4))(*((__global uchar *)offset(in, coords.s0, coords.s1)),
                                         *((__global uchar *)offset(in, coords.s2, coords.s3)),
                                         *((__global uchar *)offset(in, coords.s4, coords.s5)),
                                         *((__global uchar *)offset(in, coords.s6, coords.s7)));
}

/** Returns the current thread coordinates. */
inline const float2 get_current_coords()
{
    return (float2)(get_global_id(0) * 4, get_global_id(1));
}

/** Transforms 4 2D coordinates using the formula:
 *
 *   x0 = M[1][1] * x + M[1][2] * y + M[1][3]
 *   y0 = M[2][1] * x + M[2][2] * y + M[2][3]
 *
 * @param[in] coord 2D coordinate to transform.
 * @param[in] mtx   affine matrix
 *
 * @return a int8 containing 4 2D transformed values.
 */
inline const float8 apply_affine_transform(const float2 coord, const float8 mtx)
{
    const float4 in_x_coords = (float4)(coord.s0, 1 + coord.s0, 2 + coord.s0, 3 + coord.s0);
    // transform [x,x+1,x+2,x+3]
    const float4 new_x = mad(/*A*/ in_x_coords, (float4)(mtx.s0) /*B*/, mad((float4)(coord.s1), (float4)(mtx.s2), (float4)(mtx.s4)));
    // transform [y,y+1,y+2,y+3]
    const float4 new_y = mad(in_x_coords, (float4)(mtx.s1), mad((float4)(coord.s1), (float4)(mtx.s3), (float4)(mtx.s5)));

    return (float8)(new_x.s0, new_y.s0, new_x.s1, new_y.s1, new_x.s2, new_y.s2, new_x.s3, new_y.s3);
}


/** Given a texel coordinates this function will return the following array of coordinates:
 * [ P, right neighbour, below neighbour, below right neighbour ]
 *
 * @note No checks to see if the coordinates are out of the image are done here.
 *
 * @param[in] coord Input coordinates
 *
 * @return vector of 8 floats with the coordinates, even positions are x and odd y.
 */
inline const float8 get_neighbour_coords(const float2 coord)
{
    return (float8)(/*tl*/ coord.s0, coord.s1, /*tr*/ coord.s0 + 1, coord.s1, /*bl*/ coord.s0, coord.s1 + 1, /*br*/ coord.s0 + 1, coord.s1 + 1);
}

/** Computes the bilinear interpolation for each set of coordinates in the vector coords and returns the values
 *
 * @param[in] in          Pointer to the source image.
 * @param[in] coords      Vector of four 2D coordinates. Even pos is x and odd y.
 * @param[in] width       Width of the image
 * @param[in] height      Height of the image
 * @param[in] border_size Border size
 */
inline const VEC_DATA_TYPE(uchar, 4) bilinear_interpolate_with_border(const Image *in, const float8 coords, const float width, const float height, const float border_size)
{
    // If any of the 4 texels is out of the image's boundaries we use the border value (REPLICATE or CONSTANT) for any texel out of the image.

    // Sets the 4x4 coordinates for each of the four input texels
    const float8  fc = floor(coords);
    const float16 c1 = (float16)(
                           clamp_to_border_with_size(get_neighbour_coords((float2)(fc.s0, fc.s1)), width, height, border_size),
                           clamp_to_border_with_size(get_neighbour_coords((float2)(fc.s2, fc.s3)), width, height, border_size));
    const float16 c2 = (float16)(
                           clamp_to_border_with_size(get_neighbour_coords((float2)(fc.s4, fc.s5)), width, height, border_size),
                           clamp_to_border_with_size(get_neighbour_coords((float2)(fc.s6, fc.s7)), width, height, border_size));

    // Loads the values from the input image
    const float16 t = (float16)(
                          /* tl, tr, bl, br */
                          * ((__global uchar *)offset(in, c1.s0, c1.s1)), *((__global uchar *)offset(in, c1.s2, c1.s3)),
                          *((__global uchar *)offset(in, c1.s4, c1.s5)), *((__global uchar *)offset(in, c1.s6, c1.s7)),
                          *((__global uchar *)offset(in, c1.s8, c1.s9)), *((__global uchar *)offset(in, c1.sa, c1.sb)),
                          *((__global uchar *)offset(in, c1.sc, c1.sd)), *((__global uchar *)offset(in, c1.se, c1.sf)),
                          *((__global uchar *)offset(in, c2.s0, c2.s1)), *((__global uchar *)offset(in, c2.s2, c2.s3)),
                          *((__global uchar *)offset(in, c2.s4, c2.s5)), *((__global uchar *)offset(in, c2.s6, c2.s7)),
                          *((__global uchar *)offset(in, c2.s8, c2.s9)), *((__global uchar *)offset(in, c2.sa, c2.sb)),
                          *((__global uchar *)offset(in, c2.sc, c2.sd)), *((__global uchar *)offset(in, c2.se, c2.sf)));
    const float8 a  = coords - fc;
    const float8 b  = ((float8)(1.f)) - a;
    const float4 fr = (float4)(
                          ((t.s0 * b.s0 * b.s1) + (t.s1 * a.s0 * b.s1) + (t.s2 * b.s0 * a.s1) + (t.s3 * a.s0 * a.s1)),
                          ((t.s4 * b.s2 * b.s3) + (t.s5 * a.s2 * b.s3) + (t.s6 * b.s2 * a.s3) + (t.s7 * a.s2 * a.s3)),
                          ((t.s8 * b.s4 * b.s5) + (t.s9 * a.s4 * b.s5) + (t.sa * b.s4 * a.s5) + (t.sb * a.s4 * a.s5)),
                          ((t.sc * b.s6 * b.s7) + (t.sd * a.s6 * b.s7) + (t.se * b.s6 * a.s7) + (t.sf * a.s6 * a.s7)));
    return CONVERT(fr, VEC_DATA_TYPE(uchar, 4));
}

/* FIXME(COMPMID-682): Clamp border properly in UNDEFINED border mode in Warp, Scale, Remap */
/** Computes the bilinear interpolation for each set of coordinates in the vector coords and returns the values
 *
 * @param[in] in     Pointer to the source image.
 * @param[in] coords Vector of four 2D coordinates. Even pos is x and odd y.
 * @param[in] width  Width of the image
 * @param[in] height Height of the image
 */
inline const VEC_DATA_TYPE(uchar, 4) bilinear_interpolate(const Image *in, const float8 coords, const float width, const float height)
{
    return bilinear_interpolate_with_border(in, coords, width, height, 1);
}

/** Performs an affine transform on an image interpolating with the NEAREAST NEIGHBOUR method. Input and output are single channel U8.
 *
 * This kernel performs an affine transform with a 2x3 Matrix M with this method of pixel coordinate translation:
 *   x0 = M[1][1] * x + M[1][2] * y + M[1][3]
 *   y0 = M[2][1] * x + M[2][2] * y + M[2][3]
 *   output(x,y) = input(x0,y0)
 *
 * @attention The matrix coefficients need to be passed at compile time:\n
 * const char build_options [] = "-DMAT0=1 -DMAT1=2 -DMAT2=1 -DMAT3=2 -DMAT4=4 -DMAT5=2 "\n
 * clBuildProgram( program, 0, NULL, build_options, NULL, NULL);
 *
 * @param[in]  in_ptr                            Pointer to the source image. Supported data types: U8.
 * @param[in]  in_stride_x                       Stride of the source image in X dimension (in bytes)
 * @param[in]  in_step_x                         in_stride_x * number of elements along X processed per work item (in bytes)
 * @param[in]  in_stride_y                       Stride of the source image in Y dimension (in bytes)
 * @param[in]  in_step_y                         in_stride_y * number of elements along Y processed per work item (in bytes)
 * @param[in]  in_offset_first_element_in_bytes  Offset of the first element in the source image
 * @param[out] out_ptr                           Pointer to the destination image. Supported data types: U8.
 * @param[in]  out_stride_x                      Stride of the destination image in X dimension (in bytes)
 * @param[in]  out_step_x                        out_stride_x * number of elements along X processed per work item (in bytes)
 * @param[in]  out_stride_y                      Stride of the destination image in Y dimension (in bytes)
 * @param[in]  out_step_y                        out_stride_y * number of elements along Y processed per work item (in bytes)
 * @param[in]  out_offset_first_element_in_bytes Offset of the first element in the destination image
 * @param[in]  width                             Width of the destination image
 * @param[in]  height                            Height of the destination image
 */
__kernel void warp_affine(
    IMAGE_DECLARATION(in),
    IMAGE_DECLARATION(out),
    const int width,
    const int height,
	__global float matrix[9],
	const uchar constValue,
	const int type)
{
    Image in  = CONVERT_TO_IMAGE_STRUCT_NO_STEP(in);
    Image out = CONVERT_TO_IMAGE_STRUCT(out);
	
	float8 mat = (float8)(matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5], 0.0, 0.0);
	float8 coords = apply_affine_transform(get_current_coords(), mat);

	if (type == VX_INTERPOLATION_NEAREST_NEIGHBOR)
	    vstore4(read_texels4(&in, convert_int8_rtn(clamp_to_border(coords, width, height))), 0, out.ptr);
	else if (type == VX_INTERPOLATION_BILINEAR)
	    vstore4(bilinear_interpolate(&in, coords, width, height), 0, out.ptr);
	
	if (coords.even.s0 < 0 || coords.odd.s0 < 0 || coords.even.s0 >= width || coords.odd.s0 >= height)
	{
		out.ptr[0] = constValue;
	}
	if (coords.even.s1 < 0 || coords.odd.s1 < 0 || coords.even.s1 >= width || coords.odd.s1 >= height)
	{
		out.ptr[1] = constValue;
	}
	if (coords.even.s2 < 0 || coords.odd.s2 < 0 || coords.even.s2 >= width || coords.odd.s2 >= height)
	{
        out.ptr[2] = constValue;
	}
	if (coords.even.s3 < 0 || coords.odd.s3 < 0 || coords.even.s3 >= width || coords.odd.s3 >= height)
	{
		out.ptr[3] = constValue;
	}
}
