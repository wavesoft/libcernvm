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
#ifndef ARGUMENTS_LIST_H
#define ARGUMENTS_LIST_H

//#include <boost/variant.hpp>
#include <string>
#include <vector>

/**
 * Minimal implementation of boost::variant
 */
template< typename A, typename B, typename C, typename D >
class CVMVariantClass
{
public:

	// Constructor
	CVMVariantClass(const A & v) { a = v; };
	CVMVariantClass(const B & v) { b = v; };
	CVMVariantClass(const C & v) { c = v; };
	CVMVariantClass(const D & v) { d = v; };

	// Cast to internal types
	operator A () const { return a; }
	operator B () const { return b; }
	operator C () const { return c; }
	operator D () const { return d; }
	
	// Assign types
	CVMVariantClass& operator=(A v) { a = v; return *this; };
	CVMVariantClass& operator=(B v) { b = v; return *this; };
	CVMVariantClass& operator=(C v) { c = v; return *this; };
	CVMVariantClass& operator=(D v) { d = v; return *this; };

private:

	// Internal data
	A a; B b; C c; D d;

};

/* Typedef for variant callbacks */
typedef CVMVariantClass< float, double, int, std::string >								VariantArg;
typedef std::vector< VariantArg >														VariantArgList;

/**
 * Helper class for easily building callback arguments
 */
class ArgumentList {
public:
	ArgumentList( ) : args() { };
	ArgumentList( VariantArg arg ) : args(1,arg) { };
	ArgumentList& operator()( VariantArg arg ) {
		args.push_back(arg);
		return *this;
	}
	operator VariantArgList& () {
		return args;
	}
private:
	VariantArgList 		args;

};

#endif /* end of include guard: ARGUMENTS_LIST_H */
