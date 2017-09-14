#ifndef __CAL_FILE_H__
#define __CAL_FILE_H__

/*update calibration file version here*/
/*must add the related function pointer such as "t_GetItemSize";"t_ForwardConvert"*/
//#define CAL_FILE_VER 0x0000	// the very first version
//#define CAL_FILE_VER 0x0001	// add the version number of the file at the beginning,add head and tail before and after the CalFileInfo structure
//#define CAL_FILE_VER 0x0002	// get the probably correct value of x9119 from all calibration files,then create a new file named "cxu.cal" to save this value
//#define CAL_FILE_VER 0x0003	// change receiver's reference range : "output mode : "+30~-30dBm"--->"+30~-40dBm"
								//									   "in/out mode : "+30~-20dBm"--->"+30~-30dBm"
//#define CAL_FILE_VER 0x0004	// add an item:attenuation deviation in in/out mode
								// note that the driver has changed the meaning of the data,each 9 float data represents a status of the attenuation from 0dB to 90dB,step 10dB,so it's necessary to calibrate tx att and tx power again
#define CAL_FILE_VER 0x0005		// r1c first version

/*define all info's head and tail,which are used to locate the related structure*/
/*GUID_1 = {44A4AE51-6470-4C83-A568-34EDDA974411}*/
/*GUID_2 = {1E8EB58C-791E-42AA-8124-C197398944FF}*/
#define CAL_FILEVER_HEAD  0x44A4AE51
#define CAL_FILEVER_TAIL  0x64704C83

#define CAL_FILEINFO_HEAD 0xA56834ED
#define CAL_FILEINFO_TAIL 0xDA974411

#define CAL_ITEMINFO_HEAD 0x1E8EB58C	//not use actually,replace head with 0xaa and tail with 0xff
#define CAL_ITEMINFO_TAIL 0x791E42AA

#define CFO_ASSERT(stream,func)    /*calibration file operation assert*/            \
    do {                                                                            \
        if (func) {                                                                 \
            Log->SetLastError("%s:%s:%d(%d)",__FILE__,__FUNCTION__,__LINE__,errno); \
            fclose(stream);                                                         \
            return -1;                                                              \
        }                                                                           \
    } while(0)


#define CFO_ASSERT_S(stream,func,post) /*safe calibration file operation assert*/   \
    do {																			\
        if (func) {																	\
            Log->SetLastError("%s:%s:%d(%d)",__FILE__,__FUNCTION__,__LINE__,errno);	\
            fclose(stream);															\
            post;																	\
            return -1;																\
        }																			\
	} while(0)

#include "libLog.h"
#include "libBusDriver.h"
#include "TxSidebandTable.h"
#include "TxLoLeakageTable.h"
#include "TxAttTable.h"
#include "TxPowerTable.h"
#include "RxRefTable.h"
#include "RxAttTable.h"
#include "TxFilterTable.h"
#include "RxFilterTable.h"
#include "TxFilterOffTable.h"
#include "RxFilterOffTable.h"

class ICalFile
{
public:
#define CAL_TOTAL_ITEMS 25

    enum CalItem {              //list all the calibration items,the value is only allowed to accumulate
		TxSideband		= 0,	//r1a/b/c/d
		TxLoLeakage		= 1,	//r1a/b/c/d
		TxAttOP			= 2,	//r1a/b/c/d
		TxPowerOP		= 3,	//r1a/b/c/d		//default bw 160M
		TxPowerIO		= 4,	//r1a/b/c/d		//default bw 160M
		RxRef			= 5,	//r1a/b
		X9119			= 6,	//r1a/b/c/d
		TxAttIO			= 7,	//r1a/b/c/d
		RxRefOP			= 8,	//r1c/d			//default bw 160M
		RxRefIO			= 9,	//r1c/d			//default bw 160M
		RxAttOP			= 10,	//r1c/d
		RxAttIO			= 11,	//r1c/d
		TxFil_80		= 12,	//r1c/d
		TxFil_160		= 13,	//r1c/d
		TxRFFr_0		= 14,	//r1c/d
		TxRFFr_1		= 15,	//r1c/d
		TxIFFr			= 16,	//r1c/d
		RxFil_80		= 17,	//r1c/d
		RxFil_160		= 18,	//r1c/d
		RxRFFr			= 19,	//r1c/d
		RxIFFr			= 20,	//r1c/d
		TxFilOffOP_80	= 21,	//r1c/d
		TxFilOffIO_80	= 22,	//r1c/d
		RxFilOffOP_80	= 23,	//r1c/d
		RxFilOffIO_80	= 24	//r1c/d
	};

