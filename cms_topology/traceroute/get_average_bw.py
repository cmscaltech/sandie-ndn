#!/usr/bin/python3

from urllib.request import urlopen
from bs4 import BeautifulSoup
import re
import ssl
import traceback
import sys
from multiprocessing import Pool
from multiprocessing import Event
from multiprocessing import Manager
import matplotlib.pyplot as plt
import networkx as nx
from networkx.drawing.nx_agraph import graphviz_layout
from socket import timeout
import pygeoip
import random

class TracePathsBetweenNodes(object):

    def get_path_segments(self, data):
        ip_regex = re.compile(r"\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}")
        soup = BeautifulSoup(data,"lxml")
        tag=soup.pre
        source = soup.title
        source_ip = [ip_regex.search(x).group() for x in source]
        traceroute_output = tag.string.split("\\n")
        sanitized_output = [ip_regex.search(x).group() for x in traceroute_output if "ms" in x and ip_regex.search(x)]
        nodes = source_ip + sanitized_output
        edges = [(x,y) for x,y in zip(sanitized_output, sanitized_output[1:])]
        return {"nodes":nodes,"edges":edges}


    def do_http_req(self, command):
        print("Running treaceroute command", command)
        try:
            traceroute_output = urlopen(command, timeout=60)
            output_string = traceroute_output.read()
            return output_string
        except:
            print('socket timed out - URL %s', command)
            self.exit = Event()
            self.exit.set()



    def run_traceroute(self,command):
        path_segments = []
        sites = []
        output_string = self.do_http_req(command)
        if output_string is not None:
            ret = self.get_path_segments(str(output_string))
            path_segments += ret["edges"]
            sites += ret["nodes"]
            return {"edges":path_segments,"nodes":sites}
        else:
            return None

    def create_traceroute_commands(self, site_dict):
        site_list = [x for x in site_dict.values()]
        host_names_list = [x.split('/')[2] for x in site_dict.values()]
        traces = []
        for item in site_list:
            traces += [item+"gui/reverse_traceroute.cgi?target="+x+"&function=traceroute" for x in host_names_list if x not in item]
        return traces



    def get_nodes_and_edges(self, site_dict):
        commands = self.create_traceroute_commands(site_dict)
        nodes = []
        edges = []
        with Pool(processes=25) as p:
            result = p.map(self.run_traceroute, commands)

        #beautify this
        for item in result:
            #print(result)
            if item is not None:
                nodes.extend(item["nodes"])
                edges.extend(item["edges"])
        n = list(set(nodes))
        e = list(set(edges))
        return {"nodes":n, "edges":e}
