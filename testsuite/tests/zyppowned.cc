//
// zyppowned.cc
//
// Test zyppOwned*() functions of zmd-backend
//

#include <iostream>

#include <zypp/base/Logger.h>
#include "src/dbsource/zmd-backend.h"

using namespace std;

int
main()
{
    char *data[] = {
	"0http://1.2.3.4/install/novell/nld-10-alpha/nld-10alpha-i386/CD1?alias=SUSE-Linux-Enterprise-Desktop-i386-10-0-20060621-100431",
	"1https://update.novell.com/repo/$RCE/SLED10-Updates/sled-10-i586",
	"2http://www2.ati.com/suse",
	"3ftp://download.nvidia.com/novell",
	NULL
    };

    backend::zyppOwnedFilename( "/tmp/zypp-owned.tmp" );

    backend::addZyppOwned( data[0] );
    backend::addZyppOwned( data[1] );
    backend::addZyppOwned( data[2] );
    backend::addZyppOwned( data[3] );

    if (!backend::isZyppOwned( data[2] ) )
	return 1;
    backend::removeZyppOwned( data[2] );
    if (backend::isZyppOwned( data[2] ) )
	return 2;
    backend::removeZyppOwned( data[3] );
    backend::removeZyppOwned( data[0] );

    if (backend::isZyppOwned( data[0] ) )
	return 3;
    if (backend::isZyppOwned( data[3] ) )
	return 4;
    if (!backend::isZyppOwned( data[1] ) )
	return 5;
    backend::addZyppOwned( data[3] );
    backend::addZyppOwned( data[3] );
    backend::addZyppOwned( data[2] );

    return 0;
}
