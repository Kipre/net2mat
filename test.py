import re
import numpy as np
from scipy.io import loadmat, savemat
from lxml import etree
import subprocess
import os
import sys


def net2mat(xml, output):
    
    with open(xml)as f:
        raw = f.read()

    pattern = re.compile(r'<\?xml.*\?>')  # we need to get rid of the xml declaration
    xml = pattern.sub('', raw)
    tree = etree.fromstring(xml)

    connection_ids = {}
    roughness = []
    diameter = []
    length = []
    from_ids = []
    to_ids = []

    for i, connection in enumerate(tree.find("{http://gaslib.zib.de/Framework}connections").getchildren()):
        connection_ids[connection.get('id')] = i
        from_ids.append(connection.get('from'))
        to_ids.append(connection.get('to'))
        if connection.tag == '{http://gaslib.zib.de/Gas}pipe':
            roughness.append(connection.find('{http://gaslib.zib.de/Gas}roughness').get('value'))
            diameter.append(connection.find('{http://gaslib.zib.de/Gas}diameter').get('value'))
            length.append(connection.find('{http://gaslib.zib.de/Gas}length').get('value'))
        else:
            roughness.append(0)
            diameter.append(0)
            length.append(0)

    node_ids = {}
    pressure_min = []
    pressure_max = []
    height = []

    for i, node in enumerate(tree.find("{http://gaslib.zib.de/Framework}nodes").getchildren()):
        node_ids[node.get('id')] = i
        pressure_min.append(node.find('{http://gaslib.zib.de/Gas}pressureMin').get('value'))
        pressure_max.append(node.find('{http://gaslib.zib.de/Gas}pressureMax').get('value'))
        height.append(node.find('{http://gaslib.zib.de/Gas}height').get('value'))

    # Making the incidence matrix
    incidence_matrix = np.zeros((len(node_ids.keys()), len(connection_ids.keys())), dtype=np.int8)
    for connection_id, connection_num in connection_ids.items():
        from_num = node_ids[from_ids[connection_num]]
        to_num = node_ids[to_ids[connection_num]]
        incidence_matrix[from_num, connection_num] -= 1
        incidence_matrix[to_num, connection_num] += 1

    # Extracting the temperatures
    sources = tree.findall('.//{http://gaslib.zib.de/Gas}source')
    temperature = np.zeros(len(sources))

    for i, source in enumerate(sources):
        temperature[i] = source.find('{http://gaslib.zib.de/Gas}gasTemperature').get('value')

    # Saving the orders
    nodes_order = ['' for k in range(len(node_ids.keys()))] 
    for node in node_ids.keys():
        nodes_order[node_ids[node]] = node
    connections_order = ['' for k in range(len(connection_ids.keys()))]
    for connection in connection_ids.keys():
        connections_order[connection_ids[connection]] = connection 

    roughness = np.array(roughness, dtype=np.float64)
    diameter = np.array(diameter, dtype=np.float64)
    temperature = np.array(temperature, dtype=np.float64)
    pressure_min = np.array(pressure_min, dtype=np.float64)
    pressure_max = np.array(pressure_max, dtype=np.float64)
    length = np.array(length, dtype=np.float64)
    height = np.array(height, dtype=np.float64)

    # Saving the network
    savemat(output, 
            {'incidence_matrix' : incidence_matrix,
            'roughness' : roughness,
            'diameter' : diameter,
            'temperature' : temperature,
            'pressure_min' : pressure_min,
            'pressure_max' : pressure_max,
            'length' : length,
            'height' : height,
            'nodes_order' : nodes_order,
            'connections_order' : connections_order})

def reorder(py_order, cli_order):
    py_order = [a.strip() for a in list(py_order)]
    cli_order = list(cli_order)
    return [py_order.index(a) for a in cli_order]

# Check what os we are on
if sys.platform.startswith("win"):
    command = 'net2mat'
elif sys.platform.startswith("linux"):
    command = './net2mat'
else:
    raise ImportError("This tst module doesn't support this system")


# Checking that there is no output without arguments
assert subprocess.run([command], capture_output=True).returncode == 1

# Checking with all the networks
test_directory = 'test_data/'
subjects = {'connections_order':['roughness', 'diameter', 'length'],
            'nodes_order':['pressure_min', 'pressure_max', 'height']}

net_files = [os.fsdecode(file) for file in os.listdir(test_directory)]
for file in net_files:
    print(f'Doing {file} in python')
    net2mat(test_directory + file, 'py.mat')

    print(f'Doing {file} in net2mat')
    assert subprocess.run([command, test_directory + file, "cli.mat"], capture_output=True).returncode == 0

    py_mat = loadmat("py.mat")
    cli_mat = loadmat("cli.mat")
    # print(py_mat.keys())
    # print(cli_mat.keys())
    reorderings = {}
    for subject, keys in subjects.items():
        reorderings[subject] = reorder(py_mat[subject], cli_mat[subject])
        for key in keys:
            print(key)
            reordering = reorder(py_mat[subject], cli_mat[subject])
            if not np.allclose(cli_mat[key], py_mat[key][0, reorderings[subject]]):
                print(cli_mat[key] - py_mat[key][0, reorderings[subject]])
                print(cli_mat[key])
                print(cli_mat[subject])
                raise Exception(f"Difference found in {key}.")

    if not np.allclose(cli_mat['incidence_matrix'], py_mat['incidence_matrix'][reorderings['nodes_order'], :][:, reorderings['connections_order']]):
                print(cli_mat['incidence_matrix'] - py_mat['incidence_matrix'][reorderings['nodes_order'], :][:, reorderings['connections_order']])
                print(py_mat['incidence_matrix'])
                print(py_mat['incidence_matrix'][reorderings['nodes_order'], :][:, reorderings['connections_order']])
                print(cli_mat['incidence_matrix'])
                print(cli_mat['nodes_order'])
                print(cli_mat['connections_order'])
                raise Exception("Difference found in incidence matrix.")

print('OK')


