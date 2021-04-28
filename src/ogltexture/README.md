# opengltexture


<hr>

opengltexture is a Windows sample application for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/) and its SDK with OpenGL. 

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API that allows you to store the image in an OpenGL texture using the class [photron::PhotronOpenGLCapture](../../inc/PhotronOpenGLCapture.h).

The code sample shows how to integrate INFINICAM into an application that uses OpenGL. The class allows you to update an OpenGL texture that can be then used to apply a fragment shader. The examples shaders are thermal (color correction), edge detection, invert colors, flip horizontal and flip vertical.


## Environment
* installed Visual Studio 2019

    :warning: MFC Package is required.

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this source code.
   
3. Open [opengltexture.sln](./opengltexture.sln) on visual studio.

4. Copy glut64.dll from ogltexture\ogltexture\GL to bin folder

5. Build

------------

## Operation

1. Connect INIFINICAM UC-1 to your Windows PC with USB-C cable.
2. Launch opengltexture.exe in the bin folder.
3. The application will show the live output of the camera in a window.
4. Click the right menu to select the different shader
5. To exit, hit ESC key after focusing to the live window.


#### developed by: Photron Ltd.
