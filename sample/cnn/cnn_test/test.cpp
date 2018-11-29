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
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdint.h>
#include "network.h"
#include <half/sip_ml_fp16.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include "windows.h"
#else
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)

int16_t* MapFile(const char *fileName, size_t* filesize)
{
	int16_t * result;
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	BOOL fOk;
	HANDLE handle;
	HANDLE hFile = ::CreateFile(
			fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!filesize || !hFile)
	{
		return NULL;
	}

	*filesize = 0;
	fOk = GetFileAttributesEx(fileName, GetFileExInfoStandard, (void*)&fileInfo);
	if (!fOk)
	{
		return NULL;
	}

	*filesize = fileInfo.nFileSizeLow;

	handle = ::CreateFileMapping(
			hFile,
			NULL,
			PAGE_READONLY,
			0,
			0,
			NULL);
	if (!handle)
	{
		return NULL;
	}

	result = (int16_t *)::MapViewOfFile(handle, FILE_MAP_READ, 0, 0, 0);
	::CloseHandle(hFile);
	return result;
}

void DumpToFile(const char *fileName, int16_t* output, size_t size)
{
	FILE *file;
	if ((file = fopen(fileName, "a+")) != NULL)
	{
		for (int i = 0; i < size; i++)
		{
			fprintf(file, "%d\n", output[i]);
		}
		fclose(file);
	}
	else
	{
		printf("Warning: can't open output file\n");
	}
}

#else // defined(WIN32) || defined(_WIN32) || defined(__WIN32)

int16_t* MapFile(const char* filename, size_t* filesize) {
	struct stat sb;
	off_t len;
	int16_t *p;
	int fd;

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		printf("File open failed.\nfilename=%s\nerrno=%d\n", filename, errno);
		return NULL;
	}

	if (fstat(fd, &sb) == -1) {
		printf("fstat failed\n");
		return NULL;
	}

	if (!S_ISREG(sb.st_mode)) {
		printf("Unexpected mode\n");
		return NULL;
	}

	*filesize = sb.st_size;
	p = (int16_t*) mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (p == MAP_FAILED) {
		printf("mmap failed\n");
		return NULL;
	}

	if (close(fd) == -1) {
		printf("File close failed\n");
		return NULL;
	}

	return p;

	//return (int16_t*)mmap_read(filename, filesize);
}

void DumpToFile(const char *fileName, int16_t* output, size_t size) {
	FILE* fd;

	fd = fopen(fileName, "wb");
	if (fd == NULL) {
		printf("File open failed.\nfilename=%s\nerrno=%d\n", fileName, errno);
		return;
	}

	fwrite(output, size, 1, fd);
	fclose(fd);
}

#endif // defined(WIN32) || defined(_WIN32) || defined(__WIN32)

int verifyResult(const char *network, vx_enum format, int16_t *ptr,
		int results) {
	FILE *results_file;
	int max_row[5] = { 0 };
	int max_val[5] = { 0 };
	int16_t curr_max, curr_item;
	bool new_max = true;
	int result = 0;

	printf("Outputting results\n");

	std::string classes_file_name;
	if (strcmp(network, "alexnet") == 0) {
		classes_file_name = "imagenet_classes_alexnet.txt";
	} else {
		classes_file_name = "imagenet_classes.txt";
	}

	std::ifstream classes_file(classes_file_name.c_str());
	if (!classes_file) {
		printf("Test Failed!!! Couldn't open imagenet classes file.\n");
		return -1;
	}

	std::vector<std::string> lines;
	for (std::string line; std::getline(classes_file, line); /**/) {
		line.resize(50);
		lines.push_back(line);
	}

	for (int i = 0; i < 5; i++) {
		curr_max = 0;
		for (int j = 0; j < results; j++) {
			curr_item = *(ptr + j);
			if (curr_item > curr_max) {
				new_max = true;
				for (int k = 0; k < i; k++) {
					if (j == max_row[k])
						new_max = false;
				}
				if (new_max) {
					max_row[i] = j;
					max_val[i] = curr_item;
					curr_max = curr_item;
				}
			}
		}
	}

	std::string results_file_name = "test_result.txt";
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
	if ((fopen_s(&results_file, results_file_name.c_str(), "w")) != 0)
#else
	results_file = fopen(results_file_name.c_str(), "w");
	if (results_file == NULL)
#endif
	{
		printf("Test Failed!!! Couldn't open results file.\n");
		return -1;
	}

	for (int i = 0; i < 5; i++) {
		fprintf(results_file, "%s: %f\n", lines.at(max_row[i]).data(),
				short2float(max_val[i]));
	}

	if (strcmp(network, "overfeat") == 0) {
		if ((strncmp(lines.at(max_row[0]).data(), "collie", 6) != 0)
				|| (strncmp(lines.at(max_row[1]).data(), "Shetland", 6) != 0)
				|| (strncmp(lines.at(max_row[2]).data(), "borzoi", 6) != 0)) {
			printf("Test Failed!!! Incorrect results.\n");
			result = -1;
		} else {
			printf("Test Passed!!!\n");
		}
	} else {
		printf("Results not verifiable at this point.\n");
	}

	if (format == VX_TYPE_INT16) {
		printf("[1]    %s %.1f%%\n", lines.at(max_row[0]).data(),
				(float) max_val[0] / 256 * 100);
		printf("[2]    %s %.1f%%\n", lines.at(max_row[1]).data(),
				(float) max_val[1] / 256 * 100);
		printf("[3]    %s %.1f%%\n", lines.at(max_row[2]).data(),
				(float) max_val[2] / 256 * 100);
	}
	fclose(results_file);

	printf("Press enter to continue\n");
	getchar();
	return result;
}

