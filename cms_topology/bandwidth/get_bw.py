#!/usr/bin/python3

from urllib.request import urlopen
from urllib.error import URLError
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
import socket
import pygeoip
import random
import itertools
import json

class GetBwBetweenNodes(object):

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


    def get_actual_bw_readings(self, command):
        #print("Fetching bw history", command)
        bw_history_command = None
        try:
            bw_output = urlopen(command, timeout=10)
            ret_json = bw_output.read()
            content = json.loads(ret_json.decode('utf-8'))
            val_list = []
            for item in content:
                val = item['val']
                val_list.append(val)
            average_bw =  sum(val_list)/float(len(val_list)) if len(val_list) > 0 else None

        except socket.timeout as err:
            print('socket timed out - URL {0:s}'.format(command))
            pass
        except URLError:
            print('Could not open URL {0:s}'.format(command))
            pass
        return average_bw




    def get_bw_commands(self, command):
        #print("Fetching history", command)
        bw_command = None
        try:
            bw_output = urlopen(command, timeout=10)
            ret_json = bw_output.read()
            content = json.loads(ret_json.decode('utf-8'))
            try:
                meta_datakey = content[0]['metadata-key']
                bw_command = command.split('?')[0] +  meta_datakey + '/throughput/averages/86400?format=json'
            except IndexError:
                print('Empty JSOn from URL {0:s}'.format(command))
                pass

        except socket.timeout as err:
            print('socket timed out - URL {0:s}'.format(command))
            pass
        except URLError:
            print('Could not open URL {0:s}'.format(command))
            pass
        return bw_command


    def generate_bw_urls(self, site_dict):
        #"http://perfsonar02.cmsaf.mit.edu/esmond/perfsonar/archive/?event-type=throughput&source=perfsonar02.cmsaf.mit.edu&destination=perfsonar.ultralight.org"
        sites = [x.split('/')[2] for x in site_dict.values()]
        site_list = list(itertools.permutations(sites, 2))
        url_list = []
        print(list(site_list))
        for x in site_list:
            if x[0] != x[1]:
                if 'fnal' in x[0]:
                    url = "https://"+x[0] + "/esmond/perfsonar/archive/?event-type=throughput&format=json&source=" + x[0] + "&destination="+x[1]
                else:
                    url = "http://"+x[0] + "/esmond/perfsonar/archive/?event-type=throughput&format=json&source=" + x[0] + "&destination="+x[1]

                print(url)
                url_list.append({'source':x[0], 'destination':x[1], 'url':url})
        return url_list


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



