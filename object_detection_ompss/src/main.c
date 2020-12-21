#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nanos6/cluster.h>
#include <nanos6/debug.h>

#include <YoloTensorRTWrapper.h>

#define ITERATION_CAP 2000000000

#define IMAGE_WIDTH  416
#define IMAGE_HEIGHT 416
#define CHANNELS     3

#define CONFIG_FILE "config.ini"

#define _assert(cond)                                                  \
	{                                                                  \
		if (!cond)                                                     \
		{                                                              \
			fprintf(stderr, "[%s][%s]:[%u] Assertion '%s' failed. \n", \
					__func__, __FILE__, __LINE__, #cond);              \
			abort();                                                   \
		}                                                              \
	}

static inline void *_lmalloc(size_t size, const char *objName)
{
#ifdef SERIAL
	void *ret = malloc(size);
#else
	void *ret = nanos6_lmalloc(size);
#endif
	if (!ret)
	{
		perror("nanos6_lmalloc()");
		exit(1);
	}
#ifdef D
	printf("%s lmalloced with size %ld  Addr %p - %p \n", objName, size /**10e-3*/, (void *)objName, (void *)objName + size);
#endif

	return ret;
}

static inline void _lfree(void *ptr, size_t size)
{
	_assert(ptr);
	nanos6_lfree(ptr, size);
}

int main(int argc, char **argv)
{
	float maxFPS = 30.0f;
	char capStr[BUFSIZ];
	int32_t imageSize; // = IMAGE_HEIGHT * IMAGE_WIDTH * CHANNELS;
	uint8_t *pData0;
	uint8_t *pData1;
	uint32_t frameCnt;
	uint64_t iteration;

	fd_set readfds;
	FD_ZERO(&readfds);
	/* If detection is to fast we need  a break */
	struct timeval timeout;
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;
	char message[BUFSIZ];

	if (ParseConfig(CONFIG_FILE) != 1)
	{
		fprintf(stderr, "Unable to parse provided config or config does not contain all required values\n");
		return -1;
	}

	PrintSettings();

	imageSize = GetImageWidth() * GetImageHeight() * GetImageChannel();

	pData0 = (uint8_t *)_lmalloc(imageSize * sizeof(uint8_t), "data_struct_0");
	pData1 = (uint8_t *)_lmalloc(imageSize * sizeof(uint8_t), "data_struct_1");

	// Set last frame count to 0
	frameCnt = 0;

	// Wait for image handler to boot up
	sleep(5);

	sprintf(capStr, "shmsrc socket-path=/dev/shm/camera_small ! video/x-raw, format=BGR, width=%i, height=%i, \
		framerate=30/1 ! queue max-size-buffers=5 leaky=2 ! videoconvert ! video/x-raw, format=BGR ! \
		appsink drop=true",
			IMAGE_WIDTH, IMAGE_HEIGHT);

	if (InitVideoStream(capStr) != 1)
	{
		fprintf(stderr, "Failed to open video sink, exiting ...\n");
		return -1;
	}

#pragma oss task node(1) label("init_node_1")
	{
		printf("{\"STATUS\": \"initilize everything on node 1\"}\n");
		if (InitYoloTensorRT(CONFIG_FILE) != 1)
		{
			fprintf(stderr, "Failed to init Yolo, exiting ...\n");
			exit(-1);
		}
	}

#pragma oss taskwait

	printf("{\"STATUS\": \"looping starts now\"}\n");
	printf("{\"DETECTED_GESTURES\": []}\n");

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
				c2CvMat(pData0, IMAGE_HEIGHT, IMAGE_WIDTH, 1);
			} /* end task */

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
				c2CvMat(pData1, IMAGE_HEIGHT, IMAGE_WIDTH, 0);
			} /* end task */

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

	_lfree(pData0, imageSize * sizeof(uint8_t));
	_lfree(pData1, imageSize * sizeof(uint8_t));

	return 0;
}
