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

#include <CernVM/SimpleFSM.h>
#include <cstdarg>
#include <stdexcept>
#include <iostream>

/**
 * Void function FSMEnteringState
 */
void SimpleFSM::FSMEnteringState( const int state, bool final ) { }

/**
 * Reset FSM registry variables
 */
void SimpleFSM::FSMRegistryBegin() {
    CRASH_REPORT_BEGIN;
    // Reset
    fsmNodes.clear();
    fsmTmpRouteLinks.clear();
    fsmCurrentPath.clear();
    fsmRootNode = NULL;
    fsmCurrentNode = NULL;
    CRASH_REPORT_END;
}

/**
 * Add entry to the FSM registry
 */
void SimpleFSM::FSMRegistryAdd( int id, fsmHandler handler, ... ) {
    CRASH_REPORT_BEGIN;
    std::vector<int> v;
    va_list pl;
    int l;

    // Initialize node
    fsmNodes[id].id = id;
    fsmNodes[id].handler = handler;
    fsmNodes[id].children.clear();
    
    // Store route mapping to temp routes vector
    // (Will be synced by FSMRegistryEnd)
    va_start(pl, handler);
    while ((l = va_arg(pl,int)) != 0) {
        v.push_back(l);
    }
    va_end(pl);
    fsmTmpRouteLinks[id] = v;
    CRASH_REPORT_END;
}

/**
 * Complete FSM registry decleration and build FSM tree
 */
void SimpleFSM::FSMRegistryEnd( int rootID ) {
    CRASH_REPORT_BEGIN;
	std::map<int,FSMNode>::iterator pt;
	std::vector<FSMNode*> 			nodePtr;
	std::vector<int> 				links;

	// Build FSM linked list
	for (std::map<int,FSMNode>::iterator it = fsmNodes.begin(); it != fsmNodes.end(); ++it) {
		int id = (*it).first;
		FSMNode * node = &((*it).second);

		// Create links
		nodePtr.clear();
		links = fsmTmpRouteLinks[id];
		for (std::vector<int>::iterator jt = links.begin(); jt != links.end(); ++jt) {

			// Get pointer to node element in map
			pt = fsmNodes.find( *jt );
			FSMNode * refNode = &((*pt).second);

			// Update node
			node->children.push_back( refNode );

		}

	}

	// Fetch root node
	fsmTargetState = rootID;
	pt = fsmNodes.find( rootID );
	fsmRootNode = &((*pt).second);

	// Reset current node
	fsmCurrentNode = fsmRootNode;

	// Flush temp arrays
	fsmTmpRouteLinks.clear();
    CRASH_REPORT_END;
}

/**
 * Helper function to call a handler
 */
bool SimpleFSM::_callHandler( FSMNode * node, bool inThread ) {
    CRASH_REPORT_BEGIN;

	// Use guarded execution
	try {

		// Check if we are interrupted
		if (fsmtInterruptRequested) {
			CVMWA_LOG("Debuf", "FSM Handler interrupted");
			fsmInsideHandler = false;
			return false;
		}

		// Run the new state
		if (node->handler)
			node->handler();

		// Check if we are interrupted
		if (fsmtInterruptRequested) {
			CVMWA_LOG("Debuf", "FSM Handler interrupted");
			fsmInsideHandler = false;
			return false;
		}

	//} catch (std::thread_interrupted &e) {

	//	// Cleanup
	//	fsmInsideHandler = false;
	//	return false;

	//	//if (inThread) {
	//	//	// If we are in-thread, re-throw so it's
	//	//	// catched with the external wrap
	//	//	throw;
	//	//} else {
	//	//	// Otherwise, return false to indicate that
	//	//	// something failed.
	//	//	return false;
	//	//}

	} catch ( std::exception &e ) {
		CVMWA_LOG("Exception", e.what() );
		return false;

	} catch ( ... ) {
		CVMWA_LOG("Exception", "Unknown exception" );
		return false;

	}

	// Executed successfully
	return true;

    CRASH_REPORT_END;
}

