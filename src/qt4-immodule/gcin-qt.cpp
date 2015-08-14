#include "gcin-qt.h"

#ifdef QT4
using namespace Qt;
#endif

/* Static variables */
static OXIMQt *client = NULL;


/* Bindings */
void gcin_client_messenger_opened ()
{
    client->messenger_opened ();
}


void gcin_client_messenger_closed ()
{
    client->messenger_closed ();
}


/* Implementations */
OXIMQt::OXIMQt (): socket_notifier (NULL) {
    client = this;
}


OXIMQt::~OXIMQt () {
    client = NULL;
}


void OXIMQt::messenger_opened ()
{
}


void OXIMQt::messenger_closed ()
{
}


void OXIMQt::handle_message ()
{
}

