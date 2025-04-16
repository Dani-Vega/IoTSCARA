#include "WebServer.h"
#include "esp_log.h"
#include "esp_err.h"
#include <cstring>
#include <cJSON.h>
#include "wifi.h"
#include "gear.h"
#include "power.h"
#include "close.h"
#include "volume.h"
#include "update.h"

static const char *TAG = "WebServer";
static bool isPlaying = false;
static int volume = 0;
bool WebServer::nextSongRequested = false;
bool WebServer::previousSongRequested = false;
bool WebServer::togglePowerRequested = false;
bool WebServer::closeChestRequested = false;
bool WebServer::OTAUpdateRequested = false;

WebServer::WebServer(WiFiManager &wifiManager) : server(nullptr), wifiManager(wifiManager) {}

WebServer::~WebServer()
{
    stopServer();
}

void WebServer::startServer()
{
    ESP_LOGI(TAG, "Starting web server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;

    if (httpd_start(&server, &config) == ESP_OK)
    {
        // 1. Rutas específicas
        httpd_uri_t rootUri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = handleRoot,
            .user_ctx = this};
        httpd_register_uri_handler(server, &rootUri);

        httpd_uri_t wifiUri = {
            .uri = "/wifi",
            .method = HTTP_POST,
            .handler = handleWiFi,
            .user_ctx = this};
        httpd_register_uri_handler(server, &wifiUri);

        httpd_uri_t wifiScanUri = {
            .uri = "/wifi-scan",
            .method = HTTP_GET,
            .handler = handleWiFiScan,
            .user_ctx = this};
        httpd_register_uri_handler(server, &wifiScanUri);

        httpd_uri_t songUri = {
            .uri = "/song",
            .method = HTTP_POST,
            .handler = handleSong,
            .user_ctx = this};
        httpd_register_uri_handler(server, &songUri);

        httpd_uri_t colorUri = {
            .uri = "/color",
            .method = HTTP_POST,
            .handler = handleColor,
            .user_ctx = this};
        httpd_register_uri_handler(server, &colorUri);

        httpd_uri_t saveAllUri = {
            .uri = "/save-all",
            .method = HTTP_POST,
            .handler = handleSaveAll,
            .user_ctx = this};
        httpd_register_uri_handler(server, &saveAllUri);

        httpd_uri_t previousSongUri = {
            .uri = "/previous-song",
            .method = HTTP_POST,
            .handler = handlePreviousSong,
            .user_ctx = this};
        httpd_register_uri_handler(server, &previousSongUri);

        httpd_uri_t togglePlayUri = {
            .uri = "/toggle-play",
            .method = HTTP_POST,
            .handler = handleTogglePlay,
            .user_ctx = this};
        httpd_register_uri_handler(server, &togglePlayUri);

        httpd_uri_t nextSongUri = {
            .uri = "/next-song",
            .method = HTTP_POST,
            .handler = handleNextSong,
            .user_ctx = this};
        httpd_register_uri_handler(server, &nextSongUri);

        httpd_uri_t togglePowerUri = {
            .uri = "/toggle-power",
            .method = HTTP_POST,
            .handler = handleTogglePower,
            .user_ctx = this};
        httpd_register_uri_handler(server, &togglePowerUri);

        httpd_uri_t closeCofreUri = {
            .uri = "/close-cofre",
            .method = HTTP_POST,
            .handler = handleCloseCofre,
            .user_ctx = this};
        httpd_register_uri_handler(server, &closeCofreUri);

        httpd_uri_t OTAUpdateUri = {
            .uri = "/ota-update-requested",
            .method = HTTP_POST,
            .handler = handleOTAUpdateRequested,
            .user_ctx = this};
        httpd_register_uri_handler(server, &OTAUpdateUri);

        httpd_uri_t volumePopupUri = {
            .uri = "/set-volume",
            .method = HTTP_POST,
            .handler = handleSetVolume,
            .user_ctx = this};
        httpd_register_uri_handler(server, &volumePopupUri);

        httpd_uri_t playbackStatusUri = {
            .uri = "/playback-status",
            .method = HTTP_GET,
            .handler = handlePlaybackStatus,
            .user_ctx = this};
        httpd_register_uri_handler(server, &playbackStatusUri);

        httpd_uri_t isConnectedUri = {
            .uri = "/is-connected",
            .method = HTTP_GET,
            .handler = handleIsConnected,
            .user_ctx = this};
        httpd_register_uri_handler(server, &isConnectedUri);

        // 2. Manejador global para imágenes
        httpd_uri_t imageWildcardUri = {
            .uri = "/images/*", // Patrón para todas las imágenes
            .method = HTTP_GET,
            .handler = handleImage,
            .user_ctx = this};
        httpd_register_uri_handler(server, &imageWildcardUri);

        ESP_LOGI(TAG, "Web server started.");
    }
    else
    {
        ESP_LOGE(TAG, "Failed to start web server.");
    }
}

