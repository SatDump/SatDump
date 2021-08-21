/*
 * Copyright 2011-2019, European Organisation for the Exploitation of Meteorological Satellites (EUMETSAT)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RMA_ERROR_HANDLING_HEADER
#define RMA_ERROR_HANDLING_HEADER

/*******************************************************************************

TYPE:
Concrete class
      
PURPOSE:
Provide special-purpose exceptions.
class COutOfBufferException
	exception thrown when an access past the end of a buffer is attempted
class CParamNamedException
	derived from CParamException and providing a mean to identify more precisely
	the nature of the exception.
      
FUNCTION:
no particular.
      
INTERFACES:
See 'INTERFACES' in the module declaration below
      
RESOURCES:	
no particular resources
      
REFERENCE:	
none

PROCESSING:
none
      
DATA:
See 'Data' in the class header below.			
      
LOGIC:
-

*******************************************************************************/

#include <stdarg.h>
#include <string>
#include "ErrorHandling.h"
#include "Types.h"

namespace COMP
{

#define COMP_TRYTHIS \
	try              \
	{
#define COMP_CATCHTHIS \
	}                  \
	catch (...) { LOGCATCHANDTHROW; }
#define COMP_TRYTHIS_SPEED
#define COMP_CATCHTHIS_SPEED

#define cmin(a, b) (((a) < (b)) ? (a) : (b))
#define cmax(a, b) (((a) > (b)) ? (a) : (b))

	// Description:	Meant to be thrown by buffer-like objects
	//				when trying to access past the end of the buffer.
	class COutOfBufferException : public Util::CBaseException
	{

	public:
		// Description:	Constructor.
		// Returns:		Nothing.
		COutOfBufferException() : Util::CBaseException(1) {}

		// Description:	Returns an error string.
		// Returns:		The error string.
		virtual std ::string ExceptionMsg()
		{
			return "Out of buffer";
		}
	};

} // end namespace

namespace Util
{
	class CParamException : public Util::CBaseException
	{
	public:
		CParamException() : Util::CBaseException(1) {}

		//	Description:	Returns the Exception text.
		//	Returns:		Exception text.
		virtual std::string ExceptionMsg() throw() { return "Invalid Parameter"; }
	};
}

#endif
