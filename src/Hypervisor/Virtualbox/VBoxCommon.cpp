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

#include <CernVM/Hypervisor/Virtualbox/VBoxCommon.h>
#include <CernVM/Hypervisor/Virtualbox/VBoxInstance.h>

#include "CernVM/Config.h"
#include <cerrno>

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

#ifdef _WIN32
/**
 * Helper to read registry keys
 */
std::string REG_GET_STRING( HKEY hRootKey, LPCTSTR lpSubKey, LPCTSTR lpValueName, std::string defaultValue ) {

    // Open key
    HKEY hKey;
    LONG lRes = RegOpenKeyExA(hRootKey, lpSubKey, 0, KEY_READ, &hKey);
    if (lRes != ERROR_SUCCESS)
        return defaultValue;

    // Read string value
    CHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    lRes = RegQueryValueExA(hKey, lpValueName, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (lRes != ERROR_SUCCESS)
        return defaultValue;

    // Cast to string and return value
    std::string ans; ans.assign( szBuffer, dwBufferSize );
    return ans;
}
#endif

/**
 * Allocate a new hypervisor using the specified paths
 */
HVInstancePtr __vboxInstance( string hvBin ) {
    CRASH_REPORT_BEGIN;
    VBoxInstancePtr hv;

    // Create a new hypervisor instance
    hv = boost::make_shared<VBoxInstance>( hvBin );

    return hv;
    CRASH_REPORT_END;
}

/**
 * Locate the VBoxManage binary path
 */
std::string __vboxBinaryPath() {
    vector<string> paths;
    string bin, envPath, p;
    
    // Initialize path with the environment variables
    envPath = getenv("PATH");
    #ifdef _WIN32
    trimSplit( &envPath, &paths, ";", "" );
    #else
    trimSplit( &envPath, &paths, ":", "" );
    #endif

    // On windows, include the HKEY_LOCAL_MACHINE/SOFTWARE/Oracle/VirtualBox/[InstallDir]
    #ifdef _WIN32
    string regValue = REG_GET_STRING(HKEY_LOCAL_MACHINE, "SOFTWARE\\Oracle\\VirtualBox", "InstallDir", "");
    if (!regValue.empty())
        paths.push_back( regValue );
    #endif

    // Include additional default directories
    #ifdef _WIN32
    paths.push_back( "C:/Program Files/Oracle/VirtualBox" );
    paths.push_back( "C:/Program Files (x86)/Oracle/VirtualBox" );
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    paths.push_back( "/Applications/VirtualBox.app/Contents/MacOS" );
    paths.push_back( "/Applications/Utilities/VirtualBox.app/Contents/MacOS" );
    #endif
    #ifdef __linux__
    paths.push_back( "/bin" );
    paths.push_back( "/usr/bin" );
    paths.push_back( "/usr/local/bin" );
    paths.push_back( "/opt/VirtualBox/bin" );
    #endif

    // Detect hypervisor
    for (vector<string>::iterator i = paths.begin(); i != paths.end(); i++) {
        p = *i;
        
        #ifdef _WIN32
        bin = p + "/VBoxManage.exe";
        if (file_exists(bin)) {
            return bin;
        }
        #else
        bin = p + "/VBoxManage";
        if (file_exists(bin)) {
            return bin;
        }
        #endif

    }

    // Return blank hypervisor
    return "";

}

/**
 * Check if virtualbox binary exists
 */
bool vboxExists() {
    CRASH_REPORT_BEGIN;

    // Hypervisor exists if the virtualbox exists in path
    return !__vboxBinaryPath().empty();
    
    CRASH_REPORT_END;
}

/**
 * Search Virtualbox on the environment and return an initialized
 * VBoxInstance object if it was found.
 */
HVInstancePtr vboxDetect() {
    CRASH_REPORT_BEGIN;
    HVInstancePtr hv;
    std::string bin;

    // Detect VBox Binary
    bin = __vboxBinaryPath();
    if (!bin.empty()) {

        // Create a virtualbox instance
        hv = __vboxInstance( bin );

    }

    // Return hypervisor instance or nothing
	return hv;
    CRASH_REPORT_END;
}

/**
 * Start installation of VirtualBox.
 */
int vboxInstall( const DownloadProviderPtr & downloadProvider, DomainKeystore & keystore, const UserInteractionPtr & ui, const FiniteTaskPtr & pf, int retries ) {
    CRASH_REPORT_BEGIN;
    HVInstancePtr hv;
    vector<string> lines;
    int res;

    // Initialize progress feedback
    if (pf) {
        pf->setMax(5);
    }

    // Initialize sysExec Configuration
    SysExecConfig sysExecConfig;

    ////////////////////////////////////
    // Contact the information point
    ////////////////////////////////////
    ParameterMapPtr data = boost::make_shared<ParameterMap>();

    // Download trials
    for (int tries=0; tries<retries; tries++) {
        CVMWA_LOG( "Info", "Fetching data" );

        // Send progress feedback
        if (pf) pf->doing("Downloading hypervisor configuration");

        // Try to download the configuration URL
        res = keystore.downloadHypervisorConfig( downloadProvider, data );
        if ( res != HVE_OK ) {

            // Check for security errors
            if (res == HVE_NOT_VALIDATED) {
                if (pf) pf->fail("Hypervisor configuration signature could not be validated!");
                return res;
            }

            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );

                // Send progress feedback
                if (pf) pf->doing("Re-downloading hypervisor configuration");

                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Too many retries while downloading hypervisor configuration");

            return res;

        } else {

            // Send progress feedback
            if (pf) pf->done("Downloaded hypervisor configuration");

            /* Reached this point, we are good to continue */
            break;            
        }

    }
    
    ////////////////////////////////////
    // Extract information
    ////////////////////////////////////
    
    // Pick the URLs to download from
    #ifdef _WIN32
    const string kDownloadUrl = "win32";
    const string kChecksum = "win32-sha256";
    const string kInstallerName = "win32-installer";
    const string kFileExt = ".exe";
    #endif
    #if defined(__APPLE__) && defined(__MACH__)
    const string kDownloadUrl = "osx";
    const string kChecksum = "osx-sha256";
    const string kInstallerName = "osx-installer";
    const string kFileExt = ".dmg";
    #endif
    #ifdef __linux__
    
    // Do some more in-depth analysis of the linux platform
    LINUX_INFO linuxInfo;
    getLinuxInfo( &linuxInfo );

    // Detect
    #if defined(__LP64__) || defined(_LP64)
    string kDownloadUrl = "linux64-" + linuxInfo.osDistID;
    #else
    string kDownloadUrl = "linux32-" + linuxInfo.osDistID;
    #endif
    
    // Calculate keys for other installers
    string kChecksum = kDownloadUrl + "-sha256";
    string kInstallerName = kDownloadUrl + "-installer";
    
    CVMWA_LOG( "Info", "Download URL key = '" << kDownloadUrl << "'"  );
    CVMWA_LOG( "Info", "Checksum key = '" << kChecksum << "'"  );
    CVMWA_LOG( "Info", "Installer key = '" << kInstallerName << "'"  );
    
    #endif
    
    ////////////////////////////////////
    // Verify information
    ////////////////////////////////////

    // Verify that the keys we are looking for exist
    if (!data->contains( kDownloadUrl )) {
        CVMWA_LOG( "Error", "ERROR: No download URL data found" );

        // Send progress fedback
        if (pf) pf->fail("No hypervisor download URL data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (!data->contains( kChecksum )) {
        CVMWA_LOG( "Error", "ERROR: No checksum data found" );

        // Send progress fedback
        if (pf) pf->fail("No setup checksum data found");

        return HVE_EXTERNAL_ERROR;
    }
    if (!data->contains( kInstallerName )) {
        CVMWA_LOG( "Error", "ERROR: No installer program data found" );

        // Send progress fedback
        if (pf) pf->fail("No installer program data found");

        return HVE_EXTERNAL_ERROR;
    }
    
    
    #ifdef __linux__
    // Pick an extension and installation type based on the installer= parameter
    int installerType = PMAN_NONE;
    string kFileExt = ".run";
    if (data->get(kInstallerName).compare("dpkg") == 0) {
        installerType = PMAN_DPKG; /* Using debian installer */
        kFileExt = ".deb";
    } else if (data->get(kInstallerName).compare("yum") == 0) {
        installerType = PMAN_YUM; /* Using 'yum localinstall <package> -y' */
        kFileExt = ".rpm";
    }
    #endif

    ////////////////////////////////////
    // Download hypervisor installer
    ////////////////////////////////////
    string tmpHypervisorInstall;
    string checksum;

    // Prepare feedback pointers
    VariableTaskPtr downloadPf;
    if (pf) {
        downloadPf = pf->begin<VariableTask>("Downloading hypervisor installer");
    }

    // Download trials loop
    for (int tries=0; tries<retries; tries++) {

        // Download installer
        tmpHypervisorInstall = getTmpFile( getURLFilename(data->get(kDownloadUrl) ));
        CVMWA_LOG( "Info", "Downloading " << data->get(kDownloadUrl) << " to " << tmpHypervisorInstall  );
        res = downloadProvider->downloadFile( data->get(kDownloadUrl), tmpHypervisorInstall, downloadPf );
        CVMWA_LOG( "Info", "    : Got " << res  );
        if ( res != HVE_OK ) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Unable to download hypervisor installer");
            return res;
        }
        
        // Validate checksum
        if (pf) pf->doing("Validating download");
        sha256_file( tmpHypervisorInstall, &checksum );

        CVMWA_LOG( "Info", "File checksum " << checksum << " <-> " << data->get(kChecksum)  );
        if (checksum.compare( data->get(kChecksum) ) != 0) {
            if (tries<retries) {
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                if (downloadPf) downloadPf->restart("Re-downloading hypervisor installer");
                sleepMs(1000);
                continue;
            }

            // Celeanup
            ::remove( tmpHypervisorInstall.c_str() );

            // Send progress fedback
            if (pf) pf->fail("Unable to validate hypervisor installer");
            return HVE_NOT_VALIDATED;
        }

        // Send progress feedback
        if (pf) pf->done("Hypervisor installer downloaded");

        // ( Reached this point, we are good to continue )
        break;

    }
    
    ////////////////////////////////////
    // OS-Dependant installation process
    ////////////////////////////////////

    // Prepare feedback pointers
    FiniteTaskPtr installerPf;
    if (pf) {
        installerPf = pf->begin<FiniteTask>("Installing hypervisor");
    }

    // Start installer with retries
    string errorMsg;
    for (int tries=0; tries<retries; tries++) {
        #if defined(__APPLE__) && defined(__MACH__)
            if (installerPf) installerPf->setMax(4, false);
            string dskDev, dskVolume, extra;

            // Catch thread interruptions so we can cleanly unmount and delete
            // residual files and disks.
            try {

                // Attach hard disk
                CVMWA_LOG( "Info", "Attaching" << tmpHypervisorInstall );
                if (installerPf) installerPf->doing("Mouting hypervisor DMG disk");
                if (installerPf) installerPf->markLengthy(true);
                res = sysExec("/usr/bin/hdiutil", "attach " + tmpHypervisorInstall, &lines, &errorMsg, sysExecConfig);
                if (res != 0) {
                    if (tries<retries) {
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Retrying installation");
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (installerPf) installerPf->markLengthy(false);
                    if (pf) pf->fail("Unable to use hdiutil to mount DMG");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Mounted DMG disk");

                string infoLine = lines.back();
                getKV( infoLine, &dskDev, &extra, ' ', 0);
                getKV( extra, &extra, &dskVolume, ' ', dskDev.size()+1);
                CVMWA_LOG( "Info", "Got disk '" << dskDev << "', volume: '" << dskVolume  );
        
                if (installerPf) installerPf->doing("Starting installer");
                CVMWA_LOG( "Info", "Installing using " << dskVolume << "/" << data->get(kInstallerName)  );
                res = sysExec("/usr/bin/open", "-W " + dskVolume + "/" + data->get(kInstallerName), NULL, &errorMsg, sysExecConfig);
                if (res != 0) {

                    CVMWA_LOG( "Info", "Detaching" );
                    if (installerPf) installerPf->doing("Unmounting DMG");
                    res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg, sysExecConfig);
                    if (tries<retries) {
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Restarting installer");
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (installerPf) installerPf->markLengthy(false);
                    if (pf) pf->fail("Unable to launch hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Installed hypervisor");

                // Detach hard disk
                CVMWA_LOG( "Info", "Detaching" );
                if (installerPf) installerPf->doing("Cleaning-up");
                res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg, sysExecConfig);
                if (installerPf) {
                    installerPf->markLengthy(false);
                    installerPf->done("Cleaning-up completed");
                    installerPf->complete("Installed hypervisor");
                }

            } catch (boost::thread_interrupted &) {

                // If operations were interrupted within this context, it most probably means
                // that we have a residual, mounted hard disk. 
                CVMWA_LOG( "Info", "Operation interrupted. Detaching..." );

                // Try to unmount disk
                if (!dskDev.empty())
                    res = sysExec("/usr/bin/hdiutil", "detach " + dskDev, NULL, &errorMsg, sysExecConfig);

                // Try to remove the file
                ::remove( tmpHypervisorInstall.c_str() );

                // Cleanup progress feedback objects
                if (installerPf) installerPf->markLengthy(false);
                if (pf) pf->fail("Installation interrupted");

                // Rethrow
                throw;

            }

        #elif defined(_WIN32)
            if (installerPf) installerPf->setMax(2, false);

            // Start installer
            if (installerPf) installerPf->doing("Starting installer");
            CVMWA_LOG( "Info", "Starting installer" );

            // CreateProcess does not work because we need elevated permissions,
            // use the classic ShellExecute to run the installer...
            SHELLEXECUTEINFOA shExecInfo = {0};
            shExecInfo.cbSize = sizeof( SHELLEXECUTEINFO );
            shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExecInfo.hwnd = NULL;
            shExecInfo.lpVerb = NULL;
            shExecInfo.lpFile = (LPCSTR)tmpHypervisorInstall.c_str();
            shExecInfo.lpParameters = (LPCSTR)"";
            shExecInfo.lpDirectory = NULL;
            shExecInfo.nShow = SW_SHOWNORMAL;
            shExecInfo.hInstApp = NULL;

            // Validate handle
            if (installerPf) installerPf->markLengthy(true);
            if ( !ShellExecuteExA( &shExecInfo ) ) {
                cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (installerPf) installerPf->markLengthy(false);
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }

            // Validate hProcess
            if (shExecInfo.hProcess == 0) {
                cout << "ERROR: Installation could not start! Error = " << res << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Restarting installer");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (installerPf) installerPf->markLengthy(false);
                if (pf) pf->fail("Unable to launch hypervisor installer");

                return HVE_EXTERNAL_ERROR;
            }

            // Wait for termination
            WaitForSingleObject( shExecInfo.hProcess, INFINITE );
            if (installerPf) installerPf->markLengthy(false);
            if (installerPf) installerPf->done("Installer completed");

            // Complete
            if (installerPf) installerPf->complete("Installed hypervisor");

        #elif defined(__linux__)
            if (installerPf) installerPf->setMax(5, false);

            // Check if our environment has what the installer needs
            if (installerPf) installerPf->doing("Probing environment");
            if (installerPf) installerPf->markLengthy(true);
            if ((installerType != PMAN_NONE) && (installerType != linuxInfo.osPackageManager )) {
                cout << "ERROR: OS does not have the required package manager (type=" << installerType << ")" << endl;
                if (tries<retries) {
                    if (installerPf) installerPf->doing("Re-probing environment");
                    CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                    sleepMs(1000);
                    continue;
                }

                // Cleanup
                ::remove( tmpHypervisorInstall.c_str() );

                // Send progress fedback
                if (installerPf) installerPf->markLengthy(false);
                if (pf) pf->fail("Unable to probe the environment");

                return HVE_NOT_FOUND;
            }
            if (installerPf) installerPf->done("Probed environment");

            // (1) If we have xdg-open, use it to prompt the user using the system's default GUI
            // ----------------------------------------------------------------------------------
            if (linuxInfo.hasXDGOpen) {

                if (installerPf) installerPf->doing("Starting hypervisor installer");
                string cmdline = "/usr/bin/xdg-open \"" + tmpHypervisorInstall + "\"";
                res = system( cmdline.c_str() );
                if (res < 0) {
                    cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (installerPf) installerPf->markLengthy(false);
                    if (pf) pf->fail("Unable to start the hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Installer started");
            
                // Wait for 5 minutes for the binary to appear
                int counter = 0;
                while (!vboxExists()) {
                    if (++counter > 300) {
                        if (tries<retries) {
                            CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                            if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                            sleepMs(1000);
                            goto try_continue;
                        }

                        // Cleanup
                        ::remove( tmpHypervisorInstall.c_str() );

                        // Send progress fedback
                        if (installerPf) installerPf->markLengthy(false);
                        if (pf) pf->fail("Timeout occured while waiting for Virtualbox to appear");

                        return HVE_EXTERNAL_ERROR;
                    }
                    // Check with 1 sec intervals
                    sleepMs(1000);
                }

                // Done
                if (installerPf) installerPf->markLengthy(false);
                if (installerPf) installerPf->done("Installation completed");
        
                // Complete
                if (installerPf) installerPf->complete("Installed hypervisor");

            // (2) If we have GKSudo or PKExec do directly dpkg/yum install
            // ------------------------------------------------------
            } else if (linuxInfo.hasPKExec || linuxInfo.hasGKSudo) {
                string cmdline = "/bin/sh '" + tmpHypervisorInstall + "'";
                if ( installerType == PMAN_YUM ) {
                    cmdline = "/usr/bin/yum localinstall -y '" + tmpHypervisorInstall + "' -y";
                } else if ( installerType == PMAN_DPKG ) {
                    cmdline = "/usr/bin/dpkg -i '" + tmpHypervisorInstall + "'";
                }

                // Use GKSudo to invoke the cmdline
                if (installerPf) installerPf->doing("Starting installer");
                if (linuxInfo.hasPKExec) {
                    cmdline = "/usr/bin/pkexec --user root " + cmdline;
                } else {
                    cmdline = "/usr/bin/gksudo \"" + cmdline + "\"";
                }
                res = system( cmdline.c_str() );
                if (res < 0) {
                    cout << "ERROR: Could not start. Return code: " << res << endl;
                    if (tries<retries) {
                        if (installerPf) installerPf->doing("Re-starting installer");
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        sleepMs(1000);
                        continue;
                    }

                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );

                    // Send progress fedback
                    if (installerPf) installerPf->markLengthy(false);
                    if (pf) pf->fail("Unable to start the hypervisor installer");

                    return HVE_EXTERNAL_ERROR;
                }
                if (installerPf) installerPf->done("Installer completed");

                // Complete
                if (installerPf) installerPf->markLengthy(false);
                if (installerPf) installerPf->complete("Installed hypervisor");

            /* (3) Otherwise create a bash script and prompt the user */
            } else {
            
                /* TODO: I can't do much here :( */
                if (installerPf) installerPf->markLengthy(false);
                return HVE_NOT_IMPLEMENTED;
            
            }
        
        #endif
    
        // Give 5 seconds as a cool-down delay
        sleepMs(5000);

        // Check if it was successful
        hv = detectHypervisor();
        if (!hv) {
            CVMWA_LOG( "Info", "ERROR: Could not install hypervisor!" );
            if (tries<retries) {
                if (installerPf) installerPf->restart("Re-trying hypervisor installation");
                CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                sleepMs(1000);
                continue;
            }

            // Send progress fedback
            if (pf) pf->fail("Hypervisor installation seems not feasible");

            return HVE_NOT_VALIDATED;
        } else {

            // We have a hypervisor. Wait for 5 minutes until integrity validation returns true
            int counter = 0;
            if (pf) pf->doing("Validating installation integrity");
            while (!hv->validateIntegrity()) {
                if (++counter > 300) {
                    if (tries<retries) {
                        CVMWA_LOG( "Info", "Going for retry. Trials " << tries << "/" << retries << " used." );
                        if (installerPf) installerPf->doing("Re-starting hypervisor installer");
                        sleepMs(1000);
                        continue;
                    }
                    // Cleanup
                    ::remove( tmpHypervisorInstall.c_str() );
                    if (pf) pf->fail("Timeout occured while waiting for hypervisor to be ready");
                    return HVE_EXTERNAL_ERROR;
                }
                // Check with 1 sec intervals
                sleepMs(1000);
            }

            // Installation was successful. Try for 30 seconds to remove the temporary file
            if (pf) pf->doing("Cleaning-up residual files");
            while (::remove( tmpHypervisorInstall.c_str() ) != 0) {
                sleepMs(1000);
            }

            break;

        }
        
        // Anchor for continuing the outer loop
try_continue:
        void();

    }


    // Completed
    if (pf) pf->complete("Hypervisor installed successfully");
    return HVE_OK;
    
    CRASH_REPORT_END;
}

/**
 * Tool function to extract the mac address of the VM from the NIC line definition
 */
std::string _vbox_extractMac( std::string nicInfo ) {
    CRASH_REPORT_BEGIN;
    // A nic line is like this:
    // MAC: 08002724ECD0, Attachment: Host-only ...
    size_t iStart = nicInfo.find("MAC: ");
    if (iStart != string::npos ) {
        size_t iEnd = nicInfo.find(",", iStart+5);
        string mac = nicInfo.substr( iStart+5, iEnd-iStart-5 );
        
        // Convert from AABBCCDDEEFF notation to AA:BB:CC:DD:EE:FF
        return mac.substr(0,2) + ":" +
               mac.substr(2,2) + ":" +
               mac.substr(4,2) + ":" +
               mac.substr(6,2) + ":" +
               mac.substr(8,2) + ":" +
               mac.substr(10,2);
               
    } else {
        return "";
    }
    CRASH_REPORT_END;
};

/**
 * Tool function to replace the last part of the given IP
 */
std::string _vbox_changeUpperIP( std::string baseIP, int value ) {
    CRASH_REPORT_BEGIN;
    size_t iDot = baseIP.find_last_of(".");
    if (iDot == string::npos) return "";
    return baseIP.substr(0, iDot) + "." + ntos<int>(value);
    CRASH_REPORT_END;
};