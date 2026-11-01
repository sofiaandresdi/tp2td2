// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

// ------- strDuplicate ------

// cuenta cuantos caracteres tiene el string y pide ese largo + 1 con malloc para reservar memoria 
// (el +1 es para el '\0' del final). Despues copia caracter por caracter.
// devuelve una copia nueva, quien la use tiene que hacerle free.

char* strDuplicate(char* src){
int i=0;                          
int tamaño=0;                    
while (src[i]!= '\0') {           // recorro hasta el fin del string
tamaño += 1 ;                    // le sumo uno a tamaño por cada caracter recorrido 
i++;                             
}
char* duplicado= malloc (tamaño +1);   // reservo lugar para los caracteres + el '\0'
int j=0;                          // indice para copiar
while (src[j]!= '\0') {           // recorro todo el string de nuevo 
duplicado[j]=src[j];              // copio posicion por posicion el string al duplicado 
j ++;
}
duplicado[j]= '\0';               // escribo en la ultima posicion la terminacion del string 
return duplicado;                 // devuelvo la direccion del string nuevo 
}

// ------ strCompare ------
// Recorre los dos strings a la vez con un solo indice y frena en la primera
// letra distinta o cuando s1 se termina. Despues compara esas dos letras.
// Como '\0' vale 0, un string mas corto ("casa") es menor que el largo ("casas").
// No usa malloc. Devuelve 0 si son iguales, 1 si s1 es menor y -1 si es mayor.

int strCompare(char* s1, char* s2) {
int i=0;                                      
int res=-1;                                   // asumo que s1 es mayor
while (s1[i]==s2[i] && s1[i]!= '\0') {        // sigo mientras sean iguales y no termine s1
i++;
}
if (s1[i]== '\0' && s2[i]== '\0'){            // terminaron los dos juntos: son iguales
res=0;
}
if (s1[i]<s2[i]){                             // en la primera diferencia, s1 tiene el menor
res=1;
}

return res;
}

// ------ strConcatenate ------

// Cuenta el largo de los dos strings y pide tamaño1 + tamaño2 + 1 con malloc para reservar memoria.
// Copia src1 y despues src2, y cierra con un solo '\0'. Recien despues de copiar
// hace free de src1 y src2. 
// El string nuevo lo libera quien la usa.

