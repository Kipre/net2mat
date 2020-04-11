#include <iostream>
#include <iterator>
#include <map>
#include <filesystem>
#include <vector>
#include "pugixml.hpp"
#include "matio.h"

namespace fs = std::filesystem;

/* Utility functions */
void write_and_free(matvar_t *matvar, mat_t *matfp)
{
    if ( NULL == matvar ) {
        std::cerr << "Error creating variable ’"<< matvar->name << "’\n";
    } else {
        Mat_VarWrite(matfp, matvar, MAT_COMPRESSION_NONE);
        Mat_VarFree(matvar);
    }
}

/* Main function */
int main(int argc, char **argv)
{   
    mat_t *matfp;
    matvar_t *matvar;
    unsigned int nb_nodes, nb_connections, nb_sources, max_node_string = 0, max_connection_string = 0;
    int to, from, *incidence_matrix;
    double roughness, diameter, length, height, pressure_min, pressure_max;
    double *roughnesses, *diameters, *lengths, *heights, *pressure_mins, *pressure_maxes, *temperatures;
    char *nodes_order, *connections_order;
    std::vector<double> temperatures_vect;
    std::string output_path, input_path;
    std::string from_id, to_id, node_id, connection_id;
    std::map<std::string, std::tuple<double, double, double>> nodes_data;
    std::map<std::string, std::tuple<double, double, double, int, int>> connections_data;

    /* Reading the arguments */
    if (argc == 3 || argc == 2) {
        input_path = argv[1];
        fs::path first_path = fs::path(argv[1]);
        if (argc == 3) {
            fs::path second_path = fs::path(argv[2]);
            if (second_path.extension() == fs::path("/foo/bar.mat").extension()) {
                output_path = second_path.string();
            } else if (!second_path.has_filename()) {
                second_path += first_path.filename().replace_extension(".mat");
                output_path = second_path.string();
            } else if (!second_path.has_extension()) {
                second_path /= first_path.filename().replace_extension(".mat");
                output_path = second_path.string();
            } else {
                std::cerr << "Output path was not understood." << std::endl;
            }
        } else if (argc == 2) {
            output_path = first_path.replace_extension(".mat").string();
        }
    } else {
        std::cerr << "Wrong number of arguments, the call syntax is one of the following:" << std::endl
                  << "    `net2mat /path/to/load/network_name.net`" << std::endl
                  << "    `net2mat /path/to/load/network_name.net /path/to/output`" << std::endl
                  << "    `net2mat /path/to/load/network_name.net /path/to/output/output_name.mat`" << std::endl
                  << "In the first case the file gets outputed into the same directory with the " << std::endl
                  << "same name but .mat extension. In the second case it is outputed to the target" << std::endl
                  << "directory and with the same filename. In the third case the file is outputed to " << std::endl
                  << "the specified path." << std::endl;
        return EXIT_FAILURE;
    }
    
    /* Opening the XML */
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(input_path.c_str());
    if (!result) {
        std::cerr << "Error opening " << input_path << " file"<< std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Opened "<< input_path << " with success"<< std::endl;;

    /* Fetching node characteristics */ 
    for (pugi::xml_node node: doc.child("network").child("framework:nodes"))
    {
        pressure_min = node.child("pressureMin").attribute("value").as_double();
        pressure_max = node.child("pressureMax").attribute("value").as_double();
        height = node.child("height").attribute("value").as_double();
        node_id = node.attribute("id").value();
        nodes_data[node_id] = std::make_tuple(pressure_min, pressure_max, height);
        if (node_id.length() > max_node_string) {
            max_node_string = node_id.length();
        }
        std::string tmp(node.name());
        if (tmp == "source") {
            temperatures_vect.push_back(node.child("gasTemperature").attribute("value").as_double());
        }
    }
    nb_nodes = nodes_data.size();
    nb_sources = temperatures_vect.size();
    std::cout << "Fetched data for nodes, found " << nb_nodes << " nodes" << std::endl;

    /* Fetching connection characteristics */ 
    for (pugi::xml_node connection: doc.child("network").child("framework:connections"))
    {   
        std::string tmp(connection.name());
        if (tmp == "pipe") {
            roughness = connection.child("roughness").attribute("value").as_double();
            diameter = connection.child("diameter").attribute("value").as_double();
            length = connection.child("length").attribute("value").as_double();
        } else {
            roughness = 0;
            diameter = 0;
            length = 0;
        }
        from_id = connection.attribute("from").value();
        to_id = connection.attribute("to").value();
        from = std::distance(nodes_data.begin(), nodes_data.find(from_id));
        to = std::distance(nodes_data.begin(), nodes_data.find(to_id));
        connection_id = connection.attribute("id").value();
        connections_data[connection_id] = std::make_tuple(roughness, diameter, length, from, to);
        if (connection_id.length() > max_connection_string) {
            max_connection_string = connection_id.length();
        }
    }
    nb_connections = connections_data.size();
    std::cout << "Fetched data for connections, found " << nb_connections << " connections" << std::endl;

    /* Making the incidence matrix and characteristics related to connections*/
    incidence_matrix = new int [nb_nodes * nb_connections]{0};
    roughnesses = new double [nb_connections];
    diameters = new double [nb_connections];
    lengths = new double [nb_connections];
    connections_order = new char [nb_connections*max_connection_string];
    int counter = 0;
    for ( auto const& [id, val] : connections_data )
    {
        std::tie(roughness, diameter, length, from, to) = val;
        incidence_matrix[counter*nb_nodes + from]--;
        incidence_matrix[counter*nb_nodes + to]++;
        roughnesses[counter] = roughness;
        diameters[counter] = diameter;
        lengths[counter] = length;
        int current_string_length = id.length();
        for (int k=0; k < max_connection_string; k++) {
            if (k < current_string_length) {
                connections_order[k*nb_connections + counter] = id.at(k);
            } else {
                connections_order[k*nb_connections + counter] = '\0';
            }
        }
        counter++;
    }
    std::cout << "Made incidence matrix and transformed connections data" << std::endl;

    /* Characteristics related to the nodes */
    pressure_mins = new double [nb_nodes];
    pressure_maxes = new double [nb_nodes];
    heights = new double [nb_nodes];
    nodes_order = new char [nb_nodes*max_node_string];
    counter = 0;
    for( auto const& [id, val] : nodes_data )
    {
        std::tie(pressure_min, pressure_max, height) = val;
        pressure_mins[counter] = pressure_min;
        pressure_maxes[counter] = pressure_max;
        heights[counter] = height;
        int current_string_length = id.length();
        for (int k=0; k < max_node_string; k++) {
            if (k < current_string_length) {
                nodes_order[k*nb_nodes + counter] = id.at(k);
            } else {
                nodes_order[k*nb_nodes + counter] = '\0';
            }
        }
        counter++;
    }

    temperatures = new double [nb_sources];
    temperatures = &temperatures_vect[0];

    std::cout << "Transformed nodes data" << std::endl;

    /* Creating matrices and loading them into the file */
    size_t incidence_dims[2] = {nb_nodes, nb_connections};
    size_t connections_dims[2] = {1, nb_connections};
    size_t nodes_dims[2] = {1, nb_nodes};
    size_t connections_order_dims[2] = {nb_connections, max_connection_string};
    size_t nodes_order_dims[2] = {nb_nodes, max_node_string};
    size_t temperature_dims[2] = {1, nb_sources};
    
    matfp = Mat_CreateVer(output_path.c_str(), NULL, MAT_FT_DEFAULT);
    if ( NULL == matfp ) {
        std::cerr << "Error creating MAT file " << output_path 
                  << ". Check that the output path was correct." 
                  << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Created the " << output_path << " file" << std::endl;

    matvar = Mat_VarCreate("incidence_matrix", MAT_C_INT32, MAT_T_INT32, 2, incidence_dims, incidence_matrix, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("roughness", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, connections_dims, roughnesses, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("diameter", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, connections_dims, diameters, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("length", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, connections_dims, lengths, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("pressure_min", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, nodes_dims, pressure_mins, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("pressure_max", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, nodes_dims, pressure_maxes, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("height", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, nodes_dims, heights, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("temperature", MAT_C_DOUBLE, MAT_T_DOUBLE, 2, temperature_dims, temperatures, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("connections_order", MAT_C_CHAR, MAT_T_UINT8, 2, connections_order_dims, connections_order, 0);
    write_and_free(matvar, matfp);

    matvar = Mat_VarCreate("nodes_order", MAT_C_CHAR, MAT_T_UINT8, 2, nodes_order_dims, nodes_order, 0);
    write_and_free(matvar, matfp);

    std::cout << "Wrote everithing" << std::endl;

    Mat_Close(matfp);
    std::cout << "Done" << std::endl;
}

    