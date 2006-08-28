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

#include <stdlib.h>
#include <iostream>

#include <zypp/base/Logger.h>
#include <zypp/base/Sysconfig.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Pathname.h>
#include <zypp/KeyRing.h>
#include <zypp/Digest.h>


#define SECURITYPATH "/etc/sysconfig/security"
#define SECURITYTAG "CHECK_SIGNATURES"

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
	if (getenv( "KEYRING_CALLBACK" ) == NULL)
	    return true;

	char c;
	bool result = false;
	while (std::cin.get( c )) {
	    if (c == '\n')
		break;
	    if (c == '1')
		result = true;
	}
	DBG << "answer " << result << std::endl;
	return result;

    }

    ///////////////////////////////////////////////////////////////////
    // KeyRingReceive
    ///////////////////////////////////////////////////////////////////
    struct KeyRingReceive : public zypp::callback::ReceiveReport<zypp::KeyRingReport>
    {
	bool _enabled;
	KeyRingReceive()
	    : _enabled( true )
	{
	}

	void disable( bool value )
	{
	    _enabled = !value;
	    MIL << "KeyRingReceive is now " << (_enabled ? "en" : "dis") << "abled." << std::endl;
	}
 
	virtual bool askUserToAcceptUnsignedFile( const std::string &file )
	{
	  if (!_enabled) return true;
	  DBG << "21|" << file << std::endl;
	  std::cout << "21|" << file << std::endl;
	  return readCallbackAnswer();
	}
	virtual bool askUserToAcceptUnknownKey( const std::string &file, const std::string &id )
	{
	  if (!_enabled) return true;
	  DBG << "22|" << file << "|" << id << "|" << keyname << "|" << fingerprint << std::endl;
	  std::cout << "22|" << file << "|" << id << "|" << "Unknown name" << "|" << "unknown fingerprint" << std::endl;
	  return readCallbackAnswer();
	}
  
  virtual bool askUserToImportKey( const PublicKey &key )
  {
    DBG << "By default backend does not import keys for now." << std::endl;
    return false;
  }
  
  virtual bool askUserToTrustKey(  const PublicKey &key  )
	{
	  if (!_enabled) return true;
    DBG << "23|" << key.id() << "|" << key.name() <<  "|" << key.fingerprint() << std::endl;
    std::cout << "23|" << key.id() << "|" << key.name() <<  "|" << key.fingerprint() << std::endl;
	  return readCallbackAnswer();
	}
  virtual bool askUserToAcceptVerificationFailed( const std::string &file,  const PublicKey &key  )
	{
	  if (!_enabled) return true;
    DBG << "24|" << file << "|" << key.id() << "|" << key.name() << "|" << key.fingerprint() << std::endl;
    std::cout << "24|" << file << "|" << key.id() << "|" << key.name() << "|" << key.fingerprint() << std::endl;
	  return readCallbackAnswer();
	}
    };


    struct DigestReceive : public zypp::callback::ReceiveReport<zypp::DigestReport>
    {
	bool _enabled;
	DigestReceive()
	    : _enabled( true )
	{
	}

	void disable( bool value )
	{
	    _enabled = !value;
	    MIL << "DigestReceive is now " << (_enabled ? "en" : "dis") << "abled." << std::endl;
	}
 
      virtual bool askUserToAcceptNoDigest( const zypp::Pathname &file )
      {
	  if (!_enabled) return true;
	DBG << "25|" << file << std::endl;
	std::cout << "25|" << file << std::endl;
	return readCallbackAnswer();
      }
      virtual bool askUserToAccepUnknownDigest( const Pathname &file, const std::string &name )
      {
	  if (!_enabled) return true;
	DBG << "26|" << file << "|" << name << std::endl;
	std::cout << "26|" << file << "|" << name << std::endl;
	return readCallbackAnswer();
      }
      virtual bool askUserToAcceptWrongDigest( const Pathname &file, const std::string &requested, const std::string &found )
      {
	  if (!_enabled) return true;
	DBG << "27|" << file << "|" << requested << "|" << found << std::endl;
	std::cout << "27|" << file << "|" << requested << "|" << found << std::endl;
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
	std::map<std::string,std::string> data = zypp::base::sysconfig::read( SECURITYPATH );
	std::map<std::string,std::string>::const_iterator it = data.find( SECURITYTAG );
	if (it != data.end())
	    _keyRingReport.disable( it->second != "yes" );

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
	std::map<std::string,std::string> data = zypp::base::sysconfig::read( SECURITYPATH );
	std::map<std::string,std::string>::const_iterator it = data.find( SECURITYTAG );
	if (it != data.end())
	    _digestReport.disable( it->second != "yes" );

	_digestReport.connect();
    }

    ~DigestCallbacks()
    {
	_digestReport.disconnect();
    }

};


#endif // ZMD_BACKEND_KEYRINGCALLBACKS_H
