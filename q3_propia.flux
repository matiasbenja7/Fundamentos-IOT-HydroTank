// Pregunta del proyecto: ¿Cuál ha sido el volumen máximo de agua registrado en el estanque durante las últimas 24 horas?
from(bucket: "mediciones")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "mediciones")
  |> filter(fn: (r) => 
    r.equipo == "E31" and 
    r.proyecto == "P15" and 
    r.nodo == "nodo1"
  )
  |> filter(fn: (r) => r._field == "volumen_l")
  |> max()