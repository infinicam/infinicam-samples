// 
// eyes.cpp
// (Clapper: Synchronization function is removed and backuped to wyesWithClapper.cpp)
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
#define PROXY_PROCESS
#define PROXY_PROCESS_DISPLAY_FULLREZ

#define CAMERA_WIDTH 1246
#define CAMERA_HEIGHT 496
#define IMAGE_SCALE 0.125
#define DEPTH_CHART_WIDTH 1440
#define VIEW_SIZE 720
// 
// FRAMERATE CAMERA_HEIGHT
// --------- -------------
//     988      1024
//    1000      1008
//    2000       496
//    5000       176
//   10000        80
//   20000        32
//   31157        16
// 
#define SHUTTERSPEED 2000
#define FRAMERATE 2000

#define THRESHLEV 64
#define STEREO_DPT 32
#define STEREO_SBLK 17

double threshLev;
int stereoDpt;
int stereoSblk;

// Stereo image streams
photron::VideoCapture capR;
photron::VideoCapture capL;

// Stereo frame buffers
Mat frameR; // Copy of original R image
Mat frameL; // Copy of original L image

// For the fps measurement
USHORT processCounter;

// Stereo variables
Mat disparity;
Ptr<StereoBM> bm;
vector <int> depthVector;
// vector < SYSTEMTIME > timeArray;
// vector <USHORT> frameCountArray;

// Misc extern varaiables
bool terminateThread;
mutex mtx;

Rect ballRect;

