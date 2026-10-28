#ifndef DATOS_SIMULACION_H
#define DATOS_SIMULACION_H

#define TP_CANT_POSTEOS 50000
#define TP_TAM_TEXTO 141
#define TP_TAM_USUARIO 15
#define TP_TAM_FEED 20

typedef struct {
    int id;
    char texto[TP_TAM_TEXTO];
} tp_posteo_t;

typedef struct {
    char usuario[TP_TAM_USUARIO];
    int feed[TP_TAM_FEED];
} tp_usuario_t;

#endif
