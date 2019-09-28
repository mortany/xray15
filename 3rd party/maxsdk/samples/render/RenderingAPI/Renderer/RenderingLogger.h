//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma	once

#include <RenderingAPI/Renderer/IRenderingLogger.h>
#include <Rendering/IRenderMessageManager.h>
#include <Noncopyable.h>

#include <set>

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

class RenderingLogger :
    public IRenderingLogger,
    public MaxSDK::Util::Noncopyable
{
public:

    RenderingLogger(const IRenderMessageManager::MessageSource initial_message_source);
    virtual ~RenderingLogger();

    // Flushes the render log to the IU and file. Only to be called from the main thread.
    void flush_contents();

    // Replaces the message source identifier
    void set_message_source(const IRenderMessageManager::MessageSource source);

    // -- from IRenderingLogger
    virtual void LogMessage(const MessageType type, const MCHAR* message) override;
    virtual void LogMessageOnce(const MessageType type, const MCHAR* message) override;

private:

    IRenderMessageManager* const m_render_message_manager;
    IRenderMessageManager::MessageSource m_message_source;

    // This list is used to eliminate duplicate log messages in LogMessageOnce()
    std::set<MSTR> m_logged_unique_messages;
};

}}  // namespace