#pragma once

#include <string>
#include <string.h>
using namespace std;

#define CAMERA_POINTGREY	0
#define CAMERA_IMI			1
#define CAMERA_SMART		2

#define LPR_CAMPUS	0
#define LPR_HIGHWAY	1

#define IMAGE_HIGHTWAY_W	1388
#define IMAGE_HIGHTWAY_H	1036

#define IMAGE_CAMPUS_W		1624
#define IMAGE_CAMPUS_H		1228

#define IMAGE_POINTGREY_W	1600
#define IMAGE_POINTGREY_H	1200

#define IMAGE_SMART_W		1600
#define IMAGE_SMART_H		1200


#define HL_ERROR_DATA	-3
#define HL_ERROR_OPEN	-2
#define HL_ERROR_WRITE	-1
#define HL_ERROR_READ	0

#define RET_CODE_LOCK_FAIL			-2
#define RET_CODE_RECOG_COUNT_FAIL	-1
#define RET_CODE_RECOG_NOPLATE		0
#define RET_CODE_RECOG_SUCCESS		1

#define IP_SW_VALID					0
#define IP_SW_MAC_CHECK_FAIL		1
#define IP_SW_SDCARD_CHECK_FAIL		2
#define IP_SW_FILE_UNDETECTED		3
#define IP_SW_FILE_DAMAGED			4
#define IP_SW_DATE_EXPIRED			5
#define IP_SW_COUNT_EXPIRED			6
#define IP_SW_UPDATE_FAIL			7
#define IP_SW_UNKNOWN				8



class IPLPR
{
public:
	IPLPR(void);
	~IPLPR(void);
	
	private:
		int m_iSWLicenseStaus;

	private:
		bool license_verify(std::string &strFilePath, int &count, int &countThres);
		
		bool license_isMatch(string s1, string s2);
		bool license_isExpired(char strDateTime[], int len);
		bool license_FileDamaged(char * filename);
		bool license_FileExist(char * filename);
		bool license_existCard(int i, std::string &sCID);
		bool license_update(string strPath, int newCount);
		bool license_getInfor(string strPath, string &sID, char decodeDateTime[], int &recogCount, int &recogCountThres);
		void license_decode_chararray(char letters[], int length);
		
public:
	virtual int GetSWLicenseStatus();
	
	virtual int License_Check();
	
	virtual int LPR_Init(int width, int height, int sy, int ey);
	virtual int LPR_Release();

	virtual int ReadPlateNumber_File(	//INPUT PARAMS
										int iCameraType,			//0: PointGrey, 1: IMI, 2: Smart
										const char * szFilePath,	//16bit raw image file path
										int width, int height,		//image size
										int sx, int sy, int ex, int ey,				//top, down limit
										int iLPRType,				//0: campus version, 1: highway version
										int iCameraView,			//1: other, 2: driver-side
										int iDayTime,				//1							
										int iShiftCount,
										int arShiftIndex[],			

										//OUTPUT PARAMS
										int * plateX, int * plateY, int * plateWidth, int * plateHeight,	//plate location
										int * plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
										int * plateIntensity,	//plate region's intensity
										char * szPlateNumber,	//plate number
										double * dblRecogTime,	//recognition time

										int fileIndex,			//0
										int iSaveLog,			//0
										const char * szLogFile,	//""
										int threadX = -1, int threadY = -1);


	virtual int ReadPlateNumber_Data(	//INPUT PARAMS	
										int iCameraType,			//0: PointGrey, 1: IMI, 2: Smart
										unsigned char * p16Bit,		//16bit raw image data
										int width, int height,		//image size
										int sx, int sy, int ex, int ey,				//top, down limit
										int iLPRType,				//0: campus version, 1: highway version
										int iCameraView,			//0: front, 1: passenger-side, 2: driver-side
										int iDayTime,				//1
										int iShiftCount,
										int arShiftIndex[],			

										//OUTPUT PARAMS
										int * plateX, int * plateY, int * plateWidth, int * plateHeight,	//plate location
										int * plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
										int * plateIntensity,	//plate region's intensity
										char * szPlateNumber,	//plate number
										double * dblRecogTime,	//recognition time

										int fileIndex,			//0
										int iSaveLog,			//0
										const char * szLogFile,	//""
										int threadX = -1, int threadY = -1);

	virtual int ReadPlateNumber_Buffer(	//INPUT PARAMS	
										unsigned char * pBuffer,	//JPEG file buffer data
										long buffLen,
										int sx, int sy, int ex, int ey,	//top, down limit
										int iLPRType,				//0: campus version, 1: highway version, -1 unknown
										int iCameraView,			//0: front, 1: passenger-side, 2: driver-side
										int iDayTime,				//1

										//OUTPUT PARAMS
										int * plateX, int * plateY, int * plateWidth, int * plateHeight,	//plate location
										int * plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
										int * plateIntensity,	//plate region's intensity
										char * szPlateNumber,	//plate number
										double * dblRecogTime,	//recognition time

										int fileIndex,			//0
										int iSaveLog,			//0
										const char * szLogFile,	//""
										int threadX = -1, int threadY = -1);

	virtual int ReadPlateNumber_RGB(	//INPUT PARAMS	
										unsigned char * pRGB,	//JPEG file buffer data
										int width, int height,
										int sx, int sy, int ex, int ey,	//top, down limit
										int iLPRType,				//0: campus version, 1: highway version, -1 unknown
										int iCameraView,			//0: front, 1: passenger-side, 2: driver-side
										int iDayTime,				//1

										//OUTPUT PARAMS
										int * plateX, int * plateY, int * plateWidth, int * plateHeight,	//plate location
										int * plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
										int * plateIntensity,	//plate region's intensity
										char * szPlateNumber,	//plate number
										double * dblRecogTime,	//recognition time

										int fileIndex,			//0
										int iSaveLog,			//0
										const char * szLogFile,	//""
										int threadX = -1, int threadY = -1);

	virtual int ReadPlateNumber_Gray(	//INPUT PARAMS	
										unsigned char * pGray,	//JPEG file buffer data
										int width, int height,
										int sx, int sy, int ex, int ey,	//top, down limit
										int iLPRType,				//0: campus version, 1: highway version, -1 unknown
										int iCameraView,			//0: front, 1: passenger-side, 2: driver-side
										int iDayTime,				//1

										//OUTPUT PARAMS
										int * plateX, int * plateY, int * plateWidth, int * plateHeight,	//plate location
										int * plateType,		//plate color type: (1: dark char, bright background) (-1: bright char, dark background)
										int * plateIntensity,	//plate region's intensity
										char * szPlateNumber,	//plate number
										double * dblRecogTime,	//recognition time

										int fileIndex,			//0
										int iSaveLog,			//0
										const char * szLogFile,	//""
										int threadX = -1, int threadY = -1);

	virtual unsigned char * GetGrayData(char * szImagePath, int &width, int &height);
	virtual unsigned char * GetRGBData(char * szImagePath, int &width, int &height);

	//==============================================================
	virtual void IP_LPR_GetVersion(char * szVersion);

};

