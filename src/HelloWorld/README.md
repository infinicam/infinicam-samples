# HelloWorld


<hr>

HelloWorld is a Windows sample application for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/) and its SDK with OpenCV.

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API [photron::PUCLib_Wrapper](../../inc/PUCLib_Wrapper.h).

This code sample is just provides the minimal code to open a camera, capture and save one image, and then close the camera. 


## Prerequisites
* Installed Visual Studio 2019

    :warning: The MFC Package is required.

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this source code.
   
3. Open [HelloWorld.sln](./HelloWorld.sln) on visual studio.

4. Build.

------------

## Operation

1. Connect INIFINICAM UC-1 to your Windows PC with USB-C cable.
2. Launch HelloWorld.exe in the bin folder.
3. The application save the image as "test.bmp"


#### Developed by: Photron Ltd.
