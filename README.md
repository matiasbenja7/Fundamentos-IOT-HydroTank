## Taller 5 - Persistencia en InfluxDB y Consultas Flux

### Ruta de Ingesta
La arquitectura de ingesta de datos del proyecto sigue el flujo oficial del curso:
`ESP32 -> Broker MQTT (Mosquitto) -> Agente de Ingesta (Telegraf) -> Base de Datos de Series Temporales (InfluxDB) -> Visualización y Consultas (Grafana Explore)`

### Esquema de Datos del Equipo
* **Organización:** `curso-iot`
* **Bucket:** `mediciones`
* **Measurement:** `mediciones`

#### Tags Estructurales (Identidad):
* `equipo`: `E31`
* `proyecto`: `P15`
* `nodo`: `nodo1`
* `topic`: `curso/E31/P15/nodo1` (tag auxiliar)

#### Fields del Proyecto:
* `volumen_l`: Volumen medido en litros (Field principal).
* `caudal`: Tasa de flujo/caudal medido.
* `distancia`: Distancia medida por el sensor ultrasónico (cm).
* `estado`: Estado del sistema.
* `objetivo_l`: Volumen objetivo configurado.
* `rssi_dbm`: Calidad de la señal de red (dBm).
* `sensor_ok`: Indicador de operatividad del sensor.