	typedef struct FileVer {
		uint32_t m_uiHead;
		uint32_t m_uiVer;
		uint32_t m_uiTail;
	}FileVer;

	struct ItemInfo;
	typedef struct FileInfo {
		uint32_t  m_uiHead;
		char	  m_szSN[32];
		uint32_t  m_uiItems;
		ItemInfo *m_pItemInfo;		//map all calibration items' information
		uint32_t  m_uiTail;
	public:
		FileInfo();
		~FileInfo();
	}FileInfo; 

	typedef struct ItemInfo {
		uint32_t m_uiHead;
		CalItem  m_Item;
		uint32_t m_uiSize;			//each frequency's data type(byte)
		uint32_t m_uiPoint;			//points
		uint32_t m_uiTail;
	}ItemInfo;

	typedef struct ItemBuf {
		uint32_t m_uiRfIdx;
		uint32_t m_uiRfuIdx;
		void    *m_pBuf;			//point to each table dynamically
		CalItem  m_Item;			//point out pointing to which table
	public:
		ItemBuf();
        ~ItemBuf();
		ItemBuf operator = (const ItemBuf &Buf);
	}ItemBuf;

#define DECLARE_CAL_FILE()                                                                  \
public:																						\
	virtual int32_t Add(CalItem Item,void *pData);											\
    virtual int32_t Map2Buf(CalItem Item);                                                  \
    virtual int32_t Map2Mem();																\
	virtual int32_t Create();																\
	virtual int32_t Load(FileInfo &Info);													\
	virtual int32_t GetItemSize(CalItem Item,uint32_t &dwPos,uint32_t &uiSize);

public:
    ICalFile(uint32_t uiRfIdx,uint32_t uiRfuIdx);
	virtual ~ICalFile();
	friend class CfvCtrl;
public:
	virtual int32_t Add(CalItem Item,void *pData) = 0;
    virtual int32_t Map2Buf(CalItem Item) = 0;
    virtual int32_t Map2Mem() = 0;
	virtual int32_t Create() = 0;
	virtual int32_t Load(FileInfo &Info) = 0;
    //getting every item's size may differ from each file version
	virtual int32_t GetItemSize(CalItem Item,uint32_t &dwPos,uint32_t &uiSize) = 0;
public:
	BW GetBw();
	uint32_t GetRfIdx();
    uint32_t GetRfuIdx();
    uint32_t GetOrdinal();
    virtual int32_t GetFileVer();
	static uint32_t GetFixItemTableR1A(ItemInfo *pInfo,uint32_t &uiItems);
	static uint32_t GetFixItemTableR1C(ItemInfo *pInfo,uint32_t &uiItems);
	static uint32_t GetMaxItemByte(ItemInfo *pInfo,uint32_t uiItems);
	static uint32_t GetMaxItemByte();
	virtual int32_t Open();
	virtual bool IsFileValid();
    virtual int32_t WriteFromPos(char *pPath,uint32_t uiPos,uint32_t uiSize,void *pData);
    virtual int32_t ReadFromPos(char *pPath,uint32_t uiPos,uint32_t uiSize,void *pData);
	virtual int32_t Write(CalItem Item);
	virtual int32_t Read(CalItem Item,void *pData);
	virtual int32_t SetBW(BW bw);

	virtual void ShowFileVer(FileVer *pVer);
	virtual void ShowFileInfo(FileInfo *pInfo);
	virtual void ShowItemInfo(ItemInfo *pInfo);
protected:
	static ItemBuf g_ItemBuf;
	uint32_t m_uiRfuIdx;
	uint32_t m_uiRfIdx;
    uint32_t m_uiOrdinal;
    BW		 m_BW;
};

#endif