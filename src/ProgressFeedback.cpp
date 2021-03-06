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

#include <CernVM/ProgressFeedback.h>

/** #####################################################################################
 *  # ProgressTask Implementation - Base Task
 ** ##################################################################################### */

/**
 * Mark the task as completed
 */
void ProgressTask::complete ( const std::string& message ) {
    CRASH_REPORT_BEGIN;
//	std::cout << "  complete(" << message << ")" << std::endl;
    std::string msg = message;

	// Build empty message
	if (message.empty()) {
		if (!lastMessage.empty()) {
			msg = "Completed " + lastMessage;
        }
	}

	// Notify update
	_notifyCompleted(msg);

    CRASH_REPORT_END;
}

/**
 * Mark the task as failed
 */
void ProgressTask::fail ( const std::string& message, const int errorCode ) {
    CRASH_REPORT_BEGIN;
    std::string msg = message;

	// Build empty message
	if (message.empty()) {
		if (!lastMessage.empty()) {
			msg = "Failed " + lastMessage;
        }
	}

    // Mark us as completed
    completed = true;

	// Notify failure
	_notifyFailed(msg, errorCode);

    // Notify update
    if (parent)
        parent->_notifyUpdate(message);

    CRASH_REPORT_END;
}


/**
 * Forward a message without any progress update
 */
void ProgressTask::doing ( const std::string& message ) {
    CRASH_REPORT_BEGIN;
//	std::cout << "  doing(" << message << ")" << std::endl;

	// Notify started
	_notifyStarted( message );

	// Notify update
	_notifyUpdate(message);

    CRASH_REPORT_END;
}

/**
 * Mark the task as lengthy
 */
void ProgressTask::markLengthy ( const bool isLengthy, const std::string& message ) {
	CRASH_REPORT_BEGIN;

	// Mark the task as lengthy
	_notifyLengthyTask( message, isLengthy );

	CRASH_REPORT_END;
}

/**
 * Let listeners know that we are completed
 */
void ProgressTask::_notifyCompleted ( const std::string& message ) {
    CRASH_REPORT_BEGIN;

	// If we are alrady completed, do nothing
	if (completed) return;

	// Mark as completed
	completed = true;

	// Store the last message
	lastMessage = message;

	// Call all the progress callbacks
	fireProgress( message, 1.0 );
	fireCompleted( message );

	// Propagate event
	if (parent) {
		parent->_notifyUpdate( message );
	}

    CRASH_REPORT_END;
}

/**
 * Let listeners know that we have failed
 */
void ProgressTask::_notifyFailed( const std::string& message, const int errorCode ) {
    CRASH_REPORT_BEGIN;

	// Store the last message
	lastMessage = message;

    // Fire the failure event
    fireFailed( message, errorCode );

	// Forward event to the parent elements
	if (parent)
		parent->_notifyFailed( message, errorCode );

    CRASH_REPORT_END;
}

/**
 * Check for state of this progress event
 */
void ProgressTask::_notifyUpdate ( const std::string& message ) {
    CRASH_REPORT_BEGIN;

	// Store the last message
	lastMessage = message;

	// Check if we are completed
	if (isCompleted()) {

		// Send a completion notification
		_notifyCompleted( message );

	} else {

		// Call all the progress callbacks
		fireProgress( message, getProgress() );

		// Propagate event
		if (parent) {
			parent->_notifyUpdate( message );
		}

	}

    CRASH_REPORT_END;
}

/**
 * Let everybody know that we started the event
 */
void ProgressTask::_notifyStarted ( const std::string& message ) {
    CRASH_REPORT_BEGIN;

	// Don't do such thing if we are already started
	if (started) return;

	// Mark us as started
	started = true;

	// Store the last message
	lastMessage = message;

	// Fire callbacks
	fireStarted( message );

	// Propagate event
	if (parent) {
		parent->_notifyStarted( message );
	}

    CRASH_REPORT_END;
}

/**
 * Forward progress event
 * TODO: Optimizations are needed (too many nested loops on calculating getProgress)
 */
void ProgressTask::_forwardProgress( const std::string& message ) {
    CRASH_REPORT_BEGIN;

	// Store the last message
	lastMessage = message;

	// Call all the progress callbacks
	fireProgress( message, getProgress() );

	// Forward event to the parent elements
	if (parent)
		parent->_forwardProgress( message );

    CRASH_REPORT_END;
}

/**
 * Forward lengthy task to the up to the root node
 */
