
#include <sqlite3.h>
#include "zypp/CapAndItem.h"
#include "zypp/ResPool.h"

bool lockItem ( const zypp::CapAndItem &cai_r );
int read_locks (const zypp::ResPool & pool, sqlite3 *db);
