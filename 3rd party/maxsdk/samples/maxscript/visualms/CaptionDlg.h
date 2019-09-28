#pragma once

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CCaptionDlg dialog

class CCaptionDlg : public CDialog
{
	DECLARE_DYNAMIC(CCaptionDlg)

public:
	CCaptionDlg(CWnd* pParent = NULL);   // standard constructor
	CCaptionDlg(const CString captionText, CWnd* pParent = NULL);
	virtual ~CCaptionDlg();

	// Dialog Data
	enum { IDD = IDD_CAPTIONS };
	CString m_caption;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
