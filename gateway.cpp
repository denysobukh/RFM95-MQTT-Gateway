#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include "mosquittopp.h"
#include <libconfig.h++>
#include <unistd.h>
#include <sstream>
#include <time.h>
#include <signal.h>
#include <syslog.h>

#include <bcm2835.h>
#include <RH_RF69.h>
#include <RH_RF95.h>

#include "mqtt_connector.h"

#define RF_CS_PIN  RPI_V2_GPIO_P1_22 // Slave Select on GPIO25 so P1 connector pin #22
#define RF_IRQ_PIN RPI_V2_GPIO_P1_07 // IRQ on GPIO4 so P1 connector pin #7
#define RF_RST_PIN RPI_V2_GPIO_P1_11 // Reset on GPIO17 so P1 connector pin #11

RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);

struct Message {
	int32_t pressure;
	int32_t humidity;
	int32_t temperature;
	int32_t voltage;
} msg;

float freq;
int port;
std::string client_id, host, topic;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

const std::string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d %X %z", &tstruct);
	return buf;
}

void signal_handler(int sig_num){
	printf("\n%s Break received, exiting!\n", __BASEFILE__);
	force_exit=true;
	mosqpp::lib_cleanup();
	syslog (LOG_INFO, "exited");
	closelog ();
}