/**
 * Run next action in the FSM
 */
bool SimpleFSM::FSMContinue( bool inThread ) {
    CRASH_REPORT_BEGIN;
	if (fsmInsideHandler) return false;
	if (fsmCurrentPath.empty() && (fsmCurrentNode != NULL)) return false;
	fsmInsideHandler = true;

	FSMNode * next;
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);
		// Get next action in the path
		next = fsmCurrentPath.front();
	    fsmCurrentPath.pop_front();

	}

	// Skip state nodes
    { /* mutex(fsmCurrentPath)) */
        std::unique_lock<std::mutex> lock(fsmPathMutex);
        while ((!next->handler) && !fsmCurrentPath.empty()) {
            lock.unlock();
            FSMEnteringState( next->id, false );
            lock.lock();
            if (fsmCurrentPath.empty()) break;
            next = fsmCurrentPath.front();
            fsmCurrentPath.pop_front();
        }
    }

	// Change current node
	fsmCurrentNode = next;
	FSMEnteringState( next->id, (const bool) fsmCurrentPath.empty() );

	// Call handler
	if (!_callHandler(next, inThread)) 
		return false;

	// We are now outside the handler
	fsmInsideHandler = false;
    return true;
    CRASH_REPORT_END;
}

/**
 * Helper function to traverse the FSM graph, trying to find the
 * shortest path that leads to the given action.
 */
void findShortestPath( std::vector< FSMNode * > path, FSMNode * node, int state,	// Arguments
					  size_t * clipLength, std::vector< FSMNode * > ** bestPath ) {	// State
    CRASH_REPORT_BEGIN;

	// Update path
	path.push_back( node );

	// Clip protection
	if (path.size() >= *clipLength)
		return;

	// Iterate through the children
	for (std::vector<FSMNode*>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
		FSMNode * n = *it;

		// Ignore if it's already visited
		if (std::find(path.begin(), path.end(), n) != path.end())
			continue;

		// Check if we found the node
		if (n->id == state) {

			// Set new clip path
			*clipLength = path.size();
			path.push_back( n );

			// Replace best path
			if (*bestPath != NULL) delete *bestPath;
			*bestPath = new std::vector<FSMNode*>(path);

#ifdef LOGGING
			// Present best path
			std::ostringstream oss;
			for (std::vector<FSMNode*>::iterator j= (*bestPath)->begin(); j!=(*bestPath)->end(); ++j) {
				if (!oss.str().empty()) oss << ", "; oss << (*j)->id;
			}
			CVMWA_LOG("Debug", "Preferred path: " << oss.str() );
#endif

			// We cannot continue (since we found a better path)
			return;
		}

		// Continue with the items
		findShortestPath( path, n, state, clipLength, bestPath );
	}
    CRASH_REPORT_END;
}

/**
 * Build the path to go to the given state and start the FSM subsystem
 * @param int state - The target state
 * @param int stripPathComponents - The path components to strip from the beginning of the traced path
 */
void SimpleFSM::FSMGoto(int state, int stripPathComponents) {
    CRASH_REPORT_BEGIN;

	// Allow only one thread to steer the FSM
	std::unique_lock<std::mutex> lock(fsmGotoMutex);

    CVMWA_LOG("Debug", "Going towards " << state);

	// Reset path
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);
		fsmCurrentPath.clear();
	}

	// Prepare clip length
	size_t clipLength = fsmNodes.size();

	// Prepare solver
	std::vector< FSMNode * > srcPath;
	std::vector< FSMNode * > *resPath = NULL;
	findShortestPath( srcPath, fsmCurrentNode, state, &clipLength, &resPath );

	// Check if we actually found a path
	if (resPath != NULL) {

		// Update target path and release resPath memory
		{
			std::unique_lock<std::mutex> lock(fsmPathMutex);
			fsmCurrentPath.assign( resPath->begin()+stripPathComponents, resPath->end() );
		}
		delete resPath;

		// Switch active target
		fsmTargetState = state;

	}

	// If we have progress feedback, update the max
	if (fsmProgress) {
        int pathCount = 0;

		// Count the actual tasks in the path (skipping state nodes)
		{ /* mutex(fsmCurrentPath)) */
			std::unique_lock<std::mutex> lock(fsmPathMutex);
			pathCount = 0;
			for (std::list<FSMNode*>::iterator j= fsmCurrentPath.begin(); j!=fsmCurrentPath.end(); ++j) {
				FSMNode* node = *j;
				if (node->handler) pathCount++;
			}
		}

		// Restart progress
        fsmProgress->restart( fsmProgressResetMsg, false );

		// Update max tasks that will be passed through
        fsmProgress->setMax( pathCount, false );

	}

	// Notify possibly paused thread
	if (fsmThread != NULL)
		_fsmWakeup();

