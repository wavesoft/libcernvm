/**
 * This file is part of CernVM Web API Plugin.
 *
 * CVMWebAPI is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CVMWebAPI is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CVMWebAPI. If not, see <http://www.gnu.org/licenses/>.
 *
 * Developed by Ioannis Charalampidis 2013
 * Contact: <ioannis.charalampidis[at]cern.ch>
 */

#pragma once
#ifndef VBOXSESSION_H
#define VBOXSESSION_H

#include "VBoxCommon.h"

#include <string>
#include <map>

#include <CernVM/SimpleFSM.h>
#include <CernVM/Hypervisor.h>
#include <CernVM/CrashReport.h>

#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

/**
 * Mount/Unmount disk constants
 */
enum VBoxDiskType {
    T_HDD,      // Hard disk
    T_DVD,      // DVD-ROM Drive
    T_FLOPPY    // A Floppy disk drive
};

/**
 * Virtualbox Session, built around a Finite-State-Machine model
 */
class VBoxSession : public SimpleFSM, public HVSession {
public:

    VBoxSession( ParameterMapPtr param, HVInstancePtr hv ) : SimpleFSM(), HVSession(param, hv), execConfig() {
        CRASH_REPORT_BEGIN;

        FSM_REGISTRY(1,             // Entry point is on '1'
        {

            // Target states
            FSM_STATE(1, 100);      // Entry point
            FSM_STATE(2, 102,112);  // Error
            FSM_STATE(3, 104);      // Destroyed
            FSM_STATE(4, 105,108);  // Power off
            FSM_STATE(5, 107,211);  // Saved
            FSM_STATE(6, 109,111);  // Paused
            FSM_STATE(7, 110,106);  // Running

            // 100: INITIALIZE HYPERVISOR
            FSM_HANDLER(100, &VBoxSession::Initialize,              101);

            // 101: UPDATE SESSION STATE FROM THE HYPERVISOR
            FSM_HANDLER(101, &VBoxSession::UpdateSession,           2,3,4,5,6,7);

            // 102: HANDLE ERROR SEQUENCE
            FSM_HANDLER(102, &VBoxSession::HandleError,             103);
                FSM_HANDLER(103, &VBoxSession::CureError,           101);       // Try to recover error and recheck state

            // 104: CREATE SEQUENCE
            FSM_HANDLER(104, &VBoxSession::CreateVM,                4);         // Create new VM

            // 105: DESTROY SEQUENCE
            FSM_HANDLER(105, &VBoxSession::ReleaseVMScratch,        207);       // Release Scratch storage
                FSM_HANDLER(207, &VBoxSession::ReleaseVMBoot,       208);       // Release Boot Media
                FSM_HANDLER(208, &VBoxSession::DestroyVM,           3);         // Destroy VM

            // 106: POWEROFF SEQUENCE
            FSM_HANDLER(106, &VBoxSession::PoweroffVM,              209);       // Power off the VM
                FSM_HANDLER(209, &VBoxSession::ReleaseVMAPI,        4);         // Release the VM API media

            // 211: CHECK VMAPI STATE
            FSM_HANDLER(211, &VBoxSession::CheckVMAPI,              206);       // Check if we can resume from current VMAPI Data of we should restart

            // 107: DISCARD STATE SEQUENCE
            FSM_HANDLER(107, &VBoxSession::DiscardVMState,          209);       // Discard saved state of the VM
                FSM_HANDLER(209, &VBoxSession::ReleaseVMAPI,        4);         // Release the VM API media

            // 108: START SEQUENCE
            FSM_HANDLER(108, &VBoxSession::PrepareVMBoot,           210);       // Prepare start parameters
                FSM_HANDLER(210, &VBoxSession::ConfigNetwork,       201);       // Configure the network devices
                FSM_HANDLER(201, &VBoxSession::ConfigureVM,         202);       // Configure VM
                FSM_HANDLER(202, &VBoxSession::DownloadMedia,       203);       // Download required media files
                FSM_HANDLER(203, &VBoxSession::ConfigureVMBoot,     204);       // Configure Boot media
                FSM_HANDLER(204, &VBoxSession::ConfigureVMScratch,  205);       // Configure Scratch storage
                FSM_HANDLER(205, &VBoxSession::ConfigureVMAPI,      206);       // Configure API Disks
                FSM_HANDLER(206, &VBoxSession::StartVM,             7);         // Launch the VM

            // 109: SAVE STATE SEQUENCE
            FSM_HANDLER(109, &VBoxSession::SaveVMState,             5);         // Save VM state

            // 110: PAUSE SEQUENCE
            FSM_HANDLER(110, &VBoxSession::PauseVM,                 6);         // Pause VM

            // 111: PAUSE SEQUENCE
            FSM_HANDLER(111, &VBoxSession::ResumeVM,                7);         // Resume VM

            // 112: FATAL ERROR HANDLING
            FSM_HANDLER(112, &VBoxSession::FatalErrorSink,          0);         // Fatal Error Sink

        });

        // Reset error states
        errorCount = 0;
        errorTimestamp = 0;
        errorCode = 0;
        errorMessage = "";
        lastMachineInfoTimestamp = 0;
        isAborting = false;

        CRASH_REPORT_END;
    }

