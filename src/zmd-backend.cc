/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
// zmd-backend.cc
// ZMD backend helpers

#include "dbsource/utils.h"
#include "zmd-backend.h"

#include <zypp/zypp_detail/ZYppReadOnlyHack.h>

using namespace zypp;
using namespace std;

namespace backend {

ZYpp::Ptr
getZYpp( bool readonly )
{
    if (readonly)
	zypp::zypp_readonly_hack::IWantIt();

    ZYpp::Ptr Z = NULL;
    try {
	Z = zypp::getZYpp();
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	cerr << "1|A transaction is already in progress." << endl;
	cout << "A transaction is already in progress." << endl;
	exit(1);
    }
    return Z;
}

Target_Ptr
initTarget( ZYpp::Ptr Z, bool commit_only )
{
    Target_Ptr T;

    try {
	Z->initTarget( "/", commit_only );	// its always "/", and we never populate the pool (commit_only = true)
	T = Z->target();
    }
    catch (Exception & excpt_r) {
	ZYPP_CAUGHT (excpt_r);
	cerr << "3|" << joinlines( excpt_r.asUserString() ) << endl;
	cout << excpt_r.asString() << endl;
	exit(1);
    }
    return T;
}


// restore source by Alias or by Url
// prefer by Alias, use Url if Alias is empty
// will restore all sources if Alias and Url are empty

bool
restoreSources( SourceManager_Ptr manager, const string & alias, const string & url )
{
    try {
	manager->restore( "/", true, alias, url );
    }
    catch (const Exception & excpt_r) {
	cerr << "2|Can't restore sources: " << joinlines( excpt_r.asUserString() ) << endl;
	ZYPP_CAUGHT (excpt_r);
	ERR << "Couldn't restore sources, alias '" << alias << "', url '" << url << "'" << endl;
	return false;
    }
    return true;
}


// find (and restore) source by Alias or by Url
// prefer by Alias, use Url if Alias is empty

Source_Ref
findSource( SourceManager_Ptr manager, const string & alias, const Url & url )
{
    Source_Ref source;

    // restore matching sources

    if (backend::restoreSources( manager, alias, alias.empty() ? url.asString() : "" )) {
	try {
	    if (alias.empty()) {
		source = manager->findSourceByUrl( url );
	    }
	    else {
		source = manager->findSource( alias );
	    }
	}
	catch (const Exception & excpt_r) {
	    cerr << "1|Can't find source: " << joinlines( excpt_r.asUserString() ) << ", alias '" << alias << "', url '" << url << endl;
	    ZYPP_CAUGHT (excpt_r);
	    ERR << "Can't find source" << ", alias '" << alias << "', url '" << url << endl;
	}
    }

    return source;
}

};

// EOF
