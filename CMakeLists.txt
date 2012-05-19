#Perchè non impariamo ad usare anche un po' di CMAKE????
cmake_minimum_required(VERSION 2.0)

#nome progetto
project(Cyclic-Executive)

#ho scelto il nome della libreria in stile GNU: cyex (fico no??)
add_library(cyex executive.c)

#librerie che sicuramente sarà necessario creare
target_link_libraries(cyex rt pthread)
