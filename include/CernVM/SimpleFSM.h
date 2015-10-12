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
#ifndef SIMPLEFSM_H
#define SIMPLEFSM_H

#include "CernVM/Utilities.h"
#include "CernVM/ProgressFeedback.h"

#include <list>
#include <vector>
#include <map>

//#include <boost/thread.hpp>
//#include <boost/thread/condition_variable.hpp>
//#include <boost/thread/mutex.hpp>
#include <functional>
#include <thread>
#include <condition_variable>
#include <mutex>

typedef std::function< void () >	fsmHandler;

// Forward declerations
struct  _FSMNode;
typedef _FSMNode FSMNode;

/**
 * Structure of the FSM node
 * This is in principle a directional graph
 */
struct  _FSMNode{

	// FSM description
	int								id;
	unsigned char 					type;
	fsmHandler						handler;
	std::vector<FSMNode*>			children;

};

/**
 * Helper macro for FSM registry
 */
#define FSM_REGISTRY(root,block) \
 	FSMRegistryBegin();\
 	block; \
 	FSMRegistryEnd(root);

#define FSM_HANDLER(id,cb,...) \
 	FSMRegistryAdd(id, std::bind(cb, this), __VA_ARGS__, 0);

#define FSM_STATE(id,...) \
 	FSMRegistryAdd(id, 0, __VA_ARGS__, 0);

/**
 * Auto-routed Finite-State-Machine class
 */
class SimpleFSM {
public:

	/**
	 * Constructor
	 */
	SimpleFSM() : fsmtPaused(true), fsmThread(NULL), fsmtPauseMutex(), fsmtPauseChanged(),
			  	  fsmwState(NULL), fsmwStateWaiting(false), fsmwStateMutex(), fsmwStateChanged(),
				  fsmInsideHandler(false), fsmProgress(), fsmGotoMutex(), fsmTargetState(0), 
				  fsmwWaitCond(), fsmwWaitMutex(), fsmRootNode(NULL), fsmCurrentNode(),
				  fsmTmpRouteLinks(), fsmNodes(), fsmCurrentPath(), fsmThreadActive(false),
				  fsmtInterruptRequested(false)
				  { };

	/**
	 * Destructor
	 */
	virtual ~SimpleFSM();

	/**
	 * Pick the closest path to go from current state to the given state
	 */
	void 							FSMGoto				( int state, int stripPathComponents = 1 );

	/**
	 * Jump directly to the given path (and run the handler code)
	 */
	void 							FSMJump				( int state );

	/**
	 * Skew the current path by switching to given state and then continuing
	 * to the state pointed by goto
	 */
	void 							FSMSkew				( int state );

	/**
	 * Called externally to continue to the next FSM action
	 */	
	bool 							FSMContinue			( bool inThread = false );

	/**
	 * Start the FSM management thread
	 */
	std::thread *					FSMThreadStart		();

	/**
	 * Stop the FSM thread
	 */
	void 							FSMThreadStop		();

	/**
	 * Enable progress feedback on this SimpleFSM instance
	 */
	void 							FSMUseProgress		( const FiniteTaskPtr & pf, const std::string & resetMessage );

	/**
	 * Wait for FSM to reach the specified state
	 */
	void 							FSMWaitFor			( int state, int timeout = 0 );

	/**
	 * Wait for FSM to complete an active tasm
	 */
	void  							FSMWaitInactive		( int timeout = 0 );

	/**
	 * Check if the FSM is actively working in a node
	 */
	bool 							FSMActive			( );

	/**
	 * Public progress feedback instance used by actions
	 */
	FiniteTaskPtr					fsmProgress;

protected:

	/**
	 * Trigger the "Doing" action of the SimpleFSM progress feedback -if available-
	 *
	 * This function should be placed in the beginning of all the FSM action handlers in order
	 * to provide more meaningul message to the user.
	 */
	void 							FSMDoing			( const std::string & message );

	/**
	 * Trigger the "Done" action of the SimpleFSM progress feedback -if available-
	 *
	 * This function should be placed in the end of all the FSM action handlers in order
	 * to provide more meaningul message to the user.
	 */
	void 							FSMDone				( const std::string & message );

	/**
	 * Trigger the "Fail" action of the SimpleFSM progress feedback -if available-
	 */
	void 							FSMFail				( const std::string & message, const int errorCode = -1 );

	/**
	 * Overridable function to get notified when we are entering a state
	 */
	virtual void 					FSMEnteringState	( const int state, const bool final );

	/**
	 * Trigger the "begin" action of the SimpleFSM progress feedback.
	 * This function cannot be used when FSMDoing/FSMDone are used.
	 */
	template <typename T>
	 	std::shared_ptr<T> 			FSMBegin 			( const std::string & message );

	// Registry functions encapsulated by the FSM_ macros
	void 					        FSMRegistryBegin	();
	void 					        FSMRegistryAdd		( int id, fsmHandler handler, ... );
	void 					        FSMRegistryEnd		( int rootID );

	/**
	 * The entry point for the thread loop
	 */
	void 							FSMThreadLoop		();

	// Private variables
    std::map<int,std::vector<int> > fsmTmpRouteLinks;
	std::map<int,FSMNode>	        fsmNodes;
	FSMNode	*						fsmRootNode;
	FSMNode *						fsmCurrentNode;
	std::list<FSMNode*>				fsmCurrentPath;
	int								fsmTargetState;
	bool 							fsmInsideHandler;
	bool 							fsmThreadActive;
	std::thread *					fsmThread;

private:

	// Thread synchronization variables
	bool 							fsmtPaused;
	bool 							fsmtInterruptRequested;
	std::mutex 						fsmtPauseMutex;
	std::condition_variable 		fsmtPauseChanged;

	// Wait synchronization
	FSMNode *						fsmwState;
	bool 							fsmwStateWaiting;
	std::mutex 						fsmwStateMutex;
	std::condition_variable 		fsmwStateChanged;
	std::mutex 						fsmwWaitMutex;
	std::condition_variable 		fsmwWaitCond;

	// Mutex for safely stopping the thread
	std::mutex 						fsmmThreadSafe;

	// Mutex for FSMGoto
	std::mutex  					fsmGotoMutex;

	// Mutex for accessing fsmPath
	std::mutex  					fsmPathMutex;

	// Progress
	std::string 					fsmProgressResetMsg;

	// Pause/Resume for long periods of idle-ness
	void							_fsmPause();
	void 							_fsmWakeup();

	// Reusable function to run the node handler
	bool 							_callHandler( FSMNode * node, bool inThread );

};


#endif /* end of include guard: SIMPLEFSM_H */
