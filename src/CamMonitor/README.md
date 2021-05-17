# CamMonitor

![app](image/app_cammonitor.png)

<hr>

CamMonitor is sample code for [INFINICAM UC-1](https://www.photron.co.jp/products/hsvcam/infinicam/). Developped by C++ MFC.

This sample code includes all functions of INFINICAM SDK [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html). You can experience all the features using this application.
Let's start development using INFINICAM UC-1!

## How to use
<img src="image/ss/select.png" width="300">

1. Start CamMonitor and select CameraNo. CAM 0 is first camera found.
2. Set the Transfer parameters, If want to confirm other than default.
3. Press **Open** to start Live 

<img src="image/ss/live.png" width="400">

4. On the Live view, can change camera parameters and confirm the Live image.

<img src="image/ss/rec.png" width="400">

5. Change Acquitision mode to **continuous** before record movie.
6. Press Record to start recording.

<img src="image/ss/file.png" width="400">

7. Press **FILE tab** to playback the video
8. Press **Open File** and select movie file (select .cih file)
9. Press **play** to start playback


## Enviroment
* installed Visual Studio 2019

    :warning: MFC Package is required. 

## Build
1. Download and install [PUCLIB](https://www.photron.co.jp/products/hsvcam/infinicam/tech.html) SDK.

2. Clone this sourcecode.
   
3. Open [CamMonitor.sln](https://github.com/infinicam/CamMonitor/blob/master/CamMonitor.sln) on visual studio.

4. Build

### Advanced setting

INFINICAM UC-1 can control quantization parameters for compress the image. if you want to set the parametes, change this code 

```c
LiveTab.cpp
void CLiveTab::UpdateControlState()
{
    ~~
    GetDlgItem(IDC_QUANTIZATION)->EnableWindow(FALSE);
}
```
to

```c
void CLiveTab::UpdateControlState()
{
    ~~
    GetDlgItem(IDC_QUANTIZATION)->EnableWindow(bOpened && !bContinuous);
}
```

#### developped by:
<img src="image/Photron_logo.png" width="100">
