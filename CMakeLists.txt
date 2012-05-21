#Perchè non impariamo ad usare anche un po' di CMAKE????
cmake_minimum_required(VERSION 2.0)

#nome progetto
project(Cyclic-Executive)

<<<<<<< HEAD
message(WARNING "Project source path=${PROJECT_SOURCE_DIR}")

=======
#test della variabile
message(WARNING "** Project path=${PROJECT_SOURCE_DIR}")
>>>>>>> d973eb19870020494624d09d04b47cb8c45206c7

#prova opzioni
option(DUMMY_OPTION "un opzione inutile di prova per vedere come funziona! Abilita e scopri cosa fa!" ON)
option(BUILD_TEST "Compila un eseguibile di prova da task-example.c per testare la libreria" OFF)
set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src" CACHE PATH "percorso dei file sorgenti")

<<<<<<< HEAD
message(WARNING "Source path=${SOURCE_PATH}")
=======
message(WARNING "** Source path=${SOURCE_PATH}")
>>>>>>> d973eb19870020494624d09d04b47cb8c45206c7

#file di configurazione automatica riempito da cmake
configure_file("${SOURCE_PATH}/executive-config.h.in" "${SOURCE_PATH}/executive-config.h")

#ho scelto il nome della libreria in stile GNU: cyex (fico no??)
add_library(cyex ${SOURCE_PATH}/executive.c)

#librerie che sicuramente sarà necessario linkare
target_link_libraries(cyex rt pthread)

if(BUILD_TEST)
	add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(BUILD_TEST)
