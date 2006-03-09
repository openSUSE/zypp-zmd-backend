/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/MediaChangeCallback.cc
 *
*/

#ifndef ZMD_BACKEND_MEDIACHANGECALLBACK_H
#define ZMD_BACKEND_MEDIACHANGECALLBACK_H

#include <iostream>

#include <zypp/base/Logger.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Package.h>
#include <zypp/target/rpm/RpmCallbacks.h>

///////////////////////////////////////////////////////////////////
namespace ZyppRecipients {
///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // MediaChangeCallback
    ///////////////////////////////////////////////////////////////////
    struct MediaChangeReceive : public zypp::callback::ReceiveReport<zypp::media::MediaChangeReport>
    {
	zypp::Resolvable::constPtr _last;
	int last_reported;

	MediaChangeReceive( )
	{
	}

	~MediaChangeReceive( )
	{
	}

	virtual Action requestMedia(zypp::Source_Ref source, unsigned mediumNr, zypp::media::MediaChangeReport::Error error, std::string description)
	{
	    // request media via stdout

	    std::cout << "10|" << mediumNr << "|" << description << std::endl;

#if 0	// maybe 'description' is not sufficient
		std::string product_name;

		// get name of the product
		for (zypp::ResStore::iterator it = source.resolvables().begin(); it != source.resolvables().end(); it++)
		{
		    // is it a product object?
		    if (zypp::isKind<zypp::Product>(*it))
		    {
			product_name = (*it)->name();
			break;
		    }	
		}
#endif

	    // and abort here.
	    // This will end the 'transact' helper and its up to ZMD to evaluate the
	    // media change request issued to stdout above and re-start 'transact'
	    //
	    return zypp::media::MediaChangeReport::ABORT;
	}
    };


///////////////////////////////////////////////////////////////////
}; // namespace ZyppRecipients
///////////////////////////////////////////////////////////////////

class MediaChangeCallback {

  private:
    ZyppRecipients::MediaChangeReceive _changeReceiver;

  public:
    MediaChangeCallback()
    {
	_changeReceiver.connect();
    }

    ~MediaChangeCallback()
    {
	_changeReceiver.disconnect();
    }

};

#endif // ZMD_BACKEND_MEDIACHANGECALLBACK_H
