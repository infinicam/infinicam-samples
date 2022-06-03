# cvdemo


<hr>

cvdemo is a Windows sample application for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/) and its SDK with OpenCV.

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API that mimics OpenCV's Video Capture class [VideoCapture](https://docs.opencv.org/3.4/d8/dfe/classcv_1_1VideoCapture.html).

The photron OpenCV class is named [photron::VideoCapture](../../inc/VideoCapture.h) and is intended to reproduce the functionaility of cv::VideoCapture. The code sample is similar to the basic OpenCV VideoCapture sample example but just ported to photron's VideoCapture class. 


## Prerequisites
* Installed Visual Studio 2019

    :warning: The MFC Package is required.

* OpenCV Version 4.2.0 or higher (included in the Photron GitHub repository)

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this source code.

3. Set the environment variable OPENCV_DIR and set to the directory you installed OpenCV.

4. Copy the OpenCV DLL's to the bin folder.
   
5. Open [cvdemo.sln](./cvdemo.sln) on visual studio.

6. Build.

------------

## Operation

1. Connect INIFINICAM UC-1 to your Windows PC with USB-C cable.
2. Launch cvdemo.exe in the bin folder.
3. The application will show the live output of the camera in a window.
4. To exit, press the [ESC] key after focusing to the live window.


#### Developed by: Photron Ltd.
