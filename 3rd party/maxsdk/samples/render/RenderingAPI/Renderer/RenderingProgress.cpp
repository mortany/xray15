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

#include "RenderingProgress.h"

#include "../resource.h"

#include <maxapi.h>
#include <render.h>
#include <interactiverender.h>
#include <dllutilities.h>
#include <Rendering/INoSignalCheckProgress.h>
#include <Rendering/CommonRendererUI.h>

#include "3dsmax_banned.h"

using namespace MaxSDK;
using namespace std;

namespace
{
    bool is_main_thread()
    {
        return (GetCOREInterface15()->GetMainThreadID() == GetCurrentThreadId());
    }
}

namespace Max
{;
namespace RenderingAPI
{;

const size_t RenderingProgress::m_infinite_progress_total = ~size_t(0);     // -1 in size_t

RenderingProgress::RenderingProgress(
    RenderingLogger& rendering_logger,
    RendProgressCallback* const progress_callback)

    : m_progress_callback(progress_callback),
    m_abort(false),
    m_main_thread_jobs_accepted(true),
    m_first_bitmap_update(false),
    m_rendering_logger(rendering_logger),
    m_cached_progress_done(0),
    m_cached_progress_total(0),
    m_cached_progress_type(ProgressType::Translation)
{

}

RenderingProgress::~RenderingProgress()
{
    DbgAssert(m_main_thread_execute_requests.empty());

    // Report all the named timers
    report_timing_statistics();
}

void RenderingProgress::set_progress_callback(RendProgressCallback* const progress_callback)
{
    m_progress_callback = progress_callback;
}

RendProgressCallback* RenderingProgress::get_progress_callback() const
{
    return m_progress_callback;
}

void RenderingProgress::SetRenderingProgressTitle(const MCHAR* title)
{
    if(DbgVerify(m_progress_callback != nullptr))
    {
        // Execute the main thread job
        ProgressUpdate_MainThreadJob job(*m_progress_callback, m_rendering_logger, title);
        if(!RunJobFromMainThread(job) || job.abort_requested())
        {
            set_abort();
        }
    }
}

void RenderingProgress::SetRenderingProgress(const size_t done, const size_t total, const ProgressType progress_type)
{
    // Cache the progress information for later use
    m_cached_progress_done = done;
    m_cached_progress_total = total;
    m_cached_progress_type = progress_type;

    if(DbgVerify(m_progress_callback != nullptr))
    {
        // Execute the main thread job
        ProgressUpdate_MainThreadJob job(*m_progress_callback, m_rendering_logger,  done, total, progress_type);
        if(!RunJobFromMainThread(job) || job.abort_requested())
        {
            set_abort();
        }
    }
}

void RenderingProgress::SetInfiniteProgress(const size_t done, const ProgressType progress_type)
{
    SetRenderingProgress(done, m_infinite_progress_total, progress_type);
}

void RenderingProgress::SetSceneStats(int nlights, int nrayTraced, int nshadowed, int nobj, int nfaces)
{
    if(DbgVerify(m_progress_callback != nullptr))
    {
        m_progress_callback->SetSceneStats(nlights, nrayTraced, nshadowed, nobj, nfaces);
    }        
}

void RenderingProgress::ReportMissingUVWChannel(INode& node, const unsigned int channel_index)
{
    MSTR msg;
    const MCHAR* node_name = node.GetName();
    //!! TODO: Localize, which can't be done now that we're in UI freeze
    msg.printf(_T("(UVW %u): %s"), channel_index, (node_name != nullptr) ? node_name : _T("<unnamed object>"));
    m_missing_uvw_entries.AddName(msg);
}

void RenderingProgress::process_reported_missing_uvw_channels(const HWND parent_hwnd)
{
    if(m_missing_uvw_entries.Count() > 0)
    {
        if(!CommonRendererUI::DoMissingUVWChannelsDialog(m_missing_uvw_entries, parent_hwnd))
        {
            set_abort();
        }
        m_missing_uvw_entries.ZeroCount();
    }
}

bool RenderingProgress::HasAbortBeenRequested()
{
    // We artificially re-report the last cached progress information. This results in the Max window messages to be processed, making the UI
    // responsive but also forcing a check for the abort button.
    SetRenderingProgress(m_cached_progress_done, m_cached_progress_total, m_cached_progress_type);

    return m_abort;
}

bool RenderingProgress::RunJobFromMainThread(IMainThreadJob& job)
{
    if(is_main_thread())
    {
        // Already in main thread: execute immediately

        // Start by executing pending jobs. This is to prevent deadlocks if doing concurrent rendering from the main thread and from another
        // thread (e.g. MEdit + ActiveShade).
        IMainThreadTaskManager* main_thread_executor = IMainThreadTaskManager::GetInstance();
        if(DbgVerify(main_thread_executor != nullptr))
        {
            main_thread_executor->ExecutePendingTasks();
        }

        job.ExecuteMainThreadJob();
        return true;
    }
    else
    {
        // Create a container for our job
        MainThreadJobContainer job_container(job);
        bool job_enqueued = false;

        IMainThreadTaskManager* main_thread_executor = IMainThreadTaskManager::GetInstance();
        if(DbgVerify(main_thread_executor != nullptr))
        {
            lock_guard<mutex> guard(m_main_thread_jobs_lock);
            // Are we allowed to execute this job?
            if(m_main_thread_jobs_accepted)
            {
                // Enqueue the job
                MainThreadExecuteRequest* const new_request = new MainThreadExecuteRequest(job_container, *this);
                m_main_thread_execute_requests.push_back(new_request);
                main_thread_executor->PostTask(new_request);

                job_enqueued = true;
            }
        }

        if(job_enqueued)
        {
            // Wait for the job to be executed
            job_container.wait_until_executed_or_canceled();
        }

        return job_container.was_job_executed();
    }
}

void RenderingProgress::set_abort()
{
    m_abort = true;
}

void RenderingProgress::CancelMainThreadJobs()
{
    DbgAssert(is_main_thread());

    lock_guard<mutex> guard(m_main_thread_jobs_lock);

    m_main_thread_jobs_accepted = false;
    m_abort = true;
    
    for(MainThreadExecuteRequest* request : m_main_thread_execute_requests)
    {
        // Cancel the request
        // Here we lock both MainThreadExecuteRequest::m_lock and RenderingProgress::m_main_thread_jobs_lock, which we also do
        // in one another place - that could result in a deadlock, but fortunately both code paths are only ever run in the main thread,
        // which makes deadlocks impossible.
        request->cancel();
    }

    // Clear the list of requests - they're all canceled so we forget about them
    m_main_thread_execute_requests.clear();
}

void RenderingProgress::execute_request_completed(MainThreadExecuteRequest& request)
{
    DbgAssert(is_main_thread());

    // Here we lock both MainThreadExecuteRequest::m_lock and RenderingProgress::m_main_thread_jobs_lock, which we also do
    // in one another place - that could result in a deadlock, but fortunately both code paths are only ever run in the main thread,
    // which makes deadlocks impossible.
    lock_guard<mutex> guard(m_main_thread_jobs_lock);

    for(auto it = m_main_thread_execute_requests.begin(); it != m_main_thread_execute_requests.end(); ++it)
    {
        if(*it == &request)
        {
            m_main_thread_execute_requests.erase(it);
            return;
        }
    }

    // Didn't find it? Bug.
    DbgAssert(false);
}

void RenderingProgress::reset_cached_progress()
{
    m_cached_progress_done = 0;
    m_cached_progress_total = 0;
    m_cached_progress_type = ProgressType::Translation;
}

void RenderingProgress::TranslationStarted()
{
    // Reset cached progress to avoid reporting obsolete progress information
    reset_cached_progress();

    m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_LOG_TRANSLATING));
    
