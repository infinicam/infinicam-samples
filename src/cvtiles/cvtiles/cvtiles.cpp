// cvdemo.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <opencv2/core.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>

using namespace cv;
using namespace std;

#include "PhotronVideoCapture.h"



Mat3b canvas;
Mat3b background;
string winName = "Infinicam tiles";
Rect previewWindowRect;
Rect imageRect;
photron::VideoCapture cap;
int numSecondsPreview = 3;
Rect previewRect;
Mat previewCircularImage;
Mat previewImage;
int previewLineIndex = 0;
Rect thresholdBar;
int thresholdVal = 128;
Rect histRect = Rect(32, 316, 550, 224);
Rect timeBar;
float timeVal = 0.5f;
int fileNumber = 0;
std::string fileName = "";
bool savedImageReady = false;
Rect thresholdActivationZone;
bool lumaFromFullFrame = true;
bool temporal = false;
enum {
    NUM_PRIOR_LUMAS = 30,
};
float priorLumas[NUM_PRIOR_LUMAS];
int priorLumaCounter = 0;

enum {
    DARK_DETECTION,
    LIGHT_DETECTION,
    RANGE_DETECTION
};
int thresholdRadius = 5;


void setUpPreview() {
    previewRect = Rect(1327, 58, 200, 615);
    previewCircularImage = Mat::zeros(60 * numSecondsPreview /*previewRect.height*/, previewRect.width, CV_8UC3);
    previewImage = Mat::zeros(60 * numSecondsPreview /*previewRect.height*/, previewRect.width, CV_8UC3);
    previewLineIndex = 0;
}

class CVTilesListener : public photron::VideoCaptureImageListener {
public:
    CVTilesListener(int numTiles, int tileHeight, int width) {
        prior = new Mat[numTiles];
       
        this->tileHeight = tileHeight;
        this->numTiles = numTiles;
        fullImage = Mat::zeros(numTiles * 1, width, CV_8UC1);
        this->width = width;
        isSaving = false;
    }

    ~CVTilesListener()
    {
        delete[]prior;
    }
    virtual void imageReady(Mat& currentFrame, USHORT sequenceNum) {

        if (isSaving)
            return;

        if (priorSequenceNum == sequenceNum) {
            //cout << "Duplicate frame" << endl;
            return;
        }

        int dropFrames = sequenceNum - priorSequenceNum - 1;
        
        if(dropFrames>0)
            totalDropFrames += dropFrames;
        measuredDropFrames++;

        priorSequenceNum = sequenceNum;

        // Copy this into the prior
        prior[writeIndex] = currentFrame.clone();

       
        writeIndex = (writeIndex + 1) % numTiles;
        // Update circular buffer
        if (readIndex == writeIndex) {
            readIndex = (readIndex + 1) % numTiles;
        }

    }

    void read(Mat& mat) {
        int lastWrittenIndex = writeIndex - 1;
        if (lastWrittenIndex < 0)
            lastWrittenIndex += numTiles;
        lastWrittenIndex %= numTiles;
        mat = prior[lastWrittenIndex];
    }

    int getPriorSequenceNum() {
        return priorSequenceNum;
    }
    void start() {
        cap.addListener(this);
    }

    void stop() {
        cap.addListener(nullptr);
    }

    void save() {

        isSaving = true;
        int k = 0;
        for (int r = readIndex; r < readIndex + numTiles; r++, k++) {

            int i = r % numTiles;

            // check if prior[i] is invalid continue
            if (prior[i].empty())
                continue;

            int y = tileHeight >> 1;
            uchar* src = prior[i].ptr(y, 0);
            uchar* dst = fullImage.ptr(k, 0);
            // call src to dst using mempcy (figure out number opf bytes)
            std::memcpy(dst, src, width);
        }
        totalDropFrames = 0;
        measuredDropFrames = 0;
        isSaving = false;
        
        fileName = "test" + std::to_string(fileNumber) + ".jpg";
        ++fileNumber;
        imwrite(fileName, fullImage);
        savedImageReady = true;
    }


    float getDropFrames() {
        if (measuredDropFrames == 0) {
            return 0.0f;
        }
        return (float) totalDropFrames / (float) measuredDropFrames;
    }

private:
    int tileHeight;
    Mat* prior;
    int readIndex = 0;
    int writeIndex = 0;
    int readIndexPreview = 0;
    int writeIndexPreview = 0;
    int numTiles;
    Mat fullImage;
    USHORT priorSequenceNum = 0;
    int width;
    bool isSaving;
    int totalDropFrames = 0;
    int measuredDropFrames = 0;
};

