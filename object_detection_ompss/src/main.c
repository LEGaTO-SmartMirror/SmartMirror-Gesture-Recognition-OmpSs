/* 
 *  File: main.c
 *  Copyright (c) 2020 Florian Porrmann
 *  
 *  MIT License
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nanos6/cluster.h>

#include <YoloTensorRTWrapper.h>

const uint64_t ITERATION_CAP = 2000000000;

static inline void* lmalloc(const size_t size)
{
#ifdef SERIAL
	void* ret = malloc(size);
#else
	void* ret = nanos6_lmalloc(size);
#endif
	if (!ret)
	{
		perror("lmalloc()");
		exit(1);
	}

	return ret;
}

static inline void lfree(void* ptr, const size_t size)
{
	if (ptr)
	{
#ifdef SERIAL
		free(ptr);
#else
		nanos6_lfree(ptr, size);
#endif
	}
}

int main(int argc, char** argv)
{
	float maxFPS = 30.0f;
	char capStr[BUFSIZ];
	int32_t imageSize;
	uint8_t* pData0;
	uint8_t* pData1;
	uint32_t frameCnt;
	uint64_t iteration;
	int32_t imgWidth;
	int32_t imgHeight;
	size_t cfgNameLen;
	char* pConfigFile;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s CONFIG-FILE\n", argv[0]);
		fflush(stderr);
		return -1;
	}

	fd_set readfds;
	FD_ZERO(&readfds);
	/* If detection is to fast we need a break */
	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;
	char message[BUFSIZ];

	cfgNameLen  = strlen(argv[1]) + 1;
	pConfigFile = lmalloc(cfgNameLen * sizeof(char));
	strcpy(pConfigFile, argv[1]);

	if (!ParseConfig(pConfigFile))
	{
		fprintf(stderr, "Unable to parse provided config or config does not contain all required values\n");
		return -1;
	}

	PrintSettings();

	imgWidth  = GetImageWidth();
	imgHeight = GetImageHeight();

	imageSize = imgWidth * imgHeight * GetImageChannel();

	pData0 = (uint8_t*)lmalloc(imageSize * sizeof(uint8_t));
	pData1 = (uint8_t*)lmalloc(imageSize * sizeof(uint8_t));

	/* Set last frame count to 0 */
	frameCnt = 0;

	/* Wait for image handler to boot up */
	sleep(5);

	sprintf(capStr, "shmsrc socket-path=/dev/shm/camera_small ! video/x-raw, format=BGR, width=%i, height=%i, \
		framerate=30/1 ! queue max-size-buffers=5 leaky=2 ! videoconvert ! video/x-raw, format=BGR ! \
		appsink drop=true",
			imgWidth, imgHeight);

	if (!InitVideoStream(capStr))
	{
		fprintf(stderr, "Failed to open video sink, exiting ...\n");
		return -1;
	}

#pragma oss task in(pConfigFile[0; cfgNameLen])node(1) label("init_node_1")
	{
		printf("{\"STATUS\": \"initilize everything on node 1\"}\n");
		if (!InitYoloTensorRT(pConfigFile))
		{
			fprintf(stderr, "Failed to init Yolo, exiting ...\n");
			exit(-1); /* exit() is used here because return is not possible from within an OmpSs task */
		}
	}

#pragma oss taskwait

	PrintStartString();

	sleep(1);

	while (1)
	{
		/* check stdin for a new maxFPS amount */
		FD_SET(STDIN_FILENO, &readfds);

		if (select(1, &readfds, NULL, NULL, &timeout))
		{
			scanf("%s", message);
			maxFPS = atof(message);
		}

		/* if no FPS are needed and maxFPS equals 0, wait and start from the beginning */
		if (maxFPS == 0)
		{
			sleep(1);
			PrintFPS(0.0f, 0, 0.0f, 0.0f);
			continue;
		}

		if (frameCnt % 2 == 0)
		{
			GetNextFrame(pData0);

#pragma oss task in(pData0[0; imageSize])node(1) label("even_copy")
			{
				C2CvMat(pData0, 1);
			}

#pragma oss task node(1) label("process_frame_0")
			{
				ProcessNextFrame(0);
			}
#pragma oss task node(1) label("process_detections_1")
			{
				ProcessDetections(1);
			}
		}
		else
		{
			GetNextFrame(pData1);

#pragma oss task in(pData1[0; imageSize])node(1) label("odd_copy")
			{
				C2CvMat(pData1, 0);
			}

#pragma oss task node(1) label("process_frame_1")
			{
				ProcessNextFrame(1);
			}
#pragma oss task node(1) label("process_detections_0")
			{
				ProcessDetections(0);
			}
		}

#pragma oss taskwait

		iteration++;
		frameCnt++;
		CheckFPS(&frameCnt, iteration, maxFPS);

		if (iteration > ITERATION_CAP)
			iteration = 0;
	}

	Cleanup();

	lfree(pData0, imageSize * sizeof(uint8_t));
	lfree(pData1, imageSize * sizeof(uint8_t));
	lfree(pConfigFile, cfgNameLen * sizeof(char));

	return 0;
}