    // Start timer
    m_translation_stopwatch.Reset();
    m_translation_stopwatch.Start();
}

void RenderingProgress::TranslationFinished()
{
    // Reset cached progress to avoid reporting obsolete progress information
    reset_cached_progress();

    if(m_translation_stopwatch.IsRunning())
    {
        m_translation_stopwatch.Stop();

        TSTR msg;
        msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_LOG_SCENE_TRANSLATION_TIME), m_translation_stopwatch.GetElapsedTime() * 1e-3f);
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, msg);
    }
}

void RenderingProgress::RenderingStarted()
{
    // Reset cached progress to avoid reporting obsolete progress information
    reset_cached_progress();

    SetRenderingProgressTitle(MaxSDK::GetResourceStringAsMSTR(IDS_LOG_RENDERING));
    
    // Start timer
    m_rendering_stopwatch.Reset();
    m_rendering_stopwatch.Start();
    m_first_bitmap_update = true;
}

void RenderingProgress::RenderingPaused() 
{
    if(m_rendering_stopwatch.IsRunning())
    {
        m_rendering_stopwatch.Pause();
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_LOG_RENDERER_PAUSED));
    }
}

void RenderingProgress::RenderingResumed() 
{
    if(m_rendering_stopwatch.IsPaused())
    {
        m_rendering_stopwatch.Resume();
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, MaxSDK::GetResourceStringAsMSTR(IDS_LOG_RENDERER_UNPAUSED));
    }
}

