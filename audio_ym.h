#ifndef AUDIO_YM_H
#define AUDIO_YM_H

extern char AY_MONO[];
extern char AY_SABC[];
extern char AY_SACB[];

typedef unsigned char ayemu_ay_reg_frame_t[16];

typedef struct {
	int tone_a;		/**< R0, R1 */
	int tone_b;		/**< R2, R3 */
	int tone_c;		/**< R4, R5 */
	int noise;		/**< R6 */
	int R7_tone_a;	/**< R7 bit 0 */
	int R7_tone_b;	/**< R7 bit 1 */
	int R7_tone_c;	/**< R7 bit 2 */
	int R7_noise_a;	/**< R7 bit 3 */
	int R7_noise_b;	/**< R7 bit 4 */
	int R7_noise_c;	/**< R7 bit 5 */
	int vol_a;		/**< R8 bits 3-0 */
	int vol_b;		/**< R9 bits 3-0 */
	int vol_c;		/**< R10 bits 3-0 */
	int env_a;		/**< R8 bit 4 */
	int env_b;		/**< R9 bit 4 */
	int env_c;		/**< R10 bit 4 */
	int env_freq;	/**< R11, R12 */
	int env_style;	/**< R13 */
} ayemu_regdata_t;

typedef struct {
	int table[32];			/**< table of volumes for chip */
	int ChipFreq;			/**< chip emulator frequency */
	int eq[6];				/**< volumes for channels, range -100..100 **/
	int bit_a;				/**< state of channel A generator */
	int bit_b;				/**< state of channel B generator */
	int bit_c;				/**< state of channel C generator */
	int bit_n;				/**< current generator state */
	int cnt_a;				/**< back counter of A */
	int cnt_b;				/**< back counter of B */
	int cnt_c;				/**< back counter of C */
	int cnt_n;				/**< back counter of noise generator */
	int cnt_e;				/**< back counter of envelop generator */
	int EnvNum;				/**< number of current envilopment (0...15) */
	int env_pos;			/**< current position in envelop (0...127) */
	int Cur_Seed;			/**< random numbers counter */
	ayemu_regdata_t regs;	/**< parsed registers data */
	int sndfreq;	/**< output sound format */
	int ChipStep;
	int vols[6][32];		/**< stereo type (channel volumes) and chip table. This cache calculated by #table and #eq	*/
} ayemu_ay_t;

void ayemu_init(ayemu_ay_t * ay, int chipfreq, int freq, char* pan);
void ayemu_set_reg(ayemu_ay_t * ay, int sel, int dat);
uint32_t ayemu_mix(ayemu_ay_t * ay);
#endif