CVTilesListener* pListener = nullptr;
bool isPreviewWindowDragging = false;
bool isThreshBarDragging = false;
bool isTimeBarDragging = false;
bool isThreshZoneDragging = false;
bool triggerOn = false;
int detectMode = LIGHT_DETECTION;
int mouseDownPosition = -1;
int previewWindowRectX = -1;
int threshBarX = -1;
int timeBarY = -1;
int threshRadiusDown = -1;
bool fullScreenModeOn = false;

enum {
    GUI_BUTTON_CAPTURE,
    GUI_BUTTON_EXIT,
    GUI_BUTTON_TRIGGER,
    GUI_BUTTON_LIGHTDETECT,
    GUI_BUTTON_DARKDETECT,
    GUI_BUTTON_RANGEDETECT,
    GUI_BUTTON_FULLFRAMELUMA,
    GUI_BUTTON_PREVIEWONLYLUMA,
    GUI_BUTTON_TEMPORAL,
    GUI_BUTTON_DEC,
    GUI_BUTTON_INC,
    GUI_BUTTON_FULLSCREEN,
    GUI_BUTTON_SAVEDIMAGE,
    NUM_GUIS
};
bool isPressed[NUM_GUIS] = { false, false, false, true, false, false, true, false, false, false, false, false, false };
Rect guiRect[NUM_GUIS];



