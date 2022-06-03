# infinicam-sdk

<hr>

The INFINICAM SDK is a set of Windows sample applicat ions for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/).

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API. As the nature of high speed live image processing, dealing with the limited performance of host PC and massive streaming data is the key to construct an efficient analyzing system.

## Samples

* [bladeTrack](src/bladeTrack/README.md) The sample application "bladeTrack" explains most simple scheme of capture / process thread in conjunction with the live display. Users can easily modify the processing code to build the custom processing software maintaining the high speed capture and moderate rate of GUI display.

* [eyes](src/eyes/README.md) “eyes” is a sample infinicam SDK application that demonstrates the high-speed stereo scope imaging. By using the OpenCV stereo matching functions, “eyes” detects the objects depth (distance from the camera) at 1000 fps to draw a temporal depth chart, which is far more precise than traditional 30 fps processing.
The sample software illustrates how developers can work with two cameras. In addition, the sample also explains two sophisticated performance enhancement techniques: “Vertical Cropping” and “Proxy Access”. Applying these techniques and using Infinicam, one can achieve 2000 fps or more even on an ordinary laptop.

* [HelloWorld](src/HelloWorld/README.md) The code sample is just provides the minimal code to open a camera, capture and save one image, and then close the camera. 

* [cvdemo](src/cvdemo/README.md) The code sample uses photron::VideoCapture and is meant to replicate the functionaility of cv::VideoCapture. The code sample is similar to the basic OpenCV VideoCapture sample example but just ported to photron's VideoCapture class. 

* [opengltexture](src/ogltexture/README.md) The code sample shows how to integrate INFINICAM into an application that uses OpenGL. The class allows you to update an OpenGL texture that can be then used to apply a fragment shader. The examples shaders are thermal (color correction), edge detection, invert colors, flip horizontal and flip vertical.

* [opencl](src/opencl/README.md) The code sample shows how to integrate INFINICAM into an application that uses OpenCL. The class allows you to update an OpenCL image that can be then is an OpenCL kernel is applied to create a new OpenCL image which is then read onto the host and finally saved to disk.

* [SingleImage](src/SingleImage/README.md) This sample code implements the function to get image and decode using INFINICAM SDK [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html).

#### developed by: Photron Ltd. 
