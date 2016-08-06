/*
 * unsf breaks up sound fonts into GUS type patch files.
 *
 * usage: unsf <name of sound font file>
 *
 * compile: cc -o unsf unsf.c -lm
 *
 * license: cc0
 *
 * To the extent possible under law, the person who associated CC0 with
 * unsf has waived all copyright and related or neighboring rights
 * to unsf.
 *
 * You should have received a copy of the CC0 legalcode along with this
 * work. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__FreeBSD__)
    #include <sys/endian.h>
#elif defined(__APPLE__)
    #include <machine/endian.h>
#else
    #include <endian.h>
#endif

#include "libunsf.h"

unsigned int freq_table[128] =
{
 8176, 8662, 9177, 9723, 
 10301, 10913, 11562, 12250, 
 12978, 13750, 14568, 15434,
 
 16352, 17324, 18354, 19445,
 20602, 21827, 23125, 24500, 
 25957, 27500, 29135, 30868, 

 32703, 34648, 36708, 38891,
 41203, 43654, 46249, 48999,
 51913, 55000, 58270, 61735,

 65406, 69296, 73416, 77782,
 82407, 87307, 92499, 97999,
 103826, 110000, 116541, 123471,

 130813, 138591, 146832, 155563,
 164814, 174614, 184997, 195998,
 207652, 220000, 233082, 246942,

 261626, 277183, 293665, 311127,
 329628, 349228, 369994, 391995,
 415305, 440000, 466164, 493883,

 523251, 554365, 587330, 622254,
 659255, 698456, 739989, 783991,
 830609, 880000, 932328, 987767,

 1046502, 1108731, 1174659, 1244508,
 1318510, 1396913, 1479978, 1567982,
 1661219, 1760000, 1864655, 1975533,

 2093005, 2217461, 2349318, 2489016,
 2637020, 2793826, 2959955, 3135963,
 3322438, 3520000, 3729310, 3951066,

 4186009, 4434922, 4698636, 4978032,
 5274041, 5587652, 5919911, 6271927,
 6644875, 7040000, 7458620, 7902133,

 8372018, 8869844, 9397273, 9956063, 
 10548082, 11175303, 11839822, 12543854
};

double bend_fine[256] = {
 1, 1.0002256593050698, 1.0004513695322617, 1.0006771306930664, 
 1.0009029427989777, 1.0011288058614922, 1.0013547198921082, 1.0015806849023274,
 1.0018067009036538, 1.002032767907594, 1.0022588859256572, 1.0024850549693551,
 1.0027112750502025, 1.0029375461797159, 1.0031638683694153, 1.0033902416308227,
 1.0036166659754628, 1.0038431414148634, 1.0040696679605541, 1.0042962456240678,
 1.0045228744169397, 1.0047495543507072, 1.0049762854369111, 1.0052030676870944,
 1.0054299011128027, 1.0056567857255843, 1.00588372153699, 1.006110708558573,
 1.0063377468018897, 1.0065648362784985, 1.0067919769999607, 1.0070191689778405,
 1.0072464122237039, 1.0074737067491204, 1.0077010525656616, 1.0079284496849015,
 1.0081558981184175, 1.008383397877789, 1.008610948974598, 1.0088385514204294,
 1.0090662052268706, 1.0092939104055114, 1.0095216669679448, 1.0097494749257656,
 1.009977334290572, 1.0102052450739643, 1.0104332072875455, 1.0106612209429215,
 1.0108892860517005, 1.0111174026254934, 1.0113455706759138, 1.0115737902145781,
 1.0118020612531047, 1.0120303838031153, 1.0122587578762337, 1.012487183484087,
 1.0127156606383041, 1.0129441893505169, 1.0131727696323602, 1.0134014014954713,
 1.0136300849514894, 1.0138588200120575, 1.0140876066888203, 1.0143164449934257,
 1.0145453349375237, 1.0147742765327674, 1.0150032697908125, 1.0152323147233171,
 1.015461411341942, 1.0156905596583505, 1.0159197596842091, 1.0161490114311862,
 1.0163783149109531, 1.0166076701351838, 1.0168370771155553, 1.0170665358637463,
 1.0172960463914391, 1.0175256087103179, 1.0177552228320703, 1.0179848887683858,
 1.0182146065309567, 1.0184443761314785, 1.0186741975816487, 1.0189040708931674,
 1.0191339960777379, 1.0193639731470658, 1.0195940021128593, 1.0198240829868295,
 1.0200542157806898, 1.0202844005061564, 1.0205146371749483, 1.0207449257987866,
 1.0209752663893958, 1.0212056589585028, 1.0214361035178368, 1.0216666000791297,
 1.0218971486541166, 1.0221277492545349, 1.0223584018921241, 1.0225891065786274,
 1.0228198633257899, 1.0230506721453596, 1.023281533049087, 1.0235124460487257,
 1.0237434111560313, 1.0239744283827625, 1.0242054977406807, 1.0244366192415495,
 1.0246677928971357, 1.0248990187192082, 1.025130296719539, 1.0253616269099028,
 1.0255930093020766, 1.0258244439078401, 1.0260559307389761, 1.0262874698072693,
 1.0265190611245079, 1.0267507047024822, 1.0269824005529853, 1.027214148687813,
 1.0274459491187637, 1.0276778018576387, 1.0279097069162415, 1.0281416643063788,
 1.0283736740398595, 1.0286057361284953, 1.0288378505841009, 1.0290700174184932,
 1.0293022366434921, 1.0295345082709197, 1.0297668323126017, 1.0299992087803651,
 1.030231637686041, 1.0304641190414621, 1.0306966528584645, 1.0309292391488862,
 1.0311618779245688, 1.0313945691973556, 1.0316273129790936, 1.0318601092816313,
 1.0320929581168212, 1.0323258594965172, 1.0325588134325767, 1.0327918199368598,
 1.0330248790212284, 1.0332579906975481, 1.0334911549776868, 1.033724371873515,
 1.0339576413969056, 1.0341909635597348, 1.0344243383738811, 1.0346577658512259,
 1.034891246003653, 1.0351247788430489, 1.0353583643813031, 1.0355920026303078,
 1.0358256936019572, 1.0360594373081489, 1.0362932337607829, 1.0365270829717617,
 1.0367609849529913, 1.0369949397163791, 1.0372289472738365, 1.0374630076372766,
 1.0376971208186156, 1.0379312868297725, 1.0381655056826686, 1.0383997773892284,
 1.0386341019613787, 1.0388684794110492, 1.0391029097501721, 1.0393373929906822,
 1.0395719291445176, 1.0398065182236185, 1.0400411602399278, 1.0402758552053915,
 1.0405106031319582, 1.0407454040315787, 1.0409802579162071, 1.0412151647977996,
 1.0414501246883161, 1.0416851375997183, 1.0419202035439705, 1.0421553225330404,
 1.042390494578898, 1.042625719693516, 1.0428609978888699, 1.043096329176938,
 1.0433317135697009, 1.0435671510791424, 1.0438026417172486, 1.0440381854960086,
 1.0442737824274138, 1.044509432523459, 1.044745135796141, 1.0449808922574599,
 1.0452167019194181, 1.0454525647940205, 1.0456884808932754, 1.0459244502291931,
 1.0461604728137874, 1.0463965486590741, 1.046632677777072, 1.0468688601798024,
 1.0471050958792898, 1.047341384887561, 1.0475777272166455, 1.047814122878576,
 1.048050571885387, 1.0482870742491166, 1.0485236299818055, 1.0487602390954964,
 1.0489969016022356, 1.0492336175140715, 1.0494703868430555, 1.0497072096012419,
 1.0499440858006872, 1.0501810154534512, 1.050417998571596, 1.0506550351671864,
 1.0508921252522903, 1.0511292688389782, 1.0513664659393229, 1.0516037165654004,
 1.0518410207292894, 1.0520783784430709, 1.0523157897188296, 1.0525532545686513,
 1.0527907730046264, 1.0530283450388465, 1.0532659706834067, 1.0535036499504049,
 1.0537413828519411, 1.0539791694001188, 1.0542170096070436, 1.0544549034848243,
 1.0546928510455722, 1.0549308523014012, 1.0551689072644284, 1.0554070159467728,
 1.0556451783605572, 1.0558833945179062, 1.0561216644309479, 1.0563599881118126,
 1.0565983655726334, 1.0568367968255465, 1.0570752818826903, 1.0573138207562065,
 1.057552413458239, 1.0577910600009348, 1.0580297603964437, 1.058268514656918,
 1.0585073227945128, 1.0587461848213857, 1.058985100749698, 1.0592240705916123
};

double bend_coarse[128] = {
 1, 1.0594630943592953, 1.122462048309373, 1.189207115002721,
 1.2599210498948732, 1.3348398541700344, 1.4142135623730951, 1.4983070768766815,
 1.5874010519681994, 1.681792830507429, 1.7817974362806785, 1.8877486253633868,
 2, 2.1189261887185906, 2.244924096618746, 2.3784142300054421,
 2.5198420997897464, 2.6696797083400687, 2.8284271247461903, 2.996614153753363,
 3.1748021039363992, 3.363585661014858, 3.5635948725613571, 3.7754972507267741,
 4, 4.2378523774371812, 4.4898481932374912, 4.7568284600108841,
 5.0396841995794928, 5.3393594166801366, 5.6568542494923806, 5.993228307506727,
 6.3496042078727974, 6.727171322029716, 7.1271897451227151, 7.5509945014535473,
 8, 8.4757047548743625, 8.9796963864749824, 9.5136569200217682,
 10.079368399158986, 10.678718833360273, 11.313708498984761, 11.986456615013454,
 12.699208415745595, 13.454342644059432, 14.25437949024543, 15.101989002907095,
 16, 16.951409509748721, 17.959392772949972, 19.027313840043536,
 20.158736798317967, 21.357437666720553, 22.627416997969522, 23.972913230026901,
 25.398416831491197, 26.908685288118864, 28.508758980490853, 30.203978005814196,
 32, 33.902819019497443, 35.918785545899944, 38.054627680087073,
 40.317473596635935, 42.714875333441107, 45.254833995939045, 47.945826460053802,
 50.796833662982394, 53.817370576237728, 57.017517960981706, 60.407956011628393,
 64, 67.805638038994886, 71.837571091799887, 76.109255360174146,
 80.63494719327187, 85.429750666882214, 90.509667991878089, 95.891652920107603,
 101.59366732596479, 107.63474115247546, 114.03503592196341, 120.81591202325679,
 128, 135.61127607798977, 143.67514218359977, 152.21851072034829,
 161.26989438654374, 170.85950133376443, 181.01933598375618, 191.78330584021521,
 203.18733465192958, 215.26948230495091, 228.07007184392683, 241.63182404651357,
 256, 271.22255215597971, 287.35028436719938, 304.43702144069658,
 322.53978877308765, 341.71900266752868, 362.03867196751236, 383.56661168043064,
 406.37466930385892, 430.53896460990183, 456.14014368785394, 483.26364809302686,
 512, 542.44510431195943, 574.70056873439876, 608.87404288139317,
 645.0795775461753, 683.43800533505737, 724.07734393502471, 767.13322336086128,
 812.74933860771785, 861.07792921980365, 912.28028737570787, 966.52729618605372,
 1024, 1084.8902086239189, 1149.4011374687975, 1217.7480857627863,
 1290.1591550923506, 1366.8760106701147, 1448.1546878700494, 1534.2664467217226
};






/* writes a byte to the memory buffer */
static void mem_write8(int val)
{
   if (mem_size >= mem_alloced) {
      mem_alloced += 4096;
      if (!(mem = realloc(mem, mem_alloced))) {
	      fprintf(stderr, "Memory allocation of %d failed with mem size %d\n", mem_alloced, mem_size);
	      exit(1);
      }
   }

   mem[mem_size] = val;
   mem_size++;
}



