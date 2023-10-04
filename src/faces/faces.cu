#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include "..\..\include\PhotronVideoCapture.h"

#include "cuda_runtime.h"
#include "opencv2/core.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/cudaobjdetect.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudawarping.hpp"
#include "opencv2/cudaarithm.hpp"

#define DETECTION_SCALE 1.0
#define MINOBJSIZE 60
// #define FPS30

using namespace std;
using namespace cv;
using namespace cv::cuda;

bool doCameraInit;
bool cpuMode; // true: cpu, false: gpu
bool canUseGPU;

static int numFramesProcessed = 0;
static int currentCPUSeqNumber = -1;
static int currentGPUSeqNumber = -1;

static unsigned int width = 1246;
static unsigned int height = 1008;
cv::CascadeClassifier cascade_cpu;
static Ptr<cuda::CascadeClassifier> cascade_gpu;

static double scaleFactor = 5.0;
static int minNeighbors = 4;
static int minObjSize = MINOBJSIZE;

static std::vector<cv::Rect> faces;
static std::vector<cv::Rect> facesGUI;
static unsigned char* dst_gpu;
static Mat dst_cpu;
static bool capturing;
static bool recReady;
static bool recTriggered;
mutex mtx;

#define MAX_REC_DURATION 256

static std::vector <GpuMat> fBufGPU;
static std::vector <Mat> fBufCPU;
static std::vector < std::vector < cv::Rect > > facesVec;
static std::vector < SYSTEMTIME > stVec;

photron::PUCLib_Wrapper capGPU;
photron::VideoCapture capCPU;
Mat fullFrame;

const int xMargin = 8;
const int yMargin = 8;
const int xDiv = 6;
const int yDiv = 11;
int yTile;
int xTile;
static int recDuration;
int currentCPUGPU; // cpu = 0, gpu = 1, undefined = -1

static bool loopPlay;

unsigned char* dst;

shared_ptr<std::thread> thCapture = shared_ptr<std::thread>();

static void shutdownCaptureInstance()
{
	if(currentCPUGPU == 0)  // closing cpu mode 
	{
		fprintf(stderr, "Closing CPU mode\n");
		if(thCapture.get() != nullptr) {
			fprintf(stderr, "joining CPU capture thread\n");
			thCapture->join();
		}
	} 
	else if(currentCPUGPU == 1) // closing gpu mode
	{ 
		if(capGPU.isOpened()) 
		{
			fprintf(stderr, "Closing GPU mode ....");
			PUC_EndXferData(capGPU.getPUCHandle());
			capGPU.tearDownGPUDecode();
			capGPU.close();
			fprintf(stderr, "GPU is closed\n");
		}
	}
	currentCPUGPU = -1;
}

