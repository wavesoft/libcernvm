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

#include <CernVM/Threads.h>
#include <map>

/**
 * Thread details class
 */
class ThreadDetails {
public:

	/**
	 * Constructor
	 */
	ThreadDetails(std::thread * thread)
		: thread(thread), interrupted(false) { };

	/**
	 * Thread pointer
	 */
	std::thread *		thread;

	/**
	* Flag that denotes that this thread must be interrupted
	*/
	bool				interrupted;

	/**
	 * Condition variable used when the one requested
	 * interrupt is waiting for an acknowlegement.
	 */

};

/**
 * Global thread lookup table
 */
std::map< std::thread::id, ThreadDetails * > threadLookup;

/**
* Functions regarding all threads
*/
namespace threads {

	/**
	* Make the specified std::thread interruptable
	*/
	std::thread * make_interruptible(std::thread * thread)
	{
		// We don't handle null objects
		if (thread == nullptr)
			return thread;
		// Keep thread on lookup table
		threadLookup[thread->get_id()] = new ThreadDetails(thread);
		return thread;
	}

	/**
	* Interrupt a specified thread
	*/
	void interrupt(std::thread * thread, const bool waitAck)
	{
		// We don't handle null objects
		if (thread == nullptr)
			return;
		// Check if we don't handle such thread
		if (threadLookup.find(thread->get_id()) == threadLookup.end())
			return;
		// Mark thread interrupted
		threadLookup[thread->get_id()]->interrupted = true;
	}

	/**
	* Join helper that unmanaged thread when joined
	*/
	void join(std::thread * thread)
	{
		// We don't handle null objects
		if (thread == nullptr)
			return;

		// Find item
		std::map< std::thread::id, ThreadDetails * >::iterator it = threadLookup.find(thread->get_id());
		if (it == threadLookup.end()) return;
		ThreadDetails * details = it->second;

		// Join
		thread->join();

		// Delete
		threadLookup.erase(it);
		delete details;
	}

	/**
	* Interrupt a vector of threads
	*/
	void interrupt_all( const std::vector< std::thread * > & threads, const bool waitAck)
	{
		// Interrupt all threads
		for (std::vector< std::thread * >::const_iterator it = threads.begin(); it != threads.end(); ++it) {
			interrupt(*it, waitAck);
		}
	}

	/**
	* Interrupt a vector of threads
	*/
	void join_all( const std::vector< std::thread * > & threads)
	{
		// Join all threads
		for (std::vector< std::thread * >::const_iterator it = threads.begin(); it != threads.end(); ++it) {
			join(*it);
		}
	}

	/**
	* Remove one thread from collection
	*/
	void remove_one( std::vector< std::thread * > & threads, std::thread * thread)
	{
		// Find item
		std::vector< std::thread * >::iterator it = std::find(threads.begin(), threads.end(), thread);
		if (it == threads.end()) return;
		// Remove
		threads.erase(it);
	}

};

/**
* Functions regarding current thread
*/
namespace this_thread {

	/**
	* Check if this thread is interrupted
	*/
	bool is_interrupted()
	{
		// Check if this thread is interrupted
		std::thread::id id = std::this_thread::get_id();

		// Check if such thread does not exist
		std::map< std::thread::id, ThreadDetails * >::iterator it = threadLookup.find(id);
		if (it == threadLookup.end())
			return false;

		// Check the interrupted flag
		return it->second->interrupted;
	}

}