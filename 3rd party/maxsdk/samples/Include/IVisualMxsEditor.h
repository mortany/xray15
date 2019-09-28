/**********************************************************************
FILE: IVisualMxsEditor.h

DESCRIPTION:	
	 Visual Editor for Custom Attriubtes is a simple tool for easily arrangement of UI Items.
	 This Editor will start when user choose "Visual Editor" in the right-click menu of Rollout Panel for Custom Attributes.
	 IVisualMSCAMgr and IVisualMSCAForm is used to create Visual Editor, they should not be accessed by 3rd party developers.

	 This interface is used only internally

HISTORY:

Copyright (c) 2014, All Rights Reserved.
**********************************************************************/
#pragma once
#include <maxscript/util/visualmaxscript.h>

class IVisualMSCAForm;

struct UIData4ParamEditor
{
	TSTR uiName;
	TSTR caption;
	int  posX;
	int  posY;
	int  width;
	int  height;
};

// -------- Core interface to the VisualMS manager -----------
class IVisualMSCAMgr : public FPStaticInterface 
{
public:
	virtual IVisualMSCAForm* CreateForm()=0;						// create a new form
	virtual IVisualMSCAForm* CreateFormFromFile(const MCHAR* fname)=0;	// create form from a .vms file

	// function IDs 
	enum { createForm,
		createFormFromFile,
	}; 
}; 

#define VISUALMS_CA_MGR_INTERFACE   Interface_ID(0x406a2f81, 0x48460e09)
inline IVisualMSCAMgr* GetVisualMSCAMgr() { return (IVisualMSCAMgr*)GetCOREInterface(VISUALMS_CA_MGR_INTERFACE); }

class IVisualMSCAForm : public IVisualMSForm 
{
public:
	virtual void			Open(IVisualMSCallback* cb=NULL, const MCHAR* source=NULL)=0; // open the form editor on this form
	virtual void			Close()=0;									// close the form editor

	virtual void			InitForm(const MCHAR* formType, const MCHAR* formName, const MCHAR* caption)=0;
	virtual void			SetWidth(int w)=0;
	virtual void			SetHeight(int h)=0;
	virtual IVisualMSItem*	AddItem(const MCHAR* itemType, const MCHAR* itemName, const MCHAR* text, int src_from=-1, int src_to=-1)=0;
	virtual IVisualMSItem*  AddCode(const MCHAR* code, int src_from=-1, int src_to=-1)=0;
	virtual IVisualMSItem*  FindItem(const MCHAR* itemName)=0;
	virtual BOOL			GenScript(MSTR& script, const MCHAR* indent=NULL)=0;
	virtual void			GetDataForParamEditor(MaxSDK::Array<UIData4ParamEditor> &uiDatas, TSTR &addSource, const FPValue* fpv)=0;

	virtual BOOL			HasSourceBounds(int& from, int& to)=0;
	virtual void			SetSourceBounds(int from, int to)=0;
};