/* 
 *  File: YoloTensorRTWWrapper.cpp
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

/* Threshold used when comparing two bounding boxes
   needs to be defined before YoloTensorRTWrapper.h
 */
#define DIFF_THRESHOLD 0.005

#include "YoloTensorRTWrapper.h"
#include "SORT.h"
#include "Timer.h"
#include "YoloTRT.h"

#include <inipp.h>

#include <chrono>
#include <iostream>
#include <thread>

const float DEFAULT_YOLO_THRESHOLD = 0.3f;   // Threshold used in case non is specified in the config
const double ONE_SECOND            = 1000.0; // One second in milliseconds
const uint32_t MAX_BUFFERS         = 2;      // Number of buffers

// List of config values required for the application to execute properly
const std::vector<std::string> REQUIRED_CONFIG_VALUES = {
	"DLA_CORE",
	"ONNX_FILE",
	"CONFIG_FILE",
	"ENGINE_FILE",
	"CLASS_FILE",
	"DETECT_STR",
	"AMOUNT_STR",
	"FPS_STR",
	"IMAGE_WIDTH",
	"IMAGE_HEIGHT",
	"IMAGE_CHANNEL"
};

struct Settings
{
	Settings() :
		valid(false),
		dlaCore(-1),
		useFP16(false),
		onnxFile(""),
		configFile(""),
		engineFile(""),
		classFile(""),
		detectStr(""),
		amountStr(""),
		fpsStr(""),
		imgWidth(0),
		imgHeight(0),
		imgChannel(0),
		yoloType(YoloType::NON),
		yoloThreshold(0.0f)
	{
	}

	// This expects that section at least contains all the required items
	Settings(inipp::Ini<char>::Section& sec) :
		valid(false),
		dlaCore(-1),
		useFP16(false),
		onnxFile(""),
		configFile(""),
		engineFile(""),
		classFile(""),
		detectStr(""),
		amountStr(""),
		fpsStr(""),
		imgWidth(0),
		imgHeight(0),
		imgChannel(0),
		yoloType(YoloType::NON),
		yoloThreshold(0.0f)
	{
		inipp::extract(sec["DLA_CORE"], dlaCore);
		inipp::extract(sec["ONNX_FILE"], onnxFile);
		inipp::extract(sec["CONFIG_FILE"], configFile);
		inipp::extract(sec["ENGINE_FILE"], engineFile);
		inipp::extract(sec["CLASS_FILE"], classFile);

		inipp::extract(sec["DETECT_STR"], detectStr);
		inipp::extract(sec["AMOUNT_STR"], amountStr);
		inipp::extract(sec["FPS_STR"], fpsStr);

		inipp::extract(sec["IMAGE_WIDTH"], imgWidth);
		inipp::extract(sec["IMAGE_HEIGHT"], imgHeight);
		inipp::extract(sec["IMAGE_CHANNEL"], imgChannel);

		if (sec.count("USE_FP16") > 0)
			inipp::extract(sec["USE_FP16"], useFP16);
		else
			useFP16 = false;

		yoloType = YoloType::NON;
		if (sec.count("YOLO_VERSION") > 0)
		{
			int32_t v;
			inipp::extract(sec["YOLO_VERSION"], v);
			if (v == 3)
				yoloType |= YoloType::YOLO_V3;
			else if (v == 4)
				yoloType |= YoloType::YOLO_V4;
			else
				std::cerr << "Invalid version (" << sec["YOLO_VERSION"] << ") specified in YOLO_VERSION" << std::endl;
		}

		if (sec.count("YOLO_TINY") > 0)
		{
			std::string tiny;
			inipp::extract(sec["YOLO_TINY"], tiny);
			if (String2Lower(tiny) == "true")
				yoloType |= YoloType::TINY;
		}

		if (sec.count("YOLO_THRESHOLD") > 0)
			inipp::extract(sec["YOLO_THRESHOLD"], yoloThreshold);
		else
			yoloThreshold = DEFAULT_YOLO_THRESHOLD;

		valid = true;
	}

