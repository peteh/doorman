#pragma once
#ifdef ESP8266
    #include <ESP8266WebServer.h>
#endif
#ifdef ESP32
    #include <WebServer.h>
#endif

#include "settings.h"
class ApiServer
{
private:
#ifdef ESP8266
    ESP8266WebServer m_server;
#endif

#ifdef ESP32
    WebServer m_server;
#endif
    Settings *m_settings;

    /**
     * @brief Get the web Content Type that corresponds to a file type.
     *
     * @param filename the file name like image.jpg
     * @return String the content type, e.g. text/html for index.html
     */
    String getContentType(String filename);

public:
    /**
     * @brief Construct a new Api Server that handles requests on the web /api/ endpoint.
     *
     * @param settings The settings of the device
     */
    ApiServer(Settings *settings)
        : m_settings(settings)
    {
    }

    /**
     * @brief Starts the webserver and registers all routes.
     *
     */
    void begin();

    /**
     * @brief Handles the root page and forwards to the main page.
     *
     */
    void handleRoot();

    /**
     * @brief Handles web page requests like index.html or style.css
     * and loads files files from the www dir of the filesystem.
     *
     */
    void handleWeb();

    void handleMainPage();

    void handleCodesPage();

    void handleCodeConfigSave();

    void handleSettingsPage();

    void handleDeviceSettings();

    void handleDeviceSettingsSave();

    void handleCodeConfig();

    /**
     * @brief Streams the current configuration file config.json.
     *
     */
    void handleDeviceConfig();
    

    /**
     * @brief Reboots the device, e.g. to reload the WiFi Config.
     *
     */
    void handleDeviceReboot();

    /**
     * @brief Handles active clients and responses.
     *
     */
    void handleClient();
};