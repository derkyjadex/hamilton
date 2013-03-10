#include <stdint.h>
#include <stdbool.h>

static const int SAMPLE_RATE = 48000;
static const int BUFFER_SIZE = 256;
static const int NUM_CHANNELS = 4;

int audio_init(void);
void audio_free(void);
void audio_start(void);
void audio_pause(void);

struct Synth;
typedef struct Synth Synth;

struct SynthType;
typedef struct SynthType SynthType;

struct SynthType {
	const char *name;
	Synth *(*init)(const SynthType *type);
};

struct Synth {
	const SynthType *type;
	void (*free)(Synth *synth);

	const char **(*getControls)(Synth *synth, int *numControls);
	float *(*getControl)(Synth *synth, const char *control);

	void (*startNote)(Synth *synth, int num, float velocity);
	void (*stopNote)(Synth *synth, int num);

	void (*generate)(Synth *synth, int16_t *buffer, int length);
};

int lib_add_synth(const char *name, Synth *(*init)(const SynthType *type));
const SynthType *lib_get_synths(int *num);
const SynthType *lib_get_synth(const char *name);

int band_init(void);
void band_free(void);

void band_get_channel_synths(const SynthType *types[NUM_CHANNELS]);
int band_set_synth(int channel, const SynthType *type);

const char **band_get_channel_controls(int channel, int *numControls);
float *band_get_channel_control(int channel, const char *control);
void band_run(int16_t *buffer, int length);

bool mq_note(int time, int channel, bool state, int num, float velocity);
bool mq_pitch(int time, int channel, float offset);
bool mq_cc(int time, int channel, float *control, float value);
