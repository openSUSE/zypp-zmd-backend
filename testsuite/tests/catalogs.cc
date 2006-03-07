//
// catalogs.cc
//
// just read catalogs table and create empty Sources
//

#include <iostream>

#include <zypp/base/Logger.h>
#include "src/dbsource/DbSources.h"
#include "src/dbsource/DbAccess.h"


using std::endl;

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	ERR << "usage: " << argv[0] << " <database>" << endl;
	return 1;
    }

    MIL << "Creating db" << endl;

    DbAccess db (argv[1]);

    if (!db.openDb(false))
	return 1;

    MIL << "Calling dbsources" << endl;

    DbSources s(db.db());

    const SourcesList & sources = s.sources();

    MIL << "Found " << sources.size() << " sources" << endl;

    return 0;
}
