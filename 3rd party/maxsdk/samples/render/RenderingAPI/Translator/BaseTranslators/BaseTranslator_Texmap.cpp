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

// Max SDK
#include <RenderingAPI/Translator/BaseTranslators/BaseTranslator_Texmap.h>
#include <RenderingAPI/Renderer/IRenderSessionContext.h>
#include <imtl.h>
#include <stdmat.h>
#include <bitmap.h>
#include <pbbitmap.h>
#include <iparamb2.h>
#include <maxapi.h>

#include "3dsmax_banned.h"

namespace
{
    IPoint2 get_bake_resolution(Texmap& texmap, const TimeValue t, Interval& validity)
    {
        // If the texmap is a BitmapTex, use its resolution
        if(dynamic_cast<BitmapTex*>(&texmap) != nullptr)
        {
            BitmapTex& bitmap_tex = static_cast<BitmapTex&>(texmap);
            bitmap_tex.Update(t, validity);
            Bitmap* const bitmap = bitmap_tex.GetBitmap(t);
            if(bitmap != nullptr)
            {
                return IPoint2(bitmap->Width(), bitmap->Height());
            }
            else
            {
                // There's no bitmap, but we still need to bake (who knows, maybe the output parameters make it non-black?).
                return IPoint2(1, 1);
            }
        }
        else
        {
            // We don't have a standard bitmap tex, so we check if we have any TYPE_BITMAP parameters or any sub-texture, and we use the resolution
            // of the largest such sub-texture.
            IPoint2 resolution(0, 0);

            // Search for TYPE_BITMAP parameters
            const int num_param_blocks = texmap.NumParamBlocks();
            for(int param_block_index = 0; param_block_index < num_param_blocks; ++param_block_index)
            {
                IParamBlock2* const param_block = texmap.GetParamBlock(param_block_index);
                if(param_block != nullptr)
                {
                    for(const ParamDef& param_def : *param_block)
                    {
                        if(param_def.type == TYPE_BITMAP)
                        {
                            PBBitmap* const pb_bitmap = param_block->GetBitmap(param_def.ID, t, validity);

                            const IPoint2 bitmap_resolution = (pb_bitmap != nullptr) 
                                ? IPoint2(pb_bitmap->bi.Width(), pb_bitmap->bi.Height())
                                // There's no bitmap, but we still need to bake (who knows, maybe the output parameters make it non-black?).
                                : IPoint2(1, 1);

                            resolution.x = std::max<int>(resolution.x, bitmap_resolution.x);
                            resolution.y = std::max<int>(resolution.y, bitmap_resolution.y);
                        }
                    }
                }
            }

            // Look for sub-texmaps
            const int num_sub_texmaps = texmap.NumSubTexmaps();
            for (int i = 0; i < num_sub_texmaps; ++i)
            {
                Texmap* const subTexmap = texmap.GetSubTexmap(i);
                if(subTexmap != nullptr) 
                {
                    const IPoint2 sub_texmap_resolution = get_bake_resolution(*subTexmap, t, validity);
                    resolution.x = std::max<int>(resolution.x, sub_texmap_resolution.x);
                    resolution.y = std::max<int>(resolution.y, sub_texmap_resolution.y);
                }
            }

            return resolution;
        }
    }
}

namespace MaxSDK
{;
namespace RenderingAPI
{;

using namespace NotificationAPI;

BaseTranslator_Texmap::BaseTranslator_Texmap(Texmap* texmap, TranslatorGraphNode& graphNode)
    : Translator(graphNode),
    BaseTranslator_MtlBase(texmap, false, graphNode),
    m_texmap(texmap)    
{
    // Monitor the texmap for changes
    if(m_texmap != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->MonitorTexmap(
                *m_texmap, 
                EventType_Texmap_Deleted | EventType_Texmap_ParamBlock | EventType_Texmap_Uncategorized,
                *this,
                nullptr);
        }
    }
}

BaseTranslator_Texmap::~BaseTranslator_Texmap()
{
    // Stop monitoring the material for changes
    if(m_texmap != nullptr)
    {
        IImmediateInteractiveRenderingClient* const notifications = GetRenderSessionContext().GetNotificationClient();
        if(notifications != nullptr)
        {
            notifications->StopMonitoringTexmap(*m_texmap, ~size_t(0), *this, nullptr);
        }
    }
}

void BaseTranslator_Texmap::NotificationCallback_NotifyEvent(const IGenericEvent& genericEvent, void* userData) 
{
    switch(genericEvent.GetEventType())
    {
    case EventType_Texmap_Deleted:
        TranslatedObjectDeleted();
        break;
    case EventType_Texmap_ParamBlock:
    case EventType_Texmap_Uncategorized:
        Invalidate();
        break;
    default:
        // Ignore these events
        break;
    }

    __super::NotificationCallback_NotifyEvent(genericEvent, userData);
}

Texmap* BaseTranslator_Texmap::GetTexmap() const
{
    return m_texmap;
}

bool BaseTranslator_Texmap::BakeTexmap(
    Texmap& texmap, 
    const TimeValue t, 
    Interval& validity, 
    const IPoint2& default_resolution, 
    IPoint2& baked_resolution,
    std::vector<BMM_Color_fl>& pixel_data)
{
    // Update the Texmap and get its validity
    texmap.Update(t, validity);

    // Determine the resolution  at which to bake
    IPoint2 bitmap_resolution = get_bake_resolution(texmap, t, validity);
    if((bitmap_resolution.x == 0) || (bitmap_resolution.y == 0))
    {
        bitmap_resolution = default_resolution;
    }

    // Create a temporary bitmap, to which we'll be baking
    BitmapInfo bmInfo;
    bmInfo.SetWidth(bitmap_resolution.x);
    bmInfo.SetHeight(bitmap_resolution.y);
    bmInfo.SetType(BMM_FLOAT_RGBA_32);
    Bitmap* const bm = TheManager->Create(&bmInfo);
    if(DbgVerify(bm != nullptr))
    {
        // Bake the texture
        GetCOREInterface()->RenderTexmap(&texmap, bm, 1.0f, false, false, 0.0f, t, true);

        // Extract the pixel buffer
        const bool success = ExtractBitmap(*bm, baked_resolution, pixel_data);
        DbgAssert(bitmap_resolution == baked_resolution);

        // Get rid of the temporary bitmap
        bm->DeleteThis();

        return success;
    }
    else
    {
        return false;
    }
}

bool BaseTranslator_Texmap::ExtractBitmap(
    Bitmap& bitmap, 
    IPoint2& resolution,
    std::vector<BMM_Color_fl>& pixel_data)
{
    resolution = IPoint2(bitmap.Width(), bitmap.Height());
    pixel_data.resize(resolution.x * resolution.y);

    for (int row=0; row < resolution.y; ++row) 
    {
        if(bitmap.GetLinearPixels(0, row, resolution.x, &pixel_data[row*resolution.x]) == 0)
        {
            return false;
        }
    }

    return true;
}

bool BaseTranslator_Texmap::ExtractBitmap(
    Bitmap& bitmap, 
    IPoint2& resolution,
    std::vector<BMM_Color_64>& pixel_data)
{
    resolution = IPoint2(bitmap.Width(), bitmap.Height());
    pixel_data.resize(resolution.x * resolution.y);

    for (int row=0; row < resolution.y; ++row) 
    {
        if(bitmap.GetLinearPixels(0, row, resolution.x, &pixel_data[row*resolution.x]) == 0)
        {
            return false;
        }
    }

    return true;
}

}}	// namespace 


