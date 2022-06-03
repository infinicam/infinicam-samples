// eyes.cpp
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

// #define SINGLE_CAMERA_TEST

#define VISUAL_OUTPUT
#define ACCEL_THRESH 400
#define MATCH_OFFSET_THRESH 4
#define BLOB_ARRAY_RING_SIZE 20
#define CAMERA_WIDTH 1246
#define CAMERA_HEIGHT 400
#define CAM_CMD_WAIT 1000 // msec

#define IMAGE_SCALE 0.125

// Two separated image streams
photron::VideoCapture capR;
#ifndef SINGLE_CAMERA_TEST
photron::VideoCapture capL;
#endif

Mat frameR;
Mat frameL;
Mat bwR;
Mat bwL;

bool synchronized = false;

// Data for clapper processing
vector < Point > maxBlobCenterArrayR; // Temporal array for the circle marker center
vector < Point > maxBlobCenterArrayL; // Temporal array for the circle marker center
vector < Rect > cRectsR;
vector < Rect > cRectsL;

// For the fps measurement
int captureCounterL;
int captureCounterR;
USHORT seqNoL;
USHORT seqNoR;

// Stereo variables
Mat disparity;
int matchOffset;
Ptr<StereoBM> bm;

// Misc extern varaiables
double threshLev;
bool terminateThread;
mutex mtx;

vector <Mat> ringR;
vector <Mat> ringL;
double scaleBias;
int roioffx, roioffy;
int dpt, sblk;
vector <int> depthVector;
vector <int> depthAreaVector;
#define DEPTH_CHART_WIDTH 2000

// 
// Getting the acceleration vector
// 
void getAccel(vector < Point >& crd, int cFrm, double acc[2])
{
    double vec0[2];
    double vec1[2];

    vec0[0] = crd[cFrm - 1].x - crd[cFrm - 2].x;
    vec0[1] = crd[cFrm - 1].y - crd[cFrm - 2].y;
    vec1[0] = crd[cFrm].x - crd[cFrm - 1].x;
    vec1[1] = crd[cFrm].y - crd[cFrm - 1].y;

    acc[0] = vec1[0] - vec0[0];
    acc[1] = vec1[1] - vec0[1];

    return;
}

// 
// Getting the maximum acceleration vector
// 
int getMaxAccel(vector < Point >& crd, double acc[2])
{
    double maxScaleSq = 1.0;
    int maxIdx = -1;        

    for (int i = 2; i < crd.size(); i++) {
        if (crd[i-2].x == 0 && crd[i-2].y == 0)return -1;
        if (crd[i-1].x == 0 && crd[i-1].y == 0)return -1;
        if (crd[i].x == 0 && crd[i].y == 0)return -1;
        double a[2];
        getAccel(crd, i, a);
        double aSize = a[0] * a[0] + a[1] * a[1];
        if (aSize > maxScaleSq) {
            maxScaleSq = aSize;
            acc[0] = a[0];
            acc[1] = a[1];
            maxIdx = i;
        }
    }

    return maxIdx;
}

