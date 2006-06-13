//
// utils.cc
//
// utility functions for zmd backend helpers
//

#include <string>
#include <vector>
#include <zypp/base/String.h>
#include "utils.h"

std::string joinlines( const std::string & s )
{
    std::vector<std::string> lines;
    zypp::str::split( s, std::back_inserter( lines ), "\n" );
    return zypp::str::join( lines, "" );
}