void RenderingProgress::RenderingFinished()
{
    // Reset cached progress to avoid reporting obsolete progress information
    reset_cached_progress();

    if(m_rendering_stopwatch.IsRunning() || m_rendering_stopwatch.IsPaused())
    {
        m_rendering_stopwatch.Stop();

        TSTR msg;
        msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_LOG_RENDER_TIME), m_rendering_stopwatch.GetElapsedTime() * 1e-3f);
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, msg);
    }
}

float RenderingProgress::GetTranslationTimeInSeconds() const 
{
    return m_translation_stopwatch.GetElapsedTime() * 1e-3f;
}

float RenderingProgress::GetRenderingTimeInSeconds() const 
{
    return m_rendering_stopwatch.GetElapsedTime() * 1e-3f;
}

void RenderingProgress::report_bitmap_update()
{
    // Output time to first bitmap update
    if(m_first_bitmap_update && DbgVerify(m_rendering_stopwatch.IsRunning()))
    {
        TSTR msg;
        msg.printf(MaxSDK::GetResourceStringAsMSTR(IDS_LOG_FIRST_BITMAP_UPDATE_TIME), m_rendering_stopwatch.GetElapsedTime() * 1e-3f);
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, msg);
        m_first_bitmap_update = false;
    }
}

void RenderingProgress::StartTimer(const MSTR& timer_name) 
{
    // Ignore empty strings
    if(!timer_name.isNull())
    {
        // See if a timer already exists
        const auto found_it = m_named_timers.find(timer_name);
        if(found_it != m_named_timers.end())
        {
            // Resume the existing timer
            found_it->second.Resume();
        }
        else
        {
            // Create a new timer and start it
            const auto inserted_it = m_named_timers.insert(NamedTimerMap::value_type(timer_name, MaxSDK::Util::StopWatch())).first;
            inserted_it->second.Start();
        }
    }
}

void RenderingProgress::StopTimer(const MSTR& timer_name) 
{
    // Ignore empty strings
    if(!timer_name.isNull())
    {
        // See if a timer already exists
        const auto found_it = m_named_timers.find(timer_name);
        if(found_it != m_named_timers.end())
        {
            // Pause the existing timer
            found_it->second.Pause();
        }
        else
        {
            // Timer doesn't exist: bug
            DbgAssert(false);
        }
    }
}

