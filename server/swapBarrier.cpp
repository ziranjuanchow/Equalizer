
/* Copyright (c) 2006, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#include "swapBarrier.h"

using namespace std;
using namespace eqBase;

ostream& eqs::operator << ( ostream& os, const SwapBarrier* swapBarrier )
{
    if( !swapBarrier )
        return os;
    
    os << disableFlush << "swapBarrier { name \"" 
       << swapBarrier->getName() << "\" }" << enableFlush << endl;

    return os;
}
