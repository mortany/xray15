/*	
 *		VisualMSCAMgr.cpp - public manager interface for VisualMS of Custom Attributes 
 *
 *
 */

#include "stdafx.h"
#include "VisualMS.h"
#include "VisualMSThread.h"
#include "VisualMSDoc.h"
#include "MainFrm.h"
#include "FormEd.h"
#include "../../Include/IVisualMxsEditor.h"

#include <maxscript\util\visualmaxscript.h>

// ----------- FnPub interface implementation class & descriptors ---------------------

class VisualMSCAMgr : public IVisualMSCAMgr
{
private:

public:
	IVisualMSCAForm* CreateForm();						// create a new form
	IVisualMSCAForm* CreateFormFromFile(const TCHAR* fname);	// create form from a .vms file

	DECLARE_DESCRIPTOR(VisualMSCAMgr) 
	// dispatch map
	BEGIN_FUNCTION_MAP
		FN_0(createForm,		 TYPE_INTERFACE, CreateForm); 
		FN_1(createFormFromFile, TYPE_INTERFACE, CreateFormFromFile, TYPE_FILENAME); 
	END_FUNCTION_MAP 

	void    Init() { }
};

// VisualMS Manager interface instance and descriptor
VisualMSCAMgr visualMSCAMgr(VISUALMS_CA_MGR_INTERFACE, _T("visualMSCA"), 0, NULL, FP_CORE, 
	IVisualMSCAMgr::createForm,		  _T("createForm"), 0,	       TYPE_INTERFACE, 0, 0, 
	IVisualMSCAMgr::createFormFromFile, _T("createFormFromFile"), 0, TYPE_INTERFACE, 0, 1, 
		_T("fileName"), 0, TYPE_FILENAME,
	p_end 
); 


class VisualMSCAForm : public IVisualMSCAForm 
{
	bool	has_source;
	const TCHAR*  source;			// source string if any
	int		from, to;				// bounds of original rollout within source
	CVisualMSCAThread *pThread;

public:
			VisualMSCAForm();
	void	reset() { has_source = false; source = NULL; from = 0; to = 0; }
	void	Open(IVisualMSCallback* cb=NULL, const TCHAR* source=NULL);	// open the form editor on the form
	void	Close();						// close the form editor
	void	InitForm(const TCHAR* formType, const TCHAR* formName, const TCHAR* caption);
	IVisualMSItem* AddItem(const TCHAR* itemType, const TCHAR* itemName, const TCHAR* text, int src_from=-1, int src_to=-1);
	IVisualMSItem* AddCode(const TCHAR* code, int src_from=-1, int src_to=-1);
	IVisualMSItem* FindItem(const TCHAR* itemName);
	BOOL	GenScript(TSTR& script, const TCHAR* indent=NULL);
	BOOL	HasSourceBounds(int& from, int& to);
	void	SetSourceBounds(int from, int to);
	void	SetWidth(int w);
	void	SetHeight(int h);

	void    GetDataForParamEditor(MaxSDK::Array<UIData4ParamEditor> &uiDatas, TSTR &addSource, const FPValue* fpv);
};

VisualMSCAForm::VisualMSCAForm() : has_source(false), source(NULL)
{
	pThread = CVisualMSCAThread::Create(false); // create hidden
	CVisualMSDoc* doc = pThread->GetDoc();
	if (doc)
	{
		doc->SetModifiedFlag(FALSE);
		doc->m_creating = true;
	}
	reset();
}

void 
VisualMSCAForm::Open(IVisualMSCallback* cb, const TCHAR* source)	// open the form editor on the form
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	this->source = source;
	CVisualMSDoc* doc = pThread->GetDoc();
	if (doc)
	{
		doc->SetEditCallback(cb);
		doc->SetModifiedFlag(FALSE);
		doc->m_creating = false;
	}
	pThread->ShowApp();
	if (doc) doc->Invalidate();
}

void 
VisualMSCAForm::Close()				// close the form editor
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// TODO: This function should take a paremeter that lets VMS know if 
	// it should close without saving or prompt !!!
	CVisualMSDoc* doc = pThread->GetDoc();
	if (doc)
	{
		doc->SetModifiedFlag(FALSE);
	}
	pThread->Destroy();

	// DELETE THIS ?
	delete this;
}

void 
VisualMSCAForm::InitForm(const TCHAR* formType, const TCHAR* formName, const TCHAR* caption)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// reset current doc
	CVisualMSDoc* doc = pThread->GetDoc();

	// fill in form details
	CFormObj *formObj = doc->GetFormObj();
	formObj->Class() = formType;
	formObj->Name() = formName; 
	formObj->Caption() = caption; 
	formObj->PropChanged(IDX_NAME);
	doc->Invalidate();
}

