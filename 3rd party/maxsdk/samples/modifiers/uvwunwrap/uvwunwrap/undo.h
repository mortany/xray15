//**************************************************************************/
// Copyright (c) 1998-2006 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: These are our undo classes
// AUTHOR: Peter Watje
// DATE: 2006/10/07 
//***************************************************************************/
#pragma once 

class UnwrapMod;
class MeshTopoData;

//*********************************************************
// Undo record for TV posiitons only
//*********************************************************
class TVertRestore : public RestoreObj 
{
	public:
		MeshTopoData *ld;
		Tab<UVW_TVVertClass> undo, redo;
		BitArray uvsel, rvsel;
		BitArray uesel, resel;
		BitArray ufsel, rfsel;

		BOOL updateView;

		TVertRestore(MeshTopoData *ld); 
		void Restore(int isUndo);
		void Redo();
		void EndHold();
		TSTR Description();
};

// to ensure the pelt dialog is hidden during undo/redo.
class HidePeltDialogRestore : public RestoreObj
{
public:
	HidePeltDialogRestore(UnwrapMod* pMod);
	~HidePeltDialogRestore();
	void Restore(int isUndo);
	void Redo();
	void EndHold();
	TSTR Description();
private:
	UnwrapMod *mMod;
};

//*********************************************************
// Undo record for TV posiitons and face topology
//*********************************************************
class TVertAndTFaceRestore : public RestoreObj {
	public:
		MeshTopoData *ld;

		Tab<UVW_TVVertClass> undo, redo;	// undo/redo Geom data
		Tab<UVW_TVFaceClass*> fundo, fredo; // undo/redo Topo Data
		BitArray uvsel, rvsel;				// undo/redo UV vert selection
		BitArray ufsel, rfsel;				// undo/redo UV/Geom face selection
		BitArray uesel, resel;				// undo/redo UV edge selection
		BitArray ugesel, rgesel;			// undo/redo geom edge selection
		BitArray ugvsel,rgvsel;				// undo/redo geom selection

		BOOL update;

		TVertAndTFaceRestore(MeshTopoData *ld);
		~TVertAndTFaceRestore() ;
		void Restore(int isUndo) ;
		void Redo();
		void EndHold();
		TSTR Description();
	};


class TSelRestore : public RestoreObj 
{
	public:
		BOOL bUpdateView;
		MeshTopoData *ld;
		BitArray undo, redo;
		BitArray fundo, fredo;
		BitArray eundo, eredo;

		BitArray geundo, geredo;
		BitArray gvundo, gvredo;

		TSelRestore(MeshTopoData *ld);
		void Restore(int isUndo);
		void Redo();
		void EndHold();
		TSTR Description();
	};	

class ResetRestore : public RestoreObj {
	public:
		UnwrapMod *mod;
		int uchan, rchan;


		ResetRestore(UnwrapMod *m);
		~ResetRestore();
		void Restore(int isUndo);
		void Redo();
		void EndHold();

		
		TSTR Description();
	};




class UnwrapPivotRestore : public RestoreObj {
public:
	Point3 upivot, rpivot;
	UnwrapMod *mod;

	UnwrapPivotRestore(UnwrapMod *m);
	
	void Restore(int isUndo);
	void Redo();
	int Size() { return 1; }
	void EndHold() {}
	TSTR Description();// { return TSTR(GetString(IDS_PW_PIVOTRESTORE));	}
};

class UnwrapSeamAttributesRestore : public RestoreObj {
public:
	
	UnwrapMod *mod;
	BOOL uThick, uShowMapSeams, uShowPeltSeams, uReflatten;
	BOOL rThick, rShowMapSeams, rShowPeltSeams, rReflatten;

	UnwrapSeamAttributesRestore(UnwrapMod *m);
	
	void Restore(int isUndo);
	void Redo();
	int Size() { return 1; }
	void EndHold() {}
	TSTR Description();// { return TSTR(GetString(IDS_PW_PIVOTRESTORE)); 	}
};


class UnwrapMapAttributesRestore : public RestoreObj {
public:
	
	UnwrapMod *mod;
	BOOL uPreview, uNormalize;
	int uAlign;
	
	BOOL rPreview, rNormalize;
	int rAlign;
	

	UnwrapMapAttributesRestore(UnwrapMod *m);
	
	void Restore(int isUndo);
	void Redo();
	int Size() { return 1; }
	void EndHold() {}
	TSTR Description();// { return TSTR(GetString(IDS_PW_PIVOTRESTORE)); 	}
};

class HoldSuspendedOffGuard : public MaxSDK::Util::Noncopyable
{
public:
	HoldSuspendedOffGuard(bool bSuspended);
	~HoldSuspendedOffGuard();
private:
	bool mbTempSuspended;
};
