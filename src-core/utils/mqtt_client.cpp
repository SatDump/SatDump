#include "mqtt_client.h"
#include "core/exception.h"
#include "libs/mqttc/posix_sockets.h"
#include <chrono>
#include <thread>

namespace satdump
{
    void MQTTClient::publish_callback(void **publish_response_callback_state, struct mqtt_response_publish *published)
    {
        auto tthis = (MQTTClient *)(*publish_response_callback_state);
        tthis->callback(std::string((char *)published->topic_name, published->topic_name_size), (uint8_t *)published->application_message, published->application_message_size);
    }

    void *MQTTClient::client_refresher(void *client)
    {
        while (((MQTTClient *)((mqtt_client *)client)->publish_response_callback_state)->run_refresh)
        {
            mqtt_sync((struct mqtt_client *)client);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        return NULL;
    }

    MQTTClient::MQTTClient(std::string addr, std::string port, int bufsize, std::function<void(std::string topic, uint8_t *data, int len)> callback) : callback(callback)
    {
        // Open the non-blocking TCP socket (connecting to the broker)
        sockfd = open_nb_socket(addr.c_str(), port.c_str());

        if (sockfd == -1)
            throw satdump_exception("Failed to open socket: ");

        // Setup client
        sendbuf = (uint8_t *)malloc(bufsize);
        recvbuf = (uint8_t *)malloc(bufsize);
        mqtt_init(&client, sockfd, sendbuf, bufsize, recvbuf, bufsize, publish_callback);
        client.publish_response_callback_state = this;

        // Send connection request to the broker.
        mqtt_connect(&client, client_id, NULL, NULL, 0, NULL, NULL, MQTT_CONNECT_CLEAN_SESSION, 400);

        // Check that we don't have any errors
        if (client.error != MQTT_OK)
            throw satdump_exception("error: " + std::string(mqtt_error_str(client.error)));

        // Start refresh thread
        run_refresh_th = std::thread(client_refresher, &client);
    }

    MQTTClient::~MQTTClient()
    {
        run_refresh = false;
        if (run_refresh_th.joinable())
            run_refresh_th.join();
        free(sendbuf);
        free(recvbuf);
    }

    void MQTTClient::publish(std::string topic, uint8_t *data, int len, uint8_t publish_flags)
    {
        mqtt_publish(&client, topic.c_str(), data, len, publish_flags);

        if (client.error != MQTT_OK)
            throw satdump_exception("error: " + std::string(mqtt_error_str(client.error)));
    }

    void MQTTClient::subscribe(std::string topic, int max_qos)
    {
        mqtt_subscribe(&client, topic.c_str(), max_qos);

        if (client.error != MQTT_OK)
            throw satdump_exception("error: " + std::string(mqtt_error_str(client.error)));
    }
} // namespace satdump