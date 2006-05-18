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


    // read callback answer
    //   can either be '0\n' -> false
    //   or '1\n' -> true
    // reads characters from stdin until newline. Defaults to 'false'
    static bool
    readCallbackAnswer()
    {
	char c;
	bool result = false;
	while (std::cin.get( c )) {
	    if (c == '\n')
		break;
	    if (c == '1')
		result = true;
	}
	return result;
    }

    ///////////////////////////////////////////////////////////////////
    // KeyRingReceive
    ///////////////////////////////////////////////////////////////////
    struct KeyRingReceive : public zypp::callback::ReceiveReport<zypp::KeyRingReport>
    {
	virtual bool askUserToAcceptUnsignedFile( const std::string &file )
	{
	  DBG << "1|" << file << std::endl;
	  std::cout << "1|" << file << std::endl;
	  return readCallbackAnswer();
	}
	virtual bool askUserToAcceptUnknownKey( const std::string &file, const std::string &keyid, const std::string &keyname, const std::string &fingerprint )
	{
	  DBG << "2|" << file << "|" << keyid << "|" << keyname << "|" << fingerprint << std::endl;
	  std::cout << "2|" << file << "|" << keyid << "|" << keyname << "|" << fingerprint << std::endl;
	  return readCallbackAnswer();
	}
	virtual bool askUserToTrustKey( const std::string &keyid, const std::string &keyname, const std::string &fingerprint )
	{
	  DBG << "3|" << keyid << "|" << keyname <<  "|" << fingerprint << std::endl;
	  std::cout << "3|" << keyid << "|" << keyname <<  "|" << fingerprint << std::endl;
	  return readCallbackAnswer();
	}
	virtual bool askUserToAcceptVerificationFailed( const std::string &file, const std::string &keyid, const std::string &keyname, const std::string &fingerprint )
	{
	  DBG << "4|" << file << "|" << keyid << "|" << keyname << "|" << fingerprint << std::endl;
	  std::cout << "4|" << file << "|" << keyid << "|" << keyname << "|" << fingerprint << std::endl;
	  return readCallbackAnswer();
	}
    };


    struct DigestReceive : public zypp::callback::ReceiveReport<zypp::DigestReport>
    {
      virtual bool askUserToAcceptNoDigest( const zypp::Pathname &file )
      {
	DBG << "5|" << file << std::endl;
	std::cout << "5|" << file << std::endl;
	return readCallbackAnswer();
      }
      virtual bool askUserToAccepUnknownDigest( const Pathname &file, const std::string &name )
      {
	DBG << "6|" << file << "|" << name << std::endl;
	std::cout << "6|" << file << "|" << name << std::endl;
	return readCallbackAnswer();
      }
      virtual bool askUserToAcceptWrongDigest( const Pathname &file, const std::string &requested, const std::string &found )
      {
	DBG << "7|" << file << "|" << requested << "|" << found << std::endl;
	std::cout << "7|" << file << "|" << requested << "|" << found << std::endl;
	return readCallbackAnswer();
      }
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
