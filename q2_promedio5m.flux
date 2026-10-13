from(bucket: "mediciones")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "mediciones")
  |> filter(fn: (r) => 
    r.equipo == "E31" and 
    r.proyecto == "P15" and 
    r.nodo == "nodo1"
  )
  |> filter(fn: (r) => r._field == "volumen_l")
  |> aggregateWindow(every: 5m, fn: mean, createEmpty: false)
  |> yield(name: "promedio_5m")