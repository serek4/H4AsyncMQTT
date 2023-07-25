![plainhdr](../assets/pangoplain.jpg)

# Using TLS

TLS support is available for ESP32 targets only

- [Prerequisites](#prerequisites)
- [Add certificates](#add-certificates)
- [Find me daily in these FB groups](#find-me-daily-in-these-fb-groups)

## Prerequisites

- [H4AsyncTCP](https://github.com/HamzaHajeir/H4AsyncTCP), follow its own instructions to activate TLS.
- [H4 Library](https://github.com/HamzaHajeir/H4)
- [H4Tools](https://github.com/HamzaHajeir/H4Tools)

For a complete homogeneous environment with activated TLS, checkout the PlatformIO project [H4Plugins_Env](https://github.com/HamzaHajeir/H4Plugins_Env), which includes all H4 Libraries with appropriate version, and activates TLS further to the project.

## Add certificates

in your project, add these lines under `h4setup()`:

```cpp
H4AsyncMQTT mqttClient;
std::string rootCA = R"(-----BEGIN CERTIFICATE-----
....
....
-----END CERTIFICATE-----
)";


void h4setup() {
    auto testRootCA = reinterpret_cast<const uint8_t*>(const_cast<char*>(rootCA.c_str()));
    mqttClient.secureTLS(testRootCA, rootCA.length() + 1); // +1 for PEM-based certificates (DER doesn't need it)
}
```

Ensure **`https://`** part is explicitly added in the URL:

```cpp
#define MQTT_SERVER "https://192.168.1.34:8883"

...
mqttClient.connect(MQTT_SERVER);
```

---

## Find me daily in these FB groups

- [Pangolin Support](https://www.facebook.com/groups/H4AsyncMQTT/)
- [ESP8266 & ESP32 Microcontrollers](https://www.facebook.com/groups/2125820374390340/)
- [ESP Developers](https://www.facebook.com/groups/ESP8266/)
- [H4/Plugins support](https://www.facebook.com/groups/h4plugins)

I am always grateful for any $upport on [Patreon](https://www.patreon.com/esparto) :)

(C) 2020 Phil Bowles
