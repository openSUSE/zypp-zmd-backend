/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/MediaChangeCallback.h
 *
*/

#ifndef ZMD_BACKEND_MEDIACHANGECALLBACK_H
#define ZMD_BACKEND_MEDIACHANGECALLBACK_H

#include <iostream>

#include "dbsource/utils.h"

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
	int _media_nr;
	std::string _description;

	MediaChangeReceive( )
	    : _media_nr( 0 )
	{
	}

	~MediaChangeReceive( )
	{
	}

	virtual Action requestMedia( zypp::Source_Ref source, unsigned mediumNr, zypp::media::MediaChangeReport::Error error, std::string description )
	{
	    _media_nr = mediumNr;
	    _description = description;

	    DBG << "requestMedia(" << source << ", " << mediumNr << ", " << error << ", " << description << ")" << endl;

	    string scheme( source.url().getScheme() );

	    // if its not CD or DVD, report as normal error and abort

	    if (scheme != "cd"
		&& scheme != "dvd")
	    {
		cerr << "1|Error accessing package: " << error << " (" << joinlines( description ) << ")" << endl;
		mediumNr = 0;	// force abort
	    }

	    if (mediumNr > 0) {

		// request media via stdout

		std::cout << "10|" << mediumNr;

		std::string product_name;

		// get name of the product
		for (zypp::ResStore::iterator it = source.resolvables().begin(); it != source.resolvables().end(); it++)
		{
		    // is it a product object?
		    if (zypp::isKind<zypp::Product>( *it ))
		    {
			product_name = (*it)->name();
			break;
		    }	
		}

		// fall back to source url if source does not provide a product
		if (product_name.empty()) {
		    product_name = source.url().asString();
		}

		std::cout << "|" << product_name << std::endl;

		// FIXME: prodive additonal details about WHY a media change is requested
		// std::cout << "|" << description;
	    }

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

    int mediaNr() const
    { return _changeReceiver._media_nr; }

    std::string description() const
    { return _changeReceiver._description; }

};

#endif // ZMD_BACKEND_MEDIACHANGECALLBACK_H
