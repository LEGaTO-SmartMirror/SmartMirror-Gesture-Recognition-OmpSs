#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	void PrintSettings();
	bool ParseConfig(const char* pConfigFile);
	int InitVideoStream(const char* pStr);
	int InitYoloTensorRT(const char* pConfigFile);
	void GetNextFrame(uint8_t* pData);
	void c2CvMat(uint8_t* pData, const int32_t height, const int32_t width, const uint8_t buffer);
	void ProcessNextFrame(const uint8_t buffer);
	void ProcessDetections(const uint8_t buffer);
	void Cleanup();
	void CheckFPS(uint32_t* pFrameCnt, const uint64_t iteration, const float maxFPS);
	void PrintFPS(const float fps, const uint64_t iteration, const float maxFPS, const float itrTime);

#ifdef __cplusplus
}
#endif