void getMaxBlobCenter(cv::Mat &bw, cv::Point &pnt)
{
  double maxArea;
  vector < vector < Point > > contours;
  findContours(bw, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
  maxArea = 4000.;
  for (int i = 0; i < contours.size(); i++) {
    double len = arcLength(contours[i], true);
    double area = contourArea(contours[i]);
    if (area > maxArea) { // Selecting biggest circle
      Rect tmp = boundingRect(contours[i]);
      maxArea = area;
      pnt.x = tmp.x + tmp.width / 2; // Center coord or rect
      pnt.y = tmp.y + tmp.height / 2;
    }
  }
}

int getBlobLevel(cv::Mat &p, int &area)
{
  int maxLev = -1000;
  int maxCount = 0;
#if 1
  maxLev = p.at<short>(p.rows/2, p.cols/2);
  area = 1;
  return maxLev;
  for(int j=0;j<p.rows;j++){
    for(int i=0;i<p.cols;i++){
      if(p.at<short>(j,i) == maxLev){
	maxCount++;
      }
    }
  }
#else
  for(int j=0;j<p.rows;j++){
    for(int i=0;i<p.cols;i++){
      if(p.at<short>(j,i) > maxLev){
	maxLev = p.at<short>(j,i);
	maxCount = 1;
      }else if(p.at<short>(j,i) == maxLev){
	maxCount++;
      }
    }
  }
#endif
  if(maxCount > p.rows * p.cols / 16){
    area = maxCount;
    return maxLev;
  }
  area = 0;
  return 0;
}

// 
// Capture and processing thread function
// 
void captureThread()
{
  Mat OrigR;
  Mat OrigL;
  captureCounterL = 0;
  captureCounterR = 0;
  std::cerr << "starting capture thread."  << std::endl;
  while (1) {
    if (terminateThread)break;
    {
      lock_guard<mutex> lock(mtx);
      bool retR = capR.read(OrigR);
      if (!retR) {
	cerr << "ERROR! Unable to capture 0\n";
	break;
      }
      seqNoR = capR.getPUCLibWrapper()->getSequenceNumber();
      captureCounterR++;
    }
    {
#ifndef SINGLE_CAMERA_TEST
      lock_guard<mutex> lock(mtx);
      bool retL = capL.read(OrigL);
      if (!retL) {
	cerr << "ERROR! Unable to capture 1\n";
	break;
      }
      seqNoL = capL.getPUCLibWrapper()->getSequenceNumber();
      captureCounterL++;
#else
      OrigR.copyTo(OrigL);
      seqNoL = seqNoR;
      captureCounterL = captureCounterR;
#endif
    }

#if 1 // Scale Bias is not necessary if the lens for the cameras are same 
#ifndef SINGLE_CAMERA_TEST
    //  Scale Bias (In case the camera view angle is different)
    Mat tmpMat;
    resize(OrigR, tmpMat, cv::Size(0, 0), scaleBias, scaleBias, INTER_NEAREST);
    if(roioffx + OrigL.cols <= tmpMat.cols - 1 && roioffy + OrigL.rows <= tmpMat.rows - 1){
      cv::Rect roi(roioffx, roioffy, OrigL.cols, OrigL.rows);
      OrigR = tmpMat(roi);
    }
    if (OrigR.empty()) {
      cerr << "ERROR! Unable to scalebias 0\n";
      return;
    }
#endif
#endif
    
    {
      lock_guard<mutex> lock(mtx);
      OrigR.copyTo(frameR);
      OrigL.copyTo(frameL);
    }
    if(synchronized) {
      // 
      // Stereo Calculation (after clapper)
      // 
      cv::Mat stereoR, stereoL;
      if (matchOffset == 0) {
	resize(OrigR, stereoR, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
	resize(OrigL, stereoL, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
      }else{
	ringR.push_back(OrigR);
	ringL.push_back(OrigL);
	if (ringR.size() == abs(matchOffset) + 1) {
	  ringR.erase(ringR.begin());
	  ringL.erase(ringL.begin());
	  if (matchOffset > 0) {
	    resize(ringR[0], stereoR, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
	    resize(OrigL, stereoL, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
	  }else{
	    resize(OrigR, stereoR, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
	    resize(ringL[0], stereoL, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
	  }
	}else{
	  continue;
	}
      }
      {
	lock_guard<mutex> lock(mtx);
	bm->compute(stereoL, stereoR, disparity);
	int area;
	int depthVal = getBlobLevel(disparity, area);
	depthVector.push_back(depthVal);
	depthAreaVector.push_back(area);
	if(depthVector.size() == DEPTH_CHART_WIDTH){
	  depthVector.erase(depthVector.begin());
	  depthAreaVector.erase(depthAreaVector.begin());
	}
      }
      continue;
    }else{
      // 
      // Clapper
      // 

      // Max Blob Center
      Point cntR = Point(0, 0);
      Point cntL = Point(0, 0);
      threshold(OrigR, bwR, threshLev, 255, THRESH_BINARY);
      threshold(OrigL, bwL, threshLev, 255, THRESH_BINARY);
      getMaxBlobCenter(bwR, cntR);
      getMaxBlobCenter(bwL, cntL);
      maxBlobCenterArrayR.push_back(cntR);
      maxBlobCenterArrayL.push_back(cntL);
      
      if(maxBlobCenterArrayR.size() < BLOB_ARRAY_RING_SIZE){
	continue;
      }
      int apexR = -1;
      int apexL = -1;
      double accR[2];
      double accL[2];

      apexR = getMaxAccel(maxBlobCenterArrayR, accR);
      //  If big acceleration found in the duration...
      if (apexR > 0 && (abs(accR[0]) > ACCEL_THRESH || abs(accR[1]) > ACCEL_THRESH)) {
	// Check the max accel in the other camera
	apexL = getMaxAccel(maxBlobCenterArrayL, accL);
      }
      if (apexL > 0) {
	matchOffset = apexL - apexR;
	std::cerr << "(0)" << apexR << "," << maxBlobCenterArrayR[apexR - 2] << maxBlobCenterArrayR[apexR-1] << maxBlobCenterArrayR[apexR] << std::endl;
	std::cerr << "(1)" << apexL << "," << maxBlobCenterArrayL[apexL-2] << maxBlobCenterArrayL[apexL-1] << maxBlobCenterArrayL[apexL] << std::endl;
	std::cerr << "accR " << accR[0] << "," << accR[1] << std::endl;
	std::cerr << "accL " << accL[0] << "," << accL[1] << std::endl;
	std::cerr << "matchOffset = " <<  matchOffset << " ---- clapped. " << std::endl;
	if(matchOffset < MATCH_OFFSET_THRESH){
	  synchronized = true;
	  maxBlobCenterArrayR.clear();
	  maxBlobCenterArrayL.clear();
	  continue;
	}
      }
      maxBlobCenterArrayR.erase(maxBlobCenterArrayR.begin());
      maxBlobCenterArrayL.erase(maxBlobCenterArrayL.begin());
    }
  }
}

int main()
{
    //--- INITIALIZE VIDEOCAPTURE
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API

    bool refreshImg = true;
#if 1
    threshLev = 64.0;
    Mat chartCanvas;
#endif
    // 
    // Camera Open and Initialization
    // 
    cerr << "Opening Camera R ...";
    capR.open(0, apiID);
    if (!capR.isOpened()) {
        cerr << "ERROR! Unable to open camera0\n";
        return -1;
    }
    cerr << " done\n";
    Sleep(CAM_CMD_WAIT);
    UINT32 nFramerate0, nShutterspeed0;
    int width0, height0;
    cerr << "Pausing Camera R and ... ";
    capR.getPUCLibWrapper()->pause();
    cerr << "Setting resulution ...";
    Sleep(CAM_CMD_WAIT);
    PUCRESULT result01 = capR.getPUCLibWrapper()->getFramerateShutter(&nFramerate0, &nShutterspeed0);
    PUCRESULT result02 = capR.getPUCLibWrapper()->setResolution(CAMERA_WIDTH, CAMERA_HEIGHT);
    std::cerr << "Done. setrez(0) result=" << result02 << std::endl;
    // capR.getPUCLibWrapper()->getResolution(width0, height0);
    // printf("(cam0)(%d) %ld, %ld, (%d, %d)\n", result01, nFramerate0, nShutterspeed0, width0, height0);
    Sleep(CAM_CMD_WAIT);
    capR.getPUCLibWrapper()->resume();
    cerr << "Camera R resumed.\n";

    // Sleep(CAM_CMD_WAIT);

#ifndef SINGLE_CAMERA_TEST
    cerr << "Opening Camera L ...";
    capL.open(1, apiID);
    if (!capL.isOpened()) {
        cerr << "ERROR! Unable to open camera1\n";
        return -1;
    }
    Sleep(CAM_CMD_WAIT);
    cerr << " done\n";
    UINT32 nFramerate1, nShutterspeed1;
    int width1, height1;
    cerr << "Pausing Camera L and ... ";
    capL.getPUCLibWrapper()->pause();
    cerr << "Setting resulution ...";
    Sleep(CAM_CMD_WAIT);
    PUCRESULT result11 = capL.getPUCLibWrapper()->getFramerateShutter(&nFramerate1, &nShutterspeed1);
    PUCRESULT result12 = capL.getPUCLibWrapper()->setResolution(CAMERA_WIDTH, CAMERA_HEIGHT);
    std::cerr << "Done. setrez(1) result=" << result12 << std::endl;
    // capL.getPUCLibWrapper()->getResolution(width1, height1);
    // printf("(cam1)(%d) %ld, %ld, (%d, %d)\n", result11, nFramerate1, nShutterspeed1, width1, height1);
    Sleep(CAM_CMD_WAIT);
    capL.getPUCLibWrapper()->resume();
    cerr << "Camera L resumed.\n";
#endif

    // 
    // Launching capture and processing thread
    // 
    terminateThread = false;
    thread thCapture(captureThread);

    matchOffset = 10000;
    int prevSec = -1;
    Mat sframeR;
    Mat sframeL;

    // For the concatinated display
    vector <Mat> bw3ch(2);
    Mat combined;

    synchronized = true;
    scaleBias = 1.5;
    roioffx = 233;
    roioffy = 80; // 180 = CAMERA_HEIGHT 800

    dpt = 32;
    sblk = 17;
    bm = StereoBM::create(dpt, sblk); // 16,9
    int preCaptureCounterL = 0;
    int preCaptureCounterR = 0;

    for (;;)
    {
        int index0; // Latest capture index
        int index1; // Latest capture index
	if (!frameR.empty()) {
	  cerr << "frameR=(" << frameR.cols << "," << frameR.rows << ")" << std::endl;
	  cv::resize(frameR, sframeR, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_LINEAR);
	  cvtColor(sframeR, bw3ch[0], COLOR_GRAY2RGB);
	}else{
	  std::cerr << "frameR is empty" << std::endl;
	}
	if (!frameL.empty()) {
	  cerr << "frameL=(" << frameL.cols << "," << frameL.rows << ")" << std::endl;
	  cv::resize(frameL, sframeL, cv::Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_LINEAR);
	  cvtColor(sframeL, bw3ch[1], COLOR_GRAY2RGB);
	}else{
	  std::cerr << "frameL is empty" << std::endl;
	}
        index0 = (int)maxBlobCenterArrayR.size() - 1;
        index1 = (int)maxBlobCenterArrayL.size() - 1;

        if (index0 > 0 || index1 > 0) {
            Point _pntR = maxBlobCenterArrayR[index0] * IMAGE_SCALE;
            Point _pntL = maxBlobCenterArrayL[index1] * IMAGE_SCALE;
            // cross cursor
            if (_pntR.x != 0 || _pntR.y != 0) {
                Point pntR, pntL;
                pntR.x = _pntR.x - 10; pntR.y = _pntR.y;
                pntL.x = _pntR.x + 10; pntL.y = _pntR.y;
                line(bw3ch[0], pntR, pntL, Scalar(255, 0, 255), 2);
                pntR.x = _pntR.x; pntR.y = _pntR.y - 10;
                pntL.x = _pntR.x; pntL.y = _pntR.y + 10;
                line(bw3ch[0], pntR, pntL, Scalar(255, 0, 255), 2);
            }
            if (_pntR.x != 0 || _pntR.y != 0) {
                Point pntR, pntL;
                pntR.x = _pntL.x - 10; pntR.y = _pntL.y;
                pntL.x = _pntL.x + 10; pntL.y = _pntL.y;
                line(bw3ch[1], pntR, pntL, Scalar(255, 0, 255), 2);
                pntR.x = _pntL.x; pntR.y = _pntL.y - 10;
                pntL.x = _pntL.x; pntL.y = _pntL.y + 10;
                line(bw3ch[1], pntR, pntL, Scalar(255, 0, 255), 2);
            }
        }

        if (refreshImg) {
            if (!bw3ch[0].empty() && !bw3ch[1].empty()) {
                cv::hconcat(bw3ch, combined);
                imshow("Photron OpenCV Live Demo 0", combined);
            }
            if (!disparity.empty()) {
                cv::Mat disparity_map;
                double min, max;
		std::cerr << "disparity depth = " << disparity.depth() << std::endl;
		std::cerr << "ref:" << CV_16S << "," << CV_32S << std::endl;
                cv::minMaxLoc(disparity, &min, &max);
		std::cerr << "disparity min = " << min << " / max = " << max << std::endl;
                // disparity.convertTo(disparity_map, CV_8UC1, 255.0 / (max - min), -255.0 * min / (max - min));
		if(max < 512){
		  disparity.convertTo(disparity_map, CV_8UC1, 0.5, 16);
		}
                cv::imshow("result", disparity_map);
            }
	    printf("Elapsed: R=%d, L=%d, SeqNo: R=%d, L=%d\n", 
		   captureCounterR - preCaptureCounterR, 
		   captureCounterL - preCaptureCounterL,
		   seqNoR, seqNoL);

	    if (chartCanvas.empty()) {
	      chartCanvas.create(Size(2000, 512), CV_8UC3);
	    }
	    chartCanvas.setTo(Scalar(255, 255, 255));
	    for (int i = 0; i < depthVector.size(); i++) {
	      Point pt_x, pt_y;
	      pt_x.x = i;
	      pt_x.y = chartCanvas.rows - depthVector[i];
	      if(i%100==0){
		cerr << "depthVector[" << i << "]=" << depthVector[i] << "/" << depthAreaVector[i] << std::endl;
	      }
	      // pt_y.x = pt_x.x;
	      // pt_y.y = centerCoord[i].y/2;
	      if(i % 33 ==0){
		circle(chartCanvas, pt_x, 3, Scalar(0, 0, 255), -1);
	      }else{
		circle(chartCanvas, pt_x, 1, Scalar(0, 200, 0));
	      }
	      // circle(chartCanvas, pt_y, 1, Scalar(0, 0, 255));
	    }
            if (!chartCanvas.empty()) {
	      imshow("Circle Trail", chartCanvas);
            }

	    preCaptureCounterL = captureCounterL;
	    preCaptureCounterR = captureCounterR;
        }
        int c = waitKey(33);
        if (c > 0) {
            if (c == '>') {
                threshLev += 2.0;
                if (threshLev > 255)threshLev = 255;
            }
            else if (c == '<') {
                threshLev -= 2.0;
                if (threshLev < 0)threshLev = 0;
            }
            else if (c == 'r') {
                matchOffset = 10000;
                disparity.release();
                synchronized = false;
            }
            else if (c == 'c') {
                matchOffset = 0;
                synchronized = true;
            }
            else if (c == 'u') {
                scaleBias += 0.1;
                cerr << "scaleBias=" << scaleBias << std::endl;
            }
            else if (c == 'x') {
                roioffx++;
                cerr << "roix=" << roioffx << std::endl;
            }
            else if (c == 'y') {
                roioffy++;
                cerr << "roix=" << roioffy << std::endl;
            }
            else if (c == 'd') {
                scaleBias -= 0.05;
                if (scaleBias < 0.1)scaleBias = 0.05;
            }
            else if (c == 'k') {
                dpt *= 2;
                cerr << "dpt=" << dpt << std::endl;
            }
            else if (c == 'K') {
                dpt /= 2;
                if (dpt == 1)dpt = 16;
                cerr << "dpt=" << dpt << std::endl;
            }
            else if (c == 'l') {
                sblk += 2;
                cerr << "sblk=" << sblk << std::endl;
            }
            else if (c == 'L') {
                sblk -= 2;
                cerr << "sblk=" << sblk << std::endl;
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
    }
}