char* strConcatenate(char* src1, char* src2)  {
int tamaño1 = 0;                 // largo de src1
int tamaño2 = 0;                 // largo de src2
int i=0;
int j =0;
while(src1[i]!= '\0') {           // cuento los caracteres de src1
tamaño1 ++;
i ++;
}
while(src2[j]!= '\0') {           // cuento los caracteres de src2
tamaño2 ++;
j++;
}
char* res = malloc(tamaño1 + tamaño2 + 1);   // lugar para los dos strings + un '\0'
int k=0;                          // indice de posicion para res 
while (src1[k]!= '\0') {          // copio src1 al principio de res
res[k]=src1[k];
k ++;
}
int m=0;                          // m = donde leo en src2 
while(src2[m]!= '\0') {           // copio src2 a continuacion de src1, por eso sigo utilizando k 
res[k]=src2[m];
k ++;
m ++;
}
free(src1);                       // libero los argumentos ya que ya se copiaron
free(src2);
res[k]= '\0';                     // cierro el string nuevo con su '\0'
return res;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========
void gameBoardDelete(GameBoard*);

// ------ gameBoardNew ------

// Pide memoria para el tablero y para las 20 filas, una por una. Cada fila nueva se
// pone al principio de la lista, asi la primera que se crea queda al final con next NULL.
// malloc no pone nada en cero, asi que todo se inicializa a mano (celdas en 0, punteros en NULL).
// Si un malloc falla, se libera lo que ya se pidio con gameBoardDelete y se devuelve NULL.

GameBoard* gameBoardNew() {
     GameBoard* tablero = (GameBoard*) malloc(sizeof(GameBoard)); //pido memoria para un tablero
    if (tablero == NULL) return NULL;         // si malloc falló, no hay nada que hacer

    tablero->first_row = NULL;                  // sin filas todavía
    tablero->queue_head = NULL;                 // cola de piezas vacía
    tablero->queue_tail = NULL;                 // cola de piezas vacía
    tablero->first_particle = NULL;             // sin partículas
    tablero->score = 0;                         // puntaje en 0
    tablero->tick_timer = 0;                    // timer en 0
    tablero->current_piece.col = 0;             // pieza en 0 (todavía no se usa)
    tablero->current_piece.row = 0;             // pieza en 0
    tablero->current_piece.type = 0;            // pieza en 0
    tablero->current_piece.rotation = 0;        // pieza en 0
    tablero->current_piece.active = 0;          // sin pieza activa

    for (int i = 0; i < GRID_ROWS; i++) {       // repito 20 veces, una por fila
        RowNode* filaNueva = (RowNode*) malloc(sizeof(RowNode)); // pido memoria para una fila
        if (filaNueva == NULL) {                // si falló el malloc de la fila...
            gameBoardDelete(tablero);           // ...libero lo que ya había creado
            return NULL;                        // y devuelvo NULL
        }
        for (int c = 0; c < GRID_COLS; c++) {   // recorro las 10 celdas de la fila
            filaNueva->cells[c] = 0;            // celda vacía
        }
        filaNueva->next = tablero->first_row;   // la fila nueva apunta a la que era primera
        tablero->first_row = filaNueva;         // la fila nueva pasa a ser la primera
    }
    return tablero;                             // devuelvo el tablero listo
}




// ------ gameBoardDelete ------

// Recorre cada lista y libera nodo por nodo.
// Antes de hacer free se guarda el next, porque despues del free el nodo ya no se puede leer.
// El tablero se libera al final, cuando ya no hace falta usarlo.
   
void gameBoardDelete(GameBoard* board) {
      if (board == NULL) return;                  // si me pasan NULL, no hay nada que liberar

    RowNode* fila = board->first_row;           // el "dedo" arranca en la primera fila
    while (fila != NULL) {                      // mientras el dedo apunte a una fila
        RowNode* filaSiguiente = fila->next;    // guardo la siguiente ANTES de borrar
        free(fila);                             // borro la fila actual
        fila = filaSiguiente;                   // muevo el dedo a la siguiente
    }

    PieceQueueNode* pieza = board->queue_head;               // dedo en el primer nodo de la cola
    while (pieza != NULL) {                                  // mientras haya nodos
        PieceQueueNode* piezaSiguiente = pieza->next;        // guardo el siguiente
        free(pieza);                                         // borro el actual
        pieza = piezaSiguiente;                              // avanzo
    }

    ParticleNode* particula = board->first_particle;         // dedo en la primera partícula
    while (particula != NULL) {                              // mientras haya partículas
        ParticleNode* particulaSiguiente = particula->next;  // guardo la siguiente
        free(particula);                                     // borro la actual
        particula = particulaSiguiente;                      // avanzo
    }

    free(board);                                // el tablero se borra ÚLTIMO
}


// ------ gameBoardAddParticle ------

// Pide memoria para una particula, le pone posicion, velocidad al azar, color y cuanto durara,
// y la agrega al principio de la lista (la nueva apunta a la que era primera).
// Si malloc falla, no hace nada: es solo un efecto visual y el juego sigue.

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    if (board == NULL) return;                  // si no hay tablero, no hago nada

    ParticleNode* particulaNueva = (ParticleNode*) malloc(sizeof(ParticleNode)); // pido memoria para una partícula nueva
    if (particulaNueva == NULL) return;         // si malloc falló, no hago nada

    particulaNueva->x = x;                      // dónde aparece (izquierda-derecha)
    particulaNueva->y = y;                      // dónde aparece (arriba-abajo)
    particulaNueva->vel_x = (rand() % 11) - 5;  // se mueve a los lados al azar (entre -5 y 5)
    particulaNueva->vel_y = -((rand() % 8) + 1);// sube al azar (entre -8 y -1, negativo es hacia arriba)
    particulaNueva->color = color;              // mismo color que la celda rota
    particulaNueva->life_timer = 30;            // cuánto dura (el juego la borra sola cuando llega a 0)

    particulaNueva->next = board->first_particle; // la nueva apunta a la que era primera
    board->first_particle = particulaNueva;     // la nueva pasa a ser la primera
}

// ------ gameBoardCheckAndClearLines ------

// Recorre las filas con dos punteros: fila (la que se revisa) y anterior (la ultima que
// se dejo). Si una fila esta completa, crea una particula por celda, la saca de la lista
// (anterior salta a la siguiente; si era la primera, se cambia first_row) y le hace free.
// Despues de recorrer, agrega una fila vacia al principio por cada una borrada,
// asi siempre quedan 20 filas. Devuelve cuantas se borraron.