	friend std::ostream& operator<<(std::ostream& os, const Settings& s)
	{
		os << "ONNX-File     : " << s.onnxFile << std::endl
		   << "Config-File   : " << s.configFile << std::endl
		   << "Engine-File   : " << s.engineFile << std::endl
		   << "Class-File    : " << s.classFile << std::endl
		   << "DLA-Core      : " << s.dlaCore << std::endl
		   << "Use FP16      : " << s.useFP16 << std::endl
		   << "Detect String : " << s.detectStr << std::endl
		   << "Amount String : " << s.amountStr << std::endl
		   << "FPS String    : " << s.fpsStr << std::endl
		   << "Image Width   : " << s.imgWidth << std::endl
		   << "Image Height  : " << s.imgHeight << std::endl
		   << "Image Channel : " << s.imgChannel << std::endl
		   << "Yolo Type     : " << s.yoloType << std::endl
		   << "Yolo Threshold: " << s.yoloThreshold;
		return os;
	}

	bool valid;

	int32_t dlaCore;
	bool useFP16;
	std::string onnxFile;
	std::string configFile;
	std::string engineFile;
	std::string classFile;

	std::string detectStr;
	std::string amountStr;
	std::string fpsStr;

	int32_t imgWidth;
	int32_t imgHeight;
	int32_t imgChannel;

	YoloType yoloType;
	float yoloThreshold;
};

//  ========= Video Node =========
cv::VideoCapture g_cap; // Video capture object used to read the input image from the image handler

Timer g_timer;        // Timer used to measure the time required for one iteration
double g_elapsedTime; // Sum of the elapsed time, used to check if one second has passed

//  ========= Yolo Node =========
TrackingObjects g_lastTrackings; // Vector containing the last tracked objects
SORT* g_pSortTrackers;           // Pointer to n-sort trackers (n = number of classes)

cv::Mat g_frame[MAX_BUFFERS]; // Buffer for the input frame

YoloTRT* g_pYolo;                                // Pointer to a TensorRT Yolo object used to process the input image
YoloTRT::YoloResults g_yoloResults[MAX_BUFFERS]; // Buffer for the yolo results

//  ========= Both Nodes =========
Settings g_settings; // Settings container, containing all settings read from the provided config file

bool checkConfigItemPresent(const inipp::Ini<char>::Section& section, const std::string& item)
{
	if (section.count(item) == 0)
	{
		std::cerr << "Item \"" << item << "\" not present in provided configuration" << std::endl;
		return false;
	}

	return true;
}

bool checkConfigItemsPresent(const inipp::Ini<char>::Section& section, const std::vector<std::string>& items)
{
	bool valid = true;
	for (const std::string& item : items)
	{
		if (!checkConfigItemPresent(section, item))
			valid = false;
	}

	return valid;
}

BBox toCenter(const BBox& bBox)
{
	// x_y = center
	float h = bBox.height;
	float w = bBox.width;
	float x = bBox.x + (w / 2);
	float y = bBox.y + (h / 2);
	return BBox(x, y, w, h);
}

void printDetections(const TrackingObjects& trackers)
{
	std::stringstream str("");
	str << string_format("{\"%s\": [", g_settings.detectStr.c_str());

	for (const auto& [i, t] : enumerate(trackers))
	{
		BBox centerBox = toCenter(t.bBox);
		str << string_format("{\"TrackID\": %i, \"name\": \"%s\", \"center\": [%.5f,%.5f], \"w_h\": [%.5f,%.5f]}", t.trackingID, t.name.c_str(), centerBox.x, centerBox.y, centerBox.width, centerBox.height);
		// Prevent a trailing ',' for the last element
		if (i + 1 < trackers.size()) str << ", ";
	}

	g_lastTrackings = trackers;

	str << string_format("], \"%s\": %llu }", g_settings.amountStr.c_str(), g_lastTrackings.size());
	std::cout << str.str() << std::endl
			  << std::flush;
}

#ifdef WRITE_IMAGE_2_DISK
uint32_t init = 0;
#endif

