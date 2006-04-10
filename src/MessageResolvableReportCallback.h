/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zmd/backend/MessageResolvableReportCallback.h
 *
*/

#ifndef ZMD_BACKEND_MESSAGERESOLVABLEREPORTCALLBACK_H
#define ZMD_BACKEND_MESSAGERESOLVABLEREPORTCALLBACK_H

#include <iostream>

#include <zypp/base/Logger.h>
#include <zypp/ZYppCallbacks.h>
#include <zypp/Package.h>

///////////////////////////////////////////////////////////////////
namespace ZyppRecipients {
///////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // MessageResolvableReportCallback
    ///////////////////////////////////////////////////////////////////
    struct MessageResolvableReportReceive : public zypp::callback::ReceiveReport<zypp::target::MessageResolvableReport>
    {

	MessageResolvableReportReceive( )
	{
	}

	~MessageResolvableReportReceive( )
	{
	}

	virtual void show (Message::constPtr message)
	{
	    if (message == NULL) return;

	    DBG << "show(" << *message << ")" << endl;


	    std::cout << "20|" << *message << endl;

	    return;
	}
    };


///////////////////////////////////////////////////////////////////
}; // namespace ZyppRecipients
///////////////////////////////////////////////////////////////////

class MessageResolvableReportCallback {

  private:
    ZyppRecipients::MessageResolvableReportReceive _messageReceiver;

  public:
    MessageResolvableReportCallback()
    {
	_messageReceiver.connect();
    }

    ~MessageResolvableReportCallback()
    {
	_messageReceiver.disconnect();
    }
};

#endif // ZMD_BACKEND_MESSAGERESOLVABLEREPORTCALLBACK_H
