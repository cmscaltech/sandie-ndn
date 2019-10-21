MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkconsumer-cmd -w 0000:8f:00.0 -- -initcfg @init-config-client.yaml -tasks @xrdndndpdkconsumer.yaml

MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkproducer-cmd -w 0000:5e:00.0 -- -initcfg @init-config.yaml -tasks @xrdndndpdkproducer.yaml

MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkconsumer-cmd --vdev net_af_packetL,iface=enp143s0  -- -initcfg @init-config.yaml -tasks @xrdndndpdkconsumer-raw-socket.yaml

MGMT=tcp://127.0.0.1:6345 ./xrdndndpdkproducer-cmd --vdev net_af_packetL,iface=ens2 -- -initcfg @init-config.yaml -tasks @xrdndndpdkproducer-raw-socket.yaml

with and without raw socket

file tree

logging:

LOG_Xrdndnconsumer=D

* **V**: VERBOSE level (C only)
* **D**: DEBUG level
* **I**: INFO level (default)
* **W**: WARNING level
* **E**: ERROR level
* **F**: FATAL level
* **N**: disabled (in C), PANIC level (in Go)