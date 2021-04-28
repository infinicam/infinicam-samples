
#pragma warning(disable : 4996)

#include <iostream>
#include "CL/cl.hpp"
#include "PhotronOpenCLCapture.h"
using namespace std;

const char *kernel = "\
__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;\n\
__kernel void process(__read_only image2d_t in, __write_only image2d_t out)\n\
{\n\
    const int x = get_global_id(0);\n\
    const int y = get_global_id(1);\n\
    const float4 result = read_imagef(in, sampler, (int2)(x, y));\n\
    write_imagef(out, (int2)(x, y), result * fabs(sin((float)(x+y)/50.0f)));\n\
}\n";

int main(int argc, char** argv)
{
    int err = 0;
    photron::OpenCLCapture cap;
    // Saving one image only needs single thread mode API enabled
    cap.getPUCLibWrapper()->setMultiThread(false);
    // Open
    cap.open(0);
    if (!cap.isOpened()) {
        cerr << cap.getLastErrorName();
        return -1;
    }
    int width, height;
    cap.getResolution(width, height);
    
    // get all platforms (drivers), e.g. NVIDIA
    std::vector<cl::Platform> all_platforms;
    cl::Platform::get(&all_platforms);

    if (all_platforms.size() == 0) {
        std::cout << " No platforms found. Check OpenCL installation!\n";
        exit(1);
    }
    cl::Platform default_platform = all_platforms[0];
    std::cout << "Using platform: " << default_platform.getInfo<CL_PLATFORM_NAME>() << "\n";

    // get default device (CPUs, GPUs) of the default platform
    std::vector<cl::Device> all_devices;
    default_platform.getDevices(CL_DEVICE_TYPE_ALL, &all_devices);
    if (all_devices.size() == 0) {
        std::cout << " No devices found. Check OpenCL installation!\n";
        exit(1);
    }

    cl::Device default_device = all_devices[0];
    std::cout << "Using device: " << default_device.getInfo<CL_DEVICE_NAME>() << "\n";

    // a context is like a "runtime link" to the device and platform;
    // i.e. communication is possible
    cl::Context context({ default_device });

    cl::CommandQueue queue = cl::CommandQueue(context, default_device);


    // create the program that we want to execute on the device
    cl::Program::Sources sources;

    std::string kernel_code = kernel;
    sources.push_back({ kernel_code.c_str(), kernel_code.length() });

    cl::Program program(context, sources);
    if (program.build({ default_device }) != CL_SUCCESS) {
        std::cout << "Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(default_device) << std::endl;
        exit(1);
    }

    // Create an OpenCL Image / texture and transfer data to the device
    cl::Image2D clImage;
    if (cap.uploadImage(context, queue, clImage) != 0)
    {
        std::cout << "Error uploading images: " << std::endl;
        exit(1);
    }

    // Create an Output OpenCL Image 
    cl::Image2D clResult = cl::Image2D(context, CL_MEM_WRITE_ONLY, cl::ImageFormat(CL_R, CL_UNSIGNED_INT8), width, height);

    // Run Gaussian kernel
    cl::Kernel process = cl::Kernel(program, "process");
    err = process.setArg(0, clImage);
    err = process.setArg(1, clResult);

    err = queue.enqueueNDRangeKernel(
        process,
        cl::NullRange,
        cl::NDRange(width, height),
        cl::NullRange
    );

    cl::size_t<3> origin;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    cl::size_t<3> region;
    region[0] = width;
    region[1] = height;
    region[2] = 1;

    int rowBytes = cap.getRowBytes();
    unsigned char* basePtrResult = (unsigned char*)malloc(rowBytes * height);
    err = queue.enqueueReadImage(clResult, CL_TRUE, origin, region, rowBytes, 0, basePtrResult, NULL, NULL);


    // Save Frame
    photron::PUCLib_Wrapper::saveBitmap("test.bmp", basePtrResult, width, height, rowBytes);

    free(basePtrResult);

    return 0;
}