void WebServer::stopServer()
{
    if (server)
    {
        httpd_stop(server);
        server = nullptr;
        ESP_LOGI(TAG, "Web server stopped.");
    }
}

esp_err_t WebServer::handleRoot(httpd_req_t *req)
{
    const char *htmlTemplate = R"rawliteral(

<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cofrew</title>
    <style>
        /* Estilos básicos */
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            background-color: #f4f4f4;
        }

        .container {
            position: relative;
            width: 400px;
            padding: 20px;
            background-color: #ffffff;
            border-radius: 20px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
            text-align: center;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: space-between;
            height: auto;
        }

        h1 {
            font-size: 1.5rem;
            margin-bottom: 20px;
            color: #333;
        }

        h3 {
            font-size: 1.3rem;
            margin-bottom: 15px;
            color: #4caf50;
        }

        .icon-row {
            display: flex;
            justify-content: space-around;
            margin-bottom: 20px;
            width: 100%;
        }

        .icon {
            width: 80px;
            height: 80px;
            background-color: #b8eaf9;
            border-radius: 15px;
            display: flex;
            justify-content: center;
            align-items: center;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
        }

        .icon img {
            width: 130%;
            height: 130%;
            object-fit: contain;
        }

        .form-group {
            width: 100%;
            margin-bottom: 20px;
        }

        label {
            display: block;
            margin-bottom: 8px;
            font-weight: bold;
        }

        input,
        select {
            width: calc(100% - 20px);
            padding: 10px;
            margin-bottom: 15px;
            border: 1px solid #ccc;
            border-radius: 8px;
            font-size: 14px;
        }

        .button-primary {
            background-color: #4caf50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 8px;
            cursor: pointer;
            font-size: 14px;
        }

        .button-primary:hover {
            background-color: #45a049;
        }

        .color-container {
            display: flex;
            justify-content: center;
            align-items: center;
            gap: 20px;
        }

        #colorWheel {
            width: 150px;
            height: 150px;
        }

        .color-preview {
            width: 100px;
            height: 100px;
            border: 1px solid #ccc;
            border-radius: 8px;
        }

        .inputs-container {
            display: flex;
            justify-content: space-between;
            gap: 20px;
            width: 100%;
            margin-top: 45px;
        }

        .input-box {
            flex: 1;
        }

        .buttons-row {
            display: flex;
            justify-content: space-around;
            margin-top: 20px;
            width: 100%;
        }

        .button {
            width: 100px;
            height: 40px;
            background-color: #b8eaf9;
            border: none;
            border-radius: 4px;
            text-align: center;
            line-height: 40px;
            font-size: 14px;
            cursor: pointer;
        }

        .button:hover {
            background-color: #a0d6ea;
        }

        #wifiListContainer {
            width: 97%;
            height: 150px; /* Altura fija para el contenedor */
            overflow-y: auto; /* Scroll vertical si excede la altura */
            border: 1px solid #ccc;
            border-radius: 8px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
            background-color: #f9f9f9;
            margin-bottom: 20px;
            padding: 10px;
        }

        #wifiList div {
            padding: 10px;
            margin: 5px 0;
            border-radius: 5px;
            background-color: #e6f7ff; /* Color de fondo para cada red */
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
            cursor: pointer;
            transition: background-color 0.2s ease, transform 0.1s ease;
        }

        #wifiList div:hover {
            background-color: #b3e0ff;
            transform: scale(1.02);
        }

        #wifiList div:active {
            background-color: #80d0ff;
        }

        .hidden {
            display: none;
        }

        .music-controls {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin-top: 20px;
        }

        .volume-popup {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            background-color: #ffffff;
            padding: 20px;
            border-radius: 15px;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.2);
            display: none;
            flex-direction: column;
            align-items: center;
            gap: 10px;
            z-index: 1000;
            width: 300px;
            text-align: center;
        }

        #volumeSlider {
            width: 80%;
            margin: 10px 0;
        }

        #volumeValue {
            font-size: 1.2rem;
            font-weight: bold;
            color: #4caf50;
        }

        .control-button {
            width: 50px;
            height: 50px;
            font-size: 18px;
            color: #fff;
            background-color: #4caf50;
            border: none;
            border-radius: 50%;
            cursor: pointer;
            display: flex;
            justify-content: center;
            align-items: center;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
            transition: background-color 0.2s;
        }

        .control-volume-button {
            width: auto; /* Ajusta automáticamente al tamaño del contenido */
            height: auto;
            background-color: transparent; /* Fondo transparente */
            border: none; /* Sin borde */
            padding: 0; /* Sin padding */
            cursor: pointer;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        .control-button:hover {
            background-color: #45a049;
        }

        .corner-button {
            position: absolute;
            background-color: #b8eaf9;
            color: #333;
            padding: 8px 15px;
            border: none;
            border-radius: 10px;
            cursor: pointer;
            font-weight: bold;
            box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
            transition: background-color 0.3s ease;
        }

        .corner-button:hover {
            background-color: #a0d6ea;
        }

        .button-icon {
            width: 19px;
            height: 19px;
            object-fit: contain;
        }

        .small-button {
            width: 15px; /* Ajusta el ancho */
            height: 15px; /* Ajusta la altura */
            padding: 5px; /* Espaciado interno reducido */
        }

        .small-button .button-icon {
            width: 15px; /* Tamaño de la imagen */
            height: 15px;
        }


    </style>
    <script src="https://cdn.jsdelivr.net/npm/@jaames/iro@5"></script>
</head>

<body>
    <!-- Home Section -->
    <div id="home" class="container">
        <h1>COFREW</h1>
        <!-- Botón Cerrar -->
        <div id="closeButton" class="corner-button" style="top: 10px; left: 10px;" onclick="closeCofre()">
            <img src="/images/close.png" alt="Cerrar" class="button-icon">
        </div>
        <!-- Botón Nuevo (Actualizar) -->
        <div id="updateButton" class="corner-button small-button" style="top: 10px; left: 50%; transform: translateX(-50%);" onclick="showSection('update')">
            <img src="/images/update.png" alt="Actualizar" class="button-icon">
        </div>
        <!-- Botón Encender -->
        <div id="powerButton" class="corner-button" style="top: 10px; right: 10px;" onclick="togglePower()">
            <img src="/images/power.png" alt="Encender" class="button-icon">
        </div>

        <div class="icon-row">
            <div class="icon" onclick="showSection('wifi')">
                <img src="/images/wifi.png" alt="WiFi">
            </div>
            <div class="icon" onclick="showSection('config')">
                <img src="/images/gear.png" alt="Engrane" style="width: 70%; height: 70%; object-fit: contain;">
            </div>
        </div>
        <!-- Botones comunes -->
        <div class="buttons-row">
            <button class="button" onclick="resetSettings()">Restablecer</button>
            <button class="button" onclick="showSection('home')">Inicio</button>
            <button class="button" onclick="saveAll()">Guardar</button>
        </div>
    </div>

    <!-- WiFi Section -->
        <div id="wifi" class="container hidden">
            <h3>Configurar WiFi</h3>
            <div class="form-group">
                <label for="wifiList">Redes disponibles:</label>
                <div id="wifiListContainer">
                    <div id="wifiList"></div>
                </div>
            </div>
            <div class="form-group">
                <label for="ssid">SSID:</label>
                <input type="text" id="ssid" placeholder="Nombre de la red Wi-Fi">
            </div>
            <div class="form-group">
                <label for="password">Contraseña:</label>
                <input type="password" id="password" placeholder="Contraseña de la red">
            </div>
            <button class="button-primary" onclick="saveWiFi()">Guardar Wi-Fi</button>
        <!-- Botones comunes -->
        <div class="buttons-row">
            <button class="button" onclick="resetSettings()">Restablecer</button>
            <button class="button" onclick="showSection('home')">Inicio</button>
            <button class="button" onclick="saveAll()">Guardar</button>
        </div>
    </div>

    <!-- Sección Actualizar -->
    <div id="update" class="container hidden">
        <h3>Actualizar Sistema</h3>
        <button class="button-primary" onclick="requestOTAUpdate()">Actualizar</button>
        <div class="buttons-row">
            <button class="button" onclick="showSection('home')">Inicio</button>
        </div>
    </div>

    <!-- Config Section -->
    <div id="config" class="container hidden">
        <h3>Configurar Canción</h3>
        <div class="form-group">
            <label for="song">Seleccionar Canción:</label>
            <select id="song">
                <option value="0">Detener</option>
                <option value="1">White Flag</option>
                <option value="2">Brillas</option>
                <option value="3">APT</option>
            </select>
            <button class="button-primary" onclick="saveSong()">Guardar Canción</button>
        </div>

        <div class="music-controls">
            <!-- Botón de control de volumen -->
            <button class="control-volume-button" onclick="toggleVolumePopup()" aria-label="Configurar volumen">
                <img src="/images/volume.png" alt="Volumen" class="button-icon">
            </button>
            <button class="control-button" onclick="previousSong()">
                &#9664;&#9664;
            </button>
            <button class="control-button" id="playPauseButton" onclick="togglePlay()">
                &#10073;&#10073;
            </button>
            <button class="control-button" onclick="nextSong()">
                &#9654;&#9654;
            </button>
        </div>

        <div class="volume-popup hidden">
            <h3>Ajustar Volumen</h3>
            <input id="volumeSlider" type="range" min="0" max="30" value="0" />
            <span id="volumeValue">15</span>
            <button class="button-primary" onclick="saveVolume()">Guardar Volumen</button>
        </div>

        <h3>Seleccionar Color</h3>
        <div class="form-group">
            <div class="color-container">
                <div id="colorWheel"></div>
                <div id="slider" style="height: 150px;"></div>
                <div class="color-preview" id="colorPreview"></div>
            </div>
            <div class="inputs-container">
                <div class="input-box">
                    <label for="hex">Código Hexadecimal:</label>
                    <input type="text" id="hex" placeholder="#FFFFFF" onchange="updateFromHex()">
                </div>
                <div class="input-box">
                    <label for="rgb">RGB:</label>
                    <input type="text" id="rgb" placeholder="255,255,255" onchange="updateFromRGB()">
                </div>
            </div>
            <button class="button-primary" onclick="saveColor()">Guardar Color</button>
        </div>

        <!-- Botones comunes -->
        <div class="buttons-row">
            <button class="button" onclick="resetSettings()">Restablecer</button>
            <button class="button" onclick="showSection('home')">Inicio</button>
            <button class="button" onclick="saveAll()">Guardar</button>
        </div>
    </div>

    <script>
        function loadWiFiNetworks() {
            fetch('/wifi-scan')
                .then(response => response.json())
                .then(networks => {
                    const wifiList = document.getElementById('wifiList');
                    wifiList.innerHTML = ''; // Limpiar lista anterior

                    networks.forEach(network => {
                        const networkDiv = document.createElement('div');
                        networkDiv.textContent = network;

                        networkDiv.onclick = () => {
                            document.getElementById('ssid').value = network;
                            highlightSelectedNetwork(networkDiv);
                        };

                        wifiList.appendChild(networkDiv);
                    });
                })
                .catch(error => console.error('Error al cargar las redes WiFi:', error));
        }

        function highlightSelectedNetwork(selectedDiv) {
            // Quitar el resaltado de todas las redes
            document.querySelectorAll('#wifiList div').forEach(div => {
                div.style.backgroundColor = '#e6f7ff';
                div.style.color = '#333';
            });

            // Resaltar la red seleccionada
            selectedDiv.style.backgroundColor = '#4caf50';
            selectedDiv.style.color = '#fff';
        }

        loadWiFiNetworks();

        async function saveAll() {
            const ssid = document.getElementById("ssid").value;
            const password = document.getElementById("password").value;
            const song = document.getElementById("song").value;
            const color = colorPicker.color.hexString;

            const payload = {
                wifi: {
                    ssid,
                    password,
                },
                song,
                color,
            };

            try {
                const response = await fetch("/save-all", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify(payload),
                });

                if (response.ok) {
                    alert("Configuraciones guardadas correctamente.");
                } else {
                    alert("Error al guardar configuraciones.");
                }
            } catch (error) {
                console.error("Error al enviar las configuraciones:", error);
                alert("Error al guardar configuraciones.");
            }
        }

        const colorPicker = new iro.ColorPicker("#colorWheel", {
            width: 150,
            color: "#FFFFFF",
            layout: [
                { component: iro.ui.Wheel },
                { component: iro.ui.Slider, options: { sliderType: 'saturation', sliderDirection: 'vertical' } }
            ]
        });

        colorPicker.on("color:change", function (color) {
            const rgb = color.rgb;
            document.getElementById("colorPreview").style.backgroundColor = color.hexString;
            document.getElementById("hex").value = color.hexString;
            document.getElementById("rgb").value = `${rgb.r},${rgb.g},${rgb.b}`;
        });

        function updateFromHex() {
            const hex = document.getElementById("hex").value;
            colorPicker.color.hexString = hex;
        }

        function updateFromRGB() {
            const rgb = document.getElementById("rgb").value.split(",").map(Number);
            colorPicker.color.rgb = { r: rgb[0], g: rgb[1], b: rgb[2] };
        }

        function saveWiFi() {
            const ssid = document.getElementById("ssid").value;
            const password = document.getElementById("password").value;
            fetch("/wifi", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ ssid, password })
            }).then(() => alert("Wi-Fi configurado"));
        }

        let isPlaying = false;

        function previousSong() {
            fetch('/previous-song', { method: 'POST' })
        }

        function togglePlay() {
            isPlaying = !isPlaying;
            fetch('/toggle-play', { method: 'POST' })
                .then(() => {
                    const playPauseButton = document.getElementById('playPauseButton');
                    playPauseButton.innerHTML = isPlaying ? '&#10073;&#10073;' : '&#9654;';
                });
        }

        function nextSong() {
            fetch('/next-song', { method: 'POST' })
        }

        function saveSong() {
            const song = document.getElementById("song").value;
            fetch("/song", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ song })
            }).then(() => alert("Canción guardada"));
        }

        function saveColor() {
            const color = colorPicker.color.hexString;
            fetch("/color", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ color })
            }).then(() => alert("Color guardado"));
        }

        function resetSettings() {
            document.getElementById("hex").value = "#FFFFFF";
            document.getElementById("rgb").value = "255,255,255";
            document.getElementById("song").value = "noSong";
            colorPicker.color.set("#FFFFFF");
            alert("Configuraciones restablecidas");
        }

        async function showSection(sectionId) {
            if (['config', 'update'].includes(sectionId)) {
                const isConnected = await checkInternetConnection();

                if (!isConnected) {
                    if (sectionId === 'config') {
                        showAlert('No está conectado a internet', {
                            confirmText: 'Entrar de todos modos',
                            cancelText: 'Cerrar',
                            onConfirm: () => {
                                closeAlert();
                                switchSection(sectionId);
                            },
                            onCancel: closeAlert
                        });
                    } else if (sectionId === 'update') {
                        showAlert('No está conectado a internet', {
                            cancelText: 'Cerrar',
                            onCancel: closeAlert
                        });
                    }
                    return;
                }
            }
            switchSection(sectionId);
        }

        function switchSection(sectionId) {
            const sections = document.querySelectorAll('.container');
            sections.forEach(section => section.classList.add('hidden'));
            document.getElementById(sectionId).classList.remove('hidden');
        }

        async function checkInternetConnection() {
            try {
                const response = await fetch('/is-connected');
                const data = await response.json();
                return data.connected;
            } catch (error) {
                console.error('Error al verificar la conexión a Internet:', error);
                return false;
            }
        }

        function showAlert(message, { confirmText, cancelText, onConfirm, onCancel }) {
            const alertOverlay = document.createElement('div');
            alertOverlay.style.position = 'fixed';
            alertOverlay.style.top = 0;
            alertOverlay.style.left = 0;
            alertOverlay.style.width = '100%';
            alertOverlay.style.height = '100%';
            alertOverlay.style.backgroundColor = 'rgba(0, 0, 0, 0.5)';
            alertOverlay.style.display = 'flex';
            alertOverlay.style.justifyContent = 'center';
            alertOverlay.style.alignItems = 'center';
            alertOverlay.style.zIndex = 1000;

            const alertBox = document.createElement('div');
            alertBox.style.width = '300px';
            alertBox.style.padding = '20px';
            alertBox.style.backgroundColor = '#fff';
            alertBox.style.borderRadius = '10px';
            alertBox.style.boxShadow = '0 4px 10px rgba(0, 0, 0, 0.2)';
            alertBox.style.textAlign = 'center';

            const alertMessage = document.createElement('p');
            alertMessage.style.marginBottom = '20px';
            alertMessage.style.color = '#333';
            alertMessage.style.fontSize = '16px';
            alertMessage.textContent = message;

            const buttonContainer = document.createElement('div');
            buttonContainer.style.display = 'flex';
            buttonContainer.style.justifyContent = 'space-between';

            if (confirmText) {
                const confirmButton = document.createElement('button');
                confirmButton.style.backgroundColor = '#4caf50';
                confirmButton.style.color = '#fff';
                confirmButton.style.padding = '10px 15px';
                confirmButton.style.border = 'none';
                confirmButton.style.borderRadius = '5px';
                confirmButton.style.cursor = 'pointer';
                confirmButton.textContent = confirmText;
                confirmButton.onclick = onConfirm;
                buttonContainer.appendChild(confirmButton);
            }

            if (cancelText) {
                const cancelButton = document.createElement('button');
                cancelButton.style.backgroundColor = '#f44336';
                cancelButton.style.color = '#fff';
                cancelButton.style.padding = '10px 15px';
                cancelButton.style.border = 'none';
                cancelButton.style.borderRadius = '5px';
                cancelButton.style.cursor = 'pointer';
                cancelButton.textContent = cancelText;
                cancelButton.onclick = onCancel;
                buttonContainer.appendChild(cancelButton);
            }

            alertBox.appendChild(alertMessage);
            if (buttonContainer.childNodes.length > 0) {
                alertBox.appendChild(buttonContainer);
            }
            alertOverlay.appendChild(alertBox);

            alertOverlay.classList.add('alert-overlay');
            document.body.appendChild(alertOverlay);
        }

        function closeAlert() {
            const alertOverlay = document.querySelector('.alert-overlay');
            if (alertOverlay) document.body.removeChild(alertOverlay);
        }

        function togglePower() {
            fetch('/toggle-power', { method: 'POST' })
        }

        function closeCofre() {
            fetch('/close-cofre', { method: 'POST' })
        }

        function requestOTAUpdate() {
            fetch("/ota-update-requested", { method: "POST" })
            .then(response => response.text())
            .then(message => alert(message))
            .catch(error => console.error("Error al solicitar actualización:", error));
        }

        let isVolumePopupVisible = false;

        function toggleVolumePopup() {
            const popup = document.querySelector(".volume-popup");
            isVolumePopupVisible = !isVolumePopupVisible;
            popup.style.display = isVolumePopupVisible ? "flex" : "none";
        }

        function saveVolume() {
            const volume = document.getElementById("volumeSlider").value;

            console.log("Volume enviado: ", JSON.stringify({ volume })); // Depuración

            fetch("/set-volume", {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify({ volume: parseInt(volume) }), // Asegura que sea un número entero
            })
                .then((response) => {
                    if (response.ok) {
                        toggleVolumePopup();
                    } else {
                        alert("Error en la petición al servidor.");
                    }
                })
                .catch((error) => {
                    console.error("Error al guardar el volumen:", error);
                });
        }

        const volumeSlider = document.getElementById("volumeSlider");
        const volumeValue = document.getElementById("volumeValue");

        volumeSlider.addEventListener("input", () => {
            volumeValue.textContent = volumeSlider.value;
        });

        // Evento para cerrar el popup si se hace clic fuera de él
        document.addEventListener("click", function (event) {
            const popup = document.querySelector(".volume-popup");
            const volumeButton = document.querySelector(".control-volume-button");

            // Verificar si el popup está visible
            if (isVolumePopupVisible) {
                // Si el clic NO es dentro del popup ni del botón que lo abre
                if (!popup.contains(event.target) && !volumeButton.contains(event.target)) {
                    popup.style.display = "none";
                    isVolumePopupVisible = false; // Actualiza la variable de estado
                }
            }
        });

        async function fetchPlaybackStatus() {
            try {
                const response = await fetch('/playback-status');
                const data = await response.json();
                const playPauseButton = document.getElementById('playPauseButton');

                if (data.isPlaying) {
                    playPauseButton.innerHTML = '&#10073;&#10073;'; // Icono de pausa
                } else {
                    playPauseButton.innerHTML = '&#9654;'; // Icono de reproducción
                }

                isPlaying = data.isPlaying; // Sincroniza la variable local
            } catch (error) {
                console.error('Error al obtener el estado de reproducción:', error);
            }
        }
        
        window.onload = function () {
            fetchPlaybackStatus();
        };
    </script>