void ProgressTask::_notifyLengthyTask ( const std::string& message, const bool isLengthy ) {
    CRASH_REPORT_BEGIN;

	// Fire callbacks
	fireIsLengthy( message, isLengthy );

	// Propagate event
	if (parent) {
		parent->_notifyLengthyTask( message, isLengthy );
	}

    CRASH_REPORT_END;
}

/** #####################################################################################
 *  # FiniteTask Implementation - Finite Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool FiniteTask::isCompleted ( ) {
    CRASH_REPORT_BEGIN;

	// If I am already completed, return true
	if (completed) return true;

	// If I haven't started, return false
	if (!started) return false;

	// Get the number of tasks
	size_t len = tasks.size();

	// Loop over tasks
	for (size_t i=0; i<len; i++) {

		if (tasks[i] == 0) {
			// If task is not completed, just return false
			return false;

		} else if (tasks[i] == 2) {
			// If we have a sub-task, and it's not finished, return false
			if (!taskObjects[i]->isCompleted())
				return false;

		}

	}

	// Everything looks good
	return true;

    CRASH_REPORT_END;
}

/**
 * Return progress event
 */
double FiniteTask::getProgress ( ) {
    CRASH_REPORT_BEGIN;

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Get the number of tasks
	size_t len = tasks.size();

	// Calculate the step size
	double stepSize = 1.0 / (double)len;
	double value = 0.0;

	// Loop over tasks
	for (size_t i=0; i<len; i++) {

		if (tasks[i] == 1) {
			// If task is completed, add a step
			value += stepSize;

		} else if (tasks[i] == 2) {
			// If we have a sub-task, calculate the sub-progress
			value += stepSize * taskObjects[i]->getProgress();

		}

	}

	// Return value
	return value;

    CRASH_REPORT_END;
}

/**
 * Set maximum number of tasks
 */
void FiniteTask::setMax ( size_t maxTasks, bool triggerUpdate ) {
    CRASH_REPORT_BEGIN;
//	std::cout << "  setMax(" << maxTasks << ", " << triggerUpdate << ")" << std::endl;

	// Get the size of the array
	size_t len = tasks.size();

	if (maxTasks < len) {
		// Delete elements if smaller
		tasks.resize( maxTasks );
		taskObjects.resize( maxTasks );

	} else if (maxTasks > len) {
		// Allocate more elements in 'unfinished' state
		tasks.resize( maxTasks, 0 );
		taskObjects.resize( maxTasks );

	}

	// Notify update events if we are in the middle of something
	if (started && triggerUpdate) _notifyUpdate( lastMessage );

    CRASH_REPORT_END;
}

/**
 * Mark the task as completed
 */
void FiniteTask::done( const std::string& message ) {
    CRASH_REPORT_BEGIN;
//	std::cout << "  done(" << message << ")" << std::endl;

	// Notify started
	_notifyStarted( message );

	// Get the size of the array
	size_t len = tasks.size();

	// Allocate task only if we have enough slots
	if (taskIndex < len) {

		// Mark the given task as completed
		tasks[taskIndex] = 1;

		// Go to next step
		taskIndex += 1;

	}

	// Forward update
	_notifyUpdate( message );

    CRASH_REPORT_END;
}

/**
 * Allocate a new child task
 */
template <typename T> boost::shared_ptr<T> 
FiniteTask::begin( const std::string& message ) {
    CRASH_REPORT_BEGIN;

	// Notify started
	_notifyStarted( message );

	// Get the size of the array
	size_t len = tasks.size();

    // Create a new object instance
    boost::shared_ptr<T> newPtr = boost::make_shared<T>();
    newPtr->parent = shared_from_this();
    newPtr->lastMessage = message;

	// Allocate task only if we have enough slots
	if (taskIndex < len) {

		// Mark the given task as object-based
		tasks[taskIndex] = 2;

		// Make a new shared object of the given kind
		taskObjects[taskIndex] = newPtr;

		// Go to next step
		taskIndex += 1;

	}

	// Forward update
	_notifyUpdate( message );

    // Return object
    return newPtr;

    CRASH_REPORT_END;
}

/**
 * Reset state and restart
 */
void FiniteTask::restart ( const std::string& message, bool triggerUpdate ) {
    CRASH_REPORT_BEGIN;

    // Reset state
	if (completed) started = false;
    completed = false;
    taskIndex = 0;

	// Get the size of the array
	size_t len = tasks.size();

	// Loop over tasks
	for (size_t i=0; i<len; i++) {

		if (tasks[i] == 1) {
			// If task is completed, mark it as not completed
			tasks[i] = 0;

		} else if (tasks[i] == 2) {
			// If we have a sub-task, fire restart on them too
            taskObjects[i]->restart( message, false );

		}

	}

    // Fire update
    if (triggerUpdate) {
    	if (!started) _notifyStarted( message );
    	_notifyUpdate( message );
    }

    CRASH_REPORT_END;
}

