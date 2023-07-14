idf_component_register(SRCS "main.c" "components/wifi.c" "components/http.c" "components/web_server.c" "components/tasks.c" "components/mqtt.c" "components/stma.c" "components/hum_sensor.c" "components/outputs.c" "components/temp_sensor.c" "components/dht11/dht11.c"
                    INCLUDE_DIRS "." "components/include" "components/dht11/include")
