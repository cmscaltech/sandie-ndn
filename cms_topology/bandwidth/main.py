import get_bw

if __name__ == '__main__':
    site_dicts = {"T2_US_Caltech":"http://perfsonar.ultralight.org/toolkit/","T2_US_MIT":"http://perfsonar02.cmsaf.mit.edu/toolkit/","T2_US_Purdue":"http://perfsonar-cms2.itns.purdue.edu/toolkit/","T2_BR_Sprace":"http://perfsonar-bw.sprace.org.br/toolkit/","T2_US_Florida":"http://perfsonar2.ihepa.ufl.edu/toolkit/","T2_US_UCSD":"http://perfsonar-1.t2.ucsd.edu/toolkit/","T2_US_Vanderbilt":"http://perfsonar-bwctl.accre.vanderbilt.edu/toolkit/","T2_US_Wisconsin":"http://perfsonar02.hep.wisc.edu/toolkit/","T2_US_Nebraska":"http://hcc-ps02.unl.edu/toolkit/","T1_US_FNAL":"https://psonar3.fnal.gov/toolkit/"}

    labels_dict = {"18.12.1.172": ["MIT",1], "192.84.86.122":["CalTech",2], "128.211.143.4":["Purdue",3], "200.136.80.19":["Sprace",4], "128.227.221.45":["UFL",5], "169.228.130.41":["UCSD",6], "192.111.108.111":["Vanderbilt",7], "144.92.180.76":["WISC",8], "129.93.239.163":["Nebraska",9], "131.225.205.23":["FANL",10]}
    router_labels = {'64.57.30.225':'I2','190.103.185.145':'ampath','198.124.80.149':'esnet','149.165.255.193':'gigapop', '137.164.26.197':'cenic', '169.228.130.1':'ucsd', '143.215.193.3':'sox', '143.108.254.241':'ansp','164.113.255.253':'greatplains'}



    bw_history = get_bw.GetBwBetweenNodes()
    bw_urls = bw_history.generate_bw_urls(site_dicts)
    average_bw_commands = []
    for url in bw_urls:
        bw_command = bw_history.get_bw_commands(url['url'])
        if bw_command is not None:
            average_bw_commands.append({'source':url['source'], 'destination':url['destination'], 'bw_command':bw_command})


    print('source, destination, average bandwidth')
    for bw_url in average_bw_commands:
        avg_day_bw = bw_history.get_actual_bw_readings(bw_url['bw_command'])
        if avg_day_bw is not None:
            print('{0:s},{1:s},{2:12.3f}'.format(bw_url['source'], bw_url['destination'], avg_day_bw))
