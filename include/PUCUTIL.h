#ifndef __PUCUTIL_H_
#define __PUCUTIL_H_

/*
 *	PUCUTIL.h
 *	PHOTRON INFINICAM Control SDK
 *
 *	Copyright (C) 2022 PHOTRON LIMITED
 */

#include "PUCCONST.h"


#ifdef PUCUTIL_EXPORTS
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#ifndef DLLAPI
#define DLLAPI extern "C"
#endif

 /*!
 @struct PUC_GPU_SETUP_PARAM
 @~english  @brief
 @~japanese @brief GPU�f�R�[�h�Ŏg�p����p�����[�^���i�[����\����
 */
struct PUC_GPU_SETUP_PARAM
{
	/*! @~english  @brief
		@~japanese @brief GPU�����ň����摜�̉𑜓x���� */
	UINT32 width;

	/*! @~english  @brief
		@~japanese @brief GPU�����ň����摜�̉𑜓x���� */
	UINT32 height;
};


#ifdef __cplusplus
namespace pucutil
{
extern "C" {
#endif



/*!
	@~english
		@brief This extracts the sequence number from the compressed image data.
		@param[in] pData The compressed image data
		@param[in] nWidth The image width
		@param[in] nHeight The image height
		@param[out] pSeqNo The storage destination for the sequence number extracted
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe. This function can be executed in parallel.
	@~japanese
		@brief ���k�摜�f�[�^����V�[�P���X�ԍ��𒊏o���܂��B
		@param[in] pData ���k�摜�f�[�^
		@param[in] nWidth �摜�̉���
		@param[in] nHeight �摜�̍���
		@param[out] pSeqNo ���o�����V�[�P���X�ԍ��̊i�[��
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
		@note �{�֐��̓X���b�h�Z�[�t�ł��B�{�֐��͕�����s���\�ł��B
*/
DLL_EXPORT PUCRESULT WINAPI ExtractSequenceNo(const PUCHAR pData, UINT32 nWidth, UINT32 nHeight, PUSHORT pSeqNo);

/*!
	@~english
		@brief This unpacks the compressed image data to luminance data.
		@param[out] pDst The buffer at the unpacking destination. The size of the width must be allocated rounded up to a multiple of four. (e.g., If the width is 1246 px, a buffer is required 1248 bytes at least)
		@param[in] nX The upper left coordinate X for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nY The upper left coordinate Y for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nWidth The width for unpacking
		@param[in] nHeight The height for unpacking
		@param[in] nLineBytes The number of bytes of the buffer width at the unpacking destination
		@param[in] pSrc The compressed image data
		@param[in] pQVals A quantization table
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe. This function can be executed in parallel.
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
	@~japanese
		@brief ���k�摜�f�[�^���P�x�l�f�[�^�ɓW�J���܂��B
		@param[out] pDst �W�J��o�b�t�@�B������4�̔{���ɐ؂�グ���T�C�Y���m�ۂ���Ă���K�v������܂��B�i��F������1246px�̏ꍇ�A�o�b�t�@��1248�o�C�g�m�ۂ���Ă���K�v����j
		@param[in] nX �W�J�J�n���鍶����WX�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nY �W�J�J�n���鍶����WY�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nWidth �W�J���鉡��
		@param[in] nHeight �W�J���鍂��
		@param[in] nLineBytes �W�J��o�b�t�@�̉����̃o�C�g��
		@param[in] pSrc ���k�摜�f�[�^
		@param[in] pQVals �ʎq���e�[�u��
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
		@note �{�֐��̓X���b�h�Z�[�t�ł��B�{�֐��͕�����s���\�ł��B
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
*/
DLL_EXPORT PUCRESULT WINAPI DecodeData(PUINT8 pDst, UINT32 nX, UINT32 nY, UINT32 nWidth, UINT32 nHeight, UINT32 nLineBytes, const PUINT8 pSrc, const PUSHORT pQVals);

/*!
	@~english
		@brief This unpacks the compressed image data to luminance data. This process is multithreaded.
		@param[out] pDst The buffer at the unpacking destination. The size of the width must be allocated rounded up to a multiple of four. (e.g., If the width is 1246 px, a buffer is required 1248 bytes at least)
		@param[in] nX The upper left coordinate X for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nY The upper left coordinate Y for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nWidth The width for unpacking
		@param[in] nHeight The height for unpacking
		@param[in] nLineBytes The number of bytes of the buffer width at the unpacking destination
		@param[in] pSrc The compressed image data
		@param[in] pQVals A quantization table
		@param[in] nThreadCount The number of threads to process in multiple threads.
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe. This function can be executed in parallel.
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
		@see PUC_DecodeData
	@~japanese
		@brief ���k�摜�f�[�^���P�x�l�f�[�^�ɓW�J���܂��B���̃f�R�[�h�����̓}���`�X���b�h�ōs���܂��B
		@param[out] pDst �W�J��o�b�t�@�B������4�̔{���ɐ؂�グ���T�C�Y���m�ۂ���Ă���K�v������܂��B�i��F������1246px�̏ꍇ�A�o�b�t�@��1248�o�C�g�m�ۂ���Ă���K�v����j
		@param[in] nX �W�J�J�n���鍶����WX�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nY �W�J�J�n���鍶����WY�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nWidth �W�J���鉡��
		@param[in] nHeight �W�J���鍂��
		@param[in] nLineBytes �W�J��o�b�t�@�̉����̃o�C�g��
		@param[in] pSrc ���k�摜�f�[�^
		@param[in] pQVals �ʎq���e�[�u��
		@param[in] nThreadCount �}���`�X���b�h�ŏ�������X���b�h��
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
		@note �{�֐��̓X���b�h�Z�[�t�ł��B�{�֐��͕�����s���\�ł��B
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
		@see PUC_DecodeData
*/
DLL_EXPORT PUCRESULT WINAPI DecodeDataMultiThread(PUINT8 pDst, UINT32 nX, UINT32 nY, UINT32 nWidth, UINT32 nHeight, UINT32 nLineBytes, const PUINT8 pSrc, const PUSHORT pQVals, UINT32 nThreadCount);

/*!
	@~english
		@brief This unpacks the compressed image data to DCT coefficients.
		@param[out] pDst The buffer at the unpacking destination. The size of the width must be allocated rounded up to a multiple of four. (e.g., If the width is 1246 px, a buffer is required 1248 bytes at least)
		@param[in] nX The upper left coordinate X for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nY The upper left coordinate Y for starting unpacking. This must be 0, or a multiple of 8.
		@param[in] nWidth The width for unpacking
		@param[in] nHeight The height for unpacking
		@param[in] nLineBytes The number of bytes of the buffer width at the unpacking destination
		@param[in] pSrc The compressed image data
		@param[in] pQVals A quantization table
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe. This function can be executed in parallel.
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
	@~japanese
		@brief ���k�摜�f�[�^��DCT�W���ɓW�J���܂��B
		@param[out] pDst �W�J��o�b�t�@�B������4�̔{���ɐ؂�グ���T�C�Y���m�ۂ���Ă���K�v������܂��B�i��F������1246px�̏ꍇ�A�o�b�t�@��1248�o�C�g�m�ۂ���Ă���K�v����j
		@param[in] nX �W�J�J�n���鍶����WX�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nY �W�J�J�n���鍶����WY�B0��������8�̔{���ł���K�v������܂��B
		@param[in] nWidth �W�J���鉡��
		@param[in] nHeight �W�J���鍂��
		@param[in] nLineBytes �W�J��o�b�t�@�̉����̃o�C�g��
		@param[in] pSrc ���k�摜�f�[�^
		@param[in] pQVals �ʎq���e�[�u��
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
		@note �{�֐��̓X���b�h�Z�[�t�ł��B�{�֐��͕�����s���\�ł��B
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
*/
DLL_EXPORT PUCRESULT WINAPI DecodeDCTData(PINT16 pDst, UINT32 nX, UINT32 nY, UINT32 nWidth, UINT32 nHeight, UINT32 nLineBytes, const PUINT8 pSrc, const PUSHORT pQVals);

/*!
	@~english
		@brief Decodes the DC component of compressed image data.
		@param[out] pDst The buffer at the decoding destination. Must be allocated for the total number of blocks included in the decoding range.
		@param[in] nBlockX The block coordinates X for starting decoding
		@param[in] nBlockY The block coordinates Y for starting decoding
		@param[in] nBlockCountX Number of blocks in the X direction to be decoded
		@param[in] nBlockCountY Number of blocks in the Y direction to be decoded
		@param[in] pSrc The compressed image data
		@return If successful, PUC_SUCCEEDED will be returned. If failed, other responses will be returned.
		@note This function is thread-safe. This function can be executed in parallel.
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
	@~japanese
		@brief ���k�摜�f�[�^��DC������W�J���܂��B
		@param[out] pDst �W�J��o�b�t�@�B�f�R�[�h�͈͂Ɋ܂܂��u���b�N�̑��������m�ۂ���K�v������܂��B
		@param[in] nBlockX �W�J�J�n����u���b�N���WX�B
		@param[in] nBlockY �W�J�J�n����u���b�N���WY�B
		@param[in] nBlockCountX �W�J����X�����̃u���b�N��
		@param[in] nBlockCountY �W�J����Y�����̃u���b�N��
		@param[in] pSrc ���k�摜�f�[�^
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
		@note �{�֐��̓X���b�h�Z�[�t�ł��B�{�֐��͕�����s���\�ł��B
		@see PUC_GetXferDataSize
		@see PUC_GetMaxXferDataSize
*/
DLL_EXPORT PUCRESULT WINAPI DecodeDCData(PUINT8 pDst, UINT32 nBlockX, UINT32 nBlockY, UINT32 nBlockCountX, UINT32 nBlockCountY, const PUINT8 pSrc);

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief PC��GPU�����\�����擾���܂��B
		@return GPU�����\�ł����PUC_SUCEEDED, �s�\�ł����PUC_ERROR_NOTSUPPORT���Ԃ�܂��B
*/
DLL_EXPORT PUCRESULT WINAPI GetAvailableGPUProcess();

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief GPU�����Ŏg�p���郁�������m�ۂ��܂��B
		@param[in] param �������p�����[�^�ł��B
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
*/
DLL_EXPORT PUCRESULT WINAPI SetupGPUDecode(PUC_GPU_SETUP_PARAM param);

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief GPU�����Ŏg�p������������������܂��B
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
        @note DecodeGPU�Ɏg�p�����o�b�t�@�������Ɏ��s���Ă���肠��܂���B
*/
DLL_EXPORT PUCRESULT WINAPI TeardownGPUDecode();

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief ���k�摜�f�[�^���P�x�l�f�[�^�ɓW�J���܂��B(GPU�g�p)
		@param[in] download false���w�肵���ꍇ�f�R�[�h���ꂽ�f�[�^�̓f�o�C�X(GPU)�������ɕۑ�����Atrue�̏ꍇ�̓z�X�g(CPU)�������ɕۑ�����܂��B
		@param[in] pSrc �f�R�[�h�Ώۂ̃G���R�[�h���ꂽ���̃t���[���f�[�^�ł��B
		@param[in] pDst �f�R�[�h���ꂽ�������ʂ̃t���[���f�[�^�ł��Bdownload�����̐ݒ�ɂ���ăf�o�C�X�������܂��̓z�X�g�������ɏo�͂���܂��B
		@n download������true�̏ꍇ�AGPU�Ńf�R�[�h���ꂽ�f�[�^�����̈����Ŏw�肳�ꂽ�A�h���X�ɃR�s�[���܂��B���̂��ߎ��O�Ƀz�X�g�������̃o�b�t�@�̊m�ۂ��K�v�ł��B
		@n ������4�̔{���ɐ؂�グ���T�C�Y���m�ۂ���Ă���K�v������܂��B�i��F������1246px�̏ꍇ�A�o�b�t�@��1248�o�C�g�m�ۂ���Ă���K�v����j
		@n download������false�̏ꍇ�AGPU�Ńf�R�[�h���ꂽ�f�[�^�̃f�o�C�X�������̃A�h���X���擾���܂��B�z�X�g�������̊m�ۂ͕s�v�ł��B
		@return ��������PUC_SUCCEEDED�A���s ���͂���ȊO���Ԃ�܂��B
*/
DLL_EXPORT PUCRESULT WINAPI DecodeGPU(bool download, unsigned char* pSrc, unsigned char** pDst, UINT32 lineBytes);

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief �Ō�ɔ�������GPU�����ł̃G���[�R�[�h���擾���܂��B
		@param[in] errorCode �G���[�R�[�h�ł��B
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
*/
DLL_EXPORT PUCRESULT WINAPI GetGPULastError(int& errorCode);

/*!
	@~english
		@brief
		@return
	@~japanese
		@brief GPU�f�R�[�h�̃��������m�ۂ�����Ă��邩���擾���܂��B
		@param[in] status true�F�m�ۍς݁Afalse�F�m�ۂ���Ă��Ȃ�
		@return ��������PUC_SUCCEEDED�A���s���͂���ȊO���Ԃ�܂��B
*/
DLL_EXPORT PUCRESULT WINAPI IsSetupGPUDecode(bool& status);

#ifdef __cplusplus
} // extern C
} // namespace pucutil
#endif

#endif //__PUCUTIL_H