/* writes a word to the memory buffer (little endian) */
static void mem_write16(int val)
{
   mem_write8(val & 0xFF);
   mem_write8((val >> 8) & 0xFF);
}



/* writes a long to the memory buffer (little endian) */
static void mem_write32(int val)
{
   mem_write8(val & 0xFF);
   mem_write8((val >> 8) & 0xFF);
   mem_write8((val >> 16) & 0xFF);
   mem_write8((val >> 24) & 0xFF);
}


/* writes a block of data the memory buffer */
static void mem_write_block(void *data, int size)
{
   if (mem_size+size > mem_alloced) {
      mem_alloced = (mem_alloced + size + 4095) & ~4095;
      if (!(mem = realloc(mem, mem_alloced))) {
	      fprintf(stderr, "Memory allocation of %d failed with mem size %d\n", mem_alloced, mem_size);
	      exit(1);
      }
   }

   memcpy(mem+mem_size, data, size);
   mem_size += size;
}








static EMPTY_WHITE_ROOM waiting_list[MAX_WAITING];

static int waiting_list_count;

/* SoundFont parameters for the current sample */
static int sf_instrument_look_index;
static int sf_instrument_unused5;
static int sf_sample_look_index;
static int sf_start, sf_end;
static int sf_loop_start, sf_loop_end;
static int sf_key, sf_tune;
static int sf_pan;
static int sf_keyscale;
static int sf_keymin, sf_keymax;
static int sf_velmin, sf_velmax;
static int sf_sustain_mod_env;
static int sf_mod_env_to_pitch;
static int sf_delay_vol_env;
static int sf_attack_vol_env;
static int sf_hold_vol_env;
static int sf_decay_vol_env;
static int sf_release_vol_env;
static int sf_sustain_level;
static int sf_mode;

static int sp_freq_center;
static int sf_exclusiveClass;

static int sf_chorusEffectsSend;
static int sf_reverbEffectsSend;
static int sf_initialAttenuation;
static int sf_modLfoToPitch;
static int sf_vibLfoToPitch;
static int sf_velocity;
static int sf_keynum;
static int sf_keynumToModEnvHold;
static int sf_keynumToModEnvDecay;
static int sf_keynumToVolEnvHold;
static int sf_keynumToVolEnvDecay;
static int sf_modLfoToVolume;
static int sf_delayModLFO;
static int sp_delayModLFO;
static int sf_freqModLFO;
static int sf_delayVibLFO;
static int sf_freqVibLFO;
static int sf_delayModEnv;
static int sf_attackModEnv;
static int sf_holdModEnv;
static int sf_decayModEnv;
static int sf_releaseModEnv;

static short sf_initialFilterQ;
static short sf_initialFilterFc;
static short sf_modEnvToFilterFc;
static short sf_modLfoToFilterFc;
static short sp_resonance;
static short sp_modEnvToFilterFc;
static short sp_modLfoToFilterFc;
static short sp_modEnvToPitch;
static short sp_cutoff_freq;
static int sp_vibrato_depth, sp_vibrato_delay;
static unsigned char sp_vibrato_control_ratio, sp_vibrato_sweep_increment;
static int sp_tremolo_depth;
static unsigned char sp_tremolo_phase_increment, sp_tremolo_sweep_increment;
static int sp_lfo_depth;
static short sp_lfo_phase_increment;	/* sp_lfo_phase_increment is actually frequency */

