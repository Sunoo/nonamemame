/***************************************************************************

	config.c - config file access functions

***************************************************************************/

#include "config.h"
#include "mame.h"
#include "common.h"



/***************************************************************************
	CONSTANTS
***************************************************************************/

#define POSITION_BEGIN			0
#define POSITION_AFTER_PORTS	1
#define POSITION_AFTER_COINS	2
#define POSITION_AFTER_MIXER	3

#define EXTRACT_PLAYER(x)		(((x) >> 16) & 7)
#define EXTRACT_TYPE(x)			((x) & 0xffff)
#define ENCODE_PLAYER(x,p)		(x) = (EXTRACT_TYPE(x) | ((p) << 16))



/***************************************************************************
	TYPE DEFINITIONS
***************************************************************************/

struct cfg_format
{
	char cfg_string[8];
	char def_string[8];
	int (*read_input_port)(mame_file *, struct InputPort *);
	int (*read_seq)(mame_file *, InputSeq *);
	int coin_counters;
};

struct _config_file
{
	mame_file *file;
	int is_default;
	int is_write;
	const struct cfg_format *format;
	int position;
};



/***************************************************************************
	PROTOTYPES
***************************************************************************/

static config_file *config_init(const char *name, int save);



/***************************************************************************
	readint
***************************************************************************/

static int readint(mame_file *f, UINT32 *num)
{
	unsigned i;

	*num = 0;
	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;
		*num <<= 8;
		if (mame_fread(f,&c,1) != 1)
			return -1;
		*num |= c;
	}

	return 0;
}



/***************************************************************************
	writeint
***************************************************************************/

static void writeint(mame_file *f, UINT32 num)
{
	unsigned i;

	for (i = 0;i < sizeof(UINT32);i++)
	{
		unsigned char c;
		c = (num >> 8 * (sizeof(UINT32)-1)) & 0xff;
		mame_fwrite(f,&c,1);
		num <<= 8;
	}
}



/***************************************************************************
	readword
***************************************************************************/

static int readword(mame_file *f,UINT16 *num)
{
	unsigned i;
	int res;

	res = 0;
	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;
		res <<= 8;
		if (mame_fread(f,&c,1) != 1)
			return -1;
		res |= c;
	}

	*num = res;
	return 0;
}



/***************************************************************************
	writeword
***************************************************************************/

static void writeword(mame_file *f,UINT16 num)
{
	unsigned i;

	for (i = 0;i < sizeof(UINT16);i++)
	{
		unsigned char c;


		c = (num >> 8 * (sizeof(UINT16)-1)) & 0xff;
		mame_fwrite(f,&c,1);
		num <<= 8;
	}
}



/***************************************************************************
	input_port_read_ver_X
***************************************************************************/

static int input_port_read_ver_X(mame_file *f, struct InputPort *in,
	int (*seq_read_proc)(mame_file *f, InputSeq *seq))
{
	int i;
	UINT32 d;
	UINT16 w;

	for (i = 0; i < input_port_seq_count(in); i++)
	{
		if (readint(f, &d) != 0)
			return -1;
		in->type = EXTRACT_TYPE(d);
		if (i > 0)
			in->type -= IPT_EXTENSION;
		in->player = EXTRACT_PLAYER(d);

		if (readword(f, &w) != 0)
			return -1;
		in->mask = w;

		if (readword(f, &w) != 0)
			return -1;
		in->default_value = w;

		if (seq_read_proc(f, &in->seq[i]) != 0)
			return -1;
	}

	return 0;
}


/***************************************************************************
	seq_read_ver_8
***************************************************************************/

static int seq_read_ver_8(mame_file *f, InputSeq *seq)
{
	int j,len;
	UINT32 i;
	UINT16 w;

	if (readword(f,&w) != 0)
		return -1;

	len = w;
	seq_set_0(seq);
	for(j=0;j<len;++j)
	{
		if (readint(f,&i) != 0)
 			return -1;
		(*seq)[j] = savecode_to_code(i);
	}

	return 0;
}



/***************************************************************************
	input_port_read_ver_8
***************************************************************************/

static int input_port_read_ver_8(mame_file *f, struct InputPort *in)
{
	return input_port_read_ver_X(f, in, seq_read_ver_8);
}



/***************************************************************************
	config_init
***************************************************************************/