int gameBoardCheckAndClearLines(GameBoard* board) {   
    if (board == NULL) return 0;                // si no hay tablero, borre 0 filas
    int borradas = 0;                           // cuántas filas borre
    int numeroFila = 0;                         // en qué número de fila estoy (0 es la de arriba)
    RowNode* anterior = NULL;                   // la última fila que dejé en la lista
    RowNode* fila = board->first_row;           // el "dedo" arranca en la primera fila

    while (fila != NULL) {                      // mientras el dedo apunte a una fila
        RowNode* filaSiguiente = fila->next;    // guardo la siguiente ANTES de borrar

        int completa = 1;                       // supongo que la fila está completa
        for (int c = 0; c < GRID_COLS; c++) {   // miro las 10 celdas
            if (fila->cells[c] == 0) completa = 0; // si encuentro una vacía, ya no está completa
        }

        if (completa) {                         // la fila está completa: hay que borrarla
            for (int c = 0; c < GRID_COLS; c++) {   // una partícula por cada celda
                gameBoardAddParticle(board, OFFSET_X + c * CELL_SIZE,       // dónde está la celda (izquierda-derecha)
                                     OFFSET_Y + numeroFila * CELL_SIZE,     // dónde está la celda (arriba-abajo)
                                     fila->cells[c]);                       // color de la celda
            }
            if (anterior == NULL) {             // si era la primera fila...
                board->first_row = filaSiguiente;   // ...first_row salta a la siguiente
            } else {                            // si no...
                anterior->next = filaSiguiente; // ...la anterior salta a la siguiente
            }
            free(fila);                         // borro la fila
            borradas++;                         // cuento una fila más borrada
        } else {                                // la fila no está completa: se queda
            anterior = fila;                    // esta es ahora la última que dejé
        }

        fila = filaSiguiente;                   // muevo el dedo a la siguiente
        numeroFila++;                           // bajo un número de fila
    }

    for (int i = 0; i < borradas; i++) {        // por cada fila que borré, pongo una vacía arriba
        RowNode* filaNueva = (RowNode*) malloc(sizeof(RowNode)); // pido memoria para una fila nueva
        if (filaNueva == NULL) break;           // si malloc falló, paro
        for (int c = 0; c < GRID_COLS; c++) {   // recorro las 10 celdas
            filaNueva->cells[c] = 0;            // celda vacía
        }
        filaNueva->next = board->first_row;     // la nueva apunta a la que era primera
        board->first_row = filaNueva;           // la nueva pasa a ser la primera
    }
    return borradas;                            // devuelvo cuántas filas borré
}

// ------ gameBoardEnqueuePiece ------

// Agrega una pieza al final de la cola. Se usa queue_tail para no recorrer toda la cola.
// Si la cola estaba vacia, la pieza nueva es a la vez head y tail.

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    if (board == NULL) return;                  // si no hay tablero, no hago nada

    PieceQueueNode* piezaNueva = (PieceQueueNode*) malloc(sizeof(PieceQueueNode)); // pido memoria para una pieza nueva
    if (piezaNueva == NULL) return;             // si malloc falló, no hago nada

    piezaNueva->type = type;                    // guardo el tipo de pieza
    piezaNueva->next = NULL;                    // va al final, así que no tiene siguiente

    if (board->queue_tail == NULL) {            // si no había nadie en la fila...
        board->queue_head = piezaNueva;         // ...la nueva es la primera
    } else {                                    // si ya había gente...
        board->queue_tail->next = piezaNueva;   // ...la última actual apunta a la nueva
    }
    board->queue_tail = piezaNueva;             // la nueva pasa a ser la última
}


// ------ gameBoardDequeuePiece ------

// Saca la pieza del principio de la cola (head) y guarda su tipo en out_type.
// Si era la ultima, queue_tail tambien pasa a NULL (si no, apuntaria a memoria liberada).
// Si la cola esta vacia, devuelve 0 y no toca out_type. Despues libera el nodo con free.

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    if (board == NULL || board->queue_head == NULL) return 0; // fila vacía: devuelvo 0 y no toco out_type

    PieceQueueNode* nodoSacado = board->queue_head;  // guardo con una flecha cuál es el nodo que voy a sacar.    
   *out_type = nodoSacado->type;                    // copio el número de ese nodo a la variable de afuera. 
    board->queue_head = nodoSacado->next;            // muevo el primero de la fila al siguiente nodo. 


    if (board->queue_head == NULL) {                 // si ya no queda nadie en la fila...
        board->queue_tail = NULL;                    // ...el último también queda en NULL
    }

    free(nodoSacado);                                // borro el nodo que saqué
    return 1;                                        // aviso que sí saqué una pieza
}




// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}



// ===================== TESTS =====================