float RenderingProgress::GetTimerElapsedSeconds(const MCHAR* timer_name) const 
{
    // See if a timer already exists
    const auto found_it = m_named_timers.find(timer_name);
    if(found_it != m_named_timers.end())
    {
        return found_it->second.GetElapsedTime() * 1e-3f;       // convert ms to s
    }
    else
    {
        // Timer doesn't exist: bug
        DbgAssert(false);
        return 0.0f;
    }
}

void RenderingProgress::report_timing_statistics()
{
    if(!m_named_timers.empty())
    {
        // Use another map to sort the list according to time values
        typedef std::multimap<float, const MCHAR*> SortedTimesMap;
        SortedTimesMap sorted_times;
        for(NamedTimerMap::const_iterator it = m_named_timers.begin(); it != m_named_timers.end(); ++it)
        {
            const float elapsed_seconds = it->second.GetElapsedTime() * 1e-3f;  // convert ms to s
            sorted_times.insert(SortedTimesMap::value_type(elapsed_seconds, it->first));
        }

        // Build the logging string
        MSTR report_string = MaxSDK::GetResourceStringAsMSTR(IDS_TIMING_TABLE_HEADER);
        MSTR temp_time_string;
        for(SortedTimesMap::const_reverse_iterator it = sorted_times.rbegin(); it != sorted_times.rend(); ++it)
        {
            temp_time_string.printf(_T("\n\t%s: \t%.3fs"), it->second, it->first);
            report_string += temp_time_string;
        }

        // Log the string
        m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Info, report_string);

        m_named_timers.clear();
    }
}

//==================================================================================================
// class RenderingProgress::MainThreadJobContainer
//==================================================================================================

RenderingProgress::MainThreadJobContainer::MainThreadJobContainer(IMainThreadJob& job)
    : m_job(job),
    m_job_cancelled(false),
    m_job_completed(false)
{

}

void RenderingProgress::MainThreadJobContainer::execute_job()
{
    unique_lock<mutex> guard(m_completion_mutex);

    if(DbgVerify(!m_job_completed && !m_job_cancelled))
    {
        m_job.ExecuteMainThreadJob();

        // Signal job completion
        m_job_completed = true;
        m_completion_condition_variable.notify_all();
    }
}

void RenderingProgress::MainThreadJobContainer::cancel_job()
{
    unique_lock<mutex> guard(m_completion_mutex);
    if(!m_job_completed && !m_job_cancelled)
    {
        // Signal job completion
        m_job_cancelled = true;
        m_completion_condition_variable.notify_all();
    }
}

bool RenderingProgress::MainThreadJobContainer::was_job_executed() const
{
    unique_lock<mutex> guard(m_completion_mutex);
    return m_job_completed;
}

void RenderingProgress::MainThreadJobContainer::wait_until_executed_or_canceled()
{
    unique_lock<mutex> guard(m_completion_mutex);
    m_completion_condition_variable.wait(guard, [this]{ return m_job_cancelled || m_job_completed; });

    DbgAssert(m_job_cancelled || m_job_completed);
}

//==================================================================================================
// class RenderingProgress::MainThreadProgressJob 
//==================================================================================================

RenderingProgress::MainThreadExecuteRequest::MainThreadExecuteRequest(MainThreadJobContainer& job_container, RenderingProgress& rendering_progress)
    :  m_job_container(&job_container),
    m_rendering_process(&rendering_progress)
{

}

void RenderingProgress::MainThreadExecuteRequest::cancel()
{
    DbgAssert(is_main_thread());

    // Lock to prevent simultaneously canceling and executing this job
    lock_guard<mutex> guard(m_lock);

    if(m_job_container != nullptr)
    {
        m_job_container->cancel_job();

        // Set the job to null to avoid it being run
        m_job_container = nullptr;
        m_rendering_process = nullptr;
    }
}