void			
VisualMSCAForm::SetWidth(int w)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CVisualMSDoc* doc = pThread->GetDoc();
	CFormObj *formObj = doc->GetFormObj();
	formObj->m_properties[IDX_WIDTH] = w;
	formObj->PropChanged(IDX_WIDTH);
	doc->Invalidate();
}

void			
VisualMSCAForm::SetHeight(int h)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CVisualMSDoc* doc = pThread->GetDoc();
	CFormObj *formObj = doc->GetFormObj();
	formObj->m_properties[IDX_HEIGHT] = h;
	formObj->PropChanged(IDX_HEIGHT);
}

IVisualMSItem* 
VisualMSCAForm::AddItem(const TCHAR* itemType, const TCHAR* itemName, const TCHAR* text, int src_from, int src_to)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CVisualMSDoc* doc = pThread->GetDoc();

	// NOTE: Since VMS is now multi-threaded and CWnd derived objects cannot be created 
	// across threads use the new WM_CREATEOBJ message to create CGuiObj based objects.
	CRect rect(0,0,10,10);
	CGuiObj *obj = (CGuiObj*)::SendMessage(
		pThread->GetFrame()->GetSafeHwnd(),
		WM_CREATEOBJ,
		doc->ItemIDFromName(itemType),
		(LPARAM)&rect);

	if(obj)
	{
		obj->m_selected = FALSE;
		obj->src_from = src_from;
		obj->src_to = src_to;
		doc->AddObj(obj);
		obj->Class() = itemType;
		obj->Name() = itemName;
		obj->Caption() = text;
		obj->PropChanged(IDX_CAPTION);
	}
	return obj;

}

IVisualMSItem* 
VisualMSCAForm::FindItem(const TCHAR* itemName)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CVisualMSDoc* doc = pThread->GetDoc();
	for (int i = 0; i < doc->m_objects.GetSize(); i++)
	{
		CGuiObj* obj = (CGuiObj*)doc->m_objects[i];
		if (_tcsicmp(itemName, obj->Name()) == 0)
			return obj;
	}
	return NULL;
}

IVisualMSItem* 
VisualMSCAForm::AddCode(const TCHAR* code, int src_from, int src_to)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CVisualMSDoc* doc = pThread->GetDoc();
	CFormEd *formEd = &doc->GetFormObj()->m_formEd;
	CCodeObj *obj; 
	if (src_from >= 0)
	{
		// specified as offsets within source
		TCHAR c;
		// ignore if just a line of whitespace
		int sf;
		for (sf = src_from; sf < src_to && _istspace(c = code[sf]) && c != _T('\n') && c != _T('\r'); sf++) ;
		if (sf == src_to)
			return NULL;
		// trim any initial linefeed
		if (code[src_from] == _T('\n')) src_from++;
		else if (code[src_from] == _T('\r'))
			if (code[src_from] == _T('\n')) src_from += 2;
			else src_from++;
			// trim any final blank line
			while (src_to > src_from && _istspace(c = code[src_to-1]) && c != _T('\n') && c != _T('\r')) --src_to;
			// make code obj
			c = code[src_to];
			const_cast<TCHAR*>(code)[src_to] = _T('\0');
			obj = new CCodeObj (&code[src_from]);
			const_cast<TCHAR*>(code)[src_to] = c;
	}
	else
		obj = new CCodeObj (code);
	if(obj)
	{
		obj->m_selected = FALSE;
		obj->src_from = src_from;
		obj->src_to = src_to;
		doc->AddObj(obj);
	}
	return obj;
}

BOOL 
VisualMSCAForm::GenScript(TSTR& script, const TCHAR* indent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CVisualMSDoc* doc = pThread->GetDoc();
	if (doc)
	{
		CString cscript;
		bool res = doc->GenScript(cscript, indent);
		if (res)
		{
			script = cscript;
			return TRUE;
		}
	}
	return FALSE;
}

void
VisualMSCAForm::GetDataForParamEditor(MaxSDK::Array<UIData4ParamEditor> &uiDatas, TSTR &addSource, const FPValue* fpv)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CVisualMSDoc* doc = pThread->GetDoc();
	if (doc)
	{
		CString caddSource;
		doc->GetDataForParamEditor(uiDatas, caddSource, fpv);

		// Set result back
		addSource = caddSource;
	}
}

BOOL			
VisualMSCAForm::HasSourceBounds(int& from, int& to)
{
	if (has_source)
	{
		from = this->from;
		to = this->to;
	}
	return has_source;
}

void			
VisualMSCAForm::SetSourceBounds(int from, int to)
{
	has_source = true;
	this->from = from;
	this->to = to;
}

// ------------ VisualMS manager interface implementation methods ---------

IVisualMSCAForm* 
VisualMSCAMgr::CreateForm()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	VisualMSCAForm *pVmsForm = new VisualMSCAForm();
	return pVmsForm;
}

IVisualMSCAForm* 
VisualMSCAMgr::CreateFormFromFile(const TCHAR* fname)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return NULL;
}