static config_file *config_init(const char *name, int save)
{
	static const struct cfg_format formats[] =
	{
		/* mame 0.74 with 8 coin counters */
		{ "MAMECFG\x9",	"MAMEDEF\x7",	input_port_read_ver_8, seq_read_ver_8, 8 },
	};

	config_file *cfg;
	char header[8];
	const char *format_header;
	int i;

	cfg = malloc(sizeof(struct _config_file));
	if (!cfg)
		goto error;
	memset(cfg, 0, sizeof(*cfg));

	cfg->file = mame_fopen(name ? name : "default", 0, FILETYPE_CONFIG, save);
	if (!cfg->file)
		goto error;

	cfg->is_default = name ? 0 : 1;
	cfg->is_write = save ? 1 : 0;

	if (save)
	{
		/* save */
		cfg->format = &formats[0];

		format_header = cfg->is_default ? formats[0].def_string : formats[0].cfg_string;

		if (mame_fwrite(cfg->file, format_header, sizeof(header)) != sizeof(header))
			goto error;
	}
	else
	{
		/* load */
		if (mame_fread(cfg->file, header, sizeof(header)) != sizeof(header))
			goto error;

		for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++)
		{
			format_header = cfg->is_default ? formats[i].def_string : formats[i].cfg_string;
			if (!memcmp(header, format_header, sizeof(header)))
			{
				cfg->format = &formats[i];
				break;
			}
		}
		if (!cfg->format)
			goto error;
	}

	cfg->position = POSITION_BEGIN;
	return cfg;

error:
	if (cfg)
		config_close(cfg);
	return NULL;
}



/***************************************************************************
	count_input_ports
***************************************************************************/

static unsigned int count_input_ports(const struct InputPort *in)
{
	unsigned int total = 0;
	while (in->type != IPT_END)
	{
		total += input_port_seq_count(in);
		in++;
	}
	return total;
}



/***************************************************************************
	config_open
***************************************************************************/

config_file *config_open(const char *name)
{
	return config_init(name, 0);
}



/***************************************************************************
	config_create
***************************************************************************/

config_file *config_create(const char *name)
{
	return config_init(name, 1);
}



/***************************************************************************
	config_close
***************************************************************************/

void config_close(config_file *cfg)
{
	if (cfg->file)
		mame_fclose(cfg->file);
	free(cfg);
}



/***************************************************************************
	config_read_ports
***************************************************************************/

int config_read_ports(config_file *cfg, struct InputPort *input_ports_default, struct InputPort *input_ports)
{
	unsigned int total;
	unsigned int saved_total;
	struct InputPort *in;
	struct InputPort saved;
	int (*read_input_port)(mame_file *, struct InputPort *);

	if (cfg->is_write || cfg->is_default)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_BEGIN)
		return CONFIG_ERROR_BADPOSITION;

	read_input_port = cfg->format->read_input_port;

	/* calculate the size of the array */
	total = count_input_ports(input_ports_default);

	/* read array size */
	if (readint(cfg->file, &saved_total) != 0)
		return CONFIG_ERROR_CORRUPT;
	if (saved_total != total)
		return CONFIG_ERROR_CORRUPT;

	/* read the original settings and compare them with the ones defined in the driver */
	in = input_ports_default;
	while (in->type != IPT_END)
	{
		int seqnum;
		
		if (read_input_port(cfg->file, &saved) != 0)
			return CONFIG_ERROR_CORRUPT;
			
		if (in->mask != saved.mask ||
			in->default_value != saved.default_value ||
			in->type != saved.type ||
			in->player != saved.player)
		{
			return CONFIG_ERROR_CORRUPT;	/* the default values are different */
		}

		for (seqnum = 0; seqnum < input_port_seq_count(&saved); seqnum++)
		{
			if (seq_cmp(&in->seq[seqnum], &saved.seq[seqnum]) !=0 )
				return CONFIG_ERROR_CORRUPT;	/* the default values are different */
		}

		in++;
	}

	/* read the current settings */
	in = input_ports;
	while (in->type != IPT_END)
	{
		if (read_input_port(cfg->file, in) != 0)
			break;
		in++;
	}

	cfg->position = POSITION_AFTER_PORTS;
	return CONFIG_ERROR_SUCCESS;
}



/***************************************************************************
	config_read_default_ports
***************************************************************************/

