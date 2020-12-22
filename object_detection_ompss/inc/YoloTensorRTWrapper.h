#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif
	// ======= Config Functions =======
	bool ParseConfig(const char* pConfigFile);
	void PrintSettings();
	int32_t GetImageWidth();
	int32_t GetImageHeight();
	int32_t GetImageChannel();

	// ======= Init Functions =======
	int InitVideoStream(const char* pStr);
	int InitYoloTensorRT(const char* pConfigFile);

	// ======= Processing Functions =======
	void GetNextFrame(uint8_t* pData);
	void ProcessNextFrame(const uint8_t buffer);
	void ProcessDetections(const uint8_t buffer);

	// ======= Utility Functions =======
	void C2CvMat(uint8_t* pData, const uint8_t buffer);
	void CheckFPS(uint32_t* pFrameCnt, const uint64_t iteration, const float maxFPS);
	void PrintFPS(const float fps, const uint64_t iteration, const float maxFPS, const float itrTime);
	void PrintStartString();
	void Cleanup();

#ifdef __cplusplus
}
#endif