void callBackFunc(int event, int x, int y, int flags, void* userdata)
{
    if (event == EVENT_LBUTTONDOWN)
    {
        if (guiRect[GUI_BUTTON_CAPTURE].contains(Point(x, y)))
        {
            rectangle(canvas, guiRect[GUI_BUTTON_CAPTURE], Scalar(255, 255, 255), 2);
            imshow(winName, canvas);
            pListener->save();
            waitKey(1);
            return;
        }
        else if (guiRect[GUI_BUTTON_EXIT].contains(Point(x, y)))
        {
            rectangle(canvas, guiRect[GUI_BUTTON_EXIT], Scalar(255, 255, 255), 2);
            imshow(winName, canvas);
            waitKey(1);
            return;
        }
        else if (guiRect[GUI_BUTTON_TRIGGER].contains(Point(x,y)))
        {
            rectangle(canvas, guiRect[GUI_BUTTON_TRIGGER], Scalar(255, 255, 255), 2);
            imshow(winName, canvas);
            waitKey(1);
            return;
        }
        else if (previewWindowRect.contains(Point(x, y))) 
        {
            mouseDownPosition = x;
            isPreviewWindowDragging = true;
            previewWindowRectX = previewWindowRect.x;
            return;
        }
        else if (thresholdBar.contains(Point(x, y)))
        {
            mouseDownPosition = x;
            isThreshBarDragging = true;
            threshBarX = thresholdBar.x;
            return;
        }
        else if (timeBar.contains(Point(x, y)))
        {
            mouseDownPosition = y;
            isTimeBarDragging = true;
            timeBarY = timeBar.y;
            return;
        }
        else if (thresholdActivationZone.contains(Point(x, y))) 
        {
            if (detectMode == RANGE_DETECTION && temporal) {
                mouseDownPosition = x;
                isThreshZoneDragging = true;
                threshRadiusDown = thresholdRadius;
                return;
            }
        }
        else {
            for (int i = 0; i < NUM_GUIS; ++i) {
                if (guiRect[i].contains(Point(x, y))) {
                    isPressed[i] = true;
                    return;
                }
            }
        }
    }

    if (event == EVENT_MOUSEMOVE) {
        if (isPreviewWindowDragging == true) {
            int xdelta = x - mouseDownPosition;
            // Now use xdelta to update the preview window
            int newPrevWindowXLocation = previewWindowRectX + xdelta;
            if (newPrevWindowXLocation < imageRect.x)
                newPrevWindowXLocation = imageRect.x;
            else if (newPrevWindowXLocation + previewWindowRect.width >= imageRect.x + imageRect.width)
                newPrevWindowXLocation = imageRect.x + imageRect.width - previewWindowRect.width + 1;
            previewWindowRect.x = newPrevWindowXLocation;
        }
        else if (isThreshBarDragging == true) {
            int xdelta = x - mouseDownPosition;
            // Now use xdelta to update the threshold bar
            int newThreshBarXLocation = threshBarX + xdelta;
            if (newThreshBarXLocation < histRect.x)
                newThreshBarXLocation = histRect.x;
            else if (newThreshBarXLocation >= (histRect.x + histRect.width))
                newThreshBarXLocation = histRect.x + histRect.width;
           
            thresholdVal = (float) 255.0f * (newThreshBarXLocation - histRect.x) / (histRect.width);
        }
        else if (isTimeBarDragging == true) {
            int ydelta = y - mouseDownPosition;
            // Now use xdelta to update the threshold bar
            int newTimeBarYLocation = timeBarY + ydelta;
            if (newTimeBarYLocation < ((previewRect.height + previewRect.y) - (previewRect.height / numSecondsPreview)))
                newTimeBarYLocation = (previewRect.height + previewRect.y) - (previewRect.height / numSecondsPreview);
            else if (newTimeBarYLocation >= (previewRect.y + previewRect.height))
                newTimeBarYLocation = previewRect.y + previewRect.height;

            timeVal = (float) numSecondsPreview - numSecondsPreview * ((float) (newTimeBarYLocation - previewRect.y) / (float) (previewRect.height));
        }
        else if (isThreshZoneDragging == true) {
            int xdelta = x - mouseDownPosition;
            // Now use xdelta to update the threshold bar
            int val = threshRadiusDown + 255 * ((float) xdelta / histRect.width);
            if (val < 1)
                val = 1;
            else if (val >= 50)
                val = 50;

            thresholdRadius = val;
        }
    }

    if (event == EVENT_LBUTTONUP)
    {
        if (guiRect[GUI_BUTTON_CAPTURE].contains(Point(x, y)))
        {
            canvas = background.clone();
            return;
        }
        else if (guiRect[GUI_BUTTON_EXIT].contains(Point(x, y)))
        {
            cap.getPUCLibWrapper()->close();
            exit(-1);
        }
        else if (guiRect[GUI_BUTTON_TRIGGER].contains(Point(x, y)))
        {
            triggerOn = !triggerOn;
            return;
        }
        else if (guiRect[GUI_BUTTON_LIGHTDETECT].contains(Point(x, y)))
        {
            detectMode = LIGHT_DETECTION;
            temporal = false;
            return;
        }
        else if (guiRect[GUI_BUTTON_DARKDETECT].contains(Point(x, y)))
        {
            detectMode = DARK_DETECTION;
            temporal = false;
            return;
        }
        else if (guiRect[GUI_BUTTON_RANGEDETECT].contains(Point(x, y)))
        {
            detectMode = RANGE_DETECTION;
            temporal = true;
            return;
        }
        else if (guiRect[GUI_BUTTON_FULLFRAMELUMA].contains(Point(x, y)))
        {
            lumaFromFullFrame = true;
            return;
        }
        else if (guiRect[GUI_BUTTON_PREVIEWONLYLUMA].contains(Point(x, y)))
        {
            lumaFromFullFrame = false;
            return;
        }
        else if (guiRect[GUI_BUTTON_TEMPORAL].contains(Point(x, y)))
        {
            if (detectMode == RANGE_DETECTION)
                return;
            temporal = !temporal;
            return;
        }
        else if (guiRect[GUI_BUTTON_INC].contains(Point(x, y)))
        {
            numSecondsPreview += 1;
            if (numSecondsPreview > 5)
                numSecondsPreview = 5;
            setUpPreview();
        }
        else if (guiRect[GUI_BUTTON_DEC].contains(Point(x, y)))
        {
            numSecondsPreview -= 1;
            if (numSecondsPreview < 1)
                numSecondsPreview = 1;
            setUpPreview();
        }
        else if (guiRect[GUI_BUTTON_FULLSCREEN].contains(Point(x, y)))
        {
            fullScreenModeOn = !fullScreenModeOn;
            return;
        }
        else if (guiRect[GUI_BUTTON_SAVEDIMAGE].contains(Point(x, y)))
        {
            system(fileName.c_str());
            savedImageReady = false;
        }

        isPreviewWindowDragging = false;
        isThreshBarDragging = false;
        isTimeBarDragging = false;
        isThreshZoneDragging = false;
        for (int i = 0; i < NUM_GUIS; ++i) {
            isPressed[i] = false;
        }
    }


}