/* interprets a SoundFont generator object */
static void apply_generator(sfGenList *g, int preset, int global)
{
   switch (g->sfGenOper) {

      case SFGEN_startAddrsOffset:
	 sf_start += g->genAmount.shAmount;
	 break;

      case SFGEN_endAddrsOffset:
	 sf_end += g->genAmount.shAmount;
	 break;

      case SFGEN_startloopAddrsOffset:
	 sf_loop_start += g->genAmount.shAmount;
	 break;

      case SFGEN_endloopAddrsOffset:
	 sf_loop_end += g->genAmount.shAmount;
	 break;

      case SFGEN_startAddrsCoarseOffset:
	 sf_start += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_endAddrsCoarseOffset:
	 sf_end += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_startloopAddrsCoarse:
	 sf_loop_start += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_endloopAddrsCoarse:
	 sf_loop_end += (int)g->genAmount.shAmount * 32768;
	 break;

      case SFGEN_modEnvToPitch:
	 if (preset)
	    sf_mod_env_to_pitch += g->genAmount.shAmount;
	 else
	    sf_mod_env_to_pitch = g->genAmount.shAmount;
	 break;

      case SFGEN_sustainModEnv:
	 if (preset)
	    sf_sustain_mod_env += g->genAmount.shAmount;
	 else
	    sf_sustain_mod_env = g->genAmount.shAmount;
	 break;

      case SFGEN_delayVolEnv:
	 if (preset)
	    sf_delay_vol_env += g->genAmount.shAmount;
	 else
	    sf_delay_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_attackVolEnv:
	 if (preset)
	    sf_attack_vol_env += g->genAmount.shAmount;
	 else
	    sf_attack_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_holdVolEnv:
	 if (preset)
	    sf_hold_vol_env += g->genAmount.shAmount;
	 else
	    sf_hold_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_decayVolEnv:
	 if (preset)
	    sf_decay_vol_env += g->genAmount.shAmount;
	 else
	    sf_decay_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_sustainVolEnv:
	 if (preset)
	    sf_sustain_level += g->genAmount.shAmount;
	 else
	    sf_sustain_level = g->genAmount.shAmount;
	 break;

      case SFGEN_releaseVolEnv:
	 if (preset)
	    sf_release_vol_env += g->genAmount.shAmount;
	 else
	    sf_release_vol_env = g->genAmount.shAmount;
	 break;

      case SFGEN_pan:
	 if (preset)
	    sf_pan += g->genAmount.shAmount;
	 else
	    sf_pan = g->genAmount.shAmount;
	 break;

      case SFGEN_keyRange:
	 if (preset) {
	     if (g->genAmount.ranges.byLo >= sf_keymin && g->genAmount.ranges.byHi <= sf_keymax) {
	         sf_keymin = g->genAmount.ranges.byLo;
	         sf_keymax = g->genAmount.ranges.byHi;
	     }
	 }
	 else {
	     sf_keymin = g->genAmount.ranges.byLo;
	     sf_keymax = g->genAmount.ranges.byHi;
	 }
	 break;

      case SFGEN_velRange:
	 if (preset) {
	     if (g->genAmount.ranges.byLo >= sf_velmin && g->genAmount.ranges.byHi <= sf_velmax) {
	         sf_velmin = g->genAmount.ranges.byLo;
	         sf_velmax = g->genAmount.ranges.byHi;
	     }
	 }
	 else {
	     sf_velmin = g->genAmount.ranges.byLo;
	     sf_velmax = g->genAmount.ranges.byHi;
	 }
	 break;

      case SFGEN_coarseTune:
	 if (preset)
	    sf_tune += g->genAmount.shAmount * 100;
	 else
	    sf_tune = g->genAmount.shAmount * 100;
	 break;

      case SFGEN_fineTune:
	 if (preset)
	    sf_tune += g->genAmount.shAmount;
	 else
	    sf_tune = g->genAmount.shAmount;
	 break;

      case SFGEN_sampleModes:
	 sf_mode = g->genAmount.wAmount;
	 break;

      case SFGEN_scaleTuning:
	 if (preset)
	    sf_keyscale += g->genAmount.shAmount;
	 else
	    sf_keyscale = g->genAmount.shAmount;
	 break;

      case SFGEN_overridingRootKey:
	 if (g->genAmount.shAmount >= 0 && g->genAmount.shAmount <= 127)
	    sf_key = g->genAmount.shAmount;
	 break;

      case SFGEN_exclusiveClass:
	    sf_exclusiveClass = g->genAmount.shAmount;
	 break;

      case SFGEN_initialAttenuation:
	 if (preset)
	    sf_initialAttenuation += g->genAmount.shAmount;
	 else
	    sf_initialAttenuation = g->genAmount.shAmount;
	 break;
      case SFGEN_chorusEffectsSend:
	 if (preset)
	    sf_chorusEffectsSend += g->genAmount.shAmount;
	 else
	    sf_chorusEffectsSend = g->genAmount.shAmount;
	 break;
      case SFGEN_reverbEffectsSend:
	 if (preset)
	    sf_reverbEffectsSend += g->genAmount.shAmount;
	 else
	    sf_reverbEffectsSend = g->genAmount.shAmount;
	 break;
      case SFGEN_modLfoToPitch:
	 if (preset)
	    sf_modLfoToPitch += g->genAmount.shAmount;
	 else
	    sf_modLfoToPitch = g->genAmount.shAmount;
	 break;
      case SFGEN_vibLfoToPitch:
	 if (preset)
	    sf_vibLfoToPitch += g->genAmount.shAmount;
	 else
	    sf_vibLfoToPitch = g->genAmount.shAmount;
	 break;
      case SFGEN_velocity:
	    sf_velocity = g->genAmount.shAmount;
	 break;
      case SFGEN_keynum:
	    sf_keynum = g->genAmount.shAmount;
	 break;
      case SFGEN_keynumToModEnvHold:
	 if (preset)
	    sf_keynumToModEnvHold += g->genAmount.shAmount;
	 else
	    sf_keynumToModEnvHold = g->genAmount.shAmount;
	 break;
      case SFGEN_keynumToModEnvDecay:
	 if (preset)
	    sf_keynumToModEnvDecay += g->genAmount.shAmount;
	 else
	    sf_keynumToModEnvDecay = g->genAmount.shAmount;
	 break;
      case SFGEN_keynumToVolEnvHold:
	 if (preset)
	    sf_keynumToVolEnvHold += g->genAmount.shAmount;
	 else
	    sf_keynumToVolEnvHold = g->genAmount.shAmount;
	 break;
      case SFGEN_keynumToVolEnvDecay:
	 if (preset)
	    sf_keynumToVolEnvDecay += g->genAmount.shAmount;
	 else
	    sf_keynumToVolEnvDecay = g->genAmount.shAmount;
	 break;
      case SFGEN_modLfoToVolume:
	 if (preset)
	    sf_modLfoToVolume += g->genAmount.shAmount;
	 else
	    sf_modLfoToVolume = g->genAmount.shAmount;
	 break;
      case SFGEN_delayModLFO:
	 if (preset)
	    sf_delayModLFO += g->genAmount.shAmount;
	 else
	    sf_delayModLFO = g->genAmount.shAmount;
	 break;
      case SFGEN_freqModLFO:
	 if (preset)
	    sf_freqModLFO += g->genAmount.shAmount;
	 else
	    sf_freqModLFO = g->genAmount.shAmount;
	 break;
      case SFGEN_delayVibLFO:
	 if (preset)
	    sf_delayVibLFO += g->genAmount.shAmount;
	 else
	    sf_delayVibLFO = g->genAmount.shAmount;
	 break;
      case SFGEN_freqVibLFO:
	 if (preset)
	    sf_freqVibLFO += g->genAmount.shAmount;
	 else
	    sf_freqVibLFO = g->genAmount.shAmount;
	 break;
      case SFGEN_delayModEnv:
	 if (preset)
	    sf_delayModEnv += g->genAmount.shAmount;
	 else
	    sf_delayModEnv = g->genAmount.shAmount;
	 break;
      case SFGEN_attackModEnv:
	 if (preset)
	    sf_attackModEnv += g->genAmount.shAmount;
	 else
	    sf_attackModEnv = g->genAmount.shAmount;
	 break;
      case SFGEN_holdModEnv:
	 if (preset)
	    sf_holdModEnv += g->genAmount.shAmount;
	 else
	    sf_holdModEnv = g->genAmount.shAmount;
	 break;
      case SFGEN_decayModEnv:
	 if (preset)
	    sf_decayModEnv += g->genAmount.shAmount;
	 else
	    sf_decayModEnv = g->genAmount.shAmount;
	 break;
      case SFGEN_releaseModEnv:
	 if (preset)
	    sf_releaseModEnv += g->genAmount.shAmount;
	 else
	    sf_releaseModEnv = g->genAmount.shAmount;
	 break;
      case SFGEN_initialFilterQ:
	 if (preset)
	    sf_initialFilterQ += g->genAmount.shAmount;
	 else
	    sf_initialFilterQ = g->genAmount.shAmount;
	 break;
      case SFGEN_initialFilterFc:
	 if (preset)
	    sf_initialFilterFc += g->genAmount.shAmount;
	 else
	    sf_initialFilterFc = g->genAmount.shAmount;
	 break;
      case SFGEN_modEnvToFilterFc:
	 if (preset)
	    sf_modEnvToFilterFc += g->genAmount.shAmount;
	 else
	    sf_modEnvToFilterFc = g->genAmount.shAmount;
	 break;
      case SFGEN_modLfoToFilterFc:
	 if (preset)
	    sf_modLfoToFilterFc += g->genAmount.shAmount;
	 else
	    sf_modLfoToFilterFc = g->genAmount.shAmount;
	 break;
      case SFGEN_instrument:
	 sf_instrument_look_index = g->genAmount.shAmount;
	 break;
      case SFGEN_sampleID:
	 sf_sample_look_index = g->genAmount.shAmount;
	 break;
      case SFGEN_unused5:
	 sf_instrument_unused5 = g->genAmount.shAmount;
	 if (opt_verbose) printf("APS parameter %d\n", sf_instrument_unused5);
	 break;
      default:
	 fprintf(stderr,"Warning: generator %d with value %d not handled at the %s %s level\n",
			 g->sfGenOper, g->genAmount.shAmount,
			 global? "global":"local",
			 preset? "preset":"instrument");
	 break;
   }
}



