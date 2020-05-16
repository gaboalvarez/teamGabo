#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/node.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

#include <shared/utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct {
    char* nombre;
    int x;
    int y;
} Pokemon;
typedef struct {
    char* nombre;
} PokemonFantasia;
typedef struct {
	int ID_entrenador;
	int posicion[2];
	t_list* mios;
	int estado;
	t_list* objetivos;
	t_list* objetivosActuales;
}Entrenador;
typedef enum {
	NUEVO = 1,
	LISTO,
	EJECUTANDO,
	BLOQUEADO,
	SALIR,
	PUNTOMUERTO
}Estado;

static Pokemon *crearPokemon(char *nombre,int x, int y) {
	Pokemon *new = malloc(sizeof(Pokemon));
    new->nombre = strdup(nombre);
    new->y=y;
    new->x=x;
    return new;
}
static PokemonFantasia *crearObjetivo(char *nombre) {
	PokemonFantasia *new = malloc(sizeof(PokemonFantasia));
    new->nombre = strdup(nombre);
    return new;
}

int cant_entrenadores(char** posiciones){
	int cantidad = 0;
	int i=0;
	while(posiciones[i] != NULL){
		cantidad++;
		i++;
	}
	return cantidad;
}

void asignarPosicion(Entrenador* persona,char** posicion){
	char* token = strtok((*posicion), "|");
	int eje=0;
	while (token != NULL) {
		(*persona).posicion[eje] = atoi(token);
	    token = strtok(NULL, "|");
	    eje++;
	}
}
void asignarPertenecientes(Entrenador* persona,char** pokemons){
	char* token = strtok((*pokemons), "|");
	(*persona).mios = list_create();
	while (token != NULL) {
		Pokemon* p;
		p = crearPokemon(token,0,0);
		list_add((*persona).mios,p);
		token = strtok(NULL, "|");
	}
}

char* retornarNombrePosta(Pokemon* p){
	return p->nombre;
}
char* retornarNombreFantasia(PokemonFantasia* p){
	return p->nombre;
}

void asignarObjetivos(Entrenador* persona,char** pokemons){
		char* token = strtok((*pokemons), "|");
		(*persona).objetivos = list_create();

		while (token != NULL) {
			PokemonFantasia* p;
			p = crearObjetivo(token);
			list_add((*persona).objetivos,p);
			token = strtok(NULL, "|");
		}
}

void asignarObjetivosActuales(Entrenador* persona){
	(*persona).objetivosActuales = list_create();
	t_list* nombresObjetivos = list_map((*persona).objetivos,(void*) retornarNombreFantasia);
	t_list* nombresPertenecientes = list_map((*persona).mios,(void*) retornarNombrePosta);
	int indiceObjetivos=0;
	int indicePertenecientes;
	int repetido;
	int noRepetido;
	while(indiceObjetivos<list_size(nombresObjetivos)){
		indicePertenecientes=0;
		repetido=1;
		noRepetido=0;
		while((indicePertenecientes<list_size(nombresPertenecientes))&&(repetido)){
			//printf("comparo objetivo nro %d: %s con perteneciente nro %d: %s \n",indiceObjetivos,list_get(nombresObjetivos,indiceObjetivos),indicePertenecientes,list_get(nombresPertenecientes,indicePertenecientes));
			if(!strcmp(list_get(nombresObjetivos,indiceObjetivos),list_get(nombresPertenecientes,indicePertenecientes))){
				repetido=0;
				list_remove(nombresObjetivos,indiceObjetivos);
				list_remove(nombresPertenecientes,indicePertenecientes);
				indiceObjetivos--;
				//printf("saco 2 elementos \n");
			}else{
				noRepetido++;
			}
			indicePertenecientes++;
		}
		//printf("tam objetivos: %d \n",list_size(nombresObjetivos));
		//printf("tam pertenecientes: %d \n",list_size(nombresPertenecientes));

		indiceObjetivos++;
	}
	if(list_size(nombresObjetivos)>0){
		for(int i=0;i<list_size(nombresObjetivos);i++){
			PokemonFantasia* p;
			p = crearObjetivo(list_get(nombresObjetivos,i));
			list_add((*persona).objetivosActuales,p);
		}
	}
	//printf("tam objetivos actuales: %d \n",list_size((*persona).objetivosActuales));
}

void inicializar_atributos(int indice, Entrenador* entrenador,char** posicion,char** pertenecientes,char** objetivos){
	(*entrenador).ID_entrenador = indice;
	asignarPosicion(&(*entrenador),&(*posicion));
	asignarPertenecientes(&(*entrenador),&(*pertenecientes));
	asignarObjetivos(&(*entrenador),&(*objetivos));
	asignarObjetivosActuales(&(*entrenador));
	if(list_size((*entrenador).mios) < list_size((*entrenador).objetivos)){
		(*entrenador).estado = NUEVO;
		}else{
			if(list_size((*entrenador).objetivosActuales)>0){
				(*entrenador).estado = PUNTOMUERTO;
			}else{
				(*entrenador).estado = SALIR;
			}
		}
	//printf("x: %d y: %d \n ",entrenador.posicion[0],entrenador.posicion[1]);
	//printf("xd: %d \n",list_size(entrenador.objetivos));
}

void ejecucionEntrenador(Entrenador* miEntrenador){
	printf("Hola! soy el entrenador %d, me faltan %d pokemons para completar mi objetivo\n",(*miEntrenador).ID_entrenador,list_size((*miEntrenador).objetivosActuales));
}

int main(void) {
	t_log* logger;
	t_config* config;
	logger = iniciar_logger("Team.log", "Team");
	config = leer_config("configTeam.config", logger);
	char** posiciones = config_get_array_value(config,"POSICIONES_ENTRENADORES"); // lista de strings, ultimo elemento nulo
	char** pertenecientes = config_get_array_value(config,"POKEMON_ENTRENADORES");
	char** objetivos = config_get_array_value(config,"OBJETIVOS_ENTRENADORES");

	int cantidad_entrenadores = cant_entrenadores(posiciones);
	//printf("cantidad entrenadores en el archivo: %d \n",cantidad_entrenadores);

	Entrenador entrenadores[cantidad_entrenadores];
	pthread_t hilos[cantidad_entrenadores];

	for(int a=0;a<cantidad_entrenadores;a++){
		inicializar_atributos(a,&(entrenadores[a]),&(posiciones[a]),&(pertenecientes[a]),&(objetivos[a]));
		//printf("x entrenador: %d\n",(entrenadores[a]).posicion[0]);
		//printf("1er pokemon: %s\n",retornarNombrePosta(list_get(entrenadores[a].mios,0)));
		//printf("1er objetivo: %s\n",retornarNombreFantasia(list_get(entrenadores[a].objetivos,0)));
		pthread_create(&(hilos[a]), NULL, (void*) ejecucionEntrenador, &(entrenadores[a]));
	}
	printf("entrenadores inicializados \n");
	printf("voy a esperar a que finalicen \n");
	pthread_detach(hilos[0]);
	pthread_detach(hilos[1]);
	pthread_detach(hilos[2]);
	printf(" \n programa finalizado");
}
