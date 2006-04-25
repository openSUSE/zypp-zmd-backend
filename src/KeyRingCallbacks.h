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
#include <zypp/Digest.h>

///////////////////////////////////////////////////////////////////
namespace zypp {
///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // KeyRingReceive
    ///////////////////////////////////////////////////////////////////
    struct KeyRingReceive : public zypp::callback::ReceiveReport<zypp::KeyRingReport>
    {
	virtual bool askUserToAcceptUnsignedFile( const std::string &file )
	{ return true; }
	virtual bool askUserToAcceptUnknownKey( const std::string &file, const std::string &keyid, const std::string &keyname )
	{ return true; }
	virtual bool askUserToTrustKey( const std::string &keyid, const std::string &keyname, const std::string &keydetails )
	{ return true; }
	virtual bool askUserToAcceptVerificationFailed( const std::string &file, const std::string &keyid, const std::string &keyname )
	{ return true; }
    };


    struct DigestReceive : public zypp::callback::ReceiveReport<zypp::DigestReport>
    {
      virtual bool askUserToAcceptNoDigest( const zypp::Pathname &file )
      { return true; }
      virtual bool askUserToAccepUnknownDigest( const Pathname &file, const std::string &name )
      { return true; }
      virtual bool askUserToAcceptWrongDigest( const Pathname &file, const std::string &requested, const std::string &found )
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

class DigestCallbacks {

  private:
    zypp::DigestReceive _digestReport;

  public:
    DigestCallbacks()
    {
	_digestReport.connect();
    }

    ~DigestCallbacks()
    {
	_digestReport.disconnect();
    }

};


#endif // ZMD_BACKEND_KEYRINGCALLBACKS_H
