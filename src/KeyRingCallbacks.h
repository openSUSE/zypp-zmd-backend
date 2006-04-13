/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/KeyRingCallbacks.cc
 *
*/

#ifndef ZMD_BACKEND_KEYRINGCALLBACKS_H
#define ZMD_BACKEND_KEYRINGCALLBACKS_H

#include <iostream>

#include <zypp/base/Logger.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Pathname.h>
#include <zypp/KeyRing.h>

///////////////////////////////////////////////////////////////////
namespace zypp {
///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // KeyRingReceive
    ///////////////////////////////////////////////////////////////////
    struct KeyRingReceive : public zypp::callback::ReceiveReport<zypp::KeyRingReport>
    {
	virtual bool askUserToAcceptUnsignedFile( const Pathname &file )
	{ return true; }
	virtual bool askUserToAcceptUnknownKey( const Pathname &file, const std::string &keyid, const std::string &keyname )
	{ return true; }
	virtual bool askUserToTrustKey( const std::string &keyid, const std::string &keyname, const std::string &keydetails )
	{ return true; }
	virtual bool askUserToAcceptVerificationFailed( const Pathname &file, const std::string &keyid, const std::string &keyname )
	{ return true; }
	virtual bool askUserToAcceptFileWithoutChecksum( const zypp::Pathname &file )
	{ return true; }
    };


///////////////////////////////////////////////////////////////////
}; // namespace zypp
///////////////////////////////////////////////////////////////////

class KeyRingCallbacks {

  private:
    zypp::KeyRingReceive _keyRingReport;

  public:
    KeyRingCallbacks()
    {
	_keyRingReport.connect();
    }

    ~KeyRingCallbacks()
    {
	_keyRingReport.disconnect();
    }

};

#endif // ZMD_BACKEND_KEYRINGCALLBACKS_H