static void on_mouse(int event, int x, int y, int flags, void *)
{
	if(event == EVENT_LBUTTONDOWN)
	{
		int jj = (y - yMargin) / yTile;
		int ii = (x - xMargin) / xTile;

		if(jj == 10 && ii >= 4) 
		{	// Exit button
			if(!loopPlay) 
			{ 
				// Not memory playback
				capturing = false;
				recReady = false;
				recTriggered = false;
				shutdownCaptureInstance();    
			} 
			else 
			{	// memory playback
				loopPlay = false;
			}
			free(dst);
			exit(0);
		}

		if(jj == 0)
		{
			if(ii < 3)
			{
				if(!cpuMode)
				{
					cpuMode = true;
					loopPlay = false;
					doCameraInit = true;
				}
			}
			else
			{
				if(canUseGPU){
					if(cpuMode){
						cpuMode = false;
						loopPlay = false;
						doCameraInit = true;
					}
				}
			}
		}
		else if(jj==2)
		{
			if(ii <= 1) {
				scaleFactor -= 0.1;
				if(scaleFactor < 1.2) {
					scaleFactor = 1.2;
				}
			}
			else if(ii >= 4)
			{
				scaleFactor += 0.1;
				if(scaleFactor > 10.0) {
					scaleFactor = 10.0;
				}
			}
		}
		else if(jj==4)
		{
			if(ii <= 1) {
				minNeighbors -= 1;
				if(minNeighbors < 0) {
					minNeighbors = 0;
				}
			}
			else if(ii >= 4) {
				minNeighbors += 1;
				if(minNeighbors > 100){
					minNeighbors = 100;	  
				}
			}
		}
		else if(jj==6)
		{
			if(ii <= 1) {
				minObjSize -= 5;
				if(minObjSize < 5.0) {
					minObjSize = 5;
				}
			}
			else if(ii >= 4) {
				minObjSize += 5;
				if(minObjSize > 300.0) {
				  minObjSize = 300;
				}
			}
		}
		else if(jj==8) 
		{
			if(ii <= 1) {
				recDuration -= 16;
				if(recDuration < 16){
					recDuration = 16;
				}
			}
			else if(ii >= 4) {
				recDuration += 16;
				if(recDuration > MAX_REC_DURATION){
					recDuration = MAX_REC_DURATION;
				}
			}
		}
		else if(jj == 10 && ii == 3) {
			Mat helpPNG;
			std::string fname = samples::findFile("helpPNG.PNG");
			helpPNG = imread(fname);
			imshow("Help", helpPNG);
		}
		else if(jj == 10 && ii == 1) { // Save Param
			FILE *fp;
			fp = fopen("facesParam.txt", "w");
			if(fp != NULL) {
				fprintf(fp, "%lf\n%d\n%d\n%d\n", 
				scaleFactor,
				minNeighbors,
				minObjSize,
				recDuration);
				fclose(fp);
			}
			else {
				fprintf(stderr, "File output error!\n");
			}
		}
		else if(jj == 10 && ii == 0) { // Rec/Cam/Stop
			if(capturing) {
				if(recReady) {
					if(recTriggered) {
						// Rec loop
						fprintf(stderr, "Stop Rec and change mode to Playback\n");	    
						capturing = false;
						recReady = false;
						recTriggered = false;
					}
					else {
					// RecReady loop
					fprintf(stderr, "Change mode to Live\n");
					recReady = false;
					}
				}
				else {
					// Live loop
					fprintf(stderr, "Change mode to recReady\n");
					recReady = true;
				}
			}
			else {
				// Play back
				loopPlay = false;
			}
		}
	}
}

static void monoResize(const GpuMat& src, GpuMat& resized, double scale)
{
    Size sz(cvRound(src.cols * scale), cvRound(src.rows * scale));
    if (scale != 1) {
      // cv::cuda::resize(src, resized, sz);
    }
	else {
      resized = src;
    }
}

static int getElapsedMillisec(SYSTEMTIME &st, SYSTEMTIME &previousSt)
{
	int c_msec = st.wSecond * 1000 + st.wMilliseconds;
	int p_msec = previousSt.wSecond * 1000 + previousSt.wMilliseconds;
	if(c_msec > p_msec) {
		return c_msec - p_msec;
	}
	return 60000 - p_msec + c_msec;
}

// Callback function for CPU
static void capThreadCPU()
{
	while (1)
	{
		if (!capturing)
			return;

		capCPU.read(dst_cpu);
		if (dst_cpu.empty()) {
			cerr << "ERROR! Unable to capture\n";
			return;
		}

		SYSTEMTIME st;
		GetSystemTime(&st);

		Mat resized_cpu;
		dst_cpu.copyTo(resized_cpu);

		USHORT tmpSeqNum = capCPU.getPUCLibWrapper()->getFullSequenceNumber();
		if(tmpSeqNum == currentCPUSeqNumber){
			continue;
		}

		// cascade_cpu.detectMultiScale(resized_cpu, faces, 5.0, 0, CASCADE_SCALE_IMAGE, cv::Size(MINOBJSIZE,MINOBJSIZE));
		cascade_cpu.detectMultiScale(resized_cpu, faces, scaleFactor, minNeighbors, CASCADE_SCALE_IMAGE, cv::Size(minObjSize, minObjSize));
    
		{
			lock_guard<mutex> lock(mtx);
			dst_cpu.copyTo(fullFrame); // todo
			facesGUI = faces;
		}

		if(recReady) 
		{
			if(faces.size() > 0 || recTriggered) 
			{
				recTriggered = true;
				if(fBufCPU.size() < recDuration) {
					Mat tmp;
					resized_cpu.copyTo(tmp);
					fBufCPU.push_back(tmp);
					facesVec.push_back(faces);
					SYSTEMTIME st;
					GetSystemTime(&st);
					stVec.push_back(st);
				}
				else {
					capturing = false;
					recReady = false;
					recTriggered = false;
				}
			}
		}

		numFramesProcessed++;
		currentCPUSeqNumber = tmpSeqNum;
	}
}