</body>

</html>

    )rawliteral";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, htmlTemplate, HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleWiFi(httpd_req_t *req)
{
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char buffer[256];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1); // Leer el cuerpo de la solicitud
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    buffer[ret] = '\0';                // Asegurar que el buffer sea una cadena terminada en null
    cJSON *root = cJSON_Parse(buffer); // Parsear el JSON recibido
    if (!root)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *ssidJson = cJSON_GetObjectItem(root, "ssid");
    const cJSON *passwordJson = cJSON_GetObjectItem(root, "password");

    if (!ssidJson || !passwordJson || !cJSON_IsString(ssidJson) || !cJSON_IsString(passwordJson))
    {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid fields");
        return ESP_FAIL;
    }

    // Guardar credenciales en NVS
    const std::string newSSID = ssidJson->valuestring;
    const std::string newPassword = passwordJson->valuestring;
    server->wifiManager.saveCredentialsToNVS(newSSID, newPassword);

    // Cambiar a modo STA con las nuevas credenciales
    server->wifiManager.switchToSTA(newSSID, newPassword);

    cJSON_Delete(root);

    // Responder con éxito
    return httpd_resp_send(req, "WiFi settings applied successfully.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleWiFiScan(httpd_req_t *req)
{
    // Obtener la instancia de WebServer desde el contexto del usuario
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        ESP_LOGE(TAG, "Failed to get server instance from user context");
        return ESP_FAIL;
    }

    // Usar el miembro wifiManager de la instancia
    std::vector<std::string> networks = server->wifiManager.scanNetworks();
    if (networks.empty())
    {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "[]", HTTPD_RESP_USE_STRLEN);
    }

    cJSON *jsonArray = cJSON_CreateArray();
    for (const auto &network : networks)
    {
        cJSON_AddItemToArray(jsonArray, cJSON_CreateString(network.c_str()));
    }

    const char *jsonString = cJSON_PrintUnformatted(jsonArray);
    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);
    cJSON_Delete(jsonArray);
    free((void *)jsonString);

    return ret;
}

