// CaptionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "visualms.h"
#include "CaptionDlg.h"
#include "afxdialogex.h"


// CCaptionDlg dialog

IMPLEMENT_DYNAMIC(CCaptionDlg, CDialog)

CCaptionDlg::CCaptionDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCaptionDlg::IDD, pParent)
{
	m_caption.Empty();
}

CCaptionDlg::CCaptionDlg(const CString captionText, CWnd* pParent /*=NULL*/)
	: CDialog(CCaptionDlg::IDD, pParent)
{
	m_caption = captionText;
}

CCaptionDlg::~CCaptionDlg()
{
}

void CCaptionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_OBJ_CAPTION, m_caption);
}


BEGIN_MESSAGE_MAP(CCaptionDlg, CDialog)
END_MESSAGE_MAP()


// CCaptionDlg message handlers
