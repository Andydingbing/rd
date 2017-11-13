#ifndef __LOG_H__
#define __LOG_H__

#ifndef GUI_MFC
#define GUI_MFC
#endif

#ifndef _WINDOWS
#define _WINDOWS
#endif

#ifndef _WIN64
#define _WIN64
#endif

#include "stdint_common.h"
#include <stdio.h>
#include <vector>

#ifndef _AMD64_
#define _AMD64_
#endif

#if defined(_WINDOWS) && defined (GUI_MFC)
#include <WinDef.h>
#include <WinBase.h>
#include <WinUser.h>

#define WM_CSE_MSG_LOG (WM_USER + 1000)
#define WM_CSE_REG_LOG (WM_USER + 1001)
#endif

#define SAFE_DELETE(ptr) if (ptr) {delete ptr;ptr = NULL;}

#ifdef _MSC_VER
#define STRUCT_ALIGN_S(struct_name,align_size)  \
        typedef struct struct_name {

#define STRUCT_ALIGN_E(struct_name,align_size)  \
        }struct_name;

#define CLASS_ALIGN_S(class_name,align_size)	\
		class class_name {

#define CLASS_ALIGN_E(class_name,align_size)	\
		};

#define STRUCT_ALIGN_INHERIT_S(struct_name,align_size,parent)   \
        typedef struct struct_name : public parent {

#define STRUCT_ALIGN_INHERIT_E(struct_name,align_size)          \
        }struct_name;

#define CLASS_ALIGN_INHERIT_S(class_name,align_size,parent)		\
		class class_name : public parent {

#define CLASS_ALIGN_INHERIT_E(class_name,align_size)			\
		};
#elif defined __GNUC__
#define STRUCT_ALIGN_S(struct_name,align_size)  \
        typedef struct struct_name {

#define STRUCT_ALIGN_E(struct_name,align_size)  \
        }__attribute__((aligned(align_size))) struct_name;

#define CLASS_ALIGN_S(class_name,align_size)	\
		class class_name {

#define CLASS_ALIGN_E(class_name,align_size)	\
		}__attribute__((aligned(align_size)));

#define STRUCT_ALIGN_INHERIT_S(struct_name,align_size,parent)   \
        typedef struct struct_name : public parent {

#define STRUCT_ALIGN_INHERIT_E(struct_name,align_size)          \
        }__attribute__((aligned(align_size))) struct_name;

#define CLASS_ALIGN_INHERIT_S(class_name,align_size,parent)		\
		class class_name : public parent {

#define CLASS_ALIGN_INHERIT_E(class_name,align_size)			\
		}__attribute((aligned(align_size)));
#endif

#ifdef _MSC_VER
#pragma pack(4)
#endif
STRUCT_ALIGN_S(MsgLog,4)
        int  m_iResult : 32;
        char m_szTime[32];
        char m_szMsg[512];
    public:
        MsgLog();
STRUCT_ALIGN_E(MsgLog,4)
#ifdef _MSC_VER
#pragma pack()
#endif

#ifdef _MSC_VER
#pragma pack(2)
#endif
STRUCT_ALIGN_S(RegLog,2)
        int  m_iResult : 16;
        char m_szFpga[16];
        int  m_iAddr : 32;
        char m_szTime[32];
        unsigned m_uiWValue : 32;
        unsigned m_uiRValue : 32;
    public:
        RegLog();
STRUCT_ALIGN_E(RegLog,2)
#ifdef _MSC_VER
#pragma pack()
#endif

#ifdef _MSC_VER
#pragma warning ( disable : 4996 )
#elif defined __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define Log CLog::Instance()

using namespace std;

enum LogSw {
	L_ALL_OFF	= 0x00000000,
	L_PROMPT	= 0x00000001,
	L_MSG		= 0x00000004,
	L_MSG_F		= 0x00000008,
	L_REG		= 0x00000010,
	L_REG_F		= 0x00000020,
	L_TRACE		= 0x00000040,
	L_ALL_ON	= 0xffffffff
};

class CLog
{
public:
	CLog();
	CLog(CLog &);
	CLog & operator = (CLog &);
	~CLog();
    static CLog &Instance();
	void Init();
    void Init(vector<MsgLog> *pMsgLog,vector<RegLog> *pRegLog);

#ifdef GUI_MFC
	void Init(HWND hWnd);
	void Init(vector<HWND> hWnd);
	void Init(HWND hWnd,vector<MsgLog> *pMsgLog,vector<RegLog> *pRegLog);
	void Init(vector<HWND> hWnd,vector<MsgLog> *pMsgLog,vector<RegLog> *pRegLog);
private:
    vector<HWND> *m_phWnd;
#else
    void Init(void (*pMsgCallback)(),void (*pRegCallback)());
    void Init(void (*pMsgCallback)(),void (*pRegCallback)(),vector<MsgLog> *pMsgLog,vector<RegLog> *pRegLog);
private:
    void (*m_pMsgCallback)();
    void (*m_pRegCallback)();
#endif

public:
	vector<MsgLog> *GetMsgLog();
	vector<RegLog> *GetRegLog();
	void Set(uint64_t uiLogSw);
	uint64_t Get();
	bool IsEnabled(LogSw Sw);
	void SetEnable(LogSw Sw,bool bEn);
	void SetDefault();
    void SetLastError(const char *pFormat, ...);
    void SetLastError(int iResult,const char *pFormat, ...);
    char *GetLastError();

	virtual void stdprintf(const char *format, ...);
	virtual void printf(const char *format, ...);
	virtual void trace(const char *format, ...);

    virtual int AddMsgList(const char *pFormat, ...);
    virtual int AddMsgList(int iResult,const char *pFormat, ...);
    virtual int AddRegList(int iResult,char *pFpga,int iAddr,unsigned uiWValue = 0xffffffff,unsigned uiRValue = 0xffffffff);
public:
	virtual double GetTimeStamp();
private:
	vector<MsgLog> *m_pMsgLog;
	vector<RegLog> *m_pRegLog;
	FILE *m_fpMsg;
	FILE *m_fpReg;
	void *m_pConsole;
	
	bool m_bOwnData;
	char m_szLastError[512];
	uint64_t m_uiSw;
};

#endif