/* converts AWE32 (MIDI) pitches to GUS (frequency) format */
static int key2freq(int note, int cents)
{
   return pow(2.0, (float)(note*100+cents)/1200.0) * 8175.800781;
}



/* converts the strange AWE32 timecent values to milliseconds */
static int timecent2msec(int t)
{
   double msec;
   msec = (double)(1000 * pow(2.0, (double)( t ) / 1200.0));
   return (int)msec;
}



/* converts milliseconds to the even stranger floating point GUS format */
static int msec2gus(int t, int r)
{
   static int vexp[4] = { 1, 8, 64, 512 };
   int e, m;

   if (r <= 0)
      return 0x3F;

   t = t * 32 / r;

   if (t <= 0)
      return 0x3F;

   for (e=3; e>=0; e--) {
      m = (vexp[e] * 16 + t/2) / t;

      if ((m > 0) && (m < 64))
	 return ((e << 6) | m);
   }

   return 0xC1;
}

/* Bits in modes: */
#define MODES_16BIT	(1<<0)
#define MODES_UNSIGNED	(1<<1)
#define MODES_LOOPING	(1<<2)
#define MODES_PINGPONG	(1<<3)
#define MODES_REVERSE	(1<<4)
#define MODES_SUSTAIN	(1<<5)
#define MODES_ENVELOPE	(1<<6)
#define MODES_FAST_RELEASE	(1<<7)

/* The sampleFlags value, in my experience, is not to be trusted,
 * so the following uses some heuristics to guess whether to
 * set looping and sustain modes. (gl)
 */
static int getmodes(int sampleFlags, int program, int banknum)
{
	int modes;
	int orig_sampleFlags = sampleFlags;

	modes = MODES_ENVELOPE;

      if (opt_8bit)
	 modes |= MODES_UNSIGNED;                      /* signed waveform */
      else
	 modes |= MODES_16BIT;                      /* 16-bit waveform */


	if (sampleFlags == 3) modes |= MODES_FAST_RELEASE;

	/* arbitrary adjustments (look at sustain of vol envelope? ) */

      if (opt_adjust_sample_flags) {

	if (sampleFlags && sf_sustain_mod_env == 0) sampleFlags = 3;
	else if (sampleFlags && sf_sustain_mod_env >= 1000) sampleFlags = 1;
	else if (banknum != 128 && sampleFlags == 1) {
		/* organs, accordians */
		if (program >= 16 && program <= 23) sampleFlags = 3;
		/* strings */
		else if (program >= 40 && program <= 44) sampleFlags = 3;
		/* strings, voice */
		else if (program >= 48 && program <= 54) sampleFlags = 3;
		/* horns, woodwinds */
		else if (program >= 56 && program <= 79) sampleFlags = 3;
		/* lead, pad, fx */
		else if (program >= 80 && program <=103) sampleFlags = 3;
		/* bagpipe, fiddle, shanai */
		else if (program >=109 && program <=111) sampleFlags = 3;
		/* breath noise, ... telephone, helicopter */
		else if (program >=121 && program <=125) sampleFlags = 3;
		/* applause */
		else if (program ==126) sampleFlags = 3;
	}

	if (opt_verbose && orig_sampleFlags != sampleFlags)
		printf("changed sampleFlags from %d to %d\n",
				orig_sampleFlags, sampleFlags);
      }
      else if (sampleFlags == 1) sampleFlags = 3;

	if (sampleFlags == 1 || sampleFlags == 3)
		modes |= MODES_LOOPING;
	if (sampleFlags == 3)
		modes |= MODES_SUSTAIN;
	return modes;
}


/* calculate root pitch */
/* This code is derived from some version of Timidity++ and comes
 * from Takashi Iwai and/or Masanao Izumi (who are not responsible
 * for my interpretation of it). (gl)
 */
static int calc_root_pitch(void)
{
	int root, tune;

	root = sf_key; /* sample originalPitch */
	tune = sf_tune; /* sample pitchCorrection */

	/* tuning */
	/*tune += sf_keyscale; Why did I say this? */
/* ??
		tune += lay->val[SF_coarseTune] * 100
			+ lay->val[SF_fineTune];
*/

	/* it's too high.. */
	if (root >= sf_keymax + 60)
		root -= 60;
/*
	if (lay->set[SF_keyRange] &&
	    root >= HI_VAL(lay->val[SF_keyRange]) + 60)
		root -= 60;

*/

	while (tune <= -100) {
		root++;
		tune += 100;
	}
	while (tune > 0) {
		root--;
		tune -= 100;
	}

    if (root > 0) sp_freq_center = root;
    else sp_freq_center = 60;

    tune = (-tune * 256) / 100;

    if (root > 127)
	return (int)((double)freq_table[127] *
				  bend_coarse[root - 127] * bend_fine[tune]);
    else if (root < 0)
	return (int)((double)freq_table[0] /
				  bend_coarse[-root] * bend_fine[tune]);
    else
	return (int)((double)freq_table[root] * bend_fine[tune]);

}


#define CB_TO_VOLUME(centibel) (255 * (1.0 - ((double)(centibel)/100.0) / (1200.0 * log10(2.0)) ))
/* convert peak volume to linear volume (0-255) */
static unsigned int calc_volume(void)
{
	int v;
	double ret;

	if (!sf_initialAttenuation) return 255;
	v = sf_initialAttenuation;
	if (v < 0) v = 0;
	else if (v > 960) v = 960;
	ret = CB_TO_VOLUME((double)v);
	if (ret < 1.0) return 0;
	if (ret > 255.0) return 255;
	return (unsigned int)ret;
}


