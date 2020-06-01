export CC=${CC:-gcc}
export CGO_CFLAGS_ALLOW='.*'

CFLAGS='-Wall -Werror -Wno-error=deprecated-declarations -m64 -pthread -O3 -g -I../../../ '$(pkg-config --cflags libdpdk | sed 's/-include [^ ]*//')
LIBS='-lurcu-qsbr -lurcu-cds -lubpf -lspdk -lspdk_env_dpdk -lrte_bus_pci -lrte_bus_vdev -lrte_pmd_ring '$(pkg-config --libs libdpdk)' -lnuma -lm'

NDNDPDKLIBS_PATH=${PWD}'/../../ndn-dpdk/build'
NDNDPDKLIBS='-lndn-dpdk-iface -lndn-dpdk-ndn -lndn-dpdk-dpdk -lndn-dpdk-core'
LOCALLIBS_PATH=${PWD}'/libs'
LIB_COMMON='-lcommon'
LIB_FILESYSTEM='-lfilesystem'

if [[ -n $RELEASE ]]; then
  CFLAGS=$CFLAGS' -DNDEBUG -DZF_LOG_DEF_LEVEL=ZF_LOG_INFO'
fi
