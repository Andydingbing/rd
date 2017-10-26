#include "stdafx.h"
#include "RFU_Debug_Tool.h"
#include "LogDlg.h"
#include "CalBaseThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CLogDlg, CDialog)

CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLogDlg::IDD, pParent)
{
}

CLogDlg::~CLogDlg()
{
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MESSAGE_LIST, m_MsgList);
	DDX_Control(pDX, IDC_REG_LIST, m_RegList);
}


BEGIN_MESSAGE_MAP(CLogDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_GETDISPINFO, IDC_MESSAGE_LIST, &CLogDlg::OnLvnGetdispinfoMessageList)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_REG_LIST, &CLogDlg::OnLvnGetdispinfoRegList)
	ON_MESSAGE(WM_CSE_MSG_LOG, &CLogDlg::AddMsgList)
	ON_MESSAGE(WM_CSE_REG_LOG, &CLogDlg::AddRegList)
END_MESSAGE_MAP()


BOOL CLogDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect crMsg,crDlg,crProg;
	m_MsgList.GetWindowRect(&crMsg);
	ScreenToClient(&crMsg);
	crProg.top = crMsg.bottom;
	crProg.bottom = crProg.top + PROGRESS_HEIGHT;
	this->GetWindowRect(&crDlg);
	ScreenToClient(&crDlg);
	crProg.left = crDlg.left;
	crProg.right = crDlg.right;

	m_Progress.Create(WS_CHILD | WS_VISIBLE | WS_EX_DLGMODALFRAME,crProg,this,IDC_PROGRESS);
	m_Progress.SetPos(0);
	m_Progress.ShowWindow(SW_NORMAL);

	DWORD dwStyleList = m_MsgList.GetExtendedStyle();
	dwStyleList |= LVS_EX_GRIDLINES;
	dwStyleList |= LVS_EX_FULLROWSELECT;
	dwStyleList &= ~LVS_EX_AUTOSIZECOLUMNS;
	m_MsgList.SetExtendedStyle(dwStyleList);
	m_MsgList.SetItemCount(0);
	m_MsgList.InsertColumn(0,_T("Time"),LVCFMT_LEFT,115);
	m_MsgList.InsertColumn(1,_T("Message"),LVCFMT_LEFT,350);
	m_MsgList.InsertColumn(2,_T("Result"),LVCFMT_LEFT,50);
	m_MsgList.InsertColumn(3,_T("Errno"),LVCFMT_LEFT,75);

	m_RegList.SetExtendedStyle(dwStyleList);
	m_RegList.SetItemCount(0);
	m_RegList.InsertColumn(0,_T("Time"),LVCFMT_LEFT,54);
	m_RegList.InsertColumn(1,_T("FPGA"),LVCFMT_LEFT,68);
	m_RegList.InsertColumn(2,_T("Addr"),LVCFMT_LEFT,60);
	m_RegList.InsertColumn(3,_T("Write"),LVCFMT_LEFT,85);
	m_RegList.InsertColumn(4,_T("Read"),LVCFMT_LEFT,85);
	m_RegList.InsertColumn(5,_T("Result"),LVCFMT_LEFT,50);

	return TRUE;
}

void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	int iMsgListWidth = cx - REG_LIST_WIDTH;
	int iListHeight = cy - PROGRESS_HEIGHT;

	if (::IsWindowVisible(m_MsgList.GetSafeHwnd())) {
		m_MsgList.MoveWindow(0,0,iMsgListWidth,iListHeight,TRUE);
		m_MsgList.SetColumnWidth(1,iMsgListWidth - 270);
	}
	if (::IsWindowVisible(m_RegList.GetSafeHwnd()))
		m_RegList.MoveWindow(iMsgListWidth,0,REG_LIST_WIDTH,iListHeight,TRUE);
	if (::IsWindowVisible(m_Progress.GetSafeHwnd()))
		m_Progress.MoveWindow(0,iListHeight,cx,PROGRESS_HEIGHT,TRUE);
}

BOOL CLogDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
		return TRUE;
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
		return TRUE;
	else
		return CDialog::PreTranslateMessage(pMsg);
}

