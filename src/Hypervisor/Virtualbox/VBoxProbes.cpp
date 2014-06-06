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

#include <CernVM/CrashReport.h>
#include <CernVM/Hypervisor/Virtualbox/VBoxProbes.h>
#include <CernVM/Hypervisor.h>

/**
 * Check if log file exists
 */
bool VBoxLogProbe::exists() {
	return file_exists(logFile);
}

/**
 * Analyze log file
 */
void VBoxLogProbe::analyze() {
    CRASH_REPORT_BEGIN;

	// Reset state
	hasState = false;
    state = SS_POWEROFF;

	// Reset resolution
	hasResolutionChange = false;
    resWidth = 0;
    resHeight = 0;
    resBpp = 0;

    // Local config
    bool blockStateChange = false;

    // Locate Logfile
    CVMWA_LOG("Debug", "Looking for state change  in " << logFile );
    if (!file_exists(logFile)) {
    	state = SS_MISSING;
    	return;
    }

    // Open input stream
    ifstream fIn(logFile.c_str(), ifstream::in);

    // Read as few bytes as possible
    string inBufferLine, stateStr;
    size_t iStart, iEnd, i1, i2, qStart, qEnd;
    char inBuffer[1024];

    // If we are doing tailRead, seek to the
    // specified tail size.
    if (tailSize > 0) {

		// Calculate file length
		fIn.seekg( 0, fIn.end );
		int seekSize = fIn.tellg();
		if (seekSize > tailSize) seekSize = tailSize;

		// Move 80kb before te end of the file
		fIn.clear();
		fIn.seekg( -seekSize, fIn.end );

    }

    // Start scanning
    while (!fIn.eof()) {

        // Read line
        fIn.getline( inBuffer, 1023 );

        // Handle it via higher-level API
        inBufferLine.assign( inBuffer );

        if ( ((iStart = inBufferLine.find("Changing the VM state from")) != string::npos) && !blockStateChange ) {

        	// We got a state change
			hasState = true;

            // Pick the appropriate ending
            iEnd = inBufferLine.length();
            i1 = inBufferLine.find("\r");
            i2 = inBufferLine.find("\n");
            if (i1 < iEnd) iEnd=i1;
            if (i2 < iEnd) iEnd=i2;

            // Find first quotation
            qStart = inBufferLine.find('\'', iStart);
            if (qStart == string::npos) continue;
            qEnd = inBufferLine.find('\'', qStart+1);
            if (qEnd == string::npos) continue;

            // Find second quotation
            qStart = inBufferLine.find('\'', qEnd+1);
            if (qStart == string::npos) continue;
            qEnd = inBufferLine.find('\'', qStart+1);
            if (qEnd == string::npos) continue;

            // Extract string
            stateStr = inBufferLine.substr( qStart+1, qEnd-qStart-1 );

            // Compare to known state names
            CVMWA_LOG("Debug","Got switch to " << stateStr);
            if      (stateStr.compare("RUNNING") == 0) state = SS_RUNNING;
            else if (stateStr.compare("SUSPENDED") == 0) state = SS_PAUSED;
            else if (stateStr.compare("OFF") == 0) state = SS_POWEROFF;

            // If we got 'SAVING' it means the VM was saved
            if (stateStr.compare("SAVING") == 0) {
                fIn.close();
                blockStateChange = true;
                state = SS_SAVED;
            }

        } else if ((iStart = inBufferLine.find("Display::handleDisplayResize")) != string::npos) {

        	// We got a resolutino change
        	hasResolutionChange = true;

			// Get W component
			qStart = inBufferLine.find("w=", iStart);
			if (qStart == string::npos) continue;
			qEnd = inBufferLine.find(" ", qStart);
			if (qEnd == string::npos) continue;
			resWidth = ston<int>( inBufferLine.substr( qStart+2, qEnd-qStart-2 ) );

			// Get H component
			qStart = inBufferLine.find("h=", iStart);
			if (qStart == string::npos) continue;
			qEnd = inBufferLine.find(" ", qStart);
			if (qEnd == string::npos) continue;
			resHeight = ston<int>( inBufferLine.substr( qStart+2, qEnd-qStart-2 ) );

			// Get BPP component
			qStart = inBufferLine.find("bpp=", iStart);
			if (qStart == string::npos) continue;
			qEnd = inBufferLine.find(" ", qStart);
			if (qEnd == string::npos) continue;
			resBpp = ston<int>( inBufferLine.substr( qStart+2, qEnd-qStart-4 ) );

        }

    }

    fIn.close();

    CRASH_REPORT_END;
}