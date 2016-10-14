

#json1_file = open('anu_viewer-v2.json')
#json1_str = json1_file.read()
#json1_data = json.loads.(json1_str)[0]
#from pprint import pprint
#with open('anu_viewer-v2.json') as data_file:


import json
import ast

viewer_string=open('anu_viewer-v2.json','r') 

data_str = viewer_string.read()

json_acceptable_string = data_str.replace("'","\"")

data = json.loads(json_acceptable_string) #data gives type 'dict'

data_nodes_list = data["node"]

#Reference 1: id:209, Engn building lat = -35.27620, long = 149.11956
#Reference 2: id:10, Law building lat = -35.28072, long = 149.11871

i = len(data_nodes_list)
lat1=-35.27620
lat2=-35.28072

lat_list= []
for n in range(0,i-1):
    
    m = (lat2-lat1)/(data_nodes_list[10]["x"]-data_nodes_list[209]["x"])
    lat_new = m*(data_nodes_list[n]["x"]-data_nodes_list[209]["x"])+lat1
    lat_list.append(lat_new)
    data_nodes_list[n]["x"] = lat_list[n]

    json.dump(data_nodes_list[n],"anu_viewer-v3.json")
#check if the format of the json, matches the one in the example
#goal, to be able to access x and y values and put them into a list.
