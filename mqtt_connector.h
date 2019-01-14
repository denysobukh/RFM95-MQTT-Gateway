#ifndef MQTT_CONNECTOR_H
#define MQTT_CONNECTOR_H

#include <mosquittopp.h>

class mqtt_connector : public mosqpp::mosquittopp
{
	public:
		mqtt_connector(const char *id, const char *host, int port);
		~mqtt_connector();

		void on_connect(int rc);
		void on_message(const struct mosquitto_message *message);
		void on_subscribe(int mid, int qos_count, const int *granted_qos);
                int send(const char* topic, const char* payload, int size);

		void on_disconnect(int rc);
 		void on_error();
};

#endif
