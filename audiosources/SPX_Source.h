/// @file
/// @author  Boris Mikic
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php
/// 
/// @section DESCRIPTION
/// 
/// Provides a source for OGG format.

#if HAVE_SPX
#ifndef XAL_SPX_SOURCE_H
#define XAL_SPX_SOURCE_H

#include <hltypes/hstring.h>

#include "Source.h"
#include "xalExport.h"

namespace xal
{
	class xalExport SPX_Source : public Source
	{
	public:
		SPX_Source(chstr filename);
		~SPX_Source();

		bool load(unsigned char** output);

	};

}

#endif
#endif
