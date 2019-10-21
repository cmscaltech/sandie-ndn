package xrdndndpdkconsumer

/*
#cgo CFLAGS:-Werror -Wno-error=deprecated-declarations -m64 -pthread -O0 -g -march=native -I/usr/local/include/dpdk -I/usr/include/dpdk -DNDEBUG -DZF_LOG_DEF_LEVEL=ZF_LOG_INFO
#cgo LDFLAGS:-L${SRCDIR}/../../../ndn-dpdk/build/ -L${SRCDIR}/../libs/ -lxrdndndpdk-common -lndn-dpdk-iface -lndn-dpdk-ndn -lndn-dpdk-dpdk -lndn-dpdk-core -L/usr/local/libv -lurcu-qsbr -lurcu-cds -lubpf -lspdk -lspdk_env_dpdk -ldpdk -lnuma -lm
*/
import "C"
