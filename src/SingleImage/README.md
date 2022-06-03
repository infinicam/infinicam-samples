# SingleImage

SingleImage is quick sample code to use [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/). Developed by C language.

This sample code implements the function to get image and decode using INFINICAM SDK [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html).


## Prerequisites
* Installed Visual Studio 2019

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone repository.
   
3. Open solution file on visual studio.

4. Build.

## Multi Thread Decode
You can change the code below to switch between single-threaded and multi-threaded decoding.

```c
// Note:Switch decoding to multi threading
#define MULTITHREAD_DECODE


#ifdef MULTITHREAD_DECODE
	result = PUC_DecodeDataMultiThread(pDecodeBuf, 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q, 8);
#else
	result = PUC_DecodeData(pDecodeBuf, 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q);
#endif // MULTITHREAD_DECODE
```

This is the result of multi-threaded decoding. 

<img src="image/result.png" width="400">

#### Developed by:
<img src="image/Photron_logo.png" width="100">
