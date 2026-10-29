# GT6 — Grafana, alertas y comando por Node-RED

## Estado de los artefactos

- `grafana/dashboard.json`: plantilla importable de dashboard con tres paneles. Al importarla en el Grafana del curso, seleccionar el datasource InfluxDB ya configurado y comprobar que el identificador y los nombres de campo coincidan con las series disponibles.
- `nodered/flujo.json`: flujo base para leer la telemetría E31/P15/nodo1 y publicar comandos ON/OFF en `curso/E31/P15/nodo1/cmd`. Antes de desplegar, vincular los nodos MQTT al broker ya configurado en el entorno compartido de Node-RED.
- `img/`: guardar aquí capturas reales del dashboard y de la regla de alerta en estado Firing.
- `Hydrotank.ino`: firmware del nodo.

## Datos del equipo

- Equipo: E31
- Proyecto: P15
- Nodo: nodo1
- Bucket InfluxDB: `mediciones`
- Measurement: `mediciones`
- Field principal para el dashboard: `volumen_l`
- Topics: `curso/E31/P15/nodo1`, `curso/E31/P15/nodo1/estado` y `curso/E31/P15/nodo1/cmd`

## Diseño propuesto del dashboard

1. **Time series — Volumen del estanque:** evolución de `volumen_l` en las últimas 24 h, en litros.
2. **Stat — Volumen actual:** último valor de `volumen_l`. El umbral rojo de ejemplo está en 0,3 L y debe confirmarse o cambiarse por el valor que se haya declarado en el charter y la FSM.
3. **Panel propio — Caudal:** evolución temporal de `caudal`, para detectar cambios en el flujo durante el llenado.

## Regla de alerta (completar tras probar en Grafana)

La guía GT6 solicita un intervalo de evaluación de 1 minuto y un período pendiente (`for`) de 2 minutos. La consulta debe filtrar E31/P15/nodo1 y el field elegido. Registrar aquí:
- Condición y field evaluado: PENDIENTE DE CONFIRMAR EN GRAFANA.
- Umbral y fuente (charter/FSM GT2): PENDIENTE DE CONFIRMAR.
- Desviación estándar `σ` medida en GT1: PENDIENTE DE CONFIRMAR.
- Margen entre operación normal y umbral, en múltiplos de σ: PENDIENTE DE CALCULAR.
- Comportamiento si no llegan datos por más de 5 minutos: la consulta puede quedar sin datos y Grafana mostrar `NoData`; verificar el comportamiento real de la regla.

## Prueba de alerta y capturas

No declarar la regla completada hasta observar el ciclo real `Normal → Pending → Firing → Normal`. Guardar una captura de la regla en `Firing` y otra del dashboard con datos visibles en `img/`.

## Prueba del comando remoto

En Node-RED, probar los inject ON y OFF y observar en el monitor serie del ESP32 el mensaje de comando recibido. La guía pide que ON y OFF actúen sobre un indicador no crítico a través de la FSM, con tiempo máximo de actuación y validación del estado. El comando remoto no debe operar directamente la bomba principal.

## Exportación y seguridad

Exportar desde Grafana el dashboard a `grafana/dashboard.json` y desde Node-RED la pestaña del equipo a `nodered/flujo.json`. No incluir credenciales ni tokens. Las capturas deben ser evidencia auténtica del entorno ejecutado.