void getBallRect(Mat &img, Rect &br)
{
  double maxArea;
  vector < vector < Point > > contours;
  Mat bw;
  threshold(img, bw, threshLev, 255, THRESH_BINARY);
  findContours(bw, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
  maxArea = 40.;
  br.x = 0;
  br.y = 0;
  br.width = 1;
  br.height = 1;
  for (int i = 0; i < contours.size(); i++) {
    double len = arcLength(contours[i], true);
    double area = contourArea(contours[i]);
    if (area > maxArea) { // Selecting biggest circle
      br = boundingRect(contours[i]);
      maxArea = area;
    }
  }
}

int getBallDisparity(Mat &p, Rect br)
{
  // Getting Average distance in the br rectangle 
#if 1
  double acumVal = 0.;
  int acumCnt = 0;
  for(int j = br.y;j < br.y + br.height;j++){
    for(int i = br.x;i < br.x + br.width;i++){
      if((int)p.at<short>(j,i) > 32){
	acumVal += (int)p.at<short>(j,i);
	acumCnt++;
      }
    }
  }
  
  return (int)(acumVal / acumCnt);
#else
  Point centerCrd;
  centerCrd.x = br.x + br.width / 2;
  centerCrd.y = br.y + br.height / 2;
  if(centerCrd.y > 0 && centerCrd.y < p.rows - 1 && 
     centerCrd.x > 0 && centerCrd.x < p.cols - 1){
    return (int)p.at<short>(centerCrd.y, centerCrd.x);
  }
  return 0;
#endif
}

// 
// Capture and processing thread function
// 
void captureProcessThread()
{
  Mat OrigR;
  Mat OrigL;
  processCounter = 0;
  cerr << "starting capture thread."  << endl;
  int capR_RetryCounter = 0;
  int capL_RetryCounter = 0;
  while (1) {
    if (terminateThread)break;
    {
      lock_guard<mutex> lock(mtx);
#ifdef PROXY_PROCESS
      bool retR = capR.readProxy(OrigR);
#else
      bool retR = capR.read(OrigR);
#endif
      if (!retR) {
	if(capR_RetryCounter++ > 1000){
	  cerr << "ERROR! Unable to capture R\n";
	  break;
	}
	cerr << "Retrying R-Capture: " << capR_RetryCounter << endl;
	continue;
      }else{
	capR_RetryCounter = 0;
      }
    }
    {
#ifndef SINGLE_CAMERA_TEST
      lock_guard<mutex> lock(mtx);
#ifdef PROXY_PROCESS
      bool retL = capL.readProxy(OrigL);
#else
      bool retL = capL.read(OrigL);
#endif
      if (!retL) {
	if(capL_RetryCounter++ > 1000){
	  cerr << "ERROR! Unable to capture L\n";
	  break;
	}
	cerr << "Retrying L-Capture: " << capL_RetryCounter << endl;
	continue;
      }else{
	capL_RetryCounter = 0;
      }
#else
      OrigR.copyTo(OrigL);
#endif
    }

    if (OrigR.empty()){
      cerr << "R channel is empty" << endl;
      break;
    }
    if (OrigL.empty()){
      cerr << "R channel is empty" << endl;
      break;
    }
    if (OrigR.rows != OrigL.rows || OrigR.cols != OrigL.cols){
      cerr << "R / L size mismatch ------ " << endl;
      break;
    }
    

    // 
    // Stereo Calculation
    // 
    processCounter++;
    if(processCounter >= 65534)processCounter = 0;
    {
      lock_guard<mutex> lock(mtx);
      OrigR.copyTo(frameR);
      OrigL.copyTo(frameL);
    }

    // Resize image for the faster processing
    Mat stereoR, stereoL;
#ifdef PROXY_PROCESS // DC ACCESS automatically fetch 1/8 image
    OrigR.copyTo(stereoR);
    OrigL.copyTo(stereoL);
#else // Otherwise do the scaling by CPU here
    resize(OrigR, stereoR, Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
    resize(OrigL, stereoL, Size(0, 0), IMAGE_SCALE, IMAGE_SCALE, INTER_NEAREST);
#endif

    // Calc ball center coordinate to sample the depth map
    Rect brR;
    Rect brL;
    // 
    getBallRect(stereoR, brR);
    getBallRect(stereoL, brL);
    ballRect.x = (int)((brR.x + brL.x) / 2.0);
    ballRect.y = (int)((brR.y + brL.y) / 2.0);
    ballRect.width = (int)((brR.width + brL.width) / 2.0);
    ballRect.height = (int)((brR.height + brL.height) / 2.0);

    // Stereo block matching and depth map generation
    {
      lock_guard<mutex> lock(mtx);
      bm->compute(stereoL, stereoR, disparity);
      int depthVal;
      depthVal = getBallDisparity(disparity, ballRect);

      // Store all the depth data to a vector (ring buffer)
      depthVector.push_back(depthVal);

      // FIFO for chart drawing area
      if(depthVector.size() == DEPTH_CHART_WIDTH){
	depthVector.erase(depthVector.begin());
      }
    }
  }
}

static void genPseudoMap(Mat &disparityPseudo, Mat &disparityMap)
{
#if 0
  for(int j=0;j<disparityPseudo.rows;j++){
    for(int i=0;i<disparityPseudo.cols;i++){
      int val = disparityMap.at<uchar>(j,i);
      disparityPseudo.at<Vec3b>(j,i)[0] =  val;
      disparityPseudo.at<Vec3b>(j,i)[1] =  val;
      disparityPseudo.at<Vec3b>(j,i)[2] =  val;
    }
  }

#else
  for(int j=0;j<disparityPseudo.rows;j++){
    for(int i=0;i<disparityPseudo.cols;i++){
      int val = disparityMap.at<uchar>(j,i);
      if(val < 64){ // Red to Yellow
	int oVal = (val) * 4;
	disparityPseudo.at<Vec3b>(j,i)[0] =  0;
	disparityPseudo.at<Vec3b>(j,i)[1] =  oVal;
	disparityPseudo.at<Vec3b>(j,i)[2] =  255;
      }else if(val < 128){ // Yellow to Green
	int oVal = (val - 64) * 4;
	disparityPseudo.at<Vec3b>(j,i)[0] =  0;
	disparityPseudo.at<Vec3b>(j,i)[1] =  255;
	disparityPseudo.at<Vec3b>(j,i)[2] =  255 - oVal;
      }else if(val < 192){ // Green to Cyan
	int oVal = (val - 128) * 4;
	disparityPseudo.at<Vec3b>(j,i)[0] =  oVal;
	disparityPseudo.at<Vec3b>(j,i)[1] =  255;
	disparityPseudo.at<Vec3b>(j,i)[2] =  0;
      }else{ // Cyan to Blue
	int oVal = (val - 192) * 4;
	disparityPseudo.at<Vec3b>(j,i)[0] =  255;
	disparityPseudo.at<Vec3b>(j,i)[1] =  255 - oVal;
	disparityPseudo.at<Vec3b>(j,i)[2] =  0;
      }
    }
  }
#endif
}

static int getElapsedMillisec(SYSTEMTIME &st, SYSTEMTIME &previousSt)
{
  int c_msec = st.wSecond * 1000 + st.wMilliseconds;
  int p_msec = previousSt.wSecond * 1000 + previousSt.wMilliseconds;
  if(c_msec > p_msec){
    return c_msec - p_msec;
  }
  return 60000 - p_msec + c_msec;
}

int main()
{
  cerr << "eyes.exe Version 1.02 (2902nov2021)" << endl;

  bool frozenChart = false;

  //--- INITIALIZE VIDEOCAPTURE
  int apiID = CAP_ANY;      // 0 = autodetect default API
  Mat chartCanvas;

  threshLev = THRESHLEV;
  stereoDpt = STEREO_DPT;
  stereoSblk = STEREO_SBLK;

  // 
  // Camera Open and Initialization
  // 
  UINT32 nFramerate1, nShutterspeed1;
  cerr << "Open / Initialize Camera R .";
  capR.open(0, apiID); cerr << ".";
  if (!capR.isOpened()){cerr << "ERROR! Unable to open camera0\n";return -1;}
#ifdef PROXY_PROCESS
#ifdef PROXY_PROCESS_DISPLAY_FULLREZ // full rez capture  
  capR.setFrameSampleRate(40,1);
#else
  capR.setFrameSampleRate(0,1);
#endif
#else
  capR.setFrameSampleRate(1,0);
#endif
  capR.getPUCLibWrapper()->pause();cerr << ".";
  nFramerate1 = FRAMERATE;
  nShutterspeed1 = SHUTTERSPEED;
  PUCRESULT result00 = capR.getPUCLibWrapper()->setFramerateShutter(nFramerate1, nShutterspeed1);
  PUCRESULT result01 = capR.getPUCLibWrapper()->getFramerateShutter(&nFramerate1, &nShutterspeed1);
  PUCRESULT result02 = capR.getPUCLibWrapper()->setResolution(CAMERA_WIDTH, CAMERA_HEIGHT);
  capR.getPUCLibWrapper()->resume();
  cerr << " done\n";
  cerr << "setFramerate: " << result00 << ", getFramerate: " << result01 << ", setRes: " << result02 << endl;
  cerr << "rate1:" << nFramerate1 << ", shutter1:" << nShutterspeed1 << endl;

#ifndef SINGLE_CAMERA_TEST
  UINT32 nFramerate2, nShutterspeed2;
  cerr << "Open / Initialize Camera L .";
  capL.open(1, apiID);cerr << ".";
  if (!capL.isOpened()){cerr << "ERROR! Unable to open camera1\n";return -1;}
#ifdef PROXY_PROCESS
#ifdef PROXY_PROCESS_DISPLAY_FULLREZ // full rez capture  
  capL.setFrameSampleRate(40,1);
#else
  capL.setFrameSampleRate(0,1);
#endif
#else
  capL.setFrameSampleRate(1,0);
#endif
  capL.getPUCLibWrapper()->pause();cerr << ".";
  nFramerate2 = FRAMERATE;
  nShutterspeed2 = SHUTTERSPEED;
  PUCRESULT result10 = capL.getPUCLibWrapper()->setFramerateShutter(nFramerate2, nShutterspeed2);
  PUCRESULT result11 = capL.getPUCLibWrapper()->getFramerateShutter(&nFramerate2, &nShutterspeed2);
  PUCRESULT result12 = capL.getPUCLibWrapper()->setResolution(CAMERA_WIDTH, CAMERA_HEIGHT);
  capL.getPUCLibWrapper()->resume();
  cerr << " done\n";
  cerr << "setFramerate: " << result00 << ", getFramerate: " << result01 << ", setRes: " << result02 << endl;
#endif // non SINGLE_CAMERA_TEST
  cerr << "rate2:" << nFramerate2 << ", shutter2:" << nShutterspeed2 << endl;

  // 
  // Stereo BM is initialized before the thread start 
  // 
  bm = StereoBM::create(stereoDpt, stereoSblk); // 32, 17

  // 
  // Launching capture/processing thread
  // 
  terminateThread = false;
  thread thCapture(captureProcessThread);

  // wait for the thread starting
  for(int n=0;n<1000;n++){    
    if(!frameR.empty())break;
    cerr << "waiting thread R start (" << n << ")..." << endl;
  }
  cerr << "STARTED THREAD R " << frameR.size() << endl;

  for(int n=0;n<1000;n++){    
    if(!frameL.empty())break;
    cerr << "waiting thread L start (" << n << ")..." << endl;
  }
  cerr << "STARTED THREAD L "  << frameL.size() << endl;

  // ------- DISPLAY LOOP (30fps) -------
  int silentTerminalCounter = 0;
  int previousProcessCounter = 0;
  int previousCaptureCounter = 0;
  SYSTEMTIME previousSt;

  double procFPS = 0.0;
  
  while(1){
    // For the concatinated display
    vector <Mat> bw3ch(2);

    Mat _frameR;
    Mat _frameL;

#ifdef PROXY_PROCESS
#ifdef PROXY_PROCESS_DISPLAY_FULLREZ // full rez capture
    // {
    // lock_guard<mutex> lock(mtx);
      bool retR = capR.read(_frameR);
      if(_frameR.empty())continue;
      cvtColor(_frameR, bw3ch[0], COLOR_GRAY2RGB);
#ifndef SINGLE_CAMERA_TEST
      bool retL = capL.read(_frameL);
#else
      _frameR.copyTo(_frameL);
#endif
      if(_frameL.empty())continue;
      cvtColor(_frameL, bw3ch[1], COLOR_GRAY2RGB);
     // }
#else // PROXY_PROCESS_DISPLAY_FULLREZ --- uprez from proxy
    if(frameR.empty())continue;
    resize(frameR, _frameR, Size(0, 0), 1.0 / IMAGE_SCALE, 1.0 / IMAGE_SCALE, INTER_NEAREST);
    cvtColor(_frameR, bw3ch[0], COLOR_GRAY2RGB);
    
    if(frameL.empty())continue;
    resize(frameL, _frameL, Size(0, 0), 1.0 / IMAGE_SCALE, 1.0 / IMAGE_SCALE, INTER_NEAREST);
    cvtColor(_frameL, bw3ch[1], COLOR_GRAY2RGB);
#endif
#else // PROXY_PROCESS
    if(frameR.empty())continue;
    cvtColor(frameR, bw3ch[0], COLOR_GRAY2RGB);
    if(frameL.empty())continue;    
    cvtColor(frameL, bw3ch[1], COLOR_GRAY2RGB);
#endif // PROXY_PROCESS
    
    if (!bw3ch[0].empty() && !bw3ch[1].empty()) {
      // 
      // Display Window 0 : Horizontal concatinated
      // 
      Mat concatinated;
      Mat concatinatedView;
      hconcat(bw3ch, concatinated);
      // View size alignment
      if(concatinated.cols > VIEW_SIZE * 2){ // If it is bigger than 2x view size, shrink
	double fxfy2 = (double)(VIEW_SIZE * 2) / (double)concatinated.cols;
	if(!concatinated.empty()){
	  resize(concatinated, concatinatedView, Size(0,0), fxfy2, fxfy2, INTER_LINEAR);
	}
      }else{ // else as is
	concatinated.copyTo(concatinatedView);
      }

      // 
      // Display Window 1 : Double exposure (combined)
      // 
      Mat combined(bw3ch[0].size(), CV_8UC1);
      Mat combinedView;
      for(int j=0;j<combined.rows;j++){
	for(int i=0;i<combined.cols;i++){
	  combined.at<uchar>(j,i) = bw3ch[0].at<Vec3b>(j,i)[0] / 2 + bw3ch[1].at<Vec3b>(j,i)[0] / 2;
	}
      }
      // View size alignment
      if(combined.cols > VIEW_SIZE){
	double fxfy = (double)(VIEW_SIZE) / (double)combined.cols;
	if(!combined.empty()){
	  resize(combined, combinedView, Size(0,0), fxfy, fxfy, INTER_LINEAR);	
	}
      }else{
	combined.copyTo(combinedView);
      }
      if(combinedView.empty())continue;
      // 
      // Display Window 2 : Depth Pseudo Color Map
      // 
      Mat disparityMap;
      Mat disparityPseudo(disparity.size(), CV_8UC3);
      Mat disparityPseudoView;
      disparity.convertTo(disparityMap, CV_8UC1, 0.5, 16);
      genPseudoMap(disparityPseudo, disparityMap);
      Point p0, p1;
      p0.x = ballRect.x;
      p0.y = ballRect.y;
      p1.x = ballRect.x + ballRect.width;
      p1.y = ballRect.y + ballRect.height;
      rectangle(disparityPseudo, p0, p1, Scalar(255,0,255), 1);
      // View size alignment
      if(!disparityPseudo.empty()){
	resize(disparityPseudo, disparityPseudoView, combinedView.size(), 0, 0, INTER_LINEAR);
      }

      if(concatinatedView.empty())continue;
	imshow("Photron OpenCV Live Display 0", concatinatedView);
      if(combinedView.empty())continue;
	imshow("Photron OpenCV Live Display 1", combinedView);
      if(disparityPseudoView.empty())continue;
	imshow("Photron OpenCV Live Display 2", disparityPseudoView);
    }
    
    //
    // Info output
    // 
#ifdef  PROXY_PROCESS
    int captureCounter = capR.getPUCLibWrapper()->getProxySequenceNumber();
#else
    int captureCounter = capR.getPUCLibWrapper()->getFullSequenceNumber();
#endif
    
    if(silentTerminalCounter > 30){
      // Info Output
      SYSTEMTIME st;
      GetSystemTime(&st);
      int millisecondElapse = getElapsedMillisec(st, previousSt);
      previousSt = st;

      int processCounterElapse;
      if(processCounter >= previousProcessCounter){
	processCounterElapse = processCounter - previousProcessCounter;
      }else{
	processCounterElapse = processCounter + (65536- previousProcessCounter);
      }
      
      int captureCounterElapse;
      if(captureCounter >= previousCaptureCounter){
	captureCounterElapse = captureCounter - previousCaptureCounter;
      }else{
	captureCounterElapse = captureCounter + (65536 - previousCaptureCounter);
      }
      printf("Proc'd: %d, Captured: %d, Elapse %d msec\n", 
	     processCounterElapse, captureCounterElapse, millisecondElapse);
      
      previousProcessCounter = processCounter;
      previousCaptureCounter = captureCounter;
      silentTerminalCounter = 0;
      if(captureCounterElapse < FRAMERATE/10){
	procFPS = (double)processCounterElapse / millisecondElapse * 1000.0;
      }else{
	procFPS = (double)(min(captureCounterElapse,processCounterElapse)) / millisecondElapse * 1000.0;
      }
    }
    silentTerminalCounter++;
    
    // 
    // Chart Display Window
    // 
    if(chartCanvas.empty()) {
      chartCanvas.create(Size(DEPTH_CHART_WIDTH, 450), CV_8UC3);
    }

    chartCanvas.setTo(Scalar(255, 255, 255));

    int offset = captureCounter % FRAMERATE;
    Point p0, p1, plotPt;
    p0.y = 0;
    p1.y = chartCanvas.rows-5;
    // for (int i = 0; i < depthVector.size(); i++) {
    for (int i = depthVector.size() - 1; i >= 0 ; i--) {
      // Depth wave
      // plotPt.x = ((int)depthVector.size() - i + 1);
      plotPt.x = i;
      if(plotPt.x >= chartCanvas.cols - 2)continue;
      plotPt.y = chartCanvas.rows - (depthVector[i] * chartCanvas.rows / 512);
      circle(chartCanvas, plotPt, 1, Scalar(0, 200, 0));
      // 1/30 Second line
      if(i < offset){
	if((offset - i) % (FRAMERATE/30) == 0){
	  p0.x = p1.x = i;
	  line(chartCanvas, p0, p1, cv::Scalar(192,192,192), 1);
	  // Red plot for 1/30 sec
	  circle(chartCanvas, plotPt, 5, Scalar(0, 0, 200));
	}
      }else{
	if((i - offset) % (FRAMERATE/30) == 0){
	  p0.x = p1.x = i;
	  line(chartCanvas, p0, p1, cv::Scalar(192,192,192), 1);
	  // Red plot for 1/30 sec
	  circle(chartCanvas, plotPt, 5, Scalar(0, 0, 200));
	}
      }
      // 1 Second line
      for(int k = offset;k < depthVector.size();k += FRAMERATE){
	if(i == k){
	  p0.x = p1.x = i;
	  line(chartCanvas, p0, p1, cv::Scalar(0,0,0), 4);
	}
      }
    }
    char str[64];
    sprintf(str, "%.2lf FPS", procFPS);
    putText(chartCanvas, str, cv::Point(20, 30), FONT_HERSHEY_PLAIN, 1.5, Scalar(0,0,0), 2, 8);
    if (!chartCanvas.empty() && !frozenChart) {
      imshow("Circle Trail", chartCanvas);
    }
    int c = waitKey(16);
    if (c > 0) {
      if (c == 'z') {
	if(frozenChart)frozenChart = false;
	else frozenChart = true;
      }else if(c == 'm'){
      }else if (c == 0x1b) { // Hit esc to exit
	terminateThread = true;
	thCapture.join();
	break;
      }
#if 1
      if (c == '>') {
	threshLev += 2.0;
	if (threshLev > 255)threshLev = 255;
      }
      else if (c == '<') {
	threshLev -= 2.0;
	if (threshLev < 0)threshLev = 0;
      }
      else if (c == 'k') {
	stereoDpt *= 2;
	bm->setNumDisparities(stereoDpt);
	cerr << "dpt=" << stereoDpt << endl;
      }
      else if (c == 'K') {
	stereoDpt /= 2;
	if (stereoDpt == 1)stereoDpt = 16;
	bm->setNumDisparities(stereoDpt);
	cerr << "dpt=" << stereoDpt << endl;
      }
      else if (c == 'l') {
	stereoSblk += 2;
	bm->setSmallerBlockSize(stereoSblk);
	cerr << "sblk=" << stereoSblk << endl;
      }
      else if (c == 'L') {
	stereoSblk -= 2;
	bm->setSmallerBlockSize(stereoSblk);
	cerr << "sblk=" << stereoSblk << endl;
      }
#endif
    }
  }
}