int main(int argc, char** argv) {

	// Print preamble:
	std::cout << "RFM95 to MQTT Gateway\r\n==================" << std::endl;

	signal (SIGINT, signal_handler);

	setlogmask (LOG_UPTO (LOG_INFO));
	openlog ("RFM95 to MQTT Gateway", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	syslog (LOG_INFO, "started by user %d", getuid ());

	// help https://hyperrealm.github.io/libconfig/libconfig_manual.html#The-C_002b_002b-API
	libconfig::Config cfg;

	// Read the file. If there is an error, report it and exit.
	try
	{
		cfg.readFile("gateway.cfg");
	}
	catch (const libconfig::FileIOException &fioex)
	{
		std::cerr << "I/O error while reading file gateway.cfg" << std::endl;
		syslog (LOG_INFO, "I/O error while reading file gateway.cfg");
		return(EXIT_FAILURE);
	}
	catch (const libconfig::ParseException &pex)
	{
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
		<< " - " << pex.getError() << std::endl;
		return(EXIT_FAILURE);
	}

	try
	{
		freq = cfg.lookup("radio_freq");
		client_id = cfg.lookup("client_id").c_str();
		host = cfg.lookup("host").c_str();
		port = cfg.lookup("port");
		topic = cfg.lookup("topic").c_str();
	}
	catch (const libconfig::SettingNotFoundException &nfex)
	{
		std::cerr << "No setting in configuration file." << std::endl;
		return(EXIT_FAILURE);
	}

	std::cout << "Radio frequency: " << freq << std::endl;
	std::cout << "Host: " << host << std::endl;
	std::cout << "Port: " << port << std::endl;
	std::cout << "Client id: " << client_id << std::endl;
	std::cout << "MQTT Topic: " << topic << std::endl;

	// Setup and configure rf radio

	if (!bcm2835_init()) {
		fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
		return 1;
	}
	printf( "RF95 CS=GPIO%d", RF_CS_PIN);

	#ifdef RF_IRQ_PIN
	printf( ", IRQ=GPIO%d", RF_IRQ_PIN );
	// IRQ Pin input/pull down
	pinMode(RF_IRQ_PIN, INPUT);
	bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
	// Now we can enable Rising edge detection
	bcm2835_gpio_ren(RF_IRQ_PIN);
	#endif

	#ifdef RF_RST_PIN
	printf( ", RST=GPIO%d", RF_RST_PIN );
	// Pulse a reset on module
	pinMode(RF_RST_PIN, OUTPUT);
	digitalWrite(RF_RST_PIN, LOW );
	bcm2835_delay(150);
	digitalWrite(RF_RST_PIN, HIGH );
	bcm2835_delay(100);
	#endif

	std::cout << "\nRadio initialized." << std::endl;

	class mqtt_connector *mqtt_;
	try
	{
		mosqpp::lib_init();
	}
	catch (int e) {
		std::cout << "mosqpp::lib_init exception " << e << std::endl;
		return(EXIT_FAILURE);
	}

	try
	{
		mqtt_ = new mqtt_connector(client_id.c_str(), host.c_str(), port);
	}
	catch (int e) {
		std::cout << "mqtt_connector exception " << e << std::endl;
		return(EXIT_FAILURE);
	}

	if (!rf95.init()) {
		fprintf( stderr, "\nRF95 module init failed, Please verify wiring/module\n" );
		return -1;
	}
	// Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

	// The default transmitter power is 13dBm, using PA_BOOST.
	// If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
	// you can set transmitter powers from 5 to 23 dBm:
	//  driver.setTxPower(23, false);
	// If you are using Modtronix inAir4 or inAir9,or any other module which uses the
	// transmitter RFO pins and not the PA_BOOST pins
	// then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true.
	// Failure to do that will result in extremely low transmit powers.
	// rf95.setTxPower(14, true);


	// RF95 Modules don't have RFO pin connected, so just use PA_BOOST
	// check your country max power useable, in EU it's +14dB
	rf95.setTxPower(14, false);

	// You can optionally require this module to wait until Channel Activity
	// Detection shows no activity on the channel before transmitting by setting
	// the CAD timeout to non-zero:
	//rf95.setCADTimeout(10000);

	// Adjust Frequency
	rf95.setFrequency(freq);

	// If we need to send something

	//rf95.setThisAddress(RF_NODE_ID);
	//rf95.setHeaderFrom(RF_NODE_ID);

	// Be sure to grab all node packet
	// we're sniffing to display, it's a demo
	rf95.setPromiscuous(true);

	//        Bw125Cr45Sf128 = 0,        ///< Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Default medium range
	//        Bw500Cr45Sf128,            ///< Bw = 500 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on. Fast+short range
	//        Bw31_25Cr48Sf512,          ///< Bw = 31.25 kHz, Cr = 4/8, Sf = 512chips/symbol, CRC on. Slow+long range
	//        Bw125Cr48Sf4096,           ///< Bw = 125 kHz, Cr = 4/8, Sf = 4096chips/symbol, CRC on. Slow+long range

	rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);
	rf95.setPreambleLength(8);

	// We're ready to listen for incoming message
	rf95.setModeRx();

	syslog (LOG_INFO, "Radio OK NodeID = %s Listening @ %3.2fMHz\n", client_id, freq);

	//Begin the main body of code
	while (!force_exit)
	{

		#ifdef RF_IRQ_PIN
		// We have a IRQ pin ,pool it instead reading
		// Modules IRQ registers from SPI in each loop

		// Rising edge fired ?
		if (bcm2835_gpio_eds(RF_IRQ_PIN)) {
			// Now clear the eds flag by setting it to 1
			bcm2835_gpio_set_eds(RF_IRQ_PIN);
			//printf("Packet Received, Rising event detect for pin GPIO%d\n", RF_IRQ_PIN);
			#endif

			if (rf95.available()) {
				// Should be a message for us now
				uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
				uint8_t len  	= sizeof(buf);
				uint8_t from 	= rf95.headerFrom();
				uint8_t to   	= rf95.headerTo();
				uint8_t id   	= rf95.headerId();
				uint8_t flags	= rf95.headerFlags();;
				int8_t  rssi 	= rf95.lastRssi();

				if (rf95.recv(buf, &len)) {

					printf("Packet #%d %02d bytes, Addr %d->%d, RSSI: %ddB, Flags[0x%02X]: ", id, len, from, to, rssi, flags);
					printbuffer(buf, len);

					if (sizeof(msg) == len) {
						memcpy(&msg, buf, len);
						printf("\r\nDecoded: T = %i, H = %i, p = %i, U = %i \r\n", msg.temperature, msg.humidity, msg.pressure, msg.voltage);
						try
						{
							std::ostringstream ostr;
							ostr << "<message time=\"" << currentDateTime() << "\"";
							ostr << " from=\"" << std::to_string(from) << "\"";
							ostr << " to=\"" << std::to_string(to) << "\"";
							ostr << " rssi=\"" << std::to_string(rssi) << "\"";
							ostr << " id=\"" << std::to_string(id) << "\">";
							ostr << "<temperature>" << msg.temperature / 10.0 << "</temperature>";
							ostr << "<humidity>" << msg.humidity / 10.0 << "</humidity>";
							ostr << "<pressure>" << msg.pressure << "</pressure>";
							ostr << "<voltage>" << msg.voltage / 100.0 << "</voltage>";

							ostr << "</message>";
							std::string m = ostr.str();

							int i = 1, r;
							do {
								r = mqtt_->send(topic.c_str(), m.size(), m.c_str(), 0, true);
							  if (r != MOSQ_ERR_SUCCESS) {
									printf("Message sending error (retry #%d) : %s\n", i, mosqpp::strerror(r));
									mqtt_->reconnect();
								}
							} while (r != MOSQ_ERR_SUCCESS && i++ < 3);
						}
						catch (int e) {
							std::cout << "mqtt_send exception " << e << std::endl;
							return(EXIT_FAILURE);
						}

					}
					else {
						printf("\r\nUnknown payload\n\r");
					}



				} else {
					printf("receive failed");
				}
				printf("\n");
			}

			#ifdef RF_IRQ_PIN
		}
		#endif

		bcm2835_delay(5);
	}

	printf( "\n%s Ending\n", __BASEFILE__ );
	bcm2835_close();
	return 0;
}
