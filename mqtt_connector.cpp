#include "mqtt_connector.h"
#include <mosquittopp.h>
#include <iostream>

mqtt_connector::mqtt_connector(const char *id, const char *host, int port) : mosquittopp(id)
{
	int keepalive = 300;
	int rc = connect(host, port, keepalive);
        std::cout << "Connect: " << mosqpp::strerror(rc) << std::endl;
};

mqtt_connector::~mqtt_connector()
{
}

void mqtt_connector::on_connect(int rc)
{
	std::cout << "Connected with code " << rc << std::endl;
	if(rc == 0){
		/* Only attempt to subscribe on a successful connect. */
		//subscribe(NULL, "temperature/celsius");
	}
}

void mqtt_connector::on_message(const struct mosquitto_message *message)
{
}

void mqtt_connector::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
	std::cout << "Subscription succeeded.\n" << std::endl;
}

int mqtt_connector::send(const char* topic, const char* payload, int size)
{
	return publish(NULL, topic, size, payload);
}

void mqtt_connector::on_disconnect(int rc)
{
	std::cout << "Disconnected with code: " << mosqpp::strerror(rc) << std::endl;
}

void mqtt_connector::on_error()
{
	std::cout << "MQTT error " << std::endl;
}