int config_read_default_ports(config_file *cfg, struct ipd *input_ports_default)
{
	UINT32 type;
	int player, seqnum;
	InputSeq def_seq;
	InputSeq seq;
	int i;
	int (*read_seq)(mame_file *, InputSeq *);

	if (cfg->is_write || !cfg->is_default)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_BEGIN)
		return CONFIG_ERROR_BADPOSITION;

	read_seq = cfg->format->read_seq;

	for (;;)
	{
		if (readint(cfg->file, &type) != 0)
			break;
		player = EXTRACT_PLAYER(type);
		type = EXTRACT_TYPE(type);
		seqnum = 0;
		if (type >= __ipt_max)
		{
			seqnum = 1;
			type -= IPT_EXTENSION;
		}

		if (read_seq(cfg->file, &def_seq)!=0)
			break;
		if (read_seq(cfg->file, &seq)!=0)
			break;

		i = 0;
		while (input_ports_default[i].type != IPT_END)
		{
			if (input_ports_default[i].type == type && (input_ports_default[i].player == 0 || input_ports_default[i].player == player))
			{
				/* load stored settings only if the default hasn't changed */
				if (seq_cmp(&input_ports_default[i].seq[seqnum], &def_seq)==0)
					seq_copy(&input_ports_default[i].seq[seqnum], &seq);
			}

			i++;
		}
	}

	cfg->position = POSITION_AFTER_PORTS;
	return CONFIG_ERROR_SUCCESS;
}



/***************************************************************************
	config_read_coin_and_ticket_counters
***************************************************************************/

int config_read_coin_and_ticket_counters(config_file *cfg, unsigned int *coins, unsigned int *lastcoin,
	unsigned int *coinlockedout, unsigned int *dispensed_tickets)
{
	int coin_counters;
	int i;

	if (cfg->is_write)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_AFTER_PORTS)
		return CONFIG_ERROR_BADPOSITION;

	coin_counters = cfg->format->coin_counters;

	/* Clear the coin & ticket counters/flags - LBO 042898 */
	for (i = 0; i < COIN_COUNTERS; i ++)
		coins[i] = lastcoin[i] = coinlockedout[i] = 0;
	*dispensed_tickets = 0;

	/* read in the coin/ticket counters */
	for (i = 0; i < COIN_COUNTERS; i ++)
	{
		if (readint(cfg->file, &coins[i]) != 0)
			goto done;
	}
	if (readint(cfg->file, dispensed_tickets) != 0)
		goto done;

done:
	cfg->position = POSITION_AFTER_COINS;
	return 0;
}



/***************************************************************************
	config_read_mixer_config
***************************************************************************/

int config_read_mixer_config(config_file *cfg, struct mixer_config *mixercfg)
{
#ifdef VOLUME_AUTO_ADJUST
	UINT32 volume = 0;
#endif /* VOLUME_AUTO_ADJUST */
	if (cfg->is_write)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_AFTER_COINS)
		return CONFIG_ERROR_BADPOSITION;

	memset(mixercfg->default_levels, 0xff, sizeof(mixercfg->default_levels));
	memset(mixercfg->mixing_levels, 0xff, sizeof(mixercfg->mixing_levels));
	mame_fread(cfg->file, mixercfg->default_levels, MIXER_MAX_CHANNELS);
	mame_fread(cfg->file, mixercfg->mixing_levels, MIXER_MAX_CHANNELS);

#ifdef VOLUME_AUTO_ADJUST
	mame_fread(cfg->file, &volume, sizeof volume);
	if (volume == VOLUME_MULTIPLIER_EXIST)
		mame_fread(cfg->file, &mixercfg->volume_multiplier, sizeof mixercfg->volume_multiplier);
	else
		mixercfg->volume_multiplier = DEFAULT_VOLUME_MULTIPLIER;
#endif /* VOLUME_AUTO_ADJUST */

	cfg->position = POSITION_AFTER_MIXER;
	return CONFIG_ERROR_SUCCESS;
}



/***************************************************************************
	seq_write
***************************************************************************/

static void seq_write(mame_file *f, const InputSeq *seq)
{
	int j, len;

	for (len = 0; len < SEQ_MAX; ++len)
		if ((*seq)[len] == CODE_NONE)
			break;

	writeword(f, len);
	for (j = 0; j < len; ++j)
		writeint(f, code_to_savecode((*seq)[j]));
}



/***************************************************************************
	input_port_write
***************************************************************************/

