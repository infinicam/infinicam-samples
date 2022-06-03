// cvdemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>
using namespace cv;
using namespace std;

#include "PhotronVideoCapture.h"

int main(int argc, char** argv)
{
    Mat frame;
    //--- INITIALIZE VIDEOCAPTURE
    photron::VideoCapture cap;
    // open the default camera using default API
    // cap.open(0);
    // OR advance usage: select any API backend
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    // open selected camera using selected API
    cap.open(deviceID, apiID);
    // check if we succeeded
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }
    //--- GRAB AND WRITE LOOP
    cout << "Start grabbing" << endl
        << "Press any key to terminate" << endl;

    int prevmsec = 0;
    SYSTEMTIME st;
    bool showDc = true;

    cap.setFrameSampleRate(40,1);

    int counter = 0;
    for (;;)
    {
        GetSystemTime(&st);

        int dur;
        if (prevmsec > st.wMilliseconds) {
            dur = st.wMilliseconds + 1000 - prevmsec;
        }
        else {
            dur = st.wMilliseconds - prevmsec;
        }

        Mat frame;
        cap.readProxy(frame);   

        //printf("%d\n", dur);
        prevmsec = st.wMilliseconds;


        if (!frame.empty()) 
            imshow("FastCam DC", frame);
        
        Mat fullFrame;
        cap.read(fullFrame);
        if (!fullFrame.empty()) 
            imshow("FastCam DCT", fullFrame);
        
        
        int key = waitKey(1);
        if (key == 27)
            break;


        ++counter;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}