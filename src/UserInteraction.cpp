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

#include <CernVM/UserInteraction.h>
#include <boost/bind.hpp>

// Initialize singleton to default
UserInteractionPtr defaultSingleton;

/**
 * Return default user interaction pointer
 */
UserInteractionPtr UserInteraction::Default() {
    CRASH_REPORT_BEGIN;

	// Automatically create an 'accept' singleton
	// if it's not still there.
	if (!defaultSingleton)
		defaultSingleton = boost::make_shared<AcceptInteraction>();

	// Return singleton isntance
	return defaultSingleton;

    CRASH_REPORT_END;
}

/**
 * Display the specified message and wait for an OK/Cancel response.
 */
int UserInteraction::confirm ( const std::string& title, const std::string & message, int timeout ) {
	if (!cbConfirm) return UI_UNDEFINED;
	// Set result to -1 (Pending response)
	result = -1;
	cbConfirm( title, message, boost::bind( &UserInteraction::__cbResult, this, _1 ) );
	return __waitResult( timeout );
}

/**
 * Display the specified message and wait until the user clicks OK.
 */
int UserInteraction::alert ( const std::string& title, const std::string& message, int timeout ) {
	if (!cbAlert) return UI_UNDEFINED;
	// Set result to -1 (Pending response)
	result = -1;
	cbAlert( title, message, boost::bind( &UserInteraction::__cbResult, this, _1 ) );
	return __waitResult( timeout );
}

/**
 * Display a licence whose contents is fetched from the given URL and
 * wait for user response for accepting or declining it.
 */
int UserInteraction::confirmLicenseURL	( const std::string& title, const std::string& url, int timeout ) {
	if (!cbLicenseURL) return UI_UNDEFINED;
	// Set result to -1 (Pending response)
	result = -1;
	cbLicenseURL( title, url, boost::bind( &UserInteraction::__cbResult, this, _1 ) );
	return __waitResult( timeout );
}

/**
 * Display a licence whose contents is provided as a parameter
 */
int UserInteraction::confirmLicense	( const std::string& title, const std::string& buffer, int timeout ) {
	if (!cbLicense) return UI_UNDEFINED;
	// Set result to -1 (Pending response)
	result = -1;
	cbLicense( title, buffer, boost::bind( &UserInteraction::__cbResult, this, _1 ) );
	return __waitResult( timeout );
}

/**
 * Define a handler for confirm message
 */
void UserInteraction::setConfirmHandler	( const callbackConfirm & cb ) {
    CRASH_REPORT_BEGIN;
	cbConfirm = cb;
    CRASH_REPORT_END;
}

/**
 * Define a handler for alert message
 */
void UserInteraction::setAlertHandler ( const callbackAlert & cb ) {
    CRASH_REPORT_BEGIN;
	cbAlert = cb;
    CRASH_REPORT_END;
}

/**
 * Define a handler for license message (offline)
 */
void UserInteraction::setLicenseHandler ( const callbackLicense & cb ) {
    CRASH_REPORT_BEGIN;
	cbLicense = cb;
    CRASH_REPORT_END;
}

/**
 * Define a handler for license message (online)
 */
void UserInteraction::setLicenseURLHandler ( const callbackLicense & cb ) {
    CRASH_REPORT_BEGIN;
	cbLicenseURL = cb;
    CRASH_REPORT_END;
}

/**
 * Trigger user interaction abort
 */
int UserInteraction::abort( bool wait, int setResult ) {
	CRASH_REPORT_BEGIN;

	// If there was nothing to abort return 0
    {
        boost::unique_lock<boost::mutex> lock(mutex);
		if (result >= 0)
			return 0;
	}

	// Abort
	aborted = true;

	// Fire __cbResult to release confirm/alert lock
	__cbResult( setResult );

	// Check if we should wait for abort_handled
	if (wait) {

		// Reset abortHandled flag
		abortHandledFlag = false;

		// Wait on abortHandledMutex
		boost::unique_lock<boost::mutex> lock(abortHandledMutex);
		while(!abortHandledFlag) {
			abortHandledCond.wait(lock);
		}

	} else {

        boost::unique_lock<boost::mutex> lock(abortHandledMutex);
        this->abortHandledFlag = true;

	}

	// Otherwise return 1
	return 1;

	CRASH_REPORT_END;
}

/**
 * Callback by the entity which is handling the aborted case
 */
void UserInteraction::abortHandled() {
	CRASH_REPORT_BEGIN;

	// Ensure unique access
    {
        boost::unique_lock<boost::mutex> lock(abortHandledMutex);
        if (this->abortHandledFlag) return;
        this->abortHandledFlag = true;
    }

    // Release mutex
    abortHandledCond.notify_all();

	CRASH_REPORT_END;
}

/** 
 * Local function to wait for callback
 */
int UserInteraction::__waitResult ( int timeout ) {

    // Reset
	result = -1;
	// Wait on mutex
	boost::unique_lock<boost::mutex> lock(mutex);
	while(result < 0) {
		cond.wait(lock);
	}

	// Return and reset 
	return result;

}

/**
 * Callback function for receiving feedback from the callback handlers
 */
void UserInteraction::__cbResult( int result ) {
    CRASH_REPORT_BEGIN;

	// If result is negative, switch to '0' = Undefined
	if (result < 0) result = 0;

	// Update result
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        this->result = result;
    }

    // Release mutex
    cond.notify_all();

    CRASH_REPORT_END;
}