// Callback function for GPU
static void capThreadGPU(PPUC_XFER_DATA_INFO info, void* userData)
{	
	if(!capturing)
		return;

	photron::PUCLib_Wrapper *pCap = (photron::PUCLib_Wrapper *) userData;
	auto result = pCap->decodeGPU(false, info->pData, &dst_gpu, width);
	GpuMat frame_gpu(height, width, CV_8UC1, dst_gpu);
	GpuMat resized_gpu;
	monoResize(frame_gpu, resized_gpu, DETECTION_SCALE);

	USHORT tmpSeqNum = info->nSequenceNo;
	if(tmpSeqNum == currentGPUSeqNumber) {
		return;
	}

	cascade_gpu->setFindLargestObject(false);
	cascade_gpu->setScaleFactor(scaleFactor);
	cascade_gpu->setMinNeighbors(minNeighbors); // 4 -> 8 -> 2 -> 4
	cascade_gpu->setMinObjectSize(cv::Size(minObjSize,minObjSize));

	GpuMat faceBuf_gpu;
	cascade_gpu->detectMultiScale(resized_gpu, faceBuf_gpu);
	cascade_gpu->convert(faceBuf_gpu, faces);

	{
		lock_guard<mutex> lock(mtx);
		facesGUI = faces;
	}

	if(recReady)
	{
		if(faces.size() > 0 || recTriggered) {
			recTriggered = true;
			if(fBufGPU.size() < recDuration){
				GpuMat tmp;
				resized_gpu.copyTo(tmp);
				fBufGPU.push_back(tmp);
				facesVec.push_back(faces);
				SYSTEMTIME st;
				GetSystemTime(&st);
				stVec.push_back(st);
			}
			else {
				capturing = false;
				recReady = false;
				recTriggered = false;
			}
		}
	}
  
	numFramesProcessed++;
	currentGPUSeqNumber = tmpSeqNum;
}

