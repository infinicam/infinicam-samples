#pragma once

/*! 
	@mainpage
	@~english
		@brief OpenGL VideoCapture
	@~japanese
		@brief 
	
	@copyright Copyright (C) 2020 PHOTRON LIMITED
*/

#include "PUCLib_Wrapper.h"

namespace photron {

	class OpenGLCapture {
		photron::PUCLib_Wrapper* m_capture;
		GLuint textures[1];
	public:
		OpenGLCapture() {
			m_capture = new PUCLib_Wrapper();
		}
		~OpenGLCapture() {
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

		void createTexture() {
			glGenTextures(1, textures);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		bool updateTexture()
		{
			int width, height, rowBytes;
			unsigned char* pDecodeBuf = m_capture->read(width, height, rowBytes);
			if (!pDecodeBuf)
				return false;

			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pDecodeBuf);
			return true;
		}

		void deleteTexture() {
			glDeleteTextures(1, textures);
		}

		void getResolution(int& width, int& height) {
			m_capture->getResolution(width, height);
		}
	};

}
