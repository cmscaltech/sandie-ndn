import requests

class GetLocationDict(object):
    def get_lat_lon(self,ip_list=[]):
        print(ip_list)
        lats = []
        lons = []
        ip_locations = {}
        print("Processing {} IPs...".format(len(ip_list)))
        for ip in ip_list:
            r = requests.get("https://freegeoip.net/json/" + ip)
            json_response = r.json()
            print("{ip}, {region_name}, {country_name}, {latitude}, {longitude}".format(**json_response))
            if json_response['latitude'] and json_response['longitude']:
                ip_locations[ip] = {"latitude":json_response['latitude'], "longitude":json_response['longitude']}
        return ip_locations