void correrTests() {
    int fallos = 0;   // cuenta cuantos tests fallaron
    // Test de strDuplicate 
    char* copia = strDuplicate("");                 // string vacio
    if (!(copia != NULL && copia[0] == '\0')) {
        fallos++;
    }
    free(copia);                                    // la copia sale de malloc: hay que liberarla

    copia = strDuplicate("x");                      // un solo caracter
    if (!(copia != NULL && strCompare(copia, "x") == 0)) {
        fallos++;
    }
    free(copia);

    copia = strDuplicate("hola mundo");             // varios caracteres distintos
    if (!(copia != NULL && strCompare(copia, "hola mundo") == 0)) {
        fallos++;
    }
    free(copia);

    // Test de strCompare 
    // Recordar: 0 = iguales, 1 = el primero es menor, -1 = el primero es mayor.
    if (!(strCompare("", "") == 0)) {
        fallos++;
    }
    if (!(strCompare("a", "a") == 0)) {
        fallos++;
    }
    if (!(strCompare("casa", "casco") == 1)) {      // "casa" es menor que "casco"
        fallos++;
    }
    if (!(strCompare("casco", "casa") == -1)) {     // mismo caso, orden invertido
        fallos++;
    }
    if (!(strCompare("casa", "casas") == 1)) {      // un prefijo es menor
        fallos++;
    }

    // Test de strConcatenate 
    char* a = strDuplicate("");
    char* b = strDuplicate("abc");
    char* junto = strConcatenate(a, b);
    if (!(junto != NULL && strCompare(junto, "abc") == 0)) {
        fallos++;
    }
    free(junto);

    a = strDuplicate("hola ");
    b = strDuplicate("mundo");
    junto = strConcatenate(a, b);
    if (!(junto != NULL && strCompare(junto, "hola mundo") == 0)) {
        fallos++;
    }
    free(junto);

    // Test de gameBoardEnqueuePiece y gameBoardBequeuePiece
    GameBoard* tablero = gameBoardNew();
    if (tablero == NULL) {
        printf("Tests terminados. gameBoardNew devolvio NULL, no se pudo seguir.\n");
        return;
    }

    // Se agregan 5 piezas (tipos 1 a 5) y tienen que salir en el mismo orden.
    for (int i = 1; i <= 5; i++) {
        gameBoardEnqueuePiece(tablero, i);
    }
    int tipo = 0;
    int ordenBien = 1;                              // pasa a 0 si alguna sale mal
    for (int i = 1; i <= 5; i++) {
        int resultado = gameBoardDequeuePiece(tablero, &tipo);
        if (resultado != 1 || tipo != i) {
            ordenBien = 0;
        }
    }
    if (!(ordenBien == 1)) {
        fallos++;
    }

    // El sexto pop tiene que devolver 0 (ya no queda nadie)...
    if (!(gameBoardDequeuePiece(tablero, &tipo) == 0)) {
        fallos++;
    }
    // ...y la cola tiene que quedar vacia: head y tail en NULL.
    if (!(tablero->queue_head == NULL && tablero->queue_tail == NULL)) {
        fallos++;
    }

    // Despues de vaciarla, tiene que poder volver a usarse.
    gameBoardEnqueuePiece(tablero, 7);
    if (!(gameBoardDequeuePiece(tablero, &tipo) == 1 && tipo == 7)) {
        fallos++;
    }

    // Test de las lineas del tablero 

    // Test 1: se llena la fila de abajo (indice 19) y se borra.
    RowNode* ultima = getRowAt(tablero, 19);
    for (int c = 0; c < GRID_COLS; c++) {
        ultima->cells[c] = 3;                       // 3 = cualquier numero distinto de 0
    }
    RowNode* primeraAntes = tablero->first_row;     // para ver despues si cambio
    int borradas = gameBoardCheckAndClearLines(tablero);
    if (!(borradas == 1)) {
        fallos++;
    }
    if (!(tablero->first_row != primeraAntes)) {    // la primera fila ahora es una nueva
        fallos++;
    }

    int filaNuevaVacia = 1;                         // la fila nueva tiene que estar vacia
    for (int c = 0; c < GRID_COLS; c++) {
        if (tablero->first_row->cells[c] != 0) {
            filaNuevaVacia = 0;
        }
    }
    if (!(filaNuevaVacia == 1)) {
        fallos++;
    }

    int cantidad = 0;                               // la lista tiene que seguir teniendo 20 filas
    RowNode* fila = tablero->first_row;
    while (fila != NULL) {
        cantidad++;
        fila = fila->next;
    }
    if (!(cantidad == 20)) {
        fallos++;
    }

    // Test 2: se llenan las dos primeras filas (0 y 1) y se borran juntas.
    for (int c = 0; c < GRID_COLS; c++) {
        getRowAt(tablero, 0)->cells[c] = 5;
        getRowAt(tablero, 1)->cells[c] = 6;
    }
    borradas = gameBoardCheckAndClearLines(tablero);
    if (!(borradas == 2)) {
        fallos++;
    }

    cantidad = 0;
    fila = tablero->first_row;
    while (fila != NULL) {
        cantidad++;
        fila = fila->next;
    }
    if (!(cantidad == 20)) {
        fallos++;
    }

    // Se libera todo: filas, cola y particulas.
    gameBoardDelete(tablero);

    // resultado de los test
    printf("Tests terminados. Fallos: %d\n", fallos);
}

   
   


// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