void CLogDlg::InitProgress(char *pName,int32_t iPts,uint32_t uiThreadID)
{
	m_Progress.SetWindowText(pName);
	m_Progress.SetRange32(0,iPts);
	m_Progress.SetPos(0);
	CCSEWinThread::m_strProc.Format("%s",pName);
}

void CLogDlg::SetProgress(int32_t iPos,uint32_t uiThreadID)
{
	m_Progress.SetPos(iPos);
}

void CLogDlg::OnLvnGetdispinfoMessageList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	LVITEMA pItem= pDispInfo->item;
	int iItemIdx = pItem.iItem;

	if (pItem.mask & LVIF_TEXT) {
		CString strText;

		if (0 == pItem.iSubItem)
			strText.Format("%s",Log.GetMsgLog()->at(iItemIdx).m_szTime);
		else if (1 == pItem.iSubItem)
			strText.Format("%s",Log.GetMsgLog()->at(iItemIdx).m_szMsg);
		else if (2 == pItem.iSubItem)
			strText = Log.GetMsgLog()->at(iItemIdx).m_iResult ? _T("�w") : _T("��");
		else if (3 == pItem.iSubItem)
			strText.Format("0x%08X",Log.GetMsgLog()->at(iItemIdx).m_iResult);

		lstrcpyn(pItem.pszText, strText, pItem.cchTextMax);
	}
	*pResult = 0;
}

void CLogDlg::OnLvnGetdispinfoRegList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	LVITEMA pItem = pDispInfo->item;
	int iItemIdx = pItem.iItem;

	if (pItem.mask & LVIF_TEXT) {
		CString strText;

		if (0 == pItem.iSubItem)
			strText.Format("%s",Log.GetRegLog()->at(iItemIdx).m_szTime);
		else if (1 == pItem.iSubItem)
			strText.Format("%s",Log.GetRegLog()->at(iItemIdx).m_szFpga);
		else if (2 == pItem.iSubItem && 0xffff != Log.GetRegLog()->at(iItemIdx).m_iAddr)
			strText.Format("0x%04X",Log.GetRegLog()->at(iItemIdx).m_iAddr);
		else if (3 == pItem.iSubItem && 0xffffffff != Log.GetRegLog()->at(iItemIdx).m_uiWValue)
			strText.Format("0x%08X",Log.GetRegLog()->at(iItemIdx).m_uiWValue);
		else if (4 == pItem.iSubItem && 0xffffffff != Log.GetRegLog()->at(iItemIdx).m_uiRValue)
			strText.Format("0x%08X",Log.GetRegLog()->at(iItemIdx).m_uiRValue);
		else if (5 == pItem.iSubItem && 0xffff != Log.GetRegLog()->at(iItemIdx).m_iAddr)
			strText = Log.GetRegLog()->at(iItemIdx).m_iResult ? _T("�w") : _T("��");

		lstrcpyn(pItem.pszText, strText, pItem.cchTextMax);
	}
	*pResult = 0;
}

LPARAM CLogDlg::AddMsgList(WPARAM wParam,LPARAM lParam)
{
	int iMsgCnt = (int)(Log.GetMsgLog()->size());
	m_MsgList.SetItemCount(iMsgCnt);
	m_MsgList.EnsureVisible(iMsgCnt - 1,FALSE);
	m_MsgList.SetItemState(iMsgCnt - 1,LVIS_SELECTED,LVIS_SELECTED);
	return 0;
}

LPARAM CLogDlg::AddRegList(WPARAM wParam,LPARAM lParam)
{
	int iRegCnt = (int)(Log.GetRegLog()->size());
	m_RegList.SetItemCount(iRegCnt);
	m_RegList.EnsureVisible(iRegCnt - 1,FALSE);
	m_RegList.SetItemState(iRegCnt - 1,LVIS_SELECTED,LVIS_SELECTED);
	return 0 ;
}

void InitSinglePos(char *pName,int32_t iPts,uint32_t uiThreadID)
{
	g_pLogDlg->InitProgress(pName,iPts,uiThreadID);
}