#define TO_VOLUME(centibel) (unsigned char)(255 * pow(10.0, -(double)(centibel)/200.0))
/* convert sustain volume to linear volume */
static unsigned char calc_sustain(void)
{
	int level;

	if (!sf_sustain_level) return 250;
	level = TO_VOLUME(sf_sustain_level);
	if (level > 253) level = 253;
	if (level < 100) level = 250; /* Protect against bogus value? This is for PC42c saxes. */
	return (unsigned char)level;
}
static unsigned int calc_mod_sustain(void)
{
	if (!sf_sustain_mod_env) return 250;
	return TO_VOLUME(sf_sustain_mod_env);
}
static void calc_resonance(void)
{
	short val = sf_initialFilterQ;
	//sp_resonance = pow(10.0, (double)val / 2.0 / 200.0) - 1;
	sp_resonance = val;
	if (sp_resonance < 0)
		sp_resonance = 0;
}

#define TO_HZ(abscents) (int)(8.176 * pow(2.0,(double)(abscents)/1200.0))
#define TO_HZ20(abscents) (int)(20 * 8.176 * pow(2.0,(double)(abscents)/1200.0))
/* calculate cutoff/resonance frequency */
static void calc_cutoff(void)
{
	short val;

	if (sf_initialFilterFc < 1) val = 13500;
	else val = sf_initialFilterFc;

	if (sf_modEnvToFilterFc /*&& sf_initialFilterFc*/) {
		sp_modEnvToFilterFc = pow(2.0, ((double)sf_modEnvToFilterFc/1200.0));
	}
	else sp_modEnvToFilterFc = 0;

	if (sf_modLfoToFilterFc /* && sf_initialFilterFc*/) {
		sp_modLfoToFilterFc = pow(2.0, ((double)sf_modLfoToFilterFc/1200.0));
	}
	else sp_modLfoToFilterFc = 0;

	if (sf_mod_env_to_pitch) {
		sp_modEnvToPitch = pow(2.0, ((double)sf_mod_env_to_pitch/1200.0));
	}
	else sp_modEnvToPitch = 0;

	sp_cutoff_freq = TO_HZ(val);
	if (val < 0 || val > 24000) val = 19192;
}

/*----------------------------------------------------------------
 * vibrato (LFO2) conversion
 * (note: my changes to Takashi's code are unprincipled --gl)
 *----------------------------------------------------------------*/
#ifndef VIBRATO_RATE_TUNING
#define VIBRATO_RATE_TUNING 38
#endif
static void convert_vibrato(void)
{
	int shift=0, freq=0, delay=0;

	if (sf_delayModLFO) sp_delayModLFO = (int)timecent2msec(sf_delayModLFO);

	if (sf_vibLfoToPitch) {
		shift = sf_vibLfoToPitch;
		if (sf_freqVibLFO) freq = sf_freqVibLFO;
		if (sf_delayVibLFO) delay = (int)timecent2msec(sf_delayVibLFO);
	}
	else if (sf_modLfoToPitch) {
		shift = sf_modLfoToPitch;
		if (sf_freqModLFO) freq = sf_freqModLFO;
		if (sf_delayModLFO) delay = sp_delayModLFO;
	}

	if (!shift) {
		sp_vibrato_depth = sp_vibrato_control_ratio = sp_vibrato_sweep_increment = 0;
		return;
	}

#if 0
	/* cents to linear; 400cents = 256 */
	shift = shift * 256 / 400;
	if (shift > 255) shift = 255;
	else if (shift < -255) shift = -255;
	sp_vibrato_depth = shift;
	/* This is Timidity++ code.  I don't think it makes sense.
	 * vibrato depth seems to be an unsigned 8 bit quantity.
	 */
#else
	/* cents to linear; 400cents = 256 */
	shift = (int)(pow(2.0, ((double)shift/1200.0)) * VIBRATO_RATE_TUNING);
	if (shift < 0) shift = -shift;
	if (shift < 2) shift = 2;
	if (shift > 20) shift = 20; /* arbitrary */
	sp_vibrato_depth = shift;
#endif

	/* frequency in mHz */
	if (!freq) freq = 8;
	else freq = TO_HZ(freq);

	if (freq < 1) freq = 1;

	freq *= 20;
	if (freq > 255) freq = 255;

	/* sp_vibrato_control_ratio = convert_vibrato_rate((unsigned char)freq); */
	sp_vibrato_control_ratio = (unsigned char)freq;

	/* convert mHz to control ratio */
	sp_vibrato_sweep_increment = (unsigned char)(freq/5);
	
	/* sp_vibrato_delay = delay * control_ratio;*/
	sp_vibrato_delay = delay;
}


#ifdef LFO_DEBUG
static void
      convert_lfo(char *name, int program, int banknum, int wanted_bank)
#else
static void
convert_lfo (void)
#endif
{
	int   freq = 0, shift = 0;

	if (!sf_modLfoToFilterFc) {
		sp_lfo_depth = sp_lfo_phase_increment = 0;
		return;
	}

	shift = sf_modLfoToFilterFc;
	if (sf_freqModLFO) freq = sf_freqModLFO;

	shift = (int)(pow(2.0, ((double)shift/1200.0)) * VIBRATO_RATE_TUNING);

	sp_lfo_depth = shift;

	if (!freq) freq = 8 * 20;
	else freq = TO_HZ20(freq);

	if (freq < 1) freq = 1;

	sp_lfo_phase_increment = (short)freq;
#ifdef LFO_DEBUG
	fprintf(stderr,"name=%s, bank=%d(%d), prog=%d, freq=%d\n",
			name, banknum, wanted_bank, program, freq);
#endif
}

/*----------------------------------------------------------------
 * tremolo (LFO1) conversion
 *----------------------------------------------------------------*/

static void convert_tremolo(void)
{
	int level;
	int freq;

	sp_tremolo_phase_increment = sp_tremolo_sweep_increment = sp_tremolo_depth = 0;

	if (!sf_modLfoToVolume) return;

	level = sf_modLfoToVolume;
	if (level < 0) level = -level;

	level = 255 - (unsigned char)(255 * (1.0 - (level) / (1200.0 * log10(2.0))));

	if (level < 0) level = -level;
	if (level > 20) level = 20; /* arbitrary */
	if (level < 2) level = 2;
	sp_tremolo_depth = level;

	/* frequency in mHz */
	if (!sf_freqModLFO) freq = 8;
	else {
		freq = sf_freqModLFO;
		freq = TO_HZ(freq);
	}

	if (freq < 1) freq = 1;
	freq *= 20;
	if (freq > 255) freq = 255;

	sp_tremolo_phase_increment = (unsigned char)freq;
	sp_tremolo_sweep_increment = ((unsigned char)(freq/5));
}



static int adjust_volume(int start, int length)
{
#if 0
      if (opt_adjust_volume) sp_volume = adjust_volume(sample->dwStart, length);
	 for (i=0; i<length; i++)
	    mem_write16(sf_sample_data[sample->dwStart+i]);
#endif
	    /* Try to determine a volume scaling factor for the sample.
	       This is a very crude adjustment, but things sound more
	       balanced with it. Still, this should be a runtime option. */
	 
	unsigned int  countsamp, numsamps = length;
	unsigned int  higher = 0, highcount = 0;
	short   maxamp = 0, a;
	short  *tmpdta = (short *) sf_sample_data + start;
	double new_vol;
	countsamp = numsamps;
	while (countsamp--) {
		a = *tmpdta++;
		if (a < 0)
		a = -a;
		if (a > maxamp)
		maxamp = a;
	}
	tmpdta = (short *) sf_sample_data + start;
	countsamp = numsamps;
	while (countsamp--) {
		a = *tmpdta++;
		if (a < 0)
		a = -a;
		if (a > 3 * maxamp / 4) {
		    higher += a;
		    highcount++;
		}
	}
	if (highcount)
		higher /= highcount;
	else
		higher = 10000;
	new_vol = (32768.0 * 0.875) / (double) higher;
	return (int)(new_vol * 255.0);
}



