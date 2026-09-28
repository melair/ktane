# Underground data

Generate the header:

```sh
go run . module underground fetch --output underground_raw.json
go run . module underground process --input underground_raw.json --output underground_data.h
```

Include the generated header in one C source file. IDs are array indexes and
must be regenerated together.

## Route display

```c
const UndergroundRoute *route = &underground_routes[route_id];
const char *line = underground_lines[route->line].name;
const char *destination = underground_stations[route->destination].name;
```

Display: `<line> line to <destination>`.

If `route->via != UNDERGROUND_NO_STATION` and the train has not passed its via
station, append ` via <station name>`:

```c
if (route->via != UNDERGROUND_NO_STATION &&
    current_stop_index <= route->via_stop_index) {
    const char *via = underground_stations[route->via].name;
}
```

`current_stop_index` is the zero-based position in the route's retained stop
list. The via wording disappears after `via_stop_index`.

`route->every_minutes` is the approximate service interval.

## Route stops

```c
for (uint8_t i = 0; i < route->stops_count; ++i) {
    UndergroundStationID station_id =
        underground_route_stops[route->stops_offset + i];
    const UndergroundStation *station = &underground_stations[station_id];

    const char *name = station->name;
    bool in_zone = station->in_zone;
}
```

Stops are in train visitation order. `stops_count` includes the first adjacent
out-of-zone stop at either end when present.

## Routes serving a station

```c
const UndergroundStation *station = &underground_stations[station_id];

for (uint8_t i = 0; i < station->routes_count; ++i) {
    UndergroundRouteID route_id =
        underground_station_routes[station->routes_offset + i];
    const UndergroundRoute *route = &underground_routes[route_id];
}
```