esp_err_t WebServer::handleSong(httpd_req_t *req)
{
    // Obtener la instancia de WebServer desde el contexto del usuario
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        ESP_LOGE(TAG, "Failed to get server instance from user context");
        return ESP_FAIL;
    }

    char buffer[256];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer));
    if (ret > 0)
    {
        buffer[ret] = '\0';
        ESP_LOGI(TAG, "Received song config: %s", buffer);

        // Parsear el JSON y actualizar el valor de currentSong
        cJSON *root = cJSON_Parse(buffer);
        if (root)
        {
            cJSON *songJson = cJSON_GetObjectItem(root, "song");
            if (songJson && cJSON_IsString(songJson))
            {
                server->currentSong = songJson->valuestring; // Actualiza el valor actual
                ESP_LOGI(TAG, "Updated current song to: %s", server->currentSong.c_str());
            }
            cJSON_Delete(root);
        }

        httpd_resp_send(req, "Song settings saved", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t WebServer::handleColor(httpd_req_t *req)
{
    // Obtener la instancia de WebServer desde el contexto del usuario
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        ESP_LOGE(TAG, "Failed to get server instance from user context");
        return ESP_FAIL;
    }

    char buffer[256];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer));
    if (ret > 0)
    {
        buffer[ret] = '\0';
        ESP_LOGI(TAG, "Received color config: %s", buffer);

        // Parsear el JSON y actualizar el valor de currentColor
        cJSON *root = cJSON_Parse(buffer);
        if (root)
        {
            cJSON *colorJson = cJSON_GetObjectItem(root, "color");
            if (colorJson && cJSON_IsString(colorJson))
            {
                server->currentColor = colorJson->valuestring; // Actualiza el valor actual
                ESP_LOGI(TAG, "Updated current color to: %s", server->currentColor.c_str());
            }
            cJSON_Delete(root);
        }

        httpd_resp_send(req, "Color settings saved", HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t WebServer::handleSaveAll(httpd_req_t *req)
{
    // Obtener la instancia de WebServer desde el contexto del usuario
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        ESP_LOGE(TAG, "Failed to get server instance from user context");
        return ESP_FAIL;
    }

    char buffer[512]; // Buffer más grande para múltiples configuraciones
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    buffer[ret] = '\0';
    ESP_LOGI(TAG, "Received save-all config: %s", buffer);

    // Parsear el JSON recibido
    cJSON *root = cJSON_Parse(buffer);
    if (!root)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extraer y guardar la canción
    cJSON *songJson = cJSON_GetObjectItem(root, "song");
    if (songJson && cJSON_IsString(songJson))
    {
        server->currentSong = songJson->valuestring;
        ESP_LOGI(TAG, "Song updated: %s", server->currentSong.c_str());
    }

    // Extraer y guardar el color
    cJSON *colorJson = cJSON_GetObjectItem(root, "color");
    if (colorJson && cJSON_IsString(colorJson))
    {
        server->currentColor = colorJson->valuestring;
        ESP_LOGI(TAG, "Color updated: %s", server->currentColor.c_str());
    }

    cJSON_Delete(root);

    // Responder con éxito
    return httpd_resp_send(req, "All settings saved successfully.", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleImage(httpd_req_t *req)
{
    const char *uri = req->uri;

    // Determinar qué imagen devolver
    if (strcmp(uri, "/images/wifi.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)wifi_png, wifi_png_len);
    }
    else if (strcmp(uri, "/images/gear.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)gear_png, gear_png_len);
    }
    else if (strcmp(uri, "/images/close.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)close_png, close_png_len);
    }
    else if (strcmp(uri, "/images/power.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)power_png, power_png_len);
    }
    else if (strcmp(uri, "/images/volume.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)volume_png, volume_png_len);
    }
    else if (strcmp(uri, "/images/update.png") == 0)
    {
        httpd_resp_set_type(req, "image/png");
        return httpd_resp_send(req, (const char *)update_png, update_png_len);
    }

    // Si no coincide ninguna imagen, devolver 404
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Image not found");
    return ESP_FAIL;
}

esp_err_t WebServer::handleIsConnected(httpd_req_t *req)
{
    WebServer *server = static_cast<WebServer *>(req->user_ctx);
    if (!server)
    {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    bool isConnected = server->wifiManager.isConnectedToInternet();

    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "connected", isConnected);

    const char *jsonString = cJSON_PrintUnformatted(response);
    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);

    cJSON_Delete(response);
    free((void *)jsonString);

    return ret;
}

esp_err_t WebServer::handlePreviousSong(httpd_req_t *req)
{
    previousSongRequested = true;
    ESP_LOGI(TAG, "Reproduciendo canción anterior");
    return httpd_resp_send(req, "Previous song command received", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleTogglePlay(httpd_req_t *req)
{
    isPlaying = !isPlaying;

    if (isPlaying)
    {
        ESP_LOGI(TAG, "Reproducción reanudada");
    }
    else
    {
        ESP_LOGI(TAG, "Reproducción pausada");
    }

    return httpd_resp_send(req, isPlaying ? "Playing" : "Paused", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleNextSong(httpd_req_t *req)
{
    nextSongRequested = true;
    ESP_LOGI(TAG, "Reproduciendo siguiente canción");
    return httpd_resp_send(req, "Next song command received", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handlePlaybackStatus(httpd_req_t *req)
{
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "isPlaying", isPlaying);

    const char *jsonString = cJSON_PrintUnformatted(response);
    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, jsonString, HTTPD_RESP_USE_STRLEN);

    cJSON_Delete(response);
    free((void *)jsonString);

    return ret;
}

esp_err_t WebServer::handleTogglePower(httpd_req_t *req)
{
    togglePowerRequested = true;
    ESP_LOGI(TAG, "Power toggled");
    return httpd_resp_send(req, "Power toggled", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleCloseCofre(httpd_req_t *req)
{
    closeChestRequested = true;
    ESP_LOGI(TAG, "Cofre cerrado");
    return httpd_resp_send(req, "Cofre cerrado", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleSetVolume(httpd_req_t *req)
{
    char buffer[256];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid request");
    }

    buffer[ret] = '\0';
    cJSON *root = cJSON_Parse(buffer);
    if (!root)
    {
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
    }

    cJSON *volumeJson = cJSON_GetObjectItem(root, "volume");
    if (!volumeJson || !cJSON_IsNumber(volumeJson))
    {
        cJSON_Delete(root);
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid volume");
    }

    volume = volumeJson->valueint;
    ESP_LOGI(TAG, "Volume set to: %d", volume);

    cJSON_Delete(root);
    return httpd_resp_send(req, "Volume updated", HTTPD_RESP_USE_STRLEN);
}

esp_err_t WebServer::handleOTAUpdateRequested(httpd_req_t *req)
{
    OTAUpdateRequested = true;
    ESP_LOGI(TAG, "Update Requested");
    return httpd_resp_send(req, "Update Requested", HTTPD_RESP_USE_STRLEN);
}

bool WebServer::getIsPlaying()
{
    return isPlaying;
}

bool WebServer::isNextSongRequested()
{
    bool requested = nextSongRequested;
    nextSongRequested = false; // Reset after reading
    return requested;
}

bool WebServer::isPreviousSongRequested()
{
    bool requested = previousSongRequested;
    previousSongRequested = false; // Reset after reading
    return requested;
}

bool WebServer::istogglePowerRequested()
{
    bool requested = togglePowerRequested;
    togglePowerRequested = false; // Reset after reading
    return requested;
}

bool WebServer::iscloseChestRequested()
{
    bool requested = closeChestRequested;
    closeChestRequested = false; // Reset after reading
    return requested;
}

bool WebServer::isOTAUpdateRequested()
{
    bool requested = OTAUpdateRequested;
    OTAUpdateRequested = false; // Reset after reading
    return requested;
}

int WebServer::getVolume()
{
    return volume;
}