/** #####################################################################################
 *  # VariableTask Implementation - Variable Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool VariableTask::isCompleted ( ) {
    CRASH_REPORT_BEGIN;

	// If I haven't started, return false
	if (!started) return false;

	// If I am already completed, return true
	if (completed) return true;

	// If we have current == max return true
	if (current == max) return true;

	// Otherwise, false
	return false;

    CRASH_REPORT_END;
}

/**
 * Return progress event
 */
double VariableTask::getProgress ( ) {
    CRASH_REPORT_BEGIN;

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Return ammount of tasks completed
	return (double)current / (double)max;

    CRASH_REPORT_END;
}

/**
 * Set max value
 */
void VariableTask::setMax ( size_t maxValue, bool triggerUpdate ) {
    CRASH_REPORT_BEGIN;
//	std::cout << "  setMax(" << maxValue << ", " << triggerUpdate << ")" << std::endl;

	// Update max value
	max = maxValue;

	// Trigger update if we were in the middle of something
	if (started && triggerUpdate) _notifyUpdate( lastMessage );

    CRASH_REPORT_END;
}

/**
 * Update value
 */
void VariableTask::update ( size_t value ) {
    CRASH_REPORT_BEGIN;

	// Notify started
	_notifyStarted( lastMessage );

	// Update value
	current = value;

	// Check spinner suffix
	std::string suffix = "";
	if (showSpinner) {
		const char suffixes[] = { '|', '/', '-', '\\' };

		// Trim 
		if (lastMessage[lastMessage.length()-1] == suffixes[spinnerIndex]) {
			lastMessage = lastMessage.substr( 0, lastMessage.length() - 4 );
		}

		// Advance spinner
		if (++spinnerIndex >= 4)
			spinnerIndex = 0;

		// calculate suffix
		suffix  = "   ";
		suffix += suffixes[spinnerIndex];
	}

	// Trigger update
	_notifyUpdate( lastMessage + suffix );

    CRASH_REPORT_END;
}

/**
 * Update the default value
 */
void VariableTask::setMessage ( const std::string& message ) {
    CRASH_REPORT_BEGIN;
	lastMessage = message;
    CRASH_REPORT_END;
}

/**
 * Reset state and restart
 */
void VariableTask::restart ( const std::string& message, bool triggerUpdate ) {
    CRASH_REPORT_BEGIN;

    // Reset states
	if (completed) started = false;
    completed = false;

    // Reset progress
    current = 0;

    // Fire update
    if (triggerUpdate) {
    	if (!started) _notifyStarted( message );
    	_notifyUpdate( message );
    }

    CRASH_REPORT_END;
}

/** #####################################################################################
 *  # BooleanTask Implementation - Variable Task Set
 ** ##################################################################################### */

/**
 * Check if the task is completed
 */
bool BooleanTask::isCompleted ( ) {
    CRASH_REPORT_BEGIN;

	// If I am already completed, return 1.0
	if (completed) return true;

	// If I haven't started, return 0.0
	if (!started) return false;

	// Otherwise I am not completed
	return true;

    CRASH_REPORT_END;
}

/**
 * Return progress event
 */
double BooleanTask::getProgress ( ) {
    CRASH_REPORT_BEGIN;

	// If I am already completed, return 1.0
	if (completed) return 1.0;

	// If I haven't started, return 0.0
	if (!started) return 0.0;

	// Otherwise I am not completed
	return 0.0;
    CRASH_REPORT_END;
}

/**
 * Reset the state to non-completed
 */
void BooleanTask::restart ( const std::string& message, bool triggerUpdate ) {
    CRASH_REPORT_BEGIN;

	// Reset state
	if (completed) started = false;
	completed = false;

    // Notify update
    if (triggerUpdate) {
    	if (!started) _notifyStarted( message );
    	_notifyUpdate( message );
    }

    CRASH_REPORT_END;
}

/**
 * Implement some known templates
 */
template FiniteTaskPtr      FiniteTask::begin<FiniteTask>( const std::string& message );
template VariableTaskPtr    FiniteTask::begin<VariableTask>( const std::string& message );
template BooleanTaskPtr     FiniteTask::begin<BooleanTask>( const std::string& message );
