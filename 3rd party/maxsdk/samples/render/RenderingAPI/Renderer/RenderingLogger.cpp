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

#include "RenderingLogger.h"

#include "3dsmax_banned.h"

namespace
{
    bool is_main_thread()
    {
        return (GetCOREInterface15()->GetMainThreadID() == GetCurrentThreadId());
    }
}

namespace MaxSDK
{;
namespace RenderingAPI
{;

std::unique_ptr<IRenderingLogger> IRenderingLogger::AllocateInstance(const IRenderMessageManager::MessageSource initial_message_source)
{
    return std::unique_ptr<IRenderingLogger>(new Max::RenderingAPI::RenderingLogger(initial_message_source));
}

}
}

namespace Max
{;
namespace RenderingAPI
{;

RenderingLogger::RenderingLogger(const IRenderMessageManager::MessageSource initial_message_source)
    : m_render_message_manager(GetRenderMessageManager()),
    m_message_source(initial_message_source)
{
    // Open the log file 
    if(m_render_message_manager != nullptr)
    {
        m_render_message_manager->OpenLogFile(m_message_source);
    }
}

RenderingLogger::~RenderingLogger()
{
    // Flush the contents of the log
    flush_contents();

    // Close the log file 
    if(m_render_message_manager != nullptr)
    {
        m_render_message_manager->CloseLogFile(m_message_source);
    }
}

void RenderingLogger::flush_contents()
{
    // Only to be called from the main thread
    if(DbgVerify(is_main_thread()))
    {
        if(m_render_message_manager != nullptr)
        {
            // Flush the message window
            m_render_message_manager->FlushPendingMessages(m_message_source);

            // Flush the log file
            m_render_message_manager->FlushLogFile(m_message_source);
        }
    }
}

void RenderingLogger::set_message_source(const IRenderMessageManager::MessageSource new_source)
{
    if(m_message_source != new_source)
    {
        flush_contents();

        // Close and re-open the log file for the new message source
        if(m_render_message_manager != nullptr)
        {
            m_render_message_manager->CloseLogFile(m_message_source);
            m_render_message_manager->OpenLogFile(new_source);
        }

        m_message_source = new_source;
    }
}

void RenderingLogger::LogMessage(const MessageType type, const MCHAR* message)
{
    if(m_render_message_manager != nullptr)
    {
        IRenderMessageManager::MessageType message_type;
        DWORD system_type;
        switch(type)
        {
        case MessageType::Progress:
            message_type = IRenderMessageManager::kType_Progress;
            system_type = SYSLOG_INFO;
            break;
        case MessageType::Info:
            message_type = IRenderMessageManager::kType_Info;
            system_type = SYSLOG_INFO;
            break;
        case MessageType::Warning:
            message_type = IRenderMessageManager::kType_Warning;
            system_type = SYSLOG_WARN;
            break;
        case MessageType::Error:
            message_type = IRenderMessageManager::kType_Error;
            system_type = SYSLOG_WARN;      // Report as warning to the system, to avoid aborting backburner
            break;
        default:
            DbgAssert(false);
            // fall into...
        case MessageType::Debug:
#ifdef _DEBUG
            // In debug builds, send debug messages as info, such that they show up in the render message dialog.
            message_type = IRenderMessageManager::kType_Info;
            system_type = SYSLOG_INFO;
#else
            message_type = IRenderMessageManager::kType_Debug;
            system_type = SYSLOG_DEBUG;
#endif
            break;
        }

        m_render_message_manager->LogMessage(m_message_source, message_type, (system_type | SYSLOG_BROADCAST), message);
    }
}

void RenderingLogger::LogMessageOnce(const MessageType type, const MCHAR* message)
{
    // Check if this message has already been logged
    const MSTR message_mstr(message);
    if(m_logged_unique_messages.insert(message_mstr).second)
    {
        // Not already logged
        LogMessage(type, message);
    }
    else
    {
        // Already logged: do nothing
    }
}



}}  // namespace