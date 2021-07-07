/**
@file	ALPR_api.h
@date	Oct/24/2018
@author	Jihong Kang
@brief	Automobile License Plate Recognition 엔진 라이브러리의 api
*/
#pragma once
#include "opencv2/opencv.hpp"
#include "LPResult.h"
#include "LPDataInfo.h"
#include <vector>
#include <string>
#include <memory>

/** 
@class ALPR_api
@brief ALPR library's API class.
*/
class ALPR_api
{
public:

	ALPR_api();

	/**
	* A constructor.
	* @param 	max_batchsize maximum batchsize that can process at once
	* @gpu_id	GPU ID to use
	*/
	ALPR_api(int max_batchsize=1, int gpu_id=0);

	/**
	* A destructor.
	*/
	~ALPR_api();
	

	/**
	* 인자로 주어진 영상에서 번호판을 찾고 인식 결과를 리턴하는 함수.
	* @param imgInfoList LPDataInfo 형식으로 저장된 인식 대상 영상들의 벡터 리스트. 벡터의 크기는 (1~MaxBatchSize) 사이의 수이어야 한다.
						 각 LPDataInfo에는 cvimage에 저장된 cv::Mat 형식의 이미지 또는 
						 rawData에 저장된 raw이미지의 포인터와 rawWidth, rawHeight, imgType가 저장되어 있어야 한다.
	* @see LPResult class
	* @return 인식된 번호판 결과(outer vector size = 입력 영상 수, LPResult vector size=단일 영상에서 검출된 번호판 수)
	*/
	std::vector<std::vector<LPResult>> RecognizeLP(std::vector<LPDataInfo> imgInfoList);

	/**
	* 인자로 주어진 영상에서 번호판 위치를 검출하여 리턴하는 함수. 
	* @param imgInfoList LPDataInfo 형식으로 저장된 인식 대상 영상들의 벡터 리스트. 벡터의 크기는 (1~MaxBatchSize) 사이의 수이어야 한다.
						 각 LPDataInfo에는 cvimage에 저장된 cv::Mat 형식의 이미지 또는 
						 rawData에 저장된 raw이미지의 포인터와 rawWidth, rawHeight, imgType가 저장되어 있어야 한다.
	* @return 검출된 번호판 결과(outer vector size = 입력 영상 수, LPResult vector size=단일 영상에서 검출된 번호판 수)
	*/
	std::vector<std::vector<cv::Rect>> FindLPPosition(std::vector<LPDataInfo> imgInfoList);
	
	/**
	* return SW license status
	*/
	int GetSWLicenseStatus();

	/**
	* return the library's version. Ex: "3.0"
	*/
	std::string GetVersionString();

protected:

	/**
	* 번호판 인식 엔진을 초기화. 생성자에서 호출됨.
	* @see m_bInitialized
	* @return ALPR 엔진 초기화 성공 여부를 리턴.
	*/
	bool Initialize(int max_batchsize, int gpu_id);

	/**
	* 엔진 초기화 여부 플래그
	*/
	bool m_bInitialized;

	//Indicate the sw license status
	int m_nSWLicenseStatus;	

	//SW license status code
	const int SW_VALID = 0;
	const int SW_MAC_CHECK_FAIL = 1;
	const int SW_SDCARD_CHECK_FAIL = 2;
	const int SW_FILE_UNDETECTED = 3;
	const int SW_FILE_DAMAGED = 4;
	const int SW_DATE_EXPIRED = 5;
	const int SW_COUNT_EXPIRED = 6;
	const int SW_UPDATE_FAIL = 7;
	const int SW_UNKNOWN = 8;

	class impl; 
	std::unique_ptr<impl> pimpl;
};
