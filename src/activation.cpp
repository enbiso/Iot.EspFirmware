#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "activation.h"
#include "config.h"
#include "logger.h"

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer captiveWebServer(80);

void _activation_root_handler();
void _activation_save_handler();

void activation_setup()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("InactiveIOT");

    // if DNSServer is started with "*" for domain name, it will reply with
    // provided IP to all DNS request
    dnsServer.start(DNS_PORT, "*", apIP);

    captiveWebServer.on("/", HTTP_GET, _activation_root_handler);
    captiveWebServer.on("/", HTTP_POST, _activation_save_handler);
    // replay to all requests with same HTML
    captiveWebServer.onNotFound([]() {
        captiveWebServer.sendHeader("Location", "/", true); //Redirect to our html web page
        captiveWebServer.send(302, "text/plane", "");
    });
    captiveWebServer.begin();
}

void activation_execute()
{
    dnsServer.processNextRequest();
    captiveWebServer.handleClient();
}

void _activation_root_handler()
{
    LOG_TRACE("_activation_root_handler");
    String response = R"=====(
<html lang="en" class="route-index">

<head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Setup IOT</title>
    <style>
        body {
    	margin: 30px;
        font-size: 14px;
    	font-family: system-ui;
    }
    .row {
     	display: flex;
        margin: 5px;
    }
    label {
    	margin-right: 20px;
        padding: 10px 0px;
        display: flex;
    	width: 50%;
    }
    input[type="text"],  input[type="password"] {
        padding:10px;
        font-size: 14px;
        width: 50%;
    }
    .radios {
        padding:10px;
        width: 50%;
    }
    button {
        padding:10px;
    	background-color: #0078e7;
    	color: white;
    	border: 0px;
        font-size: 16px;
    	margin: 10px auto;
    	width: 100%;
    }
  </style>
</head>

<body>
    <form method="POST" action="/">
        <div class="row">
            <label>Device Name</label>
            <input required type="text" name="name" placeholder="Device Name" />
        </div>
        <div class="row">
            <label>Access Password</label>
            <input required type="text" name="access" placeholder="Access Password" />
        </div>
        <div class="row">
            <label>WiFi SSID</label>
            <input required type="text" name="wifi_ssid" placeholder="WIFI SSID" />
        </div>
        <div class="row">
            <label>WiFi Password</label>
            <input type="password" name="wifi_password" placeholder="WIFI Password" />
        </div>
        <div class="row">
            <label>DHCP</label>
            <div class="radios">
                <input type="radio" name="dhcp" value="1" checked>&nbsp;Enable&nbsp;
                <input type="radio" name="dhcp" value="0">&nbsp;Disable
            </div>
        </div>
        <div id="dhcp-section">
            <div class="row">
                <label>IP Address</label>
                <input class="dchp-input" type="text" name="ip" placeholder="IP Address" />
            </div>
            <div class="row">
                <label>Subnet Mask</label>
                <input class="dchp-input" type="text" name="subnet" placeholder="Subnet Mask" />
            </div>
            <div class="row">
                <label>Default Gateway</label>
                <input class="dchp-input" type="text" name="gateway" placeholder="Gateway" />
            </div>
        </div>
        <div class="row">
            <label>Custom DNS</label>
            <div class="radios">
                <input type="radio" name="dns" value="1">&nbsp;Enable&nbsp;
                <input type="radio" name="dns" value="0" checked>&nbsp;Disable
            </div>
        </div>
        <div id="dns-section">
            <div class="row">
                <label>Primary DNS</label>
                <input class="dns-input" type="text" name="dns1" placeholder="Primary DNS" />
            </div>
            <div class="row">
                <label>Secondary DNS</label>
                <input class="dns-input" type="text" name="dns2" placeholder="Secondary DNS" />
            </div>
        </div>
        <div class="row">
            <button type="submit">Activate</button>
        </div>
    </form>
    <script>
        const dnsRadios = document.getElementsByName('dns');
        const dhcpRadios = document.getElementsByName('dhcp');
        function dnsChange(value) {
            const inputs = document.getElementsByClassName('dns-input');
            for (var j = 0; j < inputs.length; j++) {
                inputs[j].disabled = value === '0';
            }
            document.getElementById('dns-section').style.display = value === '0' ? 'none' : 'block';
        }
        function dhcpChange(value) {
            const inputs = document.getElementsByClassName('dchp-input');
            for (var j = 0; j < inputs.length; j++) {
                inputs[j].disabled = value === '1';
            }
            if (value === '0') {
                for (var j = 0, length = dnsRadios.length; j < length; j++) {
                    dnsRadios[j].checked = dnsRadios[j].value === '1';
                }
                dnsChange("1");
            }
            document.getElementById('dhcp-section').style.display = value === '1' ? 'none' : 'block';
        }
        for (var i = 0, length = dnsRadios.length; i < length; i++) {
            dnsRadios[i].addEventListener('change', function () {
                dnsChange(this.value);
            });
        }
        for (var i = 0, length = dhcpRadios.length; i < length; i++) {
            dhcpRadios[i].addEventListener('change', function () {
                dhcpChange(this.value);
            });
        }
        dnsChange("0");
        dhcpChange("1");
    </script>

    <body>

</html>
)=====";
    captiveWebServer.send(200, "text/html", response);
}

void _activation_save_handler()
{
    LOG_TRACE("_activation_save_handler");
    activate_data data;
    data.name = captiveWebServer.arg("name");
    data.access = captiveWebServer.arg("access");
    data.wifi_ssid = captiveWebServer.arg("wifi_ssid");
    data.wifi_password = captiveWebServer.arg("wifi_password");
    data.dhcp = captiveWebServer.arg("dhcp") == "1";
    if (!data.dhcp)
    {
        data.ip.fromString(captiveWebServer.arg("ip"));
        data.subnet.fromString(captiveWebServer.arg("subnet"));
        data.gateway.fromString(captiveWebServer.arg("gateway"));
    }
    data.dns = captiveWebServer.arg("dns") == "1";
    if (data.dns)
    {
        data.dns1.fromString(captiveWebServer.arg("dns1"));
        data.dns2.fromString(captiveWebServer.arg("dns2"));
    }
    config_activate(data);
    LOG_INFO("Device Activated. Restarting...")
    ESP.restart();
}