static void pendingMenu(cv::Mat &menuBuf, const char *str)
{
	menuBuf.setTo(cv::Scalar(160,160,160));
  
	putText(menuBuf, str, 
		cv::Point(menuBuf.cols / 2 - 50, menuBuf.rows / 2 - 10), 
		FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	cv::imshow("Faces Menu", menuBuf);
	cv::waitKey(1);
}

static void refreshMenu(cv::Mat &menuBuf, bool cpuMode)
{
	menuBuf.setTo(cv::Scalar(100,100,100));

	cv::Point pt0, pt1;
  
	yTile = (menuBuf.rows - yMargin * 2) / yDiv;
	xTile = (menuBuf.cols - xMargin * 2) / xDiv;

	pt0.y = yMargin + 0 * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + 0 * xTile;
	pt1.x = pt0.x + xTile * (xDiv / 2) - 2;

	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, cpuMode ? "[v] CPU" : "[ ] CPU", 
		cv::Point(pt0.x + 25, pt0.y + 20),
		FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);
  
	pt0.y = yMargin + 0 * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + (xDiv / 2) * xTile;
	pt1.x = pt0.x + xTile * (xDiv / 2) - 2;

	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, cpuMode ? "[ ] GPU" : "[v] GPU", 
		cv::Point(pt0.x + 25, pt0.y + 20), 
		FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	int ySection; // 0,1,2,3,4,5
	int xSection; // 0,1,2,3,4,5
	int xWidth;	  // 1,2,3,4,5,6
	char numLabel[8];

	sprintf(numLabel, "%.1f", scaleFactor);

	ySection = 1;
	xSection = 0;
	pt0.y = yMargin + ySection * yTile;
	pt0.x = xMargin + xSection * xTile;
	putText(menuBuf, "Scale Factor" , cv::Point(pt0.x + 5, pt0.y + 30), FONT_HERSHEY_PLAIN, 1.0, Scalar(255,255,255), 1, 8);

	ySection = 3;
	xSection = 0;
	pt0.y = yMargin + ySection * yTile;
	pt0.x = xMargin + xSection * xTile;
	putText(menuBuf, "Min. Neighbor" , cv::Point(pt0.x + 5, pt0.y + 30), FONT_HERSHEY_PLAIN, 1.0, Scalar(255,255,255), 1, 8);

	ySection = 5;
	xSection = 0;
	pt0.y = yMargin + ySection * yTile;
	pt0.x = xMargin + xSection * xTile;
	putText(menuBuf, "Min. Size" , cv::Point(pt0.x + 5, pt0.y + 30), FONT_HERSHEY_PLAIN, 1.0, Scalar(255,255,255), 1, 8);  

	ySection = 7;
	xSection = 0;
	pt0.y = yMargin + ySection * yTile;
	pt0.x = xMargin + xSection * xTile;
	putText(menuBuf, "Rec. Duration" , cv::Point(pt0.x + 5, pt0.y + 30), FONT_HERSHEY_PLAIN, 1.0, Scalar(255,255,255), 1, 8);  

	ySection = 2;
	xSection = 0;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "<" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 2;
	xSection = 2;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(200,200,200), -1, 4);
	putText(menuBuf, numLabel , cv::Point(pt0.x + 25, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 2;
	xSection = 4;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, ">" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	sprintf(numLabel, "%d", minNeighbors);
	ySection = 4;
	xSection = 0;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "<" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 4;
	xSection = 2;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(200,200,200), -1, 4);
	putText(menuBuf, numLabel , cv::Point(pt0.x + 25, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 4;
	xSection = 4;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, ">" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	sprintf(numLabel, "%d", minObjSize);
	ySection = 6;
	xSection = 0;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "<" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 6;
	xSection = 2;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(200,200,200), -1, 4);
	putText(menuBuf, numLabel , cv::Point(pt0.x + 25, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 6;
	xSection = 4;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, ">" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	sprintf(numLabel, "%d", recDuration);
	ySection = 8;
	xSection = 0;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "<" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);
	ySection = 8;
	xSection = 2;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(200,200,200), -1, 4);
	putText(menuBuf, numLabel , cv::Point(pt0.x + 25, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 8;
	xSection = 4;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, ">" , cv::Point(pt0.x + 30, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	ySection = 10;
	xSection = 3;
	xWidth = 1;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	putText(menuBuf, "(?)" , cv::Point(pt0.x + 10, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 255, 255), 1, 8);

	// Function Button
	ySection = 10;
	xSection = 0;
	xWidth = 1;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;

	if(capturing) {
		if(recReady) {
			if(recTriggered) {
				cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160, 160, 160), -1, 4);
				putText(menuBuf, "STOP" , cv::Point(pt0.x + 2, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 255, 255), 1, 8);
			}
			else {
				cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160, 160, 160), -1, 4);
				putText(menuBuf, "CAM" , cv::Point(pt0.x + 2, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 255, 255), 1, 8);
			}
		}
		else {
			cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(0, 0, 200), -1, 4);
			putText(menuBuf, "REC" , cv::Point(pt0.x + 2, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 255, 255), 1, 8);
		}
	}
	else {
		cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160, 160, 160), -1, 4);
		putText(menuBuf, "CAM" , cv::Point(pt0.x + 2, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(255, 255, 255), 1, 8);
	}

	// Parameter Save
	ySection = 10;
	xSection = 1;
	xWidth = 1;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "Sav" , cv::Point(pt0.x + 2, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);
  
	// Exit Button
	ySection = 10;
	xSection = 4;
	xWidth = 2;
	pt0.y = yMargin + ySection * yTile;
	pt1.y = pt0.y + yTile - 2;
	pt0.x = xMargin + xSection * xTile;
	pt1.x = pt0.x + xTile * xWidth - 2;
	cv::rectangle(menuBuf, pt0, pt1, cv::Scalar(160,160,160), -1, 4);
	putText(menuBuf, "Exit" , cv::Point(pt0.x + 15, pt0.y + 20), FONT_HERSHEY_PLAIN, 1.0, Scalar(0, 0, 0), 1, 8);

	cv::imshow("Faces Menu", menuBuf);
}


