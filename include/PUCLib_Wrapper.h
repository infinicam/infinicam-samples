#pragma once

/*!
	@mainpage
	@~english
		@brief OpenCV like VideoCapture class
	@~japanese
		@brief OpenCV風のVideoCaptureクラス

	@copyright Copyright (C) 2021 PHOTRON LIMITED
*/

#pragma warning (disable : 4996)
#include <stdio.h>
#include <Windows.h>
#include <string>
#include <mutex>
#include "PUCLIB.h"

// Use Multithread
#define USE_DECODE_MULITHRREAD

namespace photron {

	class PUCLib_WrapperImageListener {
	public:
		virtual void imageReady(unsigned char* image, int width, int height, int rowBytes, USHORT sequenceNum) = 0;
	};

	class PUCLib_Wrapper {
		bool m_isSingleThread = false; // set to false for fast performance
		int m_numDecodeThreads = 16;
	public:

		/*!
			@~english
				@brief Constructor
				@details Creates a wrapper that simplifies the operation of the camera
				@note This function is thread-safe.
				@see open
			@~japanese
				@brief コンストラクタです。
				@details カメラ操作を簡素化するラッパの生成します。
				@note 本関数はスレッドセーフです。
				@see ~PUCLib_Wrapper()
		*/
		PUCLib_Wrapper() {
			static bool firstTime = true;
			if (firstTime) {
				firstTime = false;
				result = PUC_Initialize();
			}
		}

		/*!
			@~english
				@brief Sets the mode to multithread (default) or single thread
				@details Sets the mode to multithread (default) or single thread. Call before open or when after open only when paused.
				@note This function is thread-safe.
			@~japanese
				@brief マルチスレッドモードに設定(デフォルト)またはシングルスレッドモードに設定  
				@details マルチスレッドモード(デフォルト)またはシングルスレッドモードに設定します。オープン前に呼び出すか、オープン後であれば一時停止して呼び出すこと。
				@note  本関数はスレッドセーフです。
		*/
		void setMultiThread(bool multiThread) {
			m_isSingleThread = !multiThread;
		}

		void addListener(PUCLib_WrapperImageListener* listener) {
			this->listener = listener;
		}
		
		/*!
			@~english
				@brief Sets the number of threads
				@details Sets the number of threads for decode operation
				@note This function is thread-safe.
			@~japanese
				@brief スレッド数の設定
				@details デコード処理時のスレッド数を設定します。
				@note 本関数はスレッドセーフです。
		*/
		void setNumDecodeThreads(int num) {
			m_numDecodeThreads = num;
		}

		/*!
			@~english
				@brief Destructor
				@details Call this to delete the instance.
				@note This function is thread-safe.
				@see PUCLib_Wrapper
			@~japanese
				@brief デストラクタです。
				@details インスタンス削除時にコールしてください。
				@note 本関数はスレッドセーフです。
				@see PUCLib_Wrapper()
		*/
		~PUCLib_Wrapper() {
			close();
		}


		/*!
			@~english
				@brief Return the last error
				@details Use this function to inform you of the error string
				@return Returns the last error as a string (const char *)
				@note This function is thread-safe.
			@~japanese
				@brief 最後のエラーを返します。
				@details  エラー文字列を得るためにこの関数を用いてください。
				@return 文字列(const char *)として最後のエラーを返します。
				@note 本関数はスレッドセーフです。
		*/
		const char* getLastErrorName() const {
			return m_lastErrorName.c_str();
		}

		/*!
			@~english
				@brief This opens the device.
				@details Note that specifying a device number of the opened device closes the device temporarily.
				@param[in] deviceID Specify the retrieved device ID here.
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
				@see close()
			@~japanese
				@brief デバイスをオープンします。
				@details 既にオープン中のデバイス番号を指定すると一度クローズされますのでご注意ください。
				@param[in] nDeviceNo 検索したデバイス番号を指定します。
				@param[out] pDeviceHandle オープンしたデバイスを操作するためのハンドルが格納されます。
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
				@see close
		*/
		PUCRESULT open(int deviceID) {
			PUCRESULT result = PUC_SUCCEEDED;
			PUC_DETECT_INFO detectInfo = { 0 };

			result = PUC_DetectDevice(&detectInfo);
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_DetectDevice error";
				goto EXIT_LABEL;
			}
			if (detectInfo.nDeviceCount == 0)
			{
				m_lastErrorName = "device count : 0";
				goto EXIT_LABEL;
			}

