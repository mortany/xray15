//
// Copyright [2011] Autodesk, Inc.  All rights reserved. 
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.
//
#pragma once

#include "WindowsDefines.h"
#include "baseinterface.h"
#include "strclass.h"

class IViewInternalExp;
class ViewEx;
class ViewExp;

#define VIEW_PANEL_INTERFACE Interface_ID(0xe3505bb9, 0x1d734df6)

/**
* Many viewpanel related methods of CORE interface only work on the active
* tabbed view panel. For example, Interface::getNumViewports() only return
* the number of viewports of current view panel. This interface provides 
* methods to get/modify settings of a tabbed view panel no matter it is currently
* active or not.
*/
class IViewPanel : public BaseInterface
{
public:
	virtual ~IViewPanel() {}

	virtual Interface_ID GetID() { return VIEW_PANEL_INTERFACE; }

	/**
	 * Gets number of the enabled and non-extended viewports.
	 * \return the number of the enabled and non-extended viewports.
	 */
	virtual size_t GetNumberOfViewports() const = 0;

	/**
	 * Gets the layout of this view panel. 
	 * \return the layout id
	 * \see ViewPanelLayoutConfigIds 
	 */
	virtual int GetLayout() const = 0;

	/**
	 * Set layout of this view panel.
	 * \param[in] layout the layout configuration to use
	 * \see ViewPanelLayoutConfigIds 
	 */
	virtual void SetLayout(int layout) = 0;

	/**
	 * Set layout of this view panel.
	 * \param[in] layout the layout configuration to use
	 * \param[in] forceSettings force the config setting to be re-applied
	 * \see ViewPanelLayoutConfigIds 
	 */
	virtual void SetLayout(int layout, bool forceSettings) = 0;

	/**
	 * Gets index of the viewport with the specified window handle.
	 * User can use the viewport index to get the corresponding ViewExp
	 * interface by calling IViewPanel::GetViewExpByIndex(int index).
	 * \param[in] hwnd the window handle of a viewport in this viewpanel.
	 * \return the index of the viewport with the given window handle.
	 * If none of the viewport matches the given handle, -1 is returned.
	 */
	virtual int	GetViewportIndex(HWND hwnd) const = 0;

	/**
	 * Set active viewport according to the input param.
	 * \param[in] index the index of the viewport to be activated
	 * \return true if this operation succeeds, false otherwise.
	*/
	virtual bool SetActiveViewport(int index) = 0;

	/**
	 * Get the index of the active viewport.
	 * \return the index of the active viewport.
	 * If there is no active viewport in this viewpanel, -1 is returned.
	*/
	virtual int	GetActiveViewportIndex(void) const = 0;

	
	/**
	 * Get the window handle of the tabbed view panel
	 * \return Return the window handle of the view panel.
	*/
	virtual HWND GetHWnd() const = 0;

	/**
	* Set the view panel name.
	* \param[in] newName the new name for the view panel.
	*/
	virtual void SetViewPanelName(const MSTR& newName) = 0;

	/**
	* Get the view panel name.
	* \return Return the view panel name.
	*/
	virtual const MSTR& GetViewPanelName() const = 0;

	/**
	* Get the ViewExp interface given the viewport index.
	* \param[in] index a valid viewport index
	* \return if input is valid, the specified ViewExp interface will be returned.
	* Otherwise, the ViewExp interface of the first viewport(index = 0) will be returned.
	*/
	virtual ViewExp& GetViewExpByIndex(int index) const = 0;

    /**
     * Gets if this view panel is a floating view panel.
     * \return Return true if it is a floating view panel.
     */
    virtual bool IsViewPanelFloating() const = 0;

    /**
     * Gets if this view panel is currently visible .
     * \return Return true if this view panel is currently visible.
     */
    virtual bool IsViewPanelVisible() const = 0;
};