/* copies data from the waiting list into a GUS .pat struct */
static int grab_soundfont_sample(char *name, int program, int banknum, int wanted_bank)
{
   sfSample *sample;
   sfGenList *igen;
   sfGenList *pgen;
   sfGenList *global_izone;
   sfGenList *global_pzone;
   float vol, total_vol;
   int igen_count;
   int pgen_count;
   int global_izone_count;
   int global_pzone_count;
   int length;
   int min_freq, max_freq;
   int root_freq;
   int flags;
   int i, n;
   int delay, attack, hold, decay, release, sustain;
   int mod_delay, mod_attack, mod_hold, mod_decay, mod_release, mod_sustain;
   int freq_scale;
   unsigned int sp_volume, sample_volume;

   int debug_examples = 0;


   if (opt_header) {
        VelocityRangeList *vlist;
	int velcount, velcount_part1, k, velmin, velmax, left_patches, right_patches;

	mem_size = 0;

	mem_write_block("GF1PATCH110\0ID#000002\0", 22);

	for (i=0; i<60 && cpyrt[i]; i++) mem_write8(cpyrt[i]);
	for ( ; i<60; i++) mem_write8(0);

	mem_write8(1);                         /* number of instruments */
	mem_write8(14);                        /* number of voices */
	mem_write8(0);                         /* number of channels */
	mem_write16(waiting_list_count);       /* number of waveforms */
	mem_write16(127);                      /* master volume */
	mem_write32(0);                        /* data size (wrong!) */

/* Signal SF2 extensions present */
	mem_write_block("SF2EXT\0", 7);

	/* Remove this -- no longer used. */
/*
	if (wanted_bank == 128) mem_write8(drum_samples_right[banknum][program]);
	else mem_write8(voice_samples_right[banknum][program]);
*/

/* 36 bytes were reserved; now 29 left */
	for (i=8; i<37; i++)                   /* reserved */
	   mem_write8(0);

	mem_write16(0);                        /* instrument number */

	for (i=0; name[i] && i < 16; i++)      /* instrument name */
	   mem_write8(name[i]);

	while (i < 16) {                       /* pad instrument name */
	   mem_write8(0);
	   i++;
	}

	mem_write32(0);                        /* instrument size (wrong!) */
	mem_write8(1);                         /* number of layers */


    /* List of velocity layers with left and right patch counts. There is room for 10 here.
     * For each layer, give four bytes: velocity min, velocity max, #left patches, #right patches.
     */
	if (wanted_bank == 128 || opt_drum) vlist = drum_velocity[banknum][program];
	else vlist = voice_velocity[banknum][program];
	if (vlist) velcount = vlist->range_count;
	else velcount = 1;
	if (opt_small) velcount = 1;
	if (velcount > 19) velcount = 19;
	if (velcount > 9) velcount_part1 = 9;
	else velcount_part1 = velcount;

	mem_write8(velcount);

	for (k = 0; k < velcount_part1; k++) {
		if (vlist) {
			velmin = vlist->velmin[k];
			velmax = vlist->velmax[k];
			left_patches = vlist->left_patches[k] + vlist->mono_patches[k];
			if (vlist->right_patches[k] && !opt_mono)
				right_patches = vlist->right_patches[k]/* + vlist->mono_patches[k]*/;
			else right_patches = 0;
		}
		else {
			fprintf(stderr,"Internal error.\n");
			exit(1);
		}

		mem_write8(velmin);
		mem_write8(velmax);
		mem_write8(left_patches);
		mem_write8(right_patches);
	}

	for (i=0; i<40-1-4*velcount_part1; i++)                   /* reserved */
	   mem_write8(0);


	mem_write8(0);                         /* layer duplicate */
	mem_write8(0);                         /* layer number */
	mem_write32(0);                        /* layer size (wrong!) */
	mem_write8(waiting_list_count);        /* number of samples */

	if (velcount > velcount_part1) {
	   for (k = velcount_part1; k < velcount; k++) {
		if (vlist) {
			velmin = vlist->velmin[k];
			velmax = vlist->velmax[k];
			left_patches = vlist->left_patches[k] + vlist->mono_patches[k];
			if (vlist->right_patches[k] && !opt_mono)
				right_patches = vlist->right_patches[k]/* + vlist->mono_patches[k]*/;
			else right_patches = 0;
		}
		else {
			fprintf(stderr,"Internal error.\n");
			exit(1);
		}

		mem_write8(velmin);
		mem_write8(velmax);
		mem_write8(left_patches);
		mem_write8(right_patches);
	   }
	   for (i=0; i<40-4*(velcount-velcount_part1); i++)                   /* reserved */
	      mem_write8(0);
	}
	else {
	   for (i=0; i<40; i++)                   /* reserved */
	      mem_write8(0);
	}
   }
   /* bodge alert!!! I don't know how to make the volume parameters come
    * out right. If I ignore them, some things are very wrong (eg. with
    * the Emu 2MB bank the pads have overloud 'tinkle' layers, and the
    * square lead is way too loud). But if I process the attenuation
    * parameter the way I think it should be done, other things go wrong
    * (the church organ and honkytonk piano come out very quiet). So I
    * look at the volume setting and then normalise the results, to get
    * the differential between each layer but still keep a uniform overall
    * volume for the patch. Totally incorrect, but it usually seems to do 
    * the right thing...
    *
    * (This is currently disabled for 16 bit samples. gl)
    */

   total_vol = 0;

   for (n=0; n<waiting_list_count; n++) {
      int v = 0;
      int keymin = 0;
      int keymax = 127;

      /* look for volume and keyrange generators */
      for (i=0; i<waiting_list[n].igen_count; i++) {
	 if (waiting_list[n].igen[i].sfGenOper == SFGEN_initialAttenuation) {
	    v = waiting_list[n].igen[i].genAmount.shAmount;
	 }
	 else if (waiting_list[n].igen[i].sfGenOper == SFGEN_keyRange) {
	    keymin = waiting_list[n].igen[i].genAmount.ranges.byLo;
	    keymax = waiting_list[n].igen[i].genAmount.ranges.byHi;
	 }
      }

      for (i=0; i<waiting_list[n].pgen_count; i++) {
	 if (waiting_list[n].pgen[i].sfGenOper == SFGEN_initialAttenuation) {
	    v += waiting_list[n].pgen[i].genAmount.shAmount;
	 }
	 else if (waiting_list[n].pgen[i].sfGenOper == SFGEN_keyRange) {
	    keymin = waiting_list[n].pgen[i].genAmount.ranges.byLo;
	    keymax = waiting_list[n].pgen[i].genAmount.ranges.byHi;
	 }
      }


      /* convert centibels to scaling factor (I _think_ this is right :-) */
      vol = 1.0;
      while (v-- > 0)
	 vol /= pow(10, 0.005);

      waiting_list[n].volume = vol;

      if ((keymin <= 60) && (keymax >= 60))
	 total_vol += vol;
   }

   /* This normalization does not seem to work well for
    * timidity -- perhaps because it does it's own normalization.
    * So I don't use this value for the case of 16 bit samples,
    * though I've left it in place for 8 bit samples.  Perhaps
    * unwisely. (gl)
    */
   /* normalise the layer volumes so they sum to unity */
   if (total_vol > 0) {
      for (n=0; n<waiting_list_count; n++)
	 waiting_list[n].volume = MID(0.2, waiting_list[n].volume/total_vol, 1.0);
   }

   /* for each sample... */
   for (n=0; n<waiting_list_count; n++) {
      sample = waiting_list[n].sample;
      igen = waiting_list[n].igen;
      pgen = waiting_list[n].pgen;
      global_izone = waiting_list[n].global_izone;
      global_pzone = waiting_list[n].global_pzone;
      igen_count = waiting_list[n].igen_count;
      pgen_count = waiting_list[n].pgen_count;
      global_izone_count = waiting_list[n].global_izone_count;
      global_pzone_count = waiting_list[n].global_pzone_count;
      vol = waiting_list[n].volume;

      /* set default generator values */
      sf_instrument_look_index = -1; /* index into INST subchunk */
      sf_sample_look_index = -1;

      sf_start = sample->dwStart;
      sf_end = sample->dwEnd;
      sf_loop_start = sample->dwStartloop;
      sf_loop_end = sample->dwEndloop;
      sf_key = sample->byOriginalKey;
      sf_tune = sample->chCorrection;
      sf_sustain_mod_env = 0;
      sf_mod_env_to_pitch = 0;

      sf_delay_vol_env = -12000;
      sf_attack_vol_env = -12000;
      sf_hold_vol_env = -12000;
      sf_decay_vol_env = -12000;
      sf_release_vol_env = -12000;
      sf_sustain_level = 250;

      sf_delayModEnv = -12000;
      sf_attackModEnv = -12000;
      sf_holdModEnv = -12000;
      sf_decayModEnv = -12000;
      sf_releaseModEnv = -12000;

      sf_pan = 0;
      sf_keyscale = 100;
      sf_keymin = 0;
      sf_keymax = 127;
      sf_velmin = 0;
      sf_velmax = 127;
      sf_mode = 0;
      /* I added the following. (gl) */
      sf_instrument_unused5 = -1;
      sf_exclusiveClass = 0;
      sf_initialAttenuation = 0;
      sf_chorusEffectsSend = 0;
      sf_reverbEffectsSend = 0;
      sf_modLfoToPitch = 0;
      sf_vibLfoToPitch = 0;
      sf_keynum = -1;
      sf_velocity = -1;
      sf_keynumToModEnvHold = 0;
      sf_keynumToModEnvDecay = 0;
      sf_keynumToVolEnvHold = 0;
      sf_keynumToVolEnvDecay = 0;
      sf_modLfoToVolume = 0;
      sf_delayModLFO = 0;
      sf_freqModLFO = 0;
      sf_delayVibLFO = 0;
      sf_freqVibLFO = 0;
      sf_initialFilterQ = 0;
      sf_initialFilterFc = 0;
      sf_modEnvToFilterFc = 0;
      sf_modLfoToFilterFc = 0;

      sp_freq_center = 60;

      /* process the lists of generator data */
      for (i=0; i<global_izone_count; i++)
	 apply_generator(&global_izone[i], FALSE, TRUE);

      for (i=0; i<igen_count; i++)
	 apply_generator(&igen[i], FALSE, FALSE);

      for (i=0; i<global_pzone_count; i++)
	 apply_generator(&global_pzone[i], TRUE, TRUE);

      for (i=0; i<pgen_count; i++)
	 apply_generator(&pgen[i], TRUE, FALSE);

      /* convert SoundFont values into some more useful formats */
      length = sf_end - sf_start;

      if (length < 0) {
	      fprintf(stderr,"\nSample for %s has negative length.\n", name);
	      return FALSE;
      }
      sf_loop_start = MID(0, sf_loop_start-sf_start, sf_end);
      sf_loop_end = MID(0, sf_loop_end-sf_start, sf_end);

      /*sf_pan = MID(0, sf_pan*16/1000+7, 15);*/
      sf_pan = MID(0, sf_pan*256/1000+127, 255);

      if (sf_keyscale == 100) freq_scale = 1024;
      else freq_scale = MID(0, sf_keyscale*1024/100, 2048);

/* I don't know about this tuning. (gl) */
      //sf_tune += sf_mod_env_to_pitch * MID(0, 1000-sf_sustain_mod_env, 1000) / 1000;

      min_freq = freq_table[sf_keymin];
      max_freq = freq_table[sf_keymax];

      root_freq = calc_root_pitch();

      sustain = calc_sustain();
      sp_volume = calc_volume();

      if (sustain < 0) sustain = 0;
      if (sustain > sp_volume - 2) sustain = sp_volume - 2;

/*
	if (!lay->set[SF_releaseEnv2] && banknum < 128) release = 400;
	if (!lay->set[SF_decayEnv2] && banknum < 128) decay = 400;
*/
      delay = timecent2msec(sf_delay_vol_env);
      attack = timecent2msec(sf_attack_vol_env);
      hold = timecent2msec(sf_hold_vol_env);
      decay = timecent2msec(sf_decay_vol_env);
      release = timecent2msec(sf_release_vol_env);

      mod_sustain = calc_mod_sustain();
      mod_delay = timecent2msec(sf_delayModEnv);
      mod_attack = timecent2msec(sf_attackModEnv);
      mod_hold = timecent2msec(sf_holdModEnv);
      mod_decay = timecent2msec(sf_decayModEnv);
      mod_release = timecent2msec(sf_releaseModEnv);

      /* The output from this code is almost certainly not a 'correct'
       * .pat file. There are a lot of things I don't know about the
       * format, which have been filled in by guesses or values copied
       * from the Gravis files. And I have no idea what I'm supposed to
       * put in all the size fields, which are currently set to zero :-)
       *
       * But, the results are good enough for DIGMID to understand, and
       * the CONVERT program also seems quite happy to accept them, so
       * it is at least mostly correct...
       */
      sample = waiting_list[n].sample;

      mem_write8('s');                    /* sample name */
      mem_write8('m');
      mem_write8('p');
      mem_write8('0'+(n+1)/10);
      mem_write8('0'+(n+1)%10);
      if (waiting_list[n].stereo_mode == LEFT_SAMPLE)
	      mem_write8('L');
      else if (waiting_list[n].stereo_mode == RIGHT_SAMPLE)
	      mem_write8('R');
      else if (waiting_list[n].stereo_mode == MONO_SAMPLE)
	      mem_write8('M');
      else mem_write8('0'+waiting_list[n].stereo_mode);
      mem_write8(0);


      mem_write8(0);                      /* fractions */

      if (opt_8bit) {
	 mem_write32(length);             /* waveform size */
	 mem_write32(sf_loop_start);      /* loop start */
	 mem_write32(sf_loop_end);        /* loop end */
      }
      else {
	 mem_write32(length*2);           /* waveform size */
	 mem_write32(sf_loop_start*2);    /* loop start */
	 mem_write32(sf_loop_end*2);      /* loop end */
      }

      mem_write16(sample->dwSampleRate);  /* sample freq */

      mem_write32(min_freq);              /* low freq */
      mem_write32(max_freq);              /* high freq */
      mem_write32(root_freq);             /* root frequency */

      mem_write16(512);                   /* finetune */
      /*mem_write8(sf_pan);*/                 /* balance */
      mem_write8(7);	                 /* balance = middle */

/* if (wanted_bank != 128 && banknum == 0 && program == 56) debug_examples = -1; */
if (debug_examples < 0) {
	debug_examples++;
      printf("attack_vol_env=%d, hold_vol_env=%d, decay_vol_env=%d, release_vol_env=%d, sf_delay=%d\n",
		      sf_attack_vol_env, sf_hold_vol_env, sf_decay_vol_env, sf_release_vol_env,
		      sf_delay_vol_env);
	printf("iA= %d, sp_volume=%d sustain=%d attack=%d ATTACK=%d\n", sf_initialAttenuation,
		       	sp_volume, sustain, attack, msec2gus(attack, sp_volume));
	printf("\thold=%d r=%d HOLD=%d\n", hold, sp_volume-1, msec2gus(hold, sp_volume-1));
	printf("\tdecay=%d r=%d DECAY=%d\n", hold, sp_volume-1-sustain, msec2gus(hold, sp_volume-1-sustain));
	printf("\trelease=%d r=255 RELEASE=%d\n", release, msec2gus(release, 255));
	printf("  levels: %d %d %d %d\n", sp_volume, sp_volume-1, sustain, 0);
}

      mem_write8(msec2gus(attack, sp_volume));                   /* envelope rates */
      mem_write8(msec2gus(hold, sp_volume - 1));
      mem_write8(msec2gus(decay, sp_volume - 1 - sustain));
      mem_write8(msec2gus(release, 255));
      mem_write8(0x3F);
      mem_write8(0x3F);

      mem_write8(sp_volume);                    /* envelope offsets */
      mem_write8(sp_volume - 1);
      mem_write8(sustain);
      mem_write8(0);
      mem_write8(0);
      mem_write8(0);

      convert_tremolo();
      mem_write8(sp_tremolo_sweep_increment);    /* tremolo sweep */
      mem_write8(sp_tremolo_phase_increment);    /* tremolo rate */
      mem_write8(sp_tremolo_depth);              /* tremolo depth */

      convert_vibrato();
      mem_write8(sp_vibrato_sweep_increment);     /* vibrato sweep */
      mem_write8(sp_vibrato_control_ratio);      /* vibrato rate */
      mem_write8(sp_vibrato_depth);               /* vibrato depth */

#ifdef LFO_DEBUG
      convert_lfo(name, program, banknum, wanted_bank);
#else
      convert_lfo();
#endif

      flags = getmodes(sf_mode, program, wanted_bank);

      mem_write8(flags);                  /* write sample mode */

      /* The value for sp_freq_center was set in calc_root_pitch(). */
      mem_write16(sp_freq_center);
      mem_write16(freq_scale);           /* scale factor */

      if (opt_adjust_volume) {
	  if (opt_veryverbose) printf("vol comp %d", sp_volume);
          sample_volume = adjust_volume(sample->dwStart, length);
	  if (opt_veryverbose) printf(" -> %d\n", sample_volume);
      }
      else sample_volume = sp_volume;

      mem_write16(sample_volume); /* I'm not sure this is here. (gl) */

/* Begin SF2 extensions */
      mem_write8(delay);
      mem_write8(sf_exclusiveClass);
      mem_write8(sp_vibrato_delay);

      mem_write8(msec2gus(mod_attack, sp_volume));                   /* envelope rates */
      mem_write8(msec2gus(mod_hold, sp_volume - 1));
      mem_write8(msec2gus(mod_decay, sp_volume - 1 - mod_sustain));
      mem_write8(msec2gus(mod_release, 255));
      mem_write8(0x3F);
      mem_write8(0x3F);

      mem_write8(sp_volume);                    /* envelope offsets */
      mem_write8(sp_volume - 1);
      mem_write8(mod_sustain);
      mem_write8(0);
      mem_write8(0);
      mem_write8(0);

      mem_write8(sp_delayModLFO);

      mem_write8(sf_chorusEffectsSend);
      mem_write8(sf_reverbEffectsSend);

      calc_resonance();
      mem_write16(sp_resonance);

      calc_cutoff();
      mem_write16(sp_cutoff_freq);
/*
fprintf(stderr,"b=%2d p=%3d wb=%2d cutoff=%5d res=%3d\n", banknum,
		program, wanted_bank, sp_cutoff_freq, sp_resonance);
*/
      mem_write8(sp_modEnvToPitch);
      mem_write8(sp_modEnvToFilterFc);
      mem_write8(sp_modLfoToFilterFc);

      mem_write8(sf_keynumToModEnvHold);
      mem_write8(sf_keynumToModEnvDecay);
      mem_write8(sf_keynumToVolEnvHold);
      mem_write8(sf_keynumToVolEnvDecay);

      mem_write8(sf_pan);                 /* balance */

      mem_write16(sp_lfo_phase_increment);    /* lfo */
      mem_write8(sp_lfo_depth);

      if (sf_instrument_unused5 == -1)
	mem_write8(255);
      else mem_write8(sf_instrument_unused5);

#if 0
/* 36 (34?) bytes were reserved; now 1 left */
      for (i=35; i<36; i++)                /* reserved */
	 mem_write8(0);
#endif
      if (opt_8bit) {                     /* sample waveform */
	 for (i=0; i<length; i++)
	    mem_write8((int)((sf_sample_data[sample->dwStart+i] >> 8) * vol) ^ 0x80);
      }
      else {
	 for (i=0; i<length; i++)
	    mem_write16(sf_sample_data[sample->dwStart+i]);
      }
   }
   return TRUE;
}















