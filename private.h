#include "public.h"

int lib_init(void);
void lib_free(void);

int band_init(void);
void band_free(void);

extern Synth *synths[NUM_CHANNELS];

typedef enum {
	NOTE_OFF = 0,
	NOTE_ON = 1,
	PITCH = 2,
	CONTROL = 3,
} MessageType;

typedef struct {
	uint32_t time;
	int channel;
	MessageType type;
	union {
		struct {
			int num;
			float velocity;
		} note;
		struct {
			float offset;
		} pitch;
		struct {
			float *ptr;
			float value;
		} control;
	} data;
} Message;

int mq_init(void);
void mq_free(void);
bool mq_push(Message *message);
Message *mq_pop();
