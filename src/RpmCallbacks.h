/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/RpmCallbacks.cc
 *
*/

#ifndef ZMD_BACKEND_RPMCALLBACKS_H
#define ZMD_BACKEND_RPMCALLBACKS_H

#include <iostream>

#include <zypp/base/Logger.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Package.h>
#include <zypp/target/rpm/RpmCallbacks.h>

///////////////////////////////////////////////////////////////////
namespace ZyppRecipients {
///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // InstallPkgCallback
    ///////////////////////////////////////////////////////////////////
    struct InstallPkgReceive : public zypp::callback::ReceiveReport<zypp::target::rpm::InstallResolvableReport>
    {
	zypp::Resolvable::constPtr _last;
	int & _step;				// step counter for install & receive steps
	int last_reported;

	InstallPkgReceive( int & step )
	    : _step( step )
	{
	}

	~InstallPkgReceive( )
	{
	}

	virtual void reportbegin()
	{
	  //                                ! this is 'preparing'
	  std::cout << "1|" << ++_step << "|1|" << std::endl;
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Resolvable::constPtr resolvable)
	{
DBG << "start(" << *resolvable << ")" << std::endl;
	  // initialize the counter
	  last_reported = 0;

	  // if we have started this resolvable already, don't do it again
	  if( _last == resolvable )
	    return;
	  //                              ! this is 'installing' 
	  std::cout << "1|" << ++_step << "|2|" << *resolvable << std::endl;
	  _last = resolvable;
	}

	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{
	    std::cout << "2|" << value << "|100" << std::endl;
	    return true;
	}

	virtual Action problem(
	  zypp::Resolvable::constPtr resolvable
	  , zypp::target::rpm::InstallResolvableReport::Error error
	  , std::string description
	  , zypp::target::rpm::InstallResolvableReport::RpmLevel level
	)
	{
DBG << "problem(" << *resolvable << "," << description << ")" << std::endl;
	    if (level != zypp::target::rpm::InstallResolvableReport::RPM_NODEPS_FORCE)
	    {
		DBG << "Retrying installation problem with too low severity (" << level << ")" << std::endl;
		return zypp::target::rpm::InstallResolvableReport::ABORT;
	    }

	    _last = zypp::Resolvable::constPtr();

#if 0
		// "R" =  retry
		if (ret == "R") return zypp::target::rpm::InstallResolvableReport::RETRY;

		// "C" = cancel
		if (ret == "C") return zypp::target::rpm::InstallResolvableReport::ABORT;

		// otherwise ignore
		return zypp::target::rpm::InstallResolvableReport::IGNORE;
#endif

	    return zypp::target::rpm::InstallResolvableReport::problem( resolvable, error, description, level );
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, Error error, std::string reason, zypp::target::rpm::InstallResolvableReport::RpmLevel level)
	{
DBG << "finish(" << *resolvable << "," << error << ", " << reason << ")" << std::endl;
	    string errmsg;
	    switch (error) {
		case NO_ERROR:
		    return;
		break;
		case NOT_FOUND:
		    errmsg = "Package not found";
		break;
		case IO:
		    errmsg = "I/O error";
		break;
		case INVALID:
		    errmsg = "Invalid package";
		break;
	    }
	    if (level == RPM_NODEPS_FORCE) {
		std::cout << "3|" << errmsg << ": " << reason << std::endl;
	    }
	    return;
	}
    };


    ///////////////////////////////////////////////////////////////////
    // RemovePkgCallback
    ///////////////////////////////////////////////////////////////////
    struct RemovePkgReceive : public zypp::callback::ReceiveReport<zypp::target::rpm::RemoveResolvableReport>
    {
	int & _step;				// step counter for install & receive steps

	RemovePkgReceive( int & step )
	    : _step( step )
	{
	}

	virtual void reportbegin()
	{
	  //                                ! this is 'preparing'
	  std::cout << "1|" << ++_step << "|1|" << std::endl;
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Resolvable::constPtr resolvable)
	{
	  //                                ! this is 'removing' 
	  std::cout << "1|" << ++_step << "|3|" << *resolvable << std::endl;
	}

	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{
	    std::cout << "2|" << value << "|100" << std::endl;
	    return true;
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::target::rpm::RemoveResolvableReport::Error error, std::string reason)
	{
	    if (error != NO_ERROR) {
		std::cout << "3|" << reason << std::endl;
	    }
	}
    };


///////////////////////////////////////////////////////////////////
}; // namespace ZyppRecipients
///////////////////////////////////////////////////////////////////

class RpmCallbacks {

  private:
    ZyppRecipients::InstallPkgReceive _installReceiver;
    ZyppRecipients::RemovePkgReceive _removeReceiver;

    int _step_counter;

  public:
    RpmCallbacks()
	: _installReceiver( _step_counter )
	, _removeReceiver( _step_counter )
	, _step_counter( 0 )
    {
	_installReceiver.connect();
	_removeReceiver.connect();
    }

    ~RpmCallbacks()
    {
	_installReceiver.disconnect();
	_removeReceiver.disconnect();
    }

};

#endif // ZMD_BACKEND_RPMCALLBACKS_H
