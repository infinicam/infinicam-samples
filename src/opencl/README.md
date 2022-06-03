# opencl


<hr>

opencl is a Windows sample application for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/) and its SDK with OpenCL.

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API that allows you to store the image in an OpenCL image using the class [photron::PhotronOpenCLCapture](../../inc/PhotronOpenCLCapture.h).

This code sample shows how to integrate INFINICAM into an application that uses OpenCL. The class allows you to update an OpenCL image that can be then is an OpenCL kernel is applied to create a new OpenCL image which is then read onto the host and finally saved to disk.


## Prerequisites
* Installed Visual Studio 2019

    :warning: The MFC Package is required.

* OpenCV Version 4.2.0 or higher (included in the Photron GitHub repository)

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this source code.
   
3. Open [opencl.sln](./opencl.sln) on visual studio.

4. Build.

------------

## Operation

1. Connect INIFINICAM UC-1 to your Windows PC with USB-C cable.
2. Launch opencl.exe in the bin folder.
3. The application save the image as "test.bmp"


#### Developed by: Photron Ltd.
