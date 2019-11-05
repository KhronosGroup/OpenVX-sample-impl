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

#include <vx_support.h>

#define CASE_STRINGERIZE(err, label, function, file, line) \
    case err: \
        fprintf(stderr, "%s: OpenCL error "#err" at %s in %s:%d\n", label, function, file, line); \
        break

static size_t flen(FILE *fp)
{
    size_t len = 0;
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return len;
}

static size_t flines(FILE *fp)
{
    size_t numLines = 0;
    if (fp) {
        char line[CL_MAX_LINESIZE];
        fseek(fp, 0, SEEK_SET);
        while (fgets(line, sizeof(line), fp) != NULL) {
            numLines++;
        }
        //printf("%lu lines in file %p\n",numLines,fp);
        fseek(fp, 0, SEEK_SET);
    }
    return numLines;
}
        
cl_int clBuildError(cl_int build_status, const char *label, const char *function, const char *file, int line)
{
    switch (build_status)
    {
        case CL_BUILD_SUCCESS:
            fprintf(stdout, "%s: Build Successful!\n", label);
            break;
        CASE_STRINGERIZE(CL_BUILD_NONE, label, function, file, line);
        CASE_STRINGERIZE(CL_BUILD_ERROR, label, function, file, line);
        CASE_STRINGERIZE(CL_BUILD_IN_PROGRESS, label, function, file, line);
        default:
            fprintf(stderr, "%s: Unknown build error %d at %s in %s:%d\n", label, build_status, function, file, line);
            break;
    }
    return build_status;
}

cl_int clPrintError(cl_int err, const char *label, const char *function, const char *file, int line)
{
    switch (err)
    {
        //CASE_STRINGERIZE(CL_SUCCESS, label, function, file, line);
        case CL_SUCCESS:
            break;
        CASE_STRINGERIZE(CL_BUILD_PROGRAM_FAILURE, label, function, file, line);
        CASE_STRINGERIZE(CL_COMPILER_NOT_AVAILABLE, label, function, file, line);
        CASE_STRINGERIZE(CL_DEVICE_NOT_AVAILABLE, label, function, file, line);
        CASE_STRINGERIZE(CL_DEVICE_NOT_FOUND, label, function, file, line);
        CASE_STRINGERIZE(CL_IMAGE_FORMAT_MISMATCH, label, function, file, line);
        CASE_STRINGERIZE(CL_IMAGE_FORMAT_NOT_SUPPORTED, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_ARG_INDEX, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_ARG_SIZE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_ARG_VALUE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_BINARY, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_BUFFER_SIZE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_BUILD_OPTIONS, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_COMMAND_QUEUE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_CONTEXT, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_DEVICE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_DEVICE_TYPE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_EVENT, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_EVENT_WAIT_LIST, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_GL_OBJECT, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_GLOBAL_OFFSET, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_HOST_PTR, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_IMAGE_SIZE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_KERNEL_NAME, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_KERNEL, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_KERNEL_ARGS, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_KERNEL_DEFINITION, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_MEM_OBJECT, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_OPERATION, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_PLATFORM, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_PROGRAM, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_PROGRAM_EXECUTABLE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_QUEUE_PROPERTIES, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_SAMPLER, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_VALUE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_WORK_DIMENSION, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_WORK_GROUP_SIZE, label, function, file, line);
        CASE_STRINGERIZE(CL_INVALID_WORK_ITEM_SIZE, label, function, file, line);
        CASE_STRINGERIZE(CL_MAP_FAILURE, label, function, file, line);
        CASE_STRINGERIZE(CL_MEM_OBJECT_ALLOCATION_FAILURE, label, function, file, line);
        CASE_STRINGERIZE(CL_MEM_COPY_OVERLAP, label, function, file, line);
        CASE_STRINGERIZE(CL_OUT_OF_HOST_MEMORY, label, function, file, line);
        CASE_STRINGERIZE(CL_OUT_OF_RESOURCES, label, function, file, line);
        CASE_STRINGERIZE(CL_PROFILING_INFO_NOT_AVAILABLE, label, function, file, line);
        default:
            fprintf(stderr, "%s: Unknown error %d at %s in %s:%d\n", label, err, function, file, line);
            break;
    }
    return err;
}

char *clLoadSources(char *filename, size_t *programSize)
{
    FILE *pFile = NULL;
    char *programSource = NULL;
    VX_PRINT(VX_ZONE_INFO, "Reading source file %s\n", filename);
    pFile = fopen((char *)filename, "rb");
    if (pFile != NULL && programSize)
    {
        // obtain file size:
        fseek(pFile, 0, SEEK_END);
        *programSize = ftell(pFile);
        rewind(pFile);

        int size = *programSize + 1;
        programSource = (char*)malloc(sizeof(char)*(size));
        if (programSource == NULL)
        {
            fclose(pFile);
            free(programSource);
            return NULL;
        }

        fread(programSource, sizeof(char), *programSize, pFile);
        programSource[*programSize] = '\0';
        fclose(pFile);
    }
    return programSource;
}

#if defined(EXPERIMENTAL_USE_FNMATCH)
static int vx_source_filter(const struct dirent *de)
{
    if (de && 0 == fnmatch("vx_*.cl", de->d_name,
#if defined(__QNX__) || defined(__APPLE__)
    FNM_PERIOD|FNM_PATHNAME))
#else
    FNM_PERIOD|FNM_FILE_NAME))