void RenderingProgress::MainThreadExecuteRequest::Execute()
{
    DbgAssert(is_main_thread());

    // Lock to prevent simultaneously canceling and executing this job
    lock_guard<mutex> guard(m_lock);

    if(m_job_container != nullptr)
    {
        m_job_container->execute_job();

        // Remove this request from the list as it's going to be deleted upon this method returning
        DbgAssert(m_rendering_process != nullptr);
        if(m_rendering_process != nullptr)
        {
            m_rendering_process->execute_request_completed(*this);
        }
    }
    else
    {
        // This job was canceled: just do nothing
    }
}

//==================================================================================================
// class RenderingProgress::MainThreadProgressJob 
//==================================================================================================

RenderingProgress::ProgressUpdate_MainThreadJob::ProgressUpdate_MainThreadJob(RendProgressCallback& progress_callback, RenderingLogger& rendering_logger, const TCHAR* title)
    : m_is_title_job(true),
    m_title(title),
    m_done(0),
    m_total(0),
    m_progress_type(ProgressType::Rendering),
    m_progress_callback(progress_callback),
    m_rendering_logger(rendering_logger),
    m_abort_request(false)
{

}

RenderingProgress::ProgressUpdate_MainThreadJob::ProgressUpdate_MainThreadJob(RendProgressCallback& progress_callback, RenderingLogger& rendering_logger, const size_t done, const size_t total, const ProgressType progress_type)
    : m_is_title_job(false),
    m_title(nullptr),
    m_done(done),
    m_total(total),
    m_progress_type(progress_type),
    m_progress_callback(progress_callback),
    m_rendering_logger(rendering_logger),
    m_abort_request(false)  
{

}

RenderingProgress::ProgressUpdate_MainThreadJob::~ProgressUpdate_MainThreadJob()
{

}

bool RenderingProgress::ProgressUpdate_MainThreadJob::abort_requested() const
{
    return m_abort_request;
}

void RenderingProgress::ProgressUpdate_MainThreadJob::ExecuteMainThreadJob()
{
    if(m_is_title_job)
    {
        m_progress_callback.SetTitle(m_title);
        IRenderProgressCallback* iprog = dynamic_cast<IRenderProgressCallback*>(&m_progress_callback);
        if(iprog != nullptr)
        {
            iprog->SetIRenderTitle(m_title);
        }

        // Also report as a message to the logger, if the string isn't empty
        if((m_title != nullptr) && (m_title[0] != _T('\0')))
        {
            m_rendering_logger.LogMessage(IRenderingLogger::MessageType::Progress, m_title);
        }
    }
    else
    {
        IRenderProgressCallback* iprog = dynamic_cast<IRenderProgressCallback*>(&m_progress_callback);
        if(iprog != nullptr)
        {
            const IRenderProgressCallback::LineOrientation line_orientation = (m_progress_type == ProgressType::Translation) ? IRenderProgressCallback::LO_Horizontal : IRenderProgressCallback::LO_Vertical;
            iprog->SetProgressLineOrientation(line_orientation);
        }

        // Use the 'no-signals' version of the progress callback, if it's available. If we don't use it, then the active shade progress callback will 
        // process and filter window messages, discarding any messages that don't belong to the active shade window. This causes some glitch with mouse
        // interaction, etc.
        const bool infinite_progress = (m_total == m_infinite_progress_total);
        INoSignalCheckProgress* const no_signals_progress_callback = dynamic_cast<INoSignalCheckProgress*>(&m_progress_callback);
        if(no_signals_progress_callback != nullptr)
        {
            no_signals_progress_callback->UpdateProgress(static_cast<int>(m_done), infinite_progress ? -1 : static_cast<int>(m_total));
        }
        else
        {
            m_progress_callback.SetStep((m_progress_type == ProgressType::Translation) ? 1 : 2, 2);

            // Use the regular progress callback - probably because we're not in active shade.
            if(m_progress_callback.Progress(static_cast<int>(m_done), infinite_progress ? -1 : static_cast<int>(m_total)) == RENDPROG_ABORT)
            {
                m_abort_request = true;
            }
        }
    }    
}

}}  // namespace