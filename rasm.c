#define PROGRAM_NAME      "RASM"
#define PROGRAM_VERSION   "0.106"
#define PROGRAM_DATE      "xx/01/2019"
#define PROGRAM_COPYRIGHT "� 2017 BERGE Edouard / roudoudou from Resistance"

#define RASM_VERSION PROGRAM_NAME" v"PROGRAM_VERSION

#define DEBUG_PREPRO 0

/***
Rasm (roudoudou assembler) Z80 assembler

doc & latest official release at: http://www.cpcwiki.eu/forum/programming/rasm-z80-assembler-in-beta/

You may send requests/bugs in the same topic

-----------------------------------------------------------------------------------------------------
This software is using MIT "expat" license

� Copyright � BERGE Edouard (roudoudou)

Permission  is  hereby  granted,  free  of charge,to any person obtaining a copy  of  this  software
and  associated  documentation/source   files   of RASM, to deal in the Software without restriction,
including without limitation the  rights  to  use, copy,   modify,   merge,   publish,    distribute,
sublicense,  and/or  sell  copies of the Software, and  to  permit  persons  to  whom the Software is
furnished  to  do  so,  subject  to  the following conditions:

The above copyright  notice  and  this  permission notice   shall   be  included  in  all  copies  or
substantial portions of the Software.
The   Software   is   provided  "as is",   without warranty   of   any   kind,  express  or  implied,
including  but  not  limited  to the warranties of merchantability,   fitness   for   a    particular
purpose  and  noninfringement.  In  no event shall the  authors  or  copyright  holders be liable for
any  claim, damages  or other  liability,  whether in  an  action  of  contract, tort  or  otherwise,
arising from,  out of  or in connection  with  the software  or  the  use  or  other  dealings in the
Software. �
-----------------------------------------------------------------------------------------------------
Linux compilation with GCC or Clang:
cc rasm_v0106.c -O2 -lm -lrt -march=native -o rasm
strip rasm

Windows compilation with Visual studio:
cl.exe rasm_v0106.c -O2

DOS (windows compatible) 32 bits compilation with Watcom:
wcl386 rasm_v0106.c -6r -6s -fp6 -d0 -k4000000 -ox /bt=DOS /l=dos4g

MorphOS compilation (ixemul):
ppc-morphos-gcc-5 -O2 -c -o rasm rasm_v0106.c
strip rasm
*/

#ifdef __WATCOMC__
#define OS_WIN 1
#endif

#ifdef _WIN32
#define OS_WIN 1
#endif

#ifdef _WIN64
#define OS_WIN 1
#endif

#ifndef RDD
	/* public lib */
	#include"minilib.h"
#else
	/* private dev lib wont be published */
	#include"../tools/library.h"
	#define TxtSplitWithChar _internal_TxtSplitWithChar
#endif

#ifndef NO_3RD_PARTIES
#define __FILENAME__ "3rd parties"
/* 3rd parties compression */
#include"zx7.h"
#include"lz4.h"
#include"exomizer.h"
#endif

#ifdef __MORPHOS__
/* Add standard version string to executable */
const char __attribute__((section(".text"))) ver_version[]={ "\0$VER: "PROGRAM_NAME" "PROGRAM_VERSION" ("PROGRAM_DATE") "PROGRAM_COPYRIGHT"" };
#endif

#undef __FILENAME__
#define __FILENAME__ "rasm.c"

/*******************************************************************
         c o m m a n d    l i n e    p a r a m e t e r s 
*******************************************************************/
enum e_dependencies_type {
E_DEPENDENCIES_NO=0,
E_DEPENDENCIES_LIST,
E_DEPENDENCIES_MAKE
};

struct s_parameter {
	char **labelfilename;
	char *filename;
	char *outputfilename;
	int export_local;
	int export_var;
	int export_equ;
	int export_sym;
	char *flexible_export;
	int export_sna;
	int export_snabrk;
	int export_brk;
	int verbose;
	int nowarning;
	int checkmode;
	int dependencies;
	int maxerr;
	int extended_error;
	int edskoverwrite;
	float rough;
	int as80,dams;
	int v2;
	char *symbol_name;
	char *binary_name;
	char *cartridge_name;
	char *snapshot_name;
	char *breakpoint_name;
	char **symboldef;
	int nsymb,msymb;
	char **pathdef;
	int npath,mpath;
};



/*******************************************************************
 c o m p u t e   o p e r a t i o n s   f o r   c a l c u l a t o r
*******************************************************************/

enum e_compute_operation_type {
E_COMPUTE_OPERATION_PUSH_DATASTC=0,
E_COMPUTE_OPERATION_OPEN=1,
E_COMPUTE_OPERATION_CLOSE=2,
E_COMPUTE_OPERATION_ADD=3,
E_COMPUTE_OPERATION_SUB=4,
E_COMPUTE_OPERATION_DIV=5,
E_COMPUTE_OPERATION_MUL=6,
E_COMPUTE_OPERATION_AND=7,
E_COMPUTE_OPERATION_OR=8,
E_COMPUTE_OPERATION_MOD=9,
E_COMPUTE_OPERATION_XOR=10,
E_COMPUTE_OPERATION_NOT=11,
E_COMPUTE_OPERATION_SHL=12,
E_COMPUTE_OPERATION_SHR=13,
E_COMPUTE_OPERATION_BAND=14,
E_COMPUTE_OPERATION_BOR=15,
E_COMPUTE_OPERATION_LOWER=16,
E_COMPUTE_OPERATION_GREATER=17,
E_COMPUTE_OPERATION_EQUAL=18,
E_COMPUTE_OPERATION_NOTEQUAL=19,
E_COMPUTE_OPERATION_LOWEREQ=20,
E_COMPUTE_OPERATION_GREATEREQ=21,
/* math functions */
E_COMPUTE_OPERATION_SIN=22,
E_COMPUTE_OPERATION_COS=23,
E_COMPUTE_OPERATION_INT=24,
E_COMPUTE_OPERATION_FLOOR=25,
E_COMPUTE_OPERATION_ABS=26,
E_COMPUTE_OPERATION_LN=27,
E_COMPUTE_OPERATION_LOG10=28,
E_COMPUTE_OPERATION_SQRT=29,
E_COMPUTE_OPERATION_ASIN=30,
E_COMPUTE_OPERATION_ACOS=31,
E_COMPUTE_OPERATION_ATAN=32,
E_COMPUTE_OPERATION_EXP=33,
E_COMPUTE_OPERATION_LOW=34,
E_COMPUTE_OPERATION_HIGH=35,
E_COMPUTE_OPERATION_PSG=36,
E_COMPUTE_OPERATION_RND=37,
E_COMPUTE_OPERATION_END=38
};

struct s_compute_element {
enum e_compute_operation_type operator;
double value;
int priority;
};

struct s_compute_core_data {
	/* evaluator v3 may be recursive */
	char *varbuffer;
	int maxivar;
	struct s_compute_element *tokenstack;
	int maxtokenstack;
	struct s_compute_element *operatorstack;
	int maxoperatorstack;
};

/***********************************************************
  w a v   h e a d e r    f o r    a u d i o    i m p o r t
***********************************************************/
struct s_wav_header {
unsigned char ChunkID[4];
unsigned char ChunkSize[4];
unsigned char Format[4];
unsigned char SubChunk1ID[4];
unsigned char SubChunk1Size[4];
unsigned char AudioFormat[2];
unsigned char NumChannels[2];
unsigned char SampleRate[4];
unsigned char ByteRate[4];
unsigned char BlockAlign[2];
unsigned char BitsPerSample[2];
unsigned char SubChunk2ID[4];
unsigned char SubChunk2Size[4];
};

enum e_audio_sample_type {
AUDIOSAMPLE_SMP,
AUDIOSAMPLE_SM2,
AUDIOSAMPLE_SM4,
AUDIOSAMPLE_DMA,
AUDIOSAMPLE_END
};

/***********************************************************************
  e x p r e s s i o n   t y p e s   f o r   d e l a y e d   w r i t e
***********************************************************************/
enum e_expression {
	E_EXPRESSION_J8,    /* relative 8bits jump */
	E_EXPRESSION_0V8,   /* 8 bits value to current adress */
	E_EXPRESSION_V8,    /* 8 bits value to current adress+1 */
	E_EXPRESSION_V16,   /* 16 bits value to current adress+1 */
	E_EXPRESSION_V16C,  /* 16 bits value to current adress+1 */
	E_EXPRESSION_0V16,  /* 16 bits value to current adress */
	E_EXPRESSION_0V32,  /* 32 bits value to current adress */
	E_EXPRESSION_0VR,   /* AMSDOS real value (5 bytes) to current adress */
	E_EXPRESSION_IV8,   /* 8 bits value to current adress+2 */
	E_EXPRESSION_IV81,  /* 8 bits value+1 to current adress+2 */
	E_EXPRESSION_3V8,   /* 8 bits value to current adress+3 used with LD (IX+n),n */
	E_EXPRESSION_IV16,  /* 16 bits value to current adress+2 */
	E_EXPRESSION_RST,   /* the offset of RST is translated to the opcode */
	E_EXPRESSION_IM     /* the interrupt mode is translated to the opcode */
};

struct s_expression {	
	char *reference;          /* backup when used inside loop (or macro?) */
	int iw;                   /* word index in the main wordlist */
	int o;                    /* offset de depart 0, 1 ou 3 selon l'opcode */
	int ptr;                  /* offset courant pour calculs relatifs */
	int wptr;                 /* where to write the result  */
	enum e_expression zetype; /* type of delayed write */
	int lz;                   /* lz zone */
	int ibank;                /* ibank of expression */
	int iorgzone;             /* org of expression */
};

struct s_expr_dico {
	char *name;
	int crc;
	double v;
};

struct s_label {
	char *name;   /* is alloced for local repeat or struct -> in this case iw=-1 */
	int iw;       /* index of the word of label name */
	int crc;      /* crc of the label name */
	int ptr;      /* "physical" adress */
	int lz;       /* is the label in a crunched section (or after)? */
	int iorgzone; /* org of label */
	int ibank;    /* current CPR bank / always zero in classic mode */
	/* errmsg */
	int fileidx;
	int fileline;
};

struct s_alias {
	char *alias;
	char *translation;
	int crc,len;
};

struct s_ticker {
	char *varname;
	int crc;
	long nopstart;
	long tickerstart;
};

/***********************************************************************
   m e r k e l    t r e e s    f o r    l a b e l,  v a r,  a l i a s
***********************************************************************/
struct s_crclabel_tree {
	struct s_crclabel_tree *radix[256];
	struct s_label *label;
	int nlabel,mlabel;
};
struct s_crcdico_tree {
	struct s_crcdico_tree *radix[256];
	struct s_expr_dico *dico;
	int ndico,mdico;
};
struct s_crcused_tree {
	struct s_crcused_tree *radix[256];
	char **used;
	int nused,mused;
};
struct s_crcstring_tree {
	struct s_crcstring_tree *radix[256];
	char **text;
	int ntext,mtext;
	char **replace;
	int nreplace,mreplace;
};
/*************************************************
          m e m o r y    s e c t i o n
*************************************************/
struct s_lz_section {
	int iw;
	int memstart,memend;
	int lzversion; /* 4 -> LZ4 / 7 -> ZX7 / 48 -> LZ48 / 49 -> LZ49 / 8 -> Exomizer */
	int iorgzone;
	int ibank;
	/* idx backup */
	int iexpr;
	int ilabel;
};

struct s_orgzone {
	int ibank,protect;
	int memstart,memend;
	int ifile,iline;
	int nocode;
};

/**************************************************
         i n c b i n     s t o r a g e
**************************************************/
struct s_hexbin {
	unsigned char *data;
	int datalen,rawlen;
	char *filename;
};

/**************************************************
          e d s k    m a n a g e m e n t        
**************************************************/
struct s_edsk_sector_global_struct {
unsigned char track;
unsigned char side;
unsigned char id;
unsigned char size;
unsigned char st1;
unsigned char st2;
unsigned short int length;
unsigned char *data;
};

struct s_edsk_track_global_struct  {
int sectornumber;
/* information purpose */
int sectorsize;
int gap3length;
int fillerbyte;
int datarate;
int recordingmode;
struct s_edsk_sector_global_struct *sector;
};

struct s_edsk_global_struct {
int tracknumber;
int sidenumber;
int tracksize; /* DSK legacy */
struct s_edsk_track_global_struct *track;
};

struct s_edsk_wrapper_entry {
unsigned char user;
unsigned char filename[11];
unsigned char subcpt;
unsigned char extendcounter;
unsigned char reserved;
unsigned char rc;
unsigned char blocks[16];
};

struct s_edsk_wrapper {
char *edsk_filename;
struct s_edsk_wrapper_entry entry[64];
int nbentry;
unsigned char blocks[178][1024]; /* DATA format */
int face;
};

struct s_save {
	int ibank;
	int ioffset;
	int isize;
	int iw;
	int amsdos;
	int dsk,face,iwdskname;
};


/********************
      L O O P S
********************/

enum e_loop_style {
E_LOOPSTYLE_REPEATN,
E_LOOPSTYLE_REPEATUNTIL,
E_LOOPSTYLE_WHILE
};

struct s_repeat {
	int start;
	int cpt;
	int value;
	int maxim;
	int repeat_counter;
	char *repeatvar;
	int repeatcrc;
};

struct s_whilewend {
	int start;
	int cpt;
	int value;
	int maxim;
	int while_counter;
};

struct s_switchcase {
	int refval;
	int execute;
	int casematch;
};

struct s_repeat_index {
	int ifile;
	int ol,oidx;
	int cl,cidx;
};


enum e_ifthen_type {
E_IFTHEN_TYPE_IF=0,
E_IFTHEN_TYPE_IFNOT=1,
E_IFTHEN_TYPE_IFDEF=2,
E_IFTHEN_TYPE_IFNDEF=3,
E_IFTHEN_TYPE_ELSE=4,
E_IFTHEN_TYPE_ELSEIF=5,
E_IFTHEN_TYPE_IFUSED=6,
E_IFTHEN_TYPE_IFNUSED=7,
E_IFTHEN_TYPE_END
};

struct s_ifthen {
	char *filename;
	int line,v;
	enum e_ifthen_type type;
};

/**************************************************
          w o r d    p r o c e s s i n g
**************************************************/
struct s_wordlist {
	char *w;
	int l,t,e; /* e=1 si egalite dans le mot */
	int ifile;
};

struct s_macro {
	char *mnemo;
	int crc;
	/* une macro concatene des chaines et des parametres */
	struct s_wordlist *wc;
	int nbword,maxword;
	/**/
	char **param;
	int nbparam;
};

struct s_macro_position {
	int start,end,value;
};

/* preprocessing only */
struct s_macro_fast {
	char *mnemo;
	int crc;
};

struct s_math_keyword {
	char *mnemo;
	int crc;
	enum e_compute_operation_type operation;
};

struct s_expr_word {
	char *w;
	int aw;
	int op;
	int comma;
	int fct;
	double v;
};

struct s_listing {
	char *listing;
	int ifile;
	int iline;
};

enum e_tagtranslateoption {
E_TAGOPTION_NONE=0,
E_TAGOPTION_REMOVESPACE
};

#ifdef RASM_THREAD
struct s_rasm_thread {
	pthread_t thread;
	int lz;
	unsigned char *datain;
	int datalen;
	unsigned char *dataout;
	int lenout;
	int status;
};
#endif


/*********************************************************
            S N A P S H O T     E X P O R T
*********************************************************/
struct s_snapshot_symbol {
	unsigned char size;
	unsigned char name[256];
	unsigned char reserved[6];
	unsigned char bigendian_adress[2];
};

struct s_snapshot {
	char idmark[8];
	char unused1[8];
	unsigned char version; /* 3 */
	struct {
		struct {
			unsigned char F;
			unsigned char A;
			unsigned char C;
			unsigned char B;
			unsigned char E;
			unsigned char D;
			unsigned char L;
			unsigned char H;
		}general;
		unsigned char R;
		unsigned char regI; /* I incompatible with tgmath.h */
		unsigned char IFF0;
		unsigned char IFF1;
		unsigned char LX;
		unsigned char HX;
		unsigned char LY;
		unsigned char HY;
		unsigned char LSP;
		unsigned char HSP;
		unsigned char LPC;
		unsigned char HPC;
		unsigned char IM; /* 0,1,2 */
		struct {
			unsigned char F;
			unsigned char A;
			unsigned char C;
			unsigned char B;
			unsigned char E;
			unsigned char D;
			unsigned char L;
			unsigned char H;
		}alternate;
	}registers;
		
	struct {
		unsigned char selectedpen;
		unsigned char palette[17];
		unsigned char multiconfiguration;
	}gatearray;
	unsigned char ramconfiguration;
	struct {
		unsigned char selectedregister;
		unsigned char registervalue[18];
	}crtc;
	unsigned char romselect;
	struct {
		unsigned char portA;
		unsigned char portB;
		unsigned char portC;
		unsigned char control;
	}ppi;
	struct {
		unsigned char selectedregister;
		unsigned char registervalue[16];
	}psg;
	unsigned char dumpsize[2]; /* 64 then use extended memory chunks */
	
	unsigned char CPCType; /* 0=464 / 1=664 / 2=6128 / 4=6128+ / 5=464+ / 6=GX4000 */
	unsigned char interruptnumber;
	unsigned char multimodebytes[6];
	unsigned char unused2[0x9C-0x75];
	
	/* offset #9C */
	struct {
		unsigned char motorstate;
		unsigned char physicaltrack;
	}fdd;
	unsigned char unused3[3];
	unsigned char printerstrobe;
	unsigned char unused4[2];
	struct {
		unsigned char model; /* 0->4 */
		unsigned char unused5[4];
		unsigned char HCC;
		unsigned char unused;
		unsigned char CLC;
		unsigned char RLC;
		unsigned char VTC;
		unsigned char HSC;
		unsigned char VSC;
		unsigned short int flags;
	}crtcstate;
	unsigned char vsyncdelay;
	unsigned char interruptscanlinecounter;
	unsigned char interruptrequestflag;
	unsigned char unused6[0xFF-0xB5+1];
};

struct s_snapshot_chunks {
	unsigned char chunkname[4]; /* MEM1 -> MEM8 */
	unsigned int chunksize;
};

struct s_breakpoint {
	int address;
	int bank;
};


/*********************************
        S T R U C T U R E S
*********************************/
struct s_rasmstructfield {
	char *fullname;
	char *name;
	int offset;
	int size;
	int crc;
};

struct s_rasmstruct {
	char *name;
	int crc;
	int size;
	/* fields */
	struct s_rasmstructfield *rasmstructfield;
	int irasmstructfield,mrasmstructfield;
};

/*******************************************
        G L O B A L     S T R U C T
*******************************************/
struct s_assenv {
	/* current memory */
	int maxptr;
	/* CPR memory */
	unsigned char **mem;
	int iwnamebank[35];
	int nbbank,maxbank;
	int forcecpr,bankmode,activebank,amsdos,forcesnapshot,packedbank;
	struct s_snapshot snapshot;
	int bankset[9];   /* 64K selected flag */
	int bankused[35]; /* 16K selected flag */
	int bankgate[36];
	int rundefined;
	/* parsing */
	struct s_wordlist *wl;
	int nbword;
	int idx,stage;
	char *label_filename;
	int label_line;
	char **filename;
	int ifile,maxfile;
	int nberr,flux,verbose;
	int fastmatch[256];
	unsigned char charset[256];
	int maxerr,extended_error,nowarning;
	/* ORG tracking */
	int codeadr,outputadr,nocode;
	int minadr,maxadr;
	struct s_orgzone *orgzone;
	int io,mo;
	/* Struct */
	struct s_rasmstruct *rasmstruct;
	int irasmstruct,mrasmstruct;
	int getstruct;
	int backup_outputadr,backup_codeadr;
	char *backup_filename;
	int backup_line;
	struct s_rasmstruct *rasmstructalias;
	int irasmstructalias,mrasmstructalias;
	/* expressions */
	struct s_expression *expression;
	int ie,me;
	int maxam,as80,dams;
	float rough;
	struct s_compute_core_data *computectx,ctx1,ctx2;
	struct s_crcstring_tree stringtree;
	/* label */
	struct s_label *label;
	int il,ml;
	struct s_crclabel_tree labeltree; /* fast label access */
	char *module;
	struct s_breakpoint *breakpoint;
	int ibreakpoint,maxbreakpoint;
	char *lastgloballabel;
	int lastgloballabellen;
	/* repeat */
	struct s_repeat *repeat;
	int ir,mr;
	/* while/wend */
	struct s_whilewend *whilewend;
	int iw,mw;
	/* if/then/else */
	//int *ifthen;
	struct s_ifthen *ifthen;
	int ii,mi;
	/* switch/case */
	struct s_switchcase *switchcase;
	int isw,msw;
	/* expression dictionnary */
	struct s_expr_dico *dico;
	int idic,mdic;
	struct s_crcdico_tree dicotree; /* fast dico access */
	struct s_crcused_tree usedtree; /* fast used access */
	/* ticker */
	struct s_ticker *ticker;
	int iticker,mticker;
	long tick,nop;
	/* crunch section flag */
	struct s_lz_section *lzsection;
	int ilz,mlz;
	int lz,curlz;
	/* macro */
	struct s_macro *macro;
	int imacro,mmacro;
	/* labels locaux */
	int repeatcounter,whilecounter,macrocounter;
	struct s_macro_position *macropos;
	int imacropos,mmacropos;
	/* alias */
	struct s_alias *alias;
	int ialias,malias;
	/* hexbin */
	struct s_rasm_thread **rasm_thread;
	int irt,mrt;
	struct s_hexbin *hexbin;
	int ih,mh;
	char **includepath;
	int ipath,mpath;
	/* automates */
	char AutomateExpressionValidCharExtended[256];
	char AutomateExpressionValidCharFirst[256];
	char AutomateExpressionValidChar[256];
	char AutomateExpressionDecision[256];
	char AutomateValidLabelFirst[256];
	char AutomateValidLabel[256];
	char AutomateDigit[256];
	char AutomateHexa[256];
	struct s_compute_element AutomateElement[256];
	unsigned char psgtab[256];
	unsigned char psgfine[256];
	/* output */
	char *outputfilename;
	int export_sym,export_local;
	int export_var,export_equ;
	int export_sna,export_snabrk;
	int export_brk;
	char *flexible_export;
	char *breakpoint_name;
	char *symbol_name;
	char *binary_name;
	char *cartridge_name;
	char *snapshot_name;
	struct s_save *save;
	int nbsave,maxsave;
	struct s_edsk_wrapper *edsk_wrapper;
	int nbedskwrapper,maxedskwrapper;
	int edskoverwrite;
	int checkmode,dependencies;
	int stop;
};

struct s_asm_keyword {
	char *mnemo;
	int crc;
	void (*makemnemo)(struct s_assenv *ae);
};

struct s_math_keyword math_keyword[]={
{"SIN",0,E_COMPUTE_OPERATION_SIN},
{"COS",0,E_COMPUTE_OPERATION_COS},
{"INT",0,E_COMPUTE_OPERATION_INT},
{"ABS",0,E_COMPUTE_OPERATION_ABS},
{"LN",0,E_COMPUTE_OPERATION_LN},
{"LOG10",0,E_COMPUTE_OPERATION_LOG10},
{"SQRT",0,E_COMPUTE_OPERATION_SQRT},
{"FLOOR",0,E_COMPUTE_OPERATION_FLOOR},
{"ASIN",0,E_COMPUTE_OPERATION_ASIN},
{"ACOS",0,E_COMPUTE_OPERATION_ACOS},
{"ATAN",0,E_COMPUTE_OPERATION_ATAN},
{"EXP",0,E_COMPUTE_OPERATION_EXP},
{"LO",0,E_COMPUTE_OPERATION_LOW},
{"HI",0,E_COMPUTE_OPERATION_HIGH},
{"PSGVALUE",0,E_COMPUTE_OPERATION_PSG},
{"RND",0,E_COMPUTE_OPERATION_RND},
{"",0,-1}
};

#define CRC_SWITCH    0x01AEDE4A
#define CRC_CASE      0x0826B794
#define CRC_DEFAULT   0x9A0DAC7D
#define CRC_BREAK     0xCD364DDD
#define CRC_ENDSWITCH 0x18E9FB21

#define CRC_ELSEIF 0xE175E230
#define CRC_ELSE   0x3FF177A1
#define CRC_ENDIF  0xCD5265DE
#define CRC_IF     0x4BD52507
#define CRC_IFDEF  0x4CB29DD6
#define CRC_UNDEF  0xCCD2FDEA
#define CRC_IFNDEF 0xD9AD0824
#define CRC_IFNOT  0x4CCAC9F8
#define CRC_WHILE  0xBC268FF1
#define CRC_UNTIL  0xCC12A604
#define CRC_MEND   0xFFFD899C
#define CRC_ENDM   0x3FF9559C
#define CRC_MACRO  0x64AA85EA
#define CRC_IFUSED 0x91752638
#define CRC_IFNUSED 0x1B39A886

#define CRC_SIN 0xE1B71962
#define CRC_COS 0xE077C55D

#define CRC_0    0x7A98A6A8
#define CRC_1    0x7A98A6A9
#define CRC_2    0x7A98A6AA


#define CRC_NC   0x4BD52B09
#define CRC_Z    0x7A98A6D2
#define CRC_NZ   0x4BD52B20
#define CRC_P    0x7A98A6C8
#define CRC_PO   0x4BD53717
#define CRC_PE   0x4BD5370D
#define CRC_M    0x7A98A6C5

/* 8 bits registers */
#define CRC_F    0x7A98A6BE
#define CRC_I    0x7A98A6C1
#define CRC_R    0x7A98A6CA
#define CRC_A    0x7A98A6B9
#define CRC_B    0x7A98A6BA
#define CRC_C    0x7A98A6BB
#define CRC_D    0x7A98A6BC
#define CRC_E    0x7A98A6BD
#define CRC_H    0x7A98A6C0
#define CRC_L    0x7A98A6C4
/* dual naming */
#define CRC_XH   0x4BD50718
#define CRC_XL   0x4BD5071C
#define CRC_YH   0x4BD50519
#define CRC_YL   0x4BD5051D
#define CRC_HX   0x4BD52718
#define CRC_LX   0x4BD52F1C
#define CRC_HY   0x4BD52719
#define CRC_LY   0x4BD52F1D
#define CRC_IXL  0xE19F1765
#define CRC_IXH  0xE19F1761
#define CRC_IYL  0xE19F1166
#define CRC_IYH  0xE19F1162

/* 16 bits registers */
#define CRC_BC   0x4BD5D2FD
#define CRC_DE   0x4BD5DF01
#define CRC_HL   0x4BD5270C
#define CRC_IX   0x4BD52519
#define CRC_IY   0x4BD5251A
#define CRC_SP   0x4BD5311B
#define CRC_AF   0x4BD5D4FF
/* memory convention */
#define CRC_MHL  0xD0765F5D
#define CRC_MDE  0xD0467D52
#define CRC_MBC  0xD05E694E
#define CRC_MIX  0xD072B76A
#define CRC_MIY  0xD072B16B
#define CRC_MSP  0xD01A876C
#define CRC_MC   0xE018210C
/* struct parsing */
#define CRC_DEFB	0x37D15389
#define CRC_DB		0x4BD5DEFE
#define CRC_DEFW	0x37D1539E
#define CRC_DW		0x4BD5DF13
#define CRC_DEFI	0x37D15390
#define CRC_DEFS	0x37D1539A
#define CRC_DS		0x4BD5DF0F
#define CRC_DEFR	0x37D15399
#define CRC_DR		0x4BD5DF0E



/*
# base=16
% base=2
0-9 base=10
A-Z variable ou fonction (cos, sin, tan, sqr, pow, mod, and, xor, mod, ...)
+*-/&^m| operateur
*/

#define AutomateExpressionValidCharExtendedDefinition "0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZ_{}@+-*/~^$#%�<=>|&"
#define AutomateExpressionValidCharFirstDefinition "#%0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZ_@${"
#define AutomateExpressionValidCharDefinition "0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZ_{}@$"
#define AutomateValidLabelFirstDefinition ".ABCDEFGHIJKLMNOPQRSTUVWXYZ_@"
#define AutomateValidLabelDefinition "0123456789.ABCDEFGHIJKLMNOPQRSTUVWXYZ_@{}"
#define AutomateDigitDefinition ".0123456789"
#define AutomateHexaDefinition "0123456789ABCDEF"

#ifndef NO_3RD_PARTIES
unsigned char *LZ4_crunch(unsigned char *data, int zelen, int *retlen){
	unsigned char *lzdest=NULL;
	lzdest=MemMalloc(65536);
	*retlen=LZ4_compress_HC((char*)data,(char*)lzdest,zelen,65536,9);
	return lzdest;
}
#endif
unsigned char *LZ48_encode_legacy(unsigned char *data, int length, int *retlength);
#define LZ48_crunch LZ48_encode_legacy
unsigned char *LZ49_encode_legacy(unsigned char *data, int length, int *retlength);
#define LZ49_crunch LZ49_encode_legacy


/*
 * optimised reading of text file in one shot
 */
unsigned char *_internal_readbinaryfile(char *filename, int *filelength)
{
        #undef FUNC
        #define FUNC "_internal_readbinaryfile"

        unsigned char *binary_data=NULL;

        *filelength=FileGetSize(filename);
        binary_data=MemMalloc((*filelength)+1);
        /* we try to read one byte more to close the file just after the read func */
        if (FileReadBinary(filename,(char*)binary_data,(*filelength)+1)!=*filelength) {
                logerr("Cannot fully read %s",filename);
                exit(INTERNAL_ERROR);
        }
        return binary_data;
}
char **_internal_readtextfile(char *filename, char replacechar)
{
        #undef FUNC
        #define FUNC "_internal_readtextfile"

        char **lines_buffer=NULL;
        unsigned char *bigbuffer;
        int nb_lines=0,max_lines=0,i=0,e=0;
        int file_size;

        bigbuffer=_internal_readbinaryfile(filename,&file_size);

        while (i<file_size) {
                while (e<file_size && bigbuffer[e]!=0x0A) {
                        /* Windows de meeeeeeeerrrdde... */
                        if (bigbuffer[e]==0x0D) bigbuffer[e]=replacechar;
                        e++;
                }
                if (e<file_size) e++;
                if (nb_lines>=max_lines) {
                        max_lines=max_lines*2+10;
                        lines_buffer=MemRealloc(lines_buffer,(max_lines+1)*sizeof(char **));
                }
                lines_buffer[nb_lines]=MemMalloc(e-i+1);
                memcpy(lines_buffer[nb_lines],bigbuffer+i,e-i);
                lines_buffer[nb_lines][e-i]=0;
                if (0)
                {
                        int yy;
                        for (yy=0;lines_buffer[nb_lines][yy];yy++) {
                                if (lines_buffer[nb_lines][yy]>31) printf("%c",lines_buffer[nb_lines][yy]); else printf("(0x%X)",lines_buffer[nb_lines][yy]);
                        }
                        printf("\n");
                }
                nb_lines++;
                i=e;
        }
        if (!max_lines) {
                lines_buffer=MemMalloc(sizeof(char**));
                lines_buffer[0]=NULL;
        } else {
                lines_buffer[nb_lines]=NULL;
        }
        MemFree(bigbuffer);
        return lines_buffer;
}

#define FileReadLines(filename) _internal_readtextfile(filename,':')
#define FileReadLinesRAW(filename) _internal_readtextfile(filename,0x0D)
#define FileReadContent(filename,filesize) _internal_readbinaryfile(filename,filesize)


/***
	TxtReplace
	
	input:
	in_str:     string where replace will occur
	in_substr:  substring to look for
	out_substr: replace substring
	recurse:    loop until no in_substr is found
	
	note: in_str MUST BE previously mallocated if out_substr is bigger than in_substr
*/
#ifndef RDD
char *TxtReplace(char *in_str, char *in_substr, char *out_substr, int recurse)
{
	#undef FUNC
	#define FUNC "TxtReplace"
	
	char *str_look,*m1,*m2;
	char *out_str;
	int sl,l1,l2,dif,cpt;

	if (in_str==NULL)
		return NULL;
		
	sl=strlen(in_str);
	l1=strlen(in_substr);
	/* empty string, nothing to do except return empty string */
	if (!sl || !l1)
		return in_str;
		
	l2=strlen(out_substr);
	dif=l2-l1;
		
	/* replace string is small or equal in size, we dont realloc */
	if (dif<=0)
	{
		/* we loop while there is a replace to do */
		str_look=strstr(in_str,in_substr);
		while (str_look!=NULL)
		{
			/*logdebug(str_look);*/
			
			/* we copy the new string if his len is not null */
			if (l2)
				memcpy(str_look,out_substr,l2);
			/* only if len are different */
			if (l1!=l2)
			{
				/* we move the end of the string byte per byte
				   because memory locations overlap. This is
				   faster than memmove */
				m1=str_look+l1;
				m2=str_look+l2;
				while (*m1!=0)
				{
					*m2=*m1;
					m1++;m2++;
				}
				/* we must copy the EOL */
				*m2=*m1;
			}
			/* look for next replace */
			if (!recurse)
				str_look=strstr(str_look+l2,in_substr);
			else
				str_look=strstr(in_str,in_substr);
		}
		out_str=in_str;
	}
	else
	{
		/* we need to count each replace */
		cpt=0;
		str_look=strstr(in_str,in_substr);
		while (str_look!=NULL)
		{
			cpt++;
			str_look=strstr(str_look+l1,in_substr);
		}
		/* is there anything to do? */
		if (cpt)
		{
			/* we realloc to a size that will fit all replaces */
			out_str=MemRealloc(in_str,sl+1+dif*cpt);
			str_look=strstr(out_str,in_substr);
			while (str_look!=NULL && cpt)
			{
				/* as the replace string is bigger we
				   have to move memory first from the end */
				m1=out_str+sl;
				m2=m1+dif;
				sl+=dif;
				while (m1!=str_look+l1-dif)
				{
					*m2=*m1;
					m1--;m2--;
				}
				/* then we copy the replace string (can't be NULL in this case) */
				memcpy(str_look,out_substr,l2);
				
				/* look for next replace */
				if (!recurse)
					str_look=strstr(str_look+l2,in_substr);
				else
					str_look=strstr(in_str,in_substr);
					
				/* to prevent from naughty overlap */
				cpt--;
			}
			if (str_look!=NULL)
			{
				printf("INTERNAL ERROR - overlapping replace string (%s/%s), you can't use this one!\n",in_substr,out_substr);
				exit(ABORT_ERROR);
			}
		}
		else
			out_str=in_str;
	}
	return out_str;
}
#endif

#ifndef min
#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#endif

/* Levenshtein implementation by TheRayTracer https://gist.github.com/TheRayTracer/2644387 */
int _internal_LevenshteinDistance(char *s,  char *t)
{
	int i,j,n,m,*d;
	int im,jn;
	int r;
	
   n=strlen(s)+1;
   m=strlen(t)+1;
   d=malloc(n*m*sizeof(int));
   memset(d, 0, sizeof(int) * n * m);

   for (i = 1, im = 0; i < m; i++, im++)
   {
      for (j = 1, jn = 0; j < n; j++, jn++)
      {
         if (s[jn] == t[im])
         {
            d[(i * n) + j] = d[((i - 1) * n) + (j - 1)];
         }
         else
         {
            d[(i * n) + j] = min(d[(i - 1) * n + j] + 1, /* A deletion. */
                                 min(d[i * n + (j - 1)] + 1, /* An insertion. */
                                     d[(i - 1) * n + (j - 1)] + 1)); /* A substitution. */
         }
      }
   }
   r = d[n * m - 1];
   free(d);
   return r;
}

#ifdef RASM_THREAD
/*
 threads used for crunching
*/
void _internal_ExecuteThreads(struct s_assenv *ae,struct s_rasm_thread *rasm_thread, void *(*fct)(void *))
{
	#undef FUNC
	#define FUNC "_internal_ExecuteThreads"

	pthread_attr_t attr;
	void *status;
	int rc;
	/* launch threads */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	pthread_attr_setstacksize(&attr,65536);

	if ((rc=pthread_create(&image_threads[i].thread,&attr,fct,(void *)rasm_thread))) {
		rasm_printf(ae,"FATAL ERROR - Cannot create thread!\n");
		exit(INTERNAL_ERROR);
	}
}
void _internal_WaitForThreads(struct s_assenv *ae,struct s_rasm_thread *rasm_thread)
{
	#undef FUNC
	#define FUNC "_internal_WaitForThreads"
	int rc;
	
	if ((rc=pthread_join(rasm_thread->thread,&status))) {
		rasm_printf(ae,"FATAL ERROR - Cannot wait for thread\n");
		exit(INTERNAL_ERROR);
	}
}
void PushCrunchedFile(struct s_assenv *ae, unsigned char *datain, int datalen, int lz)
{
	#undef FUNC
	#define FUNC "PushCrunchedFile"
	
	struct s_rasm_thread *rasm_thread;
	
	rasm_thread=MemMalloc(sizeof(struct s_rasm_thread));
	memset(rasm_thread,0,sizeof(struct s_rasm_thread));
	rasm_thread->datain=datain;
	rasm_thread->datalen=datalen;
	rasm_thread->lz=lz;
	_internal_ExecuteThreads(ae,rasm_thread, void *(*fct)(void *));
	ObjectArrayAddDynamicValueConcat((void**)&ae->rasm_thread,&ae->irt,&ae->mrt,&rasm_thread,sizeof(struct s_rasm_thread *));
}
void PopAllCrunchedFiles(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "PopAllCrunchedFiles"
	
	int i;
	for (i=0;i<ae->irt;i++) {
		_internal_WaitForThreads(ae,ae->rasm_thread[i]);
	}
}
#endif


void rasm_printf(struct s_assenv *ae, ...) {
	#undef FUNC
	#define FUNC "rasm_printf"
	
	char *format;
	va_list argptr;

	if (!ae->flux && !ae->dependencies) {
		va_start(argptr,ae);
		format=va_arg(argptr,char *);
		vfprintf(stdout,format,argptr);	
		va_end(argptr);
	}
}
/***
	build the string of current line for error messages
*/
char *rasm_getline(struct s_assenv *ae, int offset) {
	#undef FUNC
	#define FUNC "rasm_getline"
	
	static char myline[40]={0};
	int idx=0,icopy,first=1;

	while (!ae->wl[ae->idx+offset].t && idx<32) {
		for (icopy=0;idx<32 && ae->wl[ae->idx+offset].w[icopy];icopy++) {
			myline[idx++]=ae->wl[ae->idx+offset].w[icopy];
		}
		if (!first) myline[idx++]=','; else first=0;
		offset++;
	}
	if (idx>=32) {
		strcpy(myline+29,"...");
	} else {
		myline[idx++]=0;
	}
	
	return myline;
}

char *SimplifyPath(char *filename) {
	#undef FUNC
	#define FUNC "SimplifyPath"

	return filename;
#if 0	
	char *pos,*repos;
	int i,len;

	char *rpath;

	rpath=realpath(filename,NULL);
	if (!rpath) {
		printf("rpath error!\n");
		switch (errno) {
			case EACCES:printf("read permission failure\n");break;
			case EINVAL:printf("wrong argument\n");break;
			case EIO:printf("I/O error\n");break;
			case ELOOP:printf("too many symbolic links\n");break;
			case ENAMETOOLONG:printf("names too long\n");break;
			case ENOENT:printf("names does not exists\n");break;
			case ENOMEM:printf("out of memory\n");break;
			case ENOTDIR:printf("a component of the path is not a directory\n");break;
			default:printf("unknown error\n");break;
		}
		exit(1);
	}
	if (strlen(rpath)<strlen(filename)) {
		strcpy(filename,rpath);
	}
	free(rpath);
	return filename;
	
#ifdef OS_WIN
	while ((pos=strstr(filename,"\\..\\"))!=NULL) {
		repos=pos-1;
		/* sequence found, looking back for '\' */
		while (repos>=filename) {
			if (*repos=='\\') {
				break;
			}
			repos--;
		}
		repos++;
		if (repos>=filename && repos!=pos) {
			len=strlen(pos)-4+1;
			pos+=4;
			for (i=0;i<len;i++) {
				*repos=*pos;
				repos++;
				pos++;
			}
		}
		if (strncmp(filename,".\\..\\",5)==0) {
			repos=filename;
			pos=repos+2;
			for (;*repos;pos++,repos++) {
				*repos=*pos;
			}
			*repos=0;
		}
	}
#else
printf("*************\nfilename=[%s]\n",filename);
	while ((pos=strstr(filename,"/../"))!=NULL) {
		repos=pos-1;
		while (repos>=filename) {
			if (*repos=='/') {
				break;
			}
			repos--;
		}
		repos++;
		if (repos>=filename && repos!=pos) {
			len=strlen(pos)-4+1;
			pos+=4;
			for (i=0;i<len;i++) {
				*repos=*pos;
				repos++;
				pos++;
			}
		}
printf("filename=[%s]\n",filename);
		if (strncmp(filename,"./../",5)==0) {
			repos=filename;
			pos=repos+2;
			for (;*repos;pos++,repos++) {
				*repos=*pos;
			}
			*repos=0;
		}
printf("filename=[%s]\n",filename);
	}
#endif
	return NULL;
#endif

}

char *GetPath(char *filename) {
	#undef FUNC
	#define FUNC "GetPath"

	static char curpath[PATH_MAX];
	int zelen,idx;

	zelen=strlen(filename);

#ifdef OS_WIN
	#define CURRENT_DIR ".\\"

	TxtReplace(filename,"/","\\",1);
	idx=zelen-1;
	while (idx>=0 && filename[idx]!='\\') idx--;
	if (idx<0) {
		/* pas de chemin */
		strcpy(curpath,".\\");
	} else {
		/* chemin trouve */
		strcpy(curpath,filename);
		curpath[idx+1]=0;
	}
#else
#ifdef __MORPHOS__
	#define CURRENT_DIR ""
#else
	#define CURRENT_DIR "./"
#endif
	idx=zelen-1;
	while (idx>=0 && filename[idx]!='/') idx--;
	if (idx<0) {
		/* pas de chemin */
		strcpy(curpath,CURRENT_DIR);
	} else {
		/* chemin trouve */
		strcpy(curpath,filename);
		curpath[idx+1]=0;
	}
#endif

	return curpath;
}
char *MergePath(struct s_assenv *ae,char *dadfilename, char *filename) {
	#undef FUNC
	#define FUNC "MergePath"

	static char curpath[PATH_MAX];
	int zelen;

	zelen=strlen(filename);
#ifdef OS_WIN
	TxtReplace(filename,"/","\\",1);

	if (filename[0] && filename[1]==':' && filename[2]=='\\') {
		/* chemin absolu */
		strcpy(curpath,filename);
	} else if (filename[0] && filename[1]==':') {
		rasm_printf(ae,"unsupported path style [%s]\n",filename);
		exit(-111);
	} else {
		if (filename[0]=='.' && filename[1]=='\\') {
			strcpy(curpath,GetPath(dadfilename));
			strcat(curpath,filename+2);
		} else {
			strcpy(curpath,GetPath(dadfilename));
			strcat(curpath,filename);
		}
	}
#else
	if (filename[0]=='/') {
		/* chemin absolu */
		strcpy(curpath,filename);
	} else if (filename[0]=='.' && filename[1]=='/') {
		strcpy(curpath,GetPath(dadfilename));
		strcat(curpath,filename+2);
	} else {
		strcpy(curpath,GetPath(dadfilename));
		strcat(curpath,filename);
	}
#endif

	return curpath;
}


void InitAutomate(char *autotab, const unsigned char *def)
{
	#undef FUNC
	#define FUNC "InitAutomate"

	int i;

	memset(autotab,0,256);
	for (i=0;def[i];i++) {
		autotab[(int)def[i]]=1;
	}
}
void StateMachineResizeBuffer(char **ABuf, int idx, int *ASize) {
	#undef FUNC
	#define FUNC "StateMachineResizeBuffer"

	if (idx>=*ASize) {
		if (*ASize<16384) {
			*ASize=(*ASize)*2;
		} else {
			*ASize=(*ASize)+16384;
		}
		*ABuf=MemRealloc(*ABuf,(*ASize)+2);
	}
}

int GetCRC(char *label)
{
	#undef FUNC
	#define FUNC "GetCRC"
	int crc=0x12345678;
	int i=0;

	while (label[i]!=0) {
		crc=(crc<<9)^(crc+label[i++]);
	}
	return crc;
}

int IsRegister(char *zeexpression)
{
	#undef FUNC
	#define FUNC "IsRegister"

	switch (GetCRC(zeexpression)) {
		case CRC_F:if (strcmp(zeexpression,"F")==0) return 1; else return 0;
		case CRC_I:if (strcmp(zeexpression,"I")==0) return 1; else return 0;
		case CRC_R:if (strcmp(zeexpression,"R")==0) return 1; else return 0;
		case CRC_A:if (strcmp(zeexpression,"A")==0) return 1; else return 0;
		case CRC_B:if (strcmp(zeexpression,"B")==0) return 1; else return 0;
		case CRC_C:if (strcmp(zeexpression,"C")==0) return 1; else return 0;
		case CRC_D:if (strcmp(zeexpression,"D")==0) return 1; else return 0;
		case CRC_E:if (strcmp(zeexpression,"E")==0) return 1; else return 0;
		case CRC_H:if (strcmp(zeexpression,"H")==0) return 1; else return 0;
		case CRC_L:if (strcmp(zeexpression,"L")==0) return 1; else return 0;
		case CRC_BC:if (strcmp(zeexpression,"BC")==0) return 1; else return 0;
		case CRC_DE:if (strcmp(zeexpression,"DE")==0) return 1; else return 0;
		case CRC_HL:if (strcmp(zeexpression,"HL")==0) return 1; else return 0;
		case CRC_IX:if (strcmp(zeexpression,"IX")==0) return 1; else return 0;
		case CRC_IY:if (strcmp(zeexpression,"IY")==0) return 1; else return 0;
		case CRC_SP:if (strcmp(zeexpression,"SP")==0) return 1; else return 0;
		case CRC_AF:if (strcmp(zeexpression,"AF")==0) return 1; else return 0;
		case CRC_XH:if (strcmp(zeexpression,"XH")==0) return 1; else return 0;
		case CRC_XL:if (strcmp(zeexpression,"XL")==0) return 1; else return 0;
		case CRC_YH:if (strcmp(zeexpression,"YH")==0) return 1; else return 0;
		case CRC_YL:if (strcmp(zeexpression,"YL")==0) return 1; else return 0;
		case CRC_HX:if (strcmp(zeexpression,"HX")==0) return 1; else return 0;
		case CRC_LX:if (strcmp(zeexpression,"LX")==0) return 1; else return 0;
		case CRC_HY:if (strcmp(zeexpression,"HY")==0) return 1; else return 0;
		case CRC_LY:if (strcmp(zeexpression,"LY")==0) return 1; else return 0;
		case CRC_IXL:if (strcmp(zeexpression,"IXL")==0) return 1; else return 0;
		case CRC_IXH:if (strcmp(zeexpression,"IXH")==0) return 1; else return 0;
		case CRC_IYL:if (strcmp(zeexpression,"IYL")==0) return 1; else return 0;
		case CRC_IYH:if (strcmp(zeexpression,"IYH")==0) return 1; else return 0;
		default:break;
	}
	return 0;
}

int StringIsMem(char *w)
{
	#undef FUNC
	#define FUNC "StringIsMem"

	int p=1,idx=1;

	if (w[0]=='(') {
		while (w[idx]) {
			switch (w[idx]) {
				case '\\':if (w[idx+1]) idx++;
					break;
				case '\'':if (w[idx+1] && w[idx+1]!='\\') idx++;
					break;
				case '(':p++;break;
				case ')':p--;
					if (!p && w[idx+1]) return 0;
					break;
				default:break;
			}
			idx++;
		}
		if (w[idx-1]!=')') return 0;
	} else {
		return 0;
	}
	return 1;

}
int StringIsQuote(char *w)
{
	#undef FUNC
	#define FUNC "StringIsQuote"

	int i,tquote,lens;

	if (w[0]=='\'' || w[0]=='"') {
		tquote=w[0];
		lens=strlen(w);
		
		/* est-ce bien une chaine et uniquement une chaine? */
		i=1;
		while (w[i] && w[i]!=tquote) {
			if (w[i]=='\\') i++;
			i++;
		}
		if (i==lens-1) {
			return tquote;
		}
	}
	return 0;
}
char *StringLooksLikeDicoRecurse(struct s_crcdico_tree *lt, int *score, char *str)
{
	#undef FUNC
	#define FUNC "StringLooksLikeDicoRecurse"

	char symbol_line[1024],*retstr=NULL,*tmpstr;
	int i,curs;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			tmpstr=StringLooksLikeDicoRecurse(lt->radix[i],score,str);
			if (tmpstr!=NULL) retstr=tmpstr;
		}
	}
	if (lt->mdico) {
		for (i=0;i<lt->ndico;i++) {
			if (strlen(lt->dico[i].name)>4) {
				curs=_internal_LevenshteinDistance(str,lt->dico[i].name);
				if (curs<*score) {
					*score=curs;
					retstr=lt->dico[i].name;
				}
			}
		}
	}
	return retstr;
}
char *StringLooksLikeDico(struct s_assenv *ae, int *score, char *str)
{
	#undef FUNC
	#define FUNC "StringLooksLikeDico"

	char *retstr=NULL,*tmpstr;
	int i;

	for (i=0;i<256;i++) {
		if (ae->dicotree.radix[i]) {
			tmpstr=StringLooksLikeDicoRecurse(ae->dicotree.radix[i],score,str);
			if (tmpstr!=NULL) retstr=tmpstr;
		}
	}
	return retstr;
}
char *StringLooksLikeMacro(struct s_assenv *ae, char *str, int *retscore)
{
	#undef FUNC
	#define FUNC "StringLooksLikeMacro"
	
	char *ret=NULL;
	int i,curs,score=3;
	/* search in macros */
	for (i=0;i<ae->imacro;i++) {
		curs=_internal_LevenshteinDistance(ae->macro[i].mnemo,str);
		if (curs<score) {
			score=curs;
			ret=ae->macro[i].mnemo;
		}
	}
	if (retscore) *retscore=score;
	return ret;
}	

char *StringLooksLike(struct s_assenv *ae, char *str)
{
	#undef FUNC
	#define FUNC "StringLooksLike"

	char *ret=NULL,*tmpret;
	int i,curs,score=4;

	/* search in variables */
	ret=StringLooksLikeDico(ae,&score,str);

	/* search in labels */
	for (i=0;i<ae->il;i++) {
		if (!ae->label[i].name && strlen(ae->wl[ae->label[i].iw].w)>4) {
			curs=_internal_LevenshteinDistance(ae->wl[ae->label[i].iw].w,str);
			if (curs<score) {
				score=curs;
				ret=ae->wl[ae->label[i].iw].w;
			}
		}
	}
	
	/* search in alias */
	for (i=0;i<ae->ialias;i++) {
		if (strlen(ae->alias[i].alias)>4) {
			curs=_internal_LevenshteinDistance(ae->alias[i].alias,str);
			if (curs<score) {
				score=curs;
				ret=ae->alias[i].alias;
			}
		}
	}
	
	tmpret=StringLooksLikeMacro(ae,str,&curs);
	if (curs<score) {
		score=curs;
		ret=tmpret;
	}
	return ret;
}

struct s_label *SearchLabel(struct s_assenv *ae, char *label, int crc);
char *GetExpFile(struct s_assenv *ae,int didx){
	#undef FUNC
	#define FUNC "GetExpFile"
	
	if (ae->label_filename) {
		return ae->label_filename;
	}
	if (!didx) {
		return ae->filename[ae->wl[ae->idx].ifile];
	} else {
		if (ae->expression && didx<ae->ie) {
			return ae->filename[ae->wl[ae->expression[didx].iw].ifile];
		} else {
			return ae->filename[ae->wl[ae->idx].ifile];
		}
	}
}

int GetExpLine(struct s_assenv *ae,int didx){
	#undef FUNC
	#define FUNC "GetExpLine"

	if (ae->label_line) return ae->label_line;
	
	if (!didx) {
		return ae->wl[ae->idx].l;
	} else {
		return ae->wl[ae->expression[didx].iw].l;
	}
}

char *GetCurrentFile(struct s_assenv *ae)
{
	return GetExpFile(ae,0);
}


/*******************************************************************************************
			    M E M O R Y       C L E A N U P 
*******************************************************************************************/
void FreeLabelTree(struct s_assenv *ae);
void FreeDicoTree(struct s_assenv *ae);
void FreeUsedTree(struct s_assenv *ae);
void ExpressionFastTranslate(struct s_assenv *ae, char **ptr_expr, int fullreplace);
char *TradExpression(char *zexp);
double ComputeExpressionCore(struct s_assenv *ae,char *original_zeexpression,int ptr, int didx);


void FreeAssenv(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "FreeAssenv"
	int i,j;

#ifndef RDD
	/* let the system free the memory in command line */
	if (!ae->flux) return;
#endif

	for (i=0;i<ae->nbbank;i++) {
		MemFree(ae->mem[i]);
	}
	MemFree(ae->mem);
	
	/* expression core buffer free */
	ComputeExpressionCore(NULL,NULL,0,0);
	ExpressionFastTranslate(NULL,NULL,0);
	/* free labels, expression, orgzone, repeat, ... */
	if (ae->mo) MemFree(ae->orgzone);
	if (ae->me) {
		for (i=0;i<ae->ie;i++) if (ae->expression[i].reference) MemFree(ae->expression[i].reference);
		MemFree(ae->expression);
	}
	if (ae->mh) {
		for (i=0;i<ae->ih;i++) {
			MemFree(ae->hexbin[i].data);
			MemFree(ae->hexbin[i].filename);
		}
		MemFree(ae->hexbin);
	}
	for (i=0;i<ae->il;i++) {
		if (ae->label[i].name && ae->label[i].iw==-1) MemFree(ae->label[i].name);
	}
	/* structures */
	for (i=0;i<ae->irasmstructalias;i++) {
		//printf("structalias[%d]=%s (%d)\n",i,ae->rasmstructalias[i].name,ae->rasmstructalias[i].size);
		MemFree(ae->rasmstructalias[i].name);
	}
	if (ae->mrasmstructalias) MemFree(ae->rasmstructalias);
	
	for (i=0;i<ae->irasmstruct;i++) {
		for (j=0;j<ae->rasmstruct[i].irasmstructfield;j++) {
			MemFree(ae->rasmstruct[i].rasmstructfield[j].fullname);
			MemFree(ae->rasmstruct[i].rasmstructfield[j].name);
		}
		if (ae->rasmstruct[i].mrasmstructfield) MemFree(ae->rasmstruct[i].rasmstructfield);
		MemFree(ae->rasmstruct[i].name);
	}
	if (ae->mrasmstruct) MemFree(ae->rasmstruct);
	
	/* other */
	if (ae->maxbreakpoint) MemFree(ae->breakpoint);
	if (ae->ml) MemFree(ae->label);
	if (ae->mr) MemFree(ae->repeat);
	if (ae->mi) MemFree(ae->ifthen);
	if (ae->msw) MemFree(ae->switchcase);
	if (ae->mw) MemFree(ae->whilewend);
	/* deprecated
	for (i=0;i<ae->idic;i++) {
		MemFree(ae->dico[i].name);
	}
	if (ae->mdic) MemFree(ae->dico);
	*/
	if (ae->mlz) MemFree(ae->lzsection);

	for (i=0;i<ae->ifile;i++) {
		MemFree(ae->filename[i]);
	}
	MemFree(ae->filename);

	for (i=0;i<ae->imacro;i++) {
		if (ae->macro[i].maxword) MemFree(ae->macro[i].wc);
		for (j=0;j<ae->macro[i].nbparam;j++) MemFree(ae->macro[i].param[j]);
		if (ae->macro[i].nbparam) MemFree(ae->macro[i].param);
	}
	if (ae->mmacro) MemFree(ae->macro);

	for (i=0;i<ae->ialias;i++) {
//printf("alias[%d][%s]=[%s]\n",i,ae->alias[i].alias,ae->alias[i].translation);
		MemFree(ae->alias[i].alias);
		MemFree(ae->alias[i].translation);
	}
	if (ae->malias) MemFree(ae->alias);

	for (i=0;ae->wl[i].t!=2;i++) {
		MemFree(ae->wl[i].w);
	}
	MemFree(ae->wl);

	if (ae->ctx1.varbuffer) {
		MemFree(ae->ctx1.varbuffer);
	}
	if (ae->ctx1.maxtokenstack) {
		MemFree(ae->ctx1.tokenstack);
	}
	if (ae->ctx1.maxoperatorstack) {
		MemFree(ae->ctx1.operatorstack);
	}
	if (ae->ctx2.varbuffer) {
		MemFree(ae->ctx2.varbuffer);
	}
	if (ae->ctx2.maxtokenstack) {
		MemFree(ae->ctx2.tokenstack);
	}
	if (ae->ctx2.maxoperatorstack) {
		MemFree(ae->ctx2.operatorstack);
	}

	for (i=0;i<ae->iticker;i++) {
		MemFree(ae->ticker[i].varname);
	}
	if (ae->mticker) MemFree(ae->ticker);

	MemFree(ae->outputfilename);
	FreeLabelTree(ae);
	FreeDicoTree(ae);
	FreeUsedTree(ae);
	if (ae->mmacropos) MemFree(ae->macropos);
	TradExpression(NULL);
	MemFree(ae);
}



void MaxError(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "MaxError"

	char **source_lines=NULL;
	int idx,crc,zeline;
	char c;

	/* extended error is useful with generated code we do not want to edit */
	if (ae->extended_error && ae->wl) {
		/* super dupper slow but anyway, there is an error... */
		if (ae->wl[ae->idx].l) {
			source_lines=FileReadLinesRAW(GetCurrentFile(ae));
			zeline=0;
			while (zeline<ae->wl[ae->idx].l-1 && source_lines[zeline]) zeline++;
			if (zeline==ae->wl[ae->idx].l-1 && source_lines[zeline]) {
				rasm_printf(ae,"-> %s",source_lines[zeline]);
			} else {
				rasm_printf(ae,"cannot read line %d of file [%s]\n",ae->wl[ae->idx].l,GetCurrentFile(ae));
			}
			FreeArrayDynamicValue(&source_lines);
		}
	}
	
	ae->nberr++;
	if (ae->nberr==ae->maxerr) {
		rasm_printf(ae,"Too many errors!\n");
		FreeAssenv(ae);
		exit(1);
	}
}

void (*___output)(struct s_assenv *ae, unsigned char v);

void ___internal_output_disabled(struct s_assenv *ae,unsigned char v)
{
	#undef FUNC
	#define FUNC "fake ___output"
}
void ___internal_output(struct s_assenv *ae,unsigned char v)
{
	#undef FUNC
	#define FUNC "___output"
	
	if (ae->outputadr<=ae->maxptr) {
		ae->mem[ae->activebank][ae->outputadr++]=v;
		ae->codeadr++;
	} else {
		rasm_printf(ae,"[%s:%d] output exceed limit %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->maxptr);
		MaxError(ae);
		ae->stop=1;
		___output=___internal_output_disabled;
	}
}
void ___internal_output_nocode(struct s_assenv *ae,unsigned char v)
{
	#undef FUNC
	#define FUNC "___output (nocode)"
	
	if (ae->outputadr<=ae->maxptr) {
		ae->outputadr++;
		ae->codeadr++;
	} else {
		rasm_printf(ae,"[%s:%d] NOCODE output exceed limit %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->maxptr);
		MaxError(ae);
		ae->stop=1;
		___output=___internal_output_disabled;
	}
}


void ___output_set_limit(struct s_assenv *ae,int zelimit)
{
	#undef FUNC
	#define FUNC "___output_set_limit"

	int limit=65535;
	
	if (zelimit<=limit) {
		/* apply limit */
		limit=zelimit;
	} else {
		rasm_printf(ae,"[%s:%d] limit exceed hardware limitation!\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		ae->stop=1;
	}
	if (ae->outputadr>=0 && ae->outputadr>limit) {
		rasm_printf(ae,"[%s:%d] limit too high for current output!\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		ae->stop=1;
	}
	ae->maxptr=limit;
}

unsigned char *MakeAMSDOSHeader(int minmem, int maxmem, char *amsdos_name) {
	#undef FUNC
	#define FUNC "MakeAMSDOSHeader"
	
	static unsigned char AmsdosHeader[128];
	char *zepoint;
	int checksum,i=0;
	/***  cpcwiki			
	Byte 00: User number
	Byte 01 to 08: filename
	Byte 09 bis 11: Extension
	Byte 18: type-byte
	Byte 21 and 22: loading address
	Byte 24 and 25: file length
	Byte 26 and 27: execution address for machine code programs
	Byte 64 and 65: (file length)
	Byte 67 and 68: checksum for byte 00 to byte 66
	To calculate the checksum, just add byte 00 to byte 66 to each other.
	*/
	memset(AmsdosHeader,0,sizeof(AmsdosHeader));
	AmsdosHeader[0]=0;
	memcpy(AmsdosHeader+1,amsdos_name,11);

	AmsdosHeader[18]=2; /* 0 basic 1 basic protege 2 binaire */
	AmsdosHeader[19]=(maxmem-minmem)&0xFF;
	AmsdosHeader[20]=(maxmem-minmem)>>8;
	AmsdosHeader[21]=minmem&0xFF;
	AmsdosHeader[22]=minmem>>8;
	AmsdosHeader[24]=AmsdosHeader[19];
	AmsdosHeader[25]=AmsdosHeader[20];
	AmsdosHeader[26]=minmem&0xFF;
	AmsdosHeader[27]=minmem>>8;
	AmsdosHeader[64]=AmsdosHeader[19];
	AmsdosHeader[65]=AmsdosHeader[20];
	AmsdosHeader[66]=0;
	
	for (i=checksum=0;i<=66;i++) {
		checksum+=AmsdosHeader[i];
	}
	AmsdosHeader[67]=checksum&0xFF;
	AmsdosHeader[68]=checksum>>8;

	/* garbage / shadow values from sector buffer? */
	memcpy(AmsdosHeader+0x47,amsdos_name,8);
	AmsdosHeader[0x4F]=0x24;
	AmsdosHeader[0x50]=0x24;
	AmsdosHeader[0x51]=0x24;
	AmsdosHeader[0x52]=0xFF;
	AmsdosHeader[0x54]=0xFF;
	AmsdosHeader[0x57]=0x02;
	AmsdosHeader[0x5A]=AmsdosHeader[21];
	AmsdosHeader[0x5B]=AmsdosHeader[22];
	AmsdosHeader[0x5D]=AmsdosHeader[24];
	AmsdosHeader[0x5E]=AmsdosHeader[25];

	sprintf((char *)AmsdosHeader+0x47+17," generated by %s ",RASM_VERSION);

	return AmsdosHeader;
}

void ExpressionFastTranslate(struct s_assenv *ae, char **ptr_expr, int fullreplace);


int cmpAmsdosentry(const void * a, const void * b)
{
	return memcmp(a,b,32);
}

int cmpmacros(const void * a, const void * b)
{
	struct s_macro *sa,*sb;
	sa=(struct s_macro *)a;
	sb=(struct s_macro *)b;
	if (sa->crc<sb->crc) return -1; else return 1;
}
int SearchAlias(struct s_assenv *ae, int crc, char *zemot)
{
    int dw,dm,du,i;

	/* inutile de tourner autour du pot pour un si petit nombre */
	if (ae->ialias<5) {
			for (i=0;i<ae->ialias;i++) {
					if (ae->alias[i].crc==crc && strcmp(ae->alias[i].alias,zemot)==0) {
							return i;
					}
			}
			return -1;
	}
	
	dw=0;
	du=ae->ialias-1;
	while (dw<=du) {
		dm=(dw+du)/2;
		if (ae->alias[dm].crc==crc) {
			/* chercher le premier de la liste */
			while (dm>0 && ae->alias[dm-1].crc==crc) dm--;
			/* controle sur le texte entier */
			while (ae->alias[dm].crc==crc && strcmp(ae->alias[dm].alias,zemot)) dm++;
			if (ae->alias[dm].crc==crc && strcmp(ae->alias[dm].alias,zemot)==0) return dm; else return -1;
		} else if (ae->alias[dm].crc>crc) {
			du=dm-1;
		} else if (ae->alias[dm].crc<crc) {
			dw=dm+1;
		}
	}
	return -1;
}
int SearchMacro(struct s_assenv *ae, int crc, char *zemot)
{
	int dw,dm,du,i;

	/* inutile de tourner autour du pot pour un si petit nombre */
	if (ae->imacro<5) {
			for (i=0;i<ae->imacro;i++) {
					if (ae->macro[i].crc==crc && strcmp(ae->macro[i].mnemo,zemot)==0) {
							return i;
					}
			}
			return -1;
	}
	
	dw=0;
	du=ae->imacro-1;
	while (dw<=du) {
		dm=(dw+du)/2;
		if (ae->macro[dm].crc==crc) {
			/* chercher le premier de la liste */
			while (dm>0 && ae->macro[dm-1].crc==crc) dm--;
			/* controle sur le texte entier */
			while (ae->macro[dm].crc==crc && strcmp(ae->macro[dm].mnemo,zemot)) dm++;
			if (ae->macro[dm].crc==crc && strcmp(ae->macro[dm].mnemo,zemot)==0) return dm; else return -1;
		} else if (ae->macro[dm].crc>crc) {
			du=dm-1;
		} else if (ae->macro[dm].crc<crc) {
			dw=dm+1;
		}
	}
	return -1;
}

void CheckAndSortAliases(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "CheckAndSortAliases"

	struct s_alias tmpalias;
	int i,dw,dm,du,crc;
	for (i=0;i<ae->ialias-1;i++) {
		/* is there previous aliases in the new alias? */
		if (strstr(ae->alias[ae->ialias-1].translation,ae->alias[i].alias)) {
			/* there is a match, apply alias translation */
			ExpressionFastTranslate(ae,&ae->alias[ae->ialias-1].translation,2);
			/* need to compute again len */
			ae->alias[ae->ialias-1].len=strlen(ae->alias[ae->ialias-1].translation);
			break;
		}
	}
	
	/* cas particuliers pour insertion en d�but ou fin de liste */
	if (ae->ialias-1) {
		if (ae->alias[ae->ialias-1].crc>ae->alias[ae->ialias-2].crc) {
			/* pas de tri il est d�j� au bon endroit */
		} else if (ae->alias[ae->ialias-1].crc<ae->alias[0].crc) {
			/* insertion tout en bas de liste */
			tmpalias=ae->alias[ae->ialias-1];
			MemMove(&ae->alias[1],&ae->alias[0],sizeof(struct s_alias)*(ae->ialias-1));
			ae->alias[0]=tmpalias;
		} else {
			/* on cherche ou inserer */
			crc=ae->alias[ae->ialias-1].crc;
			dw=0;
			du=ae->ialias-1;
			while (dw<=du) {
				dm=(dw+du)/2;
				if (ae->alias[dm].crc==crc) {
					break;
				} else if (ae->alias[dm].crc>crc) {
					du=dm-1;
				} else if (ae->alias[dm].crc<crc) {
					dw=dm+1;
				}
			}
			/* ajustement */
			if (ae->alias[dm].crc<crc) dm++;
			/* insertion */
			tmpalias=ae->alias[ae->ialias-1];
			MemMove(&ae->alias[dm+1],&ae->alias[dm],sizeof(struct s_alias)*(ae->ialias-1-dm));
			ae->alias[dm]=tmpalias;
		}
	} else {
		/* one alias need no sort */
	}
}

void InsertDicoToTree(struct s_assenv *ae, struct s_expr_dico *dico)
{
	#undef FUNC
	#define FUNC "InsertDicoToTree"

	struct s_crcdico_tree *curdicotree;
	int radix,dek=32;

	curdicotree=&ae->dicotree;
	while (dek) {
		dek=dek-8;
		radix=(dico->crc>>dek)&0xFF;
		if (curdicotree->radix[radix]) {
			curdicotree=curdicotree->radix[radix];
		} else {
			curdicotree->radix[radix]=MemMalloc(sizeof(struct s_crcdico_tree));
			curdicotree=curdicotree->radix[radix];
			memset(curdicotree,0,sizeof(struct s_crcdico_tree));
		}
	}
	ObjectArrayAddDynamicValueConcat((void**)&curdicotree->dico,&curdicotree->ndico,&curdicotree->mdico,dico,sizeof(struct s_expr_dico));
}

unsigned char *SnapshotDicoInsert(char *symbol_name, int ptr, int *retidx)
{
	static unsigned char *subchunk=NULL;
	static int subchunksize=0;
	static int idx=0;
	int symbol_len;
	
	if (retidx) {
		if (symbol_name && strcmp(symbol_name,"FREE")==0) {
			subchunksize=0;
			idx=0;
			MemFree(subchunk);
			subchunk=NULL;
		}
		*retidx=idx;
		return subchunk;
	}
	
	if (idx+65536>subchunksize) {
		subchunksize=subchunksize+65536;
		subchunk=MemRealloc(subchunk,subchunksize);
	}
	
	symbol_len=strlen(symbol_name);
	if (symbol_len>255) symbol_len=255;
	subchunk[idx++]=symbol_len;
	memcpy(subchunk+idx,symbol_name,symbol_len);
	idx+=symbol_len;
	memset(subchunk+idx,0,6);
	idx+=6;
	subchunk[idx++]=(ptr&0xFF00)/256;
	subchunk[idx++]=ptr&0xFF;
	return NULL;
}

void SnapshotDicoTreeRecurse(struct s_crcdico_tree *lt)
{
	#undef FUNC
	#define FUNC "SnapshottDicoTreeRecurse"

	char symbol_line[1024];
	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			SnapshotDicoTreeRecurse(lt->radix[i]);
		}
	}
	if (lt->mdico) {
		for (i=0;i<lt->ndico;i++) {
			if (strcmp(lt->dico[i].name,"IX") && strcmp(lt->dico[i].name,"IY") && strcmp(lt->dico[i].name,"PI") && strcmp(lt->dico[i].name,"ASSEMBLER_RASM")) {
				SnapshotDicoInsert(lt->dico[i].name,(int)floor(lt->dico[i].v+0.5),NULL);
			}
		}
	}
}
unsigned char *SnapshotDicoTree(struct s_assenv *ae, int *retidx)
{
	#undef FUNC
	#define FUNC "SnapshotDicoTree"

	unsigned char *sc;
	int idx;
	int i;

	for (i=0;i<256;i++) {
		if (ae->dicotree.radix[i]) {
			SnapshotDicoTreeRecurse(ae->dicotree.radix[i]);
		}
	}
	
	sc=SnapshotDicoInsert(NULL,0,&idx);
	*retidx=idx;
	return sc;
}

void ExportDicoTreeRecurse(struct s_crcdico_tree *lt, char *zefile, char *zeformat)
{
	#undef FUNC
	#define FUNC "ExportDicoTreeRecurse"

	char symbol_line[1024];
	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			ExportDicoTreeRecurse(lt->radix[i],zefile,zeformat);
		}
	}
	if (lt->mdico) {
		for (i=0;i<lt->ndico;i++) {
			if (strcmp(lt->dico[i].name,"IX") && strcmp(lt->dico[i].name,"IY") && strcmp(lt->dico[i].name,"PI") && strcmp(lt->dico[i].name,"ASSEMBLER_RASM")) {
				snprintf(symbol_line,sizeof(symbol_line)-1,zeformat,lt->dico[i].name,(int)floor(lt->dico[i].v+0.5));
				symbol_line[sizeof(symbol_line)-1]=0xD;
				FileWriteLine(zefile,symbol_line);
			}
		}
	}
}
void ExportDicoTree(struct s_assenv *ae, char *zefile, char *zeformat)
{
	#undef FUNC
	#define FUNC "ExportDicoTree"

	int i;

	for (i=0;i<256;i++) {
		if (ae->dicotree.radix[i]) {
			ExportDicoTreeRecurse(ae->dicotree.radix[i],zefile,zeformat);
		}
	}
}
void FreeDicoTreeRecurse(struct s_crcdico_tree *lt)
{
	#undef FUNC
	#define FUNC "FreeDicoTreeRecurse"

	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			FreeDicoTreeRecurse(lt->radix[i]);
		}
	}
	if (lt->mdico) {
		for (i=0;i<lt->ndico;i++) {
			MemFree(lt->dico[i].name);
		}
		MemFree(lt->dico);
	}
	MemFree(lt);
}
void FreeDicoTree(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "FreeDicoTree"

	int i;

	for (i=0;i<256;i++) {
		if (ae->dicotree.radix[i]) {
			FreeDicoTreeRecurse(ae->dicotree.radix[i]);
		}
	}
	if (ae->dicotree.mdico) {
		for (i=0;i<ae->dicotree.ndico;i++) MemFree(ae->dicotree.dico[i].name);
		MemFree(ae->dicotree.dico);
	}
}
struct s_expr_dico *SearchDico(struct s_assenv *ae, char *dico, int crc)
{
	#undef FUNC
	#define FUNC "SearchDico"

	struct s_crcdico_tree *curdicotree;
	struct s_expr_dico *retdico=NULL;
	int i,radix,dek=32;

	curdicotree=&ae->dicotree;

	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curdicotree->radix[radix]) {
			curdicotree=curdicotree->radix[radix];
		} else {
			/* radix not found, dico is not in index */
			return NULL;
		}
	}
	for (i=0;i<curdicotree->ndico;i++) {
		if (strcmp(curdicotree->dico[i].name,dico)==0) {
			return &curdicotree->dico[i];
		}
	}
	return NULL;
}
int DelDico(struct s_assenv *ae, char *dico, int crc)
{
	#undef FUNC
	#define FUNC "DelDico"

	struct s_crcdico_tree *curdicotree;
	struct s_expr_dico *retdico=NULL;
	int i,radix,dek=32;

	curdicotree=&ae->dicotree;

	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curdicotree->radix[radix]) {
			curdicotree=curdicotree->radix[radix];
		} else {
			/* radix not found, dico is not in index */
			return 0;
		}
	}
	for (i=0;i<curdicotree->ndico;i++) {
		if (strcmp(curdicotree->dico[i].name,dico)==0) {
			/* must free memory */
			MemFree(curdicotree->dico[i].name);
			if (i<curdicotree->ndico-1) {
				MemMove(&curdicotree->dico[i],&curdicotree->dico[i+1],(curdicotree->ndico-i-1)*sizeof(struct s_expr_dico));
			}
			curdicotree->ndico--;
			return 1;
		}
	}
	return 0;
}


void InsertUsedToTree(struct s_assenv *ae, char *used, int crc)
{
	#undef FUNC
	#define FUNC "InsertUsedToTree"

	struct s_crcused_tree *curusedtree;
	int radix,dek=32,i;
	
	curusedtree=&ae->usedtree;
	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curusedtree->radix[radix]) {
			curusedtree=curusedtree->radix[radix];
		} else {
			curusedtree->radix[radix]=MemMalloc(sizeof(struct s_crcused_tree));
			curusedtree=curusedtree->radix[radix];
			memset(curusedtree,0,sizeof(struct s_crcused_tree));
		}
	}
	for (i=0;i<curusedtree->nused;i++) if (strcmp(used,curusedtree->used[i])==0) break;
	/* no double */
	if (i==curusedtree->nused) {
		FieldArrayAddDynamicValueConcat(&curusedtree->used,&curusedtree->nused,&curusedtree->mused,used);
	}
}

void FreeUsedTreeRecurse(struct s_crcused_tree *lt)
{
	#undef FUNC
	#define FUNC "FreeUsedTreeRecurse"

	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			FreeUsedTreeRecurse(lt->radix[i]);
		}
	}
	if (lt->mused) {
		for (i=0;i<lt->nused;i++) MemFree(lt->used[i]);
		MemFree(lt->used);
	}
	MemFree(lt);
}
void FreeUsedTree(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "FreeUsedTree"

	int i;

	for (i=0;i<256;i++) {
		if (ae->usedtree.radix[i]) {
			FreeUsedTreeRecurse(ae->usedtree.radix[i]);
		}
	}
}
int SearchUsed(struct s_assenv *ae, char *used, int crc)
{
	#undef FUNC
	#define FUNC "SearchUsed"

	struct s_crcused_tree *curusedtree;
	int i,radix,dek=32;

	curusedtree=&ae->usedtree;
	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curusedtree->radix[radix]) {
			curusedtree=curusedtree->radix[radix];
		} else {
			/* radix not found, used is not in index */
			return 0;
		}
	}
	for (i=0;i<curusedtree->nused;i++) {
		if (strcmp(curusedtree->used[i],used)==0) {
			return 1;
		}
	}
	return 0;
}



void InsertTextToTree(struct s_assenv *ae, char *text, char *replace, int crc)
{
	#undef FUNC
	#define FUNC "InsertTextToTree"

	struct s_crcstring_tree *curstringtree;
	int radix,dek=32,i;
	
	curstringtree=&ae->stringtree;
	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curstringtree->radix[radix]) {
			curstringtree=curstringtree->radix[radix];
		} else {
			curstringtree->radix[radix]=MemMalloc(sizeof(struct s_crcused_tree));
			curstringtree=curstringtree->radix[radix];
			memset(curstringtree,0,sizeof(struct s_crcused_tree));
		}
	}
	for (i=0;i<curstringtree->ntext;i++) if (strcmp(text,curstringtree->text[i])==0) break;
	/* no double */
	if (i==curstringtree->ntext) {
		text=TxtStrDup(text);
		replace=TxtStrDup(replace);
		FieldArrayAddDynamicValueConcat(&curstringtree->text,&curstringtree->ntext,&curstringtree->mtext,text);
		FieldArrayAddDynamicValueConcat(&curstringtree->replace,&curstringtree->nreplace,&curstringtree->mreplace,replace);
	}
}

void FreeTextTreeRecurse(struct s_crcstring_tree *lt)
{
	#undef FUNC
	#define FUNC "FreeTextTreeRecurse"

	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			FreeTextTreeRecurse(lt->radix[i]);
		}
	}
	if (lt->mtext) {
		for (i=0;i<lt->ntext;i++) MemFree(lt->text[i]);
		MemFree(lt->text);
	}
	MemFree(lt);
}
void FreeTextTree(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "FreeTextTree"

	int i;

	for (i=0;i<256;i++) {
		if (ae->stringtree.radix[i]) {
			FreeTextTreeRecurse(ae->stringtree.radix[i]);
		}
	}
	if (ae->stringtree.mtext) MemFree(ae->stringtree.text);
}
int SearchText(struct s_assenv *ae, char *text, int crc)
{
	#undef FUNC
	#define FUNC "SearchText"

	struct s_crcstring_tree *curstringtree;
	int i,radix,dek=32;

	curstringtree=&ae->stringtree;
	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curstringtree->radix[radix]) {
			curstringtree=curstringtree->radix[radix];
		} else {
			/* radix not found, used is not in index */
			return 0;
		}
	}
	for (i=0;i<curstringtree->ntext;i++) {
		if (strcmp(curstringtree->text[i],text)==0) {
			return 1;
		}
	}
	return 0;
}







/*
struct s_crclabel_tree {




/*
struct s_crclabel_tree {
	struct s_crclabel_tree *radix[256];
	struct s_label *label;
	int nlabel,mlabel;
};
*/
void FreeLabelTreeRecurse(struct s_crclabel_tree *lt)
{
	#undef FUNC
	#define FUNC "FreeLabelTreeRecurse"

	int i;

	for (i=0;i<256;i++) {
		if (lt->radix[i]) {
			FreeLabelTreeRecurse(lt->radix[i]);
		}
	}
	/* label.name already freed elsewhere as this one is a copy */
	if (lt->mlabel) MemFree(lt->label);
	MemFree(lt);
}
void FreeLabelTree(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "FreeLabelTree"

	int i;

	for (i=0;i<256;i++) {
		if (ae->labeltree.radix[i]) {
			FreeLabelTreeRecurse(ae->labeltree.radix[i]);
		}
	}
	if (ae->labeltree.mlabel) MemFree(ae->labeltree.label);
}

struct s_label *SearchLabel(struct s_assenv *ae, char *label, int crc)
{
	#undef FUNC
	#define FUNC "SearchLabel"

	struct s_crclabel_tree *curlabeltree;
	struct s_label *retlabel=NULL;
	int i,radix,dek=32;

	curlabeltree=&ae->labeltree;
	while (dek) {
		dek=dek-8;
		radix=(crc>>dek)&0xFF;
		if (curlabeltree->radix[radix]) {
			curlabeltree=curlabeltree->radix[radix];
		} else {
			/* radix not found, label is not in index */
			return NULL;
		}
	}
	for (i=0;i<curlabeltree->nlabel;i++) {
		if (!curlabeltree->label[i].name && strcmp(ae->wl[curlabeltree->label[i].iw].w,label)==0) {
			return &curlabeltree->label[i];
		} else if (curlabeltree->label[i].name && strcmp(curlabeltree->label[i].name,label)==0) {
			return &curlabeltree->label[i];
		}
	}
	return NULL;
}

char *MakeLocalLabel(struct s_assenv *ae,char *varbuffer, int *retdek)
{
	
	#undef FUNC
	#define FUNC "MakeLocalLabel"
	
	char *locallabel;
	char hexdigit[32];
	int lenbuf=0,dek,i,im;
//int debug;
//if (strncmp(varbuffer,"@FAST",5)==0) debug=1; else debug=0;
//if (debug) printf("[%s]\n",varbuffer);	
	lenbuf=strlen(varbuffer);
	
	/* not so local labels */
	if (varbuffer[0]=='.') {
		/* create reference */
		if (ae->lastgloballabel) {
			locallabel=MemMalloc(strlen(varbuffer)+1+ae->lastgloballabellen);
			sprintf(locallabel,"%s.%s",ae->lastgloballabel,varbuffer+1);
			if (retdek) *retdek=0;
//printf("manage proximity label [%s]\n",locallabel);
			return locallabel;
		} else {
			rasm_printf(ae,"[%s:%d] cannot create proximity label or alias [%s] as there is no previous global label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,varbuffer);
			MaxError(ae);
			return varbuffer;
		}
	}
	
	if (!retdek) {
		locallabel=MemMalloc(lenbuf+(ae->ir+ae->iw+3)*8+8);
		strcpy(locallabel,varbuffer);
	} else {
		locallabel=MemMalloc((ae->ir+ae->iw+3)*8+4);
		locallabel[0]=0;
	}	
//if (debug) printf("[%s]\n",locallabel);	

	dek=0;
	dek+=strappend(locallabel,"R");
	for (i=0;i<ae->ir;i++) {
		sprintf(hexdigit,"%04X",ae->repeat[i].cpt&0xFFFF);
		dek+=strappend(locallabel,hexdigit);
	}
	if (ae->ir) {
		sprintf(hexdigit,"%04X",ae->repeat[ae->ir-1].value&0xFFFF);
		dek+=strappend(locallabel+dek,hexdigit);
	}
	
	dek+=strappend(locallabel,"W");
	for (i=0;i<ae->iw;i++) {
		sprintf(hexdigit,"%04X",ae->whilewend[i].cpt&0xFFFF);
		dek+=strappend(locallabel+dek,hexdigit);
	}
	if (ae->iw) {
		sprintf(hexdigit,"%04X",ae->whilewend[ae->iw-1].value&0xFFFF);
		dek+=strappend(locallabel+dek,hexdigit);
	}
	/* where are we? */
	if (ae->imacropos) {
		for (im=ae->imacropos-1;im>=0;im--) {
			if (ae->idx>=ae->macropos[im].start && ae->idx<ae->macropos[im].end) break;
		}
		if (im>=0) {
			/* si on n'est pas dans une macro, on n'indique rien */
			sprintf(hexdigit,"M%04X",ae->macropos[im].value&0xFFFF);
			dek+=strappend(locallabel+dek,hexdigit);
		}
	}
	if (retdek)	{
		*retdek=dek;
	}
	return locallabel;
}

char *TradExpression(char *zexp)
{
	#undef FUNC
	#define FUNC "TradExpression"
	
	static char *last_expression=NULL;
	char *wstr;
	
	if (last_expression) {MemFree(last_expression);last_expression=NULL;}
	if (!zexp) return NULL;
	
	wstr=TxtStrDup(zexp);
	wstr=TxtReplace(wstr,"[","<<",0);
	wstr=TxtReplace(wstr,"]",">>",0);
	wstr=TxtReplace(wstr,"m","%",0);

	last_expression=wstr;
	return wstr;
}

int TrimFloatingPointString(char *fps) {
	int i=0,pflag,zflag=0;
	
	while (fps[i]) {
		if (fps[i]=='.') {
			pflag=i;
			zflag=1;
		} else if (fps[i]!='0') {
			zflag=0;
		}
		i++;
	}
	/* truncate floating fract */
	if (zflag) {
		fps[pflag]=0;
	} else {
		pflag=i;
	}
	return pflag;
}

int RoundComputeExpressionCore(struct s_assenv *ae,char *zeexpression,int ptr,int didx);

/*
	translate tag or formula between curly brackets
	used in label declaration
	used in print directive
*/
char *TranslateTag(struct s_assenv *ae, char *varbuffer, int *touched, int enablefast, int removespace) {
	/*******************************************************
	       v a r i a b l e s     i n    s t r i n g s
	*******************************************************/
	char *starttag,*endtag,*tagcheck,*expr;
	int newlen,lenw,taglen,tagidx,tagcount,validx;
	char curvalstr[256]={0};

	*touched=0;
	while ((starttag=strchr(varbuffer,'{'))!=NULL) {
		if ((endtag=strchr(starttag,'}'))==NULL) {
			rasm_printf(ae,"[%s:%d] invalid tag in string [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),varbuffer);
			MaxError(ae);
			return NULL;
		}
		/* allow inception */
		tagcount=1;
		tagcheck=starttag+1;
		while (*tagcheck && tagcount) {
			if (*tagcheck=='}') tagcount--; else if (*tagcheck=='{') tagcount++;
			tagcheck++;			
		}
		if (tagcount) {
			rasm_printf(ae,"[%s:%d] invalid brackets combination in string [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),varbuffer);
			MaxError(ae);
			return NULL;
		} else {
			endtag=tagcheck-1;
		}
		*touched=1;
		taglen=endtag-starttag+1;
		tagidx=starttag-varbuffer;
		lenw=strlen(varbuffer); // before the EOF write
		*endtag=0;
		/*** c o m p u t e    e x p r e s s i o n ***/
		expr=TxtStrDup(starttag+1);
		if (removespace) expr=TxtReplace(expr," ","",0);
		if (enablefast) ExpressionFastTranslate(ae,&expr,0);
		validx=(int)RoundComputeExpressionCore(ae,expr,ae->codeadr,0);
		if (validx<0) {
			strcpy(curvalstr,"");
			rasm_printf(ae,"[%s:%d] indexed tag must NOT be a negative value [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),varbuffer);
			MaxError(ae);
			MemFree(expr);
			return NULL;
		} else {
			#ifdef OS_WIN
			snprintf(curvalstr,sizeof(curvalstr)-1,"%d",validx);
			newlen=strlen(curvalstr);
			#else
			newlen=snprintf(curvalstr,sizeof(curvalstr)-1,"%d",validx);
			#endif
		}
		MemFree(expr);
		if (newlen>taglen) {
			/* realloc */
			varbuffer=MemRealloc(varbuffer,lenw+newlen-taglen+1);
		}
		if (newlen!=taglen ) {
			MemMove(varbuffer+tagidx+newlen,varbuffer+tagidx+taglen,lenw-taglen-tagidx+1);
		}
		strncpy(varbuffer+tagidx,curvalstr,newlen); /* copy without zero terminator */
	}
	return varbuffer;
}

double ComputeExpressionCore(struct s_assenv *ae,char *original_zeexpression,int ptr, int didx)
{
	#undef FUNC
	#define FUNC "ComputeExpressionCore"

	/* static execution buffers */
	static double *accu=NULL;
	static int maccu=0;
	static struct s_compute_element *computestack=NULL;
	static int maxcomputestack=0;
	int i,j,paccu=0;
	int nbtokenstack=0;
	int nbcomputestack=0;
	int nboperatorstack=0;

	struct s_compute_element stackelement;
	int o2,okclose,itoken;
	
	int idx=0,crc,icheck,is_binary,ivar=0;
	char asciivalue[11];
	unsigned char c;
	/* backup alias replace */
	char *zeexpression,*expr;
	int original=1;
	int ialias,startvar;
	int newlen,lenw;
	/* dictionnary */
	struct s_expr_dico *curdic;
	struct s_label *curlabel;
	char *localname;
	int minusptr,imkey,bank,page,idxmacro;
	double curval;
	/* negative value */
	int allow_minus_as_sign=0;
	/* extended replace in labels */
	int curly=0,curlyflag=0;
	char *Automate;

	/* memory cleanup */
	if (!ae) {
		if (maccu) MemFree(accu);
		accu=NULL;maccu=0;
		if (maxcomputestack) MemFree(computestack);
		computestack=NULL;maxcomputestack=0;
#if 0	
		if (maxivar) MemFree(varbuffer);
		if (maxtokenstack) MemFree(tokenstack);
		if (maxoperatorstack) MemFree(operatorstack);
		maxtokenstack=maxoperatorstack=0;
		maxivar=1;
		varbuffer=NULL;
		tokenstack=NULL;
		operatorstack=NULL;
#endif
		return 0.0;
	}

	/* be sure to have at least some bytes allocated */
	StateMachineResizeBuffer(&ae->computectx->varbuffer,128,&ae->computectx->maxivar);

	zeexpression=original_zeexpression;
	if (!zeexpression[0]) {
		return 0;
	}
//printf("compute[%s]\n",zeexpression);
	/* double hack if the first value is negative */
	if (zeexpression[0]=='-') {
		if (ae->AutomateExpressionValidCharFirst[(int)zeexpression[1]&0xFF]) {
			allow_minus_as_sign=1;
		} else {
			memset(&stackelement,0,sizeof(stackelement));
			ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->tokenstack,&nbtokenstack,&ae->computectx->maxtokenstack,&stackelement,sizeof(stackelement));
		}
	}
	/* is there ascii char? */
	while ((c=zeexpression[idx])!=0) {
		if (c=='\'' || c=='"') {
			/* echappement */
			if (zeexpression[idx+1]=='\\') {
				if (zeexpression[idx+2] && zeexpression[idx+3]==c) {
					sprintf(asciivalue,"#%03X",zeexpression[idx+2]);
					memcpy(zeexpression+idx,asciivalue,4);
					idx+=3;
				} else {
					rasm_printf(ae,"[%s:%d] Only single escaped char may be quoted [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
					MaxError(ae);
					zeexpression[0]=0;
					return 0;
				}
			} else if (zeexpression[idx+1] && zeexpression[idx+2]==c) {
					sprintf(asciivalue,"#%02X",zeexpression[idx+1]);
					memcpy(zeexpression+idx,asciivalue,3);
					idx+=2;
			} else {
				rasm_printf(ae,"[%s:%d] Only single char may be quoted [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
				MaxError(ae);
				zeexpression[0]=0;
				return 0;
			}
		}
		
		idx++;
	}
	/***********************************************************
	  C O M P U T E   E X P R E S S I O N   M A I N    L O O P
	***********************************************************/
	idx=0;
	while ((c=zeexpression[idx])!=0) {
		switch (c) {
			/* parenthesis */
			case ')':
				/* next to a closing parenthesis, a minus is an operator */
				allow_minus_as_sign=0;
				break;
			case '(':
			/* operator detection */
			case '*':
			case '/':
			case '^':
			case '[':
			case 'm':
			case '+':
			case ']':
				allow_minus_as_sign=1;
				break;
			case '&':
				allow_minus_as_sign=1;
				if (c=='&' && zeexpression[idx+1]=='&') {
					idx++;
					c='a'; // boolean AND
				}
				break;
			case '|':
				allow_minus_as_sign=1;
				if (c=='|' && zeexpression[idx+1]=='|') {
					idx++;
					c='o'; // boolean OR
				}
				break;
			/* testing */
			case '<':
				allow_minus_as_sign=1;
				if (zeexpression[idx+1]=='=') {
					idx++;
					c='k'; // boolean LOWEREQ
				} else if (zeexpression[idx+1]=='>') {
					idx++;
					c='n'; // boolean NOTEQUAL
				} else {
					c='l';
				}
				break;
			case '>':
				allow_minus_as_sign=1;
				if (zeexpression[idx+1]=='=') {
					idx++;
					c='h'; // boolean GREATEREQ
				} else {
					c='g';
				}
				break;
			case '!':
				allow_minus_as_sign=1;
				if (zeexpression[idx+1]=='=') {
					idx++;
					c='n'; // boolean NOTEQUAL
				} else {
					//rasm_printf(ae,"[%s:%d] ! sign must be followed by = [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
					//MaxError(ae);
					c='b';
				}
				break;
			case '=':
				allow_minus_as_sign=1;
				/* expecting == */
				if (zeexpression[idx+1]=='=') {
					idx++;
					c='e'; // boolean EQUAL
				/* except in maxam mode with a single = */
				} else if (ae->maxam) {
					c='e'; // boolean EQUAL
				/* cannot affect data inside an expression */
				} else {
					rasm_printf(ae,"[%s:%d] expression [%s] cannot set variable inside an expression\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
					MaxError(ae);
					return 0;
				}
				break;
			case '-':
				if (allow_minus_as_sign) {
					/* previous char was an opening parenthesis or an operator */
					ivar=0;
					ae->computectx->varbuffer[ivar++]='-';
					StateMachineResizeBuffer(&ae->computectx->varbuffer,ivar,&ae->computectx->maxivar);
					c=zeexpression[++idx];
					if (ae->AutomateExpressionValidCharFirst[(int)c&0xFF]) {
						ae->computectx->varbuffer[ivar++]=c;
						StateMachineResizeBuffer(&ae->computectx->varbuffer,ivar,&ae->computectx->maxivar);
						c=zeexpression[++idx];
						while (ae->AutomateExpressionValidChar[(int)c&0xFF]) {
							ae->computectx->varbuffer[ivar++]=c;
							StateMachineResizeBuffer(&ae->computectx->varbuffer,ivar,&ae->computectx->maxivar);
							c=zeexpression[++idx];
						}
					}
					ae->computectx->varbuffer[ivar]=0;
					if (ivar<2) {
						rasm_printf(ae,"[%s:%d] expression [%s] invalid minus sign\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
						MaxError(ae);
						if (!original) {
							MemFree(zeexpression);
						}
						return 0;
					}
					break;
				}
				allow_minus_as_sign=1;
				break;
				
			/* operator OR binary value */
			case '%':
				/* % symbol may be a modulo or a binary literal value */
				is_binary=0;
				for (icheck=1;zeexpression[idx+icheck];icheck++) {
					switch (zeexpression[idx+icheck]) {
						case '1':
						case '0':/* still binary */
							is_binary=1;
							break;
						case '+':
						case '-':
						case '/':
						case '*':
						case '|':
						case 'm':
						case '%':
						case '^':
						case '&':
						case '(':
						case ')':
						case '=':
						case '<':
						case '>':
						case '!':
						case '[':
						case ']':
							if (is_binary) is_binary=2; else is_binary=-1;
							break;
						default:
							is_binary=-1;
					}
					if (is_binary==2) {
						break;
					}
					if (is_binary==-1) {
						is_binary=0;
						break;
					}
				}
				if (!is_binary) {
					allow_minus_as_sign=1;
					break;
				}
			default:
				allow_minus_as_sign=0;
				/* semantic analysis */
				startvar=idx;
				ivar=0;
				/* first char does not allow same chars as next chars */
				if (ae->AutomateExpressionValidCharFirst[((int)c)&0xFF]) {
//printf("getword %c",c);fflush(stdout);
					ae->computectx->varbuffer[ivar++]=c;
					if (c=='{') {
						/* not a formula but only a prefix tag */
						curly++;
					}
					StateMachineResizeBuffer(&ae->computectx->varbuffer,ivar,&ae->computectx->maxivar);
					idx++;
					c=zeexpression[idx];
					Automate=ae->AutomateExpressionValidChar;
//printf("%c",c);fflush(stdout);
					while (Automate[((int)c)&0xFF]) {
						if (c=='{') {
							curly++;
							curlyflag=1;
//printf(" curly ");							
							Automate=ae->AutomateExpressionValidCharExtended;
						} else if (c=='}') {
							curly--;
							if (!curly) {
								Automate=ae->AutomateExpressionValidChar;
							}
						}
						ae->computectx->varbuffer[ivar++]=c;
						StateMachineResizeBuffer(&ae->computectx->varbuffer,ivar,&ae->computectx->maxivar);
						idx++;
						c=zeexpression[idx];
//printf("%c",c);fflush(stdout);
					}
				}
				ae->computectx->varbuffer[ivar]=0;
//printf(" -> [%s]\n",ae->computectx->varbuffer);
				if (!ivar) {
					rasm_printf(ae,"[%s:%d] invalid char (%d=%c) expression [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),c,c>31?c:' ',TradExpression(zeexpression));
					MaxError(ae);
					if (!original) {
						MemFree(zeexpression);
					}
					return 0;
				} else if (curly) {
					rasm_printf(ae,"[%s:%d] wrong curly brackets in expression [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
					MaxError(ae);
					if (!original) {
						MemFree(zeexpression);
					}
					return 0;
				}
		}
		if (c && !ivar) idx++;
	
		/************************************
		   S T A C K   D I S P A T C H E R
		************************************/
		/* push operator or stack value */
		if (!ivar) {
			/************************************
			          O P E R A T O R 
			************************************/
			stackelement=ae->AutomateElement[c];
			if (stackelement.operator>E_COMPUTE_OPERATION_GREATEREQ) {
				rasm_printf(ae,"[%s:%d] expression [%s] has unknown operator %c (%d)\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),c>31?c:'.',c);
				MaxError(ae);
			}
			/* stackelement.value isn't used */
		} else {
			/************************************
			              V A L U E
			************************************/
//printf("value varbuffer=[%s] c=%c\n",ae->computectx->varbuffer,c);
			if (ae->computectx->varbuffer[0]=='-') minusptr=1; else minusptr=0;
			/* constantes ou variables/labels */
			switch (ae->computectx->varbuffer[minusptr]) {
				case '0':
					/* 0x hexa value hack */
					if (ae->computectx->varbuffer[minusptr+1]=='X' && ae->AutomateHexa[ae->computectx->varbuffer[minusptr+2]]) {
						for (icheck=minusptr+3;ae->computectx->varbuffer[icheck];icheck++) {
							if (ae->AutomateHexa[ae->computectx->varbuffer[icheck]]) continue;
							rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid hex number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
							MaxError(ae);
							break;
						}
						curval=strtol(ae->computectx->varbuffer+minusptr+2,NULL,16);
						break;
					} else
					/* 0b binary value hack */
					if (ae->computectx->varbuffer[minusptr+1]=='B' && (ae->computectx->varbuffer[minusptr+2]>='0' && ae->computectx->varbuffer[minusptr+2]<='1')) {
						for (icheck=minusptr+3;ae->computectx->varbuffer[icheck];icheck++) {
							if (ae->computectx->varbuffer[icheck]>='0' && ae->computectx->varbuffer[icheck]<='1') continue;
							rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid binary number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
							MaxError(ae);
							break;
						}
						curval=strtol(ae->computectx->varbuffer+minusptr+2,NULL,2);
						break;
					}
					/* 0o octal value hack */
					if (ae->computectx->varbuffer[minusptr+1]=='O' && (ae->computectx->varbuffer[minusptr+2]>='0' && ae->computectx->varbuffer[minusptr+2]<='5')) {
						for (icheck=minusptr+3;ae->computectx->varbuffer[icheck];icheck++) {
							if (ae->computectx->varbuffer[icheck]>='0' && ae->computectx->varbuffer[icheck]<='5') continue;
							rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid octal number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
							MaxError(ae);
							break;
						}
						curval=strtol(ae->computectx->varbuffer+minusptr+2,NULL,2);
						break;
					}
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					/* check number */
					for (icheck=minusptr;ae->computectx->varbuffer[icheck];icheck++) {
						if (ae->AutomateDigit[ae->computectx->varbuffer[icheck]]) continue;
						/* Intel hexa & binary style */
						switch (ae->computectx->varbuffer[strlen(ae->computectx->varbuffer)-1]) {
							case 'H':
								for (icheck=minusptr;ae->computectx->varbuffer[icheck+1];icheck++) {
									if (ae->AutomateHexa[ae->computectx->varbuffer[icheck]]) continue;
									rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid hex number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
									MaxError(ae);
								}
								curval=strtol(ae->computectx->varbuffer+minusptr,NULL,16);
								break;
							case 'B':
								for (icheck=minusptr;ae->computectx->varbuffer[icheck+1];icheck++) {
									if (ae->computectx->varbuffer[icheck]=='0' || ae->computectx->varbuffer[icheck]=='1') continue;
									rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid binary number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
									MaxError(ae);
								}
								curval=strtol(ae->computectx->varbuffer+minusptr,NULL,2);
								break;
							default:
								rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
								MaxError(ae);
						}
						icheck=0;
						break;
					}
					if (!ae->computectx->varbuffer[icheck]) curval=atof(ae->computectx->varbuffer+minusptr);
					break;
				case '%':
					/* check number */
					if (!ae->computectx->varbuffer[minusptr+1]) {
						rasm_printf(ae,"[%s:%d] expression [%s] - %s is an empty binary number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
						MaxError(ae);
					}
					for (icheck=minusptr+1;ae->computectx->varbuffer[icheck];icheck++) {
						if (ae->computectx->varbuffer[icheck]=='0' || ae->computectx->varbuffer[icheck]=='1') continue;
						rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid binary number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
						MaxError(ae);
						break;
					}
					curval=strtol(ae->computectx->varbuffer+minusptr+1,NULL,2);
					break;
				case '#':
					/* check number */
					if (!ae->computectx->varbuffer[minusptr+1]) {
						rasm_printf(ae,"[%s:%d] expression [%s] - %s is an empty hex number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
						MaxError(ae);
					}
					for (icheck=minusptr+1;ae->computectx->varbuffer[icheck];icheck++) {
						if (ae->AutomateHexa[ae->computectx->varbuffer[icheck]]) continue;
						rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid hex number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
						MaxError(ae);
						break;
					}
					curval=strtol(ae->computectx->varbuffer+minusptr+1,NULL,16);
					break;
				default:
//printf("default curlyflag=%d\n",curlyflag);	
					if (1 || !curlyflag) {
						/* $ hex value hack */
						if (ae->computectx->varbuffer[minusptr+0]=='$' && ae->AutomateHexa[ae->computectx->varbuffer[minusptr+1]]) {
							for (icheck=minusptr+2;ae->computectx->varbuffer[icheck];icheck++) {
								if (ae->AutomateHexa[ae->computectx->varbuffer[icheck]]) continue;
								rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid hex number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
								MaxError(ae);
								break;
							}
							curval=strtol(ae->computectx->varbuffer+minusptr+1,NULL,16);
							break;
						}
						/* @ octal value hack */
						if (ae->computectx->varbuffer[minusptr+0]=='@' &&  ((ae->computectx->varbuffer[minusptr+1]>='0' && ae->computectx->varbuffer[minusptr+1]<='7'))) {
							for (icheck=minusptr+2;ae->computectx->varbuffer[icheck];icheck++) {
								if (ae->computectx->varbuffer[icheck]>='0' && ae->computectx->varbuffer[icheck]<='7') continue;
								rasm_printf(ae,"[%s:%d] expression [%s] - %s is not a valid octal number\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
								MaxError(ae);
								break;
							}
							curval=strtol(ae->computectx->varbuffer+minusptr+1,NULL,8);
							break;
						}
						/* Intel hexa value hack */
						if (ae->AutomateHexa[ae->computectx->varbuffer[minusptr+0]]) {
							if (ae->computectx->varbuffer[strlen(ae->computectx->varbuffer)-1]=='H') {
								for (icheck=minusptr;ae->computectx->varbuffer[icheck+1];icheck++) {
									if (!ae->AutomateHexa[ae->computectx->varbuffer[icheck]]) break;
								}
								if (!ae->computectx->varbuffer[icheck+1]) {
									curval=strtol(ae->computectx->varbuffer+minusptr,NULL,16);
									break;
								}
							}
						}
					}
					
					
					if (curlyflag) {
						char *minivarbuffer;
						int touched;

//printf("in: zexpr->[%s] varbuffer=[%s]\n",zeexpression,ae->computectx->varbuffer);
						/* besoin d'un sous-contexte */
						minivarbuffer=TxtStrDup(ae->computectx->varbuffer+minusptr);
						ae->computectx=&ae->ctx2;
						minivarbuffer=TranslateTag(ae,minivarbuffer, &touched,0,E_TAGOPTION_NONE);
						ae->computectx=&ae->ctx1;
						if (!touched) {
							strcpy(ae->computectx->varbuffer+minusptr,minivarbuffer);
						} else {
							StateMachineResizeBuffer(&ae->computectx->varbuffer,strlen(minivarbuffer)+2,&ae->computectx->maxivar);
							strcpy(ae->computectx->varbuffer+minusptr,minivarbuffer);
						}
						MemFree(minivarbuffer);
						curlyflag=0;
//printf("out:zexpr->[%s] varbuffer=[%s]\n",zeexpression,ae->computectx->varbuffer);
					}

//printf("getcrc [%s]\n",ae->computectx->varbuffer);					
					crc=GetCRC(ae->computectx->varbuffer+minusptr);
					/***************************************************
					     L O O K I N G   F O R   A   F U N C T I O N
					***************************************************/
					for (imkey=0;math_keyword[imkey].mnemo[0];imkey++) {
						if (crc==math_keyword[imkey].crc && strcmp(ae->computectx->varbuffer+minusptr,math_keyword[imkey].mnemo)==0) {
							if (c=='(') {
								/* push function as operator! */
								stackelement.operator=math_keyword[imkey].operation;
								//stackelement.priority=0;
								/************************************************
								      C R E A T E    E X T R A     T O K E N
								************************************************/
								ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->tokenstack,&nbtokenstack,&ae->computectx->maxtokenstack,&stackelement,sizeof(stackelement));
								stackelement.operator=E_COMPUTE_OPERATION_OPEN;
								ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->tokenstack,&nbtokenstack,&ae->computectx->maxtokenstack,&stackelement,sizeof(stackelement));
								allow_minus_as_sign=1;
								idx++;
							} else {
								rasm_printf(ae,"[%s:%d] expression [%s] - %s is a reserved keyword!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),math_keyword[imkey].mnemo);
								MaxError(ae);
								curval=0;
								idx++;
							}
							ivar=0;
							break;
						}
					}
					if (math_keyword[imkey].mnemo[0]) continue;
					
					if (ae->computectx->varbuffer[minusptr+0]=='$' && ae->computectx->varbuffer[minusptr+1]==0) {
						curval=ptr;
					} else {
						curdic=SearchDico(ae,ae->computectx->varbuffer+minusptr,crc);
						if (curdic) {
							curval=curdic->v;
							break;
						} else {
							/* getbank hack */
							if (ae->computectx->varbuffer[minusptr]!='{') {
								bank=0;
								page=0;
							} else if (strncmp(ae->computectx->varbuffer+minusptr,"{BANK}",6)==0) {
								bank=6;
								page=0;
								/* oblig� de recalculer le CRC */
								crc=GetCRC(ae->computectx->varbuffer+minusptr+bank);
							} else if (strncmp(ae->computectx->varbuffer+minusptr,"{PAGE}",6)==0) {
								bank=6;
								page=1;
								/* oblig� de recalculer le CRC */
								crc=GetCRC(ae->computectx->varbuffer+minusptr+bank);
							} else if (strncmp(ae->computectx->varbuffer+minusptr,"{PAGESET}",9)==0) {
								bank=9;
								page=2;
								/* oblig� de recalculer le CRC */
								crc=GetCRC(ae->computectx->varbuffer+minusptr+bank);
							} else if (strncmp(ae->computectx->varbuffer+minusptr,"{SIZEOF}",8)==0) {
								bank=8;
								page=3;
								/* oblig� de recalculer le CRC */
								crc=GetCRC(ae->computectx->varbuffer+minusptr+bank);
								/* search in structures prototypes and subfields */
								for (i=0;i<ae->irasmstruct;i++) {
									if (ae->rasmstruct[i].crc==crc && strcmp(ae->rasmstruct[i].name,ae->computectx->varbuffer+minusptr+bank)==0) {
										curval=ae->rasmstruct[i].size;
										break;
									}

									for (j=0;j<ae->rasmstruct[i].irasmstructfield;j++) {
										if (ae->rasmstruct[i].rasmstructfield[j].crc==crc && strcmp(ae->rasmstruct[i].rasmstructfield[j].fullname,ae->computectx->varbuffer+minusptr+bank)==0) {
											curval=ae->rasmstruct[i].rasmstructfield[j].size;
											i=ae->irasmstruct+1;
											break;
										}
									}
								}

								if (i==ae->irasmstruct) {
									/* search in structures aliases */
									for (i=0;i<ae->irasmstructalias;i++) {
										if (ae->rasmstructalias[i].crc==crc && strcmp(ae->rasmstructalias[i].name,ae->computectx->varbuffer+minusptr+bank)==0) {
											curval=ae->rasmstructalias[i].size;
											break;
										}
									}
									if (i==ae->irasmstructalias) {
										rasm_printf(ae,"[%s:%d] cannot SIZEOF unknown structure [%s]!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),ae->computectx->varbuffer+minusptr+bank);
										MaxError(ae);
										curval=0;
									}
								}
							} else {
								rasm_printf(ae,"[%s:%d] expression [%s] - %s is an unknown prefix!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
								MaxError(ae);
							}
							/* limited label translation while processing crunched blocks
							   ae->curlz == current crunched block processed
							   expression->crunch_block=0 -> oui
							   expression->crunch_block=1 -> oui si m�me block
							   expression->crunch_block=2 -> non car sera relog�e
							*/
							if (page!=3) {
								curlabel=SearchLabel(ae,ae->computectx->varbuffer+minusptr+bank,crc);
								if (curlabel) {
									if (ae->stage<2) {
										if (curlabel->lz==-1) {
											if (!bank) {
												curval=curlabel->ptr;
											} else {
//printf("page=%d | ptr=%X ibank=%d\n",page,curlabel->ptr,curlabel->ibank);
												switch (page) {
													case 2: /* PAGESET */
														if (curlabel->ibank<36) {
															if (curlabel->ibank>3) curval=((curlabel->ibank>>2)-1)*8+0xC2; else curval=0xC0;
														} else {
															rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGESET - label [%s] is in a temporary space!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
															MaxError(ae);
															curval=curlabel->ibank;
														}
														break;
													case 1:/* PAGE */
														if (curlabel->ibank<36) {
																if (ae->bankset[curlabel->ibank>>2]) {
																	curval=ae->bankgate[(curlabel->ibank&0xFC)+(curlabel->ptr>>14)];
																} else {
																	curval=ae->bankgate[curlabel->ibank];
																}
														} else {
															rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGE - label [%s] is in a temporary space!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
															MaxError(ae);
															curval=curlabel->ibank;
														}
														break;
													case 0:curval=curlabel->ibank;break;
													default:rasm_printf(ae,"[%s:%d] INTERNAL ERROR (unknown paging)\n",GetExpFile(ae,didx),GetExpLine(ae,didx));FreeAssenv(ae);exit(-664);
												}
											}
										} else {
											/* label MUST be in the crunched block */
											if (curlabel->iorgzone==ae->expression[didx].iorgzone && curlabel->ibank==ae->expression[didx].ibank && curlabel->lz<=ae->expression[didx].lz) {
												if (!bank) {
													curval=curlabel->ptr;
												} else {
													if (page) {
														switch (page) {
															case 2:
																if (curlabel->ibank<36) {
																	curval=(curlabel->ibank>>2)*8+2+0xC0;
																} else {
																	rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGESET - label [%s] is in a temporary space!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
																	MaxError(ae);
																	curval=curlabel->ibank;
																}
																break;
															case 1:
																if (curlabel->ibank<36) {
																	if (ae->bankset[curlabel->ibank>>2]) {
																		curval=ae->bankgate[(curlabel->ibank&0xFC)+(curlabel->ptr>>14)];
																	} else {
																		curval=ae->bankgate[curlabel->ibank];
																	}
																} else {
																	rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGE - label [%s] is in a temporary space!\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer);
																	MaxError(ae);
																	curval=curlabel->ibank;
																}
																break;
															case 0:curval=curlabel->ibank;break;
															default:rasm_printf(ae,"[%s:%d] INTERNAL ERROR (unknown paging)\n",GetExpFile(ae,didx),GetExpLine(ae,didx));FreeAssenv(ae);exit(-664);
														}
													}
												}
											} else {
												rasm_printf(ae,"[%s:%d] Label [%s](%d) cannot be computed because it is located after the crunched zone %d\n",GetExpFile(ae,didx),GetExpLine(ae,didx),ae->computectx->varbuffer,curlabel->lz,ae->expression[didx].lz);
												MaxError(ae);
												curval=0;
											}
										}
									} else {
										if (bank) {
											curval=curlabel->ibank;
										} else {
											curval=curlabel->ptr;
										}
									}
								} else {
									/***********
										to allow aliases declared after use
									***********/
									if ((ialias=SearchAlias(ae,crc,ae->computectx->varbuffer+minusptr))>=0) {
										newlen=ae->alias[ialias].len;
										lenw=strlen(zeexpression);
										if (newlen>ivar) {
											/* realloc bigger */
											if (original) {
												expr=MemMalloc(lenw+newlen-ivar+1);
												memcpy(expr,zeexpression,lenw+1);
												zeexpression=expr;
												original=0;
											} else {
												zeexpression=MemRealloc(zeexpression,lenw+newlen-ivar+1);
											}
										}
										/* startvar? */
										if (newlen!=ivar) {
											MemMove(zeexpression+startvar+newlen,zeexpression+startvar+ivar,lenw-startvar-ivar+1);
										}
										strncpy(zeexpression+startvar,ae->alias[ialias].translation,newlen); /* copy without zero terminator */
										idx=startvar;
										ivar=0;
										continue;
									} else {
										/* last chance to get a keyword */
										if (strcmp(ae->computectx->varbuffer+minusptr,"REPEAT_COUNTER")==0) {
											if (ae->ir) {
												curval=ae->repeat[ae->ir-1].repeat_counter;
											} else {
												rasm_printf(ae,"[%s:%d] cannot use REPEAT_COUNTER keyword outside a repeat loop\n",GetExpFile(ae,didx),GetExpLine(ae,didx));
												MaxError(ae);
												curval=0;
											}
										} else if (strcmp(ae->computectx->varbuffer+minusptr,"WHILE_COUNTER")==0) {
											if (ae->iw) {
												curval=ae->whilewend[ae->iw-1].while_counter;
											} else {
												rasm_printf(ae,"[%s:%d] cannot use WHILE_COUNTER keyword outside a while loop\n",GetExpFile(ae,didx),GetExpLine(ae,didx));
												MaxError(ae);
												curval=0;
											}
										} else {
											/* in case the expression is a register */
											if (IsRegister(ae->computectx->varbuffer+minusptr)) {
												rasm_printf(ae,"[%s:%d] cannot use register %s in this context\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
											} else {
												rasm_printf(ae,"[%s:%d] expression [%s] keyword [%s] not found in variables, labels or aliases\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression),ae->computectx->varbuffer+minusptr);
												if (ae->extended_error) {
													char *lookstr;
													lookstr=StringLooksLike(ae,ae->computectx->varbuffer+minusptr);
													if (lookstr) {
														rasm_printf(ae," did you mean [%s] ?\n",lookstr);
													}
												}
											}
											MaxError(ae);
											curval=0;
										}
									}
								}
							}
						}
					}
			}
			if (minusptr) curval=-curval;
			stackelement.operator=E_COMPUTE_OPERATION_PUSH_DATASTC;
			stackelement.value=curval;
			/* priority isn't used */
			
			allow_minus_as_sign=0;
			ivar=0;
		}
		/************************************
		      C R E A T E    T O K E N
		************************************/
		ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->tokenstack,&nbtokenstack,&ae->computectx->maxtokenstack,&stackelement,sizeof(stackelement));
	}
	/*******************************************************
	      C R E A T E    E X E C U T I O N    S T A C K
	*******************************************************/
#define DEBUG_STACK 0
#if DEBUG_STACK
	for (itoken=0;itoken<nbtokenstack;itoken++) {
		switch (ae->computectx->tokenstack[itoken].operator) {
			case E_COMPUTE_OPERATION_PUSH_DATASTC:printf("%lf ",ae->computectx->tokenstack[itoken].value);break;
			case E_COMPUTE_OPERATION_OPEN:printf("(");break;
			case E_COMPUTE_OPERATION_CLOSE:printf(")");break;
			case E_COMPUTE_OPERATION_ADD:printf("+ ");break;
			case E_COMPUTE_OPERATION_SUB:printf("- ");break;
			case E_COMPUTE_OPERATION_DIV:printf("/ ");break;
			case E_COMPUTE_OPERATION_MUL:printf("* ");break;
			case E_COMPUTE_OPERATION_AND:printf("and ");break;
			case E_COMPUTE_OPERATION_OR:printf("or ");break;
			case E_COMPUTE_OPERATION_MOD:printf("mod ");break;
			case E_COMPUTE_OPERATION_XOR:printf("xor ");break;
			case E_COMPUTE_OPERATION_NOT:printf("! ");break;
			case E_COMPUTE_OPERATION_SHL:printf("<< ");break;
			case E_COMPUTE_OPERATION_SHR:printf(">> ");break;
			case E_COMPUTE_OPERATION_BAND:printf("&& ");break;
			case E_COMPUTE_OPERATION_BOR:printf("|| ");break;
			case E_COMPUTE_OPERATION_LOWER:printf("< ");break;
			case E_COMPUTE_OPERATION_GREATER:printf("> ");break;
			case E_COMPUTE_OPERATION_EQUAL:printf("== ");break;
			case E_COMPUTE_OPERATION_NOTEQUAL:printf("!= ");break;
			case E_COMPUTE_OPERATION_LOWEREQ:printf("<= ");break;
			case E_COMPUTE_OPERATION_GREATEREQ:printf(">= ");break;
			case E_COMPUTE_OPERATION_SIN:printf("sin ");break;
			case E_COMPUTE_OPERATION_COS:printf("cos ");break;
			case E_COMPUTE_OPERATION_INT:printf("int ");break;
			case E_COMPUTE_OPERATION_FLOOR:printf("floor ");break;
			case E_COMPUTE_OPERATION_ABS:printf("abs ");break;
			case E_COMPUTE_OPERATION_LN:printf("ln ");break;
			case E_COMPUTE_OPERATION_LOG10:printf("log10 ");break;
			case E_COMPUTE_OPERATION_SQRT:printf("sqrt ");break;
			case E_COMPUTE_OPERATION_ASIN:printf("asin ");break;
			case E_COMPUTE_OPERATION_ACOS:printf("acos ");break;
			case E_COMPUTE_OPERATION_ATAN:printf("atan ");break;
			case E_COMPUTE_OPERATION_EXP:printf("exp ");break;
			case E_COMPUTE_OPERATION_LOW:printf("low ");break;
			case E_COMPUTE_OPERATION_HIGH:printf("high ");break;
			case E_COMPUTE_OPERATION_PSG:printf("psg ");break;
			case E_COMPUTE_OPERATION_RND:printf("rnd ");break;
			default:printf("bug\n");break;
		}
		
	}
	printf("\n");
#endif

	for (itoken=0;itoken<nbtokenstack;itoken++) {
		switch (ae->computectx->tokenstack[itoken].operator) {
			case E_COMPUTE_OPERATION_PUSH_DATASTC:
#if DEBUG_STACK
printf("data\n");
#endif
				ObjectArrayAddDynamicValueConcat((void **)&computestack,&nbcomputestack,&maxcomputestack,&ae->computectx->tokenstack[itoken],sizeof(stackelement));
				break;
			case E_COMPUTE_OPERATION_OPEN:
				ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->operatorstack,&nboperatorstack,&ae->computectx->maxoperatorstack,&ae->computectx->tokenstack[itoken],sizeof(stackelement));
#if DEBUG_STACK
printf("ajout (\n");
#endif
				break;
			case E_COMPUTE_OPERATION_CLOSE:
#if DEBUG_STACK
printf("close\n");
#endif
				/* pop out token until the opened parenthesis is reached */
				o2=nboperatorstack-1;
				okclose=0;
				while (o2>=0) {
					if (ae->computectx->operatorstack[o2].operator!=E_COMPUTE_OPERATION_OPEN) {
						ObjectArrayAddDynamicValueConcat((void **)&computestack,&nbcomputestack,&maxcomputestack,&ae->computectx->operatorstack[o2],sizeof(stackelement));
						nboperatorstack--;
#if DEBUG_STACK
printf("op--\n");
#endif
						o2--;
					} else {
						/* discard opening parenthesis as operator */
#if DEBUG_STACK
printf("discard )\n");
#endif
						nboperatorstack--;
						okclose=1;
						o2--;
						break;
					}
				}
				if (!okclose) {
					rasm_printf(ae,"[%s:%d] missing parenthesis [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
					MaxError(ae);
					if (!original) {
						MemFree(zeexpression);
					}
					return 0;
				}
				/* if upper token is a function then pop from the stack */
				if (o2>=0 && ae->computectx->operatorstack[o2].operator>=E_COMPUTE_OPERATION_SIN) {
					ObjectArrayAddDynamicValueConcat((void **)&computestack,&nbcomputestack,&maxcomputestack,&ae->computectx->operatorstack[o2],sizeof(stackelement));
					nboperatorstack--;
#if DEBUG_STACK
printf("pop function\n");
#endif
				}
				break;
			case E_COMPUTE_OPERATION_ADD:
			case E_COMPUTE_OPERATION_SUB:
			case E_COMPUTE_OPERATION_DIV:
			case E_COMPUTE_OPERATION_MUL:
			case E_COMPUTE_OPERATION_AND:
			case E_COMPUTE_OPERATION_OR:
			case E_COMPUTE_OPERATION_MOD:
			case E_COMPUTE_OPERATION_XOR:
			case E_COMPUTE_OPERATION_NOT:
			case E_COMPUTE_OPERATION_SHL:
			case E_COMPUTE_OPERATION_SHR:
			case E_COMPUTE_OPERATION_BAND:
			case E_COMPUTE_OPERATION_BOR:
			case E_COMPUTE_OPERATION_LOWER:
			case E_COMPUTE_OPERATION_GREATER:
			case E_COMPUTE_OPERATION_EQUAL:
			case E_COMPUTE_OPERATION_NOTEQUAL:
			case E_COMPUTE_OPERATION_LOWEREQ:
			case E_COMPUTE_OPERATION_GREATEREQ:
#if DEBUG_STACK
printf("operator\n");
#endif
				o2=nboperatorstack-1;
				while (o2>=0 && ae->computectx->operatorstack[o2].operator!=E_COMPUTE_OPERATION_OPEN) {
					if (ae->computectx->tokenstack[itoken].priority>=ae->computectx->operatorstack[o2].priority || ae->computectx->operatorstack[o2].operator>=E_COMPUTE_OPERATION_SIN) {
						ObjectArrayAddDynamicValueConcat((void **)&computestack,&nbcomputestack,&maxcomputestack,&ae->computectx->operatorstack[o2],sizeof(stackelement));
						nboperatorstack--;
						o2--;
					} else {
						break;
					}
				}
				ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->operatorstack,&nboperatorstack,&ae->computectx->maxoperatorstack,&ae->computectx->tokenstack[itoken],sizeof(stackelement));
				break;
			case E_COMPUTE_OPERATION_SIN:
			case E_COMPUTE_OPERATION_COS:
			case E_COMPUTE_OPERATION_INT:
			case E_COMPUTE_OPERATION_FLOOR:
			case E_COMPUTE_OPERATION_ABS:
			case E_COMPUTE_OPERATION_LN:
			case E_COMPUTE_OPERATION_LOG10:
			case E_COMPUTE_OPERATION_SQRT:
			case E_COMPUTE_OPERATION_ASIN:
			case E_COMPUTE_OPERATION_ACOS:
			case E_COMPUTE_OPERATION_ATAN:
			case E_COMPUTE_OPERATION_EXP:
			case E_COMPUTE_OPERATION_LOW:
			case E_COMPUTE_OPERATION_HIGH:
			case E_COMPUTE_OPERATION_PSG:
			case E_COMPUTE_OPERATION_RND:
#if DEBUG_STACK
printf("ajout de la fonction\n");
#endif
				ObjectArrayAddDynamicValueConcat((void **)&ae->computectx->operatorstack,&nboperatorstack,&ae->computectx->maxoperatorstack,&ae->computectx->tokenstack[itoken],sizeof(stackelement));
				break;
			default:break;
		}
	}
	/* pop remaining operators */
	while (nboperatorstack>0) {
		ObjectArrayAddDynamicValueConcat((void **)&computestack,&nbcomputestack,&maxcomputestack,&ae->computectx->operatorstack[--nboperatorstack],sizeof(stackelement));
	}
	
	/********************************************
	        E X E C U T E        S T A C K
	********************************************/
	if (ae->maxam || ae->as80) {
		int workinterval;
		if (ae->as80) workinterval=0xFFFFFFFF; else workinterval=0xFFFF;
		for (i=0;i<nbcomputestack;i++) {
			switch (computestack[i].operator) {
				/************************************************
				  c a s e s   s h o u l d    b e    s o r t e d
				************************************************/
				case E_COMPUTE_OPERATION_PUSH_DATASTC:
					if (maccu<=paccu) {
						maccu=16+paccu;
						accu=MemRealloc(accu,sizeof(double)*maccu);
					}
					accu[paccu]=computestack[i].value;paccu++;
					break;
				case E_COMPUTE_OPERATION_OPEN:
				case E_COMPUTE_OPERATION_CLOSE:/* cannot happend */ break;
				case E_COMPUTE_OPERATION_ADD:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]+(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_SUB:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]-(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_MUL:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]*(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_DIV:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]/(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_AND:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_OR:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]|(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_XOR:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]^(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_MOD:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]%(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_SHL:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2])<<((int)accu[paccu-1]);paccu--;break;
				case E_COMPUTE_OPERATION_SHR:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2])>>((int)accu[paccu-1]);paccu--;break;				
				case E_COMPUTE_OPERATION_BAND:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&&(int)accu[paccu-1])&workinterval;paccu--;break;
				case E_COMPUTE_OPERATION_BOR:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]||(int)accu[paccu-1])&workinterval;paccu--;break;
				/* comparison */
				case E_COMPUTE_OPERATION_LOWER:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)<((int)accu[paccu-1]&workinterval);paccu--;break;
				case E_COMPUTE_OPERATION_LOWEREQ:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)<=((int)accu[paccu-1]&workinterval);paccu--;break;
				case E_COMPUTE_OPERATION_EQUAL:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)==((int)accu[paccu-1]&workinterval);paccu--;break;
				case E_COMPUTE_OPERATION_NOTEQUAL:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)!=((int)accu[paccu-1]&workinterval);paccu--;break;
				case E_COMPUTE_OPERATION_GREATER:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)>((int)accu[paccu-1]&workinterval);paccu--;break;
				case E_COMPUTE_OPERATION_GREATEREQ:if (paccu>1) accu[paccu-2]=((int)accu[paccu-2]&workinterval)>=((int)accu[paccu-1]&workinterval);paccu--;break;
				/* functions */
				case E_COMPUTE_OPERATION_SIN:if (paccu>0) accu[paccu-1]=(int)sin(accu[paccu-1]*3.1415926545/180.0);break;
				case E_COMPUTE_OPERATION_COS:if (paccu>0) accu[paccu-1]=(int)cos(accu[paccu-1]*3.1415926545/180.0);break;
				case E_COMPUTE_OPERATION_ASIN:if (paccu>0) accu[paccu-1]=(int)asin(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_ACOS:if (paccu>0) accu[paccu-1]=(int)acos(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_ATAN:if (paccu>0) accu[paccu-1]=(int)atan(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_INT:break;
				case E_COMPUTE_OPERATION_FLOOR:if (paccu>0) accu[paccu-1]=(int)floor(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_ABS:if (paccu>0) accu[paccu-1]=(int)fabs(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_EXP:if (paccu>0) accu[paccu-1]=(int)exp(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_LN:if (paccu>0) accu[paccu-1]=(int)log(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_LOG10:if (paccu>0) accu[paccu-1]=(int)log10(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_SQRT:if (paccu>0) accu[paccu-1]=(int)sqrt(accu[paccu-1])&workinterval;break;
				case E_COMPUTE_OPERATION_LOW:if (paccu>0) accu[paccu-1]=((int)accu[paccu-1])&0xFF;break;
				case E_COMPUTE_OPERATION_HIGH:if (paccu>0) accu[paccu-1]=(((int)accu[paccu-1])&0xFF00)>>8;break;
				case E_COMPUTE_OPERATION_PSG:if (paccu>0) accu[paccu-1]=ae->psgfine[((int)accu[paccu-1])&0xFF];break;
				case E_COMPUTE_OPERATION_RND:if (paccu>0) accu[paccu-1]=rand()%((int)accu[paccu-1]);break;
				default:rasm_printf(ae,"[%s:%d] invalid computing state! (%d)\n",GetExpFile(ae,didx),GetExpLine(ae,didx),computestack[i].operator);paccu=0;
			}
			if (!paccu) {
				rasm_printf(ae,"[%s:%d] Missing operande for calculation [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
				MaxError(ae);
				break;
			}
		}
	} else {
		for (i=0;i<nbcomputestack;i++) {
#if 0
			int kk;
			for (kk=0;kk<paccu;kk++) printf("stack[%d]=%lf\n",kk,accu[kk]);
			if (computestack[i].operator==E_COMPUTE_OPERATION_PUSH_DATASTC) {
				printf("pacc=%d push %.1lf\n",paccu,computestack[i].value);
			} else {
				printf("pacc=%d operation %s p=%d\n",paccu,computestack[i].operator==E_COMPUTE_OPERATION_MUL?"*":
								computestack[i].operator==E_COMPUTE_OPERATION_ADD?"+":
								computestack[i].operator==E_COMPUTE_OPERATION_DIV?"/":
								computestack[i].operator==E_COMPUTE_OPERATION_SUB?"-":
								computestack[i].operator==E_COMPUTE_OPERATION_BAND?"&&":
								computestack[i].operator==E_COMPUTE_OPERATION_BOR?"||":
								computestack[i].operator==E_COMPUTE_OPERATION_SHL?"<<":
								computestack[i].operator==E_COMPUTE_OPERATION_SHR?">>":
								computestack[i].operator==E_COMPUTE_OPERATION_LOWER?"<":
								computestack[i].operator==E_COMPUTE_OPERATION_GREATER?">":
								computestack[i].operator==E_COMPUTE_OPERATION_EQUAL?"==":
								computestack[i].operator==E_COMPUTE_OPERATION_INT?"INT":
								computestack[i].operator==E_COMPUTE_OPERATION_LOWEREQ?"<=":
								computestack[i].operator==E_COMPUTE_OPERATION_GREATEREQ?">=":
								computestack[i].operator==E_COMPUTE_OPERATION_OPEN?"(":
								computestack[i].operator==E_COMPUTE_OPERATION_CLOSE?")":
								"<autre>",computestack[i].priority);
			}
#endif
			switch (computestack[i].operator) {
				case E_COMPUTE_OPERATION_PUSH_DATASTC:
					if (maccu<=paccu) {
						maccu=16+paccu;
						accu=MemRealloc(accu,sizeof(double)*maccu);
					}
					accu[paccu]=computestack[i].value;paccu++;
					break;
				case E_COMPUTE_OPERATION_OPEN:
				case E_COMPUTE_OPERATION_CLOSE: /* cannot happend */ break;
				case E_COMPUTE_OPERATION_ADD:if (paccu>1) accu[paccu-2]+=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_SUB:if (paccu>1) accu[paccu-2]-=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_MUL:if (paccu>1) accu[paccu-2]*=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_DIV:if (paccu>1) accu[paccu-2]/=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_AND:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))&((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_OR:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))|((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_XOR:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))^((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_NOT:/* half operator, half function */ if (paccu>0) accu[paccu-1]=!((int)floor(accu[paccu-1]+0.5));break;
				case E_COMPUTE_OPERATION_MOD:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))%((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_SHL:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))<<((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_SHR:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))>>((int)floor(accu[paccu-1]+0.5));paccu--;break;				
				case E_COMPUTE_OPERATION_BAND:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))&&((int)floor(accu[paccu-1]+0.5));paccu--;break;
				case E_COMPUTE_OPERATION_BOR:if (paccu>1) accu[paccu-2]=((int)floor(accu[paccu-2]+0.5))||((int)floor(accu[paccu-1]+0.5));paccu--;break;
				/* comparison */
				case E_COMPUTE_OPERATION_LOWER:if (paccu>1) accu[paccu-2]=accu[paccu-2]<accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_LOWEREQ:if (paccu>1) accu[paccu-2]=accu[paccu-2]<=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_EQUAL:if (paccu>1) accu[paccu-2]=fabs(accu[paccu-2]-accu[paccu-1])<0.000001;paccu--;break;
				case E_COMPUTE_OPERATION_NOTEQUAL:if (paccu>1) accu[paccu-2]=accu[paccu-2]!=accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_GREATER:if (paccu>1) accu[paccu-2]=accu[paccu-2]>accu[paccu-1];paccu--;break;
				case E_COMPUTE_OPERATION_GREATEREQ:if (paccu>1) accu[paccu-2]=accu[paccu-2]>=accu[paccu-1];paccu--;break;
				/* functions */
				case E_COMPUTE_OPERATION_SIN:if (paccu>0) accu[paccu-1]=sin(accu[paccu-1]*3.1415926545/180.0);break;
				case E_COMPUTE_OPERATION_COS:if (paccu>0) accu[paccu-1]=cos(accu[paccu-1]*3.1415926545/180.0);break;
				case E_COMPUTE_OPERATION_ASIN:if (paccu>0) accu[paccu-1]=asin(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_ACOS:if (paccu>0) accu[paccu-1]=acos(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_ATAN:if (paccu>0) accu[paccu-1]=atan(accu[paccu-1])*180.0/3.1415926545;break;
				case E_COMPUTE_OPERATION_INT:if (paccu>0) accu[paccu-1]=floor(accu[paccu-1]+0.5);break;
				case E_COMPUTE_OPERATION_FLOOR:if (paccu>0) accu[paccu-1]=floor(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_ABS:if (paccu>0) accu[paccu-1]=fabs(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_EXP:if (paccu>0) accu[paccu-1]=exp(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_LN:if (paccu>0) accu[paccu-1]=log(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_LOG10:if (paccu>0) accu[paccu-1]=log10(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_SQRT:if (paccu>0) accu[paccu-1]=sqrt(accu[paccu-1]);break;
				case E_COMPUTE_OPERATION_LOW:if (paccu>0) accu[paccu-1]=((int)floor(accu[paccu-1]+0.5))&0xFF;break;
				case E_COMPUTE_OPERATION_HIGH:if (paccu>0) accu[paccu-1]=(((int)floor(accu[paccu-1]+0.5))&0xFF00)>>8;break;
				case E_COMPUTE_OPERATION_PSG:if (paccu>0) accu[paccu-1]=ae->psgfine[((int)floor(accu[paccu-1]+0.5))&0xFF];break;
				case E_COMPUTE_OPERATION_RND:if (paccu>0) accu[paccu-1]=rand()%((int)accu[paccu-1]);break;
				default:rasm_printf(ae,"[%s:%d] invalid computing state! (%d)\n",GetExpFile(ae,didx),GetExpLine(ae,didx),computestack[i].operator);paccu=0;
			}
			if (!paccu) {
				rasm_printf(ae,"[%s:%d] Missing operande for calculation [%s]\n",GetExpFile(ae,didx),GetExpLine(ae,didx),TradExpression(zeexpression));
				MaxError(ae);
				break;
			}
		}
	}
	if (!original) {
		MemFree(zeexpression);
	}
	if (paccu==1) {
		return accu[0];
	} else {
		if (paccu) {
			rasm_printf(ae,"[%s:%d] Missing operator\n",GetExpFile(ae,didx),GetExpLine(ae,didx));
			MaxError(ae);
		} else {
			rasm_printf(ae,"[%s:%d] Missing operande for calculation\n",GetExpFile(ae,didx),GetExpLine(ae,didx));
			MaxError(ae);
		}
		return accu[paccu-1];
	}
}
int RoundComputeExpressionCore(struct s_assenv *ae,char *zeexpression,int ptr,int didx) {
	return floor(ComputeExpressionCore(ae,zeexpression,ptr,didx)+ae->rough);
}

void ExpressionSetDicoVar(struct s_assenv *ae,char *name, double v)
{
	#undef FUNC
	#define FUNC "ExpressionSetDicoVar"

	struct s_expr_dico curdic;
	curdic.name=TxtStrDup(name);
	curdic.crc=GetCRC(name);
	curdic.v=v;
	//ObjectArrayAddDynamicValueConcat((void**)&ae->dico,&ae->idic,&ae->mdic,&curdic,sizeof(curdic));
	if (SearchLabel(ae,curdic.name,curdic.crc)) {
		rasm_printf(ae,"[%s:%d] cannot create variable [%s] as there is already a label with the same name\n",GetExpFile(ae,0),GetExpLine(ae,0),name);
		MaxError(ae);
		MemFree(curdic.name);
		return;
	}
	InsertDicoToTree(ae,&curdic);
}
double ComputeExpression(struct s_assenv *ae,char *expr, int ptr, int didx, int expected_eval)
{
	#undef FUNC
	#define FUNC "ComputeExpression"

	char *ptr_exp,*ptr_exp2,backupeval;
	int crc,idic,idx=0,ialias,touched;
	double v,vl;
	struct s_alias curalias;
	struct s_expr_dico *curdic;
	char *minibuffer;

	while (!ae->AutomateExpressionDecision[((int)expr[idx])&0xFF]) idx++;

	switch (ae->AutomateExpressionDecision[((int)expr[idx])&0xFF]) {
		/*****************************************
		          M A K E     A L I A S
		*****************************************/
		case '~':
			memset(&curalias,0,sizeof(curalias));
			ptr_exp=expr+idx;
			*ptr_exp=0;
			ptr_exp2=expr+idx+1;
			
			/* alias locaux ou de proximit� */
			if (strchr("@.",expr[0])) {
				/* local label creation handle formula in tags */
				curalias.alias=MakeLocalLabel(ae,expr,NULL);
			} else if (strchr(expr,'{')) {
				/* alias name contains formula */
				curalias.alias=TranslateTag(ae,TxtStrDup(expr),&touched,0,E_TAGOPTION_NONE);
//printf("alias name contains formula [%s] -> [%s]\n",expr,curalias.alias);
			} else {
				curalias.alias=TxtStrDup(expr);
			}
			curalias.crc=GetCRC(curalias.alias);
			if ((ialias=SearchAlias(ae,curalias.crc,curalias.alias))>=0) {
				rasm_printf(ae,"[%s:%d] Duplicate alias [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
				MaxError(ae);
				MemFree(curalias.alias);
			} else if (SearchDico(ae,curalias.alias,curalias.crc)) {
				rasm_printf(ae,"[%s:%d] Alias cannot override existing variable [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
				MaxError(ae);
				MemFree(curalias.alias);
			} else {
				curalias.translation=MemMalloc(strlen(ptr_exp2)+1+2);
				sprintf(curalias.translation,"(%s)",ptr_exp2);
				ExpressionFastTranslate(ae,&curalias.translation,2);
				curalias.len=strlen(curalias.translation);
				ObjectArrayAddDynamicValueConcat((void**)&ae->alias,&ae->ialias,&ae->malias,&curalias,sizeof(curalias));
				CheckAndSortAliases(ae);
			}
			*ptr_exp='~';
			return 0;
		/*****************************************
		               S E T     V A R
		*****************************************/
		case '=':
			/* patch NOT */
			if (ae->AutomateExpressionDecision[((int)expr[idx+1])&0xFF]==0 || expr[idx+1]=='!') {
				if (expected_eval) {
					if (ae->maxam) {
						/* maxam mode AND expected a value -> force comparison */
					} else {
						rasm_printf(ae,"[%s:%d] meaningless use of an expression [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
						MaxError(ae);
						return 0;
					}
				} else {
					/* affectation */
					if (expr[0]<'A' || expr[0]>'Z') {
						rasm_printf(ae,"[%s:%d] variable name must begin by a letter [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
						MaxError(ae);
						return 0;
					} else {
						ptr_exp=expr+idx;
						v=ComputeExpressionCore(ae,ptr_exp+1,ptr,didx);
						*ptr_exp=0;
						crc=GetCRC(expr);
						if ((ialias=SearchAlias(ae,crc,expr))>=0) {
							rasm_printf(ae,"[%s:%d] Variable cannot override existing alias [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
							MaxError(ae);
							return 0;
						}
						curdic=SearchDico(ae,expr,crc);
						if (curdic) {
							/* on affecte */
							curdic->v=v;
						} else {
							/* on cree une nouvelle variable */
							ExpressionSetDicoVar(ae,expr,v);
						}
						*ptr_exp='=';
						return v;
					}
				}
			}
			break;
		/*****************************************
		     P U R E    E X P R E S S I O N
		*****************************************/
		default:break;
	}
	return ComputeExpressionCore(ae,expr,ptr,didx);
}
int RoundComputeExpression(struct s_assenv *ae,char *expr, int ptr, int didx, int expression_expected) {
	return floor(ComputeExpression(ae,expr,ptr,didx,expression_expected)+ae->rough);
}

/*
	ExpressionFastTranslate
	
	purpose: translate all known symbols in an expression (especially variables acting like counters)
*/
void ExpressionFastTranslate(struct s_assenv *ae, char **ptr_expr, int fullreplace)
{
	#undef FUNC
	#define FUNC "ExpressionFastTranslate"

	struct s_label *curlabel;
	struct s_expr_dico *curdic;
	static char *varbuffer=NULL;
	static int ivar=0,maxivar=1;
	char curval[256]={0};
	int c,lenw=0,idx=0,crc,startvar,newlen,ialias,found_replace,yves,dek,reidx,lenbuf,rlen,tagoffset;
	double v;
	char tmpuchar[16];
	char *expr,*locallabel;
	int curly=0,curlyflag=0;
	char *Automate;
	int recurse=-1,recursecount=0;
	
	if (!ae || !ptr_expr) {
		if (varbuffer) MemFree(varbuffer);
		varbuffer=NULL;
		maxivar=1;
		ivar=0;
		return;
	}
	/* be sure to have at least some bytes allocated */
	StateMachineResizeBuffer(&varbuffer,128,&maxivar);
	expr=*ptr_expr;

//printf("fast [%s]\n",expr);

	while (!ae->AutomateExpressionDecision[((int)expr[idx])&0xFF]) idx++;

	switch (ae->maxam) {
		default:
		case 0: /* full check */
			if (expr[idx]=='~' || (expr[idx]=='=' && expr[idx+1]!='=')) {reidx=idx+1;break;}
			reidx=0;
			break;
		case 1: /* partial check with maxam */
			if (expr[idx]=='~') {reidx=idx+1;break;}
			reidx=0;
			break;
	}

	idx=0;
	/* is there ascii char? */
	while ((c=expr[idx])!=0) {
		if (c=='\'' || c=='"') {
			/* echappement */
			if (expr[idx+1]=='\\') {
				if (expr[idx+2] && expr[idx+3]==c) {
					/* no charset conversion for escaped chars */
					c=expr[idx+2];
					switch (c) {
						case 'b':c='\b';break;
						case 'v':c='\v';break;
						case 'f':c='\f';break;
						case '0':c='\0';break;
						case 'r':c='\r';break;
						case 'n':c='\n';break;
						case 't':c='\t';break;
						default:break;
					}
					sprintf(tmpuchar,"#%03X",c);
					memcpy(expr+idx,tmpuchar,4);
					idx+=3;
				} else {
					rasm_printf(ae,"[%s:%d] expression [%s] - Only single escaped char may be quoted\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
					MaxError(ae);
					expr[0]=0;
					return;
				}
			} else if (expr[idx+1] && expr[idx+2]==c) {
					sprintf(tmpuchar,"#%02X",ae->charset[(int)expr[idx+1]]);
					memcpy(expr+idx,tmpuchar,3);
					idx+=2;
			} else {
				rasm_printf(ae,"[%s:%d] expression [%s] - Only single char may be quoted\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
				MaxError(ae);
				expr[0]=0;
				return;
			}
		}
		idx++;
	}
	
	idx=reidx;
	while ((c=expr[idx])!=0) {
		switch (c) {
			/* operator / parenthesis */
			case '!':
			case '=':
			case '>':
			case '<':
			case '(':
			case ')':
			case ']':
			case '[':
			case '*':
			case '/':
			case '+':
			case '~':
			case '-':
			case '^':
			case 'm':
			case '|':
			case '&':
				idx++;
				break;
			default:
				startvar=idx;
				if (ae->AutomateExpressionValidCharFirst[((int)c)&0xFF]) {
					varbuffer[ivar++]=c;
					if (c=='{') {
						/* this is only tag and not a formula */
						curly++;
					}
					StateMachineResizeBuffer(&varbuffer,ivar,&maxivar);
					idx++;
					c=expr[idx];

					Automate=ae->AutomateExpressionValidChar;
					while (Automate[((int)c)&0xFF]) {
						if (c=='{') {
							curly++;
							curlyflag=1;					
							Automate=ae->AutomateExpressionValidCharExtended;
						} else if (c=='}') {
							curly--;
							if (!curly) {
								Automate=ae->AutomateExpressionValidChar;
							}
						}
						varbuffer[ivar++]=c;
						StateMachineResizeBuffer(&varbuffer,ivar,&maxivar);
						idx++;
						c=expr[idx];
					}
				}
				varbuffer[ivar]=0;
				if (!ivar) {
					rasm_printf(ae,"[%s:%d] invalid expression [%s] c=[%c] idx=%d\n",GetExpFile(ae,0),GetExpLine(ae,0),expr,c,idx);
					MaxError(ae);
					return;
				} else if (curly) {
					rasm_printf(ae,"[%s:%d] wrong curly brackets in expression [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
					MaxError(ae);
					return;
				}
		}
		if (ivar && (varbuffer[0]<'0' || varbuffer[0]>'9')) {
			/* numbering var or label */
			if (curlyflag) {
				char *minivarbuffer;
				int touched;
				int newlen;

				minivarbuffer=TranslateTag(ae,TxtStrDup(varbuffer), &touched,0,E_TAGOPTION_NONE);
				StateMachineResizeBuffer(&varbuffer,strlen(minivarbuffer)+1,&maxivar);
				strcpy(varbuffer,minivarbuffer);
				newlen=strlen(varbuffer);
				lenw=strlen(expr);
				/* must update source */
				if (newlen>ivar) {
					/* realloc bigger */
					expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
				}
				if (newlen!=ivar ) {
					lenw=strlen(expr);
					MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
				}
				strncpy(expr+startvar,minivarbuffer,newlen); /* copy without zero terminator */
				idx=startvar+newlen;
				/***/
				MemFree(minivarbuffer);
				curlyflag=0;
				/******* ivar must be updated in case of label or alias following ***********/
				ivar=newlen;
			}
			
			/* recherche dans dictionnaire et remplacement */
			crc=GetCRC(varbuffer);
			found_replace=0;
			/* pour les affectations ou les tests conditionnels on ne remplace pas le dico (pour le Push oui par contre!) */
			if (fullreplace) {
				if (varbuffer[0]=='$' && !varbuffer[1]) {
					#ifdef OS_WIN
					snprintf(curval,sizeof(curval)-1,"%d",ae->codeadr);
					newlen=strlen(curval);
					#else
					newlen=snprintf(curval,sizeof(curval)-1,"%d",ae->codeadr);
					#endif
					lenw=strlen(expr);
					if (newlen>ivar) {
						/* realloc bigger */
						expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
					}
					if (newlen!=ivar ) {
						MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
						found_replace=1;
					}
					strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
					idx=startvar+newlen;
					ivar=0;
					found_replace=1;
				} else {
					curdic=SearchDico(ae,varbuffer,crc);
					if (curdic) {
						v=curdic->v;

						#ifdef OS_WIN
						snprintf(curval,sizeof(curval)-1,"%lf",v);
						newlen=TrimFloatingPointString(curval);
						#else
						newlen=snprintf(curval,sizeof(curval)-1,"%lf",v);
						newlen=TrimFloatingPointString(curval);
						#endif
						lenw=strlen(expr);
						if (newlen>ivar) {
							/* realloc bigger */
							expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
						}
						if (newlen!=ivar ) {
							MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
						}
						strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
						idx=startvar+newlen;
						ivar=0;
						found_replace=1;
					}
				}
			}
			/* on cherche aussi dans les labels existants */
			if (!found_replace) {
				curlabel=SearchLabel(ae,varbuffer,crc);
				if (curlabel) {
					if (!curlabel->lz || ae->stage>1) {
						yves=curlabel->ptr;

						#ifdef OS_WIN
						snprintf(curval,sizeof(curval)-1,"%d",yves);
						newlen=strlen(curval);
						#else
						newlen=snprintf(curval,sizeof(curval)-1,"%d",yves);
						#endif
						lenw=strlen(expr);
						if (newlen>ivar) {
							/* realloc bigger */
							expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
						}
						if (newlen!=ivar ) {
							MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
							found_replace=1;
						}
						strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
						found_replace=1;
						idx=startvar+newlen;
						ivar=0;
					}
				}		
			}
			/* non trouve on cherche dans les alias */
			if (!found_replace) {
				if ((ialias=SearchAlias(ae,crc,varbuffer))>=0) {
					newlen=ae->alias[ialias].len;
					lenw=strlen(expr);
					/* infinite replacement check */
					if (recurse<=startvar) {
						/* recurse maximum count is a mix of alias len and alias number */
						if (recursecount>ae->ialias+ae->alias[ialias].len) {
							if (strchr(expr,'~')!=NULL) *strchr(expr,'~')=0;
							rasm_printf(ae,"[%s:%d] alias definition of %s has infinite recursivity\n",GetExpFile(ae,0),GetExpLine(ae,0),expr);
							MaxError(ae);
							expr[0]=0; /* avoid some errors due to shitty definition */
							return;
						} else {
							recursecount++;
						}
					}
					if (newlen>ivar) {
						/* realloc bigger */
						expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
					}
					if (newlen!=ivar) {
						MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
					}
					strncpy(expr+startvar,ae->alias[ialias].translation,newlen); /* copy without zero terminator */
					found_replace=1;
					/* need to parse again alias because of delayed declarations */
					recurse=startvar;
					idx=startvar;
					ivar=0;
				} else {
				}
			}
			if (!found_replace) {
				//printf("fasttranslate test local label\n");
				/* non trouve c'est peut-etre un label local - mais pas de l'octal */
				if (varbuffer[0]=='@' && (varbuffer[1]<'0' || varbuffer[1]>'9')) {
					lenbuf=strlen(varbuffer);
					locallabel=MakeLocalLabel(ae,varbuffer,&dek);
					/*** le grand remplacement ***/
					/* local to macro or loop */
					rlen=strlen(expr+startvar+lenbuf)+1;
					expr=*ptr_expr=MemRealloc(expr,strlen(expr)+dek+1);
					MemMove(expr+startvar+lenbuf+dek,expr+startvar+lenbuf,rlen);
					strncpy(expr+startvar+lenbuf,locallabel,dek);
					idx+=dek;
					MemFree(locallabel);
					found_replace=1;
				} else if (varbuffer[0]=='.' && (varbuffer[1]<'0' || varbuffer[1]>'9')) {
					/* proximity label */
					lenbuf=strlen(varbuffer);
					locallabel=MakeLocalLabel(ae,varbuffer,&dek);
					/*** le grand remplacement ***/
					rlen=strlen(expr+startvar+lenbuf)+1;
					dek=strlen(locallabel);
//printf("exprin =[%s]   rlen=%d dek-lenbuf=%d\n",expr,rlen,dek-lenbuf);
					expr=*ptr_expr=MemRealloc(expr,strlen(expr)+dek-lenbuf+1);
					MemMove(expr+startvar+dek,expr+startvar+lenbuf,rlen);
					strncpy(expr+startvar,locallabel,dek);
					idx+=dek-lenbuf;
					MemFree(locallabel);
//printf("exprout=[%s]\n",expr);

//@@TODO ajouter une recherche d'alias?

				} else if (varbuffer[0]=='{') {
					if (strncmp(varbuffer,"{BANK}",6)==0 || strncmp(varbuffer,"{PAGE}",6)==0) tagoffset=6; else
					if (strncmp(varbuffer,"{PAGESET}",9)==0) tagoffset=9; else
					if (strncmp(varbuffer,"{SIZEOF}",8)==0) tagoffset=8; else
					{
						tagoffset=0;
						rasm_printf(ae,"[%s:%d] Unknown prefix tag\n",GetExpFile(ae,0),GetExpLine(ae,0));
						MaxError(ae);
					}
					
					if (varbuffer[tagoffset]=='@') {
						startvar+=tagoffset;
						lenbuf=strlen(varbuffer+tagoffset);
						locallabel=MakeLocalLabel(ae,varbuffer+tagoffset,&dek);
						/*** le grand remplacement ***/
						rlen=strlen(expr+startvar+lenbuf)+1;
						expr=*ptr_expr=MemRealloc(expr,strlen(expr)+dek+1);
						MemMove(expr+startvar+lenbuf+dek,expr+startvar+lenbuf,rlen);
						strncpy(expr+startvar+lenbuf,locallabel,dek);
						idx+=dek;
						MemFree(locallabel);
						found_replace=1;
					} else if (varbuffer[tagoffset]=='$') {
						int tagvalue=-1;
						if (strcmp(varbuffer,"{BANK}$")==0) {
							if (ae->forcecpr) {
								if (ae->activebank<32) {
									tagvalue=ae->activebank;
								} else {
									rasm_printf(ae,"[%s:%d] expression [%s] cannot use BANK $ in a temporary space!\n",GetExpFile(ae,0),GetExpLine(ae,0),TradExpression(expr));
									MaxError(ae);
									tagvalue=0;
								}
							} else if (ae->forcesnapshot) {
								if (ae->activebank<36) {
									if (ae->bankset[ae->activebank>>2]) {
										tagvalue=(ae->activebank&0xFC)+(ae->codeadr>>14);
									} else {
										tagvalue=ae->activebank;
									}
								} else {
									rasm_printf(ae,"[%s:%d] expression [%s] cannot use BANK $ in a temporary space!\n",GetExpFile(ae,0),GetExpLine(ae,0),TradExpression(expr));
									MaxError(ae);
									tagvalue=0;
								}
							}
						} else if (strcmp(varbuffer,"{PAGE}$")==0) {
							if (ae->activebank<36) {
									if (ae->bankset[ae->activebank>>2]) {
										tagvalue=ae->bankgate[(ae->activebank&0xFC)+(ae->codeadr>>14)];
									} else {
										tagvalue=ae->bankgate[ae->activebank];
									}
							} else {
								rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGE $ in a temporary space!\n",GetExpFile(ae,0),GetExpLine(ae,0),TradExpression(expr));
								MaxError(ae);
								tagvalue=ae->activebank;
							}
						} else if (strcmp(varbuffer,"{PAGESET}$")==0) {
							if (ae->activebank<36) {
								if (ae->activebank>3) tagvalue=((ae->activebank>>2)-1)*8+0xC2; else tagvalue=0xC0;
							} else {
								rasm_printf(ae,"[%s:%d] expression [%s] cannot use PAGESET $ in a temporary space!\n",GetExpFile(ae,0),GetExpLine(ae,0),TradExpression(expr));
								MaxError(ae);
								tagvalue=ae->activebank;
							}
						}
						/* replace */
						#ifdef OS_WIN
						snprintf(curval,sizeof(curval)-1,"%d",tagvalue);
						newlen=strlen(curval);
						#else
						newlen=snprintf(curval,sizeof(curval)-1,"%d",tagvalue);
						#endif
						lenw=strlen(expr);
						if (newlen>ivar) {
							/* realloc bigger */
							expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
						}
						if (newlen!=ivar ) {
							MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
							found_replace=1;
						}
						strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
						idx=startvar+newlen;
						ivar=0;
						found_replace=1;
					}
				}
			}
			
			
			
			
			
			
			if (!found_replace && strcmp(varbuffer,"REPEAT_COUNTER")==0) {
				if (ae->ir) {
					yves=ae->repeat[ae->ir-1].repeat_counter;
					#ifdef OS_WIN
					snprintf(curval,sizeof(curval)-1,"%d",yves);
					newlen=strlen(curval);
					#else
					newlen=snprintf(curval,sizeof(curval)-1,"%d",yves);
					#endif
					lenw=strlen(expr);
					if (newlen>ivar) {
						/* realloc bigger */
						expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
					}
					if (newlen!=ivar ) {
						MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
						found_replace=1;
					}
					strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
					found_replace=1;
					idx=startvar+newlen;
					ivar=0;
				} else {
					rasm_printf(ae,"[%s:%d] cannot use REPEAT_COUNTER outside repeat loop\n",GetExpFile(ae,0),GetExpLine(ae,0));
					MaxError(ae);
				}
			}
			if (!found_replace && strcmp(varbuffer,"WHILE_COUNTER")==0) {
				if (ae->iw) {
					yves=ae->whilewend[ae->iw-1].while_counter;
					#ifdef OS_WIN
					snprintf(curval,sizeof(curval)-1,"%d",yves);
					newlen=strlen(curval);
					#else
					newlen=snprintf(curval,sizeof(curval)-1,"%d",yves);
					#endif
					lenw=strlen(expr);
					if (newlen>ivar) {
						/* realloc bigger */
						expr=*ptr_expr=MemRealloc(expr,lenw+newlen-ivar+1);
					}
					if (newlen!=ivar ) {
						MemMove(expr+startvar+newlen,expr+startvar+ivar,lenw-startvar-ivar+1);
						found_replace=1;
					}
					strncpy(expr+startvar,curval,newlen); /* copy without zero terminator */
					found_replace=1;
					idx=startvar+newlen;
					ivar=0;
				} else {
					rasm_printf(ae,"[%s:%d] cannot use WHILE_COUNTER outside repeat loop\n",GetExpFile(ae,0),GetExpLine(ae,0));
					MaxError(ae);
				}
			}
			/* unknown symbol -> add to used symbol pool */
			if (!found_replace) {
				InsertUsedToTree(ae,varbuffer,crc);
			}
		}
		ivar=0;
	}
}

void PushExpression(struct s_assenv *ae,int iw,enum e_expression zetype)
{
	#undef FUNC
	#define FUNC "PushExpression"
	
	struct s_expression curexp={0};
	int startptr=0;

	if (!ae->nocode) {
		curexp.iw=iw;
		curexp.wptr=ae->outputadr;
		curexp.zetype=zetype;
		curexp.ibank=ae->activebank;
		curexp.iorgzone=ae->io-1;
		curexp.lz=ae->lz;
		/* on traduit de suite les variables du dictionnaire pour les boucles et increments
			SAUF si c'est une affectation */
		if (!ae->wl[iw].e) {
			switch (zetype) {
				case E_EXPRESSION_V16C:
 					/* check non register usage */
					switch (GetCRC(ae->wl[iw].w)) {
						case CRC_IX:
						case CRC_IY:
						case CRC_MIX:
						case CRC_MIY:
							rasm_printf(ae,"[%s:%d] invalid register usage\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->maxptr);
							MaxError(ae);
						default:break;
					}
				case E_EXPRESSION_J8:
				case E_EXPRESSION_V8:
				case E_EXPRESSION_V16:
				case E_EXPRESSION_IM:startptr=-1;
							break;
				case E_EXPRESSION_IV8:
				case E_EXPRESSION_IV81:
				case E_EXPRESSION_IV16:startptr=-2;
							break;
				case E_EXPRESSION_3V8:startptr=-3;
							break;
				default:break;
			}
			/* hack pourri pour g�rer le $ */
			ae->codeadr+=startptr;
			/* ok mais les labels locaux des macros? */
			if (ae->ir || ae->iw || ae->imacro) {
				curexp.reference=TxtStrDup(ae->wl[iw].w);
				ExpressionFastTranslate(ae,&curexp.reference,1);
			} else {
				ExpressionFastTranslate(ae,&ae->wl[iw].w,1);
			}
			ae->codeadr-=startptr;
		}
		/* calcul adresse de reference et post-incrementation pour sauter les data */
		switch (zetype) {
			case E_EXPRESSION_J8:curexp.ptr=ae->codeadr-1;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_0V8:curexp.ptr=ae->codeadr;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_V8:curexp.ptr=ae->codeadr-1;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_0V16:curexp.ptr=ae->codeadr;ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_0V32:curexp.ptr=ae->codeadr;ae->outputadr+=4;ae->codeadr+=4;break;
			case E_EXPRESSION_0VR:curexp.ptr=ae->codeadr;ae->outputadr+=5;ae->codeadr+=5;break;
			case E_EXPRESSION_V16C:
			case E_EXPRESSION_V16:curexp.ptr=ae->codeadr-1;ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_IV81:curexp.ptr=ae->codeadr-2;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IV8:curexp.ptr=ae->codeadr-2;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_3V8:curexp.ptr=ae->codeadr-3;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IV16:curexp.ptr=ae->codeadr-2;ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_RST:curexp.ptr=ae->codeadr;ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IM:curexp.ptr=ae->codeadr-1;ae->outputadr++;ae->codeadr++;break;
		}
		if (ae->outputadr<=ae->maxptr) {
			ObjectArrayAddDynamicValueConcat((void **)&ae->expression,&ae->ie,&ae->me,&curexp,sizeof(curexp));
		} else {
			/* to avoid double error message */
			if (!ae->stop) rasm_printf(ae,"[%s:%d] output exceed limit %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->maxptr);
			MaxError(ae);
			ae->stop=1;
			return;
		}
	} else {
		switch (zetype) {
			case E_EXPRESSION_J8:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_0V8:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_V8:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_0V16:ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_0V32:ae->outputadr+=4;ae->codeadr+=4;break;
			case E_EXPRESSION_0VR:ae->outputadr+=5;ae->codeadr+=5;break;
			case E_EXPRESSION_V16C:
			case E_EXPRESSION_V16:ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_IV81:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IV8:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_3V8:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IV16:ae->outputadr+=2;ae->codeadr+=2;break;
			case E_EXPRESSION_RST:ae->outputadr++;ae->codeadr++;break;
			case E_EXPRESSION_IM:ae->outputadr++;ae->codeadr++;break;
		}
		if (ae->outputadr<=ae->maxptr) {
		} else {
			rasm_printf(ae,"[%s:%d] NOCODE output exceed limit %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->maxptr);
			FreeAssenv(ae);exit(3);
		}
	}
}

/*
The CP/M 2.2 directory has only one type of entry:

UU F1 F2 F3 F4 F5 F6 F7 F8 T1 T2 T3 EX S1 S2 RC   .FILENAMETYP....
AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL AL   ................

UU = User number. 0-15 (on some systems, 0-31). The user number allows multiple
    files of the same name to coexist on the disc. 
     User number = 0E5h => File deleted
Fn - filename
Tn - filetype. The characters used for these are 7-bit ASCII.
       The top bit of T1 (often referred to as T1') is set if the file is 
     read-only.
       T2' is set if the file is a system file (this corresponds to "hidden" on 
     other systems). 
EX = Extent counter, low byte - takes values from 0-31
S2 = Extent counter, high byte.

      An extent is the portion of a file controlled by one directory entry.
    If a file takes up more blocks than can be listed in one directory entry,
    it is given multiple entries, distinguished by their EX and S2 bytes. The
    formula is: Entry number = ((32*S2)+EX) / (exm+1) where exm is the 
    extent mask value from the Disc Parameter Block.

S1 - reserved, set to 0.
RC - Number of records (1 record=128 bytes) used in this extent, low byte.
    The total number of records used in this extent is

    (EX & exm) * 128 + RC

    If RC is 80h, this extent is full and there may be another one on the disc.
    File lengths are only saved to the nearest 128 bytes.

AL - Allocation. Each AL is the number of a block on the disc. If an AL
    number is zero, that section of the file has no storage allocated to it
    (ie it does not exist). For example, a 3k file might have allocation 
    5,6,8,0,0.... - the first 1k is in block 5, the second in block 6, the 
    third in block 8.
     AL numbers can either be 8-bit (if there are fewer than 256 blocks on the
    disc) or 16-bit (stored low byte first). 
*/
int EDSK_getblockid(int *fb) {
	#undef FUNC
	#define FUNC "EDSK_getblockid"
	
	int i;
	for (i=0;i<180;i++) {
		if (fb[i]) {
			return i;
		}
	}
	return -1;
}
int EDSK_getdirid(struct s_edsk_wrapper *curwrap) {
	#undef FUNC
	#define FUNC "EDSK_getdirid"
	
	int ie;
	for (ie=0;ie<64;ie++) {
		if (curwrap->entry[ie].user==0xE5) {
			//printf("getdirid -> %d\n",ie);
			return ie;
		}
	}
	return -1;
}
char *MakeAMSDOS_name(struct s_assenv *ae, char *filename)
{
	#undef FUNC
	#define FUNC "MakeAMSDOS_name"

	static char amsdos_name[12];
	int i,ia;
	char *pp;

	/* warning */
	if (strlen(filename)>12) {
		if (!ae->nowarning) rasm_printf(ae,"Warning - filename [%s] too long for AMSDOS, will be truncated\n",filename);
	} else if ((pp=strchr(filename,'.'))!=NULL) {
		if (pp-filename>8) {
			if (!ae->nowarning) rasm_printf(ae,"Warning - filename [%s] too long for AMSDOS, will be truncated\n",filename);
		}
	}
	/* copy filename */
	for (i=0;filename[i]!=0 && filename[i]!='.' && i<8;i++) {
		amsdos_name[i]=toupper(filename[i]);
	}
	/* fill with spaces */
	for (ia=i;ia<8;ia++) {
		amsdos_name[ia]=0x20;
	}
	/* looking for extension */
	for (;filename[i]!=0 && filename[i]!='.';i++);
	/* then copy it if any */
	if (filename[i]=='.') {
		i++;
		for (ia=0;filename[i]!=0 && ia<3;ia++) {
			amsdos_name[8+ia]=toupper(filename[i++]);
		}
	}
	amsdos_name[11]=0;
	return amsdos_name;
}


void EDSK_load(struct s_assenv *ae,struct s_edsk_wrapper *curwrap, char *edskfilename, int face)
{
	#undef FUNC
	#define FUNC "EDSK_load"

	unsigned char header[256];
	unsigned char *data;
	int tracknumber,sidenumber,tracksize,disksize;
	int i,b,s,f,t,curtrack,sectornumber,sectorsize,sectorid,reallength;
	int currenttrackposition=0,currentsectorposition,tmpcurrentsectorposition;
	unsigned char checksectorid[9];
	int curblock=0,curoffset=0;
	
	if (FileReadBinary(edskfilename,(char*)&header,0x100)!=0x100) {
		rasm_printf(ae,"Cannot read EDSK header!");
		FreeAssenv(ae);exit(ABORT_ERROR);
	}
	if (strncmp((char *)header,"MV - CPC",8)==0) {
		rasm_printf(ae,"updating DSK to EDSK [%s] / creator: %-14.14s",edskfilename,header+34);
		
		tracknumber=header[34+14];
		sidenumber=header[34+14+1];
		tracksize=header[34+14+1+1]+header[34+14+1+1+1]*256;
		rasm_printf(ae,"tracks: %d  sides:%d   track size:%d",tracknumber,sidenumber,tracksize);
		if (tracknumber>40 || sidenumber>2) {
			rasm_printf(ae,"[%s] DSK format is not supported in update mode\n",edskfilename);
			FreeAssenv(ae);exit(ABORT_ERROR);
		}
		if (face>=sidenumber) {
			rasm_printf(ae,"[%s] DSK has no face %d - DSK updated\n",edskfilename,face);
			return;
		}

		data=MemMalloc(tracksize*tracknumber*sidenumber);
		memset(data,0,tracksize*tracknumber*sidenumber);
		if (FileReadBinary(edskfilename,(char *)data,tracksize*tracknumber*sidenumber)!=tracksize*tracknumber*sidenumber) {
			rasm_printf(ae,"Cannot read DSK tracks!");
			FreeAssenv(ae);exit(ABORT_ERROR);
		}
		//loginfo("track data read (%dkb)",tracksize*tracknumber*sidenumber/1024);
		f=face;
		for (t=0;t<tracknumber;t++) {
			curtrack=t*sidenumber+f;

			i=(t*sidenumber+f)*tracksize;
			if (strncmp((char *)data+i,"Track-Info\r\n",12)) {
				rasm_printf(ae,"Invalid track information block side %d track %d",f,t);
				FreeAssenv(ae);exit(ABORT_ERROR);
			}
			sectornumber=data[i+21];
			sectorsize=data[i+20];
			if (sectornumber!=9 || sectorsize!=2) {
				rasm_printf(ae,"Cannot read [%s] Invalid DATA format",edskfilename);
				FreeAssenv(ae);exit(ABORT_ERROR);
			}
			memset(checksectorid,0,sizeof(checksectorid));			
			/* we want DATA format */
			for (s=0;s<sectornumber;s++) {
				if (t!=data[i+24+8*s]) {
					rasm_printf(ae,"Invalid track number in sector %02X track %d",data[i+24+8*s+2],t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
				if (f!=data[i+24+8*s+1]) {
					rasm_printf(ae,"Invalid side number in sector %02X track %d",data[i+24+8*s+2],t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
				if (data[i+24+8*s+2]<0xC1 || data[i+24+8*s+2]>0xC9) {
					rasm_printf(ae,"Invalid sector ID in sector %02X track %d",data[i+24+8*s+2],t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				} else {
					checksectorid[data[i+24+8*s+2]-0xC1]=1;
				}				
				if (data[i+24+8*s+3]!=2) {
					rasm_printf(ae,"Invalid sector size in sector %02X track %d",data[i+24+8*s+2],t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
			}
			for (s=0;s<sectornumber;s++) {
				if (!checksectorid[s]) {
					rasm_printf(ae,"Missing sector %02X track %d",s+0xC1,t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
			}
			/* piste � piste on lit les blocs DANS L'ORDRE LOGIQUE!!! */
			for (b=0xC1;b<=0xC9;b++)
			for (s=0;s<sectornumber;s++) {
				if (data[i+24+8*s+2]==b) {
					memcpy(&curwrap->blocks[curblock][curoffset],&data[i+0x100+s*512],512);
					curoffset+=512;
					if (curoffset>=1024) {
						curoffset=0;
						curblock++;
					}
				}
			}
		}
	} else if (strncmp((char *)header,"EXTENDED",8)==0) {
		rasm_printf(ae,"updating EDSK [%s] / creator: %-14.14s",edskfilename,header+34);
		tracknumber=header[34+14];
		sidenumber=header[34+14+1];
		// not in EDSK tracksize=header[34+14+1+1]+header[34+14+1+1+1]*256;
		//loginfo("tracks: %d  sides:%d",tracknumber,sidenumber);
		if (tracknumber>40 || sidenumber>2) {
			rasm_printf(ae,"[%s] DSK format is not supported in update mode\n",edskfilename);
			FreeAssenv(ae);exit(ABORT_ERROR);
		}
		if (face>=sidenumber) {
			rasm_printf(ae,"[%s] DSK has no face %d - DSK updated\n",edskfilename,face);
			return;
		}

		for (i=disksize=0;i<tracknumber*sidenumber;i++) disksize+=header[0x34+i]*256;
		//loginfo("total track size: %dkb",disksize/1024);

		data=MemMalloc(disksize);
		memset(data,0,disksize);
		if (FileReadBinary(edskfilename,(char *)data,disksize)!=disksize) {
			rasm_printf(ae,"Cannot read DSK tracks!");
			FreeAssenv(ae);exit(ABORT_ERROR);
		}

		f=face;
		for (t=0;t<tracknumber;t++) {
			curtrack=t*sidenumber+f;
			i=currenttrackposition;
			currentsectorposition=i+0x100;

			if (!header[0x34+curtrack] && t<40) {
				printf("Unexpected unformated track Side %d Track %02d",f,t);
			} else {
				currenttrackposition+=header[0x34+curtrack]*256;

				if (strncmp((char *)data+i,"Track-Info\r\n",12)) {
					rasm_printf(ae,"Invalid track information block side %d track %d",f,t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
				sectornumber=data[i+21];
				sectorsize=data[i+20];
				if (sectornumber!=9 || sectorsize!=2) {
					rasm_printf(ae,"Unsupported track %d",t);
					FreeAssenv(ae);exit(ABORT_ERROR);
				}
				memset(checksectorid,0,sizeof(checksectorid));			
				/* we want DATA format */
				for (s=0;s<sectornumber;s++) {
					sectorid=data[i+24+8*s+2];
					if (sectorid>=0xC1 && sectorid<=0xC9) checksectorid[sectorid-0xC1]=1; else {
						rasm_printf(ae,"invalid sector id for DATA track %d",t);
						return;
					}
					sectorsize=data[i+24+8*s+3];
					if (sectorsize!=2) {
						rasm_printf(ae,"invalid sector size track %d",t);
						return;
					}
					reallength=data[i+24+8*s+6]+data[i+24+8*s+7]*256; /* real length stored */
					if (reallength!=512) {
						rasm_printf(ae,"invalid sector length for track %d",t);
						return;
					}
				}

				/* piste � piste on lit les blocs DANS L'ORDRE LOGIQUE!!! */
				for (b=0xC1;b<=0xC9;b++) {
					tmpcurrentsectorposition=currentsectorposition;
					for (s=0;s<sectornumber;s++) {
						if (b==data[i+24+8*s+2]) {
							memcpy(&curwrap->blocks[curblock][curoffset],&data[tmpcurrentsectorposition],512);
							curoffset+=512;
							if (curoffset>=1024) {
								curoffset=0;
								curblock++;
							}
						}
						reallength=data[i+24+8*s+6]+data[i+24+8*s+7]*256;
						tmpcurrentsectorposition+=reallength;
					}
				}
			}
		}
		
		
	} else {
		rasm_printf(ae,"file [%s] is not a valid (E)DSK floppy image",edskfilename);
		FreeAssenv(ae);exit(-923);
	}
	FileReadBinaryClose(edskfilename);
	
	/* Rasm management of (e)DSK files is AMSDOS compatible, just need to copy CATalog blocks but sort them... */
	memcpy(&curwrap->entry[0],curwrap->blocks[0],1024);
	memcpy(&curwrap->entry[32],curwrap->blocks[1],1024);
	/* tri des entr�es selon le user */
	qsort(curwrap->entry,64,sizeof(struct s_edsk_wrapper_entry),cmpAmsdosentry);
	curwrap->nbentry=64;
	for (i=0;i<64;i++) {
		if (curwrap->entry[i].user==0xE5) {
			curwrap->nbentry=i;
			break;
		}
	}
#if 0
	printf("%d entr%s found\n",curwrap->nbentry,curwrap->nbentry>1?"ies":"y");
	for (i=0;i<curwrap->nbentry;i++) {
		printf("[%02d] - ",i);
		if (curwrap->entry[i].user<16) {
			printf("U%02d [%-8.8s.%c%c%c] %c%c subcpt=#%02X rc=#%02X blocks=",curwrap->entry[i].user,curwrap->entry[i].filename,
			curwrap->entry[i].filename[8]&0x7F,curwrap->entry[i].filename[9]&0x7F,curwrap->entry[i].filename[10],
			curwrap->entry[i].filename[8]&0x80?'P':'-',curwrap->entry[i].filename[9]&0x80?'H':'-',
			curwrap->entry[i].subcpt,curwrap->entry[i].rc);
			for (b=0;b<16;b++) if (curwrap->entry[i].blocks[b]) printf("%s%02X",b>0?" ":"",curwrap->entry[i].blocks[b]); else printf("%s  ",b>0?" ":"");
			if (i&1) printf("\n"); else printf(" | ");
		} else {
			printf("free entry                  =    rc=    blocks=                                               ");
			if (i&1) printf("\n"); else printf(" | ");
		}
	}
	if (i&1) printf("\n");
#endif
}

struct s_edsk_wrapper *EDSK_select(struct s_assenv *ae,char *edskfilename, int facenumber)
{
	#undef FUNC
	#define FUNC "EDSK_select"
	
	struct s_edsk_wrapper newwrap={0},*curwrap=NULL;
	int i;
	
	/* check if there is a DSK in memory */
	for (i=0;i<ae->nbedskwrapper;i++) {
		if (!strcmp(ae->edsk_wrapper[i].edsk_filename,edskfilename)) {
			curwrap=&ae->edsk_wrapper[i];
			break;
		}
	}
	if (i==ae->nbedskwrapper) {
		/* not in memory, create an empty struct */
		ObjectArrayAddDynamicValueConcat((void**)&ae->edsk_wrapper,&ae->nbedskwrapper,&ae->maxedskwrapper,&newwrap,sizeof(struct s_edsk_wrapper));
		curwrap=&ae->edsk_wrapper[ae->nbedskwrapper-1];
		curwrap->edsk_filename=TxtStrDup(edskfilename);
		memset(curwrap->entry,0xE5,sizeof(struct s_edsk_wrapper_entry)*64);
		memset(curwrap->blocks[0],0xE5,1024);
		memset(curwrap->blocks[1],0xE5,1024);
		curwrap->face=facenumber;
		/* and load files if the DSK exists on disk */
		if (FileExists(edskfilename)) {
			EDSK_load(ae,curwrap,edskfilename,facenumber);
		}
	}
	return curwrap;
}

int EDSK_addfile(struct s_assenv *ae,char *edskfilename,int facenumber, char *filename,unsigned char *indata,int insize, int offset)
{
	#undef FUNC
	#define FUNC "EDSK_addfile"

	struct s_edsk_wrapper *curwrap=NULL;
	char amsdos_name[12]={0};
	int idsk,j,i,ia,ib,ie,filesize,fsector,idxdata;
	int fb[180],rc,idxb;
	unsigned char *data=NULL;
	int size=0;
	int firstblock,lastblock;

	curwrap=EDSK_select(ae,edskfilename,facenumber);
	
	/* update struct */
	size=insize+128;
	data=MemMalloc(size);
	memcpy(amsdos_name,MakeAMSDOS_name(ae,filename),11);
	memcpy(data,MakeAMSDOSHeader(offset,offset+insize,amsdos_name),128);
	memcpy(data+128,indata,insize);
	/* overwrite check */
	for (i=0;i<curwrap->nbentry;i++) {
		if (!strncmp((char *)curwrap->entry[i].filename,amsdos_name,11)) {
			if (!ae->edskoverwrite) {
				rasm_printf(ae,"Error - Cannot save [%s] in edsk [%s] with overwrite disabled as the file already exists\n",amsdos_name,edskfilename);
				MaxError(ae);
				MemFree(data);
				return 0;
			} else {
				/* overwriting previous file */
				memset(&curwrap->entry[i],0xE5,sizeof(struct s_edsk_wrapper_entry));
			}
		}
	}
	/* find free blocks */
	fb[0]=fb[1]=0;
	for (i=2;i<180;i++) fb[i]=1;
	for (i=0;i<curwrap->nbentry;i++) {
		if (curwrap->entry[i].rc!=0xE5 && curwrap->entry[i].rc!=0) {
			/* entry found, compute number of blocks to read */
			rc=curwrap->entry[i].rc/8;
			if (curwrap->entry[i].rc%8) rc++; /* adjust value */
			/* mark as used */
			for (j=0;j<rc;j++) {
				fb[curwrap->entry[i].blocks[j]]=0;
			}
		}
	}
	/* set directory, blocks and data in blocks */
	firstblock=-1;
	lastblock=-1;
	filesize=size;
	idxdata=0;
	ia=0;
	while (filesize>0) {
		if (filesize>16384) {
			/* extended entry */
			if ((ie=EDSK_getdirid(curwrap))==-1)  {
				rasm_printf(ae,"Error - edsk [%s] DIRECTORY FULL\n",edskfilename);
				MemFree(data);
				return 0;
			}
			if (curwrap->nbentry<=ie) curwrap->nbentry=ie+1;
			idxb=0;
			for (i=0;i<16;i++) {
				if ((ib=EDSK_getblockid(fb))==-1) {
					rasm_printf(ae,"Error - edsk [%s] DISK FULL\n",edskfilename);
					MemFree(data);
					return 0;
				} else {
					if (firstblock==-1) firstblock=ib;
					lastblock=ib;

					memcpy(curwrap->blocks[ib],data+idxdata,1024);
					idxdata+=1024;
					filesize-=1024;
					fb[ib]=0;
					curwrap->entry[ie].blocks[idxb++]=ib;
				}
			}
			memcpy(curwrap->entry[ie].filename,amsdos_name,11);
			curwrap->entry[ie].subcpt=ia;
			curwrap->entry[ie].rc=0x80;
			curwrap->entry[ie].user=0;
			ia++;
			idxb=0;
		} else {
			/* last entry */
			if ((ie=EDSK_getdirid(curwrap))==-1)  {
				rasm_printf(ae,"Error - edsk [%s] DIRECTORY FULL\n",edskfilename);
				MemFree(data);
				return 0;
			}
			if (curwrap->nbentry<=ie) curwrap->nbentry=ie+1;
			/* calcul du nombre de sous blocs de 128 octets */
			curwrap->entry[ie].rc=filesize/128;
			if (filesize%128) {
				curwrap->entry[ie].rc+=1;
			}
			idxb=0;
			for (i=0;i<16 && filesize>0;i++) {
				if ((ib=EDSK_getblockid(fb))==-1) {
					rasm_printf(ae,"Error - edsk [%s] DISK FULL\n",edskfilename);
					MemFree(data);
					return 0;
				} else {
					if (firstblock==-1) firstblock=ib;
					lastblock=ib;

					memcpy(curwrap->blocks[ib],&data[idxdata],filesize>1024?1024:filesize);
					idxdata+=1024;
					filesize-=1024;
					fb[ib]=0;
					curwrap->entry[ie].blocks[idxb++]=ib;
				}
			}
			filesize=0;
			memcpy(curwrap->entry[ie].filename,amsdos_name,11);
			curwrap->entry[ie].subcpt=ia;
			curwrap->entry[ie].user=0;
		}
	}

#if 0
	/* AMSDOS stuff to get a regular AMSDOS header */
	curwrap->blocks[firstblock][16]=firstblock;
	curwrap->blocks[firstblock][17]=lastblock;
	curwrap->blocks[firstblock][23]=curwrap->blocks[firstblock][16];
#endif
	MemFree(data);
	return 1;
}

void EDSK_build_amsdos_directory(struct s_edsk_wrapper *face)
{
	#undef FUNC
	#define FUNC "EDSK_build_amsdos_directory"
	
	unsigned char amsdosdir[2048]={0};
	int i,idx=0,b;

	if (!face) return;
	
	
//printf("build amsdos dir with %d entries\n",face->nbentry);	
	for (i=0;i<face->nbentry;i++) {
		if (face->entry[i].rc && face->entry[i].rc!=0xE5) {
			amsdosdir[idx]=face->entry[i].user;
			memcpy(amsdosdir+idx+1,face->entry[i].filename,11);
			amsdosdir[idx+12]=face->entry[i].subcpt;
			amsdosdir[idx+13]=0;
			amsdosdir[idx+14]=0;
			amsdosdir[idx+15]=face->entry[i].rc;
			//printf("%-11.11s [%02X.%02X] blocks:",amsdosdir+idx+1,amsdosdir[idx+12],amsdosdir[idx+15]);
			for (b=0;b<16;b++) {
				if (face->entry[i].blocks[b]!=0xE5) {
					amsdosdir[idx+16+b]=face->entry[i].blocks[b];
					//printf("%02X.",amsdosdir[idx+16+b]);
					//printf("%02X.",face->entry[i].blocks[b]);
				} else {
					amsdosdir[idx+16+b]=0;
				}
			}
			//printf("\n");
		}
		idx+=32;
	}
	for (i=face->nbentry;i<64;i++) {
		//if (i==face->nbentry) printf("filling amsdos remaining entries (%d) with #E5\n",64-face->nbentry);
		memset(amsdosdir+idx,0xE5,32);
		idx+=32;
	}
	memcpy(face->blocks[0],amsdosdir,1024);
	memcpy(face->blocks[1],amsdosdir+1024,1024);
}
void EDSK_write_file(struct s_assenv *ae,struct s_edsk_wrapper *faceA,struct s_edsk_wrapper *faceB)
{
	#undef FUNC
	#define FUNC "EDSK_write_file"

	struct s_edsk_wrapper emptyface={0};
	unsigned char header[256]={0};
	unsigned char trackblock[256]={0};
	int idblock,blockoffset;
	int i,t;
	
	if (!faceA && !faceB) return;
	
	/* cr�ation des deux blocs du directory par face */
	EDSK_build_amsdos_directory(faceA);
	EDSK_build_amsdos_directory(faceB);
	/* �criture header */
	strcpy((char *)header,"EXTENDED CPC DSK File\r\nDisk-Info\r\n");
	strcpy((char *)header+0x22,RASM_VERSION);
	header[0x30]=40;
	if (!faceA) {
		faceA=&emptyface;
		faceA->edsk_filename=TxtStrDup(faceB->edsk_filename);
	}
	if (faceB!=NULL) header[0x31]=2; else header[0x31]=1;
	for (i=0;i<header[0x30]*header[0x31];i++) header[0x34+i]=19; /* tracksize=(9*512+256)/256 */
	FileWriteBinary(faceA->edsk_filename,(char *)header,256);
	
	/* �criture des pistes */
	for (t=0;t<40;t++) {
		strcpy((char *)trackblock,"Track-Info\r\n");
		trackblock[0x10]=t;
		trackblock[0x11]=0;
		trackblock[0x14]=2;
		trackblock[0x15]=9;
		trackblock[0x16]=0x4E;
		trackblock[0x17]=0xE5;
		for (i=0;i<9;i++) {
			trackblock[0x18+i*8+0]=trackblock[0x10];
			trackblock[0x18+i*8+1]=trackblock[0x11];
			trackblock[0x18+i*8+2]=i+0xC1;
			trackblock[0x18+i*8+3]=2;
			trackblock[0x18+i*8+4]=0;
			trackblock[0x18+i*8+5]=0;
			trackblock[0x18+i*8+6]=0;
			trackblock[0x18+i*8+7]=2;
		}
		/* �criture du track info */
		FileWriteBinary(faceA->edsk_filename,(char *)trackblock,256);
		/* �criture des secteurs */
		idblock=t*9/2;
		blockoffset=((t*9)%2)*512;
		for (i=0;i<9;i++) {
			FileWriteBinary(faceA->edsk_filename,(char *)&faceA->blocks[idblock][0]+blockoffset,512);
			blockoffset+=512;
			if (blockoffset>=1024) {
				blockoffset=0;
				idblock++;
			}
		}
	
		if (faceB) {
			trackblock[0x11]=1;
			for (i=0;i<9;i++) {
				trackblock[0x18+i*8+0]=trackblock[0x10];
				trackblock[0x18+i*8+1]=trackblock[0x11];
			}
			/* �criture du track info */
			FileWriteBinary(faceB->edsk_filename,(char *)trackblock,256);
			/* �criture des secteurs */
			idblock=t*9/2;
			blockoffset=((t*9)%2)*512;
			for (i=0;i<9;i++) {
				FileWriteBinary(faceB->edsk_filename,(char *)&faceB->blocks[idblock][0]+blockoffset,512);
				blockoffset+=512;
				if (blockoffset>=1024) {
					blockoffset=0;
					idblock++;
				}
			}
		}
	}
	FileWriteBinaryClose(faceA->edsk_filename);
	rasm_printf(ae,"Write edsk file %s\n",faceA->edsk_filename);
}
void EDSK_write(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "EDSK_write"

	struct s_edsk_wrapper *faceA,*faceB;
	char *edskfilename;
	int i,j;
	
	/* on passe en revue toutes les structs */
	for (i=0;i<ae->nbedskwrapper;i++) {
		/* already done */
		if (ae->edsk_wrapper[i].face==-1) continue;
		
		switch (ae->edsk_wrapper[i].face) {
			default:
			case 0:faceA=&ae->edsk_wrapper[i];faceB=NULL;break;
			case 1:faceA=NULL;faceB=&ae->edsk_wrapper[i];break;
		}
		/* doit-on fusionner avec une autre face? */
		for (j=i+1;j<ae->nbedskwrapper;j++) {
			if (!strcmp(ae->edsk_wrapper[i].edsk_filename,ae->edsk_wrapper[j].edsk_filename)) {
				/* found another face for the floppy */
				switch (ae->edsk_wrapper[j].face) {
					default:
					case 0:faceA=&ae->edsk_wrapper[j];break;
					case 1:faceB=&ae->edsk_wrapper[j];break;
				}
			}
		}
		EDSK_write_file(ae,faceA,faceB);
	}
}
void PopAllSave(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "PopAllSave"
	
	unsigned char *AmsdosHeader;
	char *dskfilename;
	char *filename;
	int offset,size;
	int i,is,erreur=0;
	
	for (is=0;is<ae->nbsave;is++) {
		/* avoid quotes */
		filename=ae->wl[ae->save[is].iw].w;
		filename[strlen(filename)-1]=0;
		filename++;
		/**/
		ae->idx=ae->save[is].ioffset; /* exp hack */
		ExpressionFastTranslate(ae,&ae->wl[ae->idx].w,0);
		offset=RoundComputeExpression(ae,ae->wl[ae->save[is].ioffset].w,0,0,0);
		ExpressionFastTranslate(ae,&ae->wl[ae->save[is].isize].w,0);
		size=RoundComputeExpression(ae,ae->wl[ae->save[is].isize].w,0,0,0);
		
		/* DSK management */
		if (ae->save[is].dsk) {
			if (ae->save[is].iwdskname!=-1) {
				/* oblig� de dupliquer � cause du reuse */
				dskfilename=TxtStrDup(ae->wl[ae->save[is].iwdskname].w);
				dskfilename[strlen(dskfilename)-1]=0;
				
				if (!EDSK_addfile(ae,dskfilename+1,ae->save[is].face,filename,ae->mem[ae->save[is].ibank]+offset,size,offset)) {
					erreur++;
					break;
				}
				MemFree(dskfilename);
			}
		} else {
			/* output file on filesystem */
			rasm_printf(ae,"Write binary file %s (%d byte%s)\n",filename,size,size>1?"s":"");
			FileRemoveIfExists(filename);
			if (ae->save[is].amsdos) {
				AmsdosHeader=MakeAMSDOSHeader(offset,offset+size,MakeAMSDOS_name(ae,filename));
				FileWriteBinary(filename,(char *)AmsdosHeader,128);
			}		
			FileWriteBinary(filename,(char*)ae->mem[ae->save[is].ibank]+offset,size);
			FileWriteBinaryClose(filename);
		}
	}
	if (!erreur) EDSK_write(ae);
	
	for (i=0;i<ae->nbedskwrapper;i++) {
		MemFree(ae->edsk_wrapper[i].edsk_filename);
	}
	if (ae->maxedskwrapper) MemFree(ae->edsk_wrapper);

	if (ae->nbsave) {
		MemFree(ae->save);
	}
}

void PopAllExpression(struct s_assenv *ae, int crunched_zone)
{
	#undef FUNC
	#define FUNC "PopAllExpression"
	
	static int first=1;
	double v;
	long r;
	int i;
	unsigned char *mem;
	char *expr;
	
	/* pop all expressions BUT thoses who where already computed (in crunched blocks) */

	/* calcul des labels et expressions en zone crunch (et locale?)
	   les labels doivent pointer:
	   - une valeur absolue (numerique ou variable calculee) -> completement transparent
	   - un label dans la meme zone de crunch -> label->lz=1 && verif de la zone crunch
	   - un label hors zone crunch MAIS avant toute zone de crunch de la bank destination (!label->lz)

	   idealement on doit tolerer les adresses situees apres le crunch dans une autre ORG zone!

	   on utilise ae->stage pour cr�er un �tat interm�diaire dans le ComputeExpressionCore
	*/
	if (crunched_zone>=0) {
		ae->stage=1;
	} else {
		/* on rescanne tout pour combler les trous */
		ae->stage=2;
		first=1;
	}
	
	for (i=first;i<ae->ie;i++) {
		/* first compute only crunched expression (0,1,2,3,...) then (-1) at the end */
		if (crunched_zone>=0) {
			/* calcul des expressions en zone crunch */
			if (ae->expression[i].lz<crunched_zone) continue;
			if (ae->expression[i].lz>crunched_zone) {
				first=i;
				break;
			}
		} else {
			if (ae->expression[i].lz>=0) continue;
		}

		mem=ae->mem[ae->expression[i].ibank];
		
		if (ae->expression[i].reference) {
			expr=ae->expression[i].reference;
		} else {
			expr=ae->wl[ae->expression[i].iw].w;
		}
		v=ComputeExpressionCore(ae,expr,ae->expression[i].ptr,i);
		r=(long)floor(v+ae->rough);
		switch (ae->expression[i].zetype) {
			case E_EXPRESSION_J8:
				r=r-ae->expression[i].ptr-2;
				if (r<-128 || r>127) {
					rasm_printf(ae,"[%s:%d] relative offset %d too far [%s]\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l,r,ae->wl[ae->expression[i].iw].w);
					MaxError(ae);
				}
				mem[ae->expression[i].wptr]=(unsigned char)r;
				break;
			case E_EXPRESSION_IV81:
				/* for enhanced 16bits instructions */
				r++;
			case E_EXPRESSION_0V8:
			case E_EXPRESSION_IV8:
			case E_EXPRESSION_3V8:
			case E_EXPRESSION_V8:
				if (r>255 || r<-128) {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: truncating value #%X to #%X\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l,r,r&0xFF);
				}
				mem[ae->expression[i].wptr]=(unsigned char)r;
				break;
			case E_EXPRESSION_IV16:
			case E_EXPRESSION_V16:
			case E_EXPRESSION_V16C:
			case E_EXPRESSION_0V16:
				if (r>65535 || r<-32768) {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: truncating value #%X to #%X\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l,r,r&0xFFFF);
				}
				mem[ae->expression[i].wptr]=(unsigned char)r&0xFF;
				mem[ae->expression[i].wptr+1]=(unsigned char)((r>>8)&0xFF);
				break;
			case E_EXPRESSION_0V32:
				/* meaningless in 32 bits architecture... */
				if (v>4294967295 || v<-2147483648) {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: truncating value\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l);
				}
				mem[ae->expression[i].wptr]=(unsigned char)r&0xFF;
				mem[ae->expression[i].wptr+1]=(unsigned char)((r>>8)&0xFF);
				mem[ae->expression[i].wptr+2]=(unsigned char)((r>>16)&0xFF);
				mem[ae->expression[i].wptr+3]=(unsigned char)((r>>24)&0xFF);
				break;
			case E_EXPRESSION_0VR:
				/* convert v double value to Amstrad REAL */
				{
					double tmpval;
					unsigned char rc[5]={0};
					int j,ib,ibb,exp=0;

					unsigned int deci;
					int fracmax=0;
					double frac;
					int mesbits[32];
					int ibit=0;
					unsigned int mask;

					deci=fabs(floor(v));
					frac=fabs(v)-deci;

					if (deci) {
						mask=0x80000000;
						while (!(deci & mask)) mask=mask/2;
						while (mask) {
							mesbits[ibit]=!!(deci & mask);
	//printf("%d",mesbits[ibit]);
							ibit++;
							mask=mask/2;
						}
	//printf("\nexposant positif: %d\n",ibit);
						exp=ibit;
	//printf(".");
						while (ibit<32 && frac!=0) {
							frac=frac*2;
							if (frac>=1.0) {
								mesbits[ibit++]=1;
	//printf("1");
								frac-=1.0;
							} else {
								mesbits[ibit++]=0;
	//printf("0");
							}
							fracmax++;
						}
					} else {
	//printf("\nexposant negatif a definir:\n");
	//printf("x.");
						
						/* handling zero */
						if (frac==0.0) {
							exp=0;
							ibit=0;
						} else {
							/* looking for first significant bit */
							while (1) {
								frac=frac*2;
								if (frac>=1.0) {
									mesbits[ibit++]=1;
	//printf("1");
									frac-=1.0;
									break; /* first significant bit found, now looking for limit */
								} else {
	//printf("o");
								}
								fracmax++;
								exp--;
							}
							while (ibit<32 && frac!=0) {
								frac=frac*2;
								if (frac>=1.0) {
									mesbits[ibit++]=1;
	//printf("1");
									frac-=1.0;
								} else {
									mesbits[ibit++]=0;
	//printf("0");
								}
								fracmax++;
							}
						}
					}

	//printf("\n%d bits utilises en mantisse\n",ibit);
					/* pack bits */
					ib=3;ibb=0x80;
					for (j=0;j<ibit;j++) {
						if (mesbits[j])	rc[ib]|=ibb;
						ibb/=2;
						if (ibb==0) {
							ibb=0x80;
							ib--;
						}
					}
					/* exponent */
					exp+=128;
					if (exp<0 || exp>255) {
						rasm_printf(ae,"[%s:%d] Exponent overflow\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l);
						MaxError(ae);
						exp=128;
					}
					rc[4]=exp;

					/* REAL sign */
					if (v>=0) {
						rc[3]&=0x7F;
					} else {
						rc[3]|=0x80;
					}

					for (j=0;j<5;j++)	mem[ae->expression[i].wptr+j]=rc[j];
				}
				break;
			case E_EXPRESSION_IM:
				switch (r) {
					case 0x00:mem[ae->expression[i].wptr]=0x46;break;
					case 0x01:mem[ae->expression[i].wptr]=0x56;break;
					case 0x02:mem[ae->expression[i].wptr]=0x5E;break;
					default:
						rasm_printf(ae,"[%s:%d] IM 0,1 or 2 only\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l);
						MaxError(ae);
						mem[ae->expression[i].wptr]=0;
				}
				break;
			case E_EXPRESSION_RST:
				switch (r) {
					case 0x00:mem[ae->expression[i].wptr]=0xC7;break;
					case 0x08:mem[ae->expression[i].wptr]=0xCF;break;
					case 0x10:mem[ae->expression[i].wptr]=0xD7;break;
					case 0x18:mem[ae->expression[i].wptr]=0xDF;break;
					case 0x20:mem[ae->expression[i].wptr]=0xE7;break;
					case 0x28:mem[ae->expression[i].wptr]=0xEF;break;
					case 0x30:mem[ae->expression[i].wptr]=0xF7;break;
					case 0x38:mem[ae->expression[i].wptr]=0xFF;break;
					default:
						rasm_printf(ae,"[%s:%d] RST #0,#8,#10,#18,#20,#28,#30,#38 only\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l);
						MaxError(ae);
						mem[ae->expression[i].wptr]=0;
				}
				break;
			default:
				rasm_printf(ae,"[%s:%d] FATAL - unknown expression type\n",GetExpFile(ae,i),ae->wl[ae->expression[i].iw].l);
				FreeAssenv(ae);exit(-8);
		}	
	}
}

void InsertLabelToTree(struct s_assenv *ae, struct s_label *label)
{
	#undef FUNC
	#define FUNC "InsertLabelToTree"

	struct s_crclabel_tree *curlabeltree;
	int radix,dek=32;

	curlabeltree=&ae->labeltree;
	while (dek) {
		dek=dek-8;
		radix=(label->crc>>dek)&0xFF;
		if (curlabeltree->radix[radix]) {
			curlabeltree=curlabeltree->radix[radix];
		} else {
			curlabeltree->radix[radix]=MemMalloc(sizeof(struct s_crclabel_tree));
			curlabeltree=curlabeltree->radix[radix];
			memset(curlabeltree,0,sizeof(struct s_crclabel_tree));
		}
	}
	ObjectArrayAddDynamicValueConcat((void**)&curlabeltree->label,&curlabeltree->nlabel,&curlabeltree->mlabel,&label[0],sizeof(struct s_label));
}

/* use by structure mechanism and label import to add fake labels */
void PushLabelLight(struct s_assenv *ae, struct s_label *curlabel) {
	#undef FUNC
	#define FUNC "PushLabelLight"
	
	struct s_label *searched_label;
	
	/* PushLabel light */
	if ((searched_label=SearchLabel(ae,curlabel->name,curlabel->crc))!=NULL) {
		rasm_printf(ae,"[%s:%d] %s caused duplicate label [%s]\n",GetExpFile(ae,0),GetExpLine(ae,0),ae->idx?"Structure insertion":"Label import",curlabel->name);
		MemFree(curlabel->name);
		MaxError(ae);
	} else {
		ObjectArrayAddDynamicValueConcat((void **)&ae->label,&ae->il,&ae->ml,curlabel,sizeof(struct s_label));
		InsertLabelToTree(ae,curlabel);
	}				
}
void PushLabel(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "PushLabel"
	
	struct s_label curlabel={0},*searched_label;
	char *curlabelname;
	int i;
	/* label with counters */
	struct s_expr_dico *curdic;
	char curval[32];
	char *varbuffer,*expr;
	char *starttag,*endtag,*tagcheck;
	int taglen,tagidx,lenw,tagcount=0;
	int crc,newlen,touched;

	if (ae->AutomateValidLabelFirst[ae->wl[ae->idx].w[0]]) {
		for (i=1;ae->wl[ae->idx].w[i];i++) {
			if (ae->wl[ae->idx].w[i]=='{') tagcount++; else if (ae->wl[ae->idx].w[i]=='}') tagcount--;
			if (!tagcount) {
				if (!ae->AutomateValidLabel[ae->wl[ae->idx].w[i]]) {
					rasm_printf(ae,"[%s:%d] Invalid char in label declaration (%c)\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w[i]);
					MaxError(ae);
					return;
				}
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] Invalid first char in label declaration (%c)\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w[0]);
		MaxError(ae);
		return;
	}
	
	switch (i) {
		case 1:
			switch (ae->wl[ae->idx].w[0]) {
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				case 'H':
				case 'L':
				case 'I':
				case 'R':
					rasm_printf(ae,"[%s:%d] Cannot use reserved word [%s] for label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w);
					MaxError(ae);
					return;
				default:break;
			}
			break;
		case 2:
			if (strcmp(ae->wl[ae->idx].w,"AF")==0 || strcmp(ae->wl[ae->idx].w,"BC")==0 || strcmp(ae->wl[ae->idx].w,"DE")==0 || strcmp(ae->wl[ae->idx].w,"HL")==0 || 
				strcmp(ae->wl[ae->idx].w,"IX")==0 || strcmp(ae->wl[ae->idx].w,"IY")==0 || strcmp(ae->wl[ae->idx].w,"SP")==0 ||
				strcmp(ae->wl[ae->idx].w,"LX")==0 || strcmp(ae->wl[ae->idx].w,"HX")==0 || strcmp(ae->wl[ae->idx].w,"XL")==0 || strcmp(ae->wl[ae->idx].w,"XH")==0 ||
				strcmp(ae->wl[ae->idx].w,"LY")==0 || strcmp(ae->wl[ae->idx].w,"HY")==0 || strcmp(ae->wl[ae->idx].w,"YL")==0 || strcmp(ae->wl[ae->idx].w,"YH")==0) {
				rasm_printf(ae,"[%s:%d] Cannot use reserved word [%s] for label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w);
				MaxError(ae);
				return;
			}
			break;
		case 3:
			if (strcmp(ae->wl[ae->idx].w,"IXL")==0 || strcmp(ae->wl[ae->idx].w,"IYL")==0 || strcmp(ae->wl[ae->idx].w,"IXH")==0 || strcmp(ae->wl[ae->idx].w,"IYH")==0) {
				rasm_printf(ae,"[%s:%d] Cannot use reserved word [%s] for label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w);
				MaxError(ae);
				return;
			}			
			break;
		case 4:
			if (strcmp(ae->wl[ae->idx].w,"VOID")==0) {
				rasm_printf(ae,"[%s:%d] Cannot use reserved word [%s] for label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w);
				MaxError(ae);
				return;
			}
		default:break;
	}

	/*******************************************************
	   v a r i a b l e s     i n    l a b e l    n a m e
	*******************************************************/
	varbuffer=TranslateTag(ae,TxtStrDup(ae->wl[ae->idx].w),&touched,1,E_TAGOPTION_NONE);
	
	/**************************************************
	   s t r u c t u r e     d e c l a r a t i o n
	**************************************************/
	if (ae->getstruct) {
		struct s_rasmstructfield rasmstructfield={0};
		if (varbuffer[0]=='@') {
			rasm_printf(ae,"[%s:%d] Please no local label in a struct [%s]\n",GetExpFile(ae,0),ae->wl[ae->idx].l,ae->wl[ae->idx].w);
			MaxError(ae);
			return;
		}
		/* copy label+offset in the structure */
		rasmstructfield.name=TxtStrDup(varbuffer);
		rasmstructfield.offset=ae->codeadr;
		ObjectArrayAddDynamicValueConcat((void **)&ae->rasmstruct[ae->irasmstruct-1].rasmstructfield,
				&ae->rasmstruct[ae->irasmstruct-1].irasmstructfield,&ae->rasmstruct[ae->irasmstruct-1].mrasmstructfield,
				&rasmstructfield,sizeof(rasmstructfield));
		/* label is structname+field */
		curlabelname=curlabel.name=MemMalloc(strlen(ae->rasmstruct[ae->irasmstruct-1].name)+strlen(varbuffer)+2);
		sprintf(curlabel.name,"%s.%s",ae->rasmstruct[ae->irasmstruct-1].name,varbuffer);
		curlabel.iw=-1;
		/* legacy */
		curlabel.crc=GetCRC(curlabel.name);
		curlabel.ptr=ae->codeadr;
	} else {
		/**************************************************
		   l a b e l s
		**************************************************/
		/* labels locaux */
		if (varbuffer[0]=='@' && (ae->ir || ae->iw || ae->imacro)) {
			curlabel.iw=-1;
			curlabelname=curlabel.name=MakeLocalLabel(ae,varbuffer,NULL);
			curlabel.crc=GetCRC(curlabel.name);
		} else {
			switch (varbuffer[0]) {
				case '.':
					if (ae->dams) {
						/* old Dams style declaration (remove the dot) */
						i=0;
						do {
							varbuffer[i]=varbuffer[i+1];
							ae->wl[ae->idx].w[i]=ae->wl[ae->idx].w[i+1];
							i++;
						} while (varbuffer[i]!=0);
						if (!touched) {
							curlabel.iw=ae->idx;
						} else {
							curlabel.iw=-1;
							curlabel.name=varbuffer;
						}
						curlabel.crc=GetCRC(varbuffer);
						curlabelname=varbuffer;
					} else {
						/* proximity labels */
						if (ae->lastgloballabel) {
							curlabelname=MemMalloc(strlen(varbuffer)+1+ae->lastgloballabellen);
							sprintf(curlabelname,"%s%s",ae->lastgloballabel,varbuffer);
							MemFree(varbuffer);
							touched=1; // cause realloc!
							/**/
							curlabel.iw=-1;
							curlabel.name=varbuffer=curlabelname;
							curlabel.crc=GetCRC(varbuffer);
							//printf("push proximity label that may be exported [%s]->[%s]\n",ae->wl[ae->idx].w,varbuffer);
						} else {
							rasm_printf(ae,"[%s:%d] cannot create proximity label [%s] as there is no previous global label\n",GetExpFile(ae,0),ae->wl[ae->idx].l,varbuffer);
							MaxError(ae);
							return;
						}
					}
					break;
				default:
					if (!touched) {
						curlabel.iw=ae->idx;
					} else {
						curlabel.iw=-1;
						curlabel.name=varbuffer;
					}
					curlabel.crc=GetCRC(varbuffer);
					curlabelname=varbuffer;
					/* global labels set new reference */
					ae->lastgloballabel=ae->wl[ae->idx].w;
					ae->lastgloballabellen=strlen(ae->wl[ae->idx].w);
//printf("SET global label [%s] l=%d\n",ae->lastgloballabel,ae->lastgloballabellen);
					break;
			}

			/* contr�le dico uniquement avec des labels non locaux */
			if (SearchDico(ae,curlabelname,curlabel.crc)) {
				rasm_printf(ae,"[%s:%d] cannot create label [%s] as there is already a variable with the same name\n",GetExpFile(ae,0),ae->wl[ae->idx].l,curlabelname);
				MaxError(ae);
				return;
			}
			if(SearchAlias(ae,curlabel.crc,curlabelname)!=-1) {
				rasm_printf(ae,"[%s:%d] cannot create label [%s] as there is already an alias with the same name\n",GetExpFile(ae,0),ae->wl[ae->idx].l,curlabelname);
				MaxError(ae);
				return;
			}
		}
		curlabel.ptr=ae->codeadr;
		curlabel.ibank=ae->activebank;
		curlabel.iorgzone=ae->io-1;
		curlabel.lz=ae->lz;
	}

	if ((searched_label=SearchLabel(ae,curlabelname,curlabel.crc))!=NULL) {
		rasm_printf(ae,"[%s:%d] Duplicate label [%s] - previously defined in [%s:%d]\n",GetExpFile(ae,0),GetExpLine(ae,0),curlabelname,ae->filename[searched_label->fileidx],searched_label->fileline);
		MaxError(ae);
		if (curlabel.iw==-1) MemFree(curlabelname);
	} else {
//printf("PushLabel(%s) name=%s crc=%X\n",curlabelname,curlabel.name?curlabel.name:"null",curlabel.crc);
		curlabel.fileidx=ae->wl[ae->idx].ifile;
		curlabel.fileline=ae->wl[ae->idx].l;
		ObjectArrayAddDynamicValueConcat((void **)&ae->label,&ae->il,&ae->ml,&curlabel,sizeof(curlabel));
		InsertLabelToTree(ae,&curlabel);
	}

	if (!touched) MemFree(varbuffer);
}


unsigned char *EncodeSnapshotRLE(unsigned char *memin, int *lenout) {
	#undef FUNC
	#define FUNC "EncodeSnapshotRLE"
	
	int i,cpt,idx=0;
	unsigned char *memout=NULL;
	
	memout=MemMalloc(65540);
	
	for (i=0;i<65536;) {
		for (cpt=1;cpt<255;cpt++) if (memin[i]!=memin[i+cpt]) break;
		if (cpt>=3 || memin[i]==0xE5) {
			memout[idx++]=0xE5;
			memout[idx++]=cpt;
			memout[idx++]=memin[i];
			i+=cpt;
		} else {
			memout[idx++]=memin[i++];
		}
	}
	if (lenout) *lenout=idx;
	if (idx<65536) return memout;
	
	MemFree(memout);
	return NULL;
}



#undef FUNC
#define FUNC "Instruction CORE"

void _IN(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		if (strcmp(ae->wl[ae->idx+2].w,"(C)")==0) {
			switch (GetCRC(ae->wl[ae->idx+1].w)) {
				case CRC_0:
				case CRC_F:___output(ae,0xED);___output(ae,0x70);ae->nop+=4;break;
				case CRC_A:___output(ae,0xED);___output(ae,0x78);ae->nop+=3;break;
				case CRC_B:___output(ae,0xED);___output(ae,0x40);ae->nop+=4;break;
				case CRC_C:___output(ae,0xED);___output(ae,0x48);ae->nop+=4;break;
				case CRC_D:___output(ae,0xED);___output(ae,0x50);ae->nop+=4;break;
				case CRC_E:___output(ae,0xED);___output(ae,0x58);ae->nop+=4;break;
				case CRC_H:___output(ae,0xED);___output(ae,0x60);ae->nop+=4;break;
				case CRC_L:___output(ae,0xED);___output(ae,0x68);ae->nop+=4;break;
				default:
					rasm_printf(ae,"[%s:%d] syntax is IN [0,F,A,B,C,D,E,H,L],(C)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
					MaxError(ae);
			}
		} else if (strcmp(ae->wl[ae->idx+1].w,"A")==0 && StringIsMem(ae->wl[ae->idx+2].w)) {
			___output(ae,0xDB);
			PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
			ae->nop+=3;
		} else {
			rasm_printf(ae,"[%s:%d] IN [0,F,A,B,C,D,E,H,L],(C) or IN A,(n) only\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] IN [0,F,A,B,C,D,E,H,L],(C) or IN A,(n) only\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _OUT(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		if (strcmp(ae->wl[ae->idx+1].w,"(C)")==0) {
			switch (GetCRC(ae->wl[ae->idx+2].w)) {
				case CRC_0:___output(ae,0xED);___output(ae,0x71);ae->nop+=4;break;
				case CRC_A:___output(ae,0xED);___output(ae,0x79);ae->nop+=4;break;
				case CRC_B:___output(ae,0xED);___output(ae,0x41);ae->nop+=4;break;
				case CRC_C:___output(ae,0xED);___output(ae,0x49);ae->nop+=4;break;
				case CRC_D:___output(ae,0xED);___output(ae,0x51);ae->nop+=4;break;
				case CRC_E:___output(ae,0xED);___output(ae,0x59);ae->nop+=4;break;
				case CRC_H:___output(ae,0xED);___output(ae,0x61);ae->nop+=4;break;
				case CRC_L:___output(ae,0xED);___output(ae,0x69);ae->nop+=4;break;
				default:
					rasm_printf(ae,"[%s:%d] syntax is OUT (C),[0,A,B,C,D,E,H,L]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
					MaxError(ae);
			}
		} else if (strcmp(ae->wl[ae->idx+2].w,"A")==0 && StringIsMem(ae->wl[ae->idx+1].w)) {
			___output(ae,0xD3);
			PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
			ae->nop+=3;
		} else {
			rasm_printf(ae,"[%s:%d] OUT (C),[0,A,B,C,D,E,H,L] or OUT (n),A only\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] OUT (C),[0,A,B,C,D,E,H,L] or OUT (n),A only\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _EX(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_HL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_DE:___output(ae,0xEB);ae->nop+=1;break;
					case CRC_MSP:___output(ae,0xE3);ae->nop+=6;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is EX HL,[(SP),DE]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_AF:
				if (strcmp(ae->wl[ae->idx+2].w,"AF'")==0) {
					___output(ae,0x08);ae->nop+=1;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is EX AF,AF'\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
			case CRC_MSP:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_HL:___output(ae,0xE3);ae->nop+=6;break;
					case CRC_IX:___output(ae,0xDD);___output(ae,0xE3);ae->nop+=7;break;
					case CRC_IY:___output(ae,0xFD);___output(ae,0xE3);ae->nop+=7;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is EX (SP),[HL,IX,IY]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_DE:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_HL:___output(ae,0xEB);ae->nop+=1;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is EX DE,HL\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_IX:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_MSP:___output(ae,0xDD);___output(ae,0xE3);ae->nop+=7;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is EX IX,(SP)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_IY:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_MSP:___output(ae,0xFD);___output(ae,0xE3);ae->nop+=7;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is EX IY,(SP)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			default:
				rasm_printf(ae,"[%s:%d] syntax is EX [AF,DE,HL,(SP),IX,IY],reg16\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] Use EX reg16,reg16\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _SBC(struct s_assenv *ae) {
	if ((!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) || ((!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) && strcmp(ae->wl[ae->idx+1].w,"A")==0)) {
		if (!ae->wl[ae->idx+1].t) ae->idx++;
		/* do implicit A */
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,0x9F);ae->nop+=1;break;
			case CRC_MHL:___output(ae,0x9E);ae->nop+=2;break;
			case CRC_B:___output(ae,0x98);ae->nop+=1;break;
			case CRC_C:___output(ae,0x99);ae->nop+=1;break;
			case CRC_D:___output(ae,0x9A);ae->nop+=1;break;
			case CRC_E:___output(ae,0x9B);ae->nop+=1;break;
			case CRC_H:___output(ae,0x9C);ae->nop+=1;break;
			case CRC_L:___output(ae,0x9D);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x9C);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x9D);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x9C);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x9D);ae->nop+=2;break;
			case CRC_IX:case CRC_IY:
				rasm_printf(ae,"[%s:%d] Use SBC with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
				ae->idx++;
				return;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0x9E);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0x9E);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xDE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_HL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0xED);___output(ae,0x42);ae->nop+=4;break;
					case CRC_DE:___output(ae,0xED);___output(ae,0x52);ae->nop+=4;break;
					case CRC_HL:___output(ae,0xED);___output(ae,0x62);ae->nop+=4;break;
					case CRC_SP:___output(ae,0xED);___output(ae,0x72);ae->nop+=4;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is SBC HL,[BC,DE,HL,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			default:
				rasm_printf(ae,"[%s:%d] syntax is SBC HL,[BC,DE,HL,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid syntax for SBC\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _ADC(struct s_assenv *ae) {
	if ((!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) || ((!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) && strcmp(ae->wl[ae->idx+1].w,"A")==0)) {
		if (!ae->wl[ae->idx+1].t) ae->idx++;
		/* also implicit A */
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,0x8F);ae->nop+=1;break;
			case CRC_MHL:___output(ae,0x8E);ae->nop+=2;break;
			case CRC_B:___output(ae,0x88);ae->nop+=1;break;
			case CRC_C:___output(ae,0x89);ae->nop+=1;break;
			case CRC_D:___output(ae,0x8A);ae->nop+=1;break;
			case CRC_E:___output(ae,0x8B);ae->nop+=1;break;
			case CRC_H:___output(ae,0x8C);ae->nop+=1;break;
			case CRC_L:___output(ae,0x8D);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x8C);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x8D);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x8C);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x8D);ae->nop+=2;break;
			case CRC_IX:case CRC_IY:
				rasm_printf(ae,"[%s:%d] Use ADC with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
				ae->idx++;
				return;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0x8E);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0x8E);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xCE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_HL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0xED);___output(ae,0x4A);ae->nop+=4;break;
					case CRC_DE:___output(ae,0xED);___output(ae,0x5A);ae->nop+=4;break;
					case CRC_HL:___output(ae,0xED);___output(ae,0x6A);ae->nop+=4;break;
					case CRC_SP:___output(ae,0xED);___output(ae,0x7A);ae->nop+=4;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is ADC HL,[BC,DE,HL,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			default:
				rasm_printf(ae,"[%s:%d] syntax is ADC HL,[BC,DE,HL,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid syntax for ADC\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _ADD(struct s_assenv *ae) {
	if ((!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) || ((!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) && strcmp(ae->wl[ae->idx+1].w,"A")==0)) {
		if (!ae->wl[ae->idx+1].t) ae->idx++;
		/* also implicit A */
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,0x87);ae->nop+=1;break;
			case CRC_MHL:___output(ae,0x86);ae->nop+=2;break;
			case CRC_B:___output(ae,0x80);ae->nop+=1;break;
			case CRC_C:___output(ae,0x81);ae->nop+=1;break;
			case CRC_D:___output(ae,0x82);ae->nop+=1;break;
			case CRC_E:___output(ae,0x83);ae->nop+=1;break;
			case CRC_H:___output(ae,0x84);ae->nop+=1;break;
			case CRC_L:___output(ae,0x85);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x84);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x85);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x84);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x85);ae->nop+=2;break;
			case CRC_IX:case CRC_IY:
				rasm_printf(ae,"[%s:%d] Use ADD with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
				ae->idx++;
				return;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0x86);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0x86);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xC6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_HL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0x09);ae->nop+=3;break;
					case CRC_DE:___output(ae,0x19);ae->nop+=3;break;
					case CRC_HL:___output(ae,0x29);ae->nop+=3;break;
					case CRC_SP:___output(ae,0x39);ae->nop+=3;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is ADD HL,[BC,DE,HL,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_IX:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0xDD);___output(ae,0x09);ae->nop+=4;break;
					case CRC_DE:___output(ae,0xDD);___output(ae,0x19);ae->nop+=4;break;
					case CRC_IX:___output(ae,0xDD);___output(ae,0x29);ae->nop+=4;break;
					case CRC_SP:___output(ae,0xDD);___output(ae,0x39);ae->nop+=4;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is ADD IX,[BC,DE,IX,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			case CRC_IY:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0xFD);___output(ae,0x09);ae->nop+=4;break;
					case CRC_DE:___output(ae,0xFD);___output(ae,0x19);ae->nop+=4;break;
					case CRC_IY:___output(ae,0xFD);___output(ae,0x29);ae->nop+=4;break;
					case CRC_SP:___output(ae,0xFD);___output(ae,0x39);ae->nop+=4;break;
					default:
						rasm_printf(ae,"[%s:%d] syntax is ADD IY,[BC,DE,IY,SP]\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
				}
				break;
			default:
				rasm_printf(ae,"[%s:%d] syntax is ADD [HL,IX,IY],reg16\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid syntax for ADD\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _CP(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,0xBF);ae->nop+=1;break;
			case CRC_MHL:___output(ae,0xBE);ae->nop+=2;break;
			case CRC_B:___output(ae,0xB8);ae->nop+=1;break;
			case CRC_C:___output(ae,0xB9);ae->nop+=1;break;
			case CRC_D:___output(ae,0xBA);ae->nop+=1;break;
			case CRC_E:___output(ae,0xBB);ae->nop+=1;break;
			case CRC_H:___output(ae,0xBC);ae->nop+=1;break;
			case CRC_L:___output(ae,0xBD);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0xBC);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0xBD);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0xBC);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0xBD);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xBE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xBE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xFE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Syntax is CP reg8/(reg16)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _RET(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_NZ:___output(ae,0xC0);ae->nop+=2;break;
			case CRC_Z:___output(ae,0xC8);ae->nop+=2;break;
			case CRC_C:___output(ae,0xD8);ae->nop+=2;break;
			case CRC_NC:___output(ae,0xD0);ae->nop+=2;break;
			case CRC_PE:___output(ae,0xE8);ae->nop+=2;break;
			case CRC_PO:___output(ae,0xE0);ae->nop+=2;break;
			case CRC_P:___output(ae,0xF0);ae->nop+=2;break;
			case CRC_M:___output(ae,0xF8);ae->nop+=2;break;
			default:
				rasm_printf(ae,"[%s:%d] Available flags for RET are C,NC,Z,NZ,PE,PO,P,M\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
	} else if (ae->wl[ae->idx].t==1) {
		___output(ae,0xC9);
		ae->nop+=3;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid RET syntax\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _CALL(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_C:___output(ae,0xDC);ae->nop+=3;break;
			case CRC_Z:___output(ae,0xCC);ae->nop+=3;break;
			case CRC_NZ:___output(ae,0xC4);ae->nop+=3;break;
			case CRC_NC:___output(ae,0xD4);ae->nop+=3;break;
			case CRC_PE:___output(ae,0xEC);ae->nop+=3;break;
			case CRC_PO:___output(ae,0xE4);ae->nop+=3;break;
			case CRC_P:___output(ae,0xF4);ae->nop+=3;break;
			case CRC_M:___output(ae,0xFC);ae->nop+=3;break;
			default:
				rasm_printf(ae,"[%s:%d] Available flags for CALL are C,NC,Z,NZ,PE,PO,P,M\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		PushExpression(ae,ae->idx+2,E_EXPRESSION_V16C);
		ae->idx+=2;
	} else if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		___output(ae,0xCD);
		PushExpression(ae,ae->idx+1,E_EXPRESSION_V16C);
		ae->idx++;
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid CALL syntax\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _JR(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_NZ:___output(ae,0x20);ae->nop+=2;break;
			case CRC_C:___output(ae,0x38);ae->nop+=2;break;
			case CRC_Z:___output(ae,0x28);ae->nop+=2;break;
			case CRC_NC:___output(ae,0x30);ae->nop+=2;break;
			default:
				rasm_printf(ae,"[%s:%d] Available flags for JR are C,NC,Z,NZ\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		PushExpression(ae,ae->idx+2,E_EXPRESSION_J8);
		ae->idx+=2;
	} else if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		___output(ae,0x18);
		PushExpression(ae,ae->idx+1,E_EXPRESSION_J8);
		ae->idx++;
		ae->nop+=3;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid JR syntax\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _JP(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_C:___output(ae,0xDA);ae->nop+=3;break;
			case CRC_Z:___output(ae,0xCA);ae->nop+=3;break;
			case CRC_NZ:___output(ae,0xC2);ae->nop+=3;break;
			case CRC_NC:___output(ae,0xD2);ae->nop+=3;break;
			case CRC_PE:___output(ae,0xEA);ae->nop+=3;break;
			case CRC_PO:___output(ae,0xE2);ae->nop+=3;break;
			case CRC_P:___output(ae,0xF2);ae->nop+=3;break;
			case CRC_M:___output(ae,0xFA);ae->nop+=3;break;
			default:
				rasm_printf(ae,"[%s:%d] Available flags for JP are C,NC,Z,NZ,PE,PO,P,M\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
		ae->idx+=2;
	} else if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_HL:case CRC_MHL:___output(ae,0xE9);ae->nop+=1;break;
			case CRC_IX:case CRC_MIX:___output(ae,0xDD);___output(ae,0xE9);ae->nop+=2;break;
			case CRC_IY:case CRC_MIY:___output(ae,0xFD);___output(ae,0xE9);ae->nop+=2;break;
			default:
				___output(ae,0xC3);
				PushExpression(ae,ae->idx+1,E_EXPRESSION_V16);
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Invalid JP syntax\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _DEC(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			switch (GetCRC(ae->wl[ae->idx+1].w)) {
				case CRC_A:___output(ae,0x3D);ae->nop+=1;break;
				case CRC_B:___output(ae,0x05);ae->nop+=1;break;
				case CRC_C:___output(ae,0x0D);ae->nop+=1;break;
				case CRC_D:___output(ae,0x15);ae->nop+=1;break;
				case CRC_E:___output(ae,0x1D);ae->nop+=1;break;
				case CRC_H:___output(ae,0x25);ae->nop+=1;break;
				case CRC_L:___output(ae,0x2D);ae->nop+=1;break;
				case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x25);ae->nop+=2;break;
				case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x2D);ae->nop+=2;break;
				case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x25);ae->nop+=2;break;
				case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x2D);ae->nop+=2;break;
				case CRC_BC:___output(ae,0x0B);ae->nop+=2;break;
				case CRC_DE:___output(ae,0x1B);ae->nop+=2;break;
				case CRC_HL:___output(ae,0x2B);ae->nop+=2;break;
				case CRC_IX:___output(ae,0xDD);___output(ae,0x2B);ae->nop+=3;break;
				case CRC_IY:___output(ae,0xFD);___output(ae,0x2B);ae->nop+=3;break;
				case CRC_SP:___output(ae,0x3B);ae->nop+=2;break;
				case CRC_MHL:___output(ae,0x35);ae->nop+=3;break;
				default:
					if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x35);
						PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
						ae->nop+=6;
					} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x35);
						PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
						ae->nop+=6;
					} else {
						rasm_printf(ae,"[%s:%d] Use DEC with A,B,C,D,E,H,L,XH,XL,YH,YL,BC,DE,HL,SP,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
					}
			}
			ae->idx++;
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] Use DEC with A,B,C,D,E,H,L,XH,XL,YH,YL,BC,DE,HL,SP,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _INC(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			switch (GetCRC(ae->wl[ae->idx+1].w)) {
				case CRC_A:___output(ae,0x3C);ae->nop+=1;break;
				case CRC_B:___output(ae,0x04);ae->nop+=1;break;
				case CRC_C:___output(ae,0x0C);ae->nop+=1;break;
				case CRC_D:___output(ae,0x14);ae->nop+=1;break;
				case CRC_E:___output(ae,0x1C);ae->nop+=1;break;
				case CRC_H:___output(ae,0x24);ae->nop+=1;break;
				case CRC_L:___output(ae,0x2C);ae->nop+=1;break;
				case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x24);ae->nop+=2;break;
				case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x2C);ae->nop+=2;break;
				case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x24);ae->nop+=2;break;
				case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x2C);ae->nop+=2;break;
				case CRC_BC:___output(ae,0x03);ae->nop+=2;break;
				case CRC_DE:___output(ae,0x13);ae->nop+=2;break;
				case CRC_HL:___output(ae,0x23);ae->nop+=2;break;
				case CRC_IX:___output(ae,0xDD);___output(ae,0x23);ae->nop+=3;break;
				case CRC_IY:___output(ae,0xFD);___output(ae,0x23);ae->nop+=3;break;
				case CRC_SP:___output(ae,0x33);ae->nop+=2;break;
				case CRC_MHL:___output(ae,0x34);ae->nop+=3;break;
				default:
					if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x34);
						PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
						ae->nop+=6;
					} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x34);
						PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
						ae->nop+=6;
					} else {
						rasm_printf(ae,"[%s:%d] Use INC with A,B,C,D,E,H,L,XH,XL,YH,YL,BC,DE,HL,SP,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
						MaxError(ae);
					}
			}
			ae->idx++;
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] Use INC with A,B,C,D,E,H,L,XH,XL,YH,YL,BC,DE,HL,SP,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _SUB(struct s_assenv *ae) {
	#ifdef OPCODE
	#undef OPCODE
	#endif
	#define OPCODE 0x90
	
	if ((!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1)  || ((!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) && strcmp(ae->wl[ae->idx+1].w,"A")==0)) {
		if (!ae->wl[ae->idx+1].t) ae->idx++;
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,OPCODE+7);ae->nop+=1;break;
			case CRC_MHL:___output(ae,OPCODE+6);ae->nop+=2;break;
			case CRC_B:___output(ae,OPCODE);ae->nop+=1;break;
			case CRC_C:___output(ae,OPCODE+1);ae->nop+=1;break;
			case CRC_D:___output(ae,OPCODE+2);ae->nop+=1;break;
			case CRC_E:___output(ae,OPCODE+3);ae->nop+=1;break;
			case CRC_H:___output(ae,OPCODE+4);ae->nop+=1;break;
			case CRC_L:___output(ae,OPCODE+5);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,OPCODE+5);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,OPCODE+5);ae->nop+=2;break;
			case CRC_IX:case CRC_IY:
				rasm_printf(ae,"[%s:%d] Use SUB with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
				MaxError(ae);
				ae->idx++;
				return;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xD6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Use SUB with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _AND(struct s_assenv *ae) {
	#ifdef OPCODE
	#undef OPCODE
	#endif
	#define OPCODE 0xA0
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,OPCODE+7);ae->nop+=1;break;
			case CRC_MHL:___output(ae,OPCODE+6);ae->nop+=2;break;
			case CRC_B:___output(ae,OPCODE);ae->nop+=1;break;
			case CRC_C:___output(ae,OPCODE+1);ae->nop+=1;break;
			case CRC_D:___output(ae,OPCODE+2);ae->nop+=1;break;
			case CRC_E:___output(ae,OPCODE+3);ae->nop+=1;break;
			case CRC_H:___output(ae,OPCODE+4);ae->nop+=1;break;
			case CRC_L:___output(ae,OPCODE+5);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,OPCODE+5);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,OPCODE+5);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xE6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Use AND with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _OR(struct s_assenv *ae) {
	#ifdef OPCODE
	#undef OPCODE
	#endif
	#define OPCODE 0xB0
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,OPCODE+7);ae->nop+=1;break;
			case CRC_MHL:___output(ae,OPCODE+6);ae->nop+=2;break;
			case CRC_B:___output(ae,OPCODE);ae->nop+=1;break;
			case CRC_C:___output(ae,OPCODE+1);ae->nop+=1;break;
			case CRC_D:___output(ae,OPCODE+2);ae->nop+=1;break;
			case CRC_E:___output(ae,OPCODE+3);ae->nop+=1;break;
			case CRC_H:___output(ae,OPCODE+4);ae->nop+=1;break;
			case CRC_L:___output(ae,OPCODE+5);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,OPCODE+5);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,OPCODE+5);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xF6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Use OR with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetExpFile(ae,0),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _XOR(struct s_assenv *ae) {
	#ifdef OPCODE
	#undef OPCODE
	#endif
	#define OPCODE 0xA8
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:___output(ae,OPCODE+7);ae->nop+=1;break;
			case CRC_MHL:___output(ae,OPCODE+6);ae->nop+=2;break;
			case CRC_B:___output(ae,OPCODE);ae->nop+=1;break;
			case CRC_C:___output(ae,OPCODE+1);ae->nop+=1;break;
			case CRC_D:___output(ae,OPCODE+2);ae->nop+=1;break;
			case CRC_E:___output(ae,OPCODE+3);ae->nop+=1;break;
			case CRC_H:___output(ae,OPCODE+4);ae->nop+=1;break;
			case CRC_L:___output(ae,OPCODE+5);ae->nop+=1;break;
			case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,OPCODE+5);ae->nop+=2;break;
			case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,OPCODE+4);ae->nop+=2;break;
			case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,OPCODE+5);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,OPCODE+6);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					ae->nop+=3;
				} else {
					___output(ae,0xEE);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_V8);
					ae->nop+=2;
				}
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] Use XOR with A,B,C,D,E,H,L,XH,XL,YH,YL,(HL),(IX),(IY)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _POP(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			switch (GetCRC(ae->wl[ae->idx].w)) {
				case CRC_AF:___output(ae,0xF1);ae->nop+=3;break;
				case CRC_BC:___output(ae,0xC1);ae->nop+=3;break;
				case CRC_DE:___output(ae,0xD1);ae->nop+=3;break;
				case CRC_HL:___output(ae,0xE1);ae->nop+=3;break;
				case CRC_IX:___output(ae,0xDD);___output(ae,0xE1);ae->nop+=4;break;
				case CRC_IY:___output(ae,0xFD);___output(ae,0xE1);ae->nop+=4;break;
				default:
					rasm_printf(ae,"[%s:%d] Use POP with AF,BC,DE,HL,IX,IY\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
			}
		} while (ae->wl[ae->idx].t!=1);
	} else {
		rasm_printf(ae,"[%s:%d] POP need at least one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _PUSH(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			switch (GetCRC(ae->wl[ae->idx].w)) {
				case CRC_AF:___output(ae,0xF5);ae->nop+=4;break;
				case CRC_BC:___output(ae,0xC5);ae->nop+=4;break;
				case CRC_DE:___output(ae,0xD5);ae->nop+=4;break;
				case CRC_HL:___output(ae,0xE5);ae->nop+=4;break;
				case CRC_IX:___output(ae,0xDD);___output(ae,0xE5);ae->nop+=5;break;
				case CRC_IY:___output(ae,0xFD);___output(ae,0xE5);ae->nop+=5;break;
				default:
					rasm_printf(ae,"[%s:%d] Use PUSH with AF,BC,DE,HL,IX,IY\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
			}
		} while (ae->wl[ae->idx].t!=1);
	} else {
		rasm_printf(ae,"[%s:%d] PUSH need at least one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _IM(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		/* la valeur du parametre va definir l'opcode du IM */
		___output(ae,0xED);
		PushExpression(ae,ae->idx+1,E_EXPRESSION_IM);
		ae->idx++;
		ae->nop+=2;
	} else {
		rasm_printf(ae,"[%s:%d] IM need one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _RLCA(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x7);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] RLCA does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RRCA(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xF);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] RRCA does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _NEG(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0x44);
		ae->nop+=2;
	} else {
		rasm_printf(ae,"[%s:%d] NEG does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _DAA(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x27);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] DAA does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CPL(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x2F);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] CPL does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RETI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0x4D);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] RETI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _SCF(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x37);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] SCF does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _LDD(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA8);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] LDD does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _LDDR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB8);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] LDDR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _LDI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA0);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] LDI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _LDIR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB0);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] LDIR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CCF(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x3F);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] CCF does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CPD(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA9);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] CPD does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CPDR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB9);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] CPDR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CPI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA1);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] CPI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _CPIR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB1);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] CPIR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _OUTD(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xAB);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] OUTD does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _OTDR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xBB);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] OTDR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _OUTI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA3);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] OUTI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _OTIR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB3);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] OTIR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RETN(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0x45);
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] RETN does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _IND(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xAA);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] IND does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _INDR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xBA);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] INDR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _INI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xA2);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] INI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _INIR(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0xB2);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] INIR does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _EXX(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xD9);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] EXX does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _HALT(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x76);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] HALT does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _RLA(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x17);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] RLA does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RRA(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0x1F);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] RRA does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RLD(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0x6F);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] RLD does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _RRD(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xED);
		___output(ae,0x67);
		ae->nop+=5;
	} else {
		rasm_printf(ae,"[%s:%d] RRD does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _NOP(struct s_assenv *ae) {
	int o;

	if (ae->wl[ae->idx].t) {
		___output(ae,0x00);
	} else if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		o=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
		if (o>=0) {
			while (o>0) {
				___output(ae,0x00);
				ae->nop+=1;
				o--;
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] NOP is supposed to be used without parameter or with one optional parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _DI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
	___output(ae,0xF3);
	ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] DI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void _EI(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___output(ae,0xFB);
		ae->nop+=1;
	} else {
		rasm_printf(ae,"[%s:%d] EI does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _RST(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t!=2) {
		/* la valeur du parametre va definir l'opcode du RST */
		PushExpression(ae,ae->idx+1,E_EXPRESSION_RST);
		ae->idx++;
		ae->nop+=4;
	} else {
		rasm_printf(ae,"[%s:%d] RST need one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DJNZ(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t!=2) {
		___output(ae,0x10);
		PushExpression(ae,ae->idx+1,E_EXPRESSION_J8);
		ae->nop+=3;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] DJNZ need one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _LD(struct s_assenv *ae) {
	/* on check qu'il y a au moins deux parametres */
	if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_A:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_I:___output(ae,0xED);___output(ae,0x57);ae->nop+=3;break;
					case CRC_R:___output(ae,0xED);___output(ae,0x5F);ae->nop+=3;break;
					case CRC_B:___output(ae,0x78);ae->nop+=1;break;
					case CRC_C:___output(ae,0x79);ae->nop+=1;break;
					case CRC_D:___output(ae,0x7A);ae->nop+=1;break;
					case CRC_E:___output(ae,0x7B);ae->nop+=1;break;
					case CRC_H:___output(ae,0x7C);ae->nop+=1;break;
					case CRC_L:___output(ae,0x7D);ae->nop+=1;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x7C);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x7D);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x7C);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x7D);ae->nop+=2;break;
					case CRC_MHL:___output(ae,0x7E);ae->nop+=2;break;
					case CRC_A:___output(ae,0x7F);ae->nop+=1;break;
					case CRC_MBC:___output(ae,0x0A);ae->nop+=2;break;
					case CRC_MDE:___output(ae,0x1A);ae->nop+=2;break;
					default:
					/* (ix+expression) (iy+expression) (expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x7E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x7E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (StringIsMem(ae->wl[ae->idx+2].w)) {
						___output(ae,0x3A);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
						ae->nop+=4;
					} else {
						___output(ae,0x3E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_I:
				if (GetCRC(ae->wl[ae->idx+2].w)==CRC_A) {
					___output(ae,0xED);___output(ae,0x47);
					ae->nop+=3;
				} else {
					rasm_printf(ae,"[%s:%d] LD I,A only\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
			case CRC_R:
				if (GetCRC(ae->wl[ae->idx+2].w)==CRC_A) {
					___output(ae,0xED);___output(ae,0x4F);
					ae->nop+=3;
				} else {
					rasm_printf(ae,"[%s:%d] LD R,A only\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
			case CRC_B:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x40);ae->nop+=1;break;
					case CRC_C:___output(ae,0x41);ae->nop+=1;break;
					case CRC_D:___output(ae,0x42);ae->nop+=1;break;
					case CRC_E:___output(ae,0x43);ae->nop+=1;break;
					case CRC_H:___output(ae,0x44);ae->nop+=1;break;
					case CRC_L:___output(ae,0x45);ae->nop+=1;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x44);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x45);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x44);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x45);ae->nop+=2;break;
					case CRC_MHL:___output(ae,0x46);ae->nop+=2;break;
					case CRC_A:___output(ae,0x47);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x46);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x46);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x06);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_C:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x48);ae->nop+=1;break;
					case CRC_C:___output(ae,0x49);ae->nop+=1;break;
					case CRC_D:___output(ae,0x4A);ae->nop+=1;break;
					case CRC_E:___output(ae,0x4B);ae->nop+=1;break;
					case CRC_H:___output(ae,0x4C);ae->nop+=1;break;
					case CRC_L:___output(ae,0x4D);ae->nop+=1;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x4C);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x4D);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x4C);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x4D);ae->nop+=2;break;
					case CRC_MHL:___output(ae,0x4E);ae->nop+=2;break;
					case CRC_A:___output(ae,0x4F);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x4E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x4E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x0E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_D:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x50);ae->nop+=1;break;
					case CRC_C:___output(ae,0x51);ae->nop+=1;break;
					case CRC_D:___output(ae,0x52);ae->nop+=1;break;
					case CRC_E:___output(ae,0x53);ae->nop+=1;break;
					case CRC_H:___output(ae,0x54);ae->nop+=1;break;
					case CRC_L:___output(ae,0x55);ae->nop+=1;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x54);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x55);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x54);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x55);ae->nop+=2;break;
					case CRC_MHL:___output(ae,0x56);ae->nop+=2;break;
					case CRC_A:___output(ae,0x57);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x56);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x56);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x16);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_E:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x58);ae->nop+=1;break;
					case CRC_C:___output(ae,0x59);ae->nop+=1;break;
					case CRC_D:___output(ae,0x5A);ae->nop+=1;break;
					case CRC_E:___output(ae,0x5B);ae->nop+=1;break;
					case CRC_H:___output(ae,0x5C);ae->nop+=1;break;
					case CRC_L:___output(ae,0x5D);ae->nop+=1;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x5C);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x5D);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x5C);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x5D);ae->nop+=2;break;
					case CRC_MHL:___output(ae,0x5E);ae->nop+=2;break;
					case CRC_A:___output(ae,0x5F);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x5E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x5E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x1E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_IYH:case CRC_HY:case CRC_YH:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0xFD);___output(ae,0x60);ae->nop+=2;break;
					case CRC_C:___output(ae,0xFD);___output(ae,0x61);ae->nop+=2;break;
					case CRC_D:___output(ae,0xFD);___output(ae,0x62);ae->nop+=2;break;
					case CRC_E:___output(ae,0xFD);___output(ae,0x63);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x64);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x65);ae->nop+=2;break;
					case CRC_A:___output(ae,0xFD);___output(ae,0x67);ae->nop+=2;break;
					default:
						___output(ae,0xFD);___output(ae,0x26);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=3;
				}
				break;
			case CRC_IYL:case CRC_LY:case CRC_YL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0xFD);___output(ae,0x68);ae->nop+=2;break;
					case CRC_C:___output(ae,0xFD);___output(ae,0x69);ae->nop+=2;break;
					case CRC_D:___output(ae,0xFD);___output(ae,0x6A);ae->nop+=2;break;
					case CRC_E:___output(ae,0xFD);___output(ae,0x6B);ae->nop+=2;break;
					case CRC_IYH:case CRC_HY:case CRC_YH:___output(ae,0xFD);___output(ae,0x6C);ae->nop+=2;break;
					case CRC_IYL:case CRC_LY:case CRC_YL:___output(ae,0xFD);___output(ae,0x6D);ae->nop+=2;break;
					case CRC_A:___output(ae,0xFD);___output(ae,0x6F);ae->nop+=2;break;
					default:
						___output(ae,0xFD);___output(ae,0x2E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=3;
				}
				break;
			case CRC_IXH:case CRC_HX:case CRC_XH:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0xDD);___output(ae,0x60);ae->nop+=2;break;
					case CRC_C:___output(ae,0xDD);___output(ae,0x61);ae->nop+=2;break;
					case CRC_D:___output(ae,0xDD);___output(ae,0x62);ae->nop+=2;break;
					case CRC_E:___output(ae,0xDD);___output(ae,0x63);ae->nop+=2;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x64);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x65);ae->nop+=2;break;
					case CRC_A:___output(ae,0xDD);___output(ae,0x67);ae->nop+=2;break;
					default:
						___output(ae,0xDD);___output(ae,0x26);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=3;
				}
				break;
			case CRC_IXL:case CRC_LX:case CRC_XL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0xDD);___output(ae,0x68);ae->nop+=2;break;
					case CRC_C:___output(ae,0xDD);___output(ae,0x69);ae->nop+=2;break;
					case CRC_D:___output(ae,0xDD);___output(ae,0x6A);ae->nop+=2;break;
					case CRC_E:___output(ae,0xDD);___output(ae,0x6B);ae->nop+=2;break;
					case CRC_IXH:case CRC_HX:case CRC_XH:___output(ae,0xDD);___output(ae,0x6C);ae->nop+=2;break;
					case CRC_IXL:case CRC_LX:case CRC_XL:___output(ae,0xDD);___output(ae,0x6D);ae->nop+=2;break;
					case CRC_A:___output(ae,0xDD);___output(ae,0x6F);ae->nop+=2;break;
					default:
						___output(ae,0xDD);___output(ae,0x2E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=3;
				}
				break;
			case CRC_H:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x60);ae->nop+=1;break;
					case CRC_C:___output(ae,0x61);ae->nop+=1;break;
					case CRC_D:___output(ae,0x62);ae->nop+=1;break;
					case CRC_E:___output(ae,0x63);ae->nop+=1;break;
					case CRC_H:___output(ae,0x64);ae->nop+=1;break;
					case CRC_L:___output(ae,0x65);ae->nop+=1;break;
					case CRC_MHL:___output(ae,0x66);ae->nop+=2;break;
					case CRC_A:___output(ae,0x67);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x66);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x66);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x26);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_L:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x68);ae->nop+=1;break;
					case CRC_C:___output(ae,0x69);ae->nop+=1;break;
					case CRC_D:___output(ae,0x6A);ae->nop+=1;break;
					case CRC_E:___output(ae,0x6B);ae->nop+=1;break;
					case CRC_H:___output(ae,0x6C);ae->nop+=1;break;
					case CRC_L:___output(ae,0x6D);ae->nop+=1;break;
					case CRC_MHL:___output(ae,0x6E);ae->nop+=2;break;
					case CRC_A:___output(ae,0x6F);ae->nop+=1;break;
					default:
					/* (ix+expression) (iy+expression) expression */
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0x6E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0x6E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=5;
					} else {
						___output(ae,0x2E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
						ae->nop+=2;
					}
				}
				break;
			case CRC_MHL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_B:___output(ae,0x70);ae->nop+=2;break;
					case CRC_C:___output(ae,0x71);ae->nop+=2;break;
					case CRC_D:___output(ae,0x72);ae->nop+=2;break;
					case CRC_E:___output(ae,0x73);ae->nop+=2;break;
					case CRC_H:___output(ae,0x74);ae->nop+=2;break;
					case CRC_L:___output(ae,0x75);ae->nop+=2;break;
					case CRC_A:___output(ae,0x77);ae->nop+=2;break;
					default:
					/* expression */
					___output(ae,0x36);
					PushExpression(ae,ae->idx+2,E_EXPRESSION_V8);
					ae->nop+=3;
				}
				break;
			case CRC_MBC:
				if (GetCRC(ae->wl[ae->idx+2].w)==CRC_A)  {
					___output(ae,0x02);
					ae->nop+=2;
				} else {
					rasm_printf(ae,"[%s:%d] LD (BC),A only\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
			case CRC_MDE:
				if (GetCRC(ae->wl[ae->idx+2].w)==CRC_A)  {
					___output(ae,0x12);
					ae->nop+=2;
				} else {
					rasm_printf(ae,"[%s:%d] LD (DE),A only\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
			case CRC_HL:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0x60);___output(ae,0x69);ae->nop+=2;break;
					case CRC_DE:___output(ae,0x62);___output(ae,0x6B);ae->nop+=2;break;
					case CRC_HL:___output(ae,0x64);___output(ae,0x6D);ae->nop+=2;break;
					default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX+",4)==0) {
						/* enhanced LD HL,(IX+nn) */
						___output(ae,0xDD);___output(ae,0x66);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xDD);___output(ae,0x6E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY+",4)==0) {
						/* enhanced LD HL,(IY+nn) */
						___output(ae,0xFD);___output(ae,0x66);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xFD);___output(ae,0x6E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (StringIsMem(ae->wl[ae->idx+2].w)) {
						___output(ae,0x2A);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
						ae->nop+=5;
					} else {
						___output(ae,0x21);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
						ae->nop+=3;
					}
				}
				break;
			case CRC_BC:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0x40);___output(ae,0x49);ae->nop+=2;break;
					case CRC_DE:___output(ae,0x42);___output(ae,0x4B);ae->nop+=2;break;
					case CRC_HL:___output(ae,0x44);___output(ae,0x4D);ae->nop+=2;break;
					default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX+",4)==0) {
						/* enhanced LD BC,(IX+nn) */
						___output(ae,0xDD);___output(ae,0x46);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xDD);___output(ae,0x4E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY+",4)==0) {
						/* enhanced LD BC,(IY+nn) */
						___output(ae,0xFD);___output(ae,0x46);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xFD);___output(ae,0x4E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (StringIsMem(ae->wl[ae->idx+2].w)) {
						___output(ae,0xED);___output(ae,0x4B);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
						ae->nop+=6;
					} else {
						___output(ae,0x01);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
						ae->nop+=3;
					}
				}
				break;
			case CRC_DE:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_BC:___output(ae,0x50);___output(ae,0x59);ae->nop+=2;break;
					case CRC_DE:___output(ae,0x52);___output(ae,0x5B);ae->nop+=2;break;
					case CRC_HL:___output(ae,0x54);___output(ae,0x5D);ae->nop+=2;break;
					default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX+",4)==0) {
						/* enhanced LD DE,(IX+nn) */
						___output(ae,0xDD);___output(ae,0x56);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xDD);___output(ae,0x5E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY+",4)==0) {
						/* enhanced LD DE,(IY+nn) */
						___output(ae,0xFD);___output(ae,0x56);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV81);
						___output(ae,0xFD);___output(ae,0x5E);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						ae->nop+=10;
					} else if (StringIsMem(ae->wl[ae->idx+2].w)) {
						___output(ae,0xED);___output(ae,0x5B);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
						ae->nop+=6;
					} else {
						___output(ae,0x11);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
						ae->nop+=3;
					}
				}
				break;
			case CRC_IX:
				if (StringIsMem(ae->wl[ae->idx+2].w)) {
					___output(ae,0xDD);___output(ae,0x2A);
					PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
					ae->nop+=6;
				} else {
					___output(ae,0xDD);___output(ae,0x21);
					PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
					ae->nop+=4;
				}
				break;
			case CRC_IY:
				if (StringIsMem(ae->wl[ae->idx+2].w)) {
					___output(ae,0xFD);___output(ae,0x2A);
					PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
					ae->nop+=6;
				} else {
					___output(ae,0xFD);___output(ae,0x21);
					PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
					ae->nop+=4;
				}
				break;
			case CRC_SP:
				switch (GetCRC(ae->wl[ae->idx+2].w)) {
					case CRC_HL:___output(ae,0xF9);ae->nop+=2;break;
					case CRC_IX:___output(ae,0xDD);___output(ae,0xF9);ae->nop+=3;break;
					case CRC_IY:___output(ae,0xFD);___output(ae,0xF9);ae->nop+=3;break;
					default:
						if (StringIsMem(ae->wl[ae->idx+2].w)) {
							___output(ae,0xED);___output(ae,0x7B);
							PushExpression(ae,ae->idx+2,E_EXPRESSION_IV16);
							ae->nop+=6;
						} else {
							___output(ae,0x31);
							PushExpression(ae,ae->idx+2,E_EXPRESSION_V16);
							ae->nop+=3;
						}
				}
				break;
			default:
				/* (ix+expression) (iy+expression) (expression) expression */
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					switch (GetCRC(ae->wl[ae->idx+2].w)) {
						case CRC_B:___output(ae,0xDD);___output(ae,0x70);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_C:___output(ae,0xDD);___output(ae,0x71);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_D:___output(ae,0xDD);___output(ae,0x72);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_E:___output(ae,0xDD);___output(ae,0x73);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_H:___output(ae,0xDD);___output(ae,0x74);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_L:___output(ae,0xDD);___output(ae,0x75);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_A:___output(ae,0xDD);___output(ae,0x77);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_HL:___output(ae,0xDD);___output(ae,0x74);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xDD);___output(ae,0x75);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						case CRC_DE:___output(ae,0xDD);___output(ae,0x72);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xDD);___output(ae,0x73);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						case CRC_BC:___output(ae,0xDD);___output(ae,0x70);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xDD);___output(ae,0x71);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						default:___output(ae,0xDD);___output(ae,0x36);
							PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
							PushExpression(ae,ae->idx+2,E_EXPRESSION_3V8);
							ae->nop+=6;
					}
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					switch (GetCRC(ae->wl[ae->idx+2].w)) {
						case CRC_B:___output(ae,0xFD);___output(ae,0x70);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_C:___output(ae,0xFD);___output(ae,0x71);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_D:___output(ae,0xFD);___output(ae,0x72);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_E:___output(ae,0xFD);___output(ae,0x73);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_H:___output(ae,0xFD);___output(ae,0x74);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_L:___output(ae,0xFD);___output(ae,0x75);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_A:___output(ae,0xFD);___output(ae,0x77);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=5;break;
						case CRC_HL:___output(ae,0xFD);___output(ae,0x74);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xFD);___output(ae,0x75);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						case CRC_DE:___output(ae,0xFD);___output(ae,0x72);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xFD);___output(ae,0x73);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						case CRC_BC:___output(ae,0xFD);___output(ae,0x70);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV81);___output(ae,0xFD);___output(ae,0x71);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);ae->nop+=10;break;
						default:___output(ae,0xFD);___output(ae,0x36);
							PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
							PushExpression(ae,ae->idx+2,E_EXPRESSION_3V8);
							ae->nop+=6;
					}
				} else if (StringIsMem(ae->wl[ae->idx+1].w)) {
					switch (GetCRC(ae->wl[ae->idx+2].w)) {
						case CRC_A:___output(ae,0x32);PushExpression(ae,ae->idx+1,E_EXPRESSION_V16);ae->nop+=4;break;
						case CRC_BC:___output(ae,0xED);___output(ae,0x43);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV16);ae->nop+=6;break;
						case CRC_DE:___output(ae,0xED);___output(ae,0x53);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV16);ae->nop+=6;break;
						case CRC_HL:___output(ae,0x22);PushExpression(ae,ae->idx+1,E_EXPRESSION_V16);ae->nop+=5;break;
						case CRC_IX:___output(ae,0xDD);___output(ae,0x22);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV16);ae->nop+=6;break;
						case CRC_IY:___output(ae,0xFD);___output(ae,0x22);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV16);ae->nop+=6;break;
						case CRC_SP:___output(ae,0xED);___output(ae,0x73);PushExpression(ae,ae->idx+1,E_EXPRESSION_IV16);ae->nop+=6;break;
						default:
							rasm_printf(ae,"[%s:%d] LD (#nnnn),[A,BC,DE,HL,SP,IX,IY] only\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
							MaxError(ae);
					}
				} else {
					rasm_printf(ae,"[%s:%d] Unknown LD format\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
				break;
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] LD needs two parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _RLC(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_B:___output(ae,0xCB);___output(ae,0x0);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x1);ae->nop+=2;break;
			case CRC_D:___output(ae,0xCB);___output(ae,0x2);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x3);ae->nop+=2;break;
			case CRC_H:___output(ae,0xCB);___output(ae,0x4);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x5);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x6);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x7);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x6);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x6);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is RLC reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"(%s:%d] syntax is RLC (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x0);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x4);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x5);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x7);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is RLC (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	}
}

void _RRC(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_B:___output(ae,0xCB);___output(ae,0x8);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x9);ae->nop+=2;break;
			case CRC_D:___output(ae,0xCB);___output(ae,0xA);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0xB);ae->nop+=2;break;
			case CRC_H:___output(ae,0xCB);___output(ae,0xC);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0xD);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0xE);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0xF);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0xE);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0xE);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is RRC reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is RRC (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x8);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x9);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0xA);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0xB);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0xC);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0xD);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0xF);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is RRC (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	}
}


void _RL(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x10);___output(ae,0xCB);___output(ae,0x11);ae->nop+=4;break;
			case CRC_B:___output(ae,0xCB);___output(ae,0x10);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x11);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x12);___output(ae,0xCB);___output(ae,0x13);ae->nop+=4;break;
			case CRC_D:___output(ae,0xCB);___output(ae,0x12);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x13);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x14);___output(ae,0xCB);___output(ae,0x15);ae->nop+=4;break;
			case CRC_H:___output(ae,0xCB);___output(ae,0x14);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x15);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x16);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x17);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x16);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x16);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is RL reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is RL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x10);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x11);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x12);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x13);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x14);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x15);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x17);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is RL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is RL (IX+n),reg8 or RL reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _RR(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x18);___output(ae,0xCB);___output(ae,0x19);ae->nop+=4;break;
			case CRC_B:___output(ae,0xCB);___output(ae,0x18);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x19);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x1A);___output(ae,0xCB);___output(ae,0x1B);ae->nop+=4;break;
			case CRC_D:___output(ae,0xCB);___output(ae,0x1A);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x1B);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x1C);___output(ae,0xCB);___output(ae,0x1D);ae->nop+=4;break;
			case CRC_H:___output(ae,0xCB);___output(ae,0x1C);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x1D);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x1E);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x1F);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x1E);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x1E);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is RR reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is RR (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x18);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x19);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1A);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1B);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1C);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1D);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x1F);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is RR (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is RR (IX+n),reg8 or RR reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}





void _SLA(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x21);___output(ae,0xCB);___output(ae,0x10);ae->nop+=4;break; /* SLA C : RL B */
			case CRC_B:___output(ae,0xCB);___output(ae,0x20);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x21);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x23);___output(ae,0xCB);___output(ae,0x12);ae->nop+=4;break; /* SLA E : RL D */
			case CRC_D:___output(ae,0xCB);___output(ae,0x22);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x23);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x25);___output(ae,0xCB);___output(ae,0x14);ae->nop+=4;break; /* SLA L : RL H */
			case CRC_H:___output(ae,0xCB);___output(ae,0x24);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x25);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x26);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x27);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x26);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x26);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is SLA reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is SLL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x20);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x21);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x22);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x23);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x24);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x25);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x27);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is SLA (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is SLA reg8/(HL)/(IX+n)/(IY+n) or SLA (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _SRA(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x28);___output(ae,0xCB);___output(ae,0x19);ae->nop+=4;break; /* SRA B : RR C */
			case CRC_B:___output(ae,0xCB);___output(ae,0x28);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x29);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x2A);___output(ae,0xCB);___output(ae,0x1B);ae->nop+=4;break; /* SRA D : RR E */
			case CRC_D:___output(ae,0xCB);___output(ae,0x2A);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x2B);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x2C);___output(ae,0xCB);___output(ae,0x1D);ae->nop+=4;break; /* SRA H : RR L */
			case CRC_H:___output(ae,0xCB);___output(ae,0x2C);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x2D);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x2E);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x2F);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x2E);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x2E);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is SRA reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is SRA (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x28);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x29);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2A);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2B);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2C);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2D);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x2F);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is SRA (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is SRA reg8/(HL)/(IX+n)/(IY+n) or SRA (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _SLL(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x31);___output(ae,0xCB);___output(ae,0x10);ae->nop+=4;break; /* SLL C : RL B */
			case CRC_B:___output(ae,0xCB);___output(ae,0x30);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x31);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x33);___output(ae,0xCB);___output(ae,0x12);ae->nop+=4;break; /* SLL E : RL D */
			case CRC_D:___output(ae,0xCB);___output(ae,0x32);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x33);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x35);___output(ae,0xCB);___output(ae,0x14);ae->nop+=4;break; /* SLL L : RL H */
			case CRC_H:___output(ae,0xCB);___output(ae,0x34);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x35);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x36);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x37);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x36);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x36);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is SLL reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is SLL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x30);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x31);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x32);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x33);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x34);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x35);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x37);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is SLL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is SLL reg8/(HL)/(IX+n)/(IY+n) or SLL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _SRL(struct s_assenv *ae) {
	/* on check qu'il y a un ou deux parametres */
	if (ae->wl[ae->idx+1].t==1) {
		switch (GetCRC(ae->wl[ae->idx+1].w)) {
			case CRC_BC:___output(ae,0xCB);___output(ae,0x38);___output(ae,0xCB);___output(ae,0x11);ae->nop+=4;break; /* SRL B : RL C */
			case CRC_B:___output(ae,0xCB);___output(ae,0x38);ae->nop+=2;break;
			case CRC_C:___output(ae,0xCB);___output(ae,0x39);ae->nop+=2;break;
			case CRC_DE:___output(ae,0xCB);___output(ae,0x3A);___output(ae,0xCB);___output(ae,0x13);ae->nop+=4;break; /* SRL D : RL E */
			case CRC_D:___output(ae,0xCB);___output(ae,0x3A);ae->nop+=2;break;
			case CRC_E:___output(ae,0xCB);___output(ae,0x3B);ae->nop+=2;break;
			case CRC_HL:___output(ae,0xCB);___output(ae,0x3C);___output(ae,0xCB);___output(ae,0x15);ae->nop+=4;break; /* SRL H : RL L */
			case CRC_H:___output(ae,0xCB);___output(ae,0x3C);ae->nop+=2;break;
			case CRC_L:___output(ae,0xCB);___output(ae,0x3D);ae->nop+=2;break;
			case CRC_MHL:___output(ae,0xCB);___output(ae,0x3E);ae->nop+=4;break;
			case CRC_A:___output(ae,0xCB);___output(ae,0x3F);ae->nop+=2;break;
			default:
				if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
					___output(ae,0xDD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x3E);
					ae->nop+=7;
				} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
					___output(ae,0xFD);___output(ae,0xCB);
					PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);
					___output(ae,0x3E);
					ae->nop+=7;
				} else {
					rasm_printf(ae,"[%s:%d] syntax is SRL reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
		}
		ae->idx++;
	} else if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
		if (strncmp(ae->wl[ae->idx+1].w,"(IX",3)==0) {
			___output(ae,0xDD);
		} else if (strncmp(ae->wl[ae->idx+1].w,"(IY",3)==0) {
			___output(ae,0xFD);
		} else {
			rasm_printf(ae,"[%s:%d] syntax is SRL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		___output(ae,0xCB);
		switch (GetCRC(ae->wl[ae->idx+2].w)) {
			case CRC_B:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x38);ae->nop+=7;break;
			case CRC_C:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x39);ae->nop+=7;break;
			case CRC_D:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3A);ae->nop+=7;break;
			case CRC_E:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3B);ae->nop+=7;break;
			case CRC_H:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3C);ae->nop+=7;break;
			case CRC_L:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3D);ae->nop+=7;break;
			case CRC_A:PushExpression(ae,ae->idx+1,E_EXPRESSION_IV8);___output(ae,0x3F);ae->nop+=7;break;
			default:			
				rasm_printf(ae,"[%s:%d] syntax is SRL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
		ae->idx++;
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is SRL reg8/(HL)/(IX+n)/(IY+n) or SRL (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}


void _BIT(struct s_assenv *ae) {
	int o;
	/* on check qu'il y a deux ou trois parametres */
	ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
	o=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
	if (o<0 || o>7) {
		rasm_printf(ae,"[%s:%d] syntax is BIT <value from 0 to 7>,... (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,o);
		MaxError(ae);
	} else {
		o=0x40+o*8;
		if (ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
			switch (GetCRC(ae->wl[ae->idx+2].w)) {
				case CRC_B:___output(ae,0xCB);___output(ae,0x0+o);ae->nop+=2;break;
				case CRC_C:___output(ae,0xCB);___output(ae,0x1+o);ae->nop+=2;break;
				case CRC_D:___output(ae,0xCB);___output(ae,0x2+o);ae->nop+=2;break;
				case CRC_E:___output(ae,0xCB);___output(ae,0x3+o);ae->nop+=2;break;
				case CRC_H:___output(ae,0xCB);___output(ae,0x4+o);ae->nop+=2;break;
				case CRC_L:___output(ae,0xCB);___output(ae,0x5+o);ae->nop+=2;break;
				case CRC_MHL:___output(ae,0xCB);___output(ae,0x6+o);ae->nop+=3;break;
				case CRC_A:___output(ae,0xCB);___output(ae,0x7+o);ae->nop+=2;break;
				default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=6;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=6;
					} else {
						rasm_printf(ae,"[%s:%d] syntax is BIT n,reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					}
			}
			ae->idx+=2;
		} else if (!ae->wl[ae->idx+1].t && !ae->wl[ae->idx+2].t && ae->wl[ae->idx+3].t==1) {
			if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
				___output(ae,0xDD);
			} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
				___output(ae,0xFD);
			} else {
				rasm_printf(ae,"[%s:%d] syntax is BIT (IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
			___output(ae,0xCB);
			switch (GetCRC(ae->wl[ae->idx+3].w)) {
				case CRC_B:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x0+o);ae->nop+=6;break;
				case CRC_C:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x1+o);ae->nop+=6;break;
				case CRC_D:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x2+o);ae->nop+=6;break;
				case CRC_E:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x3+o);ae->nop+=6;break;
				case CRC_H:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x4+o);ae->nop+=6;break;
				case CRC_L:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x5+o);ae->nop+=6;break;
				case CRC_A:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x7+o);ae->nop+=6;break;
				default:			
					rasm_printf(ae,"[%s:%d] syntax is BIT n,(IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
			}
			ae->idx+=3;
		} else {
			rasm_printf(ae,"[%s:%d] syntax is BIT n,reg8/(HL)/(IX+n)[,reg8]/(IY+n)[,reg8]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	}
}

void _RES(struct s_assenv *ae) {
	int o;
	/* on check qu'il y a deux ou trois parametres */
	ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
	o=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
	if (o<0 || o>7) {
		rasm_printf(ae,"[%s:%d] syntax is RES <value from 0 to 7>,... (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,o);
		MaxError(ae);
	} else {
		o=0x80+o*8;
		if (ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
			switch (GetCRC(ae->wl[ae->idx+2].w)) {
				case CRC_B:___output(ae,0xCB);___output(ae,0x0+o);ae->nop+=2;break;
				case CRC_C:___output(ae,0xCB);___output(ae,0x1+o);ae->nop+=2;break;
				case CRC_D:___output(ae,0xCB);___output(ae,0x2+o);ae->nop+=2;break;
				case CRC_E:___output(ae,0xCB);___output(ae,0x3+o);ae->nop+=2;break;
				case CRC_H:___output(ae,0xCB);___output(ae,0x4+o);ae->nop+=2;break;
				case CRC_L:___output(ae,0xCB);___output(ae,0x5+o);ae->nop+=2;break;
				case CRC_MHL:___output(ae,0xCB);___output(ae,0x6+o);ae->nop+=4;break;
				case CRC_A:___output(ae,0xCB);___output(ae,0x7+o);ae->nop+=2;break;
				default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=7;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=7;
					} else {
						rasm_printf(ae,"[%s:%d] syntax is RES n,reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					}
			}
			ae->idx+=2;
		} else if (!ae->wl[ae->idx+1].t && !ae->wl[ae->idx+2].t && ae->wl[ae->idx+3].t==1) {
			if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
				___output(ae,0xDD);
			} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
				___output(ae,0xFD);
			} else {
				rasm_printf(ae,"[%s:%d] syntax is RES n,(IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
			___output(ae,0xCB);
			switch (GetCRC(ae->wl[ae->idx+3].w)) {
				case CRC_B:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x0+o);ae->nop+=7;break;
				case CRC_C:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x1+o);ae->nop+=7;break;
				case CRC_D:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x2+o);ae->nop+=7;break;
				case CRC_E:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x3+o);ae->nop+=7;break;
				case CRC_H:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x4+o);ae->nop+=7;break;
				case CRC_L:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x5+o);ae->nop+=7;break;
				case CRC_A:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x7+o);ae->nop+=7;break;
				default:			
					rasm_printf(ae,"[%s:%d] syntax is RES n,(IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
			}
			ae->idx+=3;
		} else {
			rasm_printf(ae,"[%s:%d] syntax is RES n,reg8/(HL)/(IX+n)[,reg8]/(IY+n)[,reg8]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	}
}

void _SET(struct s_assenv *ae) {
	int o;
	/* on check qu'il y a deux ou trois parametres */
	ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
	o=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
	if (o<0 || o>7) {
		rasm_printf(ae,"[%s:%d] syntax is SET <value from 0 to 7>,... (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,o);
		MaxError(ae);
	} else {
		o=0xC0+o*8;
		if (ae->wl[ae->idx+1].t==0 && ae->wl[ae->idx+2].t==1) {
			switch (GetCRC(ae->wl[ae->idx+2].w)) {
				case CRC_B:___output(ae,0xCB);___output(ae,0x0+o);ae->nop+=2;break;
				case CRC_C:___output(ae,0xCB);___output(ae,0x1+o);ae->nop+=2;break;
				case CRC_D:___output(ae,0xCB);___output(ae,0x2+o);ae->nop+=2;break;
				case CRC_E:___output(ae,0xCB);___output(ae,0x3+o);ae->nop+=2;break;
				case CRC_H:___output(ae,0xCB);___output(ae,0x4+o);ae->nop+=2;break;
				case CRC_L:___output(ae,0xCB);___output(ae,0x5+o);ae->nop+=2;break;
				case CRC_MHL:___output(ae,0xCB);___output(ae,0x6+o);ae->nop+=4;break;
				case CRC_A:___output(ae,0xCB);___output(ae,0x7+o);ae->nop+=2;break;
				default:
					if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
						___output(ae,0xDD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=7;
					} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
						___output(ae,0xFD);___output(ae,0xCB);
						PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);
						___output(ae,0x6+o);
						ae->nop+=7;
					} else {
						rasm_printf(ae,"[%s:%d] syntax is SET n,reg8/(HL)/(IX+n)/(IY+n)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					}
			}
			ae->idx+=2;
		} else if (!ae->wl[ae->idx+1].t && !ae->wl[ae->idx+2].t && ae->wl[ae->idx+3].t==1) {
			if (strncmp(ae->wl[ae->idx+2].w,"(IX",3)==0) {
				___output(ae,0xDD);
			} else if (strncmp(ae->wl[ae->idx+2].w,"(IY",3)==0) {
				___output(ae,0xFD);
			} else {
				rasm_printf(ae,"[%s:%d] syntax is SET n,(IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
			___output(ae,0xCB);
			switch (GetCRC(ae->wl[ae->idx+3].w)) {
				case CRC_B:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x0+o);ae->nop+=7;break;
				case CRC_C:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x1+o);ae->nop+=7;break;
				case CRC_D:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x2+o);ae->nop+=7;break;
				case CRC_E:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x3+o);ae->nop+=7;break;
				case CRC_H:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x4+o);ae->nop+=7;break;
				case CRC_L:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x5+o);ae->nop+=7;break;
				case CRC_A:PushExpression(ae,ae->idx+2,E_EXPRESSION_IV8);___output(ae,0x7+o);ae->nop+=7;break;
				default:			
					rasm_printf(ae,"[%s:%d] syntax is SET n,(IX+n),reg8\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
			}
			ae->idx+=3;
		} else {
			rasm_printf(ae,"[%s:%d] syntax is SET n,reg8/(HL)/(IX+n)[,reg8]/(IY+n)[,reg8]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	}
}

void _DEFS(struct s_assenv *ae) {
	int i,r,v;
	if (ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] Syntax is DEFS repeat,value or DEFS repeat\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	} else do {
		ae->idx++;
		if (!ae->wl[ae->idx].t) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx].w,0);
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			r=RoundComputeExpressionCore(ae,ae->wl[ae->idx].w,ae->codeadr,0);
			v=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
			if (r<0) {
				rasm_printf(ae,"[%s:%d] DEFS size must be greater or equal to zero\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
			for (i=0;i<r;i++) {
				___output(ae,v);
				ae->nop+=1;
			}
			ae->idx++;
		} else if (ae->wl[ae->idx].t==1) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx].w,0);
			r=RoundComputeExpressionCore(ae,ae->wl[ae->idx].w,ae->codeadr,0);
			v=0;
			if (r<0) {
				rasm_printf(ae,"[%s:%d] DEFS size must be greater or equal to zero\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
			for (i=0;i<r;i++) {
				___output(ae,v);
				ae->nop+=1;
			}
		}
	} while (!ae->wl[ae->idx].t);
}

void _STR(struct s_assenv *ae) {
	unsigned char c;
	int i,tquote;

	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			if ((tquote=StringIsQuote(ae->wl[ae->idx].w))!=0) {
				i=1;
				while (ae->wl[ae->idx].w[i] && ae->wl[ae->idx].w[i]!=tquote) {
					if (ae->wl[ae->idx].w[i]=='\\') {
						i++;
						/* no conversion on escaped chars */
						c=ae->wl[ae->idx].w[i];
						switch (c) {
							case 'b':c='\b';break;
							case 'v':c='\v';break;
							case 'f':c='\f';break;
							case '0':c='\0';break;
							case 'r':c='\r';break;
							case 'n':c='\n';break;
							case 't':c='\t';break;
							default:break;
						}						
						if (ae->wl[ae->idx].w[i+1]!=tquote) {
							___output(ae,c);
						} else {
							___output(ae,c|0x80);
						}
					} else {
						/* charset conversion on the fly */
						if (ae->wl[ae->idx].w[i+1]!=tquote) {
							___output(ae,ae->charset[(int)ae->wl[ae->idx].w[i]]);
						} else {
							___output(ae,ae->charset[(int)ae->wl[ae->idx].w[i]]|0x80);
						}
					}

					i++;
				}
			} else {
				rasm_printf(ae,"[%s:%d] STR handle only quoted strings!\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] STR needs one or more quotes parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFR(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			PushExpression(ae,ae->idx,E_EXPRESSION_0VR);
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] DEFR needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFB(struct s_assenv *ae) {
	int i,tquote;
	unsigned char c;
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			if ((tquote=StringIsQuote(ae->wl[ae->idx].w))!=0) {
				i=1;
				while (ae->wl[ae->idx].w[i] && ae->wl[ae->idx].w[i]!=tquote) {
					if (ae->wl[ae->idx].w[i]=='\\') {
						i++;
						/* no conversion on escaped chars */
						c=ae->wl[ae->idx].w[i];
						switch (c) {
							case 'b':___output(ae,'\b');break;
							case 'v':___output(ae,'\v');break;
							case 'f':___output(ae,'\f');break;
							case '0':___output(ae,'\0');break;
							case 'r':___output(ae,'\r');break;
							case 'n':___output(ae,'\n');break;
							case 't':___output(ae,'\t');break;
							default:
							___output(ae,c);
							ae->nop+=1;
						}						
					} else {
						/* charset conversion on the fly */
						___output(ae,ae->charset[(int)ae->wl[ae->idx].w[i]]);
						ae->nop+=1;
					}
					i++;
				}
			} else {
				PushExpression(ae,ae->idx,E_EXPRESSION_0V8);
				ae->nop+=1;
			}
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] DEFB needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFW(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			PushExpression(ae,ae->idx,E_EXPRESSION_0V16);
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] DEFW needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFI(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			PushExpression(ae,ae->idx,E_EXPRESSION_0V32);
		} while (ae->wl[ae->idx].t==0);
	} else {
		rasm_printf(ae,"[%s:%d] DEFI needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFB_as80(struct s_assenv *ae) {
	int i,tquote;
	int modadr=0;

	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			if ((tquote=StringIsQuote(ae->wl[ae->idx].w))!=0) {
				i=1;
				while (ae->wl[ae->idx].w[i] && ae->wl[ae->idx].w[i]!=tquote) {
					if (ae->wl[ae->idx].w[i]=='\\') i++;
					/* charset conversion on the fly */
					___output(ae,ae->charset[(int)ae->wl[ae->idx].w[i]]);
					ae->nop+=1;
					ae->codeadr--;modadr++;
					i++;
				}
			} else {
				PushExpression(ae,ae->idx,E_EXPRESSION_0V8);
				ae->codeadr--;modadr++;
				ae->nop+=1;
			}
		} while (ae->wl[ae->idx].t==0);
		ae->codeadr+=modadr;
	} else {
		rasm_printf(ae,"[%s:%d] DEFB needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFW_as80(struct s_assenv *ae) {
	int modadr=0;
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			PushExpression(ae,ae->idx,E_EXPRESSION_0V16);
			ae->codeadr-=2;modadr+=2;
		} while (ae->wl[ae->idx].t==0);
		ae->codeadr+=modadr;
	} else {
		rasm_printf(ae,"[%s:%d] DEFW needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void _DEFI_as80(struct s_assenv *ae) {
	int modadr=0;
	if (!ae->wl[ae->idx].t) {
		do {
			ae->idx++;
			PushExpression(ae,ae->idx,E_EXPRESSION_0V32);
			ae->codeadr-=4;modadr+=4;
		} while (ae->wl[ae->idx].t==0);
		ae->codeadr+=modadr;
	} else {
		rasm_printf(ae,"[%s:%d] DEFI needs one or more parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
#if 0
void _DEFSTR(struct s_assenv *ae) {
	int i,tquote;
	unsigned char c;
	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		if (StringIsQuote(ae->wl[ae->idx+1].w) && StringIsQuote(ae->wl[ae->idx+2].w)) {
				i=1;
				while (ae->wl[ae->idx].w[i] && ae->wl[ae->idx].w[i]!=tquote) {
					if (ae->wl[ae->idx].w[i]=='\\') {
						i++;
						/* no conversion on escaped chars */
						c=ae->wl[ae->idx].w[i];
						switch (c) {
							case 'b':___output(ae,'\b');break;
							case 'v':___output(ae,'\v');break;
							case 'f':___output(ae,'\f');break;
							case '0':___output(ae,'\0');break;
							case 'r':___output(ae,'\r');break;
							case 'n':___output(ae,'\n');break;
							case 't':___output(ae,'\t');break;
							default:
							___output(ae,c);
						}						
					} else {
						/* charset conversion on the fly */
						___output(ae,ae->charset[(int)ae->wl[ae->idx].w[i]]);
					}
					i++;
				}
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] DEFSTR needs two parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
#endif

#undef FUNC
#define FUNC "Directive CORE"

void __internal_UpdateLZBlockIfAny(struct s_assenv *ae) {
	/* there was a crunched block opened in the previous bank */
	if (ae->lz>=0) {
		//ae->lzsection[ae->ilz-1].iorgzone=ae->io-1;
		//ae->lzsection[ae->ilz-1].ibank=ae->activebank;
	}
	ae->lz=-1;
}


void __AMSDOS(struct s_assenv *ae) {
	ae->amsdos=1;
}

void __BUILDCPR(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] BUILDCPR does not need a parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
	if (!ae->forcesnapshot) {
		ae->forcecpr=1;
	} else {
		rasm_printf(ae,"[%s:%d] Cannot select cartridge output when already in snapshot output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __BUILDSNA(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		if (strcmp(ae->wl[ae->idx+1].w,"V2")==0) {
		ae->snapshot.version=2;
		} else {
			rasm_printf(ae,"[%s:%d] BUILDSNA unrecognized option\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	}
	if (!ae->forcecpr) {
		ae->forcesnapshot=1;
	} else {
		rasm_printf(ae,"[%s:%d] Cannot select snapshot output when already in cartridge output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
	

void __LZ4(struct s_assenv *ae) {
	struct s_lz_section curlz;
	
	#ifdef NO_3RD_PARTIES
		rasm_printf(ae,"[%s:%d] Cannot use 3rd parties cruncher with this version of RASM\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(-5);
	#endif
	
	if (ae->lz>=0 && ae->lz<ae->ilz) {
		rasm_printf(ae,"[%s:%d] Cannot start a new LZ section inside another one (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->lz);
		FreeAssenv(ae);
		exit(-5);
	}
	curlz.iw=ae->idx;
	curlz.iorgzone=ae->io-1;
	curlz.ibank=ae->activebank;
	curlz.memstart=ae->outputadr;
	curlz.memend=-1;
	curlz.lzversion=4;
	ae->lz=ae->ilz;
	ObjectArrayAddDynamicValueConcat((void**)&ae->lzsection,&ae->ilz,&ae->mlz,&curlz,sizeof(curlz));
}
void __LZX7(struct s_assenv *ae) {
	struct s_lz_section curlz;
	
	#ifdef NO_3RD_PARTIES
		rasm_printf(ae,"[%s:%d] Cannot use 3rd parties cruncher with this version of RASM\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(-5);
	#endif
	
	if (ae->lz>=0 && ae->lz<ae->ilz) {
		rasm_printf(ae,"[%s:%d] Cannot start a new LZ section inside another one (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->lz);
		FreeAssenv(ae);
		exit(-5);
	}
	curlz.iw=ae->idx;
	curlz.iorgzone=ae->io-1;
	curlz.ibank=ae->activebank;
	curlz.memstart=ae->outputadr;
	curlz.memend=-1;
	curlz.lzversion=7;
	ae->lz=ae->ilz;
	ObjectArrayAddDynamicValueConcat((void**)&ae->lzsection,&ae->ilz,&ae->mlz,&curlz,sizeof(curlz));
}
void __LZEXO(struct s_assenv *ae) {
	struct s_lz_section curlz;
	
	#ifdef NO_3RD_PARTIES
		rasm_printf(ae,"[%s:%d] Cannot use 3rd parties cruncher with this version of RASM\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(-5);
	#endif
	
	if (ae->lz>=0 && ae->lz<ae->ilz) {
		rasm_printf(ae,"[%s:%d] Cannot start a new LZ section inside another one (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->lz);
		FreeAssenv(ae);
		exit(-5);
	}
	curlz.iw=ae->idx;
	curlz.iorgzone=ae->io-1;
	curlz.ibank=ae->activebank;
	curlz.memstart=ae->outputadr;
	curlz.memend=-1;
	curlz.lzversion=8;
	ae->lz=ae->ilz;
	ObjectArrayAddDynamicValueConcat((void**)&ae->lzsection,&ae->ilz,&ae->mlz,&curlz,sizeof(curlz));
}
void __LZ48(struct s_assenv *ae) {
	struct s_lz_section curlz;
	if (ae->lz>=0 && ae->lz<ae->ilz) {
		rasm_printf(ae,"[%s:%d] Cannot start a new LZ section inside another one (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->lz);
		FreeAssenv(ae);
		exit(-5);
	}
	curlz.iw=ae->idx;
	curlz.iorgzone=ae->io-1;
	curlz.ibank=ae->activebank;
	curlz.memstart=ae->outputadr;
	curlz.memend=-1;
	curlz.lzversion=48;
	ae->lz=ae->ilz;
	ObjectArrayAddDynamicValueConcat((void**)&ae->lzsection,&ae->ilz,&ae->mlz,&curlz,sizeof(curlz));
}
void __LZ49(struct s_assenv *ae) {
	struct s_lz_section curlz;
	
	if (ae->lz>=0 && ae->lz<ae->ilz) {
		rasm_printf(ae,"[%s:%d] Cannot start a new LZ section inside another one (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->lz);
		FreeAssenv(ae);
		exit(-5);
	}
	
	curlz.iw=ae->idx;
	curlz.iorgzone=ae->io-1;
	curlz.ibank=ae->activebank;
	curlz.memstart=ae->outputadr;
	curlz.memend=-1;
	curlz.lzversion=49;
	ae->lz=ae->ilz;
	ObjectArrayAddDynamicValueConcat((void**)&ae->lzsection,&ae->ilz,&ae->mlz,&curlz,sizeof(curlz));
}
void __LZCLOSE(struct s_assenv *ae) {
	if (!ae->ilz || ae->lz==-1) {
		rasm_printf(ae,"[%s:%d] Cannot close LZ section as it wasn't opened\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	
	ae->lzsection[ae->ilz-1].memend=ae->outputadr;
	ae->lzsection[ae->ilz-1].ilabel=ae->il;
	ae->lzsection[ae->ilz-1].iexpr=ae->ie;
	//ae->lz=ae->ilz;
	ae->lz=-1;
}

void __LIMIT(struct s_assenv *ae) {
	if (ae->wl[ae->idx+1].t!=2) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		___output_set_limit(ae,RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->outputadr,0,0));
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] LIMIT directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void OverWriteCheck(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "OverWriteCheck"
	
	int i,j;
	
	/* overwrite checking */
	i=ae->io-1; {
		if (ae->orgzone[i].memstart!=ae->orgzone[i].memend) {
			for (j=0;j<ae->io-1;j++) {
				if (ae->orgzone[j].memstart!=ae->orgzone[j].memend && !ae->orgzone[j].nocode) {
					if (ae->orgzone[i].ibank==ae->orgzone[j].ibank) {
						if ((ae->orgzone[i].memstart>=ae->orgzone[j].memstart && ae->orgzone[i].memstart<ae->orgzone[j].memend)
							|| (ae->orgzone[i].memend>ae->orgzone[j].memstart && ae->orgzone[i].memend<ae->orgzone[j].memend)
							|| (ae->orgzone[i].memstart<=ae->orgzone[j].memstart && ae->orgzone[i].memend>=ae->orgzone[j].memend)) {
							ae->idx--;
							if (ae->orgzone[j].protect) {
								rasm_printf(ae,"PROTECTED section error",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->orgzone[j].memstart,ae->orgzone[j].memend,ae->orgzone[j].ibank<32?ae->orgzone[j].ibank:0);
							} else {
								rasm_printf(ae,"Assembling overwrite ",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->orgzone[j].memstart,ae->orgzone[j].memend);
							}
							rasm_printf(ae," [%s] L%d [#%04X-#%04X-B%d] with [%s] L%d [#%04X/#%04X]\n",ae->filename[ae->orgzone[j].ifile],ae->orgzone[j].iline,ae->orgzone[j].memstart,ae->orgzone[j].memend,ae->orgzone[j].ibank<32?ae->orgzone[j].ibank:0,ae->filename[ae->orgzone[i].ifile],ae->orgzone[i].iline,ae->orgzone[i].memstart,ae->orgzone[i].memend);
							MaxError(ae);
							i=j=ae->io;
							break;
						}
					}
				}
			}
		}
	}	
}

void ___new_memory_space(struct s_assenv *ae)
{
	#undef FUNC
	#define FUNC "___new_memory_space"
	
	unsigned char *mem;
	struct s_orgzone orgzone={0};

	__internal_UpdateLZBlockIfAny(ae);
	if (ae->io) {
		ae->orgzone[ae->io-1].memend=ae->outputadr;
	}
	if (ae->lz>=0) {
		if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: LZ section wasn't closed before a new memory space directive\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		__LZCLOSE(ae);
	}
	ae->activebank=ae->nbbank;
	mem=MemMalloc(65536);
	memset(mem,0,65536);
	ObjectArrayAddDynamicValueConcat((void**)&ae->mem,&ae->nbbank,&ae->maxbank,&mem,sizeof(mem));

	ae->outputadr=0;
	ae->codeadr=0;
	orgzone.memstart=0;
	orgzone.ibank=ae->activebank;
	orgzone.nocode=ae->nocode=0;
	ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));

	OverWriteCheck(ae);
}

void __BANK(struct s_assenv *ae) {
	struct s_orgzone orgzone={0};
	int oldcode=0,oldoutput=0;
	int i;

	__internal_UpdateLZBlockIfAny(ae);

	if (ae->io) {
		ae->orgzone[ae->io-1].memend=ae->outputadr;
	}
	/* without parameter, create a new empty space */
	if (ae->wl[ae->idx].t==1) {
		___new_memory_space(ae);
		return;
	}
	
	ae->bankmode=1;
	if (!ae->forcecpr && !ae->forcesnapshot) ae->forcecpr=1;

	if (ae->wl[ae->idx+1].t!=2) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		ae->activebank=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		if (ae->forcecpr && (ae->activebank<0 || ae->activebank>31)) {
			rasm_printf(ae,"[%s:%d] FATAL - Bank selection must be from 0 to 31 in cartridge mode\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			FreeAssenv(ae);
			exit(2);
		} else if (ae->forcesnapshot && (ae->activebank<0 || ae->activebank>35)) {
			rasm_printf(ae,"[%s:%d] FATAL - Bank selection must be from 0 to 35 in snapshot mode\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			FreeAssenv(ae);
			exit(2);
		}
		/* bankset control */
		if (ae->forcesnapshot && ae->bankset[ae->activebank/4]) {
			rasm_printf(ae,"[%s:%d] Cannot BANK %d was already select by a previous BANKSET %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->activebank,(int)ae->activebank/4);
			MaxError(ae);
		} else {
			ae->bankused[ae->activebank]=1;
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] BANK directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
	if (ae->lz>=0) {
		if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: LZ section wasn't closed before a new BANK directive\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		__LZCLOSE(ae);
	}

	/* try to get an old ORG settings backward */
	for (i=ae->io-1;i>=0;i--) {
		if (ae->orgzone[i].ibank==ae->activebank) {
			oldcode=ae->orgzone[i].memend;
			oldoutput=ae->orgzone[i].memend;
			break;
		}
	}
	ae->outputadr=oldoutput;
	ae->codeadr=oldcode;
	orgzone.memstart=ae->outputadr;
	/* legacy */
	orgzone.ibank=ae->activebank;
	orgzone.nocode=ae->nocode=0;
	ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));

	OverWriteCheck(ae);
}

void __BANKSET(struct s_assenv *ae) {
	struct s_orgzone orgzone={0};
	int ibank;

	__internal_UpdateLZBlockIfAny(ae);

	if (!ae->forcesnapshot && !ae->forcecpr) ae->forcesnapshot=1;
	if (!ae->forcesnapshot) {
		rasm_printf(ae,"[%s:%d] BANKSET directive is specific to snapshot output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	
	if (ae->io) {
		ae->orgzone[ae->io-1].memend=ae->outputadr;
	}
	ae->bankmode=1;
	
	if (ae->wl[ae->idx+1].t!=2) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		ae->activebank=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ae->activebank*=4;
		if (ae->forcesnapshot && (ae->activebank<0 || ae->activebank>35)) {
			rasm_printf(ae,"[%s:%d] FATAL - Bank set selection must be from 0 to 8 in snapshot mode\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			FreeAssenv(ae);
			exit(2);
		}
		/* control */
		ibank=ae->activebank;
		if (ae->bankused[ibank] || ae->bankused[ibank+1]|| ae->bankused[ibank+2]|| ae->bankused[ibank+3]) {
			rasm_printf(ae,"[%s:%d] Cannot BANKSET because bank %d was already selected in single page mode\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ibank);
			MaxError(ae);
		} else {	
			ae->bankset[ae->activebank/4]=1; /* pas tr�s heureux mais bon... */
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] BANKSET directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
	if (ae->lz>=0) {
		if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: LZ section wasn't closed before a new BANKSET directive\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		__LZCLOSE(ae);
	}

	ae->outputadr=0;
	ae->codeadr=0;
	orgzone.memstart=0;
	orgzone.ibank=ae->activebank;
	orgzone.nocode=ae->nocode=0;
	ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));

	OverWriteCheck(ae);
}


void __NameBANK(struct s_assenv *ae) {
	struct s_orgzone orgzone={0};
	int ibank;

	ae->bankmode=1;
	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		if (!StringIsQuote(ae->wl[ae->idx+2].w)) {
			rasm_printf(ae,"[%s:%d] Syntax is NAMEBANK <bank number>,'<string>'\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		} else {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			ibank=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
			if (ibank<0 || ibank>35) {
				rasm_printf(ae,"[%s:%d] NAMEBANK selection must be from 0 to 31\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			} else {
				ae->iwnamebank[ibank]=ae->idx+2;
			}
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] NAMEBANK directive need one integer parameter and a string\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

/***
	Winape little compatibility for CPR writing!
*/
void __WRITE(struct s_assenv *ae) {
	int idx=1,ok=0;
	int lower=-1,upper=-1,bank=-1;

	if (!ae->wl[ae->idx].t && strcmp(ae->wl[ae->idx+1].w,"DIRECT")==0 && !ae->wl[ae->idx+1].t) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,0);
		lower=RoundComputeExpression(ae,ae->wl[ae->idx+2].w,ae->codeadr,0,0);
		if (!ae->wl[ae->idx+2].t) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+3].w,0);
			upper=RoundComputeExpression(ae,ae->wl[ae->idx+3].w,ae->codeadr,0,0);
		}
		if (!ae->wl[ae->idx+3].t) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+4].w,0);
			bank=RoundComputeExpression(ae,ae->wl[ae->idx+4].w,ae->codeadr,0,0);
		}

		if (ae->maxam) {
			if (lower==65535) lower=-1;
			if (upper==65535) upper=-1;
			if (bank==65535) bank=-1;
		}

		if (lower!=-1) {
			if (lower>=0 && lower<8) {
				ae->idx+=1;
				__BANK(ae);	
				ok=1;
			} else {
				if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: WRITE DIRECT lower ROM ignored (value %d out of bounds 0-7)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,lower);
			}
		} else if (upper!=-1) {
			if (upper>=0 && ((ae->forcecpr && upper<32) || (ae->forcesnapshot && upper<36))) {
				ae->idx+=2;
				__BANK(ae);	
				ok=1;
			} else {
				if (!ae->forcecpr && !ae->forcesnapshot) {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: WRITE DIRECT select a ROM without cartridge output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				} else {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: WRITE DIRECT upper ROM ignored (value %d out of bounds 0-31)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,upper);
				}
			}
		} else if (bank!=-1) {
			/* selection de bank on ouvre un nouvel espace */
		} else {
			if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: meaningless WRITE DIRECT\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		}
	}
	while (!ae->wl[ae->idx].t) ae->idx++;
	if (!ok) {
		___new_memory_space(ae);
	}
}
void __CHARSET(struct s_assenv *ae) {
	int i,s,e,v,tquote;

	if (ae->wl[ae->idx].t==1) {
		/* reinit charset */
		for (i=0;i<256;i++)
			ae->charset[i]=i;
	} else if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		/* string,value | byte,value */
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,0);
		v=RoundComputeExpression(ae,ae->wl[ae->idx+2].w,ae->codeadr,0,0);
		if (ae->wl[ae->idx+1].w[0]=='\'' || ae->wl[ae->idx+1].w[0]=='"') {
			tquote=ae->wl[ae->idx+1].w[0];
			if (ae->wl[ae->idx+1].w[strlen(ae->wl[ae->idx+1].w)-1]==tquote) {
				i=1;
				while (ae->wl[ae->idx+1].w[i] && ae->wl[ae->idx+1].w[i]!=tquote) {
					if (ae->wl[ae->idx+1].w[i]=='\\') i++;
					ae->charset[(int)ae->wl[ae->idx+1].w[i]]=(unsigned char)v++;
					i++;
				}
			} else {
				rasm_printf(ae,"[%s:%d] CHARSET string,value has invalid quote!\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		} else {
			i=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
			if (i>=0 && i<256) {
				ae->charset[i]=(unsigned char)v;
			} else {
				rasm_printf(ae,"[%s:%d] CHARSET byte value must be 0-255\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		}
		ae->idx+=2;
	} else if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && !ae->wl[ae->idx+2].t && ae->wl[ae->idx+3].t==1) {
		/* start,end,value */
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,0);
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+3].w,0);
		s=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		e=RoundComputeExpression(ae,ae->wl[ae->idx+2].w,ae->codeadr,0,0);
		v=RoundComputeExpression(ae,ae->wl[ae->idx+3].w,ae->codeadr,0,0);
		ae->idx+=3;
		if (s<=e && s>=0 && e<256) {
			for (i=s;i<=e;i++) {
				ae->charset[i]=(unsigned char)v++;
			}
		} else {
			rasm_printf(ae,"[%s:%d] CHARSET winape directive wrong interval value\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] CHARSET winape directive wrong parameter count\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __MACRO(struct s_assenv *ae) {
	struct s_macro curmacro={0};
	char *referentfilename,*zeparam;
	int refidx,idx,getparam=1;
	struct s_wordlist curwl;
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t!=2) {
		/* get the name */
		curmacro.mnemo=ae->wl[ae->idx+1].w;
		curmacro.crc=GetCRC(curmacro.mnemo);
		if (ae->wl[ae->idx+1].t) {
			getparam=0;
		}
		/* overload forbidden */
		if (SearchMacro(ae,curmacro.crc,curmacro.mnemo)>=0) {
			rasm_printf(ae,"[%s:%d] Macro already defined with this name\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		idx=ae->idx+2;
		while (ae->wl[idx].t!=2 && (GetCRC(ae->wl[idx].w)!=CRC_MEND || strcmp(ae->wl[idx].w,"MEND")!=0) && (GetCRC(ae->wl[idx].w)!=CRC_ENDM || strcmp(ae->wl[idx].w,"ENDM")!=0)) {
			if (GetCRC(ae->wl[idx].w)==CRC_MACRO || strcmp(ae->wl[idx].w,"MACRO")==0) {
				/* inception interdite */
				referentfilename=GetCurrentFile(ae);
				refidx=ae->idx;
				ae->idx=idx;
				rasm_printf(ae,"[%s:%d] You cannot define a macro inside another one (MACRO %s in [%s] L%d\n",GetCurrentFile(ae),ae->wl[idx].l,ae->wl[refidx+1].w,referentfilename,ae->wl[refidx].l);
				FreeAssenv(ae);
				exit(-52);
			}
			if (getparam) {
				/* on prepare les parametres au remplacement */
				zeparam=MemMalloc(strlen(ae->wl[idx].w)+3);
				if (ae->as80) {
					sprintf(zeparam,"%s",ae->wl[idx].w);
				} else {
					sprintf(zeparam,"{%s}",ae->wl[idx].w);
				}
				curmacro.nbparam++;
				curmacro.param=MemRealloc(curmacro.param,curmacro.nbparam*sizeof(char **));
				curmacro.param[curmacro.nbparam-1]=zeparam;
				if (ae->wl[idx].t) {
					/* duplicate parameters without brackets MUST be an OPTION */
					getparam=0;
				}
			} else {
				/* copie la liste de mots */	
				curwl=ae->wl[idx];
				ObjectArrayAddDynamicValueConcat((void **)&curmacro.wc,&curmacro.nbword,&curmacro.maxword,&curwl,sizeof(struct s_wordlist));
			}
			idx++;
		}
		if (ae->wl[idx].t==2) {
			rasm_printf(ae,"[%s:%d] Macro was not closed\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		ObjectArrayAddDynamicValueConcat((void**)&ae->macro,&ae->imacro,&ae->mmacro,&curmacro,sizeof(curmacro));
		/* le quicksort n'est pas optimal mais on n'est pas suppos� en cr�er des milliers */
		qsort(ae->macro,ae->imacro,sizeof(struct s_macro),cmpmacros);

		/* ajustement des mots lus */
		if (ae->wl[idx].t==2) idx--;
		ae->idx=idx;
	} else {
		rasm_printf(ae,"[%s:%d] MACRO definition need at least one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

struct s_wordlist *__MACRO_EXECUTE(struct s_assenv *ae, int imacro) {
	struct s_wordlist *cpybackup;
	int nbparam=0,idx,i,j,idad;
	int ifile,iline,iu,lenparam;
	double v;
	struct s_macro_position curmacropos={0};
	char *zeparam=NULL,*txtparamlist;
	int reload=0;
	
	idx=ae->idx;
	while (!ae->wl[idx].t) {
		nbparam++;
		idx++;
	}
	/* hack to secure macro without parameters with void argument */
	if (!ae->macro[imacro].nbparam && nbparam) {
		if (nbparam==1 && strcmp(ae->wl[ae->idx+1].w,"(VOID)")==0) {
			nbparam=0;
			reload=1;
		}
	}

	if (ae->macro[imacro].nbparam && nbparam!=ae->macro[imacro].nbparam) {
		for (i=lenparam=0;i<ae->macro[imacro].nbparam;i++) {
			lenparam+=strlen(ae->macro[imacro].param[i])+3;
		}
		txtparamlist=MemMalloc(lenparam);
		txtparamlist[0]=0;
		for (i=0;i<ae->macro[imacro].nbparam;i++) {
			strcat(txtparamlist,ae->macro[imacro].param[i]);
			strcat(txtparamlist," ");
		}

		rasm_printf(ae,"[%s:%d] MACRO [%s] was defined with %d parameter%s %s\n",GetCurrentFile(ae),ae->wl[idx].l,ae->macro[imacro].mnemo,ae->macro[imacro].nbparam,ae->macro[imacro].nbparam>1?"s":"",txtparamlist);
		MaxError(ae);
		while (!ae->wl[ae->idx].t) {
			ae->idx++;
		}
		ae->idx++;
	} else {
		/* free macro call as we will overwrite it */
		MemFree(ae->wl[ae->idx].w);
		/* is there a void to free? */
		if (reload) {
			MemFree(ae->wl[ae->idx+1].w);
		}
		/* eval parameters? */
		for (i=0;i<nbparam;i++) {
			if (strncmp(ae->wl[ae->idx+1+i].w,"{EVAL}",6)==0) {
				/* parametre entre chevrons, il faut l'interpreter d'abord */
				zeparam=TxtStrDup(ae->wl[ae->idx+1+i].w+6);
				ExpressionFastTranslate(ae,&zeparam,1);
				v=ComputeExpressionCore(ae,zeparam,ae->codeadr,0);
				MemFree(zeparam);
				zeparam=MemMalloc(32);
				snprintf(zeparam,31,"%lf",v);
				zeparam[31]=0;
				MemFree(ae->wl[ae->idx+1+i].w);
				ae->wl[ae->idx+1+i].w=zeparam;
			}
		}
		/* backup parameters */
		cpybackup=MemMalloc((nbparam+1)*sizeof(struct s_wordlist));
		for (i=0;i<nbparam;i++) {
			cpybackup[i]=ae->wl[ae->idx+1+i];
		}
		/* insert macro position */
		curmacropos.start=ae->idx;
		curmacropos.end=ae->idx+ae->macro[imacro].nbword;
		curmacropos.value=ae->macrocounter;
		ObjectArrayAddDynamicValueConcat((void**)&ae->macropos,&ae->imacropos,&ae->mmacropos,&curmacropos,sizeof(curmacropos));
		
		/* are we in a repeat/while block? */
		for (iu=0;iu<ae->ir;iu++) if (ae->repeat[iu].maxim<ae->imacropos) ae->repeat[iu].maxim=ae->imacropos;
		for (iu=0;iu<ae->iw;iu++) if (ae->whilewend[iu].maxim<ae->imacropos) ae->whilewend[iu].maxim=ae->imacropos;
		
		/* update daddy macropos */
		for (idad=0;idad<ae->imacropos-1;idad++) {
			if (ae->macropos[idad].end>curmacropos.start) {
				ae->macropos[idad].end+=ae->macro[imacro].nbword-1-nbparam-reload; /* coz la macro compte un mot! */
			}
		}
		
#if 0
		for (idad=0;idad<ae->imacropos;idad++) {
			printf("macropos[%d]=%d -> %d\n",idad,ae->macropos[idad].start,ae->macropos[idad].end);
		}
#endif		
		/* insert at macro position and replace macro+parameters */
		if (ae->macro[imacro].nbword>1+nbparam+reload) {
			ae->nbword+=ae->macro[imacro].nbword-1-nbparam-reload;
			ae->wl=MemRealloc(ae->wl,ae->nbword*sizeof(struct s_wordlist));
		} else {
			/* si on r�duit pas de realloc pour ne pas perdre de donnees */
			ae->nbword+=ae->macro[imacro].nbword-1-nbparam-reload;
		}
		iline=ae->wl[ae->idx].l;
		ifile=ae->wl[ae->idx].ifile;
		MemMove(&ae->wl[ae->idx+ae->macro[imacro].nbword],&ae->wl[ae->idx+reload+nbparam+1],(ae->nbword-ae->idx-ae->macro[imacro].nbword)*sizeof(struct s_wordlist));

		for (i=0;i<ae->macro[imacro].nbword;i++) {
			ae->wl[i+ae->idx].w=TxtStrDup(ae->macro[imacro].wc[i].w);
			ae->wl[i+ae->idx].l=iline;
			ae->wl[i+ae->idx].ifile=ifile;
			/* @@@sujet a evolution, ou double controle */
			ae->wl[i+ae->idx].t=ae->macro[imacro].wc[i].t;
			ae->wl[i+ae->idx].e=ae->macro[imacro].wc[i].e;
		}
		/* replace */
		idx=ae->idx;
		for (i=0;i<nbparam;i++) {
			for (j=idx;j<idx+ae->macro[imacro].nbword;j++) {
				/* tags in upper case for replacement in quotes */
				if (StringIsQuote(ae->wl[j].w)) {
					int lm,touched;
					for (lm=touched=0;ae->wl[j].w[lm];lm++) {
						if (ae->wl[j].w[lm]=='{') touched++; else if (ae->wl[j].w[lm]=='}') touched--; else if (touched) ae->wl[j].w[lm]=toupper(ae->wl[j].w[lm]);
					}
				}
				ae->wl[j].w=TxtReplace(ae->wl[j].w,ae->macro[imacro].param[i],cpybackup[i].w,0);
			}
			MemFree(cpybackup[i].w);
		}
		MemFree(cpybackup);
		/* macro replaced, need to rollback index */
		//ae->idx--;
	}
	/* a chaque appel de macro on incremente le compteur pour les labels locaux */
	ae->macrocounter++;

	return ae->wl;
}

/*
	ticker start, <var>
	ticker stop, <var>
*/
void __TICKER(struct s_assenv *ae) {
	struct s_expr_dico *tvar;
	struct s_ticker ticker;
	int crc,i;

	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		crc=GetCRC(ae->wl[ae->idx+2].w);

		if (strcmp(ae->wl[ae->idx+1].w,"START")==0) {
			/* is there already a counter?  */
			for (i=0;i<ae->iticker;i++) {
				if (ae->ticker[i].crc==crc && strcmp(ae->wl[ae->idx+2].w,ae->ticker[i].varname)==0) {
					break;
				}
			}
			if (i==ae->iticker) {
				ticker.varname=TxtStrDup(ae->wl[ae->idx+2].w);
				ticker.crc=crc;
				ObjectArrayAddDynamicValueConcat((void **)&ae->ticker,&ae->iticker,&ae->mticker,&ticker,sizeof(struct s_ticker));
			}
			ae->ticker[i].nopstart=ae->nop;
		} else if (strcmp(ae->wl[ae->idx+1].w,"STOP")==0) {
			for (i=0;i<ae->iticker;i++) {
				if (ae->ticker[i].crc==crc && strcmp(ae->wl[ae->idx+2].w,ae->ticker[i].varname)==0) {
					break;
				}
			}
			if (i<ae->iticker) {
				/* set var */
				if ((tvar=SearchDico(ae,ae->wl[ae->idx+2].w,crc))!=NULL) {
					/* compute nop count */
					tvar->v=ae->nop-ae->ticker[i].nopstart;
				} else {
					/* create var with nop count */
					ExpressionSetDicoVar(ae,ae->wl[ae->idx+2].w,ae->nop-ae->ticker[i].nopstart);
				}
			} else {
				rasm_printf(ae,"[%s:%d] TICKER not found\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		} else {
			rasm_printf(ae,"[%s:%d] usage is TICKER start/stop,<variable>\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		ae->idx+=2;
	} else {
		rasm_printf(ae,"[%s:%d] usage is TICKER start/stop,<variable>\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __LET(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ae->idx++;
		ExpressionFastTranslate(ae,&ae->wl[ae->idx].w,0);
		RoundComputeExpression(ae,ae->wl[ae->idx].w,ae->codeadr,0,0);
	} else {
		rasm_printf(ae,"[%s:%d] LET useless winape directive need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __RUN(struct s_assenv *ae) {
	int mypc=0;
	int ramconf=0xC0;
	
	if (!ae->wl[ae->idx].t) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		mypc=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ae->idx++;
		if (mypc<0 || mypc>65535) {
			if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: run adress truncated from %X to %X\n",GetCurrentFile(ae),ae->wl[ae->idx].l,mypc,mypc&0xFFFF);
		}
		if (!ae->wl[ae->idx].t) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			ramconf=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
			ae->idx++;
			if (ramconf<0xC0 || ramconf>0xFF) {
				if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: ram configuration out of bound %X forced to #C0\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ramconf);
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] RUN winape directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
	ae->snapshot.registers.LPC=mypc&0xFF;
	ae->snapshot.registers.HPC=(mypc>>8)&0xFF;
	ae->snapshot.ramconfiguration=ramconf;
	if (ae->rundefined) rasm_printf(ae,"[%s:%d] Warning: run adress redefinition\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
	ae->rundefined=1;
}
void __BREAKPOINT(struct s_assenv *ae) {
	struct s_breakpoint breakpoint={0};
	
	if (ae->activebank>3) breakpoint.bank=1;
	if (ae->wl[ae->idx].t) {
		breakpoint.address=ae->codeadr;
		ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));
	} else 	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		breakpoint.address=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));
	} else {
		rasm_printf(ae,"[%s:%d] syntax is BREAKPOINT [adress]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __SETCPC(struct s_assenv *ae) {
	int mycpc;

	if (!ae->forcecpr) {
		ae->forcesnapshot=1;
	} else {
		if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: Cannot SETCPC when already in cartridge output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
	}

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		mycpc=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ae->idx++;
		switch (mycpc) {
			case 0:
			case 1:
			case 2:
			case 4:
			case 5:
			case 6:
				ae->snapshot.CPCType=mycpc;
				break;
			default:
				rasm_printf(ae,"[%s:%d] SETCPC directive has wrong value (0,1,2,4,5,6 only)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] SETCPC directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __SETCRTC(struct s_assenv *ae) {
	int mycrtc;

	if (!ae->forcecpr) {
		ae->forcesnapshot=1;
	} else {
		if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: Cannot SETCRTC when already in cartridge output\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
	}

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		mycrtc=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] SETCRTC directive need one integer parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
	switch (mycrtc) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			ae->snapshot.crtcstate.model=mycrtc;
			break;
		default:
			rasm_printf(ae,"[%s:%d] SETCRTC directive has wrong value (0,1,2,3,4 only)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
	}
}


void __LIST(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] LIST winape directive do not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __NOLIST(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] NOLIST winape directive do not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __BRK(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] BRK winape directive do not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __STOP(struct s_assenv *ae) {
	rasm_printf(ae,"[%s:%d] STOP assembling requested\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
	while (ae->wl[ae->idx].t!=2) ae->idx++;
	ae->idx--;
	ae->stop=1;
}

void __PRINT(struct s_assenv *ae) {
	while (ae->wl[ae->idx].t!=1) {
		if (!StringIsQuote(ae->wl[ae->idx+1].w)) {
			char *string2print=NULL;
			int hex=0,bin=0,entier=0;
			
			if (strncmp(ae->wl[ae->idx+1].w,"{HEX}",5)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+5);
				hex=1;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{HEX2}",6)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+6);
				hex=2;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{HEX4}",6)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+6);
				hex=4;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{HEX8}",6)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+6);
				hex=8;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{BIN}",5)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+5);
				bin=1;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{BIN8}",6)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+6);
				bin=8;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{BIN16}",7)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+7);
				bin=16;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{BIN32}",7)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+7);
				bin=32;
			} else if (strncmp(ae->wl[ae->idx+1].w,"{INT}",5)==0) {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w+5);
				entier=1;
			} else {
				string2print=TxtStrDup(ae->wl[ae->idx+1].w);
			}

			ExpressionFastTranslate(ae,&string2print,1);
			if (hex) {
				int zv;
				zv=RoundComputeExpressionCore(ae,string2print,ae->codeadr,0);
				switch (hex) {
					case 1:
						if (zv&0xFFFFFF00) {
							if (zv&0xFFFF0000) {
								rasm_printf(ae,"#%-8.08X ",zv);
							} else {
								rasm_printf(ae,"#%-4.04X ",zv);
							}
						} else {
							rasm_printf(ae,"#%-2.02X ",zv);
						}
						break;
					case 2:rasm_printf(ae,"#%-2.02X ",zv);break;
					case 4:rasm_printf(ae,"#%-4.04X ",zv);break;
					case 8:rasm_printf(ae,"#%-8.08X ",zv);break;
				}
			} else if (bin) {
				int zv,d;
				zv=RoundComputeExpressionCore(ae,string2print,ae->codeadr,0);
				/* remove useless sign bits */
				if (bin<32 && (zv&0xFFFF0000)==0xFFFF0000) {
					zv&=0xFFFF;
				}
				switch (bin) {
					case 1:if (zv&0xFF00) d=15; else d=7;break;
					case 8:d=7;break;
					case 16:d=15;break;
					case 32:d=31;break;
				}
				rasm_printf(ae,"%%");
				for (;d>=0;d--) {
					if ((zv>>d)&1) rasm_printf(ae,"1"); else rasm_printf(ae,"0");
				}
				rasm_printf(ae," ");
			} else if (entier) {
				rasm_printf(ae,"%d ",(int)RoundComputeExpressionCore(ae,string2print,ae->codeadr,0));
			} else {
				rasm_printf(ae,"%.2lf ",ComputeExpressionCore(ae,string2print,ae->codeadr,0));
			}
			MemFree(string2print);
		} else {
			char *varbuffer;
			int lm,touched;
			lm=strlen(ae->wl[ae->idx+1].w)-2;
			if (lm) {
				varbuffer=MemMalloc(lm+2);
				sprintf(varbuffer,"%-*.*s ",lm,lm,ae->wl[ae->idx+1].w+1);
				/* need to upper case tags */
				for (lm=touched=0;varbuffer[lm];lm++) {
					if (varbuffer[lm]=='{') touched++; else if (varbuffer[lm]=='}') touched--; else if (touched) varbuffer[lm]=toupper(varbuffer[lm]);
				}
				/* translate tag will check tag consistency */
				varbuffer=TranslateTag(ae,varbuffer,&touched,1,E_TAGOPTION_REMOVESPACE);
				varbuffer=TxtReplace(varbuffer,"\\b","\b",0);
				varbuffer=TxtReplace(varbuffer,"\\v","\v",0);
				varbuffer=TxtReplace(varbuffer,"\\f","\f",0);
				varbuffer=TxtReplace(varbuffer,"\\r","\r",0);
				varbuffer=TxtReplace(varbuffer,"\\n","\n",0);
				varbuffer=TxtReplace(varbuffer,"\\t","\t",0);
				rasm_printf(ae,"%s ",varbuffer);
				MemFree(varbuffer);
			}
		}
		ae->idx++;
	}
	rasm_printf(ae,"\n");
}

void __FAIL(struct s_assenv *ae) {
	__PRINT(ae);
	__STOP(ae);
	MaxError(ae);
}

void __ALIGN(struct s_assenv *ae) {
	int aval,ifill=-1;
	
	if (ae->io) {
		ae->orgzone[ae->io-1].memend=ae->outputadr;
	}
	if (!ae->wl[ae->idx].t) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		aval=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
		ae->idx++;
		/* align with fill ? */
		if (!ae->wl[ae->idx].t) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			ifill=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,0);
			ae->idx++;
			if (ifill<0 || ifill>255) {
				rasm_printf(ae,"[%s:%d] ALIGN fill value must be 0 to 255\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
				ifill=0;
			}
		}

		if (aval<1 || aval>65535) {
			rasm_printf(ae,"[%s:%d] ALIGN boundary must be greater than zero and lower than 65536\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
			aval=1;
		}

		/* touch codeadr only if adress is misaligned */
		if (ae->codeadr%aval) {
			if (ifill==-1) {
				/* virtual ALIGN is moving outputadr the same value as codeadr move */
				ae->outputadr=ae->outputadr-(ae->codeadr%aval)+aval;
				ae->codeadr=ae->codeadr-(ae->codeadr%aval)+aval;
			} else {
				/* physical ALIGN fill bytes */
				while (ae->codeadr%aval) {
					___output(ae,ifill);
					ae->nop+=1;
				}
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] ALIGN <boundary>[,fill] directive need one or two integers parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void ___internal_skip_loop_block(struct s_assenv *ae, int eloopstyle) {
	int *loopstyle;
	int iloop,mloop;
	int cidx;

	cidx=ae->idx+2;
	iloop=mloop=0;
	loopstyle=NULL;
	IntArrayAddDynamicValueConcat(&loopstyle,&iloop,&mloop,eloopstyle);
	/* look for WEND */
	while (iloop) {
		if (strcmp(ae->wl[cidx].w,"REPEAT")==0) {
			if (ae->wl[cidx].t) {
				IntArrayAddDynamicValueConcat(&loopstyle,&iloop,&mloop,E_LOOPSTYLE_REPEATUNTIL);
			} else if (ae->wl[cidx+1].t) {
				IntArrayAddDynamicValueConcat(&loopstyle,&iloop,&mloop,E_LOOPSTYLE_REPEATN);
			} else {
				rasm_printf(ae,"[%s:%d] FATAL - Invalid REPEAT syntax\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				FreeAssenv(ae);
				exit(7);
			}
		} else if (strcmp(ae->wl[cidx].w,"WHILE")==0) {
			if (!ae->wl[cidx].t && ae->wl[cidx+1].t) {
				IntArrayAddDynamicValueConcat(&loopstyle,&iloop,&mloop,E_LOOPSTYLE_WHILE);
			} else {
				rasm_printf(ae,"[%s:%d] FATAL - Invalid WHILE syntax\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				FreeAssenv(ae);
				exit(7);
			}
		} else if (strcmp(ae->wl[cidx].w,"WEND")==0) {
			iloop--;
			if (loopstyle[iloop]!=E_LOOPSTYLE_WHILE) {
				rasm_printf(ae,"[%s:%d] FATAL - WEND encountered but expecting %s\n",GetCurrentFile(ae),ae->wl[ae->idx].l,loopstyle[iloop]==E_LOOPSTYLE_REPEATN?"REND":"UNTIL");
				FreeAssenv(ae);
				exit(7);
			}
		} else if (strcmp(ae->wl[cidx].w,"REND")==0) {
			iloop--;
			if (loopstyle[iloop]!=E_LOOPSTYLE_REPEATN) {
				rasm_printf(ae,"[%s:%d] FATAL - REND encountered but expecting %s\n",GetCurrentFile(ae),ae->wl[ae->idx].l,loopstyle[iloop]==E_LOOPSTYLE_REPEATUNTIL?"UNTIL":"WEND");
				FreeAssenv(ae);
				exit(7);
			}
		} else if (strcmp(ae->wl[cidx].w,"UNTIL")==0) {
			iloop--;
			if (loopstyle[iloop]!=E_LOOPSTYLE_REPEATUNTIL) {
				rasm_printf(ae,"[%s:%d] FATAL - UNTIL encountered but expecting %s\n",GetCurrentFile(ae),ae->wl[ae->idx].l,loopstyle[iloop]==E_LOOPSTYLE_REPEATN?"REND":"WEND");
				FreeAssenv(ae);
				exit(7);
			}
		}
		while (!ae->wl[cidx].t) cidx++;
		cidx++;
	}
	MemFree(loopstyle);
	ae->idx=cidx-1;
}

void __WHILE(struct s_assenv *ae) {
	struct s_whilewend whilewend={0};
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		if (!ComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,2)) {
				/* skip while block */
				___internal_skip_loop_block(ae,E_LOOPSTYLE_WHILE);
				return;
		} else {
			ae->idx++;
			whilewend.start=ae->idx;
			whilewend.cpt=0;
			whilewend.value=ae->whilecounter;
			whilewend.maxim=ae->imacropos;
			whilewend.while_counter=1;
			ae->whilecounter++;
			/* pour g�rer les macros situ�s dans le while pr�cedent apr�s un repeat/while courant */
			if (ae->iw) whilewend.maxim=ae->whilewend[ae->iw-1].maxim;
			if (ae->ir && ae->repeat[ae->ir-1].maxim>whilewend.maxim) whilewend.maxim=ae->repeat[ae->ir-1].maxim;
			ObjectArrayAddDynamicValueConcat((void**)&ae->whilewend,&ae->iw,&ae->mw,&whilewend,sizeof(whilewend));
		}
	} else {
		rasm_printf(ae,"[%s:%d] syntax is WHILE <expression>\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __WEND(struct s_assenv *ae) {
	if (ae->iw>0) {
		if (ae->wl[ae->idx].t==1) {
			//ExpressionFastTranslate(ae,&ae->wl[ae->whilewend[ae->iw-1].start].w,0); //TOTEST ���
			if (ComputeExpression(ae,ae->wl[ae->whilewend[ae->iw-1].start].w,ae->codeadr,0,2)) {
				if (ae->whilewend[ae->iw-1].while_counter>65536) {
					rasm_printf(ae,"[%s:%d] Bypass infinite WHILE loop\n",GetExpFile(ae,0),GetExpLine(ae,0));
					MaxError(ae);
					ae->iw--;
					/* refresh macro check index */
					if (ae->iw) ae->imacropos=ae->whilewend[ae->iw-1].maxim;
				} else {
					ae->whilewend[ae->iw-1].cpt++; /* for local label */
					ae->whilewend[ae->iw-1].while_counter++;
					ae->idx=ae->whilewend[ae->iw-1].start;
					/* refresh macro check index */
					ae->imacropos=ae->whilewend[ae->iw-1].maxim;
				}
			} else {
				ae->iw--;
				/* refresh macro check index */
				if (ae->iw) ae->imacropos=ae->whilewend[ae->iw-1].maxim;
			}
		} else {
			rasm_printf(ae,"[%s:%d] WEND does not need any parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] WEND encounter whereas there is no referent WHILE\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __REPEAT(struct s_assenv *ae) {
	struct s_repeat currepeat={0};
	struct s_expr_dico *rvar;
	int *loopstyle;
	int iloop,mloop;
	int cidx,crc;
	
	if (ae->wl[ae->idx+1].t!=2) {
		if (ae->wl[ae->idx].t==0) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			currepeat.cpt=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,0,0,0);
			if (!currepeat.cpt) {
				/* skip repeat block */
				___internal_skip_loop_block(ae,E_LOOPSTYLE_REPEATN);
				return;
			} else if (currepeat.cpt<1 || currepeat.cpt>65536) {
				rasm_printf(ae,"[%s:%d] Repeat value (%d) must be from 1 to 65535\n",GetCurrentFile(ae),ae->wl[ae->idx].l,currepeat.cpt);
				FreeAssenv(ae);
				exit(2);
			}
			ae->idx++;
			currepeat.start=ae->idx;
			if (ae->wl[ae->idx].t==0) {
				ae->idx++;
				if (ae->wl[ae->idx].t==1) {
					/* la variable peut exister -> OK */
					crc=GetCRC(ae->wl[ae->idx].w);
					if ((rvar=SearchDico(ae,ae->wl[ae->idx].w,crc))!=NULL) {
						rvar->v=1;
					} else {
						/* mais ne peut �tre un label ou un alias */
						ExpressionSetDicoVar(ae,ae->wl[ae->idx].w, 1);
					}
					currepeat.repeatvar=ae->wl[ae->idx].w;
					currepeat.repeatcrc=crc;
				} else {
					rasm_printf(ae,"[%s:%d] extended syntax is REPEAT <n>,<var>\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				}
			}
		} else {
			currepeat.start=ae->idx;
			currepeat.cpt=-1;
		}
		currepeat.value=ae->repeatcounter;
		currepeat.repeat_counter=1;
		ae->repeatcounter++;
		/* pour g�rer les macros situ�s dans le repeat pr�cedent apr�s le repeat courant */
		if (ae->ir) currepeat.maxim=ae->repeat[ae->ir-1].maxim;
		if (ae->iw && ae->whilewend[ae->iw-1].maxim>currepeat.maxim) currepeat.maxim=ae->whilewend[ae->iw-1].maxim;
		if (ae->imacropos>currepeat.maxim) currepeat.maxim=ae->imacropos;
		ObjectArrayAddDynamicValueConcat((void**)&ae->repeat,&ae->ir,&ae->mr,&currepeat,sizeof(currepeat));
	} else {
		rasm_printf(ae,"[%s:%d] wrong REPEAT usage\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __REND(struct s_assenv *ae) {
	struct s_expr_dico *rvar;
	if (ae->ir>0) {
		if (ae->repeat[ae->ir-1].cpt==-1) {
			rasm_printf(ae,"[%s:%d] REND encounter whereas referent REPEAT was waiting for UNTIL\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		} else {
			ae->repeat[ae->ir-1].cpt--;
			ae->repeat[ae->ir-1].repeat_counter++;
			if ((rvar=SearchDico(ae,ae->repeat[ae->ir-1].repeatvar,ae->repeat[ae->ir-1].repeatcrc))!=NULL) {
				rvar->v=ae->repeat[ae->ir-1].repeat_counter;
			}
			if (ae->repeat[ae->ir-1].cpt) {
				ae->idx=ae->repeat[ae->ir-1].start;
				/* refresh macro check index */
				ae->imacropos=ae->repeat[ae->ir-1].maxim;
			} else {
				ae->ir--;
				/* refresh macro check index */
				if (ae->ir) ae->imacropos=ae->repeat[ae->ir-1].maxim;
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] REND encounter whereas there is no referent REPEAT\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __UNTIL(struct s_assenv *ae) {
	if (ae->ir>0) {
		if (ae->repeat[ae->ir-1].cpt>=0) {
			rasm_printf(ae,"[%s:%d] UNTIL encounter whereas referent REPEAT n was waiting for REND\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		} else {
			if (ae->wl[ae->idx].t==0 && ae->wl[ae->idx+1].t==1) {
				ae->repeat[ae->ir-1].repeat_counter++;
				ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
				if (!ComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,2)) {
					if (ae->repeat[ae->ir-1].repeat_counter>65536) {
						rasm_printf(ae,"[%s:%d] Bypass infinite REPEAT loop\n",GetExpFile(ae,0),GetExpLine(ae,0));
						MaxError(ae);
						ae->ir--;
						/* refresh macro check index */
						if (ae->ir) ae->imacropos=ae->repeat[ae->ir-1].maxim;
					} else {
						ae->idx=ae->repeat[ae->ir-1].start;
						ae->repeat[ae->ir-1].cpt--; /* for local label */
						/* refresh macro check index */
						ae->imacropos=ae->repeat[ae->ir-1].maxim;
					}
				} else {
					ae->ir--;
					/* refresh macro check index */
					if (ae->ir) ae->imacropos=ae->repeat[ae->ir-1].maxim;
				}
			} else {
				rasm_printf(ae,"[%s:%d] UNTIL need one expression/evaluation as parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] UNTIL encounter whereas there is no referent REPEAT\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __ASSERT(struct s_assenv *ae) {
	char Dot3[4];
	int rexpr;

	if (!ae->wl[ae->idx].t) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		if (strlen(ae->wl[ae->idx+1].w)>29) strcpy(Dot3,"..."); else strcpy(Dot3,"");
		rexpr=!!RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
		if (!rexpr) {
			rasm_printf(ae,"[%s:%d] ASSERT %.29s%s failed with ",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->wl[ae->idx+1].w,Dot3);
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,1);
			rasm_printf(ae,"%s\n",ae->wl[ae->idx+1].w);
			MaxError(ae);
 			if (!ae->wl[ae->idx+1].t) {
				ae->idx++;
				rasm_printf(ae,"-> ");
				__PRINT(ae);
			}
			__STOP(ae);
		} else {
			while (!ae->wl[ae->idx].t) ae->idx++;
		}
	} else {
		rasm_printf(ae,"[%s:%d] ASSERT need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __IF(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	int rexpr;

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		rexpr=!!RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
		ifthen.v=rexpr;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] IF need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}

void __IF_light(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	int rexpr;

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		/* do not need to compute the value in shadow execution */
		ifthen.v=0;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] IF need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}

/* test if a label or a variable where used before */
void __IFUSED(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	int rexpr,crc;
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		crc=GetCRC(ae->wl[ae->idx+1].w);
		if ((SearchDico(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
			rexpr=1;
		} else {
			if ((SearchLabel(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
				rexpr=1;
			} else {
				if ((SearchAlias(ae,crc,ae->wl[ae->idx+1].w))!=-1) {
					rexpr=1;
				} else {
					rexpr=SearchUsed(ae,ae->wl[ae->idx+1].w,crc);
				}
			}
		}
		ifthen.v=rexpr;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFUSED;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFUSED need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFNUSED(struct s_assenv *ae) {
	__IFUSED(ae);
	ae->ifthen[ae->ii-1].v=1-ae->ifthen[ae->ii-1].v;
	ae->ifthen[ae->ii-1].type=E_IFTHEN_TYPE_IFNUSED;
}
void __IFUSED_light(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ifthen.v=0;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFUSED;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFUSED need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFNUSED_light(struct s_assenv *ae) {
	__IFUSED_light(ae);
	ae->ifthen[ae->ii-1].type=E_IFTHEN_TYPE_IFNUSED;
}

/* test if a label or a variable exists */
void __IFDEF(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	int rexpr,crc;
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		crc=GetCRC(ae->wl[ae->idx+1].w);
		if ((SearchDico(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
			rexpr=1;
		} else {
			if ((SearchLabel(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
				rexpr=1;
			} else {
				if ((SearchAlias(ae,crc,ae->wl[ae->idx+1].w))!=-1) {
					rexpr=1;
				} else {
					rexpr=0;
				}
			}
		}
		ifthen.v=rexpr;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFDEF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFDEF need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFDEF_light(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ifthen.v=0;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFDEF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFDEF need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFNDEF(struct s_assenv *ae) {
	struct s_expr_dico *curdic=NULL;
	struct s_label *curlabel=NULL;
	struct s_ifthen ifthen={0};
	int rexpr,crc;
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		crc=GetCRC(ae->wl[ae->idx+1].w);
		if ((SearchDico(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
			rexpr=0;
		} else {
			if ((SearchLabel(ae,ae->wl[ae->idx+1].w,crc))!=NULL) {
				rexpr=0;
			} else {
				if ((SearchAlias(ae,crc,ae->wl[ae->idx+1].w))!=-1) {
					rexpr=0;
				} else {
					rexpr=1;
				}
			}
		}
		ifthen.v=rexpr;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFNDEF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFNDEF need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFNDEF_light(struct s_assenv *ae) {
	struct s_expr_dico *curdic=NULL;
	struct s_label *curlabel=NULL;
	struct s_ifthen ifthen={0};
	
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ifthen.v=0;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFNDEF;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] FATAL - IFNDEF need one variable or label\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}

void __UNDEF(struct s_assenv *ae) {

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		if (!DelDico(ae,ae->wl[ae->idx+1].w,GetCRC(ae->wl[ae->idx+1].w))) {
			//rasm_printf(ae,"[%s:%d] variable to UNDEF not found!\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			//MaxError(ae);
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] syntax is UNDEF <variable>\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}

}


void __SWITCH(struct s_assenv *ae) {
	struct s_switchcase curswitch={0};

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		/* switch store the value */
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		curswitch.refval=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
		ObjectArrayAddDynamicValueConcat((void**)&ae->switchcase,&ae->isw,&ae->msw,&curswitch,sizeof(curswitch));
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] SWITCH need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __CASE(struct s_assenv *ae) {
	int rexpr;
	
	if (ae->isw) {
		if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			rexpr=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
			
			if (ae->switchcase[ae->isw-1].refval==rexpr) {
				ae->switchcase[ae->isw-1].execute=1;
				ae->switchcase[ae->isw-1].casematch=1;
			}
		} else {
			rasm_printf(ae,"[%s:%d] CASE not need one parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] CASE encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __DEFAULT(struct s_assenv *ae) {
	
	if (ae->isw) {
		if (ae->wl[ae->idx].t==1) {
			/* aucun match avant, on active, sinon on laisse tel quel */
			if (!ae->switchcase[ae->isw-1].casematch) {
				ae->switchcase[ae->isw-1].execute=1;
			}
		} else {
			rasm_printf(ae,"[%s:%d] DEFAULT do not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] DEFAULT encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __BREAK(struct s_assenv *ae) {
	
	if (ae->isw) {
		if (ae->wl[ae->idx].t==1) {
			ae->switchcase[ae->isw-1].execute=0;
		} else {
			rasm_printf(ae,"[%s:%d] BREAK do not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] BREAK encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __SWITCH_light(struct s_assenv *ae) {
	struct s_switchcase curswitch={0};

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		/* shadow execution */
		curswitch.refval=0;
		curswitch.execute=0;
		ObjectArrayAddDynamicValueConcat((void**)&ae->switchcase,&ae->isw,&ae->msw,&curswitch,sizeof(curswitch));
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] SWITCH need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __CASE_light(struct s_assenv *ae) {
	int rexpr;
	
	if (ae->isw) {
		/* shadowed execution */
	} else {
		rasm_printf(ae,"[%s:%d] CASE encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __DEFAULT_light(struct s_assenv *ae) {
	
	if (ae->isw) {
		/* shadowed execution */
	} else {
		rasm_printf(ae,"[%s:%d] DEFAULT encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __BREAK_light(struct s_assenv *ae) {
	if (ae->isw) {
		/* shadowed execution */
	} else {
		rasm_printf(ae,"[%s:%d] BREAK encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __ENDSWITCH(struct s_assenv *ae) {
	if (ae->isw) {
		if (ae->wl[ae->idx].t==1) {
			ae->isw--;
		} else {
			rasm_printf(ae,"[%s:%d] ENDSWITCH does not need any parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] ENDSWITCH encounter whereas there is no referent SWITCH\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __IFNOT(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};
	int rexpr;

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		rexpr=!RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
		ifthen.v=rexpr;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFNOT;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] IFNOT need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __IFNOT_light(struct s_assenv *ae) {
	struct s_ifthen ifthen={0};

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ifthen.v=0;
		ifthen.filename=GetCurrentFile(ae);
		ifthen.line=ae->wl[ae->idx].l;
		ifthen.type=E_IFTHEN_TYPE_IFNOT;
		ObjectArrayAddDynamicValueConcat((void **)&ae->ifthen,&ae->ii,&ae->mi,&ifthen,sizeof(ifthen));
		//IntArrayAddDynamicValueConcat(&ae->ifthen,&ae->ii,&ae->mi,rexpr);
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] IFNOT need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}

void __ELSE(struct s_assenv *ae) {
	if (ae->ii) {
		if (ae->wl[ae->idx].t==1) {
			/* ELSE a executer seulement si celui d'avant est a zero */
			switch (ae->ifthen[ae->ii-1].v) {
				case -1:break;
				case 0:ae->ifthen[ae->ii-1].v=1;break;
				case 1:ae->ifthen[ae->ii-1].v=0;break;
			}
			ae->ifthen[ae->ii-1].type=E_IFTHEN_TYPE_ELSE;
			ae->ifthen[ae->ii-1].line=ae->wl[ae->idx].l;
			ae->ifthen[ae->ii-1].filename=GetCurrentFile(ae);
		} else {
			rasm_printf(ae,"[%s:%d] ELSE does not need any parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] ELSE encounter whereas there is no referent IF\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __ELSEIF(struct s_assenv *ae) {

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ae->ifthen[ae->ii-1].type=E_IFTHEN_TYPE_ELSEIF;
		ae->ifthen[ae->ii-1].line=ae->wl[ae->idx].l;
		ae->ifthen[ae->ii-1].filename=GetCurrentFile(ae);
		if (ae->ifthen[ae->ii-1].v) {
			/* il faut signifier aux suivants qu'on va jusqu'au ENDIF */
			ae->ifthen[ae->ii-1].v=-1;
		} else {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
			ae->ifthen[ae->ii-1].v=!!RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->codeadr,0,1);
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] ELSEIF need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __ELSEIF_light(struct s_assenv *ae) {

	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		ae->ifthen[ae->ii-1].type=E_IFTHEN_TYPE_ELSEIF;
		ae->ifthen[ae->ii-1].line=ae->wl[ae->idx].l;
		ae->ifthen[ae->ii-1].filename=GetCurrentFile(ae);
		if (ae->ifthen[ae->ii-1].v) {
			/* il faut signifier aux suivants qu'on va jusqu'au ENDIF */
			ae->ifthen[ae->ii-1].v=-1;
		} else {
			ae->ifthen[ae->ii-1].v=0;
		}
		ae->idx++;
	} else {
		rasm_printf(ae,"[%s:%d] ELSEIF need one expression\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}
void __ENDIF(struct s_assenv *ae) {
	if (ae->ii) {
		if (ae->wl[ae->idx].t==1) {
			ae->ii--;
		} else {
			rasm_printf(ae,"[%s:%d] ENDIF does not need any parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	} else {
		rasm_printf(ae,"[%s:%d] ENDIF encounter whereas there is no referent IF\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void __PROTECT(struct s_assenv *ae) {
	struct s_orgzone orgzone={0};

	if (!ae->wl[ae->idx].t && !ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t==1) {
		/* add a fake ORG zone */
		ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));
		/* then switch it with the current ORG */
		orgzone=ae->orgzone[ae->io-2];
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,0);
		ae->orgzone[ae->io-2].memstart=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,0,0,0);
		ae->orgzone[ae->io-2].memend=RoundComputeExpression(ae,ae->wl[ae->idx+2].w,0,0,0);
		ae->orgzone[ae->io-2].ibank=ae->activebank;
		ae->orgzone[ae->io-2].protect=1;
		ae->orgzone[ae->io-1]=orgzone;
		
	} else {
		rasm_printf(ae,"[%s:%d] PROTECT need two parameters: startadr,endadr\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

void ___org_close(struct s_assenv *ae) {
	if (ae->lz>=0) {
		rasm_printf(ae,"[%s:%d] Cannot ORG inside a LZ section\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(-5);
	}
	__internal_UpdateLZBlockIfAny(ae);
	/* close current ORG */
	if (ae->io) {
		ae->orgzone[ae->io-1].memend=ae->outputadr;
	}
}

void ___org_new(struct s_assenv *ae, int nocode) {
	struct s_orgzone orgzone={0};
	int i;
	
	/* check current ORG request */
	for (i=0;i<ae->io;i++) {
		/* aucun contr�le sur les ORG non �crits ou en NOCODE */
		if (ae->orgzone[i].memstart!=ae->orgzone[i].memend && !ae->orgzone[i].nocode) {
			if (ae->orgzone[i].ibank==ae->activebank) {
				if (ae->outputadr<ae->orgzone[i].memend && ae->outputadr>=ae->orgzone[i].memstart) {
					if (ae->orgzone[i].protect) {
						rasm_printf(ae,"[%s:%d] ORG located a PROTECTED section [#%04X-#%04X-B%d] file [%s] line %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->orgzone[i].memstart,ae->orgzone[i].memend,ae->orgzone[i].ibank<32?ae->orgzone[i].ibank:0,ae->filename[ae->orgzone[i].ifile],ae->orgzone[i].iline);
					} else {
						rasm_printf(ae,"[%s:%d] ORG (output at #%04X) located in a previous ORG section [#%04X-#%04X-B%d] file [%s] line %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->outputadr,ae->orgzone[i].memstart,ae->orgzone[i].memend,ae->orgzone[i].ibank<32?ae->orgzone[i].ibank:0,ae->filename[ae->orgzone[i].ifile],ae->orgzone[i].iline);
					}
					MaxError(ae);
				}
			}
		}
	}
	
	OverWriteCheck(ae);
	/* if there was a crunch block before, now closed */
	if (ae->lz>=0) {
		ae->lz=-1;
	}	
	orgzone.memstart=ae->outputadr;
	orgzone.ibank=ae->activebank;
	orgzone.ifile=ae->wl[ae->idx].ifile;
	orgzone.iline=ae->wl[ae->idx].l;
	orgzone.nocode=ae->nocode=nocode;

	if (nocode) {
		___output=___internal_output_nocode;
	} else {
		___output=___internal_output;
	}
	
	ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));
}

void __ORG(struct s_assenv *ae) {
	___org_close(ae);
	
	if (ae->wl[ae->idx+1].t!=2) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,0);
		ae->codeadr=RoundComputeExpression(ae,ae->wl[ae->idx+1].w,ae->outputadr,0,0);
		if (!ae->wl[ae->idx+1].t && ae->wl[ae->idx+2].t!=2) {
			ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,0);
			ae->outputadr=RoundComputeExpression(ae,ae->wl[ae->idx+2].w,ae->outputadr,0,0);
			ae->idx+=2;
		} else {
			ae->outputadr=ae->codeadr;
			ae->idx++;
		}
	} else {
		rasm_printf(ae,"[%s:%d] ORG code location[,output location]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	
	___org_new(ae,ae->nocode);
}
void __NOCODE(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___org_close(ae);
		___org_new(ae,1);
	} else {
		rasm_printf(ae,"[%s:%d] NOCODE directive does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __CODE(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___org_close(ae);
		___org_new(ae,0);
	} else {
		rasm_printf(ae,"[%s:%d] CODE directive does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __STRUCT(struct s_assenv *ae) {
	#undef FUNC
	#define FUNC "__STRUCT"
	struct s_rasmstructfield rasmstructfield;
	struct s_rasmstruct rasmstruct={0};
	struct s_rasmstruct rasmstructalias={0};
	struct s_label curlabel={0},*searched_label;
	int crc,i,irs;

	if (!ae->wl[ae->idx].t) {
		if (ae->wl[ae->idx+1].t) {
			/**************************************************
			    s t r u c t u r e     d e c l a r a t i o n
			**************************************************/
			if (!ae->getstruct) {
				/* cannot be an existing label or EQU (but variable ok) */
				crc=GetCRC(ae->wl[ae->idx+1].w);
				if ((SearchLabel(ae,ae->wl[ae->idx+1].w,crc))!=NULL || (SearchAlias(ae,crc,ae->wl[ae->idx+1].w))!=-1) {
					rasm_printf(ae,"[%s:%d] STRUCT name must be different from existing labels ou aliases\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				} else {
					ae->backup_filename=GetCurrentFile(ae);
					ae->backup_line=ae->wl[ae->idx].l;
					ae->backup_outputadr=ae->outputadr;
					ae->backup_codeadr=ae->codeadr;
					ae->getstruct=1;
					/* STRUCT = NOCODE + ORG 0 */
					___org_close(ae);
					ae->codeadr=0;
					___org_new(ae,1);
					/* create struct */
					rasmstruct.name=TxtStrDup(ae->wl[ae->idx+1].w);
					rasmstruct.crc=GetCRC(rasmstruct.name);
					ObjectArrayAddDynamicValueConcat((void **)&ae->rasmstruct,&ae->irasmstruct,&ae->mrasmstruct,&rasmstruct,sizeof(rasmstruct));
					ae->idx++;
				}
			} else {
				rasm_printf(ae,"[%s:%d] STRUCT cannot be declared inside previous opened STRUCT [%s] Line %d\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->backup_filename,ae->backup_line);
				MaxError(ae);
			}
		} else {
			/**************************************************
				s t r u c t u r e     i n s e r t i o n
			**************************************************/
			/* insert struct param1 in memory with name param2 */
			crc=GetCRC(ae->wl[ae->idx+1].w);
			/* look for existing struct */
			for (irs=0;irs<ae->irasmstruct;irs++) {
				if (ae->rasmstruct[irs].crc==crc && strcmp(ae->rasmstruct[irs].name,ae->wl[ae->idx+1].w)==0) break;
			}
			if (irs==ae->irasmstruct) {
				rasm_printf(ae,"[%s:%d] Unknown STRUCT %s\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->wl[ae->idx+1].w);
				MaxError(ae);
			} else {
				/* create alias for sizeof */
				if (!ae->getstruct) {
					if (ae->wl[ae->idx+2].w[0]=='@') {
						rasmstructalias.name=MakeLocalLabel(ae,ae->wl[ae->idx+2].w,NULL);
					} else {
						rasmstructalias.name=TxtStrDup(ae->wl[ae->idx+2].w);
					}
				} else {
					/* struct inside struct */
					rasmstructalias.name=MemMalloc(strlen(ae->rasmstruct[ae->irasmstruct-1].name)+2+strlen(ae->wl[ae->idx+2].w));
					sprintf(rasmstructalias.name,"%s.%s",ae->rasmstruct[ae->irasmstruct-1].name,ae->wl[ae->idx+2].w);
				}
				rasmstructalias.crc=GetCRC(rasmstructalias.name);
				rasmstructalias.size=ae->rasmstruct[irs].size;
				ObjectArrayAddDynamicValueConcat((void **)&ae->rasmstructalias,&ae->irasmstructalias,&ae->mrasmstructalias,&rasmstructalias,sizeof(rasmstructalias));
				
				/* create label for global struct ptr */
				curlabel.iw=-1;
				curlabel.ptr=ae->codeadr;
				if (!ae->getstruct) {
					if (ae->wl[ae->idx+2].w[0]=='@') curlabel.name=MakeLocalLabel(ae,ae->wl[ae->idx+2].w,NULL); else curlabel.name=TxtStrDup(ae->wl[ae->idx+2].w);
					curlabel.crc=GetCRC(curlabel.name);
					PushLabelLight(ae,&curlabel);
				} else {
					/* or check for non-local name in struct declaration */
					if (ae->wl[ae->idx+2].w[0]=='@') {
						rasm_printf(ae,"[%s:%d] Meaningless use of local label in a STRUCT definition\n",GetExpFile(ae,0),GetExpLine(ae,0));
						MaxError(ae);
					} else {
						curlabel.name=TxtStrDup(rasmstructalias.name);
						curlabel.crc=GetCRC(curlabel.name);
						PushLabelLight(ae,&curlabel);
					}
				}

				/* first field is in fact the very beginning of the structure */
				if (ae->getstruct) {
					rasmstructfield.name=TxtStrDup(ae->wl[ae->idx+2].w);
					rasmstructfield.offset=ae->codeadr;
					ObjectArrayAddDynamicValueConcat((void **)&ae->rasmstruct[ae->irasmstruct-1].rasmstructfield,
							&ae->rasmstruct[ae->irasmstruct-1].irasmstructfield,&ae->rasmstruct[ae->irasmstruct-1].mrasmstructfield,
							&rasmstructfield,sizeof(rasmstructfield));
				}				
				
				/* create subfields */
				curlabel.iw=-1;
				curlabel.ptr=ae->codeadr;
				for (i=0;i<ae->rasmstruct[irs].irasmstructfield;i++) {
					curlabel.ptr=ae->codeadr+ae->rasmstruct[irs].rasmstructfield[i].offset;
					if (!ae->getstruct) {
						curlabel.name=MemMalloc(strlen(ae->wl[ae->idx+2].w)+strlen(ae->rasmstruct[irs].rasmstructfield[i].name)+2);
						sprintf(curlabel.name,"%s.%s",ae->wl[ae->idx+2].w,ae->rasmstruct[irs].rasmstructfield[i].name);
						if (ae->wl[ae->idx+2].w[0]=='@') {
							char *newlabel;
							newlabel=MakeLocalLabel(ae,curlabel.name,NULL);
							MemFree(curlabel.name);
							curlabel.name=newlabel;
						}
						curlabel.crc=GetCRC(curlabel.name);
						PushLabelLight(ae,&curlabel);
					/* are we using a struct in a struct definition? */
					} else {
						/* copy structname+label+offset in the structure */
						rasmstructfield.name=MemMalloc(strlen(ae->wl[ae->idx+2].w)+strlen(ae->rasmstruct[irs].rasmstructfield[i].name)+2);
						sprintf(rasmstructfield.name,"%s.%s",ae->wl[ae->idx+2].w,ae->rasmstruct[irs].rasmstructfield[i].name);
						rasmstructfield.offset=curlabel.ptr;
						ObjectArrayAddDynamicValueConcat((void **)&ae->rasmstruct[ae->irasmstruct-1].rasmstructfield,
								&ae->rasmstruct[ae->irasmstruct-1].irasmstructfield,&ae->rasmstruct[ae->irasmstruct-1].mrasmstructfield,
								&rasmstructfield,sizeof(rasmstructfield));
								
						/* need to push also generic label */
						curlabel.name=MemMalloc(strlen(ae->rasmstruct[ae->irasmstruct-1].name)+strlen(rasmstructfield.name)+2); /* overwrite PTR */
						sprintf(curlabel.name,"%s.%s",ae->rasmstruct[ae->irasmstruct-1].name,rasmstructfield.name);
						curlabel.crc=GetCRC(curlabel.name);
						PushLabelLight(ae,&curlabel);
					}					
				}
				for (i=0;i<ae->rasmstruct[irs].size;i++) ___output(ae,0);
				ae->idx+=2;
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] STRUCT directive needs one or two parameters\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}
void __ENDSTRUCT(struct s_assenv *ae) {
	#undef FUNC
	#define FUNC "__ENDSTRUCT"
	struct s_label curlabel={0},*searched_label;
	int i,newlen;

	if (!ae->wl[ae->idx].t) {
		rasm_printf(ae,"[%s:%d] ENDSTRUCT directive does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	} else {
		if (ae->getstruct) {
			ae->rasmstruct[ae->irasmstruct-1].size=ae->codeadr;
			ae->getstruct=0;

			/* SIZEOF like Vasm with struct name */
			curlabel.name=TxtStrDup(ae->rasmstruct[ae->irasmstruct-1].name);
			curlabel.crc=ae->rasmstruct[ae->irasmstruct-1].crc;
			curlabel.iw=-1;
			curlabel.ptr=ae->rasmstruct[ae->irasmstruct-1].size;
			PushLabelLight(ae,&curlabel);
			
			/* compute size for each field */
			newlen=strlen(ae->rasmstruct[ae->irasmstruct-1].name)+2;
			for (i=0;i<ae->rasmstruct[ae->irasmstruct-1].irasmstructfield-1;i++) {
				ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].size=ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i+1].offset-ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].offset;
				ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname=MemMalloc(newlen+strlen(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].name));
				sprintf(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname,"%s.%s",ae->rasmstruct[ae->irasmstruct-1].name,ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].name);
				ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].crc=GetCRC(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname);
			}
			ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].size=ae->rasmstruct[ae->irasmstruct-1].size-ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].offset;
			ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname=MemMalloc(newlen+strlen(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].name));
			sprintf(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname,"%s.%s",ae->rasmstruct[ae->irasmstruct-1].name,ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].name);
			ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].crc=GetCRC(ae->rasmstruct[ae->irasmstruct-1].rasmstructfield[i].fullname);

			/* like there was no byte */
			ae->outputadr=ae->backup_outputadr;
			ae->codeadr=ae->backup_codeadr;

			___org_close(ae);
			___org_new(ae,0);
		} else {
			rasm_printf(ae,"[%s:%d] ENDSTRUCT encountered outside STRUCT declaration\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
	}
}

void __MEMSPACE(struct s_assenv *ae) {
	if (ae->wl[ae->idx].t) {
		___new_memory_space(ae);
	} else {
		rasm_printf(ae,"[%s:%d] MEMSPACE directive does not need parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}
}

int (*_internal_getsample)(unsigned char *data, int *idx);
#undef FUNC
#define FUNC "_internal_AudioGetSampleValue"

int __internal_getsample8(unsigned char *data, int *idx) {
	return data[*idx++]+0x80;
}
int __internal_getsample16little(unsigned char *data, int *idx) {
	int cursample;
	cursample=data[*idx+1]+0x80;*idx+=2;
	return cursample;
}
int __internal_getsample24little(unsigned char *data, int *idx) {
	int cursample;
	cursample=data[*idx+2+0x80];*idx+=3;
	return cursample;
}
/* big-endian */
int __internal_getsample16big(unsigned char *data, int *idx) {
	int cursample;
	cursample=data[*idx]+0x80;*idx+=2;
	return cursample;
}
int __internal_getsample24big(unsigned char *data, int *idx) {
	int cursample;
	cursample=data[*idx]+0x80;*idx+=3;
	return cursample;
}
/* float & endian shit */
int _isLittleEndian() /* from lz4.h */
{
    const union { U32 u; BYTE c[4]; } one = { 1 };
    return one.c[0];
}

unsigned char * __internal_floatinversion(unsigned char *data) {
	static unsigned char bswap[4];
	bswap[0]=data[3];
	bswap[1]=data[2];
	bswap[2]=data[1];
	bswap[3]=data[0];
	return bswap;
}

int __internal_getsample32bigbig(unsigned char *data, int *idx) {
	float fsample;
	int cursample;
	fsample=*((float*)(data+*idx));
	*idx+=4;
	cursample=(floor)((fsample+1.0)*127.5+0.5);
	return cursample;
}
int __internal_getsample32biglittle(unsigned char *data, int *idx) {
	float fsample;
	int cursample;
	fsample=*((float*)(__internal_floatinversion(data+*idx)));
	*idx+=4;
	cursample=(floor)((fsample+1.0)*127.5+0.5);
	return cursample;
}

#define __internal_getsample32littlelittle __internal_getsample32bigbig
#define __internal_getsample32littlebig __internal_getsample32biglittle


void _AudioLoadSample(struct s_assenv *ae, unsigned char *data, int filesize, enum e_audio_sample_type sample_type, float normalize)
{
	#undef FUNC
	#define FUNC "AudioLoadSample"

	struct s_wav_header *wav_header;
	int i,j,n,idx,controlsize;
	int nbchannel,bitspersample,nbsample;
	int bigendian=0,cursample;
	double accumulator;
	unsigned char samplevalue=0, sampleprevious=0;
	int samplerepeat=0,ipause;

	unsigned char *subchunk;
	int subchunksize;

	wav_header=(struct s_wav_header *)data;
	
	if (strncmp(wav_header->ChunkID,"RIFF",4)) {
		if (strncmp(wav_header->ChunkID,"RIFX",4)) {
			rasm_printf(ae,"[%s:%d] WAV import - unsupported audio sample type (chunkid must be 'RIFF' or 'RIFX')\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
			return;
		} else {
			bigendian=1;
		}
	}
	if (strncmp(wav_header->Format,"WAVE",4)) {
		rasm_printf(ae,"[%s:%d] WAV import - unsupported audio sample type (format must be 'WAVE')\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	controlsize=wav_header->SubChunk1Size[0]+wav_header->SubChunk1Size[1]*256+wav_header->SubChunk1Size[2]*65536+wav_header->SubChunk1Size[3]*256*65536;
	if (controlsize!=16) {
		rasm_printf(ae,"[%s:%d] WAV import - invalid wav chunk size (subchunk1 control)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	if (strncmp(wav_header->SubChunk1ID,"fmt",3)) {
		rasm_printf(ae,"[%s:%d] WAV import - unsupported audio sample type (subchunk1id must be 'fmt')\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}

	subchunk=(unsigned char *)&wav_header->SubChunk2ID;
	while (strncmp(subchunk,"data",4)) {
		subchunksize=8+subchunk[4]+subchunk[5]*256+subchunk[6]*65536+subchunk[7]*256*65536;
		if (subchunksize>=filesize) {
			rasm_printf(ae,"[%s:%d] WAV import - data subchunk not found\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
			return;
		}
		subchunk+=subchunksize;
	}
	subchunksize=8+subchunk[4]+subchunk[5]*256+subchunk[6]*65536+subchunk[7]*256*65536;
	controlsize=subchunksize;
	subchunk+=8;

	nbchannel=wav_header->NumChannels[0]+wav_header->NumChannels[1]*256;
	if (nbchannel<1) {
		rasm_printf(ae,"[%s:%d] WAV import - invalid number of audio channel\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
		return;
	}
	bitspersample=wav_header->BitsPerSample[0]+wav_header->BitsPerSample[1]*256;
	switch (bitspersample) {
		case 8:_internal_getsample=__internal_getsample8;break;
		case 16:if (!bigendian) _internal_getsample=__internal_getsample16little; else _internal_getsample=__internal_getsample16big;break;
		case 24:if (!bigendian) _internal_getsample=__internal_getsample24little; else _internal_getsample=__internal_getsample24big;break;
		case 32:if (!bigendian) {
					if (_isLittleEndian()) {
						_internal_getsample=__internal_getsample32littlelittle;
					} else {
						_internal_getsample=__internal_getsample32littlebig;
					}
				} else {
					if (_isLittleEndian()) {
						_internal_getsample=__internal_getsample32biglittle;
					} else {
						_internal_getsample=__internal_getsample32bigbig;
					}
				}
				break;
		default:
			rasm_printf(ae,"[%s:%d] WAV import - unsupported bits per sample (%d)\n",GetCurrentFile(ae),ae->wl[ae->idx].l,bitspersample);
			MaxError(ae);
			return;
	}

	nbsample=controlsize/nbchannel/(bitspersample/8);
	//rasm_printf(ae,"@@DBG nbsample=%d (sze=%d,chn=%d,bps=%d) st=%c\n",nbsample,controlsize,nbchannel,bitspersample,sample_type);
	
	idx=subchunk-data;
	switch (sample_type) {
		default:
		case AUDIOSAMPLE_SMP:
			for (i=0;i<nbsample;i++) {
				/* downmixing */
				accumulator=0.0;
				for (n=0;n<nbchannel;n++) {
					accumulator+=_internal_getsample(data,&idx);
				}
				/* normalize */
				cursample=floor(((accumulator/nbchannel)*normalize)+0.5);
				
				/* PSG levels */
				samplevalue=ae->psgfine[cursample];
				
				/* output */
				___output(ae,samplevalue);
			}
			break;
		case AUDIOSAMPLE_SM2:
			for (i=0;i<nbsample;i+=2) {
				for (j=0;j<2;j++) {
					/* downmixing */
					accumulator=0.0;
					for (n=0;n<nbchannel;n++) {
						accumulator+=_internal_getsample(data,&idx);
					}
					/* normalize */
					cursample=floor(((accumulator/nbchannel)*normalize)+0.5);
					/* PSG levels & packing */
					samplevalue=(samplevalue<<4)+(ae->psgfine[cursample]);
				}
				
				/* output */
				___output(ae,samplevalue);
			}
			break;
		case AUDIOSAMPLE_SM4:
			/***
				SM4 format has two bits
				bits -> PSG value
				  00 ->  0
				  01 -> 13
				  10 -> 14
				  11 -> 15				
			***/
			for (i=0;i<nbsample;i+=4) {
				for (j=0;j<4;j++) {
					/* downmixing */
					accumulator=0.0;
					for (n=0;n<nbchannel;n++) {
						accumulator+=_internal_getsample(data,&idx);
					}
					/* normalize */
					cursample=floor(((accumulator/nbchannel)*normalize)+0.5);
					/* PSG levels & packing */
					samplevalue=(samplevalue<<2)+(ae->psgtab[cursample]>>2);
				}
				/* output */
				___output(ae,samplevalue);
			}
			break;
		case AUDIOSAMPLE_DMA:
			sampleprevious=255;
			for (i=0;i<nbsample;i++) {
				/* downmixing */
				accumulator=0.0;
				for (n=0;n<nbchannel;n++) {
					accumulator+=_internal_getsample(data,&idx);
				}
				/* normalize */
				cursample=floor(((accumulator/nbchannel)*normalize)+0.5);
				
				/* PSG levels */
				samplevalue=ae->psgtab[cursample];
				
				if (samplevalue==sampleprevious) {
					samplerepeat++;
				} else {
					if (!samplerepeat) {
						/* DMA output */
						___output(ae,sampleprevious);
						___output(ae,0x0A); /* volume canal C */
					} else {
						/* DMA pause */
						___output(ae,sampleprevious);
						___output(ae,0x0A); /* volume canal C */
						while (samplerepeat) {
							ipause=samplerepeat<4096?samplerepeat:4095;
							___output(ae,ipause&0xFF);
							___output(ae,0x10 | ((ipause>>8) &0xF)); /* pause */
							
							samplerepeat-=4096;
							if (samplerepeat<0) samplerepeat=0;
						}
					}
					sampleprevious=samplevalue;
				}
			}
			if (samplerepeat) {
				/* DMA pause */
				___output(ae,sampleprevious);
				___output(ae,0x0A); /* volume canal C */
				while (samplerepeat) {
					ipause=samplerepeat<4096?samplerepeat:4095;
					___output(ae,ipause&0xFF);
					___output(ae,0x10 | ((ipause>>8) &0xF)); /* pause */
					
					samplerepeat-=4096;
					if (samplerepeat<0) samplerepeat=0;
				}
			}
			___output(ae,0x20);
			___output(ae,0x40); /* stop or reloop? */
			break;
	}
}

/*
	meta fonction qui g�re le INCBIN standard plus les variantes SMP et DMA
*/
void __HEXBIN(struct s_assenv *ae) {
	int hbinidx,overwritecheck=1,crc;
	struct s_expr_dico *rvar;
	unsigned int idx;
	int size=0,offset=0;
	float normalize;
	
	if (!ae->wl[ae->idx].t) {
		ExpressionFastTranslate(ae,&ae->wl[ae->idx+1].w,1);
		hbinidx=RoundComputeExpressionCore(ae,ae->wl[ae->idx+1].w,ae->codeadr,0);
		if (!ae->wl[ae->idx+1].t) {
			/* SMP,SM2,SM4,DMA */
			if (strchr("SD",ae->wl[ae->idx+2].w[0]) && ae->wl[ae->idx+2].w[1]=='M' &&
					strchr("P24A",ae->wl[ae->idx+2].w[2]) && !ae->wl[ae->idx+2].w[3]) {
				switch (ae->wl[ae->idx+2].w[2]) {
					case 'P':_AudioLoadSample(ae,ae->hexbin[hbinidx].data,ae->hexbin[hbinidx].datalen, AUDIOSAMPLE_SMP,1.0);break;
					case '2':_AudioLoadSample(ae,ae->hexbin[hbinidx].data,ae->hexbin[hbinidx].datalen, AUDIOSAMPLE_SM2,1.0);break;
					case '4':_AudioLoadSample(ae,ae->hexbin[hbinidx].data,ae->hexbin[hbinidx].datalen, AUDIOSAMPLE_SM4,1.0);break;
					case 'A':_AudioLoadSample(ae,ae->hexbin[hbinidx].data,ae->hexbin[hbinidx].datalen, AUDIOSAMPLE_DMA,1.0);break;
					default:printf("warning remover\n");break;
				}				
				ae->idx+=2;
				return;
			} else {
				/* legacy binary file */
				ExpressionFastTranslate(ae,&ae->wl[ae->idx+2].w,1);
				offset=RoundComputeExpressionCore(ae,ae->wl[ae->idx+2].w,ae->codeadr,0);
				if (!ae->wl[ae->idx+2].t) {
					if (ae->wl[ae->idx+3].w[0]) {
						ExpressionFastTranslate(ae,&ae->wl[ae->idx+3].w,1);
						size=RoundComputeExpressionCore(ae,ae->wl[ae->idx+3].w,ae->codeadr,0);
					} else {
						size=0;
					}
					if (size<-65535 || size>65536) {
						rasm_printf(ae,"[%s:%d] INCBIN invalid size\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					}
					if (!ae->wl[ae->idx+3].t) {
						if (ae->wl[ae->idx+4].w[0]) {
							ExpressionFastTranslate(ae,&ae->wl[ae->idx+4].w,1);
							offset+=65536*RoundComputeExpressionCore(ae,ae->wl[ae->idx+4].w,ae->codeadr,0);
						}
						if (!ae->wl[ae->idx+4].t) {
							if (strcmp(ae->wl[ae->idx+5].w,"OFF")==0) {
								overwritecheck=0;
							} else if (strcmp(ae->wl[ae->idx+5].w,"ON")==0) {
								overwritecheck=1;
							} else if (ae->wl[ae->idx+5].w[0]) {
								rasm_printf(ae,"[%s:%d] INCBIN invalid overwrite value. Must be 'OFF' or 'ON'\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
								MaxError(ae);
							}
							if (!ae->wl[ae->idx+5].t) {
								/* copy raw len to a (new) variable */
								crc=GetCRC(ae->wl[ae->idx+6].w);
								if ((rvar=SearchDico(ae,ae->wl[ae->idx+6].w,crc))!=NULL) {
									rvar->v=ae->hexbin[hbinidx].rawlen;
								} else {
									/* mais ne peut �tre un label ou un alias */
									ExpressionSetDicoVar(ae,ae->wl[ae->idx+6].w,ae->hexbin[hbinidx].rawlen);
								}
								ae->idx+=6;
							} else {
								ae->idx+=5;
							}
						} else {
							ae->idx+=4;
						}
					} else {
						ae->idx+=3;
					}
				} else {
					ae->idx+=2;
				}
			}
		} else {
			ae->idx++;
		}

		if (ae->hexbin[hbinidx].datalen<0) {
			rasm_printf(ae,"[%s:%d] file not found [%s]\n",GetCurrentFile(ae),ae->wl[ae->idx].l,ae->hexbin[hbinidx].filename);
			MaxError(ae);
		} else {
			if (hbinidx<ae->ih && hbinidx>=0) {
				if (size<0) {
					size=ae->hexbin[hbinidx].datalen-size;
					if (size<1) {
						rasm_printf(ae,"[%s:%d] INCBIN negative size is greater or equal to filesize\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					}
				}
				/* negative offset conversion */
				if (offset<0) {
					offset=ae->hexbin[hbinidx].datalen+offset;
				}
				if (!size) {
					if (!offset) {
						size=ae->hexbin[hbinidx].datalen;
					} else {
						size=ae->hexbin[hbinidx].datalen-offset;
					}
				}
				if (size>ae->hexbin[hbinidx].datalen) {
					rasm_printf(ae,"[%s:%d] INCBIN size is greater than filesize\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					MaxError(ae);
				} else {
					if (size+offset>ae->hexbin[hbinidx].datalen) {
						rasm_printf(ae,"[%s:%d] INCBIN size+offset is greater than filesize\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
						MaxError(ae);
					} else {
						if (overwritecheck) {
							for (idx=offset;idx<size+offset;idx++) {
								___output(ae,ae->hexbin[hbinidx].data[idx]);
							}
						} else {
							___org_close(ae);
							___org_new(ae,0);
							for (idx=offset;idx<size+offset;idx++) {
								___output(ae,ae->hexbin[hbinidx].data[idx]);
							}
							/* hack to disable overwrite check */
							ae->orgzone[ae->io-1].nocode=2;
							___org_close(ae);
							___org_new(ae,0);
						}
					}
				}
			} else {
				rasm_printf(ae,"[%s:%d] INTERNAL - HEXBIN refer to unknown structure\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
				MaxError(ae);
			}
		}
	} else {
		rasm_printf(ae,"[%s:%d] INTERNAL - HEXBIN need one HEX parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		FreeAssenv(ae);
		exit(2);
	}
}

/*
save "nom",start,size -> save binary
save "nom",start,size,AMSDOS -> save binary with Amsdos header
save "nom",start,size,DSK,"dskname" -> save binary on DSK data format
save "nom",start,size,DSK,"dskname",B -> select face
save "nom",start,size,DSK,B -> current DSK, choose face
save "nom",start,size,DSK -> current DSK, current face
*/
void __SAVE(struct s_assenv *ae) {
	struct s_save cursave={0};
	unsigned int offset=0,size=0;
	int ko=1;
	
	if (!ae->wl[ae->idx].t) {
		/* nom de fichier entre quotes ou bien mot clef DSK */
		if (!StringIsQuote(ae->wl[ae->idx+1].w)) {
			rasm_printf(ae,"[%s:%d] SAVE invalid filename quote\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
			ko=0;
		} else {
			if (!ae->wl[ae->idx+1].t) {
				if (!ae->wl[ae->idx+2].t && ae->wl[ae->idx+3].t!=2) {
					cursave.ibank=ae->activebank;
					cursave.ioffset=ae->idx+2;
					cursave.isize=ae->idx+3;
					cursave.iw=ae->idx+1;
					if (!ae->wl[ae->idx+3].t) {
						if (strcmp(ae->wl[ae->idx+4].w,"AMSDOS")==0) {
							cursave.amsdos=1;
						} else if (strcmp(ae->wl[ae->idx+4].w,"DSK")==0) {
							cursave.dsk=1;
							if (!ae->wl[ae->idx+4].t) {
								cursave.iwdskname=ae->idx+5;
								if (!ae->wl[ae->idx+5].t) {
									/* face selection - 0 as default */
									switch (ae->wl[ae->idx+6].w[0]) {
										case '1':
										case 'B':
											cursave.face=1;
											break;
										case '0':
										case 'A':
										default:
											cursave.face=0;
											break;
									}
								}
							} else {
								if (ae->nbsave && ae->save[ae->nbsave-1].iwdskname!=-1) {
									cursave.iwdskname=ae->save[ae->nbsave-1].iwdskname; /* previous DSK */
									cursave.face=ae->save[ae->nbsave-1].face; /* previous face */
								} else {
									cursave.iwdskname=-1;
									rasm_printf(ae,"[%s:%d] cannot autoselect DSK as there was not a previous selection\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
									MaxError(ae);
								}
							}
						} else {
							rasm_printf(ae,"[%s:%d] SAVE 4th parameter must be empty or AMSDOS or DSK\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
							MaxError(ae);
							ko=0;
						}
					}
					ObjectArrayAddDynamicValueConcat((void**)&ae->save,&ae->nbsave,&ae->maxsave,&cursave,sizeof(cursave));
					ko=0;
				}
			}
		}
	}
	if (ko) {
		rasm_printf(ae,"[%s:%d] Use SAVE 'filename',offset,size[,AMSDOS|DSK[,A|B|'dskname'[,A|B]]]\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
		MaxError(ae);
	}

	while (!ae->wl[ae->idx].t) ae->idx++;
}


void __MODULE(struct s_assenv *ae) {
	if (!ae->wl[ae->idx].t && ae->wl[ae->idx+1].t==1) {
		if (StringIsQuote(ae->wl[ae->idx+1].w)) {
			ae->module=MemMalloc(strlen(ae->wl[ae->idx+1].w));
			/* duplicate and remove quotes */
			strcpy(ae->module,ae->wl[ae->idx+1].w+1);
			ae->module[strlen(ae->module)-1]=0;
		} else {
			rasm_printf(ae,"[%s:%d] MODULE directive need one text parameter\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
			MaxError(ae);
		}
		ae->idx++;
	} else {
		if (ae->module) MemFree(ae->module);
		ae->module=NULL;
	}
}

struct s_asm_keyword instruction[]={
{"LD",0,_LD},
{"DEC",0,_DEC},
{"INC",0,_INC},
{"ADD",0,_ADD},
{"SUB",0,_SUB},
{"OR",0,_OR},
{"AND",0,_AND},
{"XOR",0,_XOR},
{"POP",0,_POP},
{"PUSH",0,_PUSH},
{"DJNZ",0,_DJNZ},
{"JR",0,_JR},
{"JP",0,_JP},
{"CALL",0,_CALL},
{"RET",0,_RET},
{"EX",0,_EX},
{"ADC",0,_ADC},
{"SBC",0,_SBC},
{"EXX",0,_EXX},
{"CP",0,_CP},
{"BIT",0,_BIT},
{"RES",0,_RES},
{"SET",0,_SET},
{"IN",0,_IN},
{"OUT",0,_OUT},
{"RLC",0,_RLC},
{"RRC",0,_RRC},
{"RL",0,_RL},
{"RR",0,_RR},
{"SLA",0,_SLA},
{"SRA",0,_SRA},
{"SLL",0,_SLL},
{"SL1",0,_SLL},
{"SRL",0,_SRL},
{"RST",0,_RST},
{"HALT",0,_HALT},
{"DI",0,_DI},
{"EI",0,_EI},
{"NOP",0,_NOP},
{"DEFR",0,_DEFR},
{"DEFB",0,_DEFB},
{"DEFM",0,_DEFB},
{"DR",0,_DEFR},
{"DM",0,_DEFB},
{"DB",0,_DEFB},
{"DEFW",0,_DEFW},
{"DW",0,_DEFW},
{"DEFS",0,_DEFS},
{"DS",0,_DEFS},
{"STR",0,_STR},
{"LDI",0,_LDI},
{"LDIR",0,_LDIR},
{"OUTI",0,_OUTI},
{"INI",0,_INI},
{"RLCA",0,_RLCA},
{"RRCA",0,_RRCA},
{"NEG",0,_NEG},
{"RLA",0,_RLA},
{"RRA",0,_RRA},
{"RLD",0,_RLD},
{"RRD",0,_RRD},
{"DAA",0,_DAA},
{"CPL",0,_CPL},
{"SCF",0,_SCF},
{"LDD",0,_LDD},
{"LDDR",0,_LDDR},
{"CCF",0,_CCF},
{"OUTD",0,_OUTD},
{"IND",0,_IND},
{"RETI",0,_RETI},
{"RETN",0,_RETN},
{"IM",0,_IM},
{"DEFI",0,_DEFI},
{"CPD",0,_CPD},
{"CPI",0,_CPI},
{"CPDR",0,_CPDR},
{"CPIR",0,_CPIR},
{"OTDR",0,_OTDR},
{"OTIR",0,_OTIR},
{"INDR",0,_INDR},
{"INIR",0,_INIR},
{"REPEAT",0,__REPEAT},
{"REND",0,__REND},
{"UNTIL",0,__UNTIL},
{"ORG",0,__ORG},
{"PROTECT",0,__PROTECT},
{"WHILE",0,__WHILE},
{"WEND",0,__WEND},
{"HEXBIN",0,__HEXBIN},
{"ALIGN",0,__ALIGN},
{"ELSEIF",0,__ELSEIF},
{"ELSE",0,__ELSE},
{"IF",0,__IF},
{"ENDIF",0,__ENDIF},
{"IFNOT",0,__IFNOT},
{"IFDEF",0,__IFDEF},
{"IFNDEF",0,__IFNDEF},
{"IFUSED",0,__IFUSED},
{"IFNUSED",0,__IFNUSED},
{"UNDEF",0,__UNDEF},
{"CASE",0,__CASE},
{"BREAK",0,__BREAK},
{"DEFAULT",0,__DEFAULT},
{"SWITCH",0,__SWITCH},
{"ENDSWITCH",0,__ENDSWITCH},
{"WRITE",0,__WRITE},
{"CODE",0,__CODE},
{"NOCODE",0,__NOCODE},
{"MEMSPACE",0,__MEMSPACE},
{"MACRO",0,__MACRO},
{"TICKER",0,__TICKER},
{"LET",0,__LET},
{"ASSERT",0,__ASSERT},
{"CHARSET",0,__CHARSET},
{"RUN",0,__RUN},
{"SAVE",0,__SAVE},
{"BRK",0,__BRK},
{"NOLIST",0,__NOLIST},
{"LIST",0,__LIST},
{"STOP",0,__STOP},
{"PRINT",0,__PRINT},
{"FAIL",0,__FAIL},
{"BREAKPOINT",0,__BREAKPOINT},
{"BANK",0,__BANK},
{"BANKSET",0,__BANKSET},
{"NAMEBANK",0,__NameBANK},
{"LIMIT",0,__LIMIT},
{"LZEXO",0,__LZEXO},
{"LZX7",0,__LZX7},
{"LZ4",0,__LZ4},
{"LZ48",0,__LZ48},
{"LZ49",0,__LZ49},
{"LZCLOSE",0,__LZCLOSE},
{"BUILDCPR",0,__BUILDCPR},
{"BUILDSNA",0,__BUILDSNA},
{"SETCPC",0,__SETCPC},
{"SETCRTC",0,__SETCRTC},
{"AMSDOS",0,__AMSDOS},
{"OTD",0,_OUTD},
{"OTI",0,_OUTI},
{"SHL",0,_SLA},
{"SHR",0,_SRL},
{"STRUCT",0,__STRUCT},
{"ENDSTRUCT",0,__ENDSTRUCT},
{"ENDS",0,__ENDSTRUCT},
{"",0,NULL}
};

int Assemble(struct s_assenv *ae, unsigned char **dataout, int *lenout)
{
	#undef FUNC
	#define FUNC "Assemble"

	unsigned char *AmsdosHeader;
	struct s_expression curexp={0};
	struct s_wordlist *wordlist;
	struct s_expr_dico curdico={0};
	struct s_label *curlabel;
	int icrc,curcrc,i,j,k;
	unsigned char *lzdata=NULL;
	int lzlen,lzshift,input_size;
	size_t slzlen;
	unsigned char *input_data;
	struct s_orgzone orgzone={0};
	int iorgzone,ibank,offset,endoffset;
	int il,maxrom;
	char *TMP_filename=NULL;
	int minmem=65536,maxmem=0,lzmove;
	char symbol_line[1024];
	int ifast,executed;
	/* debug */
	int curii,inhibe;
	int ok;

	rasm_printf(ae,"Assembling\n");

	srand((unsigned)time(0));
	
	wordlist=ae->wl;
	ae->wl=wordlist;
	/* start outside crunched section */
	ae->lz=-1;
	
	/* default orgzone */
	orgzone.ibank=36;
	ObjectArrayAddDynamicValueConcat((void**)&ae->orgzone,&ae->io,&ae->mo,&orgzone,sizeof(orgzone));
	___output=___internal_output;
	/* init des automates */
	InitAutomate(ae->AutomateHexa,AutomateHexaDefinition);
	InitAutomate(ae->AutomateDigit,AutomateDigitDefinition);
	InitAutomate(ae->AutomateValidLabel,AutomateValidLabelDefinition);
	InitAutomate(ae->AutomateValidLabelFirst,AutomateValidLabelFirstDefinition);
	InitAutomate(ae->AutomateExpressionValidCharExtended,AutomateExpressionValidCharExtendedDefinition);
	InitAutomate(ae->AutomateExpressionValidCharFirst,AutomateExpressionValidCharFirstDefinition);
	InitAutomate(ae->AutomateExpressionValidChar,AutomateExpressionValidCharDefinition);
	ae->AutomateExpressionDecision['<']='<';
	ae->AutomateExpressionDecision['>']='>';
	ae->AutomateExpressionDecision['=']='=';
	ae->AutomateExpressionDecision['!']='!';
	ae->AutomateExpressionDecision[0]='E';
	/* gestion d'alias */
	ae->AutomateExpressionDecision['~']='~';
	/* set operator precedence */
	if (!ae->maxam) {
		for (i=0;i<256;i++) {
			switch (i) {
				/* priority 0 */
				case '(':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_OPEN;ae->AutomateElement[i].priority=0;break;
				case ')':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_CLOSE;ae->AutomateElement[i].priority=0;break;
				/* priority 1 */
				case 'b':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_NOT;ae->AutomateElement[i].priority=1;break;
				/* priority 2 */
				case '*':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_MUL;ae->AutomateElement[i].priority=2;break;
				case '/':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_DIV;ae->AutomateElement[i].priority=2;break;
				case 'm':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_MOD;ae->AutomateElement[i].priority=2;break;
				/* priority 3 */
				case '+':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_ADD;ae->AutomateElement[i].priority=3;break;
				case '-':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SUB;ae->AutomateElement[i].priority=3;break;
				/* priority 4 */
				case '[':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SHL;ae->AutomateElement[i].priority=4;break;
				case ']':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SHR;ae->AutomateElement[i].priority=4;break;
				/* priority 5 */
				case 'l':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_LOWER;ae->AutomateElement[i].priority=5;break;
				case 'g':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_GREATER;ae->AutomateElement[i].priority=5;break;
				case 'e':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_EQUAL;ae->AutomateElement[i].priority=5;break;
				case 'n':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_NOTEQUAL;ae->AutomateElement[i].priority=5;break;
				case 'k':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_LOWEREQ;ae->AutomateElement[i].priority=5;break;
				case 'h':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_GREATEREQ;ae->AutomateElement[i].priority=5;break;
				/* priority 6 */
				case '&':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_AND;ae->AutomateElement[i].priority=6;break;
				/* priority 7 */
				case '^':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_XOR;ae->AutomateElement[i].priority=7;break;
				/* priority 8 */
				case '|':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_OR;ae->AutomateElement[i].priority=8;break;
				/* priority 9 */
				case 'a':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_BAND;ae->AutomateElement[i].priority=9;break;
				/* priority 10 */
				case 'o':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_BOR;ae->AutomateElement[i].priority=10;break;
				default:ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_END;
			}
		}
	} else {
		for (i=0;i<256;i++) {
			switch (i) {
				/* priority 0 */
				case '(':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_OPEN;ae->AutomateElement[i].priority=0;break;
				case ')':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_CLOSE;ae->AutomateElement[i].priority=0;break;
				/* priority 0.5 */
				case 'b':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_NOT;ae->AutomateElement[i].priority=128;break;
				/* priority 1 */
				case '*':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_MUL;ae->AutomateElement[i].priority=464;break;
				case '/':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_DIV;ae->AutomateElement[i].priority=464;break;
				case 'm':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_MOD;ae->AutomateElement[i].priority=464;break;
				case '+':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_ADD;ae->AutomateElement[i].priority=464;break;
				case '-':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SUB;ae->AutomateElement[i].priority=464;break;
				case '[':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SHL;ae->AutomateElement[i].priority=464;break;
				case ']':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_SHR;ae->AutomateElement[i].priority=464;break;
				case '&':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_AND;ae->AutomateElement[i].priority=464;break;
				case '^':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_XOR;ae->AutomateElement[i].priority=464;break;
				case '|':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_OR;ae->AutomateElement[i].priority=464;break;
				/* priority 2 */
				case 'l':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_LOWER;ae->AutomateElement[i].priority=664;break;
				case 'g':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_GREATER;ae->AutomateElement[i].priority=664;break;
				case 'e':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_EQUAL;ae->AutomateElement[i].priority=664;break;
				case 'n':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_NOTEQUAL;ae->AutomateElement[i].priority=664;break;
				case 'k':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_LOWEREQ;ae->AutomateElement[i].priority=664;break;
				case 'h':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_GREATEREQ;ae->AutomateElement[i].priority=664;break;
				/* priority 3 */
				case 'a':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_BAND;ae->AutomateElement[i].priority=6128;break;
				case 'o':ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_BOR;ae->AutomateElement[i].priority=6128;break;
				default:ae->AutomateElement[i].operator=E_COMPUTE_OPERATION_END;
			}
		}
	}

	/* psg conversion */
	for (i=j=0;i<100;i++) ae->psgtab[j++]=0;
	for (i=0;i<49;i++) ae->psgtab[j++]=13;
	for (i=0;i<35;i++) ae->psgtab[j++]=14;
	for (i=0;i<72;i++) ae->psgtab[j++]=15;
	if (j!=256) {
		rasm_printf(ae,"Internal error with PSG conversion table\n");
		exit(-44);
	}
	for (i=j=0;i<1;i++) ae->psgfine[j++]=0;
	for (i=0;i<1;i++) ae->psgfine[j++]=1;
	for (i=0;i<1;i++) ae->psgfine[j++]=2;
	for (i=0;i<2;i++) ae->psgfine[j++]=3;
	for (i=0;i<2;i++) ae->psgfine[j++]=4;
	for (i=0;i<2;i++) ae->psgfine[j++]=5;
	for (i=0;i<3;i++) ae->psgfine[j++]=6;
	for (i=0;i<4;i++) ae->psgfine[j++]=7;
	for (i=0;i<7;i++) ae->psgfine[j++]=8;
	for (i=0;i<9;i++) ae->psgfine[j++]=9;
	for (i=0;i<13;i++) ae->psgfine[j++]=10;
	for (i=0;i<19;i++) ae->psgfine[j++]=11;
	for (i=0;i<27;i++) ae->psgfine[j++]=12;
	for (i=0;i<37;i++) ae->psgfine[j++]=13;
	for (i=0;i<53;i++) ae->psgfine[j++]=14;
	for (i=0;i<75;i++) ae->psgfine[j++]=15;
	if (j!=256) {
		rasm_printf(ae,"Internal error with PSG conversion table\n");
		exit(-44);
	}
	/* default var */
	ExpressionSetDicoVar(ae,"PI",3.1415926545);
	ExpressionSetDicoVar(ae,"ASSEMBLER_RASM",1);
	
	/* add a fictive expression to simplify test when parsing expressions */
	ObjectArrayAddDynamicValueConcat((void **)&ae->expression,&ae->ie,&ae->me,&curexp,sizeof(curexp));
	
	/* compute CRC for keywords and directives */
	for (icrc=0;instruction[icrc].mnemo[0];icrc++) instruction[icrc].crc=GetCRC(instruction[icrc].mnemo);
	for (icrc=0;math_keyword[icrc].mnemo[0];icrc++) math_keyword[icrc].crc=GetCRC(math_keyword[icrc].mnemo);

	if (ae->as80==1) { /* not for UZ80 */
		for (icrc=0;instruction[icrc].mnemo[0];icrc++) {
			if (strcmp(instruction[icrc].mnemo,"DEFB")==0 || strcmp(instruction[icrc].mnemo,"DB")==0) {
				instruction[icrc].makemnemo=_DEFB_as80;
			} else if (strcmp(instruction[icrc].mnemo,"DEFW")==0 || strcmp(instruction[icrc].mnemo,"DW")==0) {
				instruction[icrc].makemnemo=_DEFW_as80;
			} else if (strcmp(instruction[icrc].mnemo,"DEFI")==0) {
				instruction[icrc].makemnemo=_DEFI_as80;
			}
		}
	}
	/* Execution des mots clefs */
	/**********************************************************
	       A S S E M B L I N G    M A I N    L O O P
	**********************************************************/
	ae->idx=1;
	while (wordlist[ae->idx].t!=2) {
		curcrc=GetCRC(wordlist[ae->idx].w);
		/*********************
		 d e b u g   i n f o
		*********************/
		if (ae->verbose&4) {
			int iiii=0;
			rasm_printf(ae,"%d [%s] L%d [%s]e=%d ",ae->idx,ae->filename[wordlist[ae->idx].ifile],wordlist[ae->idx].l,wordlist[ae->idx].w,wordlist[ae->idx].e);
			while (!wordlist[ae->idx+iiii++].t) rasm_printf(ae," [%s]e=%d ",wordlist[ae->idx+iiii].w,wordlist[ae->idx+iiii].e);
			
			for (iiii=0;iiii<ae->imacropos;iiii++) {
				rasm_printf(ae,"M[%d] s=%d e=%d ",iiii,ae->macropos[iiii].start,ae->macropos[iiii].end);
			}
			rasm_printf(ae,"\n");
		}
		/********************************************************************
		  c o n d i t i o n n a l    a s s e m b l y    m a n a g e m e n t
		********************************************************************/
		if (ae->ii || ae->isw) {
			/* inhibition of if/endif */
			for (inhibe=curii=0;curii<ae->ii;curii++) {
				if (!ae->ifthen[curii].v || ae->ifthen[curii].v==-1) {
					inhibe=1;
					break;
				}
			}
			/* when inhibited we are looking only for a IF/IFDEF/IFNOT/IFNDEF/ELSE/ELSEIF/ENDIF or SWITCH/CASE/DEFAULT/ENDSWITCH */
			if (inhibe) {
				/* this section does NOT need to be agressively optimized !!! */
				if (curcrc==CRC_ELSEIF && strcmp(wordlist[ae->idx].w,"ELSEIF")==0) {
					/* true IF needs to be done ONLY on the active level */
					if (curii==ae->ii-1) __ELSEIF(ae); else __ELSEIF_light(ae);
				} else if (curcrc==CRC_ELSE && strcmp(wordlist[ae->idx].w,"ELSE")==0) {
					__ELSE(ae);
				} else if (curcrc==CRC_ENDIF && strcmp(wordlist[ae->idx].w,"ENDIF")==0) {
					__ENDIF(ae);
				} else if (curcrc==CRC_IF && strcmp(wordlist[ae->idx].w,"IF")==0) {
					/* as we are inhibited we do not have to truly compute IF */
					__IF_light(ae);
				} else if (curcrc==CRC_IFDEF && strcmp(wordlist[ae->idx].w,"IFDEF")==0) {
					__IFDEF_light(ae);
				} else if (curcrc==CRC_IFNOT && strcmp(wordlist[ae->idx].w,"IFNOT")==0) {
					__IFNOT_light(ae);
				} else if (curcrc==CRC_IFUSED && strcmp(wordlist[ae->idx].w,"IFUSED")==0) {
					__IFUSED_light(ae);
				} else if (curcrc==CRC_IFNUSED && strcmp(wordlist[ae->idx].w,"IFNUSED")==0) {
					__IFNUSED_light(ae);
				} else if (curcrc==CRC_IFNDEF && strcmp(wordlist[ae->idx].w,"IFNDEF")==0) {
					__IFNDEF_light(ae);
				} else if (curcrc==CRC_SWITCH && strcmp(wordlist[ae->idx].w,"SWITCH")==0) {
					__SWITCH_light(ae);
				} else if (curcrc==CRC_CASE && strcmp(wordlist[ae->idx].w,"CASE")==0) {
					__CASE_light(ae);
				} else if (curcrc==CRC_ENDSWITCH && strcmp(wordlist[ae->idx].w,"ENDSWITCH")==0) {
					__ENDSWITCH(ae);
				} else if (curcrc==CRC_BREAK && strcmp(wordlist[ae->idx].w,"BREAK")==0) {
					__BREAK_light(ae);
				} else if (curcrc==CRC_DEFAULT && strcmp(wordlist[ae->idx].w,"DEFAULT")==0) {
					__DEFAULT_light(ae);
				}
				while (wordlist[ae->idx].t==0) ae->idx++;
				ae->idx++;
				continue;
			} else {
				/* inhibition of switch/case */
				for (curii=0;curii<ae->isw;curii++) {
					if (!ae->switchcase[curii].execute) {
						inhibe=2;
						break;
					}
				}
				if (inhibe) {
					/* this section does NOT need to be agressively optimized !!! */
					if (curcrc==CRC_CASE && strcmp(wordlist[ae->idx].w,"CASE")==0) {
						__CASE(ae);
					} else if (curcrc==CRC_ENDSWITCH && strcmp(wordlist[ae->idx].w,"ENDSWITCH")==0) {
						__ENDSWITCH(ae);
					} else if (curcrc==CRC_IF && strcmp(wordlist[ae->idx].w,"IF")==0) {
						/* as we are inhibited we do not have to truly compute IF */
						__IF_light(ae);
					} else if (curcrc==CRC_IFDEF && strcmp(wordlist[ae->idx].w,"IFDEF")==0) {
						__IFDEF(ae);
					} else if (curcrc==CRC_IFNOT && strcmp(wordlist[ae->idx].w,"IFNOT")==0) {
						__IFNOT(ae);
					} else if (curcrc==CRC_ELSE && strcmp(wordlist[ae->idx].w,"ELSE")==0) {
						__ELSE(ae);
					} else if (curcrc==CRC_ENDIF && strcmp(wordlist[ae->idx].w,"ENDIF")==0) {
						__ENDIF(ae);
					} else if (curcrc==CRC_ELSEIF && strcmp(wordlist[ae->idx].w,"ELSEIF")==0) {
						__ELSEIF(ae);
					} else if (curcrc==CRC_IFUSED && strcmp(wordlist[ae->idx].w,"IFUSED")==0) {
						__IFUSED(ae);
					} else if (curcrc==CRC_IFNUSED && strcmp(wordlist[ae->idx].w,"IFNUSED")==0) {
						__IFNUSED(ae);
					} else if (curcrc==CRC_IFNDEF && strcmp(wordlist[ae->idx].w,"IFNDEF")==0) {
						__IFNDEF(ae);
					} else if (curcrc==CRC_SWITCH && strcmp(wordlist[ae->idx].w,"SWITCH")==0) {
						__SWITCH(ae);
					} else if (curcrc==CRC_BREAK && strcmp(wordlist[ae->idx].w,"BREAK")==0) {
						__BREAK(ae);
					} else if (curcrc==CRC_DEFAULT && strcmp(wordlist[ae->idx].w,"DEFAULT")==0) {
						__DEFAULT(ae);
					}
					while (wordlist[ae->idx].t==0) ae->idx++;
					ae->idx++;
					continue;
				}				
			}
		}
		if (ae->imacropos) {
			/* are we still in a macro? */
			if (ae->idx>=ae->macropos[0].end) {
				/* are we out of all repetition blocks? */
				if (!ae->ir && !ae->iw) {
					ae->imacropos=0;
				}
			}
		}
		/*****************************************
		  e x e c u t e    i n s t r u c t i o n
		*****************************************/
		executed=0;
		if ((ifast=ae->fastmatch[(int)wordlist[ae->idx].w[0]])!=-1) {
			while (instruction[ifast].mnemo[0]==wordlist[ae->idx].w[0]) {
				if (instruction[ifast].crc==curcrc && strcmp(instruction[ifast].mnemo,wordlist[ae->idx].w)==0) {
					instruction[ifast].makemnemo(ae);
					executed=1;
					break;
				}
				ifast++;
			}
		}
		/*****************************************
		       e x e c u t e    m a c r o
		*****************************************/
		if (!executed) {
			/* is it a macro? */
			if ((ifast=SearchMacro(ae,curcrc,wordlist[ae->idx].w))>=0) {
				wordlist=__MACRO_EXECUTE(ae,ifast);
				continue;
			}
		}
		/*********************************************************************
		  e x e c u t e    e x p r e s s i o n   o r    p u s h    l a b e l
		*********************************************************************/
		if (!ae->stop) {
			if (!executed) {
				/* no instruction executed, this is a label or an assignement */
				if (wordlist[ae->idx].e) {
					ExpressionFastTranslate(ae,&wordlist[ae->idx].w,0);
					ComputeExpression(ae,wordlist[ae->idx].w,ae->codeadr,0,0);
				} else {
					PushLabel(ae);
				}
			} else {
				while (!wordlist[ae->idx].t) {
					ae->idx++;
				}
			}
			ae->idx++; 
		} else {
			break;
		}
	}
	if (ae->verbose&4) {
		rasm_printf(ae,"%d [%s] L%d [%s] fin de la liste de mots\n",ae->idx,ae->filename[wordlist[ae->idx].ifile],wordlist[ae->idx].l,wordlist[ae->idx].w);
	}

	if (!ae->stop) {
		/* end of assembly, check there is no opened struct */
		if (ae->getstruct) {
			rasm_printf(ae,"[%s:%d] STRUCT declaration was not closed\n",ae->backup_filename,ae->backup_line);
			MaxError(ae);
		}
		/* end of assembly, close the last ORG zone */
		if (ae->io) {
			ae->orgzone[ae->io-1].memend=ae->outputadr;
		}
		OverWriteCheck(ae);
		/* end of assembly, close crunched zone (if any) */
		__internal_UpdateLZBlockIfAny(ae);
	
		/* end of assembly, check for opened repeat and opened while loop */
		for (i=0;i<ae->ir;i++) {
			rasm_printf(ae,"[%s:%d] REPEAT was not closed\n",ae->filename[wordlist[ae->repeat[i].start].ifile],wordlist[ae->repeat[i].start].l);
			MaxError(ae);
		}
		for (i=0;i<ae->iw;i++) {
			rasm_printf(ae,"[%s:%d] WHILE was not closed\n",ae->filename[wordlist[ae->whilewend[i].start].ifile],wordlist[ae->whilewend[i].start].l);
			MaxError(ae);
		}
		/* is there any IF opened? -> need an evolution for a better error message */
		for (i=0;i<ae->ii;i++) {
			char instr[32];
			switch (ae->ifthen[i].type) {
				case E_IFTHEN_TYPE_IF:strcpy(instr,"IF");break;
				case E_IFTHEN_TYPE_IFNOT:strcpy(instr,"IFNOT");break;
				case E_IFTHEN_TYPE_IFDEF:strcpy(instr,"IFDEF");break;
				case E_IFTHEN_TYPE_IFNDEF:strcpy(instr,"IFNDEF");break;
				case E_IFTHEN_TYPE_ELSE:strcpy(instr,"ELSE");break;
				case E_IFTHEN_TYPE_ELSEIF:strcpy(instr,"ELSEIF");break;
				case E_IFTHEN_TYPE_IFUSED:strcpy(instr,"IFUSED");break;
				case E_IFTHEN_TYPE_IFNUSED:strcpy(instr,"IFNUSED");break;
				default:strcpy(instr,"<unknown>");
			}
			rasm_printf(ae,"[%s:%d] %s conditionnal block was not closed\n",ae->ifthen[i].filename,ae->ifthen[i].line,instr);
			MaxError(ae);
		}
	}
	
	/***************************************************
	         c r u n c h   L Z   s e c t i o n s
	***************************************************/
	if (!ae->stop || !ae->nberr) {
		for (i=0;i<ae->ilz;i++) {
			/* compute labels and expression inside crunched blocks */
			PopAllExpression(ae,i);
			
			ae->curlz=i;
			iorgzone=ae->lzsection[i].iorgzone;
			ibank=ae->lzsection[i].ibank;
			input_data=&ae->mem[ae->lzsection[i].ibank][ae->lzsection[i].memstart];
			input_size=ae->lzsection[i].memend-ae->lzsection[i].memstart;

			switch (ae->lzsection[i].lzversion) {
				case 7:
					#ifndef NO_3RD_PARTIES
					lzdata=ZX7_compress(optimize(input_data, input_size), input_data, input_size, &slzlen);
					lzlen=slzlen;
					#endif
					break;
				case 4:
					#ifndef NO_3RD_PARTIES
					lzdata=LZ4_crunch(input_data,input_size,&lzlen);
					#endif
					break;
				case 8:
					#ifndef NO_3RD_PARTIES
					lzdata=Exomizer_crunch(input_data,input_size,&lzlen);
					#endif
					break;
				case 48:
					lzdata=LZ48_crunch(input_data,input_size,&lzlen);
					break;
				case 49:
					lzdata=LZ49_crunch(input_data,input_size,&lzlen);
					break;
				default:
					rasm_printf(ae,"Internal error - unknown crunch method %d\n",ae->lzsection[i].lzversion);
					exit(-12);
			}
			//rasm_printf(ae,"lzsection[%d] type=%d start=%04X end=%04X crunched size=%d\n",i,ae->lzsection[i].lzversion,ae->lzsection[i].memstart,ae->lzsection[i].memend,lzlen);

			lzshift=lzlen-(ae->lzsection[i].memend-ae->lzsection[i].memstart);
			if (lzshift>0) {
				MemMove(ae->mem[ae->lzsection[i].ibank]+ae->lzsection[i].memend+lzshift,ae->mem[ae->lzsection[i].ibank]+ae->lzsection[i].memend,65536-ae->lzsection[i].memend-lzshift);
			} else if (lzshift<0) {
				lzmove=ae->orgzone[iorgzone].memend-ae->lzsection[i].memend;
				if (lzmove) {
					MemMove(ae->mem[ae->lzsection[i].ibank]+ae->lzsection[i].memend+lzshift,ae->mem[ae->lzsection[i].ibank]+ae->lzsection[i].memend,lzmove);
				}
			}
			memcpy(ae->mem[ae->lzsection[i].ibank]+ae->lzsection[i].memstart,lzdata,lzlen);
			MemFree(lzdata);
			/*******************************************************************
			  l a b e l    a n d    e x p r e s s i o n    r e l o c a t i o n
			*******************************************************************/
			/* relocate labels in the same ORG zone AND after the current crunched section */
			il=ae->lzsection[i].ilabel;
			while (il<ae->il && ae->label[il].iorgzone==iorgzone && ae->label[il].ibank==ibank) {
				curlabel=SearchLabel(ae,ae->label[il].iw!=-1?wordlist[ae->label[il].iw].w:ae->label[il].name,ae->label[il].crc);
				/* CANNOT be NULL */
				curlabel->ptr+=lzshift;
				//printf("label [%s] shifte de %d valeur #%04X -> #%04X\n",curlabel->iw!=-1?wordlist[curlabel->iw].w:curlabel->name,lzshift,curlabel->ptr-lzshift,curlabel->ptr);
				il++;
			}
			/* relocate expressions in the same ORG zone AND after the current crunched section */
			il=ae->lzsection[i].iexpr;
			while (il<ae->ie && ae->expression[il].iorgzone==iorgzone && ae->expression[il].ibank==ibank) {
				ae->expression[il].wptr+=lzshift;
				ae->expression[il].ptr+=lzshift;
				//printf("expression [%s] shiftee ptr=#%04X wptr=#%04X\n", ae->expression[il].reference?ae->expression[il].reference:wordlist[ae->expression[il].iw].w, ae->expression[il].ptr, ae->expression[il].wptr);
				il++;
			}
			/* relocate crunched sections in the same ORG zone AND after the current crunched section */
			il=i+1;
			while (il<ae->ilz && ae->lzsection[il].iorgzone==iorgzone && ae->lzsection[il].ibank==ibank) {
				//rasm_printf(ae,"reloger lzsection[%d] O%d B%d\n",il,ae->lzsection[il].iorgzone,ae->lzsection[il].ibank);
				ae->lzsection[il].memstart+=lzshift;
				ae->lzsection[il].memend+=lzshift;
				il++;
			}
			/* relocate current ORG zone */
			ae->orgzone[iorgzone].memend+=lzshift;
		}
		if (ae->ilz) {
			/* compute expression placed after the last crunched block */
			PopAllExpression(ae,ae->ilz);
		}
		/* compute expression outside crunched blocks */
		PopAllExpression(ae,-1);
	}	

/***************************************************************************************************************************************************************************************
****************************************************************************************************************************************************************************************
      W R I T E      O U T P U T      F I L E S
****************************************************************************************************************************************************************************************
***************************************************************************************************************************************************************************************/
	TMP_filename=MemMalloc(PATH_MAX);
#if 0
for (i=0;i<ae->io;i++) {
printf("ORG[%02d] start=%04X end=%04X ibank=%d nocode=%d protect=%d\n",i,ae->orgzone[i].memstart,ae->orgzone[i].memend,ae->orgzone[i].ibank,ae->orgzone[i].nocode,ae->orgzone[i].protect);
}
#endif

	if (!ae->nberr && !ae->checkmode) {
		
		/* enregistrement des fichiers programmes par la commande SAVE */
		PopAllSave(ae);
	
		if (ae->nbsave==0 || ae->forcecpr || ae->forcesnapshot) {
			/*********************************************
			**********************************************
						  C A R T R I D G E
			**********************************************
			*********************************************/
			if (ae->forcecpr) {
				char ChunkName[32];
				int ChunkSize;
				int do_it=1;
				unsigned char chunk_endian;
				
				if (ae->cartridge_name) {
					sprintf(TMP_filename,"%s",ae->cartridge_name);
				} else {
					sprintf(TMP_filename,"%s.cpr",ae->outputfilename);
				}
				FileRemoveIfExists(TMP_filename);
				
				rasm_printf(ae,"Write cartridge file %s\n",TMP_filename);
				for (i=maxrom=0;i<ae->io;i++) {
					if (ae->orgzone[i].ibank<32 && ae->orgzone[i].ibank>maxrom) maxrom=ae->orgzone[i].ibank;
				}
				/* construction du CPR */
				/* header blablabla */
				strcpy(ChunkName,"RIFF");
				FileWriteBinary(TMP_filename,ChunkName,4);
				ChunkSize=(maxrom+1)*(16384+8)+4;
				chunk_endian=ChunkSize&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
				chunk_endian=(ChunkSize>>8)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
				chunk_endian=(ChunkSize>>16)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
				chunk_endian=(ChunkSize>>24)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
				sprintf(ChunkName,"AMS!");
				FileWriteBinary(TMP_filename,ChunkName,4);
				
//				for (j=0;j<ae->io;j++) {
//printf("ORG[%03d]=B%02d/#%04X/#%04X\n",j,ae->orgzone[j].ibank,ae->orgzone[j].memstart,ae->orgzone[j].memend);
//				}
				for (i=0;i<=maxrom;i++) {
					offset=65536;
					endoffset=0;
					for (j=0;j<ae->io;j++) {
						if (ae->orgzone[j].protect) continue; /* protected zones exclusion */
						/* bank data may start anywhere (typically #0000 or #C000) */
						if (ae->orgzone[j].ibank==i && ae->orgzone[j].memstart!=ae->orgzone[j].memend) {
							if (ae->orgzone[j].memstart<offset) offset=ae->orgzone[j].memstart;
							if (ae->orgzone[j].memend>endoffset) endoffset=ae->orgzone[j].memend;
						}
					}
					if (ae->verbose & 8) {
						if (endoffset>offset) {
							int lm=0;
							if (ae->iwnamebank[i]>0) {
								lm=strlen(ae->wl[ae->iwnamebank[i]].w)-2;
							}
							rasm_printf(ae,"WriteCPR bank %2d of %5d byte%s start at #%04X",i,endoffset-offset,endoffset-offset>1?"s":" ",offset);
							if (endoffset-offset>16384) {
								rasm_printf(ae,"\nROM is too big!!!\n");
								FileWriteBinaryClose(TMP_filename);
								FileRemoveIfExists(TMP_filename);
								FreeAssenv(ae);
								exit(ABORT_ERROR);
							}
							if (lm) {
								rasm_printf(ae," (%-*.*s)\n",lm,lm,ae->wl[ae->iwnamebank[i]].w+1);
							} else {
								rasm_printf(ae,"\n");
							}
						} else {
							rasm_printf(ae,"WriteCPR bank %2d (empty)\n",i);
						}
					}
					ChunkSize=16384;
					sprintf(ChunkName,"cb%02d",i);
					FileWriteBinary(TMP_filename,ChunkName,4);
					chunk_endian=ChunkSize&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
					chunk_endian=(ChunkSize>>8)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
					chunk_endian=(ChunkSize>>16)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
					chunk_endian=(ChunkSize>>24)&0xFF;FileWriteBinary(TMP_filename,(char*)&chunk_endian,1);
					if (offset>0xC000) {
						unsigned char filler[16384]={0};
						ChunkSize=65536-offset;
						if (ChunkSize) FileWriteBinary(TMP_filename,(char*)ae->mem[i]+offset,ChunkSize);
						/* ADD zeros until the end of the bank */
						FileWriteBinary(TMP_filename,(char*)filler,16384-ChunkSize);
					} else {
						FileWriteBinary(TMP_filename,(char*)ae->mem[i]+offset,ChunkSize);
					}
				}
				FileWriteBinaryClose(TMP_filename);
				rasm_printf(ae,"Total %d bank%s (%dK)\n",maxrom+1,maxrom+1>1?"s":"",(maxrom+1)*16);
			/*********************************************
			**********************************************
						  S N A P S H O T  
			**********************************************
			*********************************************/
			} else if (ae->forcesnapshot) {
				unsigned char packed[65536]={0};
				unsigned char *rlebank=NULL;
				char ChunkName[5];
				int ChunkSize;
				int do_it=1;
				int bankset;

				if (ae->snapshot.version==2 && ae->snapshot.CPCType>2) {
					if (!ae->nowarning) rasm_printf(ae,"[%s:%d] Warning: V2 snapshot cannot select a Plus model (forced to 6128)\n",GetCurrentFile(ae),ae->wl[ae->idx].l);
					ae->snapshot.CPCType=2; /* 6128 */
				}
				
				if (ae->snapshot_name) {
					sprintf(TMP_filename,"%s",ae->snapshot_name);
				} else {
					sprintf(TMP_filename,"%s.sna",ae->outputfilename);
				}
				FileRemoveIfExists(TMP_filename);
			
				maxrom=-1;	
				for (i=0;i<ae->io;i++) {
					if (ae->orgzone[i].ibank<36 && ae->orgzone[i].ibank>maxrom && ae->orgzone[i].memstart!=ae->orgzone[i].memend) {
						maxrom=ae->orgzone[i].ibank;
					}
				}
				/* construction du SNA */
				if (ae->snapshot.version==2) {
					if (maxrom>=4) {
						ae->snapshot.dumpsize[0]=128;
					} else if (maxrom>=0) {
						ae->snapshot.dumpsize[0]=64;
					}
				}
				if (maxrom==-1) {
					rasm_printf(ae,"Warning: No byte were written in snapshot memory\n");
				} else {
					rasm_printf(ae,"Write snapshot v%d file %s\n",ae->snapshot.version,TMP_filename);
					
					/* header */
					FileWriteBinary(TMP_filename,(char *)&ae->snapshot,0x100);
					/* write all memory crunched */
					for (i=0;i<=maxrom;i+=4) {
						bankset=i/4;
						if (ae->bankset[bankset]) {
							memcpy(packed,ae->mem[i],65536);
							if (ae->verbose & 8) {
								rasm_printf(ae,"WriteSNA bank %2d,%d,%d,%d packed\n",i,i+1,i+2,i+3);
							}
						} else {
							memset(packed,0,65536);
							for (k=0;k<4;k++) {
								offset=65536;
								endoffset=0;
								for (j=0;j<ae->io;j++) {
									if (ae->orgzone[j].protect) continue; /* protected zones exclusion */
									/* bank data may start anywhere (typically #0000 or #C000) */
									if (ae->orgzone[j].ibank==i+k && ae->orgzone[j].memstart!=ae->orgzone[j].memend) {
										if (ae->orgzone[j].memstart<offset) offset=ae->orgzone[j].memstart;
										if (ae->orgzone[j].memend>endoffset) endoffset=ae->orgzone[j].memend;
									}
								}
								if (endoffset-offset>16384) {
									rasm_printf(ae,"\nBANK is too big!!!\n");
									FileWriteBinaryClose(TMP_filename);
									FileRemoveIfExists(TMP_filename);
									FreeAssenv(ae);
									exit(ABORT_ERROR);
								}
								/* banks are gathered in the 64K block */
								if (offset>0xC000) {
									ChunkSize=65536-offset;
									memcpy(packed+k*16384,(char*)ae->mem[i+k]+offset,ChunkSize);
								} else {
									memcpy(packed+k*16384,(char*)ae->mem[i+k]+offset,16384);
								}
								
								if (ae->verbose & 8) {
									if (endoffset>offset) {
										int lm=0;
										if (ae->iwnamebank[i]>0) {
											lm=strlen(ae->wl[ae->iwnamebank[i]].w)-2;
										}
										rasm_printf(ae,"WriteSNA bank %2d of %5d byte%s start at #%04X",i+k,endoffset-offset,endoffset-offset>1?"s":" ",offset);
										if (endoffset-offset>16384) {
											rasm_printf(ae,"\nRAM block is too big!!!\n");
											FileWriteBinaryClose(TMP_filename);
											FileRemoveIfExists(TMP_filename);
											FreeAssenv(ae);
											exit(ABORT_ERROR);
										}
										if (lm) {
											rasm_printf(ae," (%-*.*s)\n",lm,lm,ae->wl[ae->iwnamebank[i+k]].w+1);
										} else {
											rasm_printf(ae,"\n");
										}
									} else {
										rasm_printf(ae,"WriteSNA bank %2d (empty)\n",i+k);
									}
								}
								
							}
						}
						
						if (ae->snapshot.version==2) {
							/* snapshot v2 */
							FileWriteBinary(TMP_filename,(char*)&packed,65536);
							if (bankset) {
								/* v2 snapshot is 128K maximum */
								maxrom=7;
								break;
							}
						} else {
							/* compression par d�faut avec snapshot v3 */
							rlebank=EncodeSnapshotRLE(packed,&ChunkSize);
							sprintf(ChunkName,"MEM%d",bankset);
							FileWriteBinary(TMP_filename,ChunkName,4);
							if (rlebank!=NULL) {
								FileWriteBinary(TMP_filename,(char*)&ChunkSize,4);
								FileWriteBinary(TMP_filename,(char*)rlebank,ChunkSize);
								MemFree(rlebank);
							} else {
								ChunkSize=65536;
								FileWriteBinary(TMP_filename,(char*)&packed,ChunkSize);
							}
						}
					}

					/**************************************************************
						    snapshot additional chunks in v3+ only
					**************************************************************/
					if (ae->snapshot.version>=3) {
printf("snabrk=%d\n",ae->export_snabrk);
						/* export breakpoint */
						if (ae->export_snabrk) {
							/* BRKS chunk for Winape emulator (unofficial) 
							
							2 bytes - adress
							1 byte  - 0=base 64K / 1=extended
							2 bytes - condition (zeroed)
							*/
							struct s_breakpoint breakpoint={0};
							unsigned char *brkschunk=NULL;
							unsigned int idx=8;
							
							/* add labels and local labels to breakpoint pool (if any) */
							for (i=0;i<ae->il;i++) {
								if (!ae->label[i].name) {
									if (strncmp(ae->wl[ae->label[i].iw].w,"BRK",3)==0) {
										breakpoint.address=ae->label[i].ptr;
										if (ae->label[i].ibank>3) breakpoint.bank=1; else breakpoint.bank=0;
										ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));
									}
								} else {
									if (strncmp(ae->label[i].name,"@BRK",4)==0 || strstr(ae->label[i].name,".BRK")) {
										breakpoint.address=ae->label[i].ptr;
										if (ae->label[i].ibank>3) breakpoint.bank=1; else breakpoint.bank=0;
										ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));								
									}
								}
							}

							brkschunk=MemMalloc(ae->ibreakpoint*5+8);
							strcpy((char *)brkschunk,"BRKS");
							
							for (i=0;i<ae->ibreakpoint;i++) {
								brkschunk[idx++]=ae->breakpoint[i].address&0xFF;
								brkschunk[idx++]=(ae->breakpoint[i].address&0xFF00)/256;
								brkschunk[idx++]=ae->breakpoint[i].bank;
								brkschunk[idx++]=0;
								brkschunk[idx++]=0;
							}

							idx-=8;
							brkschunk[4]=idx&0xFF;
							brkschunk[5]=(idx>>8)&0xFF;
							brkschunk[6]=(idx>>16)&0xFF;
							brkschunk[7]=(idx>>24)&0xFF;
							FileWriteBinary(TMP_filename,(char*)brkschunk,idx+8); // 8 bytes for the chunk header
							MemFree(brkschunk);


							/* BRKC chunk for ACE emulator 
							minimal integration
							*/
							brkschunk=MemMalloc(ae->ibreakpoint*256);
							strcpy((char *)brkschunk,"BRKC");
							idx=8;
							
							for (i=0;i<ae->ibreakpoint;i++) {
								brkschunk[idx++]=0; /* 0:Execution */
								brkschunk[idx++]=0;
								brkschunk[idx++]=0;
								brkschunk[idx++]=0;
								brkschunk[idx++]=ae->breakpoint[i].address&0xFF;
								brkschunk[idx++]=(ae->breakpoint[i].address&0xFF00)/256;
								for (j=0;j<2+1+1+2+4+128;j++) {
									brkschunk[idx++]=0;
								}
								sprintf((char *)brkschunk+idx,"breakpoint%d",i); /* breakpoint user name? */
								idx+=64+8;
							}
							idx-=8;
							brkschunk[4]=idx&0xFF;
							brkschunk[5]=(idx>>8)&0xFF;
							brkschunk[6]=(idx>>16)&0xFF;
							brkschunk[7]=(idx>>24)&0xFF;
							FileWriteBinary(TMP_filename,(char *)brkschunk,idx+8); // 8 bytes for the chunk header
							MemFree(brkschunk);
						}
						/* export optionnel des symboles */
						if (ae->export_sna) {
							/* SYMB chunk for ACE emulator

							1 byte  - name size
							n bytes - name (without 0 to end the string)
							6 bytes - reserved for future use
							2 bytes - shitty big endian adress for the symbol
							*/
						
							unsigned char *symbchunk=NULL;
							unsigned int idx=8;
							int symbol_len;

							symbchunk=MemMalloc(8+ae->il*(1+255+6+2));
							strcpy((char *)symbchunk,"SYMB");

							for (i=0;i<ae->il;i++) {
								if (!ae->label[i].name) {
									symbol_len=strlen(ae->wl[ae->label[i].iw].w);
									if (symbol_len>255) symbol_len=255;
									symbchunk[idx++]=symbol_len;
									memcpy(symbchunk+idx,ae->wl[ae->label[i].iw].w,symbol_len);
									idx+=symbol_len;
									memset(symbchunk+idx,0,6);
									idx+=6;
									symbchunk[idx++]=(ae->label[i].ptr&0xFF00)/256;
									symbchunk[idx++]=ae->label[i].ptr&0xFF;
								} else {
									if (ae->export_local) {
										symbol_len=strlen(ae->label[i].name);
										if (symbol_len>255) symbol_len=255;
										symbchunk[idx++]=symbol_len;
										memcpy(symbchunk+idx,ae->label[i].name,symbol_len);
										idx+=symbol_len;
										memset(symbchunk+idx,0,6);
										idx+=6;
										symbchunk[idx++]=(ae->label[i].ptr&0xFF00)/256;
										symbchunk[idx++]=ae->label[i].ptr&0xFF;
									}
								}
							}
							if (ae->export_var) {
								unsigned char *subchunk=NULL;
								int retidx=0;
								/* var are part of fast tree search structure */
								subchunk=SnapshotDicoTree(ae,&retidx);
								if (retidx) {
									symbchunk=MemRealloc(symbchunk,idx+retidx);
									memcpy(symbchunk+idx,subchunk,retidx);
									idx+=retidx;
									SnapshotDicoInsert("FREE",0,&retidx);
								}
							}
							if (ae->export_equ) {
								symbchunk=MemRealloc(symbchunk,idx+ae->ialias*(1+255+6+2));

								for (i=0;i<ae->ialias;i++) {
									int tmpptr;
									symbol_len=strlen(ae->alias[i].alias);
									if (symbol_len>255) symbol_len=255;
									symbchunk[idx++]=symbol_len;
									memcpy(symbchunk+idx,ae->alias[i].alias,symbol_len);
									idx+=symbol_len;
									memset(symbchunk+idx,0,6);
									idx+=6;
									tmpptr=RoundComputeExpression(ae,ae->alias[i].translation,0,0,0);
									symbchunk[idx++]=(tmpptr&0xFF00)/256;
									symbchunk[idx++]=tmpptr&0xFF;
								}
							}
							idx-=8;
							symbchunk[4]=idx&0xFF;
							symbchunk[5]=(idx>>8)&0xFF;
							symbchunk[6]=(idx>>16)&0xFF;
							symbchunk[7]=(idx>>24)&0xFF;
							FileWriteBinary(TMP_filename,(char*)symbchunk,idx+8); // 8 bytes for the chunk header
						}
					} else {
						if (ae->export_snabrk) {
							if (!ae->nowarning) rasm_printf(ae,"Warning: breakpoint export is not supported with snapshot version 2\n");
						}
						if (ae->export_sna) {
							if (!ae->nowarning) rasm_printf(ae,"Warning: symbol export is not supported with snapshot version 2\n");
						}
					}

					FileWriteBinaryClose(TMP_filename);
					maxrom=(maxrom>>2)*4+4;
					rasm_printf(ae,"Total %d bank%s (%dK)\n",maxrom,maxrom>1?"s":"",(maxrom)*16);
				}
			/*********************************************
			**********************************************
					  B I N A R Y     F I L E
			**********************************************
			*********************************************/
			} else {
				int lastspaceid=-1;
				
				if (ae->binary_name) {
					sprintf(TMP_filename,"%s",ae->binary_name);
				} else {
					sprintf(TMP_filename,"%s.bin",ae->outputfilename);
				}
				FileRemoveIfExists(TMP_filename);

				/* en mode binaire classique on va recherche le dernier espace m�moire dans lequel on a travaill� qui n'est pas en 'nocode' */
				for (i=0;i<ae->io;i++) {
					/* uniquement si le ORG a ete suivi d'ecriture */
					if (ae->orgzone[i].memstart!=ae->orgzone[i].memend && ae->orgzone[i].nocode!=1) {
						lastspaceid=ae->orgzone[i].ibank;
					}
				}
				if (lastspaceid!=-1) {
					for (i=0;i<ae->io;i++) {
						if (ae->orgzone[i].protect) continue; /* protected zones exclusion */
						/* uniquement si le ORG a ete suivi d'ecriture et n'est pas en 'nocode' */
						if (ae->orgzone[i].ibank==lastspaceid && ae->orgzone[i].memstart!=ae->orgzone[i].memend && ae->orgzone[i].nocode!=1) {
							if (ae->orgzone[i].memstart<minmem) minmem=ae->orgzone[i].memstart;
							if (ae->orgzone[i].memend>maxmem) maxmem=ae->orgzone[i].memend;
						}
					}
				}
				if (maxmem-minmem<=0) {
					if (!ae->stop) {
						if (!ae->nowarning) rasm_printf(ae,"Warning: Not a single byte to output\n");
					}
					if (ae->flux) {
						*lenout=0;
					}
				} else {
					if (!ae->flux) {
						rasm_printf(ae,"Write binary file %s (%d byte%s)\n",TMP_filename,maxmem-minmem,maxmem-minmem>1?"s":"");
						if (ae->amsdos) {
							AmsdosHeader=MakeAMSDOSHeader(minmem,maxmem,TMP_filename);
							FileWriteBinary(TMP_filename,(char *)AmsdosHeader,128);
						}
						if (maxmem-minmem>0) {
							FileWriteBinary(TMP_filename,(char*)ae->mem[lastspaceid]+minmem,maxmem-minmem);
							FileWriteBinaryClose(TMP_filename);
						} else {
							if (ae->amsdos) {
								FileWriteBinaryClose(TMP_filename);
							}
						}
					} else {
						*dataout=MemMalloc(maxmem-minmem+1);
						memcpy(*dataout,ae->mem[lastspaceid]+minmem,maxmem-minmem);
						*lenout=maxmem-minmem;
					}
				}
			}
		}
		/****************************
		*****************************
		  S Y M B O L   E X P O R T
		*****************************
		****************************/
		if (ae->export_sym && !ae->export_sna) {
			if (ae->symbol_name) {
				sprintf(TMP_filename,"%s",ae->symbol_name);
			} else {
				sprintf(TMP_filename,"%s.sym",ae->outputfilename);
			}		
			FileRemoveIfExists(TMP_filename);
			rasm_printf(ae,"Write symbol file %s\n",TMP_filename);
			switch (ae->export_sym) {
				case 4:
					/* flexible */
					for (i=0;i<ae->il;i++) {
						if (!ae->label[i].name) {
							sprintf(symbol_line,ae->flexible_export,ae->wl[ae->label[i].iw].w,ae->label[i].ptr);
							FileWriteLine(TMP_filename,symbol_line);
						} else {
							if (ae->export_local) {
								sprintf(symbol_line,ae->flexible_export,ae->label[i].name,ae->label[i].ptr);
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					if (ae->export_var) {
						/* var are part of fast tree search structure */
						ExportDicoTree(ae,TMP_filename,ae->flexible_export);
					}
					if (ae->export_equ) {
						for (i=0;i<ae->ialias;i++) {
							if (strcmp(ae->alias[i].alias,"IX") && strcmp(ae->alias[i].alias,"IY")) {
								sprintf(symbol_line,ae->flexible_export,ae->alias[i].alias,RoundComputeExpression(ae,ae->alias[i].translation,0,0,0));
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					FileWriteLineClose(TMP_filename);
					break;
				case 3:
					/* winape */
					for (i=0;i<ae->il;i++) {
						if (!ae->label[i].name) {
							sprintf(symbol_line,"%s #%04X\n",ae->wl[ae->label[i].iw].w,ae->label[i].ptr);
							FileWriteLine(TMP_filename,symbol_line);
						} else {
							if (ae->export_local) {
								sprintf(symbol_line,"%s #%04X\n",ae->label[i].name,ae->label[i].ptr);
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					if (ae->export_var) {
						/* var are part of fast tree search structure */
						ExportDicoTree(ae,TMP_filename,"%s #%04X\n");
					}
					if (ae->export_equ) {
						for (i=0;i<ae->ialias;i++) {
							if (strcmp(ae->alias[i].alias,"IX") && strcmp(ae->alias[i].alias,"IY")) {
								sprintf(symbol_line,"%s #%04X\n",ae->alias[i].alias,RoundComputeExpression(ae,ae->alias[i].translation,0,0,0));
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					FileWriteLineClose(TMP_filename);
					break;
				case 2:
					/* pasmo */
					for (i=0;i<ae->il;i++) {
						if (!ae->label[i].name) {
							sprintf(symbol_line,"%s EQU 0%04XH\n",ae->wl[ae->label[i].iw].w,ae->label[i].ptr);
							FileWriteLine(TMP_filename,symbol_line);
						} else {
							if (ae->export_local) {
								sprintf(symbol_line,"%s EQU 0%04XH\n",ae->label[i].name,ae->label[i].ptr);
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					if (ae->export_var) {
						/* var are part of fast tree search structure */
						ExportDicoTree(ae,TMP_filename,"%s EQU 0%04XH\n");
					}
					if (ae->export_equ) {
						for (i=0;i<ae->ialias;i++) {
							if (strcmp(ae->alias[i].alias,"IX") && strcmp(ae->alias[i].alias,"IY")) {
								sprintf(symbol_line,"%s EQU 0%04XH\n",ae->alias[i].alias,RoundComputeExpression(ae,ae->alias[i].translation,0,0,0));
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					FileWriteLineClose(TMP_filename);
					break;
				case 1:
					/* Rasm */
					for (i=0;i<ae->il;i++) {
						if (!ae->label[i].name) {
							sprintf(symbol_line,"%s #%X B%d\n",ae->wl[ae->label[i].iw].w,ae->label[i].ptr,ae->label[i].ibank>31?0:ae->label[i].ibank);
							FileWriteLine(TMP_filename,symbol_line);
						} else {
							if (ae->export_local) {
								sprintf(symbol_line,"%s #%X B%d\n",ae->label[i].name,ae->label[i].ptr,ae->label[i].ibank>31?0:ae->label[i].ibank);
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					if (ae->export_var) {
						/* var are part of fast tree search structure */
						ExportDicoTree(ae,TMP_filename,"%s #%X B0\n");
					}
					if (ae->export_equ) {
						for (i=0;i<ae->ialias;i++) {
							if (strcmp(ae->alias[i].alias,"IX") && strcmp(ae->alias[i].alias,"IY")) {
								sprintf(symbol_line,"%s #%X B0\n",ae->alias[i].alias,RoundComputeExpression(ae,ae->alias[i].translation,0,0,0));
								FileWriteLine(TMP_filename,symbol_line);
							}
						}
					}
					FileWriteLineClose(TMP_filename);
					break;
				case 0:
				default:break;	
			}
		}
		/*********************************
		**********************************
			   B R E A K P O I N T S
		**********************************
		*********************************/
		if (ae->export_brk) {
			struct s_breakpoint breakpoint={0};
			
			if (ae->breakpoint_name) {
				sprintf(TMP_filename,"%s",ae->breakpoint_name);
			} else {
				sprintf(TMP_filename,"%s.brk",ae->outputfilename);
			}		
			FileRemoveIfExists(TMP_filename);

			/* add labels and local labels to breakpoint pool (if any) */
			for (i=0;i<ae->il;i++) {
				if (!ae->label[i].name) {
					if (strncmp(ae->wl[ae->label[i].iw].w,"BRK",3)==0) {
						breakpoint.address=ae->label[i].ptr;
						if (ae->label[i].ibank>3) breakpoint.bank=1; else breakpoint.bank=0;
						ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));
					}
				} else {
					if (strncmp(ae->label[i].name,"@BRK",4)==0) {
						breakpoint.address=ae->label[i].ptr;
						if (ae->label[i].ibank>3) breakpoint.bank=1; else breakpoint.bank=0;
						ObjectArrayAddDynamicValueConcat((void **)&ae->breakpoint,&ae->ibreakpoint,&ae->maxbreakpoint,&breakpoint,sizeof(struct s_breakpoint));								
					}
				}
			}

			if (ae->ibreakpoint) {
				rasm_printf(ae,"Write breakpoint file %s\n",TMP_filename);
				for (i=0;i<ae->ibreakpoint;i++) {
					sprintf(symbol_line,"#%04X\n",ae->breakpoint[i].address);
					FileWriteLine(TMP_filename,symbol_line);
				}
				FileWriteLineClose(TMP_filename);
			} else {
				rasm_printf(ae,"no breakpoint to output (previous file [%s] deleted anyway)\n",TMP_filename);
			}
		}

	} else {
		if (!ae->dependencies) rasm_printf(ae,"%d error%s\n",ae->nberr,ae->nberr>1?"s":"");
		minmem=65536;
		maxmem=0;
		for (i=0;i<ae->io;i++) {
			/* uniquement si le ORG a ete suivi d'ecriture */
			if (ae->orgzone[i].memstart!=ae->orgzone[i].memend) {
				if (ae->orgzone[i].memstart<minmem) minmem=ae->orgzone[i].memstart;
				if (ae->orgzone[i].memend>maxmem) maxmem=ae->orgzone[i].memend;
			}
		}
	}
/*******************************************************************************************
                        E X P O R T     D E P E N D E N C I E S
*******************************************************************************************/
	if (ae->dependencies) {
		int trigdep=0;
		
		/* depends ALL */
		if (ae->outputfilename && strcmp(ae->outputfilename,"rasmoutput")) {
			trigdep=1;
			printf("%s",ae->outputfilename);
			if (ae->dependencies==E_DEPENDENCIES_MAKE) printf(" "); else printf("\n");
		}
		for (i=1;i<ae->ifile;i++) {
			trigdep=1;
			SimplifyPath(ae->filename[i]);
			printf("%s",ae->filename[i]);
			if (ae->dependencies==E_DEPENDENCIES_MAKE) printf(" "); else printf("\n");
		}
		for (i=0;i<ae->ih;i++) {
			trigdep=1;
			SimplifyPath(ae->hexbin[i].filename);
			printf("%s",ae->hexbin[i].filename);
			if (ae->dependencies==E_DEPENDENCIES_MAKE) printf(" "); else printf("\n");
		}
		if (ae->dependencies==E_DEPENDENCIES_MAKE && trigdep) printf("\n");
	}

/*******************************************************************************************
                           V E R B O S E     S H I T
*******************************************************************************************/
	if (ae->verbose & 7) {
		rasm_printf(ae,"------ statistics ------------------\n");
		rasm_printf(ae,"%d file%s\n",ae->ifile,ae->ifile>1?"s":"");
		rasm_printf(ae,"%d binary include%s\n",ae->ih,ae->ih>1?"s":"");
		rasm_printf(ae,"%d word%s\n",ae->nbword-1,ae->nbword>2?"s":"");
		rasm_printf(ae,"%d label%s\n",ae->il,ae->il>1?"s":"");
		rasm_printf(ae,"%d struct%s\n",ae->irasmstruct,ae->irasmstruct>1?"s":"");
		rasm_printf(ae,"%d var%s\n",ae->idic,ae->idic>1?"s":"");
		rasm_printf(ae,"%d expression%s\n",ae->ie,ae->ie>1?"s":"");
		rasm_printf(ae,"%d macro%s\n",ae->imacro,ae->imacro>1?"s":"");
		rasm_printf(ae,"%d alias%s\n",ae->ialias,ae->ialias>1?"s":"");
		rasm_printf(ae,"%d ORG zone%s\n",ae->io-1,ae->io>2?"s":"");
		rasm_printf(ae,"%d virtual space%s\n",ae->nbbank,ae->nbbank>1?"s":"");
	}

/*******************************************************************************************
                                  C L E A N U P
*******************************************************************************************/
	if (TMP_filename) MemFree(TMP_filename);
	if (ae->nberr) {
		ok=-1;
		if (ae->flux && *dataout) {
			MemFree(*dataout);
			*dataout=NULL;
		}
		if (lenout) *lenout=0;
	} else {
		ok=0;
	}

	FreeAssenv(ae);

	return ok;
}


void StateMachineRemoveComz(char *Abuf) {
	#undef FUNC
	#define FUNC "StateMachineRemoveComz"
printf("StateMachineRemoveComz deprecated!\n");
exit(1);
	int i=0;
	while (Abuf[i]) {
		if (Abuf[i]==';' || (Abuf[i]=='/' && Abuf[i+1]=='/')) {
			while (Abuf[i] && Abuf[i]!=0x0D && Abuf[i]!=0x0A) Abuf[i++]=':';
			i--;
		} else if (Abuf[i]=='"') {
			while (Abuf[i] && Abuf[i]!='"') {
				i++;
			}
		} else if (Abuf[i]=='\'' && i>2 && !(toupper(Abuf[i-2])=='A' && toupper(Abuf[i-1])=='F')) {
			while (Abuf[i] && Abuf[i]!='\'') {
				i++;
			}
		}
		i++;
	}
	if (i && Abuf[i-1]==0x0A) Abuf[i-1]=':';
	//strcat(Abuf,":");
}

void EarlyPrepSrc(struct s_assenv *ae, char **listing, char *filename) {
	int l,idx,c,quote_type=0;
	int mlc_start,mlc_idx;

	/* virer les commentaires en ;, // mais aussi multi-lignes et convertir les decalages, passer les chars en upper case */
	l=idx=0;
	while (listing[l]) {
		c=listing[l][idx++];
		if (!c) {
			l++;
			idx=0;
			continue;
		} else if (!quote_type) {
			/* upper case */
			if (c>='a' && c<='z') listing[l][idx-1]=c=c-'a'+'A';

			if (c=='\'' && idx>2 && strncmp(&listing[l][idx-3],"AF'",3)==0) {
				/* il ne faut rien faire */
			} else if (c=='"' || c=='\'') {
				quote_type=c;
			} else if (c==';' || (c=='/' && listing[l][idx]=='/')) {
				idx--;
				while (listing[l][idx] && listing[l][idx]!=0x0D && listing[l][idx]!=0x0A) listing[l][idx++]=':';
				idx--;
			} else if (c=='>' && listing[l][idx]=='>' && !quote_type) {
				listing[l][idx-1]=']';
				listing[l][idx++]=' ';
				continue;
			} else if (c=='<' && listing[l][idx]=='<' && !quote_type) {
				listing[l][idx-1]='[';
				listing[l][idx++]=' ';
				continue;
			} else if (c=='/' && listing[l][idx]=='*' && !quote_type) {
				/* multi-line comment */
				mlc_start=l;
				mlc_idx=idx-1;
				idx++;
				while (1) {
					c=listing[l][idx++];
					if (!c) {
						idx=0;
						l++;
						if (!listing[l]) {
							rasm_printf(ae,"[%s:%d] opened comment to the end of the file\n",filename,l+1);
							return;
						}
					} else if (c=='*' && listing[l][idx]=='/') {
						idx++;
						break;
					}
				}
				/* merge */
				if (mlc_start==l) {
					while (mlc_idx<idx) listing[l][mlc_idx++]=' '; /* raz with spaces */
				} else {
					listing[mlc_start][mlc_idx]=0; /* raz EOL */
					mlc_start++;
					while (mlc_start<l) listing[mlc_start++][0]=0; /* raz line */
					mlc_idx=0;
					while (mlc_idx<idx) listing[l][mlc_idx++]=' '; /* raz beginning of the line */
				}
				idx++;
			}
		} else {
			/* in quote */
			if (c=='\\') {
				idx++;
			} else if (c==quote_type) {
				quote_type=0;
			}
		}
	}
}

void PreProcessingSplitListing(struct s_listing **listing, int *il, int *ml, int idx, int end, int start)
{
	#undef FUNC
	#define FUNC "PreProcessingSplitListing"
	
	struct s_listing curlisting={0};
	
	ObjectArrayAddDynamicValueConcat((void**)listing,il,ml,&curlisting,sizeof(curlisting));
	MemMove(&((*listing)[idx+2]),&((*listing)[idx+1]),(*il-idx-2)*sizeof(struct s_listing));
	(*listing)[idx+1].ifile=(*listing)[idx].ifile;
	(*listing)[idx+1].iline=(*listing)[idx].iline;
	if ((*listing)[idx].listing[start]) {
		(*listing)[idx+1].listing=TxtStrDup((*listing)[idx].listing+start);
	} else {
		(*listing)[idx+1].listing=TxtStrDup(";");
	}
	strcpy((*listing)[idx].listing+end,":");
}

void PreProcessingInsertListing(struct s_listing **reflisting, int *il, int *ml, int idx, char **zelines, int ifile)
{
	#undef FUNC
	#define FUNC "PreProcessingSplitListing"
	
	struct s_listing *listing;
	int nbinsert,li,bil;
	for (li=nbinsert=0;zelines[li];li++) nbinsert++;
	bil=*il;
	if (*il+nbinsert>=*ml) {
		*il=*ml=*il+nbinsert;
		*reflisting=MemRealloc(*reflisting,sizeof(struct s_listing)*(*ml));
	} else {
		*il=*il+nbinsert;
	}
	listing=*reflisting;
	MemMove(&listing[idx+1+nbinsert],&listing[idx+1],(bil-idx-1)*sizeof(struct s_listing));
	
	for (li=0;zelines[li];li++) {
		listing[idx+1+li].ifile=ifile;
		listing[idx+1+li].iline=li+1;
		listing[idx+1+li].listing=zelines[li];
	}
}

int cmpkeyword(const void * a, const void * b)
{
	struct s_asm_keyword *sa,*sb;
	sa=(struct s_asm_keyword *)a;
	sb=(struct s_asm_keyword *)b;
	return strcmp(sa->mnemo,sb->mnemo);
}

struct s_assenv *PreProcessing(char *filename, int flux, const char *datain, int datalen, struct s_parameter *param)
{
	#undef FUNC
	#define FUNC "PreProcessing"

	#define CharWord "@ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.=_($)][+-*/^%#|&'\"\\m}{[]"

	struct s_assenv *ae=NULL;
	struct s_wordlist curw={0};
	struct s_wordlist *wordlist=NULL;
	int nbword=0,maxword=0;
	char **zelines=NULL;

	char *filename_toread;
	
	struct s_macro_fast *MacroFast=NULL;
	int idxmacrofast=0,maxmacrofast=0;
	
	struct s_listing *listing=NULL;
	struct s_listing curlisting;
	int ilisting=0,maxlisting=0;
	
	char **listing_include=NULL;
	int i,j,l=0,idx=0,c=0,li,le;
	char Automate[256]={0};
	struct s_hexbin curhexbin;
	char *newlistingline=NULL;
	unsigned char *newdata;
	struct s_label curlabel={0};
	char *labelsep1;
	char **labelines=NULL;
	/* state machine buffer */
	unsigned char *mem=NULL;
	char *w=NULL,*wtmp=NULL;
	int lw=0,mw=256;
	char *bval=NULL;
	int ival=0,sval=256;
	char *qval=NULL;
	int iqval=0,sqval=256;
	struct s_repeat_index *TABrindex=NULL;
	struct s_repeat_index *TABwindex=NULL;
	struct s_repeat_index rindex={0};
	struct s_repeat_index windex={0};
	int nri=0,mri=0,ri=0;
	int nwi=0,mwi=0,wi=0;
	/* state machine trigger */
	int waiting_quote=0,lquote;
	int macro_trigger=0;
	int escape_code=0;
	int quote_type=0;
	int incbin=0,include=0,crunch=0;
	int rewrite=0,hadcomma=0;
	int nbinstruction;
	int ifast,texpr;
	int ispace=0;

#if DEBUG_PREPRO
printf("start prepro, alloc assenv\n");
#endif

	windex.cl=-1;
	windex.cidx=-1;
	rindex.cl=-1;
	rindex.cidx=-1;

#if DEBUG_PREPRO
printf("malloc\n");
#endif
	ae=MemMalloc(sizeof(struct s_assenv));
#if DEBUG_PREPRO
printf("memset\n");
#endif
	memset(ae,0,sizeof(struct s_assenv));

#if DEBUG_PREPRO
printf("paramz 1\n");
#endif
	if (param) {
		ae->export_local=param->export_local;
		ae->export_sym=param->export_sym;
		ae->export_var=param->export_var;
		ae->export_equ=param->export_equ;
		ae->export_sna=param->export_sna;
		ae->export_snabrk=param->export_snabrk;
		if (param->export_sna || param->export_snabrk) {
			ae->forcesnapshot=1;
		}
		ae->export_brk=param->export_brk;
		ae->edskoverwrite=param->edskoverwrite;
		ae->rough=param->rough;
		ae->as80=param->as80;
		ae->dams=param->dams;
		if (param->v2) {
			ae->forcesnapshot=1;
			ae->snapshot.version=2;
		} else {
			ae->snapshot.version=3;
		}
		ae->maxerr=param->maxerr;
		ae->extended_error=param->extended_error;
		ae->nowarning=param->nowarning;
		ae->breakpoint_name=param->breakpoint_name;
		ae->symbol_name=param->symbol_name;
		ae->binary_name=param->binary_name;
		ae->flexible_export=param->flexible_export;
		ae->cartridge_name=param->cartridge_name;
		ae->snapshot_name=param->snapshot_name;
		ae->checkmode=param->checkmode;
		if (param->rough) ae->maxam=0; else ae->maxam=1;
		/* additional symbols */
		for (i=0;i<param->nsymb;i++) {
			char *sep;
			sep=strchr(param->symboldef[i],'=');
			if (sep) {
				*sep=0;
				ExpressionSetDicoVar(ae,param->symboldef[i],atof(sep+1));
			}
		}
		if (param->msymb) {
			MemFree(param->symboldef);
			param->nsymb=param->msymb=0;
		}
		/* include paths */
		ae->includepath=param->pathdef;
		ae->ipath=param->npath;
		ae->mpath=param->mpath;
		/* old inline params */
		ae->dependencies=param->dependencies;
		ae->verbose=param->verbose;
	}
#if DEBUG_PREPRO
printf("init 0\n");
#endif
	/* generic init */
	ae->ctx1.maxivar=1;
	ae->ctx2.maxivar=1;
	ae->computectx=&ae->ctx1;
	ae->flux=flux;
	/* check snapshot structure */
	if (sizeof(ae->snapshot)!=0x100 || &ae->snapshot.fdd.motorstate-(unsigned char*)&ae->snapshot!=0x9C || &ae->snapshot.crtcstate.model-(unsigned char*)&ae->snapshot!=0xA4
		|| &ae->snapshot.romselect-(unsigned char*)&ae->snapshot!=0x55
		|| &ae->snapshot.interruptrequestflag-(unsigned char*)&ae->snapshot!=0xB4
		|| &ae->snapshot.CPCType-(unsigned char*)&ae->snapshot!=0x6D) {
		rasm_printf(ae,"snapshot structure integrity check KO\n");
		exit(349);
	}
	for (i=0;i<36;i++) {
		switch (i) {
		case 0 :case 1:case 2:case 3:ae->bankgate[i]=0xC0;break;
		case 4 :ae->bankgate[i]=0xC4;break;
		case 5 :ae->bankgate[i]=0xC5;break;
		case 6 :ae->bankgate[i]=0xC6;break;
		case 7 :ae->bankgate[i]=0xC7;break;
		case 8 :ae->bankgate[i]=0xCC;break;
		case 9 :ae->bankgate[i]=0xCD;break;
		case 10:ae->bankgate[i]=0xCE;break;
		case 11:ae->bankgate[i]=0xCF;break;
		case 12:ae->bankgate[i]=0xD4;break;
		case 13:ae->bankgate[i]=0xD5;break;
		case 14:ae->bankgate[i]=0xD6;break;
		case 15:ae->bankgate[i]=0xD7;break;
		case 16:ae->bankgate[i]=0xDC;break;
		case 17:ae->bankgate[i]=0xDD;break;
		case 18:ae->bankgate[i]=0xDE;break;
		case 19:ae->bankgate[i]=0xDF;break;
		case 20:ae->bankgate[i]=0xE4;break;
		case 21:ae->bankgate[i]=0xE5;break;
		case 22:ae->bankgate[i]=0xE6;break;
		case 23:ae->bankgate[i]=0xE7;break;
		case 24:ae->bankgate[i]=0xEC;break;
		case 25:ae->bankgate[i]=0xED;break;
		case 26:ae->bankgate[i]=0xEE;break;
		case 27:ae->bankgate[i]=0xEF;break;
		case 28:ae->bankgate[i]=0xF4;break;
		case 29:ae->bankgate[i]=0xF5;break;
		case 30:ae->bankgate[i]=0xF6;break;
		case 31:ae->bankgate[i]=0xF7;break;
		case 32:ae->bankgate[i]=0xFC;break;
		case 33:ae->bankgate[i]=0xFD;break;
		case 34:ae->bankgate[i]=0xFE;break;
		case 35:ae->bankgate[i]=0xFF;break;
		}
	}
	memcpy(ae->snapshot.idmark,"MV - SNA",8);
	ae->snapshot.registers.IM=1;

	ae->snapshot.gatearray.palette[0]=0x04;
	ae->snapshot.gatearray.palette[1]=0x0A;
	ae->snapshot.gatearray.palette[2]=0x15;
	ae->snapshot.gatearray.palette[3]=0x1C;
	ae->snapshot.gatearray.palette[4]=0x18;
	ae->snapshot.gatearray.palette[5]=0x1D;
	ae->snapshot.gatearray.palette[6]=0x0C;
	ae->snapshot.gatearray.palette[7]=0x05;
	ae->snapshot.gatearray.palette[8]=0x0D;
	ae->snapshot.gatearray.palette[9]=0x16;
	ae->snapshot.gatearray.palette[10]=0x06;
	ae->snapshot.gatearray.palette[11]=0x17;
	ae->snapshot.gatearray.palette[12]=0x1E;
	ae->snapshot.gatearray.palette[13]=0x00;
	ae->snapshot.gatearray.palette[14]=0x1F;
	ae->snapshot.gatearray.palette[15]=0x0E;
	ae->snapshot.gatearray.palette[16]=0x04;

	ae->snapshot.gatearray.multiconfiguration=0x8D; // lower/upper ROM off + mode 1
	ae->snapshot.CPCType=2; /* 6128 */
	ae->snapshot.crtcstate.model=0; /* CRTC 0 */
	ae->snapshot.vsyncdelay=2;
	strcpy((char *)ae->snapshot.unused6+3+0x20+8,RASM_VERSION);
	/* CRTC default registers */
	ae->snapshot.crtc.registervalue[0]=0x3F;
	ae->snapshot.crtc.registervalue[1]=40;
	ae->snapshot.crtc.registervalue[2]=46;
	ae->snapshot.crtc.registervalue[3]=0x8E;
	ae->snapshot.crtc.registervalue[4]=38;
	ae->snapshot.crtc.registervalue[6]=25;
	ae->snapshot.crtc.registervalue[7]=30;
	ae->snapshot.crtc.registervalue[9]=7;
	ae->snapshot.crtc.registervalue[12]=0x30;
	ae->snapshot.psg.registervalue[7]=0x3F; /* audio mix all channels OFF */
	/* standard stack */
	ae->snapshot.registers.HSP=0xC0;

	/*
		winape		sprintf(symbol_line,"%s #%4X\n",ae->label[i].name,ae->label[i].ptr);
		pasmo		sprintf(symbol_line,"%s EQU 0%4XH\n",ae->label[i].name,ae->label[i].ptr);
		rasm 		sprintf(symbol_line,"%s #%X B%d\n",ae->wl[ae->label[i].iw].w,ae->label[i].ptr,ae->label[i].ibank>31?0:ae->label[i].ibank);
	*/
#if DEBUG_PREPRO
printf("paramz\n");
#endif
	if (param && param->labelfilename) {
		for (j=0;param->labelfilename[j] && param->labelfilename[j][0];j++) {
			rasm_printf(ae,"Label import from [%s]\n",param->labelfilename[j]);
			ae->label_filename=param->labelfilename[j];
			ae->label_line=1;
			labelines=FileReadLines(param->labelfilename[j]);
			i=0;
			while (labelines[i]) {
				/* upper case */
				for (j=0;labelines[i][j];j++) labelines[i][j]=toupper(labelines[i][j]);

				if ((labelsep1=strstr(labelines[i],": EQU 0"))!=NULL) {
					/* sjasm */
					*labelsep1=0;
					curlabel.name=labelines[i];
					curlabel.iw=-1;
					curlabel.crc=GetCRC(curlabel.name);
					curlabel.ptr=strtol(labelsep1+6,NULL,16);
					PushLabelLight(ae,&curlabel);
				} else if ((labelsep1=strstr(labelines[i]," EQU 0"))!=NULL) {
					/* pasmo */
					*labelsep1=0;
					curlabel.name=labelines[i];
					curlabel.iw=-1;
					curlabel.crc=GetCRC(curlabel.name);
					curlabel.ptr=strtol(labelsep1+6,NULL,16);
					//ObjectArrayAddDynamicValueConcat((void **)&ae->label,&ae->il,&ae->ml,&curlabel,sizeof(curlabel));
					PushLabelLight(ae,&curlabel);
				} else if ((labelsep1=strstr(labelines[i]," "))!=NULL) {
					/* winape / rasm */
					if (*(labelsep1+1)=='#') {
						*labelsep1=0;
						curlabel.name=labelines[i];
						curlabel.iw=-1;
						curlabel.crc=GetCRC(curlabel.name);
						curlabel.ptr=strtol(labelsep1+2,NULL,16);
						//ObjectArrayAddDynamicValueConcat((void **)&ae->label,&ae->il,&ae->ml,&curlabel,sizeof(curlabel));
						PushLabelLight(ae,&curlabel);
					}
				}
				i++;
				ae->label_line++;
			}
			MemFree(labelines);
		}
		ae->label_filename=NULL;
		ae->label_line=0;
	}
#if DEBUG_PREPRO
printf("init 3\n");
#endif
	/* 32 CPR default roms but 36 max snapshot RAM pages + one workspace */
	for (i=0;i<37;i++) {
		mem=MemMalloc(65536);
		memset(mem,0,65536);
		ObjectArrayAddDynamicValueConcat((void**)&ae->mem,&ae->nbbank,&ae->maxbank,&mem,sizeof(mem));
	}
	ae->activebank=36;
	ae->maxptr=65535;
	for (i=0;i<256;i++) { ae->charset[i]=(unsigned char)i; }
	if (param && param->outputfilename) {
		ae->outputfilename=TxtStrDup(param->outputfilename);
	} else {
		ae->outputfilename=TxtStrDup("rasmoutput");
	}
	if (param && param->filename && !strstr(param->filename,".") && !FileExists(param->filename)) {
		/* pas d'extension, fichier non trouv� */
		l=strlen(param->filename);
		filename=MemRealloc(param->filename,l+5);
		strcat(param->filename,".asm");
		if (!FileExists(param->filename)) {
			TxtReplace(param->filename,".asm",".z80",0); /* no realloc with this */
			if (!FileExists(param->filename)) {
				param->filename[l]=0;
			}
		}
	}

	if (param && param->filename && !FileExists(param->filename)) {
		rasm_printf(ae,"Cannot find file [%s]\n",param->filename);
		exit(-1802);
	}
	
	if (param) rasm_printf(ae,"Pre-processing [%s]\n",param->filename);
	for (nbinstruction=0;instruction[nbinstruction].mnemo[0];nbinstruction++);
	qsort(instruction,nbinstruction,sizeof(struct s_asm_keyword),cmpkeyword);
	for (i=0;i<256;i++) { ae->fastmatch[i]=-1; }
	for (i=0;i<nbinstruction;i++) { if (ae->fastmatch[(int)instruction[i].mnemo[0]]==-1) ae->fastmatch[(int)instruction[i].mnemo[0]]=i; } 
	for (i=0;CharWord[i];i++) {Automate[((int)CharWord[i])&0xFF]=1;}
	 /* separators */
	Automate[' ']=2;
	Automate[',']=2;
	Automate['\t']=2;
	/* end of line */
	Automate[':']=3; /* les 0x0A et 0x0D seront deja� remplaces en ':' */
	/* expression */
	Automate['=']=4; /* on stocke l'emplacement de l'egalite */
	Automate['<']=4; /* ou des operateurs */
	Automate['>']=4; /* d'evaluation */
	Automate['!']=4;
	
	StateMachineResizeBuffer(&w,256,&mw);
	StateMachineResizeBuffer(&bval,256,&sval);
	StateMachineResizeBuffer(&qval,256,&sqval);
	w[0]=0;
	bval[0]=0;
	qval[0]=0;
	
#if DEBUG_PREPRO
printf("read file/flux\n");
#endif
	if (!ae->flux) {
		zelines=FileReadLines(filename);
		FieldArrayAddDynamicValueConcat(&ae->filename,&ae->ifile,&ae->maxfile,filename);
	} else {
		int flux_nblines=0;
		int flux_curpos;

		/* copie des donn�es */
		for (i=0;i<datalen;i++) {
			if (datain[i]=='\n') flux_nblines++;
		}
		zelines=MemMalloc(sizeof(char *)*(flux_nblines+2));
		flux_nblines=0;
		flux_curpos=0;
		for (i=0;i<datalen;i++) {
			if (datain[i]=='\n') {
				zelines[flux_nblines]=MemMalloc(i-flux_curpos+2);
				if (i-flux_curpos) memcpy(zelines[flux_nblines],datain+flux_curpos,i-flux_curpos+1);
				/* et on ajoute un petit z�ro � la fin! */
				zelines[flux_nblines][i-flux_curpos+1]=0;
				flux_curpos=i+1;
				flux_nblines++;
			}
		}
		if (i>flux_curpos) {
			zelines[flux_nblines]=MemMalloc(i-flux_curpos+1);
			memcpy(zelines[flux_nblines],datain+flux_curpos,i-flux_curpos);
			zelines[flux_nblines][i-flux_curpos]=0;
			flux_nblines++;
		}
		/* terminator */
		zelines[flux_nblines]=NULL;

		/* en mode flux on prend le repertoire courant en reference */
		FieldArrayAddDynamicValueConcat(&ae->filename,&ae->ifile,&ae->maxfile,CURRENT_DIR);
	}	

#if DEBUG_PREPRO
printf("remove comz, do includes\n");
#endif
	EarlyPrepSrc(ae,zelines,ae->filename[ae->ifile-1]);

	for (i=0;zelines[i];i++) {
		curlisting.ifile=0;
		curlisting.iline=i+1;
		curlisting.listing=zelines[i];
		ObjectArrayAddDynamicValueConcat((void**)&listing,&ilisting,&maxlisting,&curlisting,sizeof(curlisting));
	}
	MemFree(zelines);

	/* on s'assure que la derniere instruction est prise en compte a peu de frais */
	if (ilisting) {
		datalen=strlen(listing[ilisting-1].listing);
		listing[ilisting-1].listing=MemRealloc(listing[ilisting-1].listing,datalen+2);
		listing[ilisting-1].listing[datalen]=':';
		listing[ilisting-1].listing[datalen+1]=0;
	}

	waiting_quote=quote_type=0;
	l=idx=0;
	while (l<ilisting) {
		c=listing[l].listing[idx++];
		if (!c) {
			l++;
			idx=0;
			continue;
		} else if (c=='\\' && !waiting_quote) {
			idx++;
			continue;
		} else if (c==0x0D || c==0x0A) {
			listing[l].listing[idx-1]=':';
			c=':';
		} else if (c=='\'' && idx>2 && strncmp(&listing[l].listing[idx-3],"AF'",3)==0) {
			/* rien */
		} else if (c=='"' || c=='\'') {
			if (!quote_type) {
				quote_type=c;
				lquote=l;
			} else {
				if (c==quote_type) {
					quote_type=0;
				}
			}
		}

		if (waiting_quote) {
			/* expecting quote and nothing else */
			switch (waiting_quote) {
				case 1:
					if (c==quote_type) waiting_quote=2;
					break;
				case 2:
					if (!quote_type) {
						waiting_quote=3;
						qval[iqval]=0;
					} else {
						qval[iqval++]=c;
						StateMachineResizeBuffer(&qval,iqval,&sqval);
						qval[iqval]=0;
					}
			}
			if (waiting_quote==3) {
				if (incbin) {
					int fileok=0,ilookfile;
					/* qval contient le nom du fichier a lire */
					filename_toread=MergePath(ae,ae->filename[listing[l].ifile],qval);
					if (FileExists(filename_toread)) {
						fileok=1;
					} else {
						for (ilookfile=0;ilookfile<ae->ipath && !fileok;ilookfile++) {
							filename_toread=MergePath(ae,ae->includepath[ilookfile],qval);
							if (FileExists(filename_toread)) {
								fileok=1;
							}
						}
					}
					
					curhexbin.filename=TxtStrDup(filename_toread);
					if (fileok) {
						/* lecture */
						curhexbin.rawlen=curhexbin.datalen=FileGetSize(filename_toread);
						curhexbin.data=MemMalloc(curhexbin.datalen*1.3+10);
						if (ae->verbose & 7) {
							switch (crunch) {
								case 0:rasm_printf(ae,"incbin [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 4:rasm_printf(ae,"inclz4 [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 7:rasm_printf(ae,"incsx7 [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 8:rasm_printf(ae,"incexo [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 88:rasm_printf(ae,"incexb [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 48:rasm_printf(ae,"incl48 [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								case 49:rasm_printf(ae,"incl49 [%s] size=%d\n",filename_toread,curhexbin.datalen);break;
								default:rasm_printf(ae,"invalid crunch state!\n");exit(-42);
							}
						}
						if (FileReadBinary(filename_toread,(char*)curhexbin.data,curhexbin.datalen)!=curhexbin.datalen) {
							rasm_printf(ae,"read error on %s",filename_toread);
							exit(2);
						}
						FileReadBinaryClose(filename_toread);
						switch (crunch) {
							#ifndef NO_3RD_PARTIES
							case 4:
								newdata=LZ4_crunch(curhexbin.data,curhexbin.datalen,&curhexbin.datalen);
								MemFree(curhexbin.data);
								curhexbin.data=newdata;
								if (ae->verbose & 7) {
									rasm_printf(ae,"crunched into %d byte(s)\n",curhexbin.datalen);
								}
								break;
							case 7:
								{
								size_t slzlen;
								newdata=ZX7_compress(optimize(curhexbin.data, curhexbin.datalen), curhexbin.data, curhexbin.datalen, &slzlen);
								curhexbin.datalen=slzlen;
								MemFree(curhexbin.data);
								curhexbin.data=newdata;
								if (ae->verbose & 7) {
									rasm_printf(ae,"crunched into %d byte(s)\n",curhexbin.datalen);
								}
								}
								break;
							case 8:
								newdata=Exomizer_crunch(curhexbin.data,curhexbin.datalen,&curhexbin.datalen);
								MemFree(curhexbin.data);
								curhexbin.data=newdata;
								if (ae->verbose & 7) {
									rasm_printf(ae,"crunched into %d byte(s)\n",curhexbin.datalen);
								}
								break;
							#endif
							case 48:
								newdata=LZ48_crunch(curhexbin.data,curhexbin.datalen,&curhexbin.datalen);
								MemFree(curhexbin.data);
								curhexbin.data=newdata;
								if (ae->verbose & 7) {
									rasm_printf(ae,"crunched into %d byte(s)\n",curhexbin.datalen);
								}
								break;
							case 49:
								newdata=LZ49_crunch(curhexbin.data,curhexbin.datalen,&curhexbin.datalen);
								MemFree(curhexbin.data);
								curhexbin.data=newdata;
								if (ae->verbose & 7) {
									rasm_printf(ae,"crunched into %d byte(s)\n",curhexbin.datalen);
								}
								break;
							default:break;
						}
					} else {
						/* TAG + info */
						curhexbin.datalen=-1;
						curhexbin.data=MemMalloc(2);
						/* not yet an error, we will know later when executing the code */
					}
					ObjectArrayAddDynamicValueConcat((void**)&ae->hexbin,&ae->ih,&ae->mh,&curhexbin,sizeof(curhexbin));
					/* insertion */
					le=strlen(listing[l].listing);

					newlistingline=MemMalloc(le+32);
					memcpy(newlistingline,listing[l].listing,rewrite);
					rewrite+=sprintf(newlistingline+rewrite,"HEXBIN #%X",ae->ih-1);
					strcat(newlistingline+rewrite,listing[l].listing+idx);
					idx=rewrite;
					MemFree(listing[l].listing);
					listing[l].listing=newlistingline;
					incbin=0;
				} else if (include) {
					/* qval contient le nom du fichier a lire */
					int fileok=0,ilookfile;
					/* qval contient le nom du fichier a lire */
					filename_toread=MergePath(ae,ae->filename[listing[l].ifile],qval);
					if (FileExists(filename_toread)) {
						fileok=1;
					} else {
						for (ilookfile=0;ilookfile<ae->ipath && !fileok;ilookfile++) {
							filename_toread=MergePath(ae,ae->includepath[ilookfile],qval);
							if (FileExists(filename_toread)) {
								fileok=1;
							}
						}
					}
					
					if (fileok) {
						if (ae->verbose & 7) rasm_printf(ae,"include [%s]\n",filename_toread);
						
						/* lecture */
						listing_include=FileReadLines(filename_toread);
						FieldArrayAddDynamicValueConcat(&ae->filename,&ae->ifile,&ae->maxfile,filename_toread);
						/* virer les commentaires + pr�-traitement */
						EarlyPrepSrc(ae,listing_include,ae->filename[ae->ifile-1]);
						/* split de la ligne en cours + suppression de l'instruction include */
						PreProcessingSplitListing(&listing,&ilisting,&maxlisting,l,rewrite,idx);
						/* insertion des nouvelles lignes + reference fichier + numeros de ligne */
						PreProcessingInsertListing(&listing,&ilisting,&maxlisting,l,listing_include,ae->ifile-1);
						
						MemFree(listing_include); /* free le tableau mais pas les lignes */
						listing_include=NULL;
						idx=0; /* on reste sur la meme ligne mais on se prepare a relire du caractere 0! */
					} else {
						/* TAG + info */
						curhexbin.filename=TxtStrDup(filename_toread);
						curhexbin.datalen=-2;
						curhexbin.data=MemMalloc(2);
						/* not yet an error, we will know later when executing the code */
						ObjectArrayAddDynamicValueConcat((void**)&ae->hexbin,&ae->ih,&ae->mh,&curhexbin,sizeof(curhexbin));
						/* insertion */
						le=strlen(listing[l].listing);
						newlistingline=MemMalloc(le+32);
						memcpy(newlistingline,listing[l].listing,rewrite);
						rewrite+=sprintf(newlistingline+rewrite,"HEXBIN #%X",ae->ih-1);
						strcat(newlistingline+rewrite,listing[l].listing+idx);
						idx=rewrite;
						MemFree(listing[l].listing);
						listing[l].listing=newlistingline;
					}
					include=0;
				}
				waiting_quote=0;
				qval[0]=0;
				iqval=0;
			}
		} else {
			/* classic behaviour */

			/* looking for include/incbin */
			if (((c>='A' && c<='Z') || (c>='0' && c<='9') || c=='@' || c=='_')&& !quote_type) {
				bval[ival++]=c;
				StateMachineResizeBuffer(&bval,ival,&sval);
				bval[ival]=0;
			} else {
				if (strcmp(bval,"INCLUDE")==0) {
					include=1;
					waiting_quote=1;
					rewrite=idx-7-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"READ")==0) {
					include=1;
					waiting_quote=1;
					rewrite=idx-4-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCLZ4")==0) {
					incbin=1;
					crunch=4;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCEXB")==0) {
					incbin=1;
					crunch=88;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCEXO")==0) {
					incbin=1;
					crunch=8;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCZX7")==0) {
					incbin=1;
					crunch=7;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCL48")==0) {
					incbin=1;
					crunch=48;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCL49")==0) {
					incbin=1;
					crunch=49;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"INCBIN")==0) {
					incbin=1;
					crunch=0;
					waiting_quote=1;
					rewrite=idx-6-1;
					/* quote right after keyword */
					if (c==quote_type) {
						waiting_quote=2;
					}
				} else if (strcmp(bval,"WHILE")==0) {
					/* remplir la structure repeat_index */
					windex.ol=listing[l].iline;
					windex.oidx=idx;
					windex.ifile=ae->ifile-1;
					ObjectArrayAddDynamicValueConcat((void**)&TABwindex,&nwi,&mwi,&windex,sizeof(windex));
				} else if (strcmp(bval,"REPEAT")==0) {
					/* remplir la structure repeat_index */
					rindex.ol=listing[l].iline;
					rindex.oidx=idx;
					rindex.ifile=ae->ifile-1;
					ObjectArrayAddDynamicValueConcat((void**)&TABrindex,&nri,&mri,&rindex,sizeof(rindex));
				} else if (strcmp(bval,"WEND")==0) {
					/* retrouver la structure repeat_index correspondant a l'ouverture */
					for (wi=nwi-1;wi>=0;wi--) {
						if (TABwindex[wi].cl==-1) {
							TABwindex[wi].cl=c;
							TABwindex[wi].cidx=idx;
							break;
						}
					}
					if (wi==-1) {
						rasm_printf(ae,"[%s:%d] WEND refers to unknown WHILE\n",ae->filename[listing[l].ifile],listing[l].iline);
						exit(1);
					}
				} else if (strcmp(bval,"REND")==0 || strcmp(bval,"UNTIL")==0) {
					/* retrouver la structure repeat_index correspondant a l'ouverture */
					for (ri=nri-1;ri>=0;ri--) {
						if (TABrindex[ri].cl==-1) {
							TABrindex[ri].cl=c;
							TABrindex[ri].cidx=idx;
							break;
						}
					}
					if (ri==-1) {
						rasm_printf(ae,"[%s:%d] %s refers to unknown REPEAT\n",ae->filename[listing[l].ifile],listing[l].iline,bval);
						exit(1);
					}
						
				}
				bval[0]=0;
				ival=0;
			}
		}
	}
#if DEBUG_PREPRO
printf("check quotes and repeats\n");
#endif
	if (quote_type) {
		rasm_printf(ae,"[%s:%d] quote opened was not closed\n",ae->filename[listing[lquote].ifile],listing[lquote].iline);
		exit(1);
	}

	/* repeat expansion check */
	for (ri=0;ri<nri;ri++) {
		if (TABrindex[ri].cl==-1) {
			rasm_printf(ae,"[%s:%d] REPEAT was not closed\n",ae->filename[TABrindex[ri].ifile],TABrindex[ri].ol);
			MaxError(ae);
		}
	}

	/* creer une liste de mots */
	curw.w=TxtStrDup("BEGIN");
	curw.l=0;
	curw.ifile=0;
	curw.t=1;
	curw.e=0;
	ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));

	/* pour les calculs d'adresses avec IX et IY on enregistre deux variables bidons du meme nom */
	curw.e=2;
	curw.w=TxtStrDup("IX~0");
	ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
	curw.w=TxtStrDup("IY~0");
	ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
	curw.e=0;

#if DEBUG_PREPRO
	l=0;
	while (l<ilisting) {
		rasm_printf(ae,"listing[%d]\n%s\n",l,listing[l].listing);
		l++;
	}
#endif	

	texpr=quote_type=0;
	l=lw=idx=0;
	ispace=0;
	w[0]=0;
	while (l<ilisting) {
		c=listing[l].listing[idx++];
		if (!c) {
			idx=0;
			l++;
			continue;
		}

		if (!quote_type) {
#if DEBUG_PREPRO
printf("c='%c' automate[c]=%d\n",c>31?c:'.',Automate[((int)c)&0xFF]);			
#endif
			switch (Automate[((int)c)&0xFF]) {
				case 0:
					rasm_printf(ae,"[%s:%d] invalid char '%c' (%d) char %d\n",ae->filename[listing[l].ifile],listing[l].iline,c,c,idx);
					MaxError(ae);
					break;
				case 1:
					if (c=='\'' && idx>2 && strncmp(&listing[l].listing[idx-3],"AF'",3)==0) {
						w[lw++]=c;
						StateMachineResizeBuffer(&w,lw,&mw);
						w[lw]=0;
						break;
					} else if (c=='\'' || c=='"') {
						quote_type=c;
						/* debut d'une quote, on finalise le mot -> POURQUOI DONC? */
						//idx--;
#if DEBUG_PREPRO
printf("quote\n");
#endif
						/* on finalise le mot si on est en d�but d'une nouvelle instruction ET que c'est un SAVE */
						if (strcmp(w,"SAVE")==0) {
							idx--;
						} else {
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							break;
						}
					} else {
						if (c!=' ' && c!='\t') {
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
						} else {
							/* Winape/Maxam operator compatibility on expressions */
#if DEBUG_PREPRO
printf("1/2 winape maxam operator test for [%s]\n",w+ispace);
#endif
							if (texpr) {
								if (strcmp(w+ispace,"AND")==0) {
									w[ispace]='&';
									lw=ispace+1;
								} else if (strcmp(w+ispace,"OR")==0) {
#if DEBUG_PREPRO
printf("conversion OR vers |\n");
#endif
									w[ispace]='|';
									lw=ispace+1;
								} else if (strcmp(w+ispace,"MOD")==0) {
									w[ispace]='m';
									lw=ispace+1;
								} else if (strcmp(w+ispace,"XOR")==0) {
									w[ispace]='^';
									lw=ispace+1;
								} else if (strcmp(w+ispace,"%")==0) {
									w[ispace]='m';
									lw=ispace+1;
								}
							}
							ispace=lw;
						}
						break;
					}
				case 2:
					/* separator (space, tab, comma) */
#if DEBUG_PREPRO
printf("*** separator='%c'\n",c);
#endif
					
					/* patch argument suit une expression d'�valuation (ASSERT) */
					if (c==',') hadcomma=1;
					
					if (lw) {
						w[lw]=0;
						if (texpr && !wordlist[nbword-1].t && wordlist[nbword-1].e) {
							/* pour compatibilite winape avec AND,OR,XOR */
#if DEBUG_PREPRO
printf("2/2 winape maxam operator test for [%s]\n",w+ispace);
#endif
							if (strcmp(w,"AND")==0) {
								wtmp=TxtStrDup("&");
							} else if (strcmp(w,"OR")==0) {
								wtmp=TxtStrDup("|");
							} else if (strcmp(w,"XOR")==0) {
								wtmp=TxtStrDup("^");
							} else if (strcmp(w,"%")==0) {
								wtmp=TxtStrDup("m");
							} else {
								wtmp=TxtStrDup(w);
							}
							nbword--;
							lw=0;
							for (li=0;wordlist[nbword].w[li];li++) {
								w[lw++]=wordlist[nbword].w[li];
								StateMachineResizeBuffer(&w,lw,&mw);
							}
							w[lw]=0;
							MemFree(wordlist[nbword].w);
							
							for (li=0;wtmp[li];li++) {
								w[lw++]=wtmp[li];
								StateMachineResizeBuffer(&w,lw,&mw);
							}
							w[lw]=0;
							MemFree(wtmp);
							/* et on modifie l'automate pour la suite! */
							Automate[' ']=1;
							Automate['\t']=1;
							ispace=lw;
						} else if (strcmp(w,"EQU")==0) {
							/* il y avait un mot avant alors on va reorganiser la ligne */
							nbword--;
							lw=0;
							for (li=0;wordlist[nbword].w[li];li++) {
								w[lw++]=wordlist[nbword].w[li];
								StateMachineResizeBuffer(&w,lw,&mw);
							}
							MemFree(wordlist[nbword].w);
							curw.e=lw+1;
							/* on ajoute l'egalite d'alias*/
							w[lw++]='~';
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							Automate[' ']=1;
							Automate['\t']=1;
							ispace=lw;
							texpr=1;
						} else {
							curw.w=TxtStrDup(w);
							curw.l=listing[l].iline;
							curw.ifile=listing[l].ifile;
							curw.t=0;
#if DEBUG_PREPRO
if (curw.w[0]=='=') {
	printf("(1) bug prout\n");
	exit(1);
}
printf("ajout du mot [%s]\n",curw.w);
#endif
							ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
							//texpr=0; /* reset expr */
							curw.e=0;
							lw=0;
							w[lw]=0;

							/* match keyword? then next spaces will be ignored*/
							if (macro_trigger) {
								struct s_macro_fast curmacrofast;
								Automate[' ']=1;
								Automate['\t']=1;
								ispace=0;
								texpr=1;
#if DEBUG_PREPRO
printf("macro trigger w=[%s]\n",curw.w);
#endif
								/* add macro name to instruction pool for preprocessor but not struct or write */
								if (macro_trigger=='M') {
									curmacrofast.mnemo=curw.w;
									curmacrofast.crc=GetCRC(curw.w);
									ObjectArrayAddDynamicValueConcat((void**)&MacroFast,&idxmacrofast,&maxmacrofast,&curmacrofast,sizeof(struct s_macro_fast));	
								}
								macro_trigger=0;
							} else {
								int keymatched=0;
								if ((ifast=ae->fastmatch[(int)curw.w[0]])!=-1) {
									while (instruction[ifast].mnemo[0]==curw.w[0]) {
										if (strcmp(instruction[ifast].mnemo,curw.w)==0) {
											keymatched=1;														
											if (strcmp(curw.w,"MACRO")==0 || strcmp(curw.w,"STRUCT")==0 || strcmp(curw.w,"WRITE")==0) {
/* @@TODO AS80 compatibility patch!!! */
												macro_trigger=curw.w[0];
											} else {
												Automate[' ']=1;
												Automate['\t']=1;
												ispace=0;
												/* instruction en cours, le reste est a interpreter comme une expression */
#if DEBUG_PREPRO
printf("instruction en cours\n");												
#endif
												texpr=1;
											}
											break;
										}
										ifast++;
									}
								}
								if (!keymatched) {
									int macrocrc;
									macrocrc=GetCRC(curw.w);
									for (keymatched=0;keymatched<idxmacrofast;keymatched++) {
										if (MacroFast[keymatched].crc==macrocrc)
										if (strcmp(MacroFast[keymatched].mnemo,curw.w)==0) {
												Automate[' ']=1;
												Automate['\t']=1;
												ispace=0;
												/* macro en cours, le reste est a interpreter comme une expression */
												texpr=1;
												break;
										}
									}
								}
							}
						}
					} else {
						if (hadcomma) {
							rasm_printf(ae,"[%s:%d] empty parameter\n",ae->filename[listing[l].ifile],listing[l].iline);
							MaxError(ae);
						}
					}
					break;
				case 3:
					/* fin de ligne, on remet l'automate comme il faut */
#if DEBUG_PREPRO
printf("EOL\n");																	
#endif
					macro_trigger=0;
					Automate[' ']=2;
					Automate['\t']=2;
					ispace=0;
					texpr=0;
					/* si le mot lu a plus d'un caract�re */
					if (lw) {
						if (!wordlist[nbword-1].t && (wordlist[nbword-1].e || w[0]=='=') && !hadcomma) {
							/* cas particulier d'ecriture libre */
							/* bugfix inhibition 19.06.2018 */
							/* ajout du terminateur? */
							w[lw]=0;
#if DEBUG_PREPRO
printf("nbword=%d w=[%s] ->",nbword,w);fflush(stdout);
#endif
							nbword--;
							wordlist[nbword].w=MemRealloc(wordlist[nbword].w,strlen(wordlist[nbword].w)+lw+1);
							strcat(wordlist[nbword].w,w);
#if DEBUG_PREPRO
printf("%s\n",wordlist[nbword].w);
#endif
							/* on change de type! */
							wordlist[nbword].t=1;
							//ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
							curw.e=0;
							lw=0;
							w[lw]=0;
						} else if (nbword && strcmp(w,"EQU")==0) {
							/* il y avait un mot avant alors on va reorganiser la ligne */
							nbword--;
							lw=0;
							for (li=0;wordlist[nbword].w[li];li++) {
								w[lw++]=wordlist[nbword].w[li];
								StateMachineResizeBuffer(&w,lw,&mw);
								w[lw]=0;
							}
							MemFree(wordlist[nbword].w);
							/* on ajoute l'egalite ou comparaison! */
							curw.e=lw+1;
							w[lw++]='=';
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							Automate[' ']=1;
							Automate['\t']=1;
						} else {
							/* mot de fin de ligne, � priori pas une expression */
							curw.w=TxtStrDup(w);
							curw.l=listing[l].iline;
							curw.ifile=listing[l].ifile;
							curw.t=1;
#if DEBUG_PREPRO
printf("mot de fin de ligne = [%s]\n",curw.w);							
if (curw.w[0]=='=') {
	printf("(3) bug prout\n");
	exit(1);
}
#endif
							ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
							curw.e=0;
							lw=0;
							w[lw]=0;
							hadcomma=0;
						}
					} else {
						/* sinon c'est le pr�c�dent qui �tait terminateur d'instruction */
						wordlist[nbword-1].t=1;
						w[lw]=0;
					}
					hadcomma=0;
					break;
				case 4:
#if DEBUG_PREPRO
printf("expr operator=%c\n",c);
#endif
					/* expression/condition */
					texpr=1;
				    if (lw) {
						Automate[' ']=1;
						Automate['\t']=1;
						if (!curw.e) {
							curw.e=lw+1;
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
						} else {
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							/* on autorise uniquement !=, ==, <=, >= et une seule fois 
							if (c!='=' || lw!=curw.e+1) {
								rasm_printf(ae,"[%s] L%d - too much equality symbol '='\n",ae->filename[listing[l].ifile],listing[l].iline);
								MaxError(ae);
							}
							
							A l'avenir on ne notera que les affectations...
							
							*/
						}
					} else {
						/* 2018.06.06 �volution sur le ! (not) */
#if DEBUG_PREPRO
printf("*** operateur commence le mot\n");						
printf("mot precedent=[%s] t=%d\n",wordlist[nbword-1].w,wordlist[nbword-1].t);
#endif
						if (hadcomma && c=='!') {
							/* on peut commencer un argument par un NOT */
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							/* automate d�j� modifi� rien de plus */
						} else if (!wordlist[nbword-1].t) {
							/* il y avait un mot avant alors on va reorganiser la ligne */
							/* patch NOT -> SAUF si c'est une directive */
							int keymatched=0;
							if ((ifast=ae->fastmatch[(int)wordlist[nbword-1].w[0]])!=-1) {
								while (instruction[ifast].mnemo[0]==wordlist[nbword-1].w[0]) {
									if (strcmp(instruction[ifast].mnemo,wordlist[nbword-1].w)==0) {
										keymatched=1;														
										break;
									}
									ifast++;
								}
							}
							if (!keymatched) {
								int macrocrc;
								macrocrc=GetCRC(wordlist[nbword-1].w);
								for (i=0;i<idxmacrofast;i++) {
									if (MacroFast[i].crc==macrocrc)
									if (strcmp(MacroFast[i].mnemo,wordlist[nbword-1].w)==0) {
										keymatched=1;
										break;
									}
								}
							}
							if (!keymatched) {
								nbword--;
								for (li=0;wordlist[nbword].w[li];li++) {
									w[lw++]=wordlist[nbword].w[li];
									StateMachineResizeBuffer(&w,lw,&mw);
									w[lw]=0;
								}
								MemFree(wordlist[nbword].w);
								/* on ajoute l'egalite ou comparaison! */
								curw.e=lw+1;
							}
							w[lw++]=c;
							StateMachineResizeBuffer(&w,lw,&mw);
							w[lw]=0;
							/* et on modifie l'automate pour la suite! */
							Automate[' ']=1;
							Automate['\t']=1;
						} else {
							rasm_printf(ae,"[%s:%d] cannot start expression with '=','!','<','>'\n",ae->filename[listing[l].ifile],listing[l].iline);
							MaxError(ae);
						}
					}
					break;
				default:
					rasm_printf(ae,"Internal error (Automate wrong value=%d)\n",Automate[c]);
					exit(-1);
			}
		} else {
			/* lecture inconditionnelle de la quote */
#if DEBUG_PREPRO
printf("quote[%d]=%c\n",lw,c);
#endif
			w[lw++]=c;
			StateMachineResizeBuffer(&w,lw,&mw);
			w[lw]=0;
			if (!escape_code) {
				if (c=='\\') escape_code=1;
				if (lw>1 && c==quote_type) {
					quote_type=0;
				}
			} else {
				escape_code=0;
			}
		}
	}

	curw.w="END";
	curw.l=0;
	curw.t=2;
	curw.ifile=0;
	ObjectArrayAddDynamicValueConcat((void**)&wordlist,&nbword,&maxword,&curw,sizeof(curw));
	if (ae->verbose & 7) rasm_printf(ae,"wordlist contains %d element%s\n",nbword,nbword>1?"s":"");
	ae->nbword=nbword;

	/* switch words for macro declaration with AS80 & UZ80 */
	if (param && param->as80) {
		for (l=0;l<nbword;l++) {
			if (!wordlist[l].t && !wordlist[l].e && strcmp(wordlist[l+1].w,"MACRO")==0) {
				char *wtmp;
				wtmp=wordlist[l+1].w;
				wordlist[l+1].w=wordlist[l].w;
				wordlist[l].w=wtmp;
			}
		}
	}
	
	if (ae->verbose&2) {
		for (l=0;l<nbword;l++) {
			rasm_printf(ae,"[%s]e=%d ",wordlist[l].w,wordlist[l].e);
			if (wordlist[l].t) rasm_printf(ae,"(%d)\n",wordlist[l].t);
		}
	}


	MemFree(bval);
	MemFree(qval);
	MemFree(w);

	for (l=0;l<ilisting;l++) {
			MemFree(listing[l].listing);
	}	
	MemFree(listing);
	/* wordlist 
		type 0: label or instruction followed by parameter(s)
		type 1: last word of the line, last parameter of an instruction
		type 2: very last word of the list
		e tag: non zero if there is comparison or equality
	*/
	ae->wl=wordlist;
	if (param) {
		MemFree(param->filename);
	}
	if (MacroFast) MemFree(MacroFast);
	if (TABwindex) MemFree(TABwindex);
	if (TABrindex) MemFree(TABrindex);
	return ae;
}

int Rasm(struct s_parameter *param)
{
	#undef FUNC
	#define FUNC "Rasm"

	struct s_assenv *ae=NULL;

	/* read and preprocess source */
	ae=PreProcessing(param->filename,0,NULL,0,param);
	/* assemble */
	return Assemble(ae,NULL,NULL);
}

/* fonction d'export */

int RasmAssemble(const char *datain, int lenin, unsigned char **dataout, int *lenout)
{
	struct s_assenv *ae=NULL;
	
	ae=PreProcessing(NULL,1,datain,lenin,NULL);
	return Assemble(ae,dataout,lenout);
}

#define AUTOTEST_NOINCLUDE "truc equ 0:if truc:include'bite':endif:nop"

#define AUTOTEST_SETINSIDE "ld hl,0=0xC9FB"

#define AUTOTEST_OPERATOR_CONVERSION "ld hl,10 OR 20:ld a,40 and 10:ld bc,5 MOD 2:ld a,(ix+45 xor 45)"

#define AUTOTEST_UNDEF "mavar=10: ifdef mavar: undef mavar: endif: ifdef mavar: fail 'undef did not work': endif:nop "

#define AUTOTEST_INSTRMUSTFAILED "ld a,b,c:ldi a: ldir bc:exx hl,de:exx de:ex bc,hl:ex hl,bc:ex af,af:ex hl,hl:ex hl:exx hl: "\
	"neg b:push b:push:pop:pop c:sub ix:add ix:add:sub:di 2:ei 3:ld i,c:ld r,e:rl:rr:rlca a:sla:sll:"\
	"ldd e:lddr hl:adc ix:adc b,a:xor 12,13:xor b,1:xor:or 12,13:or b,1:or:and 12,13:and b,1:and:inc:dec"

#define AUTOTEST_VIRGULE "defb 5,5,,5"
#define AUTOTEST_VIRGULE2 "print '5,,5':nop"

#define AUTOTEST_OVERLOADMACPRM "macro test,idx: defb idx:endm:macro test2,idx:defb {idx}:endm:repeat 2,idx:test idx-1:test2 idx-1:rend"

#define AUTOTEST_PRINTVAR "label1:   macro test, param:        print 'param {param}', {hex}{param}:    endm::    test label1: nop"

#define AUTOTEST_PRINTSPACE "idx=5: print 'grouik { idx + 3 } ':nop"

#define AUTOTEST_NOT 	"myvar=10:myvar=10+myvar:if 5!=3:else:print glop:endif:ifnot 5:print glop:else:endif:" \
						"ifnot 0:else:print glop:endif:if !(5):print glop:endif:if !(0):else:print glop:endif:" \
						"ya=!0:if ya==1:else:print glop:endif:if !5:print glop:endif:ya = 0:ya =! 0:if ya == 1:" \
						"else:print glop:endif:if ! 5:print glop:endif:if 1-!( !0 && !0):else:print glop:endif:nop" 


#define AUTOTEST_MACRO "macro glop:@glop:ld hl,@next:djnz @glop:@next:mend:macro glop2:@glop:glop:ld hl,@next:djnz @glop:glop:" \
                       "@next:mend:cpti=0:repeat:glop:cpt=0:glop:repeat:glop2:repeat 1:@glop:dec a:ld hl,@next:glop2:glop2:" \
                       "jr nz,@glop:@next:rend:cpt=cpt+1:glop2:until cpt<3:cpti=cpti+1:glop2:until cpti<3"

#define AUTOTEST_MACRO_ADV 	"idx=10:macro mac2 param1,param2:ld hl,{param1}{idx+10}{param2}:{param1}{idx+10}{param2}:djnz {param1}{idx+10}{param2}:mend: " \
							"mac2 label,45:mac2 glop,10:djnz glop2010:jp label2045"
							
#define AUTOTEST_MACROPAR	"macro unemac, param1, param2:defb '{param1}':defb {param2}:mend:unemac grouik,'grouik'"

#define AUTOTEST_OPCODES "nop::ld bc,#1234::ld (bc),a::inc bc:inc b:dec b:ld b,#12:rlca:ex af,af':add hl,bc:ld a,(bc):dec bc:" \
                         "inc c:dec c:ld c,#12:rrca::djnz $:ld de,#1234:ld (de),a:inc de:inc d:dec d:ld d,#12:rla:jr $:" \
                         "add hl,de:ld a,(de):dec de:inc e:dec e:ld e,#12:rra::jr nz,$:ld hl,#1234:ld (#1234),hl:inc hl:inc h:" \
                         "dec h:ld h,#12:daa:jr z,$:add hl,hl:ld hl,(#1234):dec hl:inc l:dec l:ld l,#12:cpl::jr nc,$:" \
                         "ld sp,#1234:ld (#1234),a:inc sp:inc (hl):dec (hl):ld (hl),#12:scf:jr c,$:add hl,sp:ld a,(#1234):" \
                         "dec sp:inc a:dec a:ld a,#12:ccf::ld b,b:ld b,c:ld b,d:ld b,e:ld b,h:ld b,l:ld b,(hl):ld b,a:ld c,b:" \
                         "ld c,c:ld c,d:ld c,e:ld c,h:ld c,l:ld c,(hl):ld c,a::ld d,b:ld d,c:ld d,d:ld d,e:ld d,h:ld d,l:" \
                         "ld d,(hl):ld d,a:ld e,b:ld e,c:ld e,d:ld e,e:ld e,h:ld e,l:ld e,(hl):ld e,a::ld h,b:ld h,c:ld h,d:" \
                         "ld h,e:ld h,h:ld h,l:ld h,(hl):ld h,a:ld l,b:ld l,c:ld l,d:ld l,e:ld l,h:ld l,l:ld l,(hl):ld l,a::" \
                         "ld (hl),b:ld (hl),c:ld (hl),d:ld (hl),e:ld (hl),h:ld (hl),l:halt:ld (hl),a:ld a,b:ld a,c:ld a,d:" \
                         "ld a,e:ld a,h:ld a,l:ld a,(hl):ld a,a::add b:add c:add d:add e:add h:add l:add (hl):add a:adc b:" \
                         "adc c:adc d:adc e:adc h:adc l:adc (hl):adc a::sub b:sub c:sub d:sub e:sub h:sub l:sub (hl):sub a:" \
                         "sbc b:sbc c:sbc d:sbc e:sbc h:sbc l:sbc (hl):sbc a::and b:and c:and d:and e:and h:and l:and (hl):" \
                         "and a:xor b:xor c:xor d:xor e:xor h:xor l:xor (hl):xor a::or b:or c:or d:or e:or h:or l:or (hl):" \
                         "or a:cp b:cp c:cp d:cp e:cp h:cp l:cp (hl):cp a::ret nz:pop bc:jp nz,#1234:jp #1234:call nz,#1234:" \
                         "push bc:add #12:rst 0:ret z:ret:jp z,#1234:nop:call z,#1234:call #1234:adc #12:rst 8::ret nc:pop de:" \
                         "jp nc,#1234:out (#12),a:call nc,#1234:push de:sub #12:rst #10:ret c:exx:jp c,#1234:in a,(#12):" \
                         "call c,#1234:nop:sbc #12:rst #18::ret po:pop hl:jp po,#1234:ex (sp),hl:call po,#1234:push hl:" \
                         "and #12:rst #20:ret pe:jp (hl):jp pe,#1234:ex de,hl:call pe,#1234:nop:xor #12:rst #28::ret p:pop af:" \
                         "jp p,#1234:di:call p,#1234:push af:or #12:rst #30:ret m:ld sp,hl:jp m,#1234:ei:call m,#1234:nop:" \
                         "cp #12:rst #38:in b,(c):out (c),b:sbc hl,bc:ld (#1234),bc:neg:retn:im 0:ld i,a:in c,(c):out (c),c:" \
                         "adc hl,bc:ld bc,(#1234):reti:ld r,a::in d,(c):out (c),d:sbc hl,de:ld (#1234),de:retn:im 1:ld a,i:" \
                         "in e,(c):out (c),e:adc hl,de:ld de,(#1234):im 2:ld a,r::in h,(c):out (c),h:sbc hl,hl:rrd:in l,(c):" \
                         "out (c),l:adc hl,hl:rld::in 0,(c):out (c),0:sbc hl,sp:ld (#1234),sp:in a,(c):out (c),a:adc hl,sp:" \
                         "ld sp,(#1234)::ldi:cpi:ini:outi:ldd:cpd:ind:outd::ldir:cpir:inir:otir:lddr:cpdr:indr:otdr::rlc b:" \
                         "rlc c:rlc d:rlc e:rlc h:rlc l:rlc (hl):rlc a:rrc b:rrc c:rrc d:rrc e:rrc h:rrc l:rrc (hl):rrc a::" \
                         "rl b:rl c:rl d:rl e:rl h:rl l:rl (hl):rl a:rr b:rr c:rr d:rr e:rr h:rr l:rr (hl):rr a:sla b:sla c:" \
                         "sla d:sla e:sla h:sla l:sla (hl):sla a:sra b:sra c:sra d:sra e:sra h:sra l:sra (hl):sra a::sll b:" \
                         "sll c:sll d:sll e:sll h:sll l:sll (hl):sll a:srl b:srl c:srl d:srl e:srl h:srl l:srl (hl):srl a::" \
                         "bit 0,b:bit 0,c:bit 0,d:bit 0,e:bit 0,h:bit 0,l:bit 0,(hl):bit 0,a::bit 1,b:bit 1,c:bit 1,d:bit 1,e:" \
                         "bit 1,h:bit 1,l:bit 1,(hl):bit 1,a::bit 2,b:bit 2,c:bit 2,d:bit 2,e:bit 2,h:bit 2,l:bit 2,(hl):" \
                         "bit 2,a::bit 3,b:bit 3,c:bit 3,d:bit 3,e:bit 3,h:bit 3,l:bit 3,(hl):bit 3,a::bit 4,b:bit 4,c:" \
                         "bit 4,d:bit 4,e:bit 4,h:bit 4,l:bit 4,(hl):bit 4,a::bit 5,b:bit 5,c:bit 5,d:bit 5,e:bit 5,h:bit 5,l:" \
                         "bit 5,(hl):bit 5,a::bit 6,b:bit 6,c:bit 6,d:bit 6,e:bit 6,h:bit 6,l:bit 6,(hl):bit 6,a::bit 7,b:" \
                         "bit 7,c:bit 7,d:bit 7,e:bit 7,h:bit 7,l:bit 7,(hl):bit 7,a::res 0,b:res 0,c:res 0,d:res 0,e:res 0,h:" \
                         "res 0,l:res 0,(hl):res 0,a::res 1,b:res 1,c:res 1,d:res 1,e:res 1,h:res 1,l:res 1,(hl):res 1,a::" \
                         "res 2,b:res 2,c:res 2,d:res 2,e:res 2,h:res 2,l:res 2,(hl):res 2,a::res 3,b:res 3,c:res 3,d:res 3,e:" \
                         "res 3,h:res 3,l:res 3,(hl):res 3,a::res 4,b:res 4,c:res 4,d:res 4,e:res 4,h:res 4,l:res 4,(hl):" \
                         "res 4,a::res 5,b:res 5,c:res 5,d:res 5,e:res 5,h:res 5,l:res 5,(hl):res 5,a::res 6,b:res 6,c:" \
                         "res 6,d:res 6,e:res 6,h:res 6,l:res 6,(hl):res 6,a::res 7,b:res 7,c:res 7,d:res 7,e:res 7,h:res 7,l:" \
                         "res 7,(hl):res 7,a::set 0,b:set 0,c:set 0,d:set 0,e:set 0,h:set 0,l:set 0,(hl):set 0,a::set 1,b:" \
                         "set 1,c:set 1,d:set 1,e:set 1,h:set 1,l:set 1,(hl):set 1,a::set 2,b:set 2,c:set 2,d:set 2,e:set 2,h:" \
                         "set 2,l:set 2,(hl):set 2,a::set 3,b:set 3,c:set 3,d:set 3,e:set 3,h:set 3,l:set 3,(hl):set 3,a::" \
                         "set 4,b:set 4,c:set 4,d:set 4,e:set 4,h:set 4,l:set 4,(hl):set 4,a::set 5,b:set 5,c:set 5,d:set 5,e:" \
                         "set 5,h:set 5,l:set 5,(hl):set 5,a::set 6,b:set 6,c:set 6,d:set 6,e:set 6,h:set 6,l:set 6,(hl):" \
                         "set 6,a::set 7,b:set 7,c:set 7,d:set 7,e:set 7,h:set 7,l:set 7,(hl):set 7,a::add ix,bc::add ix,de::" \
                         "ld ix,#1234:ld (#1234),ix:inc ix:inc xh:dec xh:ld xh,#12:add ix,ix:ld ix,(#1234):dec ix:inc xl:" \
                         "dec xl:ld xl,#12::inc (ix+#12):dec (ix+#12):ld (ix+#12),#34:add ix,sp::ld b,xh:ld b,xl:" \
                         "ld b,(ix+#12):ld c,xh:ld c,xl:ld c,(ix+#12):::ld d,xh:ld d,xl:ld d,(ix+#12):ld e,xh:ld e,xl:" \
                         "ld e,(ix+#12)::ld xh,b:ld xh,c:ld xh,d:ld xh,e:ld xh,xh:ld xh,xl:ld h,(ix+#12):ld xh,a:ld xl,b:" \
                         "ld xl,c:ld xl,d:ld xl,e:ld xl,xh:ld xl,xl:ld l,(ix+#12):ld xl,a::ld (ix+#12),b:ld (ix+#12),c:" \
                         "ld (ix+#12),d:ld (ix+#12),e:ld (ix+#12),h:ld (ix+#12),l:ld (ix+#12),a:ld a,xh:ld a,xl:" \
                         "ld a,(ix+#12)::add xh:add xl:add (ix+#12):adc xh:adc xl:adc (ix+#12)::sub xh:sub xl:sub (ix+#12):" \
                         "sbc xh:sbc xl:sbc (ix+#12)::and xh:and xl:and (ix+#12):xor xh:xor xl:xor (ix+#12)::or xh:or xl:" \
                         "or (ix+#12):cp xh:cp xl:cp (ix+#12)::pop ix:ex (sp),ix:push ix:jp (ix)::ld sp,ix:::rlc (ix+#12),b:" \
                         "rlc (ix+#12),c:rlc (ix+#12),d:rlc (ix+#12),e:rlc (ix+#12),h:rlc (ix+#12),l:rlc (ix+#12):" \
                         "rlc (ix+#12),a:rrc (ix+#12),b:rrc (ix+#12),c:rrc (ix+#12),d:rrc (ix+#12),e:rrc (ix+#12),h:" \
                         "rrc (ix+#12),l:rrc (ix+#12):rrc (ix+#12),a::rl (ix+#12),b:rl (ix+#12),c:rl (ix+#12),d:rl (ix+#12),e:" \
                         "rl (ix+#12),h:rl (ix+#12),l:rl (ix+#12):rl (ix+#12),a:rr (ix+#12),b:rr (ix+#12),c:rr (ix+#12),d:" \
                         "rr (ix+#12),e:rr (ix+#12),h:rr (ix+#12),l:rr (ix+#12):rr (ix+#12),a::sla (ix+#12),b:sla (ix+#12),c:" \
                         "sla (ix+#12),d:sla (ix+#12),e:sla (ix+#12),h:sla (ix+#12),l:sla (ix+#12):sla (ix+#12),a:" \
                         "sra (ix+#12),b:sra (ix+#12),c:sra (ix+#12),d:sra (ix+#12),e:sra (ix+#12),h:sra (ix+#12),l:" \
                         "sra (ix+#12):sra (ix+#12),a::sll (ix+#12),b:sll (ix+#12),c:sll (ix+#12),d:sll (ix+#12),e:" \
                         "sll (ix+#12),h:sll (ix+#12),l:sll (ix+#12):sll (ix+#12),a:srl (ix+#12),b:srl (ix+#12),c:" \
                         "srl (ix+#12),d:srl (ix+#12),e:srl (ix+#12),h:srl (ix+#12),l:srl (ix+#12):srl (ix+#12),a::" \
                         "bit 0,(ix+#12):bit 1,(ix+#12):bit 2,(ix+#12):bit 3,(ix+#12):bit 4,(ix+#12):bit 5,(ix+#12):" \
                         "bit 6,(ix+#12):bit 7,(ix+#12):bit 0,(ix+#12),d:bit 1,(ix+#12),b:bit 2,(ix+#12),c:bit 3,(ix+#12),d:" \
                         "bit 4,(ix+#12),e:bit 5,(ix+#12),h:bit 6,(ix+#12),l:bit 7,(ix+#12),a:::res 0,(ix+#12),b:" \
                         "res 0,(ix+#12),c:res 0,(ix+#12),d:res 0,(ix+#12),e:res 0,(ix+#12),h:res 0,(ix+#12),l:res 0,(ix+#12):" \
                         "res 0,(ix+#12),a::res 1,(ix+#12),b:res 1,(ix+#12),c:res 1,(ix+#12),d:res 1,(ix+#12),e:" \
                         "res 1,(ix+#12),h:res 1,(ix+#12),l:res 1,(ix+#12):res 1,(ix+#12),a::res 2,(ix+#12),b:" \
                         "res 2,(ix+#12),c:res 2,(ix+#12),d:res 2,(ix+#12),e:res 2,(ix+#12),h:res 2,(ix+#12),l:res 2,(ix+#12):" \
                         "res 2,(ix+#12),a::res 3,(ix+#12),b:res 3,(ix+#12),c:res 3,(ix+#12),d:res 3,(ix+#12),e:" \
                         "res 3,(ix+#12),h:res 3,(ix+#12),l:res 3,(ix+#12):res 3,(ix+#12),a::res 4,(ix+#12),b:" \
                         "res 4,(ix+#12),c:res 4,(ix+#12),d:res 4,(ix+#12),e:res 4,(ix+#12),h:res 4,(ix+#12),l:" \
                         "res 4,(ix+#12):res 4,(ix+#12),a::res 5,(ix+#12),b:res 5,(ix+#12),c:res 5,(ix+#12),d:" \
                         "res 5,(ix+#12),e:res 5,(ix+#12),h:res 5,(ix+#12),l:res 5,(ix+#12):res 5,(ix+#12),a::" \
                         "res 6,(ix+#12),b:res 6,(ix+#12),c:res 6,(ix+#12),d:res 6,(ix+#12),e:res 6,(ix+#12),h:" \
                         "res 6,(ix+#12),l:res 6,(ix+#12):res 6,(ix+#12),a::res 7,(ix+#12),b:res 7,(ix+#12),c:" \
                         "res 7,(ix+#12),d:res 7,(ix+#12),e:res 7,(ix+#12),h:res 7,(ix+#12),l:res 7,(ix+#12):" \
                         "res 7,(ix+#12),a::set 0,(ix+#12),b:set 0,(ix+#12),c:set 0,(ix+#12),d:set 0,(ix+#12),e:" \
                         "set 0,(ix+#12),h:set 0,(ix+#12),l:set 0,(ix+#12):set 0,(ix+#12),a::set 1,(ix+#12),b:" \
                         "set 1,(ix+#12),c:set 1,(ix+#12),d:set 1,(ix+#12),e:set 1,(ix+#12),h:set 1,(ix+#12),l:" \
                         "set 1,(ix+#12):set 1,(ix+#12),a::set 2,(ix+#12),b:set 2,(ix+#12),c:set 2,(ix+#12),d:" \
                         "set 2,(ix+#12),e:set 2,(ix+#12),h:set 2,(ix+#12),l:set 2,(ix+#12):set 2,(ix+#12),a::" \
                         "set 3,(ix+#12),b:set 3,(ix+#12),c:set 3,(ix+#12),d:set 3,(ix+#12),e:set 3,(ix+#12),h:" \
                         "set 3,(ix+#12),l:set 3,(ix+#12):set 3,(ix+#12),a::set 4,(ix+#12),b:set 4,(ix+#12),c:" \
                         "set 4,(ix+#12),d:set 4,(ix+#12),e:set 4,(ix+#12),h:set 4,(ix+#12),l:set 4,(ix+#12):" \
                         "set 4,(ix+#12),a::set 5,(ix+#12),b:set 5,(ix+#12),c:set 5,(ix+#12),d:set 5,(ix+#12),e:" \
                         "set 5,(ix+#12),h:set 5,(ix+#12),l:set 5,(ix+#12):set 5,(ix+#12),a::set 6,(ix+#12),b:" \
                         "set 6,(ix+#12),c:set 6,(ix+#12),d:set 6,(ix+#12),e:set 6,(ix+#12),h:set 6,(ix+#12),l:" \
                         "set 6,(ix+#12):set 6,(ix+#12),a::set 7,(ix+#12),b:set 7,(ix+#12),c:set 7,(ix+#12),d:" \
                         "set 7,(ix+#12),e:set 7,(ix+#12),h:set 7,(ix+#12),l:set 7,(ix+#12):set 7,(ix+#12),a::add iy,bc::" \
                         "add iy,de::ld iy,#1234:ld (#1234),iy:inc iy:inc yh:dec yh:ld yh,#12:add iy,iy:ld iy,(#1234):dec iy:" \
                         "inc yl:dec yl:ld yl,#12::inc (iy+#12):dec (iy+#12):ld (iy+#12),#34:add iy,sp::ld b,yh:ld b,yl:" \
                         "ld b,(iy+#12):ld c,yh:ld c,yl:ld c,(iy+#12):::ld d,yh:ld d,yl:ld d,(iy+#12):ld e,yh:ld e,yl:" \
                         "ld e,(iy+#12)::ld yh,b:ld yh,c:ld yh,d:ld yh,e:ld yh,yh:ld yh,yl:ld h,(iy+#12):ld yh,a:ld yl,b:" \
                         "ld yl,c:ld yl,d:ld yl,e:ld yl,yh:ld yl,yl:ld l,(iy+#12):ld yl,a::ld (iy+#12),b:ld (iy+#12),c:" \
                         "ld (iy+#12),d:ld (iy+#12),e:ld (iy+#12),h:ld (iy+#12),l:ld (iy+#12),a:ld a,yh:ld a,yl:" \
                         "ld a,(iy+#12)::add yh:add yl:add (iy+#12):adc yh:adc yl:adc (iy+#12)::sub yh:sub yl:" \
                         "sub (iy+#12):sbc yh:sbc yl:sbc (iy+#12)::and yh:and yl:and (iy+#12):xor yh:xor yl:xor (iy+#12)::" \
                         "or yh:or yl:or (iy+#12):cp yh:cp yl:cp (iy+#12)::pop iy:ex (sp),iy:push iy:jp (iy)::ld sp,iy::" \
                         "rlc (iy+#12),b:rlc (iy+#12),c:rlc (iy+#12),d:rlc (iy+#12),e:rlc (iy+#12),h:rlc (iy+#12),l:" \
                         "rlc (iy+#12):rlc (iy+#12),a:rrc (iy+#12),b:rrc (iy+#12),c:rrc (iy+#12),d:rrc (iy+#12),e:" \
                         "rrc (iy+#12),h:rrc (iy+#12),l:rrc (iy+#12):rrc (iy+#12),a::rl (iy+#12),b:rl (iy+#12),c:" \
                         "rl (iy+#12),d:rl (iy+#12),e:rl (iy+#12),h:rl (iy+#12),l:rl (iy+#12):rl (iy+#12),a:rr (iy+#12),b:" \
                         "rr (iy+#12),c:rr (iy+#12),d:rr (iy+#12),e:rr (iy+#12),h:rr (iy+#12),l:rr (iy+#12):rr (iy+#12),a::" \
                         "sla (iy+#12),b:sla (iy+#12),c:sla (iy+#12),d:sla (iy+#12),e:sla (iy+#12),h:sla (iy+#12),l:" \
                         "sla (iy+#12):sla (iy+#12),a:sra (iy+#12),b:sra (iy+#12),c:sra (iy+#12),d:sra (iy+#12),e:" \
                         "sra (iy+#12),h:sra (iy+#12),l:sra (iy+#12):sra (iy+#12),a::sll (iy+#12),b:sll (iy+#12),c:" \
                         "sll (iy+#12),d:sll (iy+#12),e:sll (iy+#12),h:sll (iy+#12),l:sll (iy+#12):sll (iy+#12),a:" \
                         "srl (iy+#12),b:srl (iy+#12),c:srl (iy+#12),d:srl (iy+#12),e:srl (iy+#12),h:srl (iy+#12),l:" \
                         "srl (iy+#12):srl (iy+#12),a::bit 0,(iy+#12):bit 1,(iy+#12):bit 2,(iy+#12):bit 3,(iy+#12):" \
                         "bit 4,(iy+#12):bit 5,(iy+#12):bit 6,(iy+#12):bit 7,(iy+#12)::res 0,(iy+#12),b:res 0,(iy+#12),c:" \
                         "res 0,(iy+#12),d:res 0,(iy+#12),e:res 0,(iy+#12),h:res 0,(iy+#12),l:res 0,(iy+#12):" \
                         "res 0,(iy+#12),a::res 1,(iy+#12),b:res 1,(iy+#12),c:res 1,(iy+#12),d:res 1,(iy+#12),e:" \
                         "res 1,(iy+#12),h:res 1,(iy+#12),l:res 1,(iy+#12):res 1,(iy+#12),a::res 2,(iy+#12),b:" \
                         "res 2,(iy+#12),c:res 2,(iy+#12),d:res 2,(iy+#12),e:res 2,(iy+#12),h:res 2,(iy+#12),l:" \
                         "res 2,(iy+#12):res 2,(iy+#12),a::res 3,(iy+#12),b:res 3,(iy+#12),c:res 3,(iy+#12),d:" \
                         "res 3,(iy+#12),e:res 3,(iy+#12),h:res 3,(iy+#12),l:res 3,(iy+#12):res 3,(iy+#12),a::" \
                         "res 4,(iy+#12),b:res 4,(iy+#12),c:res 4,(iy+#12),d:res 4,(iy+#12),e:res 4,(iy+#12),h:" \
                         "res 4,(iy+#12),l:res 4,(iy+#12):res 4,(iy+#12),a::res 5,(iy+#12),b:res 5,(iy+#12),c:" \
                         "res 5,(iy+#12),d:res 5,(iy+#12),e:res 5,(iy+#12),h:res 5,(iy+#12),l:res 5,(iy+#12):" \
                         "res 5,(iy+#12),a::res 6,(iy+#12),b:res 6,(iy+#12),c:res 6,(iy+#12),d:res 6,(iy+#12),e:" \
                         "res 6,(iy+#12),h:res 6,(iy+#12),l:res 6,(iy+#12):res 6,(iy+#12),a::res 7,(iy+#12),b:" \
                         "res 7,(iy+#12),c:res 7,(iy+#12),d:res 7,(iy+#12),e:res 7,(iy+#12),h:res 7,(iy+#12),l:" \
                         "res 7,(iy+#12):res 7,(iy+#12),a::set 0,(iy+#12),b:set 0,(iy+#12),c:set 0,(iy+#12),d:" \
                         "set 0,(iy+#12),e:set 0,(iy+#12),h:set 0,(iy+#12),l:set 0,(iy+#12):set 0,(iy+#12),a::" \
                         "set 1,(iy+#12),b:set 1,(iy+#12),c:set 1,(iy+#12),d:set 1,(iy+#12),e:set 1,(iy+#12),h:" \
                         "set 1,(iy+#12),l:set 1,(iy+#12):set 1,(iy+#12),a::set 2,(iy+#12),b:set 2,(iy+#12),c:" \
                         "set 2,(iy+#12),d:set 2,(iy+#12),e:set 2,(iy+#12),h:set 2,(iy+#12),l:set 2,(iy+#12):" \
                         "set 2,(iy+#12),a::set 3,(iy+#12),b:set 3,(iy+#12),c:set 3,(iy+#12),d:set 3,(iy+#12),e:" \
                         "set 3,(iy+#12),h:set 3,(iy+#12),l:set 3,(iy+#12):set 3,(iy+#12),a::set 4,(iy+#12),b:" \
                         "set 4,(iy+#12),c:set 4,(iy+#12),d:set 4,(iy+#12),e:set 4,(iy+#12),h:set 4,(iy+#12),l:" \
                         "set 4,(iy+#12):set 4,(iy+#12),a::set 5,(iy+#12),b:set 5,(iy+#12),c:set 5,(iy+#12),d:" \
                         "set 5,(iy+#12),e:set 5,(iy+#12),h:set 5,(iy+#12),l:set 5,(iy+#12):set 5,(iy+#12),a::" \
                         "set 6,(iy+#12),b:set 6,(iy+#12),c:set 6,(iy+#12),d:set 6,(iy+#12),e:set 6,(iy+#12),h:" \
                         "set 6,(iy+#12),l:set 6,(iy+#12):set 6,(iy+#12),a::set 7,(iy+#12),b:set 7,(iy+#12),c:" \
                         "set 7,(iy+#12),d:set 7,(iy+#12),e:set 7,(iy+#12),h:set 7,(iy+#12),l:set 7,(iy+#12):" \
                         "set 7,(iy+#12),a:"

#define AUTOTEST_LABNUM 	"mavar=67:label{mavar}truc:ld hl,7+2*label{mavar}truc:mnt=1234567:lab2{mavar}{mnt}:" \
				"ld de,lab2{mavar}{mnt}:lab3{mavar}{mnt}h:ld de,lab3{mavar}{mnt}h"

#define AUTOTEST_EQUNUM		"mavar = 9:monlabel{mavar+5}truc:unalias{mavar+5}heu equ 50:autrelabel{unalias14heu}:ld hl,autrelabel50"

#define AUTOTEST_DELAYNUM	"macro test  label:    dw {label}:    endm:    repeat 3, idx:idx2 = idx-1:" \
				" test label_{idx2}:    rend:repeat 3, idx:label_{idx-1}:nop:rend"

#define AUTOTEST_STRUCT		"org #1000:label1 :struct male:age    defb 0:height defb 0:endstruct:struct female:" \
				"age    defb 0:height defb 0:endstruct:struct couple:struct male husband:" \
				"struct female wife:endstruct:if $-label1!=0:stop:endif:ld a,(ix+couple.wife.age):" \
				"ld bc,couple:ld bc,{sizeof}couple:struct couple mycouple:" \
				"if mycouple.husband != mycouple.husband.age:stop:endif:ld hl,mycouple:" \
				"ld hl,mycouple.wife.age:ld bc,{sizeof}mycouple:macro cmplastheight p1:" \
				"ld hl,@mymale.height:ld a,{p1}:cp (hl):ld bc,{sizeof}@mymale:ld hl,@mymale:ret:" \
				"struct male @mymale:mend:cmplastheight 5:cmplastheight 3:nop"

#define AUTOTEST_STRUCT2	"struct bite: preums defb 0: deuze defw 1: troize defs 10: endstruct:" \
				" if {sizeof}bite.preums!=1 || {sizeof}bite.deuze!=2 || {sizeof}bite.troize!=10: stop: endif: nop "

#define AUTOTEST_REPEAT		"ce=100:repeat 2,ce:repeat 5,cx:repeat 5,cy:defb cx*cy:rend:rend:rend:assert cx==6 && cy==6 && ce==3:" \
							"cpt=0:repeat:cpt=cpt+1:until cpt>4:assert cpt==5"

#define AUTOTEST_TICKER		"repeat 2: ticker start, mc:out (0),a:out (c),a:out (c),h:out (c),0:ticker stop, mc:if mc!=15:ld hl,bite:else:nop:endif:rend"

#define AUTOTEST_ORG		"ORG #8000,#1000:defw $:ORG $:defw $"

#define AUTOTEST_BANKORG	"bank 0:nop:org #5:nop:bank 1:unevar=10:bank 0:assert $==6:ret:bank 1:assert $==0:bank 0:assert $==7"

#define AUTOTEST_VAREQU		"label1 equ #C000:label2 equ (label1*2)/16:label3 equ label1-label2:label4 equ 15:var1=50*3+2:var2=12*label1:var3=label4-8:var4=label2:nop"

#define AUTOTEST_FORMAT		"hexa=#12A+$23B+45Ch+0x56D:deci=123.45+-78.54*2-(7-7)*2:bina=0b101010+1010b-%1111:assert hexa==3374 && deci==-33.63 && bina==37:nop"

#define AUTOTEST_CHARSET	"charset 'abcde',0:defb 'abcde':defb 'a','b','c','d','e':defb 'a',1*'b','c'*1,1*'d','e'*1:charset:" \
							"defb 'abcde':defb 'a','b','c','d','e':defb 'a',1*'b','c'*1,1*'d','e'*1"

#define AUTOTEST_CHARSET2	"charset 97,97+26,0:defb 'roua':charset:charset 97,10:defb 'roua':charset 'o',5:defb 'roua':charset 'ou',6:defb 'roua'"

#define AUTOTEST_NOCODE		"let monorg=$:NoCode:Org 0:Element1 db 0:Element2 dw 3:Element3 ds 50:Element4 defb 'rdd':Org 0:pouet defb 'nop':" \
							"Code:Org monorg:cpt=$+element2+element3+element4:defs cpt,0"

#define AUTOTEST_LZSEGMENT	"org #100:debut:jr nz,zend:lz48:repeat 128:nop:rend:lzclose:jp zend:lz48:repeat 2:dec a:jr nz,@next:ld a,5:@next:jp debut:rend:" \
							"lzclose:zend"

#define AUTOTEST_PAGETAG	"bankset 0:org #5000:label1:bankset 1:org #9000:label2:bankset 2:" \
							"assert {page}label1==0xC0:assert {page}label2==0xC6:assert {pageset}label1==#C0:assert {pageset}label2==#C2:nop"
							
#define AUTOTEST_PAGETAG2	"bankset 0:call maroutine:bank 4:org #C000:autreroutine:nop:" \
							"ret:bank 5:org #8000:maroutine:ldir:ret:bankset 2:org #9000:troize:nop:" \
							"assert {page}maroutine==#C5:assert {pageset}maroutine==#C2:assert {page}autreroutine==#C4:" \
							"assert {pageset}autreroutine==#C2:assert {page}troize==#CE:assert {pageset}troize==#CA"							
							
#define AUTOTEST_PAGETAG3	"buildsna:bank 2:assert {bank}$==2:assert {page}$==0xC0:assert {pageset}$==#C0:" \
							"bankset 1:org #4000:assert {bank}$==5:assert {page}$==0xC5:assert {pageset}$==#C2"
							
#define AUTOTEST_SWITCH		"mavar=4:switch mavar:case 1:nop:case 4:defb 4:case 3:defb 3:break:case 2:nop:case 4:defb 4:endswitch"

#define AUTOTEST_PREPRO1	" macro DEBUG_INK0, coul:        out (c), a: endm: ifndef NDEBUG:DEBUG_BORDER_NOPS = 0: endif"

#define AUTOTEST_PREPRO2	" nop : mycode other tartine ldir : defw tartine,other, mycode : assert tartine==other && other==mycode && mycode==1 && $==9"

#define AUTOTEST_PREPRO3	"nop\n \n /* test ' ici */\n nop /* retest */ 5\n /* bonjour\n prout\n caca\n pipi */" \
				" nop\n nop /* nop */ : nop\n ; grouik /*\n \n /* ; pouet */ ld hl,#2121\n "

#define AUTOTEST_PREPRO4	"glop=0\n nop ; glop=1\n assert glop==0\n nop\n glop=0\n nop // glop=1\n assert glop==0\n nop\n glop=0 : /* glop=1 */ nop\n" \
				"assert glop==0\n nop\n \n glop=0 : /* glop=1 // */ nop\n assert glop==0\n nop\n glop=0 : /* glop=1 ; */ nop\n" \
				"assert glop==0\n nop\n glop=0 ; /* glop=1\n nop // glop=2 /*\n assert glop==0\n nop\n"
							
#define AUTOTEST_PROXIM		"routine:.step1:jp .step2:.step2:jp .step1:deuze:nop:.step1:djnzn .step1:djnz routine.step2"							

#define AUTOTEST_TAGPRINT	"unevar=12:print 'trucmuche{unevar}':print '{unevar}':print '{unevar}encore','pouet{unevar}{unevar}':ret"

#define AUTOTEST_TAGFOLLOW	"ret:uv=1234567890:unlabel_commeca_{uv} equ pouetpouetpouettroulala:pouetpouetpouettroulala:assert unlabel_commeca_{uv}>0"

#define AUTOTEST_TAGREALLOC	"zoomscroller_scroller_height equ 10: another_super_long_equ equ 256: pinouille_super_long_useless_equ equ 0: " \
				"zz equ 1111111111: org #100: repeat zoomscroller_scroller_height,idx: " \
				"zoomscroller_buffer_line_{idx-1}_{pinouille_super_long_useless_equ} nop: zoomscroller_buffer_line_{idx-1}_{zz} nop: " \
				"rend: align 256: repeat zoomscroller_scroller_height,idx: zoomscroller_buffer_line_{idx-1}_{pinouille_super_long_useless_equ}_duplicate nop: " \
				"zoomscroller_buffer_line_{idx-1}_{zz}_duplicate nop: rend: repeat zoomscroller_scroller_height, line: idx = line - 1: " \
				"assert zoomscroller_buffer_line_{idx}_{pinouille_super_long_useless_equ} + another_super_long_equ == "\
				"zoomscroller_buffer_line_{idx}_{pinouille_super_long_useless_equ}_duplicate: " \
				"assert zoomscroller_buffer_line_{idx}_{zz} + another_super_long_equ == zoomscroller_buffer_line_{idx}_{zz}_duplicate: rend"

#define AUTOTEST_DEFUSED	"ld hl,labelused :ifdef labelused:fail 'labelexiste':endif:ifndef labelused:else:fail 'labelexiste':endif:" \
				"ifnused labelused:fail 'labelused':endif:ifused labelused:else:fail 'labelused':endif:labelused"

#define AUTOTEST_MACROPROX	" macro unemacro: nop: endm: global_label: ld hl, .table: .table"

#define AUTOTEST_QUOTES		"defb 'rdd':str 'rdd':charset 'rd',0:defb '\\r\\d':str '\\r\\d'"

#define AUTOTEST_NEGATIVE	"ld a,-5: ld bc,-0x3918:ld de,0+-#3918:ld de,-#3918:var1=-5+6:var2=-#3918+#3919:assert var1+var2==2"

#define AUTOTEST_FORMULA1	"a=5:b=2:assert int(a/b)==3:assert !a+!b==0:a=a*100:b=b*100:assert a*b==100000:ld hl,a*b-65536:a=123+-5*(-6/2)-50*2<<1"

#define AUTOTEST_FORMULA2 "vala= (0.5+(4*0.5))*6:valb= int((0.5+(4*0.5))*6):nop:if vala!=valb:push erreur:endif"

/* test override control between bank and bankset in snapshot mode + temp workspace */
#define AUTOTEST_BANKSET "buildsna:bank 0:nop:bank 1:nop:bank:nop:bank 2:nop:bank 3:nop:bankset 1:nop:bank 8:nop:bank 9:nop:bank 10:nop:bank 11:nop"

#define AUTOTEST_LIMITOK "org #100:limit #102:nop:limit #103:ld a,0:protect #105,#107:limit #108:xor a:org $+3:inc a" 

#define AUTOTEST_LIMITKO	"limit #100:org #100:add ix,ix"

#define AUTOTEST_LIMIT03	"limit -1 : nop"
#define AUTOTEST_LIMIT04	"limit #10000 : nop"
#define AUTOTEST_LIMIT05	"org #FFFF : ldir"
#define AUTOTEST_LIMIT06	"org #FFFF : nop"

#define AUTOTEST_LIMIT07	"org #ffff : Start:  equ $ :    di :     ld hl,#c9fb :  ld (#38),hl"

#define AUTOTEST_INHIBITION	"if 0:ifused truc:ifnused glop:ifdef bidule:ifndef machin:ifnot 1:nop:endif:nop:else:nop:endif:endif:endif:endif:endif"

#define AUTOTEST_LZ4	"lz4:repeat 10:nop:rend:defb 'roudoudoudouoneatxkjhgfdskljhsdfglkhnopnopnopnop':lzclose"

#define AUTOTEST_ENHANCED_LD	"ld h,(ix+11): ld l,(ix+10): ld h,(iy+21): ld l,(iy+20): ld b,(ix+11): ld c,(ix+10):" \
			"ld b,(iy+21): ld c,(iy+20): ld d,(ix+11): ld e,(ix+10): ld d,(iy+21): ld e,(iy+20): ld hl,(ix+10): " \
			"ld hl,(iy+20):ld bc,(ix+10):ld bc,(iy+20): ld de,(ix+10):ld de,(iy+20)"

#define AUTOTEST_ENHANCED_PUSHPOP	"push bc,de,hl,ix,iy,af:pop hl,bc,de,iy,ix,af:nop 2:" \
				"push bc:push de:push hl:push ix:push iy:push af:"\
				"pop hl:pop bc:pop de:pop iy:pop ix:pop af:nop:nop"
#define AUTOTEST_ENHANCED_LD2 "ld (ix+0),hl: ld (ix+0),de: ld (ix+0),bc: ld (iy+0),hl: ld (iy+0),de: ld (iy+0),bc:"\
"ld (ix+1),h: ld (ix+0),l: ld (ix+1),d: ld (ix+0),e: ld (ix+1),b: ld (ix+0),c: ld (iy+1),h: ld (iy+0),l: ld (iy+1),d: ld (iy+0),e: ld (iy+1),b: ld (iy+0),c"

#define AUTOTEST_INHIBITION2 "ifdef roudoudou:macro glop bank,page,param:ld a,{bank}:ld hl,{param}{bank}:if {bank}:nop:else:exx:" \
	"endif::switch {param}:nop:case 4:nop:case {param}:nop:default:nop:break:endswitch:endif:defb 'coucou'"

#define AUTOTEST_INHIBITIONMAX "roudoudou:ifndef roudoudou:if pouet:macro glop bank,page,param:ifdef nanamouskouri:ld hl,{param}{bank}:"\
	"elseif aglapi:exx:endif:if {bank}:nop:elseif {grouik}:exx:endif:switch {bite}:nop:case {nichon}:nop:default:nop:break:endswitch:else:"\
	"ifnot {jojo}:exx:endif:endif:else:defb 'coucou':endif"
	
void MiniDump(unsigned char *opcode, int opcodelen) {
	#undef FUNC
	#define FUNC "MiniDump"

	int i;
	printf("%d byte%s to dump\n",opcodelen,opcodelen>1?"s":"");
	for (i=0;i<opcodelen;i++) {
		printf("%02X \n",opcode[i]);
	}
	printf("\n");
}
						
void RasmAutotest(void)
{
	#undef FUNC
	#define FUNC "RasmAutotest"

	unsigned char *opcode=NULL;
	int opcodelen,ret,filelen;
	int cpt=0,chk,i;
	char tmpstr1[256],tmpstr2[256],*tmpstr3,**tmpsplit;

#ifdef RDD
	printf("\n%d bytes\n",_static_library_memory_used);
#endif
#if 0
	/* Autotest CORE */
	#ifdef OS_WIN
	printf(".");fflush(stdout);
	strcpy(tmpstr1,".\\archives\\job.asm");
	strcpy(tmpstr2,".\\archives\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath0) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,".\\archives\\..\\job.asm");
	strcpy(tmpstr2,".\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath1) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,".\\archives\\gruik\\..\\..\\job.asm");
	strcpy(tmpstr2,".\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath2) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,".\\archives\\..\\gruik\\..\\job.asm");
	strcpy(tmpstr2,".\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath3) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"..\\archives\\job.asm");
	strcpy(tmpstr2,"..\\archives\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath4) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}	
	strcpy(tmpstr1,".\\src\\..\\..\\src\\job.asm");
	strcpy(tmpstr2,"..\\src\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath5) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"src\\..\\..\\src\\job.asm");
	strcpy(tmpstr2,"..\\src\\job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath6) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	cpt++;
	#else
	printf(".");fflush(stdout);
	strcpy(tmpstr1,"./archives/job.asm");
	strcpy(tmpstr2,"./archives/job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath0) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"./archives/../job.asm");
	strcpy(tmpstr2,"./job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath1) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"./archives/gruik/../../job.asm");
	strcpy(tmpstr2,"./job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath2) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"./archives/../gruik/../job.asm");
	strcpy(tmpstr2,"./job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath3) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"../archives/job.asm");
	strcpy(tmpstr2,"../archives/job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath4) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}	
	strcpy(tmpstr1,"./src/../../src/job.asm");
	strcpy(tmpstr2,"../src/job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath5) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	strcpy(tmpstr1,"src/../../../src/job.asm");
	strcpy(tmpstr2,"../../src/job.asm");
	SimplifyPath(tmpstr1);if (strcmp(tmpstr1,tmpstr2)) {printf("Autotest %03d ERROR (Core:SimplifyPath6) %s!=%s\n",cpt,tmpstr1,tmpstr2);exit(-1);}
	cpt++;
	printf(".");fflush(stdout);
	if (strcmp(GetPath("/home/roudoudou/"),"/home/roudoudou/")) {printf("Autotest %03d ERROR (Core:GetPath0) [%s]\n",cpt,GetPath("/home/roudoudou/"));exit(-1);}
	if (strcmp(GetPath("/home/roudoudou"),"/home/")) {printf("Autotest %03d ERROR (Core:GetPath1) [%s]\n",cpt,GetPath("/home/roudoudou"));exit(-1);}
	cpt++;
	#endif
#endif
	
	/* Autotest preprocessing */
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_VIRGULE,strlen(AUTOTEST_VIRGULE),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (double comma must trigger an error)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_VIRGULE2,strlen(AUTOTEST_VIRGULE2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (double comma in a string must be OK)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PREPRO1,strlen(AUTOTEST_PREPRO1),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (freestyle case 1)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PREPRO2,strlen(AUTOTEST_PREPRO2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (freestyle case 2)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PREPRO3,strlen(AUTOTEST_PREPRO3),&opcode,&opcodelen);
	if (!ret && opcodelen==12 && opcode[11]==0x21) {} else {printf("Autotest %03d ERROR (multi-line comment)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PREPRO4,strlen(AUTOTEST_PREPRO4),&opcode,&opcodelen);
	if (!ret && opcodelen==12) {} else {printf("Autotest %03d ERROR (multi-line comment 2)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_OPERATOR_CONVERSION,strlen(AUTOTEST_OPERATOR_CONVERSION),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (maxam operator conversion)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_NOINCLUDE,strlen(AUTOTEST_NOINCLUDE),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (include missing file)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_FORMAT,strlen(AUTOTEST_FORMAT),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (digit formats)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	/* Autotest assembling */
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_OPCODES,strlen(AUTOTEST_OPCODES),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (all opcodes)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	/* Autotest single instruction writes that must failed */
	printf(".");fflush(stdout);
	tmpstr3=TxtStrDup(AUTOTEST_INSTRMUSTFAILED);
	tmpsplit=TxtSplitWithChar(tmpstr3,':');

	for (i=0;tmpsplit[i];i++) {
		ret=RasmAssemble(tmpsplit[i],strlen(tmpsplit[i]),&opcode,&opcodelen);
		if (ret) {} else {printf("Autotest %03d ERROR (opcodes that must fail) -> [%s]\n",cpt,tmpsplit[i]);exit(-1);}
		if (opcode) MemFree(opcode);opcode=NULL;
	}

	MemFree(tmpstr3);FreeFields(tmpsplit);
	cpt++;

	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_ORG,strlen(AUTOTEST_ORG),&opcode,&opcodelen);
	if (!ret && opcodelen==4 && opcode[1]==0x80 && opcode[2]==2 && opcode[3]==0x10) {} else {printf("Autotest %03d ERROR (ORG relocation)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_BANKORG,strlen(AUTOTEST_BANKORG),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (BANK org adr)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMITOK,strlen(AUTOTEST_LIMITOK),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (limit ok)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMITKO,strlen(AUTOTEST_LIMITKO),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (out of limit)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMIT03,strlen(AUTOTEST_LIMIT03),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (limit: negative limit)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMIT04,strlen(AUTOTEST_LIMIT04),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (limit: max limit test)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMIT05,strlen(AUTOTEST_LIMIT05),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (limit: ldir in #FFFF)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMIT06,strlen(AUTOTEST_LIMIT06),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (limit: nop in #FFFF)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LIMIT07,strlen(AUTOTEST_LIMIT07),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (limit: ld hl,#1234 in #FFFF)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LZSEGMENT,strlen(AUTOTEST_LZSEGMENT),&opcode,&opcodelen);
	if (!ret && opcodelen==23 && opcode[1]==21 && opcode[9]==23) {} else {printf("Autotest %03d ERROR (LZsegment relocation)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LZ4,strlen(AUTOTEST_LZ4),&opcode,&opcodelen);
	if (!ret && opcodelen==49 && opcode[0]==0x15 && opcode[4]==0x44 && opcode[0xB]==0xF0) {} else {printf("Autotest %03d ERROR (LZ4 segment)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_BANKSET,strlen(AUTOTEST_BANKSET),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (bank/bankset)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PAGETAG,strlen(AUTOTEST_PAGETAG),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (page/pageset)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PAGETAG2,strlen(AUTOTEST_PAGETAG2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (page/pageset 2)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PAGETAG3,strlen(AUTOTEST_PAGETAG3),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (page/pageset 3)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_UNDEF,strlen(AUTOTEST_UNDEF),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (simple undef)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_TAGPRINT,strlen(AUTOTEST_TAGPRINT),&opcode,&opcodelen);
	if (!ret && opcodelen==1 && opcode[0]==0xC9) {} else {printf("Autotest %03d ERROR (tag inside printed string)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_TAGFOLLOW,strlen(AUTOTEST_TAGFOLLOW),&opcode,&opcodelen);
	if (!ret && opcodelen==1 && opcode[0]==0xC9) {} else {printf("Autotest %03d ERROR (tag+alias fast translating)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_TAGREALLOC,strlen(AUTOTEST_TAGREALLOC),&opcode,&opcodelen);
	if (!ret && opcodelen==276) {} else {printf("Autotest %03d ERROR (tag realloc with fast translate)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PRINTVAR,strlen(AUTOTEST_PRINTVAR),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (param inside printed string)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PRINTSPACE,strlen(AUTOTEST_PRINTSPACE),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (space inside tag string for PRINT directive)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_INHIBITION,strlen(AUTOTEST_INHIBITION),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (conditionnal inhibition)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_SWITCH,strlen(AUTOTEST_SWITCH),&opcode,&opcodelen);
	if (!ret && opcodelen==3 && opcode[0]==4 && opcode[1]==3 && opcode[2]==4) {} else {printf("Autotest %03d ERROR (switch case)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_NOCODE,strlen(AUTOTEST_NOCODE),&opcode,&opcodelen);
	if (!ret && opcodelen==57) {} else {printf("Autotest %03d ERROR (code/nocode)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_VAREQU,strlen(AUTOTEST_VAREQU),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (var & equ)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_CHARSET,strlen(AUTOTEST_CHARSET),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (simple charset)\n",cpt);exit(-1);}
	if (opcodelen!=30 || memcmp(opcode,opcode+5,5) || memcmp(opcode+10,opcode+5,5)) {printf("Autotest %03d ERROR (simple charset)\n",cpt);exit(-1);}
	if (memcmp(opcode+15,opcode+20,5) || memcmp(opcode+15,opcode+25,5)) {printf("Autotest %03d ERROR (simple charset)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_CHARSET2,strlen(AUTOTEST_CHARSET2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (extended charset)\n",cpt);exit(-1);}
	for (i=chk=0;i<opcodelen;i++) chk+=opcode[i];
	if (opcodelen!=16 || chk!=0x312) {printf("Autotest %03d ERROR (extended charset)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_QUOTES,strlen(AUTOTEST_QUOTES),&opcode,&opcodelen);
	if (!ret && opcodelen==10 && opcode[5]==0xE4 && opcode[6]==0x0D && opcode[7]==0x64 && opcode[8]==0x0D && opcode[9]==0xE4) {}
		else {printf("Autotest %03d ERROR (quotes & escaped chars)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_NOT,strlen(AUTOTEST_NOT),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (not operator)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_MACRO,strlen(AUTOTEST_MACRO),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (macro usage)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_MACROPAR,strlen(AUTOTEST_MACROPAR),&opcode,&opcodelen);
	if (!ret && opcodelen==12 && memcmp(opcode,"GROUIKgrouik",12)==0) {} else {printf("Autotest %03d ERROR (macro string param)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_MACRO_ADV,strlen(AUTOTEST_MACRO_ADV),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (macro param)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_OVERLOADMACPRM,strlen(AUTOTEST_OVERLOADMACPRM),&opcode,&opcodelen);
	if (!ret && opcodelen==4 && opcode[0]==1 && opcode[1]==0 && opcode[2]==2 && opcode[3]==1) {} else {printf("Autotest %03d ERROR (macro param overload)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_LABNUM,strlen(AUTOTEST_LABNUM),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (variables in labels)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_EQUNUM,strlen(AUTOTEST_EQUNUM),&opcode,&opcodelen);
	if (!ret && opcodelen==3) {} else {printf("Autotest %03d ERROR (variables in aliases) r=%d l=%d\n",cpt,ret,opcodelen);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_DELAYNUM,strlen(AUTOTEST_DELAYNUM),&opcode,&opcodelen);
	if (!ret && opcodelen==9 && opcode[0]==6 && opcode[2]==7 && opcode[4]==8) {} else {printf("Autotest %03d ERROR (delayed expr labels) r=%d l=%d\n",cpt,ret,opcodelen);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_PROXIM,strlen(AUTOTEST_PROXIM),&opcode,&opcodelen);
	if (!ret && opcode[1]==3) {} else {printf("Autotest %03d ERROR (proximity labels)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_STRUCT,strlen(AUTOTEST_STRUCT),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (structs)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_STRUCT2,strlen(AUTOTEST_STRUCT2),&opcode,&opcodelen);
	if (!ret && opcodelen==1) {} else {printf("Autotest %03d ERROR (sizeof struct fields)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_REPEAT,strlen(AUTOTEST_REPEAT),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (extended repeat)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_TICKER,strlen(AUTOTEST_TICKER),&opcode,&opcodelen);
	if (!ret && opcodelen==18) {} else {printf("Autotest %03d ERROR (ticker (re)count)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_DEFUSED,strlen(AUTOTEST_DEFUSED),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (ifdef ifused)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_INHIBITION2,strlen(AUTOTEST_INHIBITION2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (if switch inhibition)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_INHIBITIONMAX,strlen(AUTOTEST_INHIBITIONMAX),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (moar inhibition)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_MACROPROX,strlen(AUTOTEST_MACROPROX),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (macro + prox)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_NEGATIVE,strlen(AUTOTEST_NEGATIVE),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (formula case 0)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_FORMULA1,strlen(AUTOTEST_FORMULA1),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (formula case 1)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_SETINSIDE,strlen(AUTOTEST_SETINSIDE),&opcode,&opcodelen);
	if (ret) {} else {printf("Autotest %03d ERROR (set var inside expression must trigger error)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_FORMULA2,strlen(AUTOTEST_FORMULA2),&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (formula case 2 function+multiple parenthesis)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_ENHANCED_LD,strlen(AUTOTEST_ENHANCED_LD),&opcode,&opcodelen);
	if (!ret && memcmp(opcode,opcode+opcodelen/2,opcodelen/2)==0) {} else {printf("Autotest %03d ERROR (enhanced LD)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_ENHANCED_LD2,strlen(AUTOTEST_ENHANCED_LD2),&opcode,&opcodelen);
	if (!ret && memcmp(opcode,opcode+opcodelen/2,opcodelen/2)==0) {} else {printf("Autotest %03d ERROR (enhanced LD 2)\n",cpt);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;
	
	printf(".");fflush(stdout);ret=RasmAssemble(AUTOTEST_ENHANCED_PUSHPOP,strlen(AUTOTEST_ENHANCED_PUSHPOP),&opcode,&opcodelen);
	if (!ret && memcmp(opcode,opcode+opcodelen/2,opcodelen/2)==0) {} else {printf("Autotest %03d ERROR (enhanced PUSH/POP/NOP)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;

#ifdef RDD
	printf("\n%d bytes\n",_static_library_memory_used);

	tmpstr3=FileReadContent("./test/PlayerAky.asm",&filelen);
	printf(".");fflush(stdout);ret=RasmAssemble(tmpstr3,filelen,&opcode,&opcodelen);
	if (!ret) {} else {printf("Autotest %03d ERROR (PlayerAky)\n",cpt);MiniDump(opcode,opcodelen);exit(-1);}
	if (opcode) MemFree(opcode);opcode=NULL;cpt++;


	printf("\n%d bytes\n",_static_library_memory_used);
#endif
	
	printf("All internal tests OK\n");
	#ifdef RDD
	/* private dev lib tools */
printf("checking memory\n");
	CloseLibrary();
	#endif
	exit(0);
}


/******************************************************
LZ48 v005 / LZ49 v002
******************************************************/
int LZ48_encode_extended_length(unsigned char *odata, int length)
{
	int ioutput=0;

	while (length>=255) {
		odata[ioutput++]=0xFF;
		length-=255;
	}
	/* if the last value is 255 we must encode 0 to end extended length */
	/*if (length==0) rasm_printf(ae,"bugfixed!\n");*/
	odata[ioutput++]=(unsigned char)length;
	return ioutput;
}

int LZ48_encode_block(unsigned char *odata,unsigned char *data, int literaloffset,int literalcpt,int offset,int maxlength)
{
	int ioutput=1;
	int token=0;
	int i;

	if (offset<0 || offset>255) {
		fprintf(stderr,"internal offset error!\n");
		exit(-2);
	}
	
	if (literalcpt<15) {
		token=literalcpt<<4; 
	} else {
		token=0xF0;
		ioutput+=LZ48_encode_extended_length(odata+ioutput,literalcpt-15);
	}

	for (i=0;i<literalcpt;i++) odata[ioutput++]=data[literaloffset++];

	if (maxlength<18) {
		if (maxlength>2) {
			token|=(maxlength-3);
		} else {
			/* endoffset has no length */
		}
	} else {
		token|=0xF;
		ioutput+=LZ48_encode_extended_length(odata+ioutput,maxlength-18);
	}

	odata[ioutput++]=(unsigned char)offset-1;
	
	odata[0]=(unsigned char)token;
	return ioutput;
}

unsigned char *LZ48_encode_legacy(unsigned char *data, int length, int *retlength)
{
	int i,startscan,current=1,token,ioutput=1,curscan;
	int maxoffset=0,maxlength,matchlength,literal=0,literaloffset=1;
	unsigned char *odata=NULL;
	
	odata=MemMalloc((size_t)length*1.5+10);
	if (!odata) {
		fprintf(stderr,"malloc(%.0lf) - memory full\n",(size_t)length*1.5+10);
		exit(-1);
	}

	/* first byte always literal */
	odata[0]=data[0];

	/* force short data encoding */
	if (length<5) {
		token=(length-1)<<4;
		odata[ioutput++]=(unsigned char)token;
		for (i=1;i<length;i++) odata[ioutput++]=data[current++];
		odata[ioutput++]=0xFF;
		*retlength=ioutput;
		return odata;
	}

	while (current<length) {
		maxlength=0;
		startscan=current-255;
		if (startscan<0) startscan=0;
		while (startscan<current) {
			matchlength=0;
			curscan=current;
			for (i=startscan;curscan<length;i++) {
				if (data[i]==data[curscan++]) matchlength++; else break;
			}
			if (matchlength>=3 && matchlength>maxlength) {
				maxoffset=startscan;
				maxlength=matchlength;
			}
			startscan++;
		}
		if (maxlength) {
			ioutput+=LZ48_encode_block(odata+ioutput,data,literaloffset,literal,current-maxoffset,maxlength);
			current+=maxlength;
			literaloffset=current;
			literal=0;
		} else {
			literal++;
			current++;
		}
	}
	ioutput+=LZ48_encode_block(odata+ioutput,data,literaloffset,literal,0,0);
	*retlength=ioutput;
	return odata;
}

int LZ49_encode_extended_length(unsigned char *odata, int length)
{
	int ioutput=0;

	while (length>=255) {
		odata[ioutput++]=0xFF;
		length-=255;
	}
	/* if the last value is 255 we must encode 0 to end extended length */
	/*if (length==0) rasm_printf(ae,"bugfixed!\n");*/
	odata[ioutput++]=(unsigned char)length;
	return ioutput;
}

int LZ49_encode_block(unsigned char *odata,unsigned char *data, int literaloffset,int literalcpt,int offset,int maxlength)
{
	int ioutput=1;
	int token=0;
	int i;

	if (offset<0 || offset>511) {
		fprintf(stderr,"internal offset error!\n");
		exit(-2);
	}
	
	if (literalcpt<7) {
		token=literalcpt<<4; 
	} else {
		token=0x70;
		ioutput+=LZ49_encode_extended_length(odata+ioutput,literalcpt-7);
	}

	for (i=0;i<literalcpt;i++) odata[ioutput++]=data[literaloffset++];

	if (maxlength<18) {
		if (maxlength>2) {
			token|=(maxlength-3);
		} else {
			/* endoffset has no length */
		}
	} else {
		token|=0xF;
		ioutput+=LZ49_encode_extended_length(odata+ioutput,maxlength-18);
	}

	if (offset>255) {
		token|=0x80;
		offset-=256;
	}	
	odata[ioutput++]=(unsigned char)offset-1;
	
	odata[0]=(unsigned char)token;
	return ioutput;
}

unsigned char *LZ49_encode_legacy(unsigned char *data, int length, int *retlength)
{
	int i,startscan,current=1,token,ioutput=1,curscan;
	int maxoffset=0,maxlength,matchlength,literal=0,literaloffset=1;
	unsigned char *odata=NULL;
	
	odata=MemMalloc((size_t)(length*1.5+10));
	if (!odata) {
		fprintf(stderr,"malloc(%.0lf) - memory full\n",(size_t)length*1.5+10);
		exit(-1);
	}

	/* first byte always literal */
	odata[0]=data[0];

	/* force short data encoding */
	if (length<5) {
		token=(length-1)<<4;
		odata[ioutput++]=(unsigned char)token;
		for (i=1;i<length;i++) odata[ioutput++]=data[current++];
		odata[ioutput++]=0xFF;
		*retlength=ioutput;
		return odata;
	}

	while (current<length) {
		maxlength=0;
		startscan=current-511;
		if (startscan<0) startscan=0;
		while (startscan<current) {
			matchlength=0;
			curscan=current;
			for (i=startscan;curscan<length;i++) {
				if (data[i]==data[curscan++]) matchlength++; else break;
			}
			if (matchlength>=3 && matchlength>maxlength) {
				maxoffset=startscan;
				maxlength=matchlength;
			}
			startscan++;
		}
		if (maxlength) {
			ioutput+=LZ49_encode_block(odata+ioutput,data,literaloffset,literal,current-maxoffset,maxlength);
			current+=maxlength;
			literaloffset=current;
			literal=0;
		} else {
			literal++;
			current++;
		}
	}
	ioutput+=LZ49_encode_block(odata+ioutput,data,literaloffset,literal,0,0);
	*retlength=ioutput;
	return odata;
}


/***************************************
	semi-generic body of program
***************************************/

#ifndef INTEGRATED_ASSEMBLY

/*
	Usage
	display the mandatory parameters
*/
void Usage(int help)
{
	#undef FUNC
	#define FUNC "Usage"
	
	printf("%s (c) 2017 Edouard BERGE (use -n option to display all licenses)\n",RASM_VERSION);
	#ifndef NO_3RD_PARTIES
	printf("LZ4 (c) Yann Collet / ZX7 (c) Einar Saukas / Exomizer 2 (c) Magnus Lind\n");
	#endif
	printf("\n");
	printf("SYNTAX: rasm <inputfile> [options]\n");
	printf("\n");

	if (help) {
		printf("FILENAMES:\n");
		printf("-o  <outputfile radix>   choose a common radix for all files\n");
		printf("-ob <binary filename>    choose a full filename for binary output\n");
		printf("-oc <cartridge filename> choose a full filename for cartridge output\n");
		printf("-oi <snapshot filename>  choose a full filename for snapshot output\n");
		printf("-os <symbol filename>    choose a full filename for symbol output\n");
		printf("-ok <breakpoint filename>choose a full filename for breakpoint output\n");
		printf("-I<path>                 set a path for files to read\n");
		printf("-no                      disable all file output\n");
		printf("DEPENDENCIES EXPORT:\n");
		printf("-depend=make             output dependencies on a single line\n");
		printf("-depend=list             output dependencies as a list\n");
		printf("if 'binary filename' is set then it will be outputed first\n");
		printf("SYMBOLS EXPORT:\n");
		printf("-s  export symbols %%s #%%X B%%d (label,adr,cprbank)\n");
		printf("-sp export symbols with Pasmo convention\n");
		printf("-sw export symbols with Winape convention\n");
		printf("-ss export symbols in the snapshot (SYMB chunk for ACE)\n");
		printf("-sc <format> export symbols with source code convention\n");
		printf("-l  <labelfile> import symbol file (winape,pasmo,rasm)\n");
		printf("-eb export breakpoints\n");
		printf("SYMBOLS ADDITIONAL OPTIONS:\n");
		printf("-sl export also local symbol\n");
		printf("-sv export also variables symbol\n");
		printf("-sq export also EQU symbol\n");
		printf("-sa export all symbols (like -sl -sv -sq option)\n");
		printf("-Dvariable=value import value for variable\n");
		printf("COMPATIBILITY:\n");
		printf("-m    Maxam style calculations\n");
		printf("-dams Dams 'dot' label convention\n");
		printf("-ass  AS80 behaviour mimic\n");
		printf("-uz   UZ80 behaviour mimic\n");
		
		printf("BANKING:\n");
		printf("-c  cartridge/snapshot summary\n");
		printf("EDSK generation/update:\n");
		printf("-eo overwrite files on disk if it already exists\n");
		printf("SNAPSHOT:\n");
		printf("-sb export breakpoints in snapshot (BRKS & BRKC chunks)\n");
		printf("-ss export symbols in the snapshot (SYMB chunk for ACE)\n");
		printf("-v2 export snapshot version 2 instead of version 3\n");
		printf("PARSING:\n");
		printf("-me <value>    set maximum number of error (0 means no limit)\n");
		printf("-xr            extended error display\n");
		printf("-w             disable warnings\n");
		printf("\n");
	} else {
		printf("use option -h for help\n");
		printf("\n");
	}
	
	exit(ABORT_ERROR);
}

void Licenses()
{
	#undef FUNC
	#define FUNC "Licenses"

printf("            ____ \n");
printf("           |  _ \\ __ _ ___ _ __ ___  \n");
printf("           | |_) / _` / __| '_ ` _ \\ \n");
printf("           |  _ < (_| \\__ \\ | | | | |\n");
printf("           |_| \\_\\__,_|___/_| |_| |_|\n");
printf("\n");	
printf("          is using MIT 'expat' license\n");
printf("\" Copyright (c) BERGE Edouard (roudoudou)\n\n");

printf("Permission  is  hereby  granted,  free  of charge,\n");
printf("to any person obtaining a copy  of  this  software\n");
printf("and  associated  documentation/source   files   of\n");
printf("RASM, to deal in the Software without restriction,\n");
printf("including without limitation the  rights  to  use,\n");
printf("copy,   modify,   merge,   publish,    distribute,\n");
printf("sublicense,  and/or  sell  copies of the Software,\n");
printf("and  to  permit  persons  to  whom the Software is\n");
printf("furnished  to  do  so,  subject  to  the following\n");
printf("conditions:\n");

printf("The above copyright  notice  and  this  permission\n");
printf("notice   shall   be  included  in  all  copies  or\n");
printf("substantial portions of the Software.\n");
printf("The   Software   is   provided  'as is',   without\n");
printf("warranty   of   any   kind,  express  or  implied,\n");
printf("including  but  not  limited  to the warranties of\n");
printf("merchantability,   fitness   for   a    particular\n");
printf("purpose  and  noninfringement.  In  no event shall\n");
printf("the  authors  or  copyright  holders be liable for\n");
printf("any  claim, damages  or other  liability,  whether\n");
printf("in  an  action  of  contract, tort  or  otherwise,\n");
printf("arising from,  out of  or in connection  with  the\n");
printf("software  or  the  use  or  other  dealings in the\n");
printf("Software. \"\n");

#ifndef NO_3RD_PARTIES
printf("\n\n\n\n");
printf("******* license of LZ4 cruncher / sources were modified ***********\n\n\n\n");

printf("BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)\n");

printf("Redistribution and use in source and binary forms, with or without\n");
printf("modification, are permitted provided that the following conditions are\n");
printf("met:\n\n");

printf("    * Redistributions of source code must retain the above copyright\n");
printf("notice, this list of conditions and the following disclaimer.\n");
printf("    * Redistributions in binary form must reproduce the above\n");
printf("copyright notice, this list of conditions and the following disclaimer\n");
printf("in the documentation and/or other materials provided with the\n");
printf("distribution.\n\n");

printf("THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n");
printf("'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n");
printf("LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR\n");
printf("A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n");
printf("OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n");
printf("SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT\n");
printf("LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,\n");
printf("DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY\n");
printf("THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
printf("(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n");
printf("OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\n");

printf("You can contact the author at :\n");
printf(" - LZ4 homepage : http://www.lz4.org\n");
printf(" - LZ4 source repository : https://github.com/lz4/lz4\n");


printf("\n\n\n\n");
printf("******* license of ZX7 cruncher / sources were modified ***********\n\n\n\n");


printf(" * (c) Copyright 2012 by Einar Saukas. All rights reserved.\n");
printf(" *\n");
printf(" * Redistribution and use in source and binary forms, with or without\n");
printf(" * modification, are permitted provided that the following conditions are met:\n");
printf(" *     * Redistributions of source code must retain the above copyright\n");
printf(" *       notice, this list of conditions and the following disclaimer.\n");
printf(" *     * Redistributions in binary form must reproduce the above copyright\n");
printf(" *       notice, this list of conditions and the following disclaimer in the\n");
printf(" *       documentation and/or other materials provided with the distribution.\n");
printf(" *     * The name of its author may not be used to endorse or promote products\n");
printf(" *       derived from this software without specific prior written permission.\n");
printf(" *\n");
printf(" * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' AND\n");
printf(" * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n");
printf(" * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n");
printf(" * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY\n");
printf(" * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n");
printf(" * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n");
printf(" * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n");
printf(" * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n");
printf(" * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n");
printf(" * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n");


printf("\n\n\n\n");
printf("******* license of exomizer cruncher / sources were modified ***********\n\n\n\n");


printf(" * Copyright (c) 2005 Magnus Lind.\n");
printf(" *\n");
printf(" * This software is provided 'as-is', without any express or implied warranty.\n");
printf(" * In no event will the authors be held liable for any damages arising from\n");
printf(" * the use of this software.\n");
printf(" *\n");
printf(" * Permission is granted to anyone to use this software, alter it and re-\n");
printf(" * distribute it freely for any non-commercial, non-profit purpose subject to\n");
printf(" * the following restrictions:\n");
printf(" *\n");
printf(" *   1. The origin of this software must not be misrepresented; you must not\n");
printf(" *   claim that you wrote the original software. If you use this software in a\n");
printf(" *   product, an acknowledgment in the product documentation would be\n");
printf(" *   appreciated but is not required.\n");
printf(" *\n");
printf(" *   2. Altered source versions must be plainly marked as such, and must not\n");
printf(" *   be misrepresented as being the original software.\n");
printf(" *\n");
printf(" *   3. This notice may not be removed or altered from any distribution.\n");
printf(" *\n");
printf(" *   4. The names of this software and/or it's copyright holders may not be\n");
printf(" *   used to endorse or promote products derived from this software without\n");
printf(" *   specific prior written permission.\n");
#endif

printf("\n\n");



	exit(0);
}

int _internal_check_flexible(char *fxp) {
	#undef FUNC
	#define FUNC "_internal_check_flexible"

	char *posval,*posvar;
	int cpt=0,i;

	posvar=strstr(fxp,"%s");
	posval=strstr(fxp,"%");
	if (!posval || !posvar) {
		printf("invalid flexible export string, need 2 formated fields, example: \"%%s %%d\"\n");
		exit(1);
	}
	if (posval<posvar) {
		printf("invalid flexible export string, must be %%s before the %% for value, example: \"%%s %%d\"\n");
		exit(1);
	}
	for (i=0;fxp[i];i++) if (fxp[i]=='%') cpt++;
	if (cpt>2) {
		printf("invalid flexible export string, must be only two formated fields, example: \"%%s %%d\"\n");
		exit(1);
	}

	return 1;
}
/*
	ParseOptions
	
	used to parse command line and configuration file
*/
int ParseOptions(char **argv,int argc, struct s_parameter *param)
{
	#undef FUNC
	#define FUNC "ParseOptions"
	
	char *sep;
	int i=0;

	if (strcmp(argv[i],"-autotest")==0) {
		RasmAutotest();
	} else if (strcmp(argv[i],"-uz")==0) {
		param->as80=2;
	} else if (strcmp(argv[i],"-ass")==0) {
		param->as80=1;
	} else if (strcmp(argv[i],"-eb")==0) {
		param->export_brk=1;
	} else if (strcmp(argv[i],"-dams")==0) {
		param->rough=0.0;
		param->dams=1;
	} else if (strcmp(argv[i],"-xr")==0) {
		param->extended_error=1;
	} else if (strcmp(argv[i],"-eo")==0) {
		param->edskoverwrite=1;
	} else if (strcmp(argv[i],"-depend=make")==0) {
		param->dependencies=E_DEPENDENCIES_MAKE;
		param->checkmode=1;
	} else if (strcmp(argv[i],"-depend=list")==0) {
		param->dependencies=E_DEPENDENCIES_LIST;
		param->checkmode=1;
	} else if (strcmp(argv[i],"-no")==0) {
		param->checkmode=1;
	} else if (strcmp(argv[i],"-w")==0) {
		param->nowarning=1;
	} else if (argv[i][0]=='-')	{
		switch(argv[i][1])
		{
			case 'I':
				if (argv[i][2]) {
					char *curpath;
					int l;
					l=strlen(argv[i]);
					curpath=MemMalloc(l); /* strlen(path)+2 */
					strcpy(curpath,argv[i]+2);
#ifdef OS_WIN
					if (argv[i][l-1]!='/' && argv[i][l-1]!='\\') strcat(curpath,"\\");
#else
					if (argv[i][l-1]!='/' && argv[i][l-1]!='\\') strcat(curpath,"/");
#endif
					FieldArrayAddDynamicValueConcat(&param->pathdef,&param->npath,&param->mpath,curpath);
					MemFree(curpath);
				} else {
					Usage(1);
				}
				break;
			case 'D':
				if ((sep=strchr(argv[i],'='))!=NULL) {
					if (sep!=argv[i]+2) {
						FieldArrayAddDynamicValueConcat(&param->symboldef,&param->nsymb,&param->msymb,argv[i]+2);
					} else {
						Usage(1);
					}
				} else {
					Usage(1);
				}
				break;
			case 'm':
				switch (argv[i][2]) {
					case 0:
						param->rough=0.0;
						return i;
					case 'e':
						if (argv[i][3]) Usage(1);
						if (i+1<argc) {
							param->maxerr=atoi(argv[++i]);
							return i;
						}
					default:Usage(1);
				}
				Usage(1);
				break;
			case 's':
				if (argv[i][2] && argv[i][3]) Usage(1);
				switch (argv[i][2]) {
					case 0:param->export_sym=1;return 0;
					case 'b':
						param->export_snabrk=1;printf("snabrk->1\n");return 0;
					case 'p':
						param->export_sym=2;return 0;
					case 'w':
						param->export_sym=3;return 0;
					case 'c':
						if (i+1<argc) {
							param->export_sym=4;
							param->flexible_export=TxtStrDup(argv[++i]);
							param->flexible_export=MemRealloc(param->flexible_export,strlen(param->flexible_export)+3);
							strcat(param->flexible_export,"\n");
							/* check export string */
							if (_internal_check_flexible(param->flexible_export)) return i; else Usage(1);
						}
						Usage(1);
					case 'l':
						param->export_local=1;return 0;
					case 'v':
						param->export_var=1;return 0;
					case 'q':
						param->export_equ=1;return 0;
					case 'a':
						param->export_local=1;
						param->export_var=1;
						param->export_equ=1;
						return 0;
					case 's':
						param->export_local=1;
						param->export_sym=1;
						param->export_sna=1;return 0;
					default:
					break;
				}
				Usage(1);
			case 'l':
				if (argv[i][2]) Usage(1);
				if (i+1<argc) {
					FieldArrayAddDynamicValue(&param->labelfilename,argv[++i]);
					break;
				}	
				Usage(1);
		case 'i':
printf("@@@\n@@@ --> deprecated option, there is no need to use -i option to set input file <--\n@@@\n");
				Usage(0);
			case 'o':
				if (argv[i][2] && argv[i][3]) Usage(1);
				switch (argv[i][2]) {
					case 0:
						if (i+1<argc && param->outputfilename==NULL) {
							param->outputfilename=argv[++i];
							break;
						}	
						Usage(1);
					case 'i':
						if (i+1<argc && param->snapshot_name==NULL) {
							param->snapshot_name=argv[++i];
							break;
						}
						Usage(1);
					case 'b':
						if (i+1<argc && param->binary_name==NULL) {
							param->binary_name=argv[++i];
							break;
						}
						Usage(1);
					case 'c':
						if (i+1<argc && param->cartridge_name==NULL) {
							param->cartridge_name=argv[++i];
							break;
						}
						Usage(1);
					case 'k':
						if (i+1<argc && param->breakpoint_name==NULL) {
							param->breakpoint_name=argv[++i];
							break;
						}
						Usage(1);
					case 's':
						if (i+1<argc && param->symbol_name==NULL) {
							param->symbol_name=argv[++i];
							break;
						}
						Usage(1);
					default:
						Usage(1);
				}
				break;
			case 'd':if (!argv[i][2]) param->verbose|=2; else Usage(1);
				break;
			case 'a':if (!argv[i][2]) param->verbose|=4; else Usage(1);
				break;
			case 'c':if (!argv[i][2]) param->verbose|=8; else Usage(1);
				break;
			case 'v':
				if (!argv[i][2]) {
					param->verbose=1;
				} else if (argv[i][2]=='2') {
					param->v2=1;
				}
				break;
			case 'n':if (!argv[i][2]) Licenses(); else Usage(1);
			case 'h':Usage(1);
			default:
				Usage(1);
		}
	} else {
		if (param->filename==NULL) {
			param->filename=TxtStrDup(argv[i]);
		} else if (param->outputfilename==NULL) {
			param->outputfilename=argv[i];
		} else Usage(1);
	}
	return i;
}

/*
	GetParametersFromCommandLine	
	retrieve parameters from command line and fill pointers to file names
*/
void GetParametersFromCommandLine(int argc, char **argv, struct s_parameter *param)
{
	#undef FUNC
	#define FUNC "GetParametersFromCommandLine"
	int i;
	
	for (i=1;i<argc;i++)
		i+=ParseOptions(&argv[i],argc-i,param);

	if (!param->filename) Usage(0);
	if (param->export_local && !param->export_sym) Usage(1); // � revoir?
}

/*
	main
	
	check parameters
	execute the main processing
*/


int main(int argc, char **argv)
{
	#undef FUNC
	#define FUNC "main"

	struct s_parameter param={0};
	int ret;

	param.maxerr=20;
	param.rough=0.5;

	GetParametersFromCommandLine(argc,argv,&param);
	ret=Rasm(&param);
	#ifdef RDD
	/* private dev lib tools */
printf("checking memory\n");
	CloseLibrary();
	#endif
	exit(ret);
	return 0; // Open WATCOM Warns without this...
}

#endif


