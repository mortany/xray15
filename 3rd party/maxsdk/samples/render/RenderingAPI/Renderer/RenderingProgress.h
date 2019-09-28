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

// local
#include "RenderingLogger.h"

// Max SDK
#include <StopWatch.h>
#include <RenderingAPI/Renderer/IRenderingProcess.h>
#include <Noncopyable.h>
#include <Box2.h>
#include <nametab.h>
#include <Util\MainThreadTaskManager.h>

// std
#include <vector>
#include <map>
#pragma warning(push)
#pragma warning(disable: 4265)      // <mutex> generates warning, great
#include <mutex>
#pragma warning(pop)
#include <condition_variable>

class RendProgressCallback;

namespace Max
{;
namespace RenderingAPI
{;

using namespace MaxSDK::RenderingAPI;

class RenderingProgress :
    public IRenderingProcess,
    public MaxSDK::Util::Noncopyable
{
public:

    RenderingProgress(
        RenderingLogger& rendering_logger,
        RendProgressCallback* const progress_callback);
    virtual ~RenderingProgress();

    void set_progress_callback(RendProgressCallback* const progress_callback);
    RendProgressCallback* get_progress_callback() const;

    // Sets the abort state, requesting that the renderer stops.
    // This can be called asynchronously from any thread.
    void set_abort();

    // Reports that the rendered bitmap is about to be updated for display
    void report_bitmap_update();

    // Reports a table with the accumulated timing statistics
    void report_timing_statistics();

    // Reports the missing UVW channels that were reported during translation.
    void process_reported_missing_uvw_channels(const HWND parent_hwnd);

    // -- inherited from IRenderingProcess
    virtual void SetRenderingProgressTitle(const MCHAR* title) override;
    virtual void SetRenderingProgress(const size_t done, const size_t total, const ProgressType progress_type) override;
    virtual void SetInfiniteProgress(const size_t done, const ProgressType progresstType) override;
    virtual void SetSceneStats(int nlights, int nrayTraced, int nshadowed, int nobj, int nfaces) override;
    virtual void ReportMissingUVWChannel(INode& node, const unsigned int channel_index) override;
    virtual bool HasAbortBeenRequested() override;
    virtual void CancelMainThreadJobs() override;
    virtual bool RunJobFromMainThread(IMainThreadJob& job) override;
    virtual void TranslationStarted() override;
    virtual void TranslationFinished() override;
    virtual void RenderingStarted() override;
    virtual void RenderingPaused() override;
    virtual void RenderingResumed() override;
    virtual void RenderingFinished() override;
    virtual float GetTranslationTimeInSeconds() const override;
    virtual float GetRenderingTimeInSeconds() const override;
    virtual void StartTimer(const MSTR& timer_name) override;
    virtual void StopTimer(const MSTR& timer_name) override;
    virtual float GetTimerElapsedSeconds(const MCHAR* timer_name) const override;

private:

    class MainThreadJobContainer;
    class MainThreadExecuteRequest;
    class ProgressUpdate_MainThreadJob;

    // Called once an execute request has successfully executed, to remove it from the list
    // To be called from the main thread only.
    void execute_request_completed(MainThreadExecuteRequest& request);

    // Resets the cached progress such that we won't report erroneous progress information
    void reset_cached_progress();

private:

    // Magic value to identify infinite progress through the "total"
    static const size_t m_infinite_progress_total;

    RenderingLogger& m_rendering_logger;
    RendProgressCallback* m_progress_callback;

    // Whether an abort has been requested
    bool m_abort;

    // Caches the progress last reported, such that we can artificially report the progress at regular intervals in order to force the Max window
    // message to be processed regularly, as well as check for aborts regularly.
    size_t m_cached_progress_done;
    size_t m_cached_progress_total;
    ProgressType m_cached_progress_type;

    // Whether new main thread jobs are accepted
    bool m_main_thread_jobs_accepted;
    // The set of main thread execute requests which have been posted
    std::vector<MainThreadExecuteRequest*> m_main_thread_execute_requests;
    // Lock which controls access to main thread jobs
    std::mutex m_main_thread_jobs_lock;

    // Various timers 
    MaxSDK::Util::StopWatch m_translation_stopwatch;
    MaxSDK::Util::StopWatch m_rendering_stopwatch;

    // Arbitrary timers, identified by name
    typedef std::map<MSTR, MaxSDK::Util::StopWatch> NamedTimerMap;
    NamedTimerMap m_named_timers;

    bool m_first_bitmap_update;

    // Set of missing UVW channel message strings, accumulated while translating the scene, and meant to be passed to CommonRendererUI::DoMissingUVWChannelsDialog()
    NameTab m_missing_uvw_entries;
};

// This is the container for a main thread job in the queue. It gets allocated on the task, allowing its owner
// to safely determine when the job has finished executing.
class RenderingProgress::MainThreadJobContainer :
    public MaxSDK::Util::Noncopyable
{
public:

    MainThreadJobContainer(IMainThreadJob& job);

    // Executes the job
    void execute_job();
    // Cancels the job
    void cancel_job();

    bool was_job_executed() const;

    // Waits until the job is either executed or canceled
    void wait_until_executed_or_canceled();

private:

    // The actual job to execute
    IMainThreadJob& m_job;
    // This pair of conditional_variable and mutex implement an event object to notify waiting threads
    // of the job's completion or cancellation
    mutable std::condition_variable m_completion_condition_variable;
    mutable std::mutex m_completion_mutex;
    bool m_job_cancelled;
    bool m_job_completed;
};

// This is the main thread job which gets sent to the core subystem for execution from the main thread.
// It is allocated dynamically and destroyed by the core system once the job has been executed.
class RenderingProgress::MainThreadExecuteRequest : 
    public MainThreadTask,
    public MaxSDK::Util::Noncopyable
{
public:

    MainThreadExecuteRequest(MainThreadJobContainer& job_container, RenderingProgress& rendering_progress);

    //  Cancels this request.
    void cancel();

    // -- inherited from MainThreadTask
    virtual void Execute() override;

private:

    std::mutex m_lock;
    MainThreadJobContainer* m_job_container;
    RenderingProgress* m_rendering_process;
};

// This job handles updating the progress from the main thread
class RenderingProgress::ProgressUpdate_MainThreadJob : 
    public IMainThreadJob,
    public MaxSDK::Util::Noncopyable
{
public:

    ProgressUpdate_MainThreadJob(RendProgressCallback& progress_callback, RenderingLogger& rendering_logger, const TCHAR* title);
    ProgressUpdate_MainThreadJob(RendProgressCallback& progress_callback, RenderingLogger& rendering_logger, const size_t done, const size_t total, const ProgressType progress_type);
    virtual ~ProgressUpdate_MainThreadJob();

    // Returns whether the progress callback request an abort
    bool abort_requested() const;

    // -- from IMainThreadJob
    virtual void ExecuteMainThreadJob();

private:

    const bool m_is_title_job;
    const TCHAR* m_title;
    const size_t m_done;
    const size_t m_total;
    const ProgressType m_progress_type;
    RendProgressCallback& m_progress_callback;
    RenderingLogger& m_rendering_logger;
    bool m_abort_request;
};


}}  // namespace