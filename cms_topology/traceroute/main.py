import get_traceroute_results
import get_geolocation
import plot_map
import argparse



if __name__ == '__main__':

    p = argparse.ArgumentParser()
    p.add_argument("-o", "--output_pdf", required=True, help="write the generated topology to this pdf map")
    args = p.parse_args()
    filename = args.output_pdf


    site_dicts = {"T2_US_Caltech":"http://perfsonar.ultralight.org/toolkit/","T2_US_MIT":"http://perfsonar02.cmsaf.mit.edu/toolkit/","T2_US_Purdue":"http://perfsonar-cms2.itns.purdue.edu/toolkit/","T2_BR_Sprace":"http://perfsonar-bw.sprace.org.br/toolkit/","T2_US_Florida":"http://perfsonar2.ihepa.ufl.edu/toolkit/","T2_US_UCSD":"http://perfsonar-1.t2.ucsd.edu/toolkit/","T2_US_Vanderbilt":"http://perfsonar-bwctl.accre.vanderbilt.edu/toolkit/","T2_US_Wisconsin":"http://perfsonar02.hep.wisc.edu/toolkit/","T2_US_Nebraska":"http://hcc-ps02.unl.edu/toolkit/","T1_US_FNAL":"https://psonar3.fnal.gov/toolkit/"}

    labels_dict = {"18.12.1.172": ["MIT",1], "192.84.86.121":["Caltech",2], "128.211.143.4":["Purdue",3], "200.136.80.19":["Sprace",4], "128.227.221.45":["UFL",5], "169.228.130.41":["UCSD",6], "192.111.108.111":["Vanderbilt",7], "144.92.180.76":["WISC",8], "129.93.239.163":["Nebraska",9], "131.225.205.23":["Fermilab",10]}
    router_labels = {'64.57.30.225':'I2','190.103.185.145':'ampath','198.124.80.149':'esnet','149.165.255.193':'gigapop', '137.164.26.197':'cenic', '169.228.130.1':'ucsd', '143.215.193.3':'sox', '143.108.254.241':'ansp','164.113.255.253':'greatplains'}
 #   site_dicts = {"T2_US_Caltech":"http://192.84.86.122/toolkit/","T2_US_FNAL":"https://psonar3.fnal.gov/toolkit/"}
    trace = get_traceroute_results.TracePathsBetweenNodes()
    graph = trace.get_nodes_and_edges(site_dicts)
#    print(graph["nodes"], graph["edges"])
    with open(filename, 'w') as w:
        w.write("\nSite URLs:\n")
        w.write(str(site_dicts))
        w.write("\nSite IPs:\n")
        w.write(str(labels_dict))
        w.write("\nEdges:\n")

        for item in graph["edges"]:
            w.write(str(item[0]+','+item[1]+'\n'))


    locations = get_geolocation.GetLocationDict()
    geolocations = locations.get_lat_lon(graph["nodes"])
    print(geolocations)
    #override caltech #192.84.86.122
#    geolocations['144.92.180.76']={'latitude':47.03230,'longitude':-87.06634}
    geolocations['192.84.86.121']={'latitude':34.137835,'longitude':-120.126106}
    geolocations['169.228.130.41']={'latitude':31.5875,'longitude':-119.2819}
    geolocations['198.32.155.205']={'latitude':29.6516,'longitude':-82.3248}
    geolocations['198.32.252.229']={'latitude':29.6516,'longitude':-82.3248}
    geolocations['198.32.155.206']={'latitude':29.6516,'longitude':-82.3248}
    geolocations['198.32.252.237']={'latitude':29.6516,'longitude':-82.3248}

    #plot nodes and edges based on the geolocations
    plt = plot_map.CreateMap()
    plt.plot_map(graph["edges"], geolocations, labels_dict, router_labels, filename)




