export CC=${CC:-gcc}
export CGO_CFLAGS_ALLOW='.*'

DPDKFLAGS=$(pkg-config --cflags libdpdk | sed 's/-include [^ ]*//')
DPDKLDFLAGS=$(pkg-config --libs libdpdk)

CPPFLAGS="-Wall -Werror -m64 -pthread -O3 -march=native -g -I${GOPATH}/src/github.com/usnistgov"
CXXFLAGS="-std=c++17 "$CPPFLAGS" -I${HADOOP_HOME}/include"
CFLAGS=$CPPFLAGS

LDFLAGSLOCAL='-L'${PWD}'/libs'
LDFLAGS=$LDFLAGSLOCAL" -L${GOPATH}/src/github.com/usnistgov/ndn-dpdk/build -lndn-dpdk-c -lurcu-qsbr -lurcu-cds -lubpf -lspdk -lspdk_env_dpdk -lrte_bus_pci -lrte_bus_vdev -lrte_pmd_ring -lnuma -lm -lcommon"
LIB_FILESYSTEM=$LDFLAGSLOCAL' -lfilesystem -lstdc++ -lhdfs -ljvm'

if [[ -n $RELEASE ]]; then
  CFLAGS=$CFLAGS' -DNDEBUG -DZF_LOG_DEF_LEVEL=ZF_LOG_INFO'
  CXXFLAGS=$CXXFLAGS' -DNDEBUG -DZF_LOG_DEF_LEVEL=ZF_LOG_INFO'
fi
