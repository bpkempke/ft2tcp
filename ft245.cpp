#include "ft245.h"
#include "ft245_rx_thread.h"
#include <QDebug>

void Ft245::rx_callback(const QByteArray &data)
{
	emit rx(data);
}

void Ft245::rx_thread_stopped()
{
	emit close();
}

void Ft245::open(void)
{
    struct ftdi_version_info info = ftdi_get_library_version();

    qDebug() << "ftdi_get_library_version" << info.major << "." << info.minor << "." << info.micro;

	ftdi = ftdi_new();

	if (!ftdi)
	{
        fatal("!ftdi", __FILE__, __LINE__ );

		return;
	}

	if (ftdi_set_interface(ftdi, INTERFACE_A))
	{
		fatal("ftdi_set_interface failed", __FILE__, __LINE__ );
		return close();
	}

	if (ftdi_usb_open(ftdi, 0x0403, 0x6010) < 0)
	{
		fatal("ftdi_usb_open", __FILE__, __LINE__ );
		return close();
	}

	if(ftdi_usb_purge_rx_buffer(ftdi) < 0)
	{
		fatal("ftdi_usb_purge_rx_buffer", __FILE__, __LINE__ );
		return close();
	}

    if (ftdi_set_latency_timer(ftdi, 3) < 0)
    {
        fatal("ftdi_set_latency_timer", __FILE__, __LINE__ );
        return close();
    }
#if 0
	// Fixme, seems already in ftdi_readstream
    if (ftdi_set_bitmode(ftdi, 0xff, BITMODE_SYNCFF))
	{
		fatal("ftdi_set_bitmode", __FILE__, __LINE__ );
		return close();
	}

    uint8_t tx_data[] = {0x0f};
    if (ftdi_write_data(ftdi, tx_data, sizeof(tx_data)) < 0)
    {
        fatal("ftdi_write_data", __FILE__, __LINE__ );
        return close();
    }
#endif
	ft245_rx = new Ft245RxThread;
	ft245_rx->moveToThread(&rx_thread);
	connect(&rx_thread, &QThread::finished, ft245_rx, &QObject::deleteLater);
	connect(this, &Ft245::rx_thread_start, ft245_rx, &Ft245RxThread::doWork);
	connect(this, &Ft245::rx_thread_stop, ft245_rx, &Ft245RxThread::stop);
	connect(ft245_rx, &Ft245RxThread::rx, this, &Ft245::rx);
	connect(ft245_rx, &Ft245RxThread::stopped, this, &Ft245::rx_thread_stopped);
	rx_thread.start();
	emit rx_thread_start(ftdi);

    return;
}

void Ft245::close(void)
{
	if (!ftdi)
	{
		return;
	}

	emit rx_thread_stop();

	rx_thread.quit();
	rx_thread.wait();

	if (ftdi_set_bitmode(ftdi,  0xff, BITMODE_RESET) < 0)
	{
        qDebug() << "ftdi_set_bitmode" << __FILE__ << ":" << __LINE__ ;
	}

	ftdi_usb_close(ftdi);
	ftdi_free(ftdi);
	ftdi = NULL;
}

Ft245::Ft245(QObject *parent) : QObject(parent)
{
	open();
}

Ft245::~Ft245()
{
	close();
}

void Ft245::fatal(const char *msg, const char *file, int n)
{
	qDebug() << msg << "in file" << file << ", line : " << n;

	emit fatal();
}

void Ft245::tx(const QByteArray &data)
{
    ft245_rx->tx(data);
}