int main(int argc, char* argv[]) {
	//const char *networkType = "overfeat";//"alexnet";//"char-rnn";
	char *networkType = argv[1];
	int16_t* data[5] = { 0 };
	int16_t *output;
	size_t filesize;
	int i = 0;
	std::string inputFileName;
	std::string avgFileName = "alexnet_mean_q8";
	std::string weightsFileName;
	size_t inputSize = 0, weightSize = 0;
	vx_enum format = VX_TYPE_INT16;
	vx_int8 pos = Q78_FIXED_POINT_POSITION;

	if (networkType == NULL) {
		printf(
				"Usage: cnn_test <network> [input file] [weights flie] [output file]\n");
		goto error;
	}

	if (strcmp(networkType, "overfeat") == 0) {
		output = (int16_t*) malloc(1000 * sizeof(int16_t));
		inputSize = 160083;
		if (argc > 2 && argv[2] != NULL) {
			inputFileName = argv[2];
		} else {
			inputFileName = "0003";
		}

		// Get weights
		weightSize = 145920872;
		if (argc > 3 && argv[3] != NULL) {
			weightsFileName = argv[3];
		} else {
			if (format == VX_TYPE_INT16) {
				weightsFileName = "net_weight_0_q8";
			}
#ifdef EXPERIMENTAL_PLATFORM_SUPPORTS_16_FLOAT
			else if (format == VX_TYPE_FLOAT16) {
				pos = 0;
				weightsFileName = "net_weight_0_f16";
			}
#endif
		}
	} else if (strcmp(networkType, "alexnet") == 0) {
		if (format == VX_TYPE_INT16) {
			output = (int16_t*) malloc(1000 * sizeof(int16_t));
            inputSize = 154587;
			if (argc > 2 && argv[2] != NULL) {
				inputFileName = argv[2];
			} else {
				inputFileName = "0052_256_q8";
			}
			weightsFileName = "alexnet_weights_q8_d442mw2";
			weightSize = 60965224;
		} else {
			printf("Alexnet format should be Q78.\n");
			goto error;
		}
	} else if (strcmp(networkType, "googlenet2") == 0) {
		/*format = VX_TYPE_FLOAT16; // VX_TYPE_INT16;
		pos = 0;
		output = (int16_t*) malloc(1000 * sizeof(int16_t));
		inputSize = 150529; // 196609;
		if (argc > 2 && argv[2] != NULL) {
			inputFileName = argv[2];
		} else {
			inputFileName = "0242_224_f16"; //"0242_224_q78";
		}

		weightsFileName = "googlenet2_weights_f16"; //"googlenet2_weights_q78";
		weightSize = 10129865;*/
	}
	else //if (strcmp(networkType, "test") != 0)
	{
		printf(
				"Unsupported CNN type. Supported networks are overfeat, alexnet, googlenet2.\n");
		goto error;
	}

	if (strcmp(networkType, "test") != 0) {
		// Get input
		data[i] = MapFile(inputFileName.c_str(), &filesize);
		if ((data[i] == NULL) || (filesize / sizeof(int16_t) != inputSize)) {
			printf("Invalid input file %s\n", inputFileName.c_str());
			goto error;
		}

		// Get averages
		if (strcmp(networkType, "alexnet") == 0) {
			data[++i] = MapFile(avgFileName.c_str(), &filesize);
			if ((data[i] == NULL) || (filesize / sizeof(int16_t) != 196608)) {
				printf("Invalid average file %s\n", avgFileName.c_str());
				goto error;
			}

			i++;
			data[i] = data[i - 1] + 65536;
			i++;
			data[i] = data[i - 1] + 65536;
		}

		// Get weights
		data[++i] = MapFile(weightsFileName.c_str(), &filesize);
		unsigned int nParameters = filesize / (sizeof(int16_t));
		if ((data[i] == NULL) || (nParameters != weightSize)) {
			printf("Invalid weights file\n");
			goto error;
		}
	}

	if (strcmp(networkType, "overfeat") == 0) {
		if (Overfeat(data, format, pos, false, output) == 0) {
			if (argc > 4 && argv[4] != NULL) {
				DumpToFile(argv[4], output, 1000);
			} else {
				return verifyResult(networkType, format, output, 1000);
			}
		}
	} else if (strcmp(networkType, "alexnet") == 0) {
		if (Alexnet(data, false, output) == 0) {
			if (argc > 4 && argv[4] != NULL) {
				DumpToFile(argv[4], output, 1000);
			} else {
				return verifyResult(networkType, format, output, 1000);
			}
		}
	} else if (strcmp(networkType, "googlenet2") == 0) {
		/*
		if (Googlenet2(data, format, pos, false, true, output) == 0) {
			if (argc > 4 && argv[4] != NULL) {
				DumpToFile(argv[4], output, 1000);
			} else {
				return verifyResult(networkType, format, output, 1000);
			}
		}*/
	}
	else {
		printf(
				"Available networks are overfeat, alexnet, googlenet2, bds, test\n");
		goto error;
	}

	return 0;

	error: printf("Press any key to continue...\n");
	getchar();
	return -1;
}

