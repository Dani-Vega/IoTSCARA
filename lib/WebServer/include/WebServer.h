#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include "esp_http_server.h"
#include "WiFiManager.h"

#include <functional>

// Definición de callbacks
typedef std::function<void(const std::string &)> SongCallback;
typedef std::function<void(const std::string &)> ColorCallback;

class WebServer
{
public:
    WebServer(WiFiManager &wifiManager);
    ~WebServer();

    void startServer();
    void stopServer();

    std::string getCurrentSong() const { return currentSong; }
    std::string getCurrentColor() const { return currentColor; }

    int getVolume();

    bool getIsPlaying();
    bool isNextSongRequested();
    bool isPreviousSongRequested();
    bool istogglePowerRequested();
    bool iscloseChestRequested();
    bool isOTAUpdateRequested();

private:
    httpd_handle_t server;
    WiFiManager &wifiManager; // Referencia al WiFiManager para comprobar conexión

    static esp_err_t handleRoot(httpd_req_t *req);
    static esp_err_t handleWiFi(httpd_req_t *req);
    static esp_err_t handleWiFiScan(httpd_req_t *req);
    static esp_err_t handleSong(httpd_req_t *req);
    static esp_err_t handleColor(httpd_req_t *req);
    static esp_err_t handleSaveAll(httpd_req_t *req);
    static esp_err_t handleImage(httpd_req_t *req);
    static esp_err_t handleIsConnected(httpd_req_t *req);
    static esp_err_t handlePreviousSong(httpd_req_t *req);
    static esp_err_t handleTogglePlay(httpd_req_t *req);
    static esp_err_t handleNextSong(httpd_req_t *req);
    static esp_err_t handlePlaybackStatus(httpd_req_t *req);
    static esp_err_t handleTogglePower(httpd_req_t *req);
    static esp_err_t handleCloseCofre(httpd_req_t *req);
    static esp_err_t handleSetVolume(httpd_req_t *req);
    static esp_err_t handleOTAUpdateRequested(httpd_req_t *req);

    std::string currentSong;  // Para almacenar la canción actual
    std::string currentColor; // Para almacenar el color actual

    static bool nextSongRequested;
    static bool previousSongRequested;
    static bool togglePowerRequested;
    static bool closeChestRequested;
    static bool OTAUpdateRequested;
};

#endif // WEBSERVER_H