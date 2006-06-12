/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
// zmd-backend.cc
// ZMD backend helpers

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
	cerr << "3|" << backend::striplinebreaks( excpt_r.asUserString() ) << endl;
	cout << excpt_r.asString() << endl;
	exit(1);
    }
    return T;
}


std::string
striplinebreaks( const std::string & s )
{
    std::string result = s;
    do {
	string::size_type nlpos = result.find( "\n" );
	if (nlpos == string::npos) break;
	result = result.erase( nlpos, 1 );
    }
    while( 1 );
    return result;
}


};

// EOF
