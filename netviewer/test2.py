import json
viewer_string1=open('anu_viewer-v2.json','r') 
json_data = json.loads(viewer_string1.read())

viewer_string2=open('network(9).json','r')
updated_data = json.loads(viewer_string2.read())



#Reference 1: id:209, Engn building lat = -35.276218, long = 149.119560
#Reference 2: id:10, Law building lat = -35.280695, long = 149.118733

lat1=-35.276218
lat2=-35.280695
lon1=149.119560
lon2=149.118733

i = len(json_data['node'])#file with new format
j = len(updated_data['nodes']) #file with new positions

for n in range(0,j):

    m_lat= (lat2-lat1)/(updated_data['nodes'][10]["y"]-updated_data['nodes'][209]["y"])
    lat = m_lat*(updated_data['nodes'][n]["y"]-updated_data['nodes'][209]["y"])+lat1

    json_data['node'][n]['lat']=lat #added in the json_data file (new format file)
    
for n in range(0,j):
    m_lon= (lon2-lon1)/(updated_data['nodes'][10]["x"]-updated_data['nodes'][209]["x"])
    lon = m_lon*(updated_data['nodes'][n]["x"]-updated_data['nodes'][209]["x"])+lon1
    
    json_data['node'][n]['lon']=lon


for n in range(0,i):
    json_data['node'][n].pop("x")
    json_data['node'][n].pop("y")

    with open('anu_viewer-v3.json','w') as outfile:
        json.dump(json_data, outfile)
