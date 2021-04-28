// bladeTrack.cpp
//

#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <stdio.h>
#include <thread>
#include <mutex>
using namespace cv;
using namespace std;
   
#include "PhotronVideoCapture.h"

#define VISUAL_OUTPUT

photron::VideoCapture cap;
Mat frame;
vector < Point > centerCoord; // Temporal array for the circle marker center
vector < SYSTEMTIME > timeArray;
double threshLev;
bool terminateThread;
mutex mtx;

void captureThread()
{
    Mat Orig;
    while (1) {
        if (terminateThread)break;
        cap.read(Orig);
        if (Orig.empty()) {
            cerr << "ERROR! Unable to capture\n";
            return;
        }
        SYSTEMTIME st;
        GetSystemTime(&st);

        Point p = Point(0, 0);
        Mat bw;
        vector < vector < Point > > contours;
        vector < Rect > cRects;
        double maxArea = 4000.;

        lock_guard<mutex> lock(mtx);
        Orig.copyTo(frame);
        threshold(frame, bw, threshLev, 255, THRESH_BINARY);
        findContours(bw, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
        for (int i = 0; i < contours.size(); i++) {
            double len = arcLength(contours[i], true);
            double area = contourArea(contours[i]);
            if (len * len / area < 15.0) { // Circle Probability
                if (area > maxArea) { // Selecting biggest circle
                    Rect tmp = boundingRect(contours[i]);
                    maxArea = area;
                    p.x = tmp.x + tmp.width / 2; // Center coord or rect
                    p.y = tmp.y + tmp.height / 2;
                }
            }
        }
        timeArray.push_back(st);
        centerCoord.push_back(p);
    }
}

int main()
{
    //--- INITIALIZE VIDEOCAPTURE
    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API


    threshLev = 64.0;
    bool refreshImg = true;
    Mat chartCanvas;

    cap.open(deviceID, apiID);
    if (!cap.isOpened()) {
        cerr << "ERROR! Unable to open camera\n";
        return -1;
    }


    PUC_HANDLE mHandle = cap.getPUCLibWrapper()->getPUCHandle();
    UINT32 nFramerate, nShutterspeed;

    cap.getPUCLibWrapper()->pause();
    PUCRESULT result0 = cap.getPUCLibWrapper()->setFramerateShutter((UINT32)1000, (UINT32)2000); // 1000, 2000
    cap.getPUCLibWrapper()->resume();

    PUCRESULT result1 = cap.getPUCLibWrapper()->getFramerateShutter(&nFramerate, &nShutterspeed);
    printf("framerateshutter setting returns %ld , %ld", nFramerate, nShutterspeed);


    terminateThread = false;
    thread thCapture(captureThread);

    int prevSec = -1;
    for (;;)
    {
        if (centerCoord.size() < 1)continue;
#ifdef VISUAL_OUTPUT
        int index; // Latest capture index
        Mat bw3ch;
        {
            lock_guard<mutex> lock(mtx);
            index = (int)centerCoord.size() - 1;
            cvtColor(frame, bw3ch, COLOR_GRAY2RGB);
        }

        Point p = centerCoord[index];
        // cross cursor
        if(p.x != 0 || p.y != 0){
            Point p0, p1;
            p0.x = p.x - 10; p0.y = p.y;
            p1.x = p.x + 10; p1.y = p.y;
            line(bw3ch, p0, p1, Scalar(255, 0, 255), 2);
            p0.x = p.x; p0.y = p.y - 10;
            p1.x = p.x; p1.y = p.y + 10;
            line(bw3ch, p0, p1, Scalar(255, 0, 255), 2);
        }
        if (chartCanvas.empty()) {
            chartCanvas.create(Size(bw3ch.cols/2, bw3ch.cols/2), CV_8UC3);
        }
        SYSTEMTIME st = timeArray[index];
        printf("%04d(%04d,%4d)%02d:%02d:%02d:%03d\n", index, p.x, p.y, 
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

        chartCanvas.setTo(Scalar(255, 255, 255));
        int drawStart;
        if (centerCoord.size() <= chartCanvas.cols) {
            drawStart = 0;
        }
        else {
            drawStart = (int)centerCoord.size() - chartCanvas.cols;
        }

        for (int i = drawStart; i < centerCoord.size(); i++) {
            Point pt_y, pt_x;
            pt_x.x = i - drawStart;
            pt_x.y = centerCoord[i].x/2;
            pt_y.x = pt_x.x;
            pt_y.y = centerCoord[i].y/2;
            circle(chartCanvas, pt_x, 1, Scalar(0, 200, 0));
            circle(chartCanvas, pt_y, 1, Scalar(0, 0, 255));
            if (i >= 1 && (timeArray[i - 1].wSecond != timeArray[i].wSecond)) {
                line(chartCanvas, Point(pt_x.x, 0), Point(pt_x.x, chartCanvas.rows - 1), Scalar(0, 0, 0), 2);
            }
        }

        // Seconds ruled line
        if (st.wSecond != prevSec) {
        }


        if (refreshImg) {
            imshow("Photron OpenCV Live Demo", bw3ch);
            if (!chartCanvas.empty()) {
                imshow("Circle Trail", chartCanvas);
            }
        }

        int c = waitKey(1);
        if (c > 0) {
            if (c == '>') {
                threshLev += 2.0;
                if (threshLev > 255)threshLev = 255;
            }
            else if (c == '<') {
                threshLev -= 2.0;
                if (threshLev < 0)threshLev = 0;
            }
            else if (c == 't') {
                if (refreshImg) {
                    refreshImg = false;
                }
                else {
                    refreshImg = true;
                }
            }
            else if (c == 0x1b) { // Hit esc to exit
                terminateThread = true;
                thCapture.join();
                break;
            }
        }
#else  //        VISUAL_OUTPUT
        Point p;
        p = centerCoord[centerCoord.size() - 1];
        GetSystemTime(&st);
        printf("%04d(%04d,%4d)%02d:%02d:%02d:%03d\n", centerCoord.size(), p.x, p.y, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        if (centerCoord.size() >= 2000) {
            return 0;
        }
#endif //        VISUAL_OUTPUT


    }
}
