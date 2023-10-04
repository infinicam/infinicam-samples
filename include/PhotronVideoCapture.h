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

	class VideoCaptureImageListener {
	public:
		virtual void imageReady(Mat &mat, USHORT sequenceNum) = 0;
	};


	class VideoCapture : public PUCLib_WrapperImageListener
	{
		photron::PUCLib_Wrapper* m_wrapper;
		VideoCaptureImageListener *m_listener = nullptr;

		virtual void imageReady(unsigned char* image, int width, int height, int rowBytes, USHORT sequenceNum) {
			if (m_listener == nullptr)
				return;
			Mat mat = cv::Mat(height, width, CV_8UC1, image, rowBytes);
			m_listener->imageReady(mat, sequenceNum);
		}
	public:
		VideoCapture() {
			m_wrapper = new PUCLib_Wrapper();
		}
		~VideoCapture() {
			delete m_wrapper;
		}


		void addListener(VideoCaptureImageListener* listener) {
			this->m_listener = listener;
			if(listener == nullptr)
				m_wrapper->addListener(nullptr);
			else
				m_wrapper->addListener(this);
		}

		const char* getLastErrorName() const {
			return m_wrapper->getLastErrorName();
		}

		bool open(int deviceID, int apiID) {
			return m_wrapper->open(deviceID) != PUC_SUCCEEDED;
		}

		bool isOpened() {
			return m_wrapper->isOpened();
		}

		void release() {
			m_wrapper->close();
		}

		bool read(cv::Mat &img)
		{
			int width, height, rowBytes;
			unsigned char* pDecodeBuf = m_wrapper->read(width, height, rowBytes);
			if (!pDecodeBuf)
				return false;

			img = cv::Mat(height, width, CV_8UC1, pDecodeBuf, rowBytes);
			return true;
		}

		void setFrameSampleRate(int dctRate, int dcRate) {
			m_wrapper->setFrameSampleRate(dctRate, dcRate);
		}

		bool readProxy(cv::Mat& img)
		{
			int width, height, rowBytes;
			unsigned char* pDecodeBuf = m_wrapper->readProxy(width, height, rowBytes);
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
				return m_wrapper->setFramerateShutter(value, value2);
				break;
			case CAP_PROP_FRAME_WIDTH_HEIGHT:
				return m_wrapper->setResolution(value, value2);
			case CAP_PROP_FAN_STATE:
				return m_wrapper->setFanState(static_cast<PUC_MODE>(value));
			case CAP_PROP_EXPOSURE_TIME_ON_OFF_CLK:
				return m_wrapper->setExposeTime(value, value2);
			}

			return false;
		}

		bool set(int propId, double value) {
			std::cerr << "Not supported" << std::endl;
			return false;
		}

		PUCLib_Wrapper* getPUCLibWrapper() {
			return m_wrapper;
		}

	};

}