			result = PUC_OpenDevice(detectInfo.nDeviceNoList[deviceID], &hDevice);
			if (PUC_CHK_FAILED(result))
			{
				// Camera is Detected but cannot open then call reset
				result = PUC_ResetDevice(detectInfo.nDeviceNoList[deviceID]);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_ResetDevice error";
					goto EXIT_LABEL;
				}
				result = PUC_OpenDevice(detectInfo.nDeviceNoList[deviceID], &hDevice);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_ResetDevice error";
					goto EXIT_LABEL;
				}
			}
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_OpenDevice error";
				goto EXIT_LABEL;
			}

			result = PUC_SetFramerateShutter(hDevice, m_frameRate, m_shutterSpeedFps);
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_SetFramerateShutter error";
				goto EXIT_LABEL;
			}

			result = PUC_SetResolution(hDevice, m_resolutionWidth, m_resolutionHeight);
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_SetFramerateShutter error";
				goto EXIT_LABEL;
			}

			result = PUC_GetXferDataSize(hDevice, &nDataSize);
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_GetXferDataSize error";
				goto EXIT_LABEL;
			}

			result = setupDataBuffer();


			return result;

		EXIT_LABEL:
			if (hDevice)
			{
				cleanupBuffer();

				result = PUC_CloseDevice(hDevice);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_CloseDevice error";
				}
				hDevice = NULL;
			}
			return result;
		}

		/*!
			@~english
				@brief This informs you if the device has been opened succesfully
				@details Once a device is opened then calling this function should return true
				@return A true value is returned if the device is open otherwise false
				@note This function is thread-safe.
				@see open
			@~japanese
				@brief デバイスが正常にオープンされたかどうかを通知します。
				@details デバイスがオープンされ、この関数が呼ばれたら真(true)を返す必要があります。
				@return デバイスがオープンされたら真(true)を返し、それ以外の場合に偽(false)を返します。
				@note 本関数はスレッドセーフです。
				@see open
		*/
		bool isOpened() {
			return hDevice != NULL;
		}

		/*!
			@~english
				@brief This closes the device.
				@details An error will be returned when specifying an unopened device.
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
				@see PUC_OpenDevice
			@~japanese
				@brief デバイスをクローズします。
				@details オープンしていないデバイスが指定されるとエラーを返します。
				@param[in] hDevice クローズするデバイスハンドル
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
				@see PUC_OpenDevice
		*/
		PUCRESULT close() {
			PUCRESULT result = PUC_SUCCEEDED;
			if (hDevice)
			{
				cleanupBuffer();
				result = PUC_CloseDevice(hDevice);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_CloseDevice error";
				}
			}
			hDevice = NULL;
			return result;
		}

		/*!
	@~english
		@brief This pauses the camera multithread transfetr
		@details An error will be returned when specifying an unopened device.
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe.
	@~japanese
		@brief カメラからのマルチスレッドの転送を一時停止します。
		@details オープンしていないデバイスを指定するとエラーになります。
		@return 成功するとPUC_SUCCEEDEDが返されます。失敗した場合は理由に応じたエラーが返されます。
		@note 本関数はスレッドセーフです。
*/
		PUCRESULT pause() {
			PUCRESULT result = PUC_SUCCEEDED;
			// std::cerr  << "cp-in-pause: hDevice=" << hDevice << std::endl;
			if (hDevice)
				cleanupBuffer();
			return result;
		}

		/*!
		@~english
			@brief This resumes the camera multithread transfetr
			@details An error will be returned when specifying an unopened device.
			@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
			@note This function is thread-safe.
		@~japanese
			@brief カメラからのマルチスレッドの転送を復帰します。
			@details オープンしていないデバイスを指定するとエラーになります。
			@return 成功するとPUC_SUCCEEDEDが返されます。失敗した場合は理由に応じたエラーが返されます。
			@note 本関数はスレッドセーフです。
		*/
		PUCRESULT resume() {
			PUCRESULT result = PUC_SUCCEEDED;
			if (hDevice)
				setupDataBuffer();
			return result;
		}

		/*!
			@~english
				@brief Reads the latest full sized image from the camera
				@details Returns the buffer to the image and the image resolution
				@param[out] width of the image
				@param[out] height of the image
				@param[out] rowBytes, number of bytes per row
				@return If successful, a pointer to the image buffer is returned (do not delete it, just read it)
				@note This function is thread-safe.
			@~japanese
				@brief カメラから最新の画像を読み込みます。
				@details 画像へのバッファと画像解像度を返します。
				@param[out] 横解像度
				@param[out] 縦解像度
				@param[out] rowBytes １ラインあたりのバイト数
				@return 成功した場合画像バッファへのポインタが返されます。(デリートしないでください。読み込み対応のみです。)
				@note 本関数はスレッドセーフです。
		*/
		unsigned char* read(int& width, int& height, int& rowBytes)
		{
			int index = 0;
			if (hDevice == NULL)
				return NULL;
			if (m_frameSampleRate[index] == 0)
				return NULL;

			int readBuffer = m_readBuffer[index];
			if (m_isSingleThread)
			{
				
				result = PUC_GetSingleXferData(hDevice, &xferData);
#ifdef USE_DECODE_MULITHRREAD
				result = PUC_DecodeDataMultiThread(pDecodeBuf[readBuffer], 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q, m_numDecodeThreads);
#else
				result = PUC_DecodeData(pDecodeBuf[readBuffer], 0, 0, nWidth, nHeight, nLineBytes, xferData.pData, q);
#endif
				nSequenceNo[index] = xferData.nSequenceNo;
			}
			else
			{
				int copyBuffer = 2;

#if 0
				// Block for a while
				if (nReadSequenceNo[index] == nSequenceNo[index]) {
					int numTries = 0;
					while (numTries < 100000000) {
						if (nReadSequenceNo[index] != nSequenceNo[index])
							break;
						numTries++;
					}
				}
#endif

				std::lock_guard<std::mutex> guard(m_mutex);
				readBuffer = m_readBuffer[index];
				
				memcpy(pDecodeBuf[copyBuffer], pDecodeBuf[readBuffer], int(nLineBytes) * int(nHeight));

				readBuffer = copyBuffer;
			}
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_DecodeData error";
				return NULL;
			}
			width = nWidth;
			height = nHeight;
			rowBytes = nLineBytes;
			nReadSequenceNo[index] = nSequenceNo[index];

			return pDecodeBuf[readBuffer];
		}

		/*!
			@~english
				@brief This sets the device resolution.
				@details Opening the device will reset this setting.
				@param[in] nWidth Horizontal resolution
				@param[in] nHeight Vertical resolution
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの解像度を設定します。
				@details デバイスのオープン時に設定はリセットされます。
				@param[in] nWidth 横解像度
				@param[in] nHeight 縦解像度
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setResolution(UINT32 nWidth, UINT32 nHeight) {
			if (hDevice == NULL) {
				m_resolutionWidth = nWidth;
				m_resolutionHeight = nHeight;
				return PUC_SUCCEEDED;
			}
			PUCRESULT result = PUC_SetResolution(hDevice, nWidth, nHeight);
			if (result != PUC_SUCCEEDED)
				return result;
			result = setupDataBuffer();
			return result;
		}

		/*!
			@~english
				@brief This sets the framerate and the shutter speed (1/fps) for the device.
				@details Opening the device will reset this setting. @n When executing this function, the return value of PUC_GetExposeTime will change. @n Changing the frame rate resets the output magnification rate of synchronization signal to x1.
				@param[in] nFramerate The framerate
				@param[in] nShutterSpeedFps The shutter speed (1/fps)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの撮影速度およびシャッター速度(1/fps)を設定します。
				@details デバイスのオープン時に設定はリセットされます。@n本関数を実行すると、PUC_GetExposeTimeで返却される値も変更されます。@nフレームレートを変えると同期信号の出力倍率はx1倍に戻ります。
				@param[in] nFramerate 撮影速度
				@param[in] nShutterSpeedFps シャッター速度(1/fps)
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setFramerateShutter(UINT32 nFramerate, UINT32 nShutterSpeedFps) {
			if (hDevice == NULL) {
				m_frameRate = nFramerate;
				m_shutterSpeedFps = nShutterSpeedFps;
				return PUC_SUCCEEDED;
			}
			return PUC_SetFramerateShutter(hDevice, nFramerate, nShutterSpeedFps);
		}

		/*!
		@~english
			@brief This retrieves the framerate and the shutter speed (1/fps) for the device.
			@details Note that the return value with this function will be invalid if the exposure/non-exposure time is set directly with PUC_SetExposeTime function.
			@param[out] pFramerate The storage destination for the framerate
			@param[out] pShutterSpeedFps The storage destination for the shutter speed (1/fps)
			@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
			@note This function is thread-safe.
			@see PUC_SetFramerateShutter
			@see PUC_SetExposeTime
		@~japanese
			@brief デバイスの撮影速度およびシャッター速度(1/fps)を取得します。
			@details PUC_SetExposeTime関数により露光・非露光期間を直接設定した場合、本関数で返される値は不正な値になりますのでご注意ください。
			@param[out] pFramerate 撮影速度の格納先
			@param[out] pShutterSpeedFps シャッター速度(1/fps)の格納先
			@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
			@note 本関数はスレッドセーフです。
			@see PUC_SetFramerateShutter
			@see PUC_SetExposeTime
		*/
		PUCRESULT getFramerateShutter(UINT32* pFramerate, UINT32* pShutterSpeedFps) {
			if (hDevice == NULL) {
				*pFramerate = m_frameRate;
				*pShutterSpeedFps = m_shutterSpeedFps;
				return PUC_SUCCEEDED;
			}
			return PUC_GetFramerateShutter(hDevice, pFramerate, pShutterSpeedFps);
		}



		/*!
			@~english
				@brief This overwrites one set of quantization table data stored to the device.
				@details The number of quantization tables is defined in PUC_Q_COUNT. @n Restarting the device will reset this setting.
				@param[in] nPoint The position of the quantization table
				@param[in] nVal The quantization table data being set
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
				@see PUC_GetQuantization
			@~japanese
				@brief デバイスに格納されている量子化テーブルデータを１つ書き換えます。
				@details 量子化テーブルの個数はPUC_Q_COUNTで定義されています。@nデバイスを再起動すると設定はリセットされます。
				@param[in] nPoint 量子化テーブルの位置
				@param[in] nVal 設定する量子化テーブルデータ
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
				@see PUC_GetQuantization
		*/
		PUCRESULT setQuantization(UINT32 nPoint, USHORT nVal) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetQuantization(hDevice, nPoint, nVal);
		}

		/*!
			@~english
				@brief This sets the state of the device fan.
				@details Restarting the device will reset this setting.
				@param[in] nState The fan state (ON/OFF)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスのファンの状態を設定します。
				@param[in] hDevice 操作対象のデバイスハンドル
				@param[in] nState ファン状態（ON／OFF）
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setFanState(PUC_MODE nState) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetFanState(hDevice, nState);
		}

		/*!
			@~english
				@brief This sets the synchronous signal input mode for the device.
				@details Restarting the device will reset this setting. @n Changing the synchronization signal output mode resets the output magnification rate to x1.
						 @n When setting to external device synchronization, the exposure time is automatically adjusted considering the variation of external devices.
						 Set the exposure time and framerate before setting the external device synchronization.
				@param[in] nMode The synchronous signal input mode (Internal/External)
				@param[in] nSignal Polarity (positive/negative), Specifying the polarity is not supported for devices prior to version 1.01.
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの同期信号入力モードを設定します。
				@details デバイスを再起動すると設定はリセットされます。@n同期信号入力モードを変えると出力倍率はx1倍に戻ります。
						 @n外部機器同期設定時、外部機器のばらつきを考慮した露光時間へ自動で調整されます。あらかじめ撮影したい露光時間と撮影速度に変更してから、外部機器同期を設定してください。
				@param[in] nMode 同期信号入力モード（Internal／External）
				@param[in] nSignal 極性（正極性／負極性）、極性の指定はバージョン1.01以前のデバイスでは対応していません。
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setSyncInMode(PUC_SYNC_MODE nMode, PUC_SIGNAL nSignal) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetSyncInMode(hDevice, nMode, nSignal);
		}

		/*!
			@~english
				@brief This sets the synchronous signal output polarity for the device.
				@details Restarting the device will reset this setting.
				@param[in] nSignal The polarity (positive polarity/negative polarity)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの同期信号出力の極性を設定します。
				@details デバイスを再起動すると設定はリセットされます。
				@param[in] nSignal 極性（正極性／負極性）
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setSyncOutSignal(PUC_SIGNAL nSignal) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetSyncOutSignal(hDevice, nSignal);
		}

		/*!
			@~english
				@brief This sets the delay value of synchronization signal output for the device.
				@details Restarting the device will reset this setting.
				@param[in] nDelay The delay (clock units)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの同期信号出力の遅延量を設定します。
				@details デバイスを再起動すると設定はリセットされます。
				@param[in] nDelay 遅延量(クロック単位)
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setSyncOutDelay(UINT32 nDelay) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetSyncOutDelay(hDevice, nDelay);
		}

		/*!
			@~english
				@brief This sets the output width of synchronization signal for the device.
				@details Restarting the device will reset this setting. @n Changing the output width resets the output magnification rate to x1.
				@param[in] nWidth The output width of the synchronous signal (clock units).
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの同期信号出力の出力幅を設定します。
				@details デバイスを再起動すると設定はリセットされます。@n出力幅を変えると出力倍率はx1倍に戻ります。
				@param[in] nWidth 同期信号の出力幅(クロック単位)。
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setSyncOutWidth(UINT32 nWidth) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetSyncOutWidth(hDevice, nWidth);
		}

		/*!
			@~english
				@brief This sets the output magnification rate for the synchronization signal.
				@details Opening the device will reset this setting. @n Changing the frame rate or exposure/non-exposure time also resets the output magnification rate to x1.
				@param[in] nMagnification The output magnification (e.g.: for 2x set to 2, for 4x set to 4). For 0.5x, set PUC_SYNC_OUT_MAGNIFICATION_0_5.
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief 同期信号の出力倍率を設定します。
				@details デバイスのオープン時に設定はリセットされます。@n撮影速度や露光・非露光期間を変更した場合もx1倍にリセットされます。
				@param[in] nMagnification 出力倍率（例：x2の場合は2、x4の場合は4）0.5倍の場合はPUC_SYNC_OUT_MAGNIFICATION_0_5を指定
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setSyncOutMagnification(UINT32 nMagnification) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetSyncOutMagnification(hDevice, nMagnification);
		}

		/*!
			@~english
				@brief This sets the LED state of the device.
				@details Restarting the device will reset this setting.
				@param[in] nMode The LED state (ON/OFF)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスのLED状態を設定します。
				@details デバイスを再起動すると設定はリセットされます。
				@param[in] nMode LEDの状態（ON／OFF）
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setLEDMode(PUC_MODE nMode) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetLEDMode(hDevice, nMode);
		}

		/*!
			@~english
				@brief This sets the timeout duration (ms) for data transfer from the device.
				@details Opening the device will reset this setting. @n
					Set PUC_XFER_TIMEOUT_AUTO to automatically adjust the timeout duration based on the framerate. @n
					Set PUC_XFER_TIMEOUT_INFINITE to disable timeout.
				@param[in] nSingleXferTimeOut The timeout duration (ms) for a single transfer
				@param[in] nContinuousXferTimeOut The timeout duration (ms) for continuous transfer
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの転送時のタイムアウト時間(ms)を設定します。
				@details デバイスのオープン時に設定はリセットされます。@n
					PUC_XFER_TIMEOUT_AUTOを指定すると、撮影速度に応じて自動でタイムアウトを調整します。@n
					PUC_XFER_TIMEOUT_INFINITEを指定すると、タイムアウトはなくなります。
				@param[in] nSingleXferTimeOut シングル転送のタイムアウト時間(ms)
				@param[in] nContinuousXferTimeOut 連続転送のタイムアウト時間(ms)
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setXferTimeOut(UINT32 nSingleXferTimeOut, UINT32 nContinuousXferTimeOut) {
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetXferTimeOut(hDevice, nSingleXferTimeOut, nContinuousXferTimeOut);
		}

		/*!
			@~english
				@brief This sets the exposure/non-exposure time of the device.
				@details Opening the device will reset this setting. @n Note that the return value of PUC_GetFramerateShutter function will be invalid if the exposure/non-exposure time is set directly with this function.
				@param[in] nExpOnClk The exposure period (clock units)
				@param[in] nExpOffClk The non-exposure period (clock units)
				@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
				@note This function is thread-safe.
			@~japanese
				@brief デバイスの露光・非露光期間を設定します。
				@details デバイスのオープン時に設定はリセットされます。@n本関数により露光・非露光期間を直接設定した場合、PUC_GetFramerateShutter関数で返される値は不正な値になります。
				@param[in] nExpOnClk 露光期間（クロック単位）
				@param[in] nExpOffClk 非露光期間（クロック単位）
				@return 成功時はPUC_SUCCEEDED、失敗時はそれ以外が返ります。
				@note 本関数はスレッドセーフです。
		*/
		PUCRESULT setExposeTime(UINT32 nExpOnClk, UINT32 nExpOffClk)
		{
			if (hDevice == NULL)
				return PUC_ERROR_DEVICE_NOTOPEN;
			return PUC_SetExposeTime(hDevice, nExpOnClk, nExpOffClk);
		}

		/*!
			@~english
				@brief Saves image to a BMP file
				@details Utility function to save the image buffer to a filen
				@param[in] fileName The output file name
				@param[in] data The base address of the buffer image
				@param[in] width The width of the buffer image
				@param[in] height The height of the buffer image
				@param[in] rowBytes The number bytes per row of the buffer image
				@note This function is thread-safe.
			@~japanese
				@brief 画像をBMPファイルに保存します。
				@details 画像バッファをファイルに保存するためのユーティリティ関数です。
				@param[in] fileName 出力ファイル名
				@param[in] data 画像バッファの先頭アドレス
				@param[in] width 画像バッファの横解像度
				@param[in] height 画像バッファの縦解像度
				@param[in] rowBytes １ラインあたりのバイト数
				@note 本関数はスレッドセーフです。
		*/
		static void saveBitmap(const char* fileName, unsigned char* data, int width, int height, int rowBytes)
		{
			UINT32 nInfoBytes;
			BITMAPINFO* pBitmapInfo;
			DWORD nPixelBytes;
			BITMAPFILEHEADER fileHeader;
			FILE* fp;

			nInfoBytes = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256);
			pBitmapInfo = (BITMAPINFO*)new BYTE[nInfoBytes];
			nPixelBytes = rowBytes * height;

			memset(pBitmapInfo, 0, nInfoBytes);
			pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			pBitmapInfo->bmiHeader.biWidth = width;
			pBitmapInfo->bmiHeader.biHeight = -(INT32)height;
			pBitmapInfo->bmiHeader.biPlanes = 1;
			pBitmapInfo->bmiHeader.biBitCount = 8;
			pBitmapInfo->bmiHeader.biCompression = BI_RGB;
			pBitmapInfo->bmiHeader.biSizeImage = nPixelBytes;
			pBitmapInfo->bmiHeader.biClrUsed = 256;

			for (int i = 0; i < 256; i++)
			{
				pBitmapInfo->bmiColors[i].rgbRed = (BYTE)i;
				pBitmapInfo->bmiColors[i].rgbGreen = (BYTE)i;
				pBitmapInfo->bmiColors[i].rgbBlue = (BYTE)i;
			}

			memset(&fileHeader, 0, sizeof(fileHeader));
			fileHeader.bfType = ('M' << 8) + 'B';
			fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + nInfoBytes + nPixelBytes;
			fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + nInfoBytes;

			fp = fopen(fileName, "wb");
			fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
			fwrite(pBitmapInfo, nInfoBytes, 1, fp);
			fwrite(data, nPixelBytes, 1, fp);
			fclose(fp);

			delete[] pBitmapInfo;
		}

		/*!
			@~english
				@brief Get resolution of camera images
				@details Utility function to get resolution of camera images
				@param[out] width The width of the buffer image
				@param[out] height The height of the buffer image
				@note This function is thread-safe.
			@~japanese
				@brief カメラ画像の解像度取得
				@details カメラ画像の解像度を取得する関数
				@param[out] width バッファ画像の幅
				@param[out] height バッファ画像の高さ
				@note 本関数はスレッドセーフです。
		*/
		void getResolution(int& width, int& height) {
			width = m_resolutionWidth;
			height = m_resolutionHeight;
		}

		PUC_HANDLE getPUCHandle() {
			return hDevice;
		}

		USHORT getFullSequenceNumber() const {
			return nReadSequenceNo[0];
		}
		USHORT getProxySequenceNumber() const {
			return nReadSequenceNo[1];
		}

		/*!
			@~english
				@brief Reads the latest proxy image from the camera
				@details Returns the buffer to the image and the image resolution
				@param[out] width of the image
				@param[out] height of the image
				@param[out] rowBytes, number of bytes per row
				@return If successful, a pointer to the image buffer is returned (do not delete it, just read it)
				@note This function is thread-safe.
			@~japanese
				@brief カメラから最新の画像を読み込みます。
				@details 画像へのバッファと画像解像度を返します。
				@param[out] 横解像度
				@param[out] 縦解像度
				@param[out] rowBytes １ラインあたりのバイト数
				@return 成功した場合画像バッファへのポインタが返されます。(デリートしないでください。読み込み対応のみです。)
				@note 本関数はスレッドセーフです。
		*/
		unsigned char* readProxy(int& width, int& height, int& rowBytes)
		{
			int index = 1;
			if (hDevice == NULL)
				return NULL;
			if (m_frameSampleRate[index]==0)
				return NULL;

			int readBuffer = m_readBuffer[index];
			if (m_isSingleThread)
			{
				result = PUC_GetSingleXferData(hDevice, &xferData);
				result = PUC_DecodeDCData(pDecodeBufProxy[readBuffer], 0, 0, nBlockCountX, nBlockCountY, xferData.pData);
				nSequenceNo[index] = xferData.nSequenceNo;
			}
			else
			{
#if 0
				// Block for a while
				if (nReadSequenceNo[index] == nSequenceNo[index]) {
					int numTries = 0;
					while (numTries < 100000000) {
						if (nReadSequenceNo[index] != nSequenceNo[index])
							break;
						numTries++;
					}
				}
#endif

				int copyBuffer = 2;
				std::lock_guard<std::mutex> guard(m_mutex);
				readBuffer = m_readBuffer[index];

				memcpy(pDecodeBufProxy[copyBuffer], pDecodeBufProxy[readBuffer], int(nBlockCountX) * int(nBlockCountY));

				readBuffer = copyBuffer;
			}
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_DecodeData error";
				return NULL;
			}
			width = nBlockCountX;
			height = nBlockCountY;
			rowBytes = nBlockCountX;
			nReadSequenceNo[index] = nSequenceNo[index];

			return pDecodeBufProxy[readBuffer];
		}

		void setFrameSampleRate(int fullRate, int proxyRate) {
			std::lock_guard<std::mutex> guard(m_mutexSampleRate);
			m_frameSampleRate[0] = fullRate;
			m_frameSampleRate[1] = proxyRate;
		}


	private:

		static void receive(PPUC_XFER_DATA_INFO info, void* userData) {

			PUCLib_Wrapper* that = (PUCLib_Wrapper*)userData;
			PUINT8 pData = info->pData;
			UINT32 nDataSize = info->nDataSize;
			USHORT nSequenceNo = info->nSequenceNo;
			if (that->listener) {
				that->result = PUC_DecodeDataMultiThread(that->pDecodeBuf[0], 0, 0, that->nWidth, that->nHeight, that->nLineBytes, pData, that->q, that->m_numDecodeThreads);
				that->listener->imageReady(that->pDecodeBuf[0], that->nWidth, that->nHeight, that->nLineBytes, nSequenceNo);
				return;
			}


			//if (nSequenceNo == that->nSequenceNo[1])
			//	return;
			int drawBuffer[2];
			std::lock_guard<std::mutex> guard(that->m_mutexSampleRate);
			int frameSampleRate[2];
			{
				std::lock_guard<std::mutex> guard(that->m_mutex);
				drawBuffer[0] = 1 - that->m_readBuffer[0];
				drawBuffer[1] = 1 - that->m_readBuffer[1];
				frameSampleRate[0] = that->m_frameSampleRate[0];
				frameSampleRate[1] = that->m_frameSampleRate[1];
			}

			bool readFull = (frameSampleRate[0]!=0) && that->counter % frameSampleRate[0] == 0;
			if(readFull)
			{
#ifdef USE_DECODE_MULITHRREAD
				that->result = PUC_DecodeDataMultiThread(that->pDecodeBuf[drawBuffer[0]], 0, 0, that->nWidth, that->nHeight, that->nLineBytes, pData, that->q, that->m_numDecodeThreads);
#else
				that->result = PUC_DecodeData(that->pDecodeBuf[drawBuffer[0]], 0, 0, that->nWidth, that->nHeight, that->nLineBytes, pData, that->q);
#endif
				that->nSequenceNo[0] = nSequenceNo;
			}

			bool readProxy = (frameSampleRate[1] != 0) && that->counter % frameSampleRate[1] == 0;
			if (readProxy)
			{
				that->result = PUC_DecodeDCData(that->pDecodeBufProxy[drawBuffer[1]], 0, 0, that->nBlockCountX, that->nBlockCountY, pData);
				that->nSequenceNo[1] = nSequenceNo;
			}


			that->swapBuffer(readFull, readProxy);

			++that->counter;

		}

		USHORT nSequenceNo[2] = { 0, 0 };
		PUC_HANDLE hDevice = NULL;
		UINT32 nDataSize = 0;
		PUC_XFER_DATA_INFO xferData = { 0 };
		UINT32 nWidth, nHeight, nLineBytes;
		USHORT q[PUC_Q_COUNT];
		UINT8* pDecodeBuf[3] = { NULL,NULL,NULL };
		PUCRESULT result = PUC_SUCCEEDED;
		std::string m_lastErrorName = "";
		int m_resolutionWidth = 1246;
		int m_resolutionHeight = 800;
		int m_frameRate = 1000;
		int m_shutterSpeedFps = 2000;
		USHORT nReadSequenceNo[2] = { 0,0 };
		int m_readBuffer[2] = { 0, 0 };
		std::mutex m_mutex;
		std::mutex m_mutexSampleRate;
		UINT8* pDecodeBufProxy[3] = { NULL,NULL,NULL };
		UINT32 nBlockCountX, nBlockCountY;
		int m_frameSampleRate[2] = { 1, 0 };
		int counter = 0;
		PUCLib_WrapperImageListener* listener = nullptr;

		void cleanupBuffer() {
			if (!m_isSingleThread) {
				result = PUC_EndXferData(hDevice);
			}

			if (xferData.pData)
				delete[] xferData.pData;
			if (pDecodeBuf[0])
				delete[] pDecodeBuf[0];
			if (pDecodeBuf[1])
				delete[] pDecodeBuf[1];
			if (pDecodeBuf[2])
				delete[] pDecodeBuf[2];
			if (pDecodeBufProxy[0])
				delete[] pDecodeBufProxy[0];
			if (pDecodeBufProxy[1])
				delete[] pDecodeBufProxy[1];
			if (pDecodeBufProxy[2])
				delete[] pDecodeBufProxy[2];

			xferData.pData = NULL;
			pDecodeBuf[0] = NULL;
			pDecodeBuf[1] = NULL;
			pDecodeBuf[2] = NULL;
			pDecodeBufProxy[0] = NULL;
			pDecodeBufProxy[1] = NULL;
			pDecodeBufProxy[2] = NULL;
		}

		void swapBuffer(bool updateFull, bool updateProxy) {
			// Swap Buffers
			std::lock_guard<std::mutex> guard(m_mutex);
			if(updateFull)
				m_readBuffer[0] = 1 - m_readBuffer[0];
			if (updateProxy)
				m_readBuffer[1] = 1 - m_readBuffer[1];
		}

		PUCRESULT setupDataBuffer() {
			cleanupBuffer();

			PUCRESULT result = PUC_SUCCEEDED;
			
			if (m_isSingleThread) {
				result = PUC_GetXferDataSize(hDevice, &nDataSize);
				xferData.pData = new UINT8[nDataSize];
				result = PUC_GetSingleXferData(hDevice, &xferData);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_GetSingleXferData error";
					goto EXIT_LABEL;
				}
			}
			
			result = PUC_GetResolution(hDevice, &nWidth, &nHeight);
			if (PUC_CHK_FAILED(result))
			{
				m_lastErrorName = "PUC_GetResolution error";
				goto EXIT_LABEL;
			}

			for (UINT32 i = 0; i < PUC_Q_COUNT; i++)
			{
				result = PUC_GetQuantization(hDevice, i, &q[i]);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_GetQuantization error";
					goto EXIT_LABEL;
				}
			}

			nLineBytes = nWidth % 4 == 0 ? nWidth : nWidth + (4 - nWidth % 4);
			pDecodeBuf[0] = new UINT8[nLineBytes * nHeight];
			pDecodeBuf[1] = new UINT8[nLineBytes * nHeight];
			pDecodeBuf[2] = new UINT8[nLineBytes * nHeight];

			
			nBlockCountX = nWidth % 8 == 0 ? nWidth / 8 : (nWidth + (8 - nWidth % 8)) / 8;
			nBlockCountY = nHeight % 8 == 0 ? nHeight / 8 : (nHeight + (8 - nHeight % 8)) / 8;
			pDecodeBufProxy[0] = new UINT8[nBlockCountX * nBlockCountY];
			pDecodeBufProxy[1] = new UINT8[nBlockCountX * nBlockCountY];
			pDecodeBufProxy[2] = new UINT8[nBlockCountX * nBlockCountY];


			if (!m_isSingleThread) {
				result = PUC_BeginXferData(hDevice, PUCLib_Wrapper::receive, this);
			}

			return result;

		EXIT_LABEL:
			if (hDevice)
			{
				cleanupBuffer();

				result = PUC_CloseDevice(hDevice);
				if (PUC_CHK_FAILED(result))
				{
					m_lastErrorName = "PUC_CloseDevice error";
				}
				hDevice = NULL;
			}
			return result;
		}


	};

}