extern char *optarg;
extern int optind, opterr, optopt;



int main(int argc, char *argv[])
{
	int i, c;
	char cfgname[80];
	char *inname;
	char *sep1, *sep2;

	memset(melody_velocity_override, -1, 128*128);
	memset(drum_velocity_override, -1, 128*128);

	while ((c = getopt (argc, argv, "FVvnsdmM:D:")) > 0)
		switch (c) {
			case 'v':
	    			opt_verbose = 1;
	    			break;
			case 'n':
	    			opt_no_write = 1;
	    			break;
			case 's':
	    			opt_small = 1;
	    			break;
			case 'd':
	    			opt_drum = 1;
	    			break;
			case 'm':
	    			opt_mono = 1;
	    			break;
			case 'F':
	    			opt_adjust_sample_flags = 1;
	    			break;
			case 'V':
	    			opt_adjust_volume = 0;
	    			break;
			case 'M':
				sep1 = strchr(optarg, ':');
				sep2 = strchr(optarg, '=');
				if (sep1 && sep2)
				{
				  melody_velocity_override[atoi(optarg)]
				    [atoi(sep1 + 1)] = atoi(sep2 + 1);
				  break;
				} /* if missing, fall through */
			case 'D':
				sep1 = strchr(optarg, ':');
				sep2 = strchr(optarg, '=');
				if (sep1 && sep2)
				{
				  drum_velocity_override[atoi(optarg)]
				    [atoi(sep1 + 1)] = atoi(sep2 + 1);
				  break;
				} /* if missing, fall through */
			default:
				fprintf(stderr, "usage: unsf [-v] [-n] [-s] [-d] [-m] [-F] [-V] [-M <bank>:<instrument>=<layer>]\n"
						"  [-D <bank>:<instrument>=<layer>] <filename>\n");
				return 1;
		}

	if (argc - optind != 1) {
		fprintf(stderr, "usage: unsf [-v] [-n] [-s] [-d] [-m] [-F] [-V] [-M <bank>:<instrument>=<layer>]\n"
		                "  [-D <bank>:<instrument>=<layer>] <filename>\n");
		exit(1);
	}


	inname = strrchr (argv [optind], '/');
	inname = inname ? inname + 1 : argv [optind];

	strcpy(basename, inname);
	inname = strrchr (basename, '.');
	if (inname) inname[0] = '\0';

	for (i = 0; i < strlen(basename); i++) {
		if (basename[i] == ' ') basename[i] = '_';
		else if (basename[i] == '#') basename[i] = '_';
	}

	strcpy(cfgname, basename);
	strcat(cfgname, ".cfg");
	if (!opt_no_write) {
		if ( !(cfg_fd = fopen(cfgname, "wb")) ) return 1;
	}

	opt_soundfont = argv[optind];


    typedef struct UnSF_Optionssss
    {
        int opt_8bit = 0;
        int opt_verbose = 0;
        int opt_veryverbose = 0;
        int opt_bank = 0;
        int opt_drum_bank = 0;
        int opt_header = 0;
        int opt_left_channel = 0;
        int opt_right_channel = 0;
        char *opt_soundfont = NULL;
        int opt_small = 0;
        int opt_drum = 0;
        int opt_mono = 0;
        int opt_no_write = 0;
        int opt_adjust_sample_flags = 0;
        int opt_adjust_volume = 1;
    } UnSF_Optionssss

    UnSF_Options options = { o, 0, 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, 0, 0 1};

	convert_sf_to_gus();

	if (!opt_no_write) fclose(cfg_fd);

	return 0;
}
