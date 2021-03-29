

/****************************************************************************** 
 * 
 *  file:  FunctionConstraint.h
 * 
 *  Copyright (c) 2005, Michael E. Smoot
 *  All rights reverved.
 *  Class added by Francesco Montesano, 2012
 * 
 *  See the file COPYING in the top directory of this distribution for
 *  more information.
 *  
 *  THE SOFTWARE IS PROVIDED _AS IS_, WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 *  DEALINGS IN THE SOFTWARE.  
 *  
 *****************************************************************************/ 

#ifndef TCLAP_FUNCTIONCONSTRAINT_H
#define TCLAP_FUNCTIONCONSTRAINT_H

#include <string>
#include <vector>
#include "Constraint.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define HAVE_SSTREAM
#endif

#if defined(HAVE_SSTREAM)
#include <sstream>
#elif defined(HAVE_STRSTREAM)
#include <strstream>
#else
#error "Need a stringstream (sstream or strstream) to compile!"
#endif

namespace TCLAP {

/**
 * A Constraint that constrains the Arg to only those values specified
 * in the constraint.
 */
template<class T>
class FunctionConstraint : public Constraint<T>
{

	public:

		/**
		 * Constructor. 
		 * \param f - function which encodes cusmom constraints on the value from 
		 * the command line and returns true or faulse
		 * \param desc - A description of the function behaviour
		 */
		FunctionConstraint(bool (*f)(T), std::string desc);	

		/**
		 * Virtual destructor.
		 */
		virtual ~FunctionConstraint() {}

		/**
		 * Returns a description of the Constraint. 
		 */
		virtual std::string description() const;

		/**
		 * Returns the short ID for the Constraint.
		 */
		virtual std::string shortID() const;

		/**
		 * The method used to verify that the value parsed from the command
		 * line meets the constraint.
		 * \param value - The value that will be checked. 
		 */
		virtual bool check(const T& value) const;
	
	protected:

		/**
		 * The string used to describe the allowed values of this constraint.
		 */
		std::string _typeDesc;

		/**
		 * internal copy of the function to be used to check the value from
		 * the command line
		 */
		bool (*func)(T);

};

template<class T>
FunctionConstraint<T>::FunctionConstraint(bool (*f)(T), std::string desc)
: _typeDesc(desc)
{ 
  func = (*f);
}

template<class T>
bool FunctionConstraint<T>::check( const T& val ) const
{
	return func(val);
}

template<class T>
std::string FunctionConstraint<T>::shortID() const
{
    return _typeDesc;	
}

template<class T>
std::string FunctionConstraint<T>::description() const
{
    return _typeDesc;	
}


} //namespace TCLAP
#endif 

