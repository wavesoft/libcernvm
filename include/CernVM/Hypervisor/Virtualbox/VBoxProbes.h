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
#ifndef VBOXPROBES_H
#define VBOXPROBES_H

#include <CernVM/Utilities.h>
#include <CernVM/LocalConfig.h>

#include <string>
#include <map>

using namespace std;

class VBoxLogProbe {
public:

	/**
	 * Create log probe class
	 */
	VBoxLogProbe( const string& path, int tailSize = 81920 ) {
        CRASH_REPORT_BEGIN;
		this->logFile = path + "/VBox.log";
		this->tailSize = tailSize;
        CRASH_REPORT_END;
	};

	/**
	 * Check if the file exists
	 */
	bool 			exists();

	/**
	 * Analyze the log file
	 */
	void 			analyze();

	/**
	 * Flag and information regarding state change
	 */
	bool 			hasState;
	int 			state;

	/**
	 * Flag and information regarding state change
	 */
	bool 			hasResolutionChange;
	int 			resWidth;
	int 			resHeight;
	int 			resBpp;

	/**
	 * The path to the log file
	 */
	string 			logFile;
	int				tailSize;

};


#endif /* end of include guard: VBOXPROBES_H */