#ifdef LOGGING
	// Present best path
	std::ostringstream oss;
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);		
	    if (!fsmCurrentPath.empty()) {
	        for (std::list<FSMNode*>::iterator j= fsmCurrentPath.begin(); j!=fsmCurrentPath.end(); ++j) {
			    if (!oss.str().empty()) oss << ", "; oss << (*j)->id;
		    }
	    }
	}
	CVMWA_LOG("Debug", "Best path: " << oss.str() );
#endif

    CRASH_REPORT_END;
}


/**
 * Build the path to go to the given state and start the FSM subsystem
 */
void SimpleFSM::FSMJump(int state) {
    CRASH_REPORT_BEGIN;

	// Allow only one thread to steer the FSM
	std::unique_lock<std::mutex> lock(fsmGotoMutex);

    CVMWA_LOG("Debug", "Jumping to " << state);

	// Reset path
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);
		fsmCurrentPath.clear();
	}

	// Pick the current node
	std::map<int,FSMNode>::iterator it = fsmNodes.find( state );

	if (it == fsmNodes.end()) {
		// Skip missing nodes
		fsmCurrentNode = fsmRootNode;

	} else {

        // Change current node
        fsmCurrentNode = &it->second;

        // Handle only non-state nodes
        if (fsmCurrentNode->handler) {
		    // We are entering the given state
		    FSMEnteringState( fsmCurrentNode->id, true );

		    // Call handler (and throw errors)
		    _callHandler(fsmCurrentNode, true);

        }

	}

    CRASH_REPORT_END;
}

/**
 * Skew the current path by switching to given state and then continuing
 * to the state pointed by goto
 */
void SimpleFSM::FSMSkew(int state) {
    CRASH_REPORT_BEGIN;
    CVMWA_LOG("Debug", "Skewing through " << state << " towards " << fsmTargetState);
	std::map<int,FSMNode>::iterator pt;

	// Search given state
	pt = fsmNodes.find( state );
	if (pt == fsmNodes.end()) return;

	// Switch current node to the skewed state
	fsmCurrentNode = &((*pt).second);

	// Notify state change
	bool isEmpty = false;
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);
		isEmpty = fsmCurrentPath.empty();		
	}
	FSMEnteringState( state, isEmpty );

	// Continue towards the active target only if the path is not empty. 
	// Otherwise it's enough just to change the current node
	{ /* mutex(fsmCurrentPath)) */
		std::unique_lock<std::mutex> lock(fsmPathMutex);
		isEmpty = fsmCurrentPath.empty();		
	}
	if ( !isEmpty ) {
		FSMGoto( fsmTargetState, 0 );
	}

    CRASH_REPORT_END;
}

/**
 * Local function to do FSMContinue when needed in a threaded way
 */
void SimpleFSM::FSMThreadLoop() {
    CRASH_REPORT_BEGIN;
	fsmThreadActive = true;

	// Infinite thread loop
	while (true) {
        if (fsmtInterruptRequested) return;
		bool res = true;

		// Keep running until we run out of steps
		while (res) {
            if (fsmtInterruptRequested) return;

			// Critical section
			{
				std::unique_lock<std::mutex> lock(fsmmThreadSafe);
			    res = FSMContinue(true);
			}

			// Yield our time slice after executing an action
            if (fsmtInterruptRequested) return;

		};

		// Unlock people waiting for completion
		fsmwWaitCond.notify_all();

		// After the above loop, we have drained the
		// event queue. Enter paused state and wait
		// to be notified when this changes.
		_fsmPause();
	}


	// Cleanup
	fsmThreadActive = false;
    CRASH_REPORT_END;
}

