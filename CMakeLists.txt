#Perchè non impariamo ad usare anche un po' di CMAKE????
cmake_minimum_required(VERSION 2.0)

#nome progetto
project(Cyclic-Executive)

#prova opzioni
option(DUMMY_OPTION "un opzione inutile di prova per vedere come funziona! Abilita e scopri cosa fa!" ON)
option(BUILD_TEST "Compila un eseguibile di prova da task-example.c per testare la libreria" OFF)
set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src" CACHE PATH "percorso dei file sorgenti")

#file di configurazione automatica riempito da cmake
configure_file("${SOURCE_PATH}/executive-config.h.in" "${SOURCE_PATH}/executive-config.h")

#ho scelto il nome della libreria in stile GNU: cyex (fico no??)
add_library(cyex ${SOURCE_PATH}/executive.c)

#librerie che sicuramente sarà necessario linkare
target_link_libraries(cyex rt pthread)

if(BUILD_TEST)
	add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(BUILD_TEST)

