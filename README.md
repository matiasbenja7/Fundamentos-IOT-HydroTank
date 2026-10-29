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

---

## GT6 — Grafana, alertas y comando por Node-RED

### Estado de los archivos
- `grafana/dashboard.json`: plantilla importable para un dashboard con serie de tiempo, valor actual (Stat) y panel propio.
- `nodered/flujo.json`: plantilla para recibir telemetría por MQTT y publicar comandos `ON`/`OFF` en el tópico `curso/E31/P15/nodo1/cmd`.
- `README_GT6.md`: notas específicas de configuración y pruebas de GT6.
- `img/`: carpeta destinada a capturas reales del dashboard y de la alerta en estado `Firing`.

### Dashboard propuesto
1. **Serie de tiempo:** volumen del estanque (`volumen_l`) durante las últimas 24 horas.
2. **Valor actual (Stat):** último volumen registrado, con color según el umbral real del proyecto.
3. **Panel propio:** evolución del caudal (`caudal`) para observar cambios durante el llenado.

### Regla de alerta
La configuración debe verificarse en Grafana: consulta filtrada para E31/P15/nodo1, intervalo de evaluación de 1 minuto y período pendiente (`for`) de 2 minutos. La regla debe recorrer `Normal → Pending → Firing → Normal` al probar el fenómeno real.

Antes de entregar, completar en esta documentación:
- Umbral de alerta y su origen en el charter/FSM.
- Valor de `σ` medido en GT1 y margen entre operación normal y umbral en múltiplos de `σ`.
- Justificación del período `for` de 2 minutos.
- Comportamiento de la alerta cuando el nodo deja de publicar.

### Prueba de comando remoto
La guía pide que Node-RED envíe comandos de prueba al tópico `/cmd` y que el ESP32 los procese a través de su FSM, con validación de estado y tiempo máximo de actuación. La prueba debe actuar sobre un indicador no crítico, no directamente sobre la bomba principal. Registrar el resultado después de comprobarlo en el laboratorio.

### Evidencias y entrega
Exportar desde el entorno real el dashboard y el flujo de Node-RED; guardar capturas auténticas en `img/`. No subir tokens, contraseñas ni credenciales. Confirmar que los comandos `ON` y `OFF` generan la respuesta esperada en el monitor serie y en el indicador.
