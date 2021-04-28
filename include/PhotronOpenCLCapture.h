#pragma once

/*! 
	@mainpage
	@~english
		@brief OpenCL VideoCapture
	@~japanese
		@brief 
	
	@copyright Copyright (C) 2020 PHOTRON LIMITED
*/

#include "CL/cl.hpp"
#include "PUCLib_Wrapper.h"

namespace photron {

	class OpenCLCapture {
		photron::PUCLib_Wrapper* m_capture;
		int m_rowBytes = 0;
	public:
		OpenCLCapture() {
			m_capture = new PUCLib_Wrapper();
		}
		~OpenCLCapture() {
			delete m_capture;
		}

		const char* getLastErrorName() const {
			return m_capture->getLastErrorName();
		}

		bool open(int deviceID) {
			return m_capture->open(deviceID) == PUC_SUCCEEDED;
		}

		bool isOpened() {
			return m_capture->isOpened();
		}


		int uploadImage(cl::Context& context, cl::CommandQueue &queue, cl::Image2D &image)
		{
			int width, height, rowBytes;
			unsigned char* pDecodeBuf = m_capture->read(width, height, rowBytes);
			m_rowBytes = rowBytes;
			if (!pDecodeBuf)
				return 1;
			cl::size_t<3> origin;
			origin[0] = 0;
			origin[1] = 0;
			origin[2] = 0;
			cl::size_t<3> region;
			region[0] = width;
			region[1] = height;
			region[2] = 1;
			image = cl::Image2D(context, CL_MEM_READ_ONLY, cl::ImageFormat(CL_R, CL_UNSIGNED_INT8), width, height);
			int err = queue.enqueueWriteImage(image, CL_TRUE, origin, region, rowBytes, 0, pDecodeBuf);
			return err;
		}

		void deleteTexture() {
			
		}

		void getResolution(int& width, int& height) {
			m_capture->getResolution(width, height);
		}
		int getRowBytes() {
			return m_rowBytes;
		}

		PUCLib_Wrapper* getPUCLibWrapper() {
			return m_capture;
		}

	};

}
