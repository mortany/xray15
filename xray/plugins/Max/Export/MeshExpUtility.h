// file: MeshExpUtility.h


#ifndef __MeshExpUtility__H__INCLUDED__
#define __MeshExpUtility__H__INCLUDED__

#include "NetDeviceLog.h"
#include "MeshExpUtility.rh"

#define EXPORTER_VERSION	2
#define EXPORTER_BUILD		03
//using namespace std;

// refs
class CEditableObject;

//#define	EXP_UTILITY_CLASSID 0x507d29c0

//#define EXP_UTILITY_CLASSID Class_ID(0x91c4f4b3, 0x64453c86)

class ExportItem {
public:
	INode *pNode;
	ExportItem(){ pNode = 0;};
	ExportItem( INode* _pNode ){ pNode = _pNode; };
	~ExportItem(){};
};

DEFINE_VECTOR(ExportItem,ExportItemVec,ExportItemIt);
class MeshExpUtility : public UtilityObj {
public:

	IUtil		*iu;
	Interface	*ipanel;

	HWND		hPanel;
	HWND		hItemList;

	INode*		GetExportNode	();
protected:
	ExportItemVec m_Items;

	void		RefreshExportList();
	void		UpdateSelectionListBox();

	BOOL		BuildObject		(CEditableObject*& obj, LPCTSTR m_ExportName);
	BOOL		SaveAsObject	(LPCTSTR n);
	BOOL		SaveAsSkin		(LPCTSTR n);
	BOOL		SaveSkinKeys	(LPCTSTR n);
public:
	int			m_ObjectFlipFaces;
	int			m_SkinFlipFaces;
	int			m_SkinAllowDummy;
public:
				MeshExpUtility	();
	virtual		~MeshExpUtility	();

	virtual void BeginEditParams(Interface* ip, IUtil* iu) override;
	virtual void EndEditParams(Interface* ip, IUtil* iu) override;
	virtual void SelectionSetChanged(Interface *ip,IUtil *iu) override;
	virtual void DeleteThis() override {}

	virtual void		Init			(HWND hWnd);
	virtual void		Destroy			(HWND hWnd);
	void		ExportObject	();
	void		ExportSkin		();
	void		ExportSkinKeys	();

	// Singleton access
	static MeshExpUtility* GetInstance() {
		static MeshExpUtility themaxProject2;
		return &themaxProject2;
	}

private:
	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

extern MeshExpUtility U;



#endif /*__MeshExpUtility__H__INCLUDED__*/


