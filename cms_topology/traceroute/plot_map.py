import networkx as nx
import matplotlib
matplotlib.rcParams['ps.useafm'] = True
matplotlib.rcParams['pdf.use14corefonts'] = True
matplotlib.rcParams['text.usetex'] = True

import matplotlib.pyplot as plt
from mpl_toolkits.basemap import Basemap as Basemap
import socket

class CreateMap(object):
    def __init__(self):
        self.G = nx.Graph()
        self.pos = {}

        self.m = Basemap(
                projection='merc',
                llcrnrlon=-130,
                llcrnrlat=-35,
                urcrnrlon=-40,
                urcrnrlat=50,
                lat_ts=0,
                resolution='i',
                suppress_ticks=True)


    def plot_map(self, edge_list, node_locations, labels_dict, router_labels, filename):
        print("plotting_map")
        #for pair of nodes in an edge
        #add edge
        #set_position
        for edge in edge_list:
            nodeA = edge[0]
            nodeB = edge[1]
            #print(nodeA, nodeB)
            try:
                self.pos[nodeA] = self.m(float(node_locations[nodeA]['longitude']), float(node_locations[nodeA]['latitude']))
                self.pos[nodeB] = self.m(float(node_locations[nodeB]['longitude']), float(node_locations[nodeB]['latitude']))
                self.G.add_edge(nodeA, nodeB)
            except KeyError:
                print("Passing, no location found for {0:s} or {1:s}".format(nodeA, nodeB))
                pass

        legend_text = ""
        labels = {}
        labels_pos = {}
        node_list = []
        for node in self.G.nodes():
            if node in labels_dict:
                #print(self.pos[node])
                labels[node] = labels_dict[node][1]
                node_list.append(node)
            elif node in router_labels:
                labels[node] = router_labels[node]
            else:
                labels[node] = ""
#                labels[node] = node
#                rev_dns = socket.gethostbyaddr(node)
#                print("Node {} reverse DNS {}".format(node, rev_dns))
            labels_pos[node]=self.pos[node]

        legend_list = [str(val[1])+" - "+val[0] for (key, val) in sorted(labels_dict.items(), key=lambda e:e[1][1])]
        legend_text = "\n".join(legend_list)
# Now draw the map
        plt.figure(figsize=(12,6))
        self.m.drawcountries(linewidth=0.25)
        self.m.drawstates(linewidth=0.1)
        #m.bluemarble()
        self.m.shadedrelief()
        nx.draw_networkx(self.G,self.pos,labels=labels,node_size=20,node_color='g',font_size=4,width=0.3,font_color='w',font_weight='bold')
        nx.draw_networkx_nodes(labels.keys(),labels_pos, nodelist=node_list, node_size=40, node_color='r',font_size=6,width=0.3)
#        text = nx.draw_networkx_labels(self.G,labels_pos,labels=labels,font_size=12)
#       nx.draw_networkx(self.G,self.pos,node_size=20,node_color='red')
        #m.drawlsmask()
        #m.etopo()
        ax = plt.gca()
        ax.text(800000, 900000 , legend_text, style='normal',
        bbox={'facecolor':'red', 'alpha':0.5, 'pad':10})
#        plt.show()

        plt.savefig(filename,dpi=300, bbox_inches='tight')