static void input_port_write(mame_file *f, const struct InputPort *in)
{
	int i;

	for (i = 0; i < input_port_seq_count(in); i++)
	{
		UINT32 type_and_player = in->type;
		if (i > 0)
			type_and_player += IPT_EXTENSION;
		ENCODE_PLAYER(type_and_player, in->player);
		writeint(f, type_and_player);
		writeword(f, in->mask);
		writeword(f, in->default_value);
		seq_write(f, &in->seq[i]);
	}
}



/***************************************************************************
	config_write_ports
***************************************************************************/

int config_write_ports(config_file *cfg, const struct InputPort *input_ports_default, const struct InputPort *input_ports)
{
	unsigned int total;
	const struct InputPort *in;

	if (!cfg->is_write || cfg->is_default)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_BEGIN)
		return CONFIG_ERROR_BADPOSITION;

	/* calculate the size of the array */
	total = count_input_ports(input_ports_default);

	/* write array size */
	writeint(cfg->file, total);

	/* write the original settings as defined in the driver */
	in = input_ports_default;
	while (in->type != IPT_END)
	{
		input_port_write(cfg->file, in);
		in++;
	}

	/* write the current settings */
	in = input_ports;
	while (in->type != IPT_END)
	{
		input_port_write(cfg->file, in);
		in++;
	}

	cfg->position = POSITION_AFTER_PORTS;
	return CONFIG_ERROR_SUCCESS;
}



/***************************************************************************
	config_write_default_ports
***************************************************************************/

int config_write_default_ports(config_file *cfg, const struct ipd *input_ports_default_backup, const struct ipd *input_ports_default)
{
	int i = 0, j;

	if (!cfg->is_write || !cfg->is_default)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_BEGIN)
		return CONFIG_ERROR_BADPOSITION;

	while (input_ports_default[i].type != IPT_END)
	{
		if (input_ports_default[i].type != IPT_OSD_RESERVED)
		{
			int seq_count = (input_ports_default[i].type > IPT_ANALOG_START && input_ports_default[i].type < IPT_ANALOG_END) ? 2 : 1;
			for (j = 0; j < seq_count; j++)
			{
				UINT32 type_and_player = input_ports_default[i].type;
				if (j > 0)
					type_and_player += IPT_EXTENSION;
				if (input_ports_default[i].player != 0)
					ENCODE_PLAYER(type_and_player, input_ports_default[i].player - 1);
				writeint(cfg->file, type_and_player);
				seq_write(cfg->file, &input_ports_default_backup[i].seq[j]);
				seq_write(cfg->file, &input_ports_default[i].seq[j]);
			}
		}
		i++;
	}

	cfg->position = POSITION_AFTER_PORTS;
	return CONFIG_ERROR_SUCCESS;
}



/***************************************************************************
	config_write_coin_and_ticket_counters
***************************************************************************/

int config_write_coin_and_ticket_counters(config_file *cfg, const unsigned int *coins, const unsigned int *lastcoin,
	const unsigned int *coinlockedout, unsigned int dispensed_tickets)
{
	int i;

	/* write out the coin/ticket counters for this machine - LBO 042898 */
	for (i = 0; i < COIN_COUNTERS; i ++)
		writeint(cfg->file, coins[i]);
	writeint(cfg->file, dispensed_tickets);
	cfg->position = POSITION_AFTER_COINS; 
	return CONFIG_ERROR_SUCCESS; 
}



/***************************************************************************
	config_write_mixer_config
***************************************************************************/

int config_write_mixer_config(config_file *cfg, const struct mixer_config *mixercfg)
{
#ifdef VOLUME_AUTO_ADJUST
	UINT32 volume = VOLUME_MULTIPLIER_EXIST;
#endif /* VOLUME_AUTO_ADJUST */

	if (!cfg->is_write)
		return CONFIG_ERROR_BADMODE;
	if (cfg->position != POSITION_AFTER_COINS)
		return CONFIG_ERROR_BADPOSITION;

	mame_fwrite(cfg->file, mixercfg->default_levels, MIXER_MAX_CHANNELS);
	mame_fwrite(cfg->file, mixercfg->mixing_levels, MIXER_MAX_CHANNELS);

#ifdef VOLUME_AUTO_ADJUST
	mame_fwrite(cfg->file, &volume, sizeof volume);
	mame_fwrite(cfg->file, &mixercfg->volume_multiplier, sizeof mixercfg->volume_multiplier);
#endif /* VOLUME_AUTO_ADJUST */

	cfg->position = POSITION_AFTER_MIXER;
	return CONFIG_ERROR_SUCCESS;
}