/**
 * Local function to exit the FSM Thread
 */
void SimpleFSM::FSMThreadStop() {
    CRASH_REPORT_BEGIN;
    CVMWA_LOG("Debug", "Stopping FSM thread");

	// Ensure we have a running thread
	if ((fsmThread == NULL) || (!fsmThreadActive)) {
	    CVMWA_LOG("Debug", "Thread already stopped");
		return;
	}

	// Interrupt thread
	fsmtInterruptRequested = true;

	// Notify all condition variables
	fsmwWaitCond.notify_all();
	fsmtPauseChanged.notify_all();
    
    // Unlock all mutexes
    fsmmThreadSafe.try_lock(); fsmmThreadSafe.unlock();
    fsmtPauseMutex.try_lock(); fsmtPauseMutex.unlock();
    fsmwStateMutex.try_lock(); fsmwStateMutex.unlock();
    fsmGotoMutex.try_lock(); fsmGotoMutex.unlock();
    fsmPathMutex.try_lock(); fsmPathMutex.unlock();

    // Join thread
	fsmThread->join();

	// Cleanup thread
	fsmThread = NULL;

    CRASH_REPORT_END;
}

/**
 * Infinitely pause, waiting for a wakeup signal
 */
void SimpleFSM::_fsmPause() {
	CVMWA_LOG("Debug", "Entering paused state");
    if (fsmtInterruptRequested) return;

	// If we are already not paused, don't
	// do anything
	if (fsmtPaused) {
		std::unique_lock<std::mutex> lock(fsmtPauseMutex);
	    while(fsmtPaused) {
            if (fsmtInterruptRequested) {
                fsmtPaused = false;
                break;
            };
	        fsmtPauseChanged.wait(lock);
	    }
	}

    // Reset paused state
    fsmtPaused = true;
	CVMWA_LOG("Debug", "Exiting paused state");
}

/**
 * Send a wakeup signal
 */
void SimpleFSM::_fsmWakeup() {
    CRASH_REPORT_BEGIN;
    if (fsmtInterruptRequested) return;
    CVMWA_LOG("Debug", "Waking-up paused thread");

    {
		std::unique_lock<std::mutex> lock(fsmtPauseMutex);
        fsmtPaused = false;
    }

    fsmtPauseChanged.notify_all();
    CRASH_REPORT_END;
}

/**
 * Set a progress object to use with SimpleFSM.
 *
 * This will enable automatic re-calibration of the maximum elements of the finite-task
 * progress feedback mechanism when the FSM engine is picking a different path.
 *
 */
void SimpleFSM::FSMUseProgress ( const FiniteTaskPtr & pf, const std::string & resetMessage ) {
    CRASH_REPORT_BEGIN;
	fsmProgress = pf;
	fsmProgressResetMsg = resetMessage;
    CRASH_REPORT_END;
}

/**
 * Trigger the "Doing" action of the SimpleFSM progress feedback -if available-
 *
 * This function should be placed in the beginning of all the FSM action handlers in order
 * to provide more meaningul message to the user.
 */
void SimpleFSM::FSMDoing ( const std::string & message ) {
    CRASH_REPORT_BEGIN;
	CVMWA_LOG("Debug", "Doing " << message);
	if (fsmProgress) {
		fsmProgress->doing(message);
	}
    CRASH_REPORT_END;
}

/**
 * Trigger the "Done" action of the SimpleFSM progress feedback -if available-
 *
 * This function should be placed in the end of all the FSM action handlers in order
 * to provide more meaningul message to the user.
 */
