# bladeTrack

![app](image/sc03.png)

<hr>

bladeTrack is a Windows sample application for USB high-speed streaming camera [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/) and its SDK with OpenCV.

It illustrates how the INFINICAM could be used to achieve real-time live image processing with simple capturing API. As the nature of high-speed live image processing, the limited performance of the host PC and its ability to handle large amounts of streaming data are key to building an efficient analysis system.

Unlike frame-by-frame image analysis of pre-recorded images, live image analysis requires immediate feedback of the high-speed events, minimizing the overhead of frame-by-frame processing while displaying the necessary visual information and interacting with the user and other peripherals equipments.

This strategy results in a program design that is essentially different from the usual OpenCV video processing code.

The sample application "bladeTrack" explains most simple scheme of capture / process thread in conjunction with the live display. Users can easily modify the processing code to build the custom processing software while maintaining the high-speed capture and moderate rate of GUI display.

## What is bladeTrack?

<img src="image/fanblade.png" width="300">

Fan blades spin very fast even in the "Slow" mode. If it rotates 10 times per second, the angle is 3,600 degrees. If you record the fan with a normal 30 fps camera, it will rotate 120 degrees per frame. Even with very short shutter speed and strobe light, you can only take three images for every revolution of the fan.

<img src="image/normalcam.png" width="200">

As expected, a camera with 30 fps will not provide meaningful information even with advanced image processing.

Now INFINICAM UC-1 can capture images at 1,000 frames per second, which is 33 times faster than a 30 fps camera. By applying some image detection processing, we are now able to get more detailed rotating information.

<img src="image/highspeedcam.png" width="200">

We put a round sticker on one of the blades as a marker. The application will output the x-y coordinates of the center of gravity of the circle on the screen.

The application displays the position of the center of gravity of the detected circle in real time on the live image with a crosshair cursor (not at 1,000 fps), plots the x-y coordinates in a white chart window, and the printf output the coordinates and time in a command prompt window.

<img src="image/overlay.png" width="350">  

<img src="image/chart.png" width="200">

## Prerequisites
* Installed Visual Studio 2019

    :warning: The MFC Package is required.

* OpenCV Version 4.2.0 or higher (included in the Photron GitHub repository)

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this source code.

3. Set the environment variable OPENCV_DIR and set to the directory you installed OpenCV.

4. Copy the OpenCV DLL's to the bin folder.
   
5. Open [bladeTrack.sln](./bladeTrack.sln) on visual studio.

6. Build.



------------

## Operation

1. Prepare a round marker with a radius length of at least 100 pixels in the screen.
2. Stick a marker on any moving object, such as fan blades or metronome, or even on your own wrist. Brightness contrast is important. Use a white marker for dark objects and a black marker for light objects.
3. Connect INIFINICAM UC-1 to your Windows PC with USB-C cable.
4. Launch bladeTrack.exe in the bin folder.
5. Record the marker motion by the camera.
6. You can see the cross cursor of the biggest circle in the live window and the X-Y coordinate in the chart.
7. To exit, focus on the live window and then press the [ESC] key.

The vertical black bar indicates the unit of time (seconds).

## Detection Algorithm

We use the following algorithm to detect the centroid coordinate of the marker:

1. Deep-copy the captured frame into an OpenCV Mat. This is necessary to avoid tearing the image from two adjacent frames.
2. Threshold the image into black and white image.
3. Detect contours from the image.
4. For each closed contours, calculate the perimeter and the area space.
5. Calculate the square of the perimeter and divide it by the area as the circularity.
6. If the circularity is smaller than 15 then mark it as a candidate.
7. Select the biggest contour from the candidates.
8. Get the envelop rectangle and calculate the centroid.

## Program Structure

<img src="image/flow.png" width="400">

## Program Tips

1. To capture and process as many images as possible in the software, the capture/processing part is made into a function and run as a separate thread.
2. The camera encodes the image into a compressed format to keep the bit rate low, so you need to decode the image on your PC. Since the decoding is rather heavy process, it requires multithread distribution. This is a different layer of thread than the above capture thread.
3. The display refresh depends on the OpenCV draw/show frequency. When the main thread is ready to start displaying, it will try to get the latest captured image and the detected coordinates. This means that not all frames will show the live image and the crosshair cursor. Most of them are not displayed.
4. The chart, on the other hand, needs to show all the data. All the detected coordinates are stored in an array (vector < Point > centerCoord). The main thread can access to entire data all the time. When it draws the chart, the latest N frames data is extracted and used to plot.
5. When doing the capture and process in the capture thread, std::mutex protects the data. When fetching the latest image and the associated data in the main thread, again std::mutex protects the data. If you don't use mutex, the image and the data of the GUI might be overridden by the fast capture process thread.

#### Developed by: Photron Ltd.
