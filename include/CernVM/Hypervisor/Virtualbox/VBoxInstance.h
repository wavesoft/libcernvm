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
#ifndef VBoxInstance_H
#define VBoxInstance_H

#include "VBoxCommon.h"
#include "VBoxSession.h"

#include <map>

#include "CernVM/Utilities.h"
#include "CernVM/Hypervisor.h"
#include "CernVM/ProgressFeedback.h"
#include "CernVM/CrashReport.h"
#include "CernVM/LocalConfig.h"
#include "CernVM/DomainKeystore.h"

#include <boost/regex.hpp>

/**
 * VirtualBox Hypervisor
 */
class VBoxInstance : public HVInstance {
public:

    VBoxInstance( std::string fBin ) : HVInstance(), execConfig(), reflectionValid(true) {
        CRASH_REPORT_BEGIN;

        // Populate variables
        this->sessionLoaded = false;
        this->hvBinary = fBin;

        // Load hypervisor-specific runtime configuration
        this->hvConfig = LocalConfig::forRuntime("virtualbox");

        // Detect and update VirtualBox Version & Reflection flag
        this->validateIntegrity();

        CRASH_REPORT_END;
    };


    /////////////////////////
    // HVInstance Overloads
    /////////////////////////

    virtual HVSessionPtr    sessionOpen         ( const ParameterMapPtr& parameters, const FiniteTaskPtr& pf );
    virtual void            sessionDelete       ( const HVSessionPtr& session );
    virtual void            sessionClose        ( const HVSessionPtr& session );

    virtual int             getType             ( ) { return reflectionValid ? HV_VIRTUALBOX : HV_NONE; };
    virtual int             loadSessions        ( const FiniteTaskPtr & pf = FiniteTaskPtr() );
    virtual bool            waitTillReady       ( DomainKeystore & keystore, const FiniteTaskPtr & pf = FiniteTaskPtr(), const UserInteractionPtr & ui = UserInteractionPtr() );
    virtual HVSessionPtr    allocateSession     ( );
    virtual int             getCapabilities     ( HVINFO_CAPS * caps );
    virtual void            abort               ( );
    virtual bool            validateIntegrity   ( );

    /////////////////////////
    // Friend functions
    /////////////////////////

    int                     prepareSession      ( VBoxSession * session );
    std::map<const std::string, const std::string>        
                            getMachineInfo      ( std::string uuid, int timeout = SYSEXEC_TIMEOUT );
    std::string             getProperty         ( std::string uuid, std::string name );
    std::vector< std::map< const std::string, const std::string > > 
                            getDiskList         ( );
    std::map<std::string, std::string> 
                            getAllProperties    ( std::string uuid );
    bool                    hasExtPack          ();
    int                     installExtPack      ( DomainKeystore & keystore, const DownloadProviderPtr & downloadProvider, const FiniteTaskPtr & pf = FiniteTaskPtr() );
    HVSessionPtr            sessionByVBID       ( const std::string& virtualBoxGUID );

    /////////////////////////
    // Global properties
    /////////////////////////

    std::string             hvGuestAdditions;

private:

    /////////////////////////
    // Local properties
    /////////////////////////

    LocalConfigPtr          hvConfig;
    bool                    sessionLoaded;

    // Default sysExecConfig
    SysExecConfig           execConfig;

    // The virtualbox reflection is still valid
    bool                    reflectionValid;

#ifdef __linux__
    // On linux, we also check for 'The vboxdrv kernel module is not loaded' warnings 
    bool                    vboxDrvKernelLoaded;
#endif

};

#endif /* end of include guard: VBoxInstance_H */