void SimpleFSM::FSMDone ( const std::string & message ) {
    CRASH_REPORT_BEGIN;
	CVMWA_LOG("Debug", "Done " << message);
	if (fsmProgress) {
		fsmProgress->done(message);
	}
    CRASH_REPORT_END;
}

/**
 * Trigger the "begin" action of the SimpleFSM progress feedback.
 * This function cannot be used when FSMDoing/FSMDone are used.
 */
template <typename T> std::shared_ptr<T> 
SimpleFSM::FSMBegin( const std::string& message ) {
    CRASH_REPORT_BEGIN;
	std::shared_ptr<T> ptr = std::shared_ptr<T>();
	if (fsmProgress) {
		ptr = fsmProgress->begin<T>(message);
	}
	return ptr;
    CRASH_REPORT_END;
}

/**
 * Trigger the "Fail" action of the SimpleFSM progress feedback -if available-
 */
void SimpleFSM::FSMFail ( const std::string & message, const int errorCode ) {
    CRASH_REPORT_BEGIN;
	CVMWA_LOG("Debug", "Done " << message);
	if (fsmProgress) {
		fsmProgress->fail(message, errorCode);
	}
    CRASH_REPORT_END;
}

/**
 * Wait until the FSM reaches the specified state
 */
void SimpleFSM::FSMWaitFor ( int state, int timeout ) {
    CRASH_REPORT_BEGIN;
    CVMWA_LOG("Debug", "Waiting for state " << state );

	// Find the state
	std::map<int,FSMNode>::iterator pt;
	pt = fsmNodes.find( state );
	if (pt == fsmNodes.end()) return;

	// If we are already on this state, don't do anything
	if (fsmCurrentNode == fsmwState) return;

	// Switch to the target state
	fsmwState = &((*pt).second);

	/*
	{
	boost::unique_lock<boost::mutex> lock(fsmwStateMutex);
	fsmtPaused = false;
	}
	fsmwStateChanged.notify_all();
	*/

	// Wait for state
	{
		std::unique_lock<std::mutex> lock(fsmwStateMutex);
		//fsmwStateChanged.wait(lock);
	}

    CRASH_REPORT_END;
}

/**
 * Wait for FSM to complete an active tasm
 */
void SimpleFSM::FSMWaitInactive ( int timeout ) {
    CRASH_REPORT_BEGIN;

	// Wait until we are no longer active
	{
	    std::unique_lock<std::mutex> lock(fsmwWaitMutex);		
		while (FSMActive()) {
			fsmwWaitCond.wait(lock);
		}
	}

    CRASH_REPORT_END;
}

/**
 * Check if FSM is busy navigating through a path
 */
bool SimpleFSM::FSMActive ( ) {
    CRASH_REPORT_BEGIN;
	std::unique_lock<std::mutex> lock(fsmPathMutex);
	return !fsmCurrentPath.empty();
    CRASH_REPORT_END;
}

/**
 * Start an FSM thread
 */
std::thread * SimpleFSM::FSMThreadStart() {
    CRASH_REPORT_BEGIN;
	// If we are already running, return the thread
	if (fsmThread != NULL)
		return fsmThread;

	// Reset properties
	fsmtInterruptRequested = false;
	
	// Set the current state to paused, effectively
	// stopping the FSM execution if no wakeup signals are piled
	fsmtPaused = true;

	// Start and return the new thread
	fsmThread = new std::thread(std::bind(&SimpleFSM::FSMThreadLoop, this));
	return fsmThread;
    CRASH_REPORT_END;
}

/**
 * Release mutex upon destruction
 */
SimpleFSM::~SimpleFSM() {
    CRASH_REPORT_BEGIN;

	// Join thread
	FSMThreadStop();

    CRASH_REPORT_END;
}

/**
 * Implement some known templates
 */
template FiniteTaskPtr      SimpleFSM::FSMBegin<FiniteTask>( const std::string& message );
template VariableTaskPtr    SimpleFSM::FSMBegin<VariableTask>( const std::string& message );
template BooleanTaskPtr     SimpleFSM::FSMBegin<BooleanTask>( const std::string& message );
