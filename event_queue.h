#ifndef __EVENT_QUEUE_H__
#define __EVENT_QUEUE_H__

#include "messages.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>


typedef enum {
   EV_NUCLEO,
   EV_KEYBOARD,
   EV_NUM
} event_source;

typedef enum {
   EV_COMPUTE, // request compute on nucleo with particular 
   EV_RESET_CHUNK, //reset the chunk id
   EV_ABORT,
   EV_GET_VERSION,
   EV_THREAD_EXIT,
   EV_QUIT,
   EV_SERIAL,
   EV_SET_COMPUTE,
   EV_COMPUTE_CPU,
   EV_CLEAR_BUFFER,
   EV_REFRESH,
   EV_PIPE_IN_MESSAGE,
   EV_TYPE_NUM,
   EV_ZOOM,
   EV_DECREASE_ZOOM,
   EV_MOVE_L,
   EV_MOVE_R,
   EV_MOVE_D,
   EV_MOVE_U,
   EV_MOOD_O,
   EV_MOOD_2,
   EV_MOOD_3,
   EV_MOOD_4,
   EV_MOOD_5,
   EV_MOOD_6,
} event_type;

typedef struct {
   int param;
} event_keyboard;

typedef struct {
   message *msg;
} event_serial;

typedef struct {
   event_source source;
   event_type type;
   union {
      int param;
      message *msg;
   } data;
} event;

void queue_init(void);

void queue_cleanup(void);

event queue_pop(void);

void queue_push(event ev);

void set_quit();
bool is_quit();

#endif

/* end of event_queue.h */
