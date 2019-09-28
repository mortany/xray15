//////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2015 Autodesk, Inc.  All rights reserved.
//
//  Use of this software is subject to the terms of the Autodesk license 
//  agreement provided at the time of installation or download, or which 
//  otherwise accompanies this software in either electronic or hard copy form.   
//
//////////////////////////////////////////////////////////////////////////////
#pragma once

// RTI
#include <rti/core/IRenderJob.h>
// std
#include <memory>

#ifdef RAPIDRT_CORE_MODULE
	#define RapidRTCoreExport __declspec(dllexport)
#else
	#define RapidRTCoreExport __declspec(dllimport)
#endif

namespace Max 
{;
namespace RapidRTCore
{;

// Interface for accessing the core functionality of RapidRT.
class IRapidAPIManager
{
public:

    class IRenderJob;

    // Returns the singleton for this class
    RapidRTCoreExport static IRapidAPIManager& get_instance();

    // Creates a new instance of a render job
    virtual std::unique_ptr<IRenderJob> create_render_job(        
        // Set to true to free one thread when rendering locally, to optimize for interactivity.
        const bool free_one_thread) = 0;

protected:

    // Disallow destruction through this interface
    virtual ~IRapidAPIManager() {}

};

// Encapsulates a RTI render job
class IRapidAPIManager::IRenderJob
{
public:
    
    virtual ~IRenderJob() {}

    // Returns the RTI render job that's encapsulated by this class. The RTI job is only valid until this IRenderJob is destroyed.
    virtual rti::IRenderJob& get_render_job() = 0;
};

}}  // namespace