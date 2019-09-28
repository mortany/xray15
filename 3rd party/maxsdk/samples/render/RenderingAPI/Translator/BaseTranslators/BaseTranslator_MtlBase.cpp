//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2014 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_MtlBase.h>

// local
#include "../../resource.h"
// Max SDK
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <imtl.h>
#include <dllutilities.h>

#include "3dsmax_banned.h"

namespace MaxSDK
{;
namespace RenderingAPI
{;
using namespace NotificationAPI;

namespace
{
    // Ref enum proc, used to call MtlBase::Update() on a material and all the sub-material and sub-textures in its reference hierarchy.
    class MtlBaseUpdater
        : private RefEnumProc,
        public MaxSDK::Util::Noncopyable
    {
    public:

        MtlBaseUpdater(const TimeValue t, Interval& validity)    
            : m_validity(validity),
            m_time(t)
        {
        }

        // Does whatever this class is supposed to do (see above)
        void do_your_magic(MtlBase& mother_material)
        {
            // Set the 'preventDuplicatesViaFlag' argument to false, as having it true would result in the visited flag being reset on every animatable
            // in the system at every call to EnumRefHierarchy, turning this process into an O(N^2), very slow on scenes with large numbers of objects.
            mother_material.EnumRefHierarchy(*this, true, true, true, false);
        }

    private:

        // -- inherited from RefEnumProc
        virtual int proc(ReferenceMaker *rm) override
        {
            MtlBase* const mtl_base = dynamic_cast<MtlBase*>(rm);
            if(mtl_base != nullptr)
            {
                mtl_base->Update(m_time, m_validity);

                // Load bitmaps if necessary. This enables the bitmaps to be cached by the Texmap, such that they don't get
                // re-loaded at every render.
                if(IsTex(mtl_base))
                {
                    Texmap* const texmap = static_cast<Texmap*>(mtl_base);
                    texmap->LoadMapFiles(m_time);
                }

            }

            return REF_ENUM_CONTINUE;
        }

    private:

        const TimeValue m_time;
        Interval& m_validity;
    };

}

//==================================================================================================
// clas BaseTranslator_MtlBase
//==================================================================================================

BaseTranslator_MtlBase::BaseTranslator_MtlBase(MtlBase* mtl_base, const bool monitor_mtl_base, TranslatorGraphNode& graphNode)
    : BaseTranslator_ReferenceTarget(mtl_base, monitor_mtl_base, graphNode),
    Translator(graphNode),
    m_mtl_base(mtl_base)
{

}

BaseTranslator_MtlBase::~BaseTranslator_MtlBase()
{

}

MtlBase* BaseTranslator_MtlBase::GetMtlBase() const
{
    return m_mtl_base;
}

void BaseTranslator_MtlBase::PreTranslate(const TimeValue translationTime, Interval& validity) 
{
    __super::PreTranslate(translationTime, validity);

    if(m_mtl_base != nullptr)
    {
        // Update material and initialize validity. We perform this on the entire reference hierarchy, since AMG's maxscript-based code can't 
        // update the validity interval - and we very likely depend on every sub-material or sub-texture in the hierarchy.
        UpdateMaterialHierarchy(*m_mtl_base, translationTime, validity);
    }
}

MSTR BaseTranslator_MtlBase::GetTimingCategory() const 
{
    return MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_MTLBASE);
}

void BaseTranslator_MtlBase::UpdateMaterialHierarchy(MtlBase& mtlBase, const TimeValue t, Interval& validity)
{
    MtlBaseUpdater magic_updater(t, validity);
    magic_updater.do_your_magic(mtlBase);
}

}}	// namespace 


