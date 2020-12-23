/* 
 *  File: YoloTensorRTWrapper.h
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
	bool InitVideoStream(const char* pStr);
	bool InitYoloTensorRT(const char* pConfigFile);

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
