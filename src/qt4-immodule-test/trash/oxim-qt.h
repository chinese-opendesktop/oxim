#include <QObject>
#include <QSocketNotifier>

//#include "gcin-common-qt.h"

class OXIMQt: public QObject
{

    Q_OBJECT

    public slots:

        void handle_message ();

    public:

        /**
         * Constructor.
         */
        OXIMQt ();


        /**
         * Destructor.
         */
        ~OXIMQt ();


        /**
         * A messenger is opened.
         */
        void messenger_opened ();

        /**
         * A messenger is closed.
         */
        void messenger_closed ();


    private:


        /**
         * The notifier for the messenger socket.
         */
        QSocketNotifier *socket_notifier;

};
