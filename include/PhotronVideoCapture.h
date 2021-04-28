#pragma once

/*! 
	@mainpage
	@~english
		@brief OpenCV like VideoCapture
	@~japanese
		@brief OpenCV•—‚ÌVideoCapture
	
	@copyright Copyright (C) 2020 PHOTRON LIMITED
*/

#include "PUCLib_Wrapper.h"

#include <opencv2/core.hpp>
using namespace cv;

namespace photron {

	class VideoCapture {
		photron::PUCLib_Wrapper* m_capture;
	public:
		VideoCapture() {
			m_capture = new PUCLib_Wrapper();
		}
		~VideoCapture() {
			delete m_capture;
		}

		const char* getLastErrorName() const {
			return m_capture->getLastErrorName();
		}

		bool open(int deviceID, int apiID) {
			return m_capture->open(deviceID) != PUC_SUCCEEDED;
		}

		bool isOpened() {
			return m_capture->isOpened();
		}

		bool read(cv::Mat &img)
		{
			int width, height, rowBytes;
			unsigned char* pDecodeBuf = m_capture->read(width, height, rowBytes);
			if (!pDecodeBuf)
				return false;

			img = cv::Mat(height, width, CV_8UC1, pDecodeBuf, rowBytes);
			return true;
		}

		VideoCapture& operator>> (Mat& image) {
			read(image);
			return *this;
		}

		enum {
			CAP_PROP_FRAME_WIDTH_HEIGHT = 100000,
			CAP_PROP_FRAMERATE_SHUTTER_SPEED,
			CAP_PROP_FAN_STATE,
			CAP_PROP_EXPOSURE_TIME_ON_OFF_CLK
		};

		bool set(int propId, unsigned int value, unsigned int value2 = 0) {

			switch (propId) {
			case CAP_PROP_FRAMERATE_SHUTTER_SPEED:
				return m_capture->setFramerateShutter(value, value2);
				break;
			case CAP_PROP_FRAME_WIDTH_HEIGHT:
				return m_capture->setResolution(value, value2);
			case CAP_PROP_FAN_STATE:
				return m_capture->setFanState(static_cast<PUC_MODE>(value));
			case CAP_PROP_EXPOSURE_TIME_ON_OFF_CLK:
				return m_capture->setExposeTime(value, value2);
			}

			return false;
		}

		bool set(int propId, double value) {
			cerr << "Not supported" << endl;
			return false;
		}

		PUCLib_Wrapper* getPUCLibWrapper() {
			return m_capture;
		}

	};

}
