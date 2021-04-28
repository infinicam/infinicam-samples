
#include <iostream>
#include "PUCLib_Wrapper.h"
using namespace std;

int main(int argc, char** argv)
{
    photron::PUCLib_Wrapper cap;
    // Saving one image only needs single thread mode API enabled
    cap.setMultiThread(false); 

    // Set Capture Settings
    cap.setResolution(1246, 800);
    cap.setFramerateShutter(1000, 2000);
    // Open
    PUCRESULT result = cap.open(0); 
    if (result != PUC_SUCCEEDED || !cap.isOpened()) {
        cerr << cap.getLastErrorName();
        return -1;
    }
    int width, height, rowBytes;
    // Read
    unsigned char* buffer = cap.read(width, height, rowBytes);
    if (!buffer) {
        cerr << cap.getLastErrorName();
        return -1;
    }
    // Save Frame
    cap.saveBitmap("test.bmp", buffer, width, height, rowBytes);
	return 0;
}

