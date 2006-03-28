// zmd-backend.h

#ifndef ZMD_BACKEND_H
#define ZMD_BACKEND_H

#include <zypp/base/LogControl.h>
#include <zypp/ZYpp.h>

#define ZMD_BACKEND_LOG "/var/log/zmd-backend.log"

namespace backend {

// get ZYpp pointer, exit(1) if locked
zypp::ZYpp::Ptr getZYpp();

// init Target (root="/", commit_only=true), exit(1) on error
zypp::Target_Ptr initTarget( zypp::ZYpp::Ptr Z );

}

#endif // ZMD_BACKEND_H
