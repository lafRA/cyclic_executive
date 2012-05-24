#Perchè non impariamo ad usare anche un po' di CMAKE????
cmake_minimum_required(VERSION 2.0)

#nome progetto
project(Cyclic-Executive)

#test della variabile
message(WARNING "** Project path=${PROJECT_SOURCE_DIR}")

#prova opzioni
#option(DUMMY_OPTION "un opzione inutile di prova per vedere come funziona! Abilita e scopri cosa fa!" ON)
option(BUILD_TEST "compila i test d'esempio. Se questa opzione è off si aspetta di trovare già i file ok.o notok.o nella directory ${PROJECT_SOURCE_DIR}/test/obj" OFF)
set(SOURCE_PATH "${PROJECT_SOURCE_DIR}/src" CACHE PATH "percorso dei file sorgenti")

message(WARNING "** Source path=${SOURCE_PATH}")

#file di configurazione automatica riempito da cmake
#configure_file("${SOURCE_PATH}/executive-config.h.in" "${SOURCE_PATH}/executive-config.h")

#ho scelto il nome della libreria in stile GNU: cyex (fico no??)
#add_library(cyex ${SOURCE_PATH}/executive.c)

if(BUILD_TEST)
	add_subdirectory(${PROJECT_SOURCE_DIR}/test)
endif(BUILD_TEST)

link_directories(${PROJECT_SOURCE_DIR}/bin/test)

add_executable(clock_driven_ok ${SOURCE_PATH}/executive.c)
add_executable(clock_driven_not_ok ${SOURCE_PATH}/executive.c)

#librerie che sicuramente sarà necessario linkare
target_link_libraries(clock_driven_ok ok rt pthread)
target_link_libraries(clock_driven_not_ok notok rt pthread)