    /////////////////////////////////////
    // FSM implementation functions 
    /////////////////////////////////////

    void Initialize();
    void UpdateSession();
    void HandleError();
    void CureError();
    void CreateVM();
    void ConfigureVM();
    void DownloadMedia();
    void ConfigureVMBoot();
    void ReleaseVMBoot();
    void ConfigureVMScratch();
    void ReleaseVMScratch();
    void ConfigureVMAPI();
    void ReleaseVMAPI();
    void PrepareVMBoot();
    void DestroyVM();
    void PoweroffVM();
    void DiscardVMState();
    void StartVM();
    void SaveVMState();
    void PauseVM();
    void ResumeVM();
    void FatalErrorSink();
    void ConfigNetwork();
    void CheckVMAPI();
    void CheckIntegrity();

    /////////////////////////////////////
    // HVSession Implementation
    /////////////////////////////////////

    virtual int             pause               ();
    virtual int             close               ( bool unmonitored = false );
    virtual int             resume              ();
    virtual int             reset               ();
    virtual int             stop                ();
    virtual int             hibernate           ();
    virtual int             open                ();
    virtual int             start               ( const ParameterMapPtr& userData );
    virtual int             setExecutionCap     ( int cap);
    virtual int             setProperty         ( std::string name, std::string key );
    virtual std::string     getProperty         ( std::string name );
    virtual std::string     getRDPAddress       ();
    virtual std::string     getExtraInfo        ( int extraInfo );
    virtual std::string     getAPIHost          ();
    virtual int             getAPIPort          ();
    virtual void            abort               ();
    virtual int             update              ( bool waitTillInactive = true );
    virtual void            wait                ( );

    /////////////////////////////////////
    // External updates feedback
    /////////////////////////////////////

    /**
     * Notification from the VBoxInstance that the session
     * has been destroyed from an external source.
     */
    void                    hvNotifyDestroyed   ();

    /**
     * Notification from the VBoxInstance that we are going
     * for a forceful shutdown. We should cleanup everything
     * without raising any alert during the handling.
     */
    void                    hvStop              ();

    /**
     *  Compile the user data and return it's string representation
     */
    std::string             getUserData         ();

    /**
     * Override to get notified when state is changed
     */
    void                    FSMEnteringState    ( const int state, const bool final );

protected:

    /////////////////////////////////////
    // Tool functions
    /////////////////////////////////////

    /**
     * Execute the specified command using the hypervisor binary as base
     */
    int                     wrapExec            ( std::string cmd, 
                                                  std::vector<std::string> * stdoutList, 
                                                  std::string * stderrMsg, 
                                                  const SysExecConfig& config );

    /**
     * Destroy and unregister VM
     */
    int                     destroyVM           ( );

    /**
     * (Re-)Mount a disk on the specified controller
     * This function automatically unmounts a previously attached disk if the filenames
     * do not match.
     */
    int                     mountDisk           ( const std::string & controller, const std::string & port, const std::string & device, const VBoxDiskType& type, const std::string & file, bool multiAttach = false );

    /**
     * Unmount a medium from the VirtulaBox Instance
     */
    int                     unmountDisk         ( const std::string & controller, const std::string & port, const std::string & device, const VBoxDiskType& type, const bool deleteFile = false );

    /**
     * Forward the fact that an error has occured somewhere in the FSM handling
     */
    void                    errorOccured        ( const std::string & str, int errNo );

    /**
     * Shorthand function for calling controlVM actions with predefined vboxid
     */
    int                     controlVM           ( std::string how, int timeout = SYSEXEC_TIMEOUT );

    int                     getMachineUUID      ( std::string mname, std::string * ans_uuid,  int flags );
    std::string             getDataFolder       ();
    int                     getHostOnlyAdapter  ( std::string * adapterName, const FiniteTaskPtr & fp = FiniteTaskPtr() );
    std::map<std::string, 
        std::string>        getMachineInfo      ( const std::string& machineName = "", int retries = 2, int timeout = SYSEXEC_TIMEOUT );
    std::map<std::string, 
        std::string>        getDiskInfo         ( const std::string& disk );

    int                     startVM             ();

    ////////////////////////////////////
    // Local variables
    ////////////////////////////////////
    
    std::string             dataPath;
    bool                    isAborting;

    // Error handling and healing variables
    int                     errorCode;
    std::string             errorMessage;
    int                     errorCount;
    unsigned long           errorTimestamp;

    // getMachineInfo helpers
    std::map<std::string,
        std::string>        lastMachineInfo;
    long                    lastMachineInfoTimestamp;

    // Detection of virtualbox log modification time
    unsigned long long      lastLogTime;

    // For having only a single system command running
    boost::mutex            execMutex;

    /*  Default sysExecConfig */
    SysExecConfig           execConfig;

};


#endif /* end of include guard: VBOXSESSION_H */
