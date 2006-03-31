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
	cout << "3|A transaction is already in progress." << endl;
	cerr << "A transaction is already in progress." << endl;
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
	cout << "3|" << excpt_r.asUserString() << endl;
	cerr << excpt_r.asString() << endl;
	exit(1);
    }
    return T;
}

};

// EOF