#endif
        return 1;
    else
        return 0;
}

#if defined(__QNX__)
typedef int (*sorting_f)(const void *, const void *);
#else
typedef int (*sorting_f)(const struct dirent **, const struct dirent **);
#endif

static int name_sort(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

cl_program vxLoadProgram(cl_context context, const char *src_dir, cl_int *perr)
{
    cl_program program;
    struct dirent **names = NULL;
    int i, f, num_lines = 0, cur_line = 0;
    int num_files = scandir(src_dir, &names, &vx_source_filter, &name_sort);
    size_t *lengths = NULL, lineSize = CL_MAX_LINESIZE;
    char **source = NULL;
    printf("Matched %d files\n", num_files);
    for (f = 0; f < num_files; f++) {
        if (names[f]->d_name) {
            char pathname[CL_MAX_LINESIZE];
            sprintf(pathname, "%s%s", src_dir, names[f]->d_name);
            FILE *fp = fopen(pathname, "r");
            if (fp) {
                num_lines += flines(fp);
                fclose(fp);
            }
        }
    }
    printf("Total Number Lines: %d\n", num_lines);
    // allocate big array of lines
    source = ALLOC(char *, num_lines);
    lengths = ALLOC(size_t, num_lines);
    for (i = 0; i < num_lines; i++) {
        source[i] = ALLOC(char, lineSize);
        lengths[i] = lineSize;
    }
    // load all source into a single array
    for (f = 0; f < num_files; f++) {
        if (names[f]->d_name) {
            char pathname[CL_MAX_LINESIZE];
            sprintf(pathname, "%s%s", src_dir, names[f]->d_name);
            FILE *fp = fopen(pathname, "r");
            if (fp) {
                printf("Reading from file %s\n", pathname);
                do {
                    if (fgets(source[cur_line], lengths[cur_line], fp) == NULL)
                        break;
                    // trim to exact lengths
                    lengths[cur_line] = strlen(source[cur_line]);
                    cur_line++;
                } while (1);
                printf("@ %u lines\n", cur_line);
                fclose(fp);
            }
        }
    }
    if (num_lines != cur_line) {
        fprintf(stderr, "Failed to read in all lines from source files!\n");
        return 0;
    }
#if 1
    for (i = 0; i < num_lines; i++) {
        printf("%4d [%4zu] %s", i, lengths[i], source[i]);
    }
#endif
    program = clCreateProgramWithSource(context, num_lines, (const char **)source, lengths, perr);
    CL_ERROR_MSG(*perr, "clCreateProgramWithSource");
#if 0
    if (perr != CL_SUCCESS) {
        cl_int err = 0;
        size_t src_size = 0;
        char *src = NULL;
        err = clGetProgramInfo(program, CL_PROGRAM_SOURCE, 0, NULL, &src_size);
        CL_ERROR_MSG(err, "clGetProgramInfo");
        printf("Source Code has %zu bytes\n", src_size);
        src = (char *)malloc(src_size);
        err = clGetProgramInfo(program, CL_PROGRAM_SOURCE, src_size, src, NULL);
        CL_ERROR_MSG(err, "clGetProgramInfo");
        printf("%s", src);
        free(src);
    }
#endif
    return program;
}

#elif defined(_WIN32)

cl_program vxLoadProgram(cl_context context, const char *src_dir, cl_int *perr) {
    return 0;
}

#endif



