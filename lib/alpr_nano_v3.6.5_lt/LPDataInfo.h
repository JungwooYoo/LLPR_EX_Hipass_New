#pragma once
#include <string>
#include "opencv2/opencv.hpp"

//Imagefile information including LicensePlate GroundTruth.
class LPDataInfo
{
public:
	LPDataInfo()
	:imgType(-2), imgWidth(0), imgHeight(0), rawData(0), TL(-1,-1), TR(-1,-1), BL(-1,-1), BR(-1,-1), bPosGT(0), noGT(1)
	{

	};
	~LPDataInfo(){};

	//file information
	std::string singleFilename; //single filename without path
	
	int imgType;	// -1: image format(jpeg, png, etc), 
					// 0: IMI camera RAW
					// 1: 147 camera RAW
					// 2: PointGrey camera RAW

	//Image Data - this image data should be released from memory manually at some point.
	cv::Mat cvimage;
	unsigned char *rawData;

	// Information for raw images
	int imgWidth;	
	int imgHeight;

	//Licenseplate GroundTruths(Optional)
	std::string region, type, usage, main_numbers, commercial;	//LP character groundtruth

	//GT for the license plate's position(Optional)
	cv::Point TL;
	cv::Point TR;
	cv::Point BL;
	cv::Point BR;

	bool bPosGT;	// A flag for indicating LP Position GT(Optional)
	bool noGT;	//True, if not valid LP character gt.(Optional)
};