/*
  MQTTLog.h
  Copyright (c) 2023 Dale Giancono. All rights reserved..

  This class implements a way to send log messages via MQTT

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*****************************************************************************/
/*INLCUDE GUARD                                                              */
/*****************************************************************************/
#ifndef MQTTLOG_H_
#define MQTTLOG_H_

/*****************************************************************************/
/*INLCUDES                                                                   */
/*****************************************************************************/
#include <mosquitto.h>
#include <assert.h>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <iostream>

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/
/** MQTT BROKER ADDRESS to send MQTT Logs to */
#define MQTTLOG_MQTTBROKERADDRESS                   "localhost"
/** MQTT BROKER PORT to send MQTT Logs to */
#define MQTTLOG_MQTTBROKERPORT                      (1883U)
/** MQTT keep alive period */
#define MQTTLOG_MQTTKEEPALIVEPERIOD_SECONDS         (60U)
/** MQTT quality of servce */
#define MQTTLOG_QOS                                 (0U)
/** Topic that MQTTLOG posts to */
#define MQTTLOG_MQTTTOPIC                             "/LOG"

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/

/*****************************************************************************/
/*CLASS DECLARATION                                                          */
/*****************************************************************************/
class MQTTLog
{
    public:
        MQTTLog(){};

        void initialise(std::string path);
        void log(std::string format, ...);
        void end();

    private:
        static void onMQTTConnect(
            struct mosquitto *client,
            void *obj, 
            int reason_code);
        static void onMQTTDisconnect(
            struct mosquitto *client,
            void *obj, 
            int reason_code);

        struct mosquitto* client;
        std::string logFilePath;

};

void MQTTLog::initialise(std::string path)
{
    int ret;

    /** Initialise the mosquitto mqtt library */
    ret = mosquitto_lib_init();
    assert(MOSQ_ERR_SUCCESS == ret);

    /** Creates a new mosquitto client for the camera */
    this->client = mosquitto_new(NULL, true, this);
    assert(NULL != this->client);

    mosquitto_connect_callback_set(this->client, MQTTLog::onMQTTConnect);
    mosquitto_disconnect_callback_set(this->client, MQTTLog::onMQTTDisconnect);

    ret = mosquitto_connect(
        this->client, 
        MQTTLOG_MQTTBROKERADDRESS, 
        MQTTLOG_MQTTBROKERPORT, 
        MQTTLOG_MQTTKEEPALIVEPERIOD_SECONDS);
    assert(MOSQ_ERR_SUCCESS == ret);

    ret = mosquitto_loop_start(this->client);
    assert(MOSQ_ERR_SUCCESS == ret);
    this->logFilePath = path;

    return;
}

void MQTTLog::end(void)
{
    int ret;

    /** Disconnected from MQTT broker and stop mosquiito */
    ret = mosquitto_disconnect(this->client);
    assert(MOSQ_ERR_SUCCESS == ret);
    ret = mosquitto_loop_stop(this->client, true);
    assert(MOSQ_ERR_SUCCESS == ret);
    mosquitto_destroy(this->client);
}

void MQTTLog::log(std::string format, ...)
{    
    int ret;
    int count;
    char buffer[1000];
    char varBuffer[500];
    char logBuffer[1000];
    va_list args;
    int bufferLen;
    time_t rawtime;
    struct tm * timeinfo;
    static std::string errormsg = "Could not open log file. Please check directory";
    /** The FILE that is used to write logs to. */
    FILE* file;

    /* Process the format and all the various arguments */
    va_start(args, format);
    count = vsnprintf(varBuffer, sizeof(varBuffer), format.c_str(), args);

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    count = snprintf(
        logBuffer, 
        sizeof(logBuffer), 
        "[%d-%d-%d %d:%d:%d]: %s\n", 
        timeinfo->tm_year + 1900, 
        timeinfo->tm_mon + 1, 
        timeinfo->tm_mday, 
        timeinfo->tm_hour, 
        timeinfo->tm_min, 
        timeinfo->tm_sec,
        varBuffer);


    /*Send the MQTT message */
    ret = mosquitto_publish(
        this->client,
        NULL,
        MQTTLOG_MQTTTOPIC,
        count,
        (void*)logBuffer,
        MQTTLOG_QOS,
        false);

    std::cout << logBuffer << std::endl;
    

    /** Opens the configuration file */
    file = fopen(this->logFilePath.c_str(), "a");
    if(file == NULL) 
    {
        /*Send the MQTT message */
        ret = mosquitto_publish(
            this->client,
            NULL,
            MQTTLOG_MQTTTOPIC,
            errormsg.length(),
            (void*)errormsg.c_str(),
            MQTTLOG_QOS,
            false);
        std::cout << errormsg << ": " << this->logFilePath << std::endl;
        return;
    }
    else
    {
        /* Write the message to file*/
        ret = fwrite(logBuffer, count, 1, file);
        /** Close configuration file */
        fclose(file);
    }

    return;
}

void MQTTLog::onMQTTConnect(
    struct mosquitto *client,
    void *obj, 
    int reason_code)
{
    MQTTLog* self = (MQTTLog*)obj;
    self->log("MQTTLog client connected!");
}

/**
 * @brief 
 * A callback function that is called when LibLogStream disconnects from the MQTT broker
 * @param client
 * The mqtt client
 * @param obj
 * not used
 * @param reason_code
  * not used
 * @return none
 */
 void MQTTLog::onMQTTDisconnect(
    struct mosquitto *client,
    void *obj, 
    int reason_code)
{
    MQTTLog* self = (MQTTLog*)obj;
    self->log("MQTTLog client disconnected!");
}

#endif /* MQTTLOG_H_ */