#ifndef app_mqtt_h
#define app_mqtt_h

#define MQTT_ACK "ACK"

void mqtt_setup();
void mqtt_execute();
bool mqtt_send(const char *payload);

#endif