int main()
{
	std::cout << "Start...\n";

	// Check canUseGPU
	int cudaDevCount = 0;
	if ((cudaDevCount = getCudaEnabledDeviceCount()) < 1) {
		cerr << "No GPU found or the library is compiled without CUDA support" << endl;
		canUseGPU = false;
	}
	else {
		cerr << cudaDevCount << " CUDA device found." << endl;
		cv::cuda::printShortCudaDeviceInfo(cv::cuda::getDevice());
		fprintf(stderr, "CAN use GPU\n");
		canUseGPU = true;
	}
  
	// 
	// Initializing haar cascade classifier
	// 
	string cascadeName = "haarcascade_frontalface_default.xml";
	
	// Cascade construction for CPU
	if(!cascade_cpu.load(cascadeName)) {
		cerr << "cpu cascade load error." << endl;
		return -1;
	}

	// Cascade construction for GPU
	if(canUseGPU) {
		cascade_gpu = cuda::CascadeClassifier::create(cascadeName);
		std::cout << "Loaded(CPU/GPU) " << cascadeName << std::endl;
	}

	// 
	// Load default param
	// 
	FILE *fp;
	fp = fopen("facesParam.txt", "r");
	if(fp != NULL) {
		fscanf(fp, "%lf", &scaleFactor);
		fscanf(fp, "%d", &minNeighbors);
		fscanf(fp, "%d", &minObjSize);
		fscanf(fp, "%d", &recDuration);		
		fclose(fp);
		fprintf(stderr, "Paramters are Loaded (%lf, %d, %d, %d)\n",
			scaleFactor,
			minNeighbors,
			minObjSize,
			recDuration);
	}
	else {
		fprintf(stderr, "File input error!\n");
	}

	// 
	// Common camera initialization
	// 
	PUCRESULT result;
  
	// Menu OpenCV window
	cv::namedWindow("Faces Menu", cv::WINDOW_AUTOSIZE | cv::WINDOW_FREERATIO);
	setMouseCallback("Faces Menu", on_mouse);
	cv::Mat menuBuf;
	menuBuf.create(402, 256, CV_8UC3);
	menuBuf.setTo(cv::Scalar(100,100,100));
  
	std::vector <cv::Rect> menuRect;
	cv::Rect tmpRect;
  
	refreshMenu(menuBuf, cpuMode);

	dst = (unsigned char* )malloc(width * height);

	cpuMode = false;
	recDuration = 64;
	currentCPUGPU = -1; // cpu = 0, gpu = 1, undefined = -1
	float procFPS;
  
	while(1)
	{
		doCameraInit = true;
		recReady = false;
		recTriggered = false;
    
		SYSTEMTIME previousSt;
		int previousNumFramesProcessed = 0;
		int FPS_for_calc_FPS = 0;

		while(1)
		{
			// 
			// Restarting Camera 
			// 
			if(doCameraInit) {
				fprintf(stderr, "Chaging CPU/GPU mode\n");
				// Shutting down the previous mode
				capturing = false;
				pendingMenu(menuBuf, "Initializing...");
				shutdownCaptureInstance();    
				capturing = true;

				// Initializing new mode
				if(cpuMode || !canUseGPU) { // CPUmode
					fprintf(stderr, "Initializing CPU mode\n");
					int deviceID = 0;             // 0 = open default camera
					int apiID = cv::CAP_ANY;      // 0 = autodetect default API
					if(!capCPU.isOpened()) {
						capCPU.open(deviceID, apiID);
						if (!capCPU.isOpened()) {
							cerr << "ERROR! Unable to open camera\n";
							menuBuf.setTo(cv::Scalar(0,0,200));
							putText(menuBuf, "Camera Open Error" , cv::Point(10, 40), FONT_HERSHEY_PLAIN, 1.3, Scalar(255,255,255), 1, 8);
							putText(menuBuf, "Hit ESC to exit" , cv::Point(10, 80), FONT_HERSHEY_PLAIN, 1.3, Scalar(255,255,255), 1, 8);
							cv::imshow("Faces Menu", menuBuf);
							cv::waitKey(0);
							return -1;
						}
					}

					capCPU.getPUCLibWrapper()->pause();
#ifdef FPS30	  
					capCPU.getPUCLibWrapper()->setFramerateShutter((UINT32)30, (UINT32)30); // 1000, 2000
#else
					capCPU.getPUCLibWrapper()->setFramerateShutter((UINT32)1000, (UINT32)2000); // 1000, 2000
#endif
					capCPU.getPUCLibWrapper()->resume();
	  
					thCapture = std::make_shared<std::thread>(&capThreadCPU);
					fprintf(stderr, "Starting CPU mode\n");
					cpuMode = true;
					currentCPUGPU = 0;
				}
				else { // GPUmode
					fprintf(stderr, "Initializing GPU mode.");
					// Saving one image only needs single thread mode API enabled
					capGPU.setMultiThread(false);
					// Set Capture Settings
					capGPU.setResolution(width, height);
#ifdef FPS30	  
					capGPU.setFramerateShutter(30, 30);
#else
					capGPU.setFramerateShutter(1000, 2000);
#endif	  
					fprintf(stderr, "..Opening cap instance.");

					// Open Camera
					result = capGPU.open(0);
					if (result != PUC_SUCCEEDED || !capGPU.isOpened()) {
						std::cerr << capGPU.getLastErrorName();
						currentCPUGPU = -1;
						canUseGPU = false;
						fprintf(stderr, "can NOT use GPU (capOpen Error). Trying CPU mode\n");
						cpuMode = true;
						continue;
					}
					fprintf(stderr, ".. Done\n");
					fprintf(stderr, "..Initialize GPU cap.\n");
	  
					// Initialize GPU capture
					result = capGPU.setupGPUDecode(PUC_GPU_SETUP_PARAM{ width, height });
					if (result != PUC_SUCCEEDED) {
						std::cerr << capGPU.getLastErrorName();
						canUseGPU = false;
						fprintf(stderr, "can NOT use GPU (PUC_SetupGPU Error). Trying CPU mode\n");
						currentCPUGPU = -1;
						cpuMode = true;
						continue;
					}
	  
					fprintf(stderr, "..Begin Xfer\n");
					result = PUC_BeginXferData(capGPU.getPUCHandle(), capThreadGPU, (void *) &capGPU);
					if (result != PUC_SUCCEEDED) {
						fprintf(stderr, "PUC_BeginXferData failed! %d. Trying CPU mode", result);
						currentCPUGPU = -1;
						cpuMode = true;
						continue;
					}
					fprintf(stderr, "Device Sync\n");
					cudaDeviceSynchronize();
					fprintf(stderr, "Starting GPU mode.\n");
					currentCPUGPU = 1;
				}
				doCameraInit = false;
			}
			// 
			// Camera Restarted
			// 

			// 
			// Acquire the latest frame
			// 
			if(!cpuMode) {
				auto cudaStatus = cudaMemcpy(dst, dst_gpu, (size_t)width * height, cudaMemcpyDeviceToHost);
				if (cudaStatus != cudaSuccess) {
					Sleep(100);
					continue;
				}
				Mat tmp = Mat(height, width, CV_8UC1, dst);
				tmp.copyTo(fullFrame); // TODO
			}

			if(fullFrame.empty())
				continue;
			refreshMenu(menuBuf, cpuMode);

			//
			// FPS measurement
			// 
			char str[64];
			if(FPS_for_calc_FPS % 10 == 0) { // one for every 10 steps
				SYSTEMTIME st;
				GetSystemTime(&st);
				int millisecondElapse = getElapsedMillisec(st, previousSt);
				procFPS = (numFramesProcessed - previousNumFramesProcessed) / (float)millisecondElapse * 1000;
				previousSt = st;
				previousNumFramesProcessed = numFramesProcessed;

				if(numFramesProcessed > 360000000) { // 10 hrs 1000fps
					numFramesProcessed = 0;
					previousNumFramesProcessed = 0;
				}
			}
			if(recReady) {
				if(recTriggered) {
					sprintf(str, "Recording %.2f FPS", procFPS);
					rectangle(fullFrame, cv::Point(0,0), cv::Point(fullFrame.cols-1, 40), cv::Scalar(255), -1);
					putText(fullFrame, str, cv::Point(20, 30), FONT_HERSHEY_PLAIN, 1.5, Scalar(0), 2, 8);
				}
				else {
					rectangle(fullFrame, cv::Point(0,0), cv::Point(fullFrame.cols-1, 80), cv::Scalar(160), -1);
					sprintf(str, "RecReady %.2f FPS", procFPS);
					putText(fullFrame, str, cv::Point(20, 30), FONT_HERSHEY_PLAIN, 1.5, Scalar(255), 2, 8);
				}
			}
			else {
				sprintf(str, "Camera Live %.2f FPS", procFPS);
				putText(fullFrame, str, cv::Point(20, 30), FONT_HERSHEY_PLAIN, 1.5, Scalar(255), 2, 8);
			}

			FPS_for_calc_FPS++;

			// 
			// Drawing Facial Rectangle
			// 
			for(int ii = 0; ii < facesGUI.size(); ii++) {
				cv::Point pt0, pt1;
				pt0.x = facesGUI[ii].x;
				pt0.y = facesGUI[ii].y;
				pt1.x = facesGUI[ii].x+facesGUI[ii].width;
				pt1.y = facesGUI[ii].y+facesGUI[ii].height;
				cv::rectangle(fullFrame, pt0, pt1, cv::Scalar(255), 2);
			}
      
			//
			// Bar chart for num of faces
			// 
			const int sideMargin = 5;
			const int iconRadius = 15;
			const int gap = 2;
			const float xEyePos = 0.4f; // From center . prop Radius
			const float yEyePos = -0.2f; // From center . prop Radius
			const float eyeHeight = 0.4f; // prop Radius
			const float mouthPos = 0.5f;
			const float mouthUp = 0.1f;
			const float mouthWidth = 0.3f;

			for(int ii = 0; ii < facesGUI.size() && ii < 20; ii++) {
				int cx = sideMargin + iconRadius;
				int cy = fullFrame.rows - (sideMargin + iconRadius + (iconRadius * 2 + gap) * ii) - 1;
				cv::circle(fullFrame, cv::Point(cx, cy), iconRadius, cv::Scalar(255), 2);
				int x0 = (int)(iconRadius * xEyePos);
				int yc = (int)(iconRadius * yEyePos + cy);
				int y0 = (int)(iconRadius * eyeHeight / 2);
				cv::line(fullFrame, cv::Point(cx-x0, yc-y0), cv::Point(cx-x0, yc+y0), cv::Scalar(255), 2);
				cv::line(fullFrame, cv::Point(cx+x0, yc-y0), cv::Point(cx+x0, yc+y0), cv::Scalar(255), 2);
				int ycc = (int)(iconRadius * mouthPos + cy);
				int y1 = (int)(iconRadius * mouthUp);
				int x1 = (int)(iconRadius * mouthWidth);
				cv::line(fullFrame, cv::Point(cx-x1, ycc-y1), cv::Point(cx, ycc), cv::Scalar(255), 2);
				cv::line(fullFrame, cv::Point(cx+x1, ycc-y1), cv::Point(cx, ycc), cv::Scalar(255), 2);
			}

			// 
			// Monitor Display
			// 
			imshow("Infinicam: Faces", fullFrame);

			// 
			// Key operation
			// 
			int key = waitKey(33);
			if (key == 27 || !capturing) {
				pendingMenu(menuBuf, "Closing Camera");

				if(!loopPlay) { // Not memory playback
					capturing = false;
					recReady = false;
					recTriggered = false;
					shutdownCaptureInstance();    
				}
				else { // memory playback
					loopPlay = false;
				}

				if(key == 27) {
					free(dst);
					exit(0);
				}
				break;
			}
				
			if(key=='s') { // start recReady
				recReady = true;
			}
		}
    
		// Shutting down the previous mode
		shutdownCaptureInstance();    

		std::cerr << "Starting video playback." << std::endl;
    
		loopPlay = true;
		bool stepFwd = false;
    
		refreshMenu(menuBuf, cpuMode);
    
		// Demo Video Loop 
		int recordCount = cpuMode ? (int)(fBufCPU.size()) : (int)(fBufGPU.size());
		if(recordCount >= 1) {
			do {
				for(int i = 0; i < recordCount; i++) {
					cv::Mat fBufMat;
					if(cpuMode) {
						fBufCPU[i].copyTo(fBufMat);
					}
					else {
						fBufGPU[i].download(fBufMat);
					}

					int sumWidHgt = 0;
					for(int ii = 0; ii < facesVec[i].size(); ii++) {
						cv::Point pt0, pt1;
						pt0.x = facesVec[i][ii].x;
						pt0.y = facesVec[i][ii].y;
						pt1.x = facesVec[i][ii].x+facesVec[i][ii].width;
						pt1.y = facesVec[i][ii].y+facesVec[i][ii].height;
						cv::rectangle(fBufMat, pt0, pt1, cv::Scalar(255), 2);
						sumWidHgt += facesVec[i][ii].width;
						sumWidHgt += facesVec[i][ii].height;
					}

					int elMsec = 0;
					if(i != 0) {
						elMsec = getElapsedMillisec(stVec[i], stVec[0]);
					}

					cv::Point pa, pb;
					pa.x = 0;
					pa.y = 0;
					pb.x = fBufMat.cols * i / recDuration - 1;
					pb.y = 40;
					rectangle(fBufMat, pa, pb, cv::Scalar(160,160,160), -1, 4);

					char str[128];
					if(facesVec[i].size() == 0) {
						sprintf(str, "No face found. Average Size = --- @ %d [msec] %d / %d", elMsec, i + 1, recordCount);
					}
					else if(facesVec[i].size() == 1) {
						sprintf(str, "%d face found. Average Size = %d @ %d [msec] %d / %d", (int)facesVec[i].size(), sumWidHgt/2, elMsec, i + 1, recordCount);
					}
					else {
						auto aveSize = sumWidHgt / facesVec[i].size() / 2;
						sprintf(str, "%d faces found. Average Size = %d @ %d [msec] %d / %d", (int)facesVec[i].size(), (int)aveSize, elMsec, i + 1, recordCount);
					}
					putText(fBufMat, str, cv::Point(20, 30), FONT_HERSHEY_PLAIN, 1.5, Scalar(255), 2, 8);
					imshow("Infinicam: Faces", fBufMat);
					
					int key = waitKey(stepFwd ? 0 : 33);
					if (key == 27) {
						loopPlay = false;
					}
					if (key == ' ') {
						stepFwd = !stepFwd;
					}
					if (!loopPlay) {
						break;
					}
					refreshMenu(menuBuf, cpuMode);
				}
			} while(loopPlay);

			fprintf(stderr, "Exit from loopPlay\n");

			// Cleanup loop play buffer
			facesVec.clear();
			if(cpuMode)
				fBufCPU.clear();
			else 
				fBufGPU.clear();
		}
	}
}

