// zmd-backend.h

#ifndef ZMD_BACKEND_H
#define ZMD_BACKEND_H

#include <string>

#include <zypp/base/LogControl.h>
#include <zypp/ZYpp.h>
#include <zypp/ZYppFactory.h>
#include <zypp/SourceManager.h>
#include <zypp/Source.h>
#include <zypp/Url.h>

#define ZMD_BACKEND_LOG "/var/log/zmd-backend.log"

namespace backend {

// get ZYpp pointer, exit(1) if locked
zypp::ZYpp::Ptr getZYpp( bool readonly = false );

// init Target (root="/", commit_only=true), exit(1) on error
zypp::Target_Ptr initTarget( zypp::ZYpp::Ptr Z, bool commit_only = true );

// remove line breaks
std::string striplinebreaks( const std::string & s );

// restore source by Alias or by Url
bool restoreSources( zypp::SourceManager_Ptr manager, const std::string & alias = "", const std::string & url = "" );

// find (and restore) source by Alias or by Url
zypp::Source_Ref findSource( zypp::SourceManager_Ptr manager, const std::string & alias, const zypp::Url & uri );

}

#endif // ZMD_BACKEND_H
