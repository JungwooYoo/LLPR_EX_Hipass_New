/*LPResult.h
This is an template for storing a result of automobile licese plate recognition.
Author's E-mail : chipchoc@hanmail.net
Last modified: July/09/2016
*/

#pragma once

#include "opencv2/opencv.hpp"

/**
@class LPResult
@brief The storage class for the License Plate Recognition result.
*/
class LPResult
{
public:
	LPResult()
	{
		//set initial values.
		roi = cv::Rect(-1, -1, -1, -1);
		fLPScore = -100.f;
	};
	~LPResult()	{};

	cv::Rect roi;
	std::string strMainNumber;		//4 numbers
	std::string strUsage;			//1 Hangul
	std::string strType;			//1 or 2 numbers
	std::string strRegion;			//regions
	std::string strCommercial;		//commercial

	float fLPScore;		//a score for LP existence
	
	int LP_area;	//for debug

private:

};