#if 0

 Faces Help
 
 ---------------------------------------------------------------------------------------------
 * [ ]CPU / [ ]GPU
 CPUプロセスとGPUプロセスを切り替えます。映像デコードと顔検出処理の両方が切り替わります。
 ---------------------------------------------------------------------------------------------
 * Scale Factor
 顔検出の際、基準の大きさにこの因子を繰り返し掛け合わせたサイズで探索を行います。通常1.1などの因子を用いて
 徐々にサイズを変えて探索するほうが検出精度が上がりますが、処理速度が極端に遅くなります。逆に5などの大きい
 値を用いると探索繰り返し回数が少なくなります。検知対象の大きさがある程度限定できる場合には後述の
 Min. Sizeパラメータを調節したうえでScale Factorは大きい値にすると処理が高速化します。
 ---------------------------------------------------------------------------------------------
 * Min. Neighbor
 顔のある場所で、探索窓の位置が少しだけずれても、窓の枠内に目や鼻などの要素が入っていればも顔と認識します。
 すなわち一つの顔の近辺で、少しずつずれた位置に多数検出を行います。Min. Neighborは近傍にいくつの顔が検出
 されれば本当に顔であるかどうかを決めるパラメータです。このパラメータをゼロにすると、顔ではないものを顔と
 認識する誤検出が発生しやすくなります。また、一つの顔の周りに少しずつずれた窓を多数検出します。一方、この
 パラメータを大きくすると検出もれが発生しやすくなります。
 ---------------------------------------------------------------------------------------------
 * Min. Size
 検出する顔の窓の大きさの最小サイズをピクセル単位で設定します。このサイズを小さくすると画面内で小さいサイ
 ズの顔も検出できるようになりますが、処理速度が低下します。検知対象の大きさがある程度限定できる場合には大き
 いサイズにすると高速化します。
 ---------------------------------------------------------------------------------------------
 * Rec. Duration
 RECボタンをクリックするとRecReady状態になります。顔が一つでも検出されるとそれがトリガーとなって録画を
 開始します。Rec. Durationは録画の長さ(フレーム数)です。
 ---------------------------------------------------------------------------------------------
 * REC/CAM/STOP
 Menuウインドウ左下のボタンは、モードによってREC/CAM/STOPの三種類のボタンに切り替わります。
 
 REC: Liveモード(ソフト起動後のモード)でこのボタンを押すとRecReadyモードになります。顔が一つでも検知さ
 れるとトリガーとなって録画を開始します。
 
 CAM: RecReadyモードまたはPlayBackモードでこのボタンを押すとLiveモードに戻ります。ただしトリガーがかかっ
 て録画が始まっている状態ではSTOPボタンになっています。
 
 STOP: 録画モードでこのボタンを押すとPlayBackモードになります。なお、トリガーがかかって録画が始まると顔が
 検出できなくても録画を継続し、Rec. Durationで決められたフレーム数分の録画が終わると自動的にPlayBackモー
 ドになります。
 
 (PlayBackモードでスペースキーを押すとコマ送り再生となり矢印キーなどで一コマずつ動画を進めることができ
 ます。逆再生はできません。もう一度スペースキーを押すと通常再生に戻ります。)
 ---------------------------------------------------------------------------------------------
 * Sav
 設定パラメータを保存します。次回のアプリケーション起動時にこの保存パラメータを自動読み込みします。
 ---------------------------------------------------------------------------------------------
 * Exit
 アプリケーションを終了します。


#endif
