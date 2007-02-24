
#include <sqlite3.h>
#include "zypp/CapAndItem.h"
#include "zypp/ResPool.h"

int read_locks (const zypp::ResPool & pool, sqlite3 *db);