extern "C"
{
	// //////////////////////////////////////
	// ======= Config Functions =============
	// //////////////////////////////////////

	bool ParseConfig(const char* pConfigFile)
	{
		std::ifstream is(pConfigFile);
		if (!is.is_open())
		{
			std::cerr << "[ParseConfig] Failed to open config file: " << pConfigFile << std::endl;
			return false;
		}

		inipp::Ini<char> ini;
		ini.parse(is);
		is.close();
		inipp::Ini<char>::Section sec = ini.sections[""];

		if (!checkConfigItemsPresent(sec, REQUIRED_CONFIG_VALUES)) return false;

		g_settings = Settings(sec);

		return true;
	}

	void PrintSettings()
	{
		if (g_settings.valid)
			std::cout << g_settings << std::endl;
		else
			std::cerr << "[PrintSettings] Settings have not been initialized proprly" << std::endl;
	}

	int32_t GetImageWidth()
	{
		if (g_settings.valid)
			return g_settings.imgWidth;
		else
		{
			std::cerr << "[GetImageWidth] Settings have not been initialized proprly" << std::endl;
			return -1;
		}
	}

	int32_t GetImageHeight()
	{
		if (g_settings.valid)
			return g_settings.imgHeight;
		else
		{
			std::cerr << "[GetImageHeight] Settings have not been initialized proprly" << std::endl;
			return -1;
		}
	}

	int32_t GetImageChannel()
	{
		if (g_settings.valid)
			return g_settings.imgChannel;
		else
		{
			std::cerr << "[GetImageChannel] Settings have not been initialized proprly" << std::endl;
			return -1;
		}
	}

	// //////////////////////////////////////
	// ======= Init Functions ===============
	// //////////////////////////////////////

	bool InitVideoStream(const char* pStr)
	{
		g_cap.open(pStr);
		if (!g_cap.isOpened())
		{
			std::cerr << "[InitVideoStream] Unable to open video stream: " << pStr << std::endl;
			return false;
		}

		g_elapsedTime = 0;
		g_timer.Start();

		return true;
	}

	bool InitYoloTensorRT(const char* pConfigFile)
	{
		if (!ParseConfig(pConfigFile))
		{
			std::cerr << "[InitYoloTensorRT] Unable to parse provided config or config does not contain all required values" << std::endl;
			return false;
		}

		// Set TensorRT log level
		TrtLog::gLogger.setReportableSeverity(TrtLog::Severity::kWARNING);

		g_pYolo = new YoloTRT(g_settings.onnxFile, g_settings.configFile, g_settings.engineFile,
							  g_settings.classFile, g_settings.dlaCore, g_settings.useFP16,
							  true, g_settings.yoloThreshold, g_settings.yoloType);

		g_pSortTrackers = new SORT[g_pYolo->GetClassCount()];

		// Initialize SORT tracker for each class
		for (std::size_t i = 0; i < g_pYolo->GetClassCount(); i++)
			g_pSortTrackers[i] = SORT(5, 3);

		g_lastTrackings.clear();

		return true;
	}

	// //////////////////////////////////////
	// ======= Processing Functions =========
	// //////////////////////////////////////

	void GetNextFrame(uint8_t* pData)
	{
		if (!pData) return;
		cv::Mat frame;
		if (g_cap.read(frame))
		{
			std::size_t size = frame.total() * frame.channels();
			cv::Mat flat     = frame.reshape(1, size);
			std::memcpy(pData, flat.ptr(), size);
		}
	}

	void ProcessNextFrame(const uint8_t buffer)
	{
		if (!g_frame[buffer % MAX_BUFFERS].empty())
			g_yoloResults[buffer % MAX_BUFFERS] = g_pYolo->Infer(g_frame[buffer % MAX_BUFFERS]);
	}

	void ProcessDetections(const uint8_t buffer)
	{
		bool changed                        = false;
		const YoloTRT::YoloResults& results = g_yoloResults[buffer % MAX_BUFFERS];
		cv::Mat frame                       = g_frame[buffer % MAX_BUFFERS];

		std::map<uint32_t, TrackingObjects> trackingDets;

		for (const YoloTRT::YoloResult& r : results)
		{
			trackingDets.try_emplace(r.ClassID(), TrackingObjects());
			trackingDets[r.ClassID()].push_back({ { r.x, r.y, r.w, r.h }, static_cast<uint32_t>(std::round(r.Conf() * 100)), g_pYolo->ClassName(r.ClassID()) });
		}

		TrackingObjects trackers;
		TrackingObjects dets;

		for (std::size_t i = 0; i < g_pYolo->GetClassCount(); i++)
		{
			if (trackingDets.count(i))
				dets = trackingDets[i];
			else
				dets = TrackingObjects();

			TrackingObjects t = g_pSortTrackers[i].Update(dets);
			trackers.insert(std::end(trackers), std::begin(t), std::end(t));
		}

		if (trackers.size() != g_lastTrackings.size())
			changed = true;
		else
		{
			for (const auto& [idx, obj] : enumerate(trackers))
			{
				if (g_lastTrackings[idx] != obj)
				{
					changed = true;
					break;
				}
			}
		}

		if (changed)
			printDetections(trackers);

#ifdef WRITE_IMAGE_2_DISK
		if (init > 2)
		{
			for (const TrackingObject& obj : trackers)
			{
				int32_t x = obj.bBox.x * 416;
				int32_t y = obj.bBox.y * 416;
				int32_t w = obj.bBox.width * 416;
				int32_t h = obj.bBox.height * 416;
				cv::rectangle(frame, cv::Point(x, y), cv::Point(x + w, y + h), cv::Scalar(0, 255, 0));
				cv::putText(frame, obj.name, cv::Point(x, y - 10), cv::FONT_HERSHEY_DUPLEX, 1, CV_RGB(255, 50, 50), 1);
			}
			imwrite("out.jpg", frame);
		}
		init++;
#endif // WRITE_IMAGE_2_DISK
	}

	// //////////////////////////////////////
	// ======= Utility Functions ============
	// //////////////////////////////////////

	void C2CvMat(uint8_t* pData, const uint8_t buffer)
	{
		if (pData == nullptr) return;

		g_frame[buffer % MAX_BUFFERS] = cv::Mat(GetImageHeight(), GetImageWidth(), CV_8UC3, pData);
	}

	void CheckFPS(uint32_t* pFrameCnt, const uint64_t iteration, const float maxFPS)
	{
		g_timer.Stop();

		double minFrameTime = 1000.0 / maxFPS;
		double itrTime      = g_timer.GetElapsedTimeInMilliSec();
		double fps;

		if (g_timer.GetElapsedTimeInMilliSec() < minFrameTime)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int32_t>(std::round(minFrameTime - itrTime))));
			g_elapsedTime += minFrameTime;
		}
		else
			g_elapsedTime += itrTime;

		fps = 1000 / (g_elapsedTime / (*pFrameCnt));

		if (g_elapsedTime >= ONE_SECOND)
		{
			// if (fps > maxFPS) fps = maxFPS;

			PrintFPS(fps, iteration, maxFPS, itrTime);

			*pFrameCnt    = 0;
			g_elapsedTime = 0;
		}

		g_timer.Start();
	}

	void PrintFPS(const float fps, const uint64_t iteration, const float maxFPS, const float itrTime)
	{
		if (fps == 0.0f)
			std::cout << string_format("{\"%s\": 0.0}", g_settings.fpsStr.c_str()) << std::endl
					  << std::flush;
		else
		{
			std::cout << string_format("{\"%s\": %.2f, \"Iteration\": %d, \"maxFPS\": %.2f, \"lastCurrMSec\": %.2f}",
									   g_settings.fpsStr.c_str(), fps, iteration, maxFPS, itrTime)
					  << std::endl
					  << std::flush;
		}
	}

	void PrintStartString()
	{
		std::cout << "{\"STATUS\": \"looping starts now\"}" << std::endl
				  << std::flush;
		if (g_settings.valid)
			std::cout << string_format("{\"%s\": []}", g_settings.detectStr.c_str()) << std::endl
					  << std::flush;
	}

	void Cleanup()
	{
		delete g_pYolo;
		delete[] g_pSortTrackers;
	}
}
