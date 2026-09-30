# GT4 - MQTT HydroTank

## Información del equipo

- **Equipo:** E31
- **Proyecto:** P15 - Estanque de agua
- **Nodo:** nodo1

## Tópicos MQTT

| Tópico | QoS | Retained | Descripción |
|---|---:|---|---|
| `curso/E31/P15/nodo1` | Por verificar | true | Datos principales calibrados del proyecto |
| `curso/E31/P15/nodo1/estado` | Por verificar | true | Estado actual de la FSM |
| `curso/E31/P15/nodo1/cmd` | Por verificar | false | Canal reservado para comandos |

## Payload

El nodo publica un payload JSON plano con los datos del proyecto.

Ejemplo:

```json
{
  "distancia": 131.6,
  "caudal": 0,
  "volumen_l": 0,
  "objetivo_l": 0,
  "estado": 0,
  "sensor_ok": 1,
  "rssi_dbm": -48
}
