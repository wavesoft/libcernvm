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
#ifndef CONFIG_H_THREADS
#define CONFIG_H_THREADS

#include <thread>
#include <vector>

/**
 * Functions regarding all threads
 */
namespace threads {

    /**
     * Make the specified std::thread interruptable
     */
    std::thread *   make_interruptible( std::thread * thread );

	/**
	 * Stop managing this thread
	 */
	void			unmanage( std::thread * thread );

    /**
     * Interrupt a specified thread
     */
    void            interrupt( std::thread * thread, const bool waitAck = false );

    /**
     * Join helper that unmanaged thread when joined
     */
    void            join( std::thread * thread );

    /**
     * Interrupt a vector of threads
     */
	void            interrupt_all( const std::vector< std::thread * > & threads, const bool waitAck = false);

    /**
     * Interrupt a vector of threads
     */
	void            join_all(const std::vector< std::thread * > & threads);

    /**
     * Remove one thread from collection
     */
    void            remove_one( std::vector< std::thread * > & threads, std::thread * thread );

};

/**
 * Functions regarding current thread
 */
namespace this_thread {

    /**
     * Check if this thread is interrupted
     */
    bool            is_interrupted();

}


#endif /* end of include guard: CONFIG_H_THREADS */