void drawButtons() {
    if (detectMode == LIGHT_DETECTION) {
         circle(canvas, Point(540, 610), 9, Scalar(0, 0, 0), FILLED, LINE_8);
    }
    else if (detectMode == DARK_DETECTION) {
         circle(canvas, Point(640, 610), 9, Scalar(0, 0, 0), FILLED, LINE_8);
    }
    else {
        circle(canvas, Point(749, 610), 9, Scalar(0, 0, 0), FILLED, LINE_8);
    }

    if (lumaFromFullFrame == true) {
         circle(canvas, Point(925, 643), 9, Scalar(0, 0, 255), FILLED, LINE_8);
    }
    else {
        circle(canvas, Point(925, 679), 9, Scalar(0, 0, 255), FILLED, LINE_8);
    }

    if (temporal == true) {
        rectangle(canvas, guiRect[GUI_BUTTON_TEMPORAL], Scalar(46, 35, 112), FILLED);
    }
}




int main(int argc, char** argv)
{      
    //--- INITIALIZE VIDEOCAPTURE
    int fps[] = {50, 250, 500, 950, 1000, 2000, 5000, 10000, 20000, 31157};
    int width = 1246;
    int height[] = {1024, 1024, 1024, 1024, 1008, 496, 176, 80, 32, 16};
    int nExpOnClk[] = { 19988700, 3988700, 1988700, 1041300, 988700, 488700, 188700, 88700, 88700, 38700, 22000 };
    int nExpOffClk = 11200;


    cap.getPUCLibWrapper()->setMultiThread(true);
    int mode = 9;
    int tileHeight = height[mode];
    int numTiles = 30000;

    cap.getPUCLibWrapper()->setResolution(width, tileHeight);
    cap.getPUCLibWrapper()->setFramerateShutter(fps[mode], fps[mode]);
    cap.getPUCLibWrapper()->setExposeTime(nExpOnClk[mode], nExpOffClk);

    cout << "Resolution " << width << " x " << tileHeight << "\n";
    cout << "fps " << fps[mode] << "\n";


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

    cout << "numTiles=" << numTiles << endl;

    int prevmsec = 0;
    SYSTEMTIME st;
    bool showDc = true;

    // An image
    background = imread("cvtiles_background.png");
    cvtColor(background, background, COLOR_BGRA2BGR);
    canvas = background.clone();

    // Your button
    guiRect[GUI_BUTTON_CAPTURE] = Rect(8, 669, 120, 37);
    guiRect[GUI_BUTTON_EXIT] = Rect(8, 723, 120, 37);

    // Trigger buttons
    guiRect[GUI_BUTTON_TRIGGER] = Rect(192, 652, 121, 39);
    guiRect[GUI_BUTTON_LIGHTDETECT] = Rect(532, 601, 17, 17);
    guiRect[GUI_BUTTON_DARKDETECT] = Rect(632, 601, 17, 17);
    guiRect[GUI_BUTTON_RANGEDETECT] = Rect(738, 601, 17, 17);

    imageRect = Rect(26, 127, width, 16);
    Rect scopeRect(26, 127 + 35, width, 100);

    // Luminance buttons
    guiRect[GUI_BUTTON_FULLFRAMELUMA] = Rect(914, 635, 17, 17);
    guiRect[GUI_BUTTON_PREVIEWONLYLUMA] = Rect(914, 671, 17, 17);
    guiRect[GUI_BUTTON_TEMPORAL] = Rect(917, 720, 19, 19);

    // Increment & decrement buttons
    guiRect[GUI_BUTTON_INC] = Rect(1573, 306, 37, 27);
    guiRect[GUI_BUTTON_DEC] = Rect(1573, 398, 37, 27);

    // Fullscreen button
    guiRect[GUI_BUTTON_FULLSCREEN] = Rect(22, 15, 10, 10);

    setUpPreview();

    // Preview window over live feed
    int xPrevWindRect = (imageRect.width >> 1) + imageRect.x - 100;
    int yPrevWindMargin = 4;
    previewWindowRect = Rect(xPrevWindRect, imageRect.y - yPrevWindMargin, 200, imageRect.height + 2 * yPrevWindMargin);

    namedWindow(winName);
    setMouseCallback(winName, callBackFunc);

    cap.setFrameSampleRate(1,0);

    CVTilesListener listener(numTiles, tileHeight, width);
    pListener = &listener;
    listener.start();

    // Default set PRIORLUMAS array to all 0's
    memset(priorLumas, 0, NUM_PRIOR_LUMAS * sizeof(float));

    int counter = 0;
    enum {
        TRIGGER_READY = -120
    };
    int triggerCounter = TRIGGER_READY;
    cv::Mat fullscreenImg(752, 1024, CV_8UC3, cv::Scalar(0, 0, 0));

    while(getWindowProperty(winName, cv::WND_PROP_VISIBLE))
    {
        GetSystemTime(&st);

        int dur;
        if (prevmsec > st.wMilliseconds) {
            dur = st.wMilliseconds + 1000 - prevmsec;
        }
        else {
            dur = st.wMilliseconds - prevmsec;
        }

        //printf("%d\n", dur);
        prevmsec = st.wMilliseconds;

        
        Mat currentFrame;
        listener.read(currentFrame);
        int currentSequenceNumber = listener.getPriorSequenceNum();
        float numDropFrames = listener.getDropFrames();

        if (!currentFrame.empty()) {

            
            // Display Current Frame
            int histogram[256];
            memset(histogram, 0, 1024);
            double sum = 0.0;
            double average;
            double latestAverage;

            const unsigned char* line;
            line = currentFrame.ptr(8, 0);

            int xMin = 0;
            int xMax = width - 1;
            int threshold = 128;

            if (!lumaFromFullFrame) {
                xMin = previewWindowRect.x - imageRect.x;
                if (xMin < 0) 
                    xMin = 0;

                xMax = xMin + previewWindowRect.width;
                if (xMax >= width)
                    xMax = width - 1;
            }

            int xMinAboveThresh = -1;
            int xMaxAboveThresh = -1;
            for (int x = xMin; x <= xMax; x++) {
                int val = line[x];
                histogram[val]++;
                sum += val;

                if (xMinAboveThresh ==-1 && val > threshold) {
                    xMinAboveThresh = x;
                }
                if (xMaxAboveThresh == -1 && line[width - x - 1] > threshold) {
                    xMaxAboveThresh = width - x - 1;
                }
            }
            average = sum / ((double) xMax - (double) xMin + 1.0);
            latestAverage = average;
            priorLumas[priorLumaCounter] = latestAverage;
            priorLumaCounter = (priorLumaCounter + 1) % NUM_PRIOR_LUMAS;

            if (temporal) {
                double sumOfLumaArray = 0;
                for (int i = 0; i < NUM_PRIOR_LUMAS; ++i) {
                    sumOfLumaArray += priorLumas[i];
                }
                average = sumOfLumaArray / (double) NUM_PRIOR_LUMAS;
            }


            // Find max
            float maxHistogramValue = 1;
            for (int x = 0; x < 256; x++) {
                if (histogram[x] > maxHistogramValue)
                    maxHistogramValue = histogram[x];
            }

            // Normalize
            float histMix[256];
            for (int x = 0; x < 256; x++) {
                histMix[x] = histogram[x] / maxHistogramValue;
            }

            
            canvas = background.clone();


            std::string strCounter = std::to_string(currentSequenceNumber % 30000);
            cv::putText(canvas, strCounter, cv::Point(924, 368), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 0, 0), 2, false);

            // Make status bar for Sequence Number
            int leftSeqCounter = 1050;
            int rightSeqCounter = 1260;
            int topSeqCounter = 348;
            int bottomSeqCounter = 371;
            
            float mix = float(currentSequenceNumber % 30000) / 30000.0;
            int adjustedRight = mix * rightSeqCounter + (1.0f - mix) * leftSeqCounter;
            rectangle(canvas, Rect(leftSeqCounter, topSeqCounter, adjustedRight - leftSeqCounter, bottomSeqCounter - topSeqCounter), Scalar(0, 0, 0), FILLED);
            

            std::string strDropTiles = std::to_string(numDropFrames);
            cv::putText(canvas, strDropTiles, cv::Point(924, 427), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 0, 0), 2, false);


            std::string strAverage = std::to_string(average);
            cv::putText(canvas, strAverage, cv::Point(924, 486), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(0, 0, 255), 2, false);

            
            cvtColor(currentFrame, currentFrame, COLOR_GRAY2BGR);

            currentFrame.copyTo(canvas(imageRect));

            // Display threshold bar
            float thresholdMix = (float)thresholdVal / 255.0f;
            const int thresholdWidth = 3;
            int x1 = (histRect.x + histRect.width) * thresholdMix + histRect.x * (1.0f - thresholdMix);
            thresholdBar = Rect(x1, histRect.y, thresholdWidth, histRect.height);

            // Calculate position of average luma line
            float mixAvg = float(average) / 256.f;
            int xAvg = mixAvg * (histRect.x + histRect.width) + (1.0f - mixAvg) * histRect.x;

            if (detectMode == LIGHT_DETECTION) {
                thresholdActivationZone = Rect(thresholdBar.x + thresholdBar.width, thresholdBar.y + 1, (histRect.width + histRect.x) - thresholdBar.x, thresholdBar.height - 1);
            }
            else if (detectMode == DARK_DETECTION) {
                thresholdActivationZone = Rect(histRect.x, thresholdBar.y + 1, (thresholdBar.x + thresholdBar.width) - histRect.x, thresholdBar.height - 1);
            }
            else if (detectMode == RANGE_DETECTION) {
                int radius = (thresholdRadius / 255.0f) * histRect.width;
                thresholdActivationZone = Rect(xAvg - radius, thresholdBar.y + 1, radius * 2, thresholdBar.height - 1);
                if (thresholdActivationZone.x < histRect.x)
                    thresholdActivationZone.x = histRect.x;
                if (thresholdActivationZone.x + thresholdActivationZone.width > histRect.x + histRect.width)
                    thresholdActivationZone.width = histRect.x + histRect.width - thresholdActivationZone.x;
            }
            
            if (detectMode != RANGE_DETECTION) {
                rectangle(canvas, thresholdActivationZone, Scalar(190, 226, 230), FILLED);
                rectangle(canvas, thresholdBar, Scalar(1, 173, 225), FILLED);
            }
            else {
                rectangle(canvas, histRect, Scalar(190, 226, 230), FILLED);
                rectangle(canvas, thresholdActivationZone, Scalar(238, 238, 238), FILLED);

                Rect radiusDetectUnderline = thresholdActivationZone;
                radiusDetectUnderline.y += radiusDetectUnderline.height;
                radiusDetectUnderline.height = 5;
                rectangle(canvas, radiusDetectUnderline, Scalar(1, 173, 255), FILLED);
            }

            // Display histogram
            for (int x = 0; x < 256; x++) {
                float mix = float(x) / 256.f;
                int x0 = mix * (histRect.x + histRect.width) + (1.0f - mix) * histRect.x;
                mix = float(x + 1) / 256.f;
                int x1 = mix * (histRect.x + histRect.width) + (1.0f - mix) * histRect.x;

                int w = x1 - x0 + 1;
                int y0 = histRect.y;
                int y1 = (histRect.y + histRect.height) * histMix[x] + histRect.y * (1.0 - histMix[x]);
                int h = y1 - y0 + 1;

                rectangle(canvas, Rect(x0, (histRect.y + histRect.height) - h, w, h), Scalar(0, 0, 0), FILLED);
            }
            // Draw average luma line
            if (detectMode == RANGE_DETECTION) { 
                // Draw latest average luma line
                float mixLatAvg = float(latestAverage) / 256.f;
                int xLatAvg = mixLatAvg * (histRect.x + histRect.width) + (1.0f - mixLatAvg) * histRect.x;

                cv::line(canvas, Point(xLatAvg, histRect.y), Point(xLatAvg, histRect.y + histRect.height), Scalar(0, 0, 255), 1, LINE_8);
                cv::line(canvas, Point(xAvg, histRect.y), Point(xAvg, histRect.y + histRect.height), Scalar(46, 35, 112), 3, LINE_8);
            }
            else {
                cv::line(canvas, Point(xAvg, histRect.y), Point(xAvg, histRect.y + histRect.height), Scalar(0, 0, 255), 1, LINE_8);
            }

            // Draw scope
            for (int x = 0; x < width - 1; x++) {
                float mix_x1 = (float) x / (float) width;
                float mix_x2 = (float)(x + 1) / (float)width;
                int x1 = (scopeRect.x + scopeRect.width) * mix_x1 + scopeRect.x * (1.0f - mix_x1);
                int x2 = (scopeRect.x + scopeRect.width) * mix_x2 + scopeRect.x * (1.0f - mix_x2);
                float mix_y1 = (float) line[x] / 255.f;
                float mix_y2 = (float) line[x + 1] / 255.f;
                int y1 = scopeRect.y * mix_y1 + (scopeRect.y + scopeRect.height) * (1.0f - mix_y1);
                int y2 = scopeRect.y * mix_y2 + (scopeRect.y + scopeRect.height) * (1.0f - mix_y2);
                cv::line(canvas, Point(x1, y1), Point(x2, y2), Scalar(0, 0, 255), 1, LINE_8);
            }
            // Draw average line
            mix = (float)average / 255.f;
            int avgY = scopeRect.y * mix + (scopeRect.y + scopeRect.height) * (1.0f - mix);
            cv::line(canvas, Point(scopeRect.x, avgY), Point(scopeRect.x + scopeRect.width, avgY), Scalar(0, 0, 0), 1, LINE_8);

            int xMinScope, xMaxScope;
            // Draw min and max lines
            if (xMinAboveThresh != -1) {
                mix = (float)xMinAboveThresh / (float) width;
                xMinScope = (scopeRect.x + scopeRect.width) * mix + scopeRect.x * (1.0f - mix);
                cv::line(canvas, Point(xMinScope, scopeRect.y), Point(xMinScope, scopeRect.y + scopeRect.height), Scalar(0, 0, 0), 1, LINE_8);
            }

            if (xMaxAboveThresh != -1) {
                mix = (float)xMaxAboveThresh / (float)width;
                xMaxScope  = (scopeRect.x + scopeRect.width) * mix + scopeRect.x * (1.0f - mix);
                cv::line(canvas, Point(xMaxScope, scopeRect.y), Point(xMaxScope, scopeRect.y + scopeRect.height), Scalar(0, 0, 0), 1, LINE_8);
            }


            // Draw average crosshair
            if (xMinAboveThresh != -1 && xMaxAboveThresh != -1) {
                int avg = (xMinAboveThresh + xMaxAboveThresh) >> 1;
                mix = (float)avg / (float)width;
                int x = (scopeRect.x + scopeRect.width) * mix + scopeRect.x * (1.0f - mix);
                int xMargin = 7;
                int yMargin = 1;
                cv::line(canvas, Point(x, imageRect.y + yMargin), Point(x, imageRect.y + imageRect.height - yMargin), Scalar(0, 0, 255), 1, LINE_8);
                cv::line(canvas, Point(x - xMargin, imageRect.y + 8), Point(x + xMargin, imageRect.y + 8), Scalar(0, 0, 255), 1, LINE_8);
            }

            // Copy one line into the previewImage
            unsigned char* previewSrc;
            unsigned char* previewDest;
            previewSrc = currentFrame.ptr(8, previewWindowRect.x);
            previewDest = previewCircularImage.ptr(previewLineIndex, 0);
            int numBytesPerLine = previewCircularImage.cols * 3;
            memcpy(previewDest, previewSrc, numBytesPerLine);

            // convert previewCircularImage to not circular previewImage
            int src_y1, src_y2;
            int dst_y = 0;
            if (previewLineIndex+1 < previewCircularImage.rows) {
                src_y1 = previewLineIndex + 1;
                src_y2 = previewImage.rows - 1;
                const unsigned char* src1 = previewCircularImage.ptr(src_y1, 0);
                const unsigned char* src2 = previewCircularImage.ptr(src_y2, previewCircularImage.cols - 1);
                int numBytesToCopy = src2 - src1 + 1;
                unsigned char* dst = previewImage.ptr(dst_y, 0);
                memcpy(dst, src1, numBytesToCopy);
                dst_y += src_y2 - src_y1 + 1; 
            }
            src_y1 = 0;
            src_y2 = previewLineIndex;
            const unsigned char* src1 = previewCircularImage.ptr(src_y1, 0);
            const unsigned char* src2 = previewCircularImage.ptr(src_y2, previewCircularImage.cols - 1);
            int numBytesToCopy = src2 - src1 + 1;
            unsigned char* dst = previewImage.ptr(dst_y, 0);
            memcpy(dst, src1, numBytesToCopy);

            
            // Resize the preview Image and draw to canvas
            cv::Mat resizePreviewImage;
            cv::resize(previewImage, resizePreviewImage, cv::Size(previewRect.width, previewRect.height), 0, 0, cv::INTER_LINEAR);
            resizePreviewImage.copyTo(canvas(previewRect));
        
            // Update preview line index
            previewLineIndex = (previewLineIndex + 1) % previewImage.rows;

            // Draw preview window over canvas
            rectangle(canvas, previewWindowRect, Scalar(255, 0, 0), LINE_4);

            // Update buttons
            drawButtons();

            // Update Preview Seconds Text
            std::string strNumSecPreview = std::to_string(numSecondsPreview);
            cv::putText(canvas, strNumSecPreview, cv::Point(1568, 376), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(0, 0, 0), 1, false);

            // Update Trigger On/Off Button
            if (triggerOn) {
                cv::putText(canvas, "ON", cv::Point(227, 682), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(0, 0, 0), 1, false);
            }
            else {
                cv::putText(canvas, "OFF", cv::Point(218, 682), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(0, 0, 0), 1, false);
            }

            drawButtons();

            // Print threshold value
            std::string strThreshold = std::to_string(detectMode != RANGE_DETECTION ? thresholdVal : thresholdRadius);
            cv::putText(canvas, strThreshold, cv::Point(502, 680), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(1, 173, 225), 2, false);

            // Display time bar
            float timeMix = (float) (numSecondsPreview - timeVal) / numSecondsPreview;
            const int timeHeight = 4;
            int y1 = (previewRect.y + previewRect.height) * timeMix + previewRect.y * (1.0f - timeMix);
            timeBar = Rect(previewRect.x, y1, previewRect.width, timeHeight);
            rectangle(canvas, timeBar, Scalar(167, 151, 0), FILLED);

            // Print time value
            std::string strTime = std::to_string(timeVal);
            cv::putText(canvas, strTime, cv::Point(446, 740), cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(167, 151, 0), 2, false);

            if (triggerOn && triggerCounter == TRIGGER_READY) {
                if (detectMode == LIGHT_DETECTION) {
                    if (average >= thresholdVal) {
                        triggerCounter = (int)(timeVal * 60.0f + 0.5f);
                    }
                }
                else if (detectMode == DARK_DETECTION) { 
                    if (average <= thresholdVal) {
                        triggerCounter = (int)(timeVal * 60.0f + 0.5f);
                    }
                }
                else {
                    int distance = abs(latestAverage - average);
                    if (distance > thresholdRadius) {
                        triggerCounter = (int)(timeVal * 60.0f + 0.5f);
                    }
                }
            }

            triggerCounter--;
            if (triggerCounter == 0) {
                pListener->save();
                // Set something to show its ready
            }
            if (triggerCounter < TRIGGER_READY)
                triggerCounter = TRIGGER_READY;


            if (savedImageReady) {
                guiRect[GUI_BUTTON_SAVEDIMAGE] = Rect(1326, 718, 229, 39);
                //rectangle(canvas, guiRect[GUI_BUTTON_SAVEDIMAGE], Scalar(125, 196, 147), FILLED);
                std::string strFileNumber = std::to_string(fileNumber);
                cv::putText(canvas, "Img " + strFileNumber + " Ready", cv::Point(1332, 745), cv::FONT_HERSHEY_TRIPLEX, 1, cv::Scalar(255, 50, 0), 1, false);
                cv::line(canvas, Point(guiRect[GUI_BUTTON_SAVEDIMAGE].x + 2, guiRect[GUI_BUTTON_SAVEDIMAGE].y + guiRect[GUI_BUTTON_SAVEDIMAGE].height - 4), 
                    Point(guiRect[GUI_BUTTON_SAVEDIMAGE].x + guiRect[GUI_BUTTON_SAVEDIMAGE].width - 2, guiRect[GUI_BUTTON_SAVEDIMAGE].y + guiRect[GUI_BUTTON_SAVEDIMAGE].height - 4),
                    Scalar(255, 50, 0), 2, LINE_8);
            }


            imshow(winName, canvas);

        }

        
        int key = waitKey(1);
        if (key == 27)
            break;

        if (key == 's') {

            listener.save();
        }



        ++counter;
    }
    cap.getPUCLibWrapper()->close();
    listener.stop();

    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}