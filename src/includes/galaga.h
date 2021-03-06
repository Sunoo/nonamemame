/* vidhrdw/bosco.c */
extern data8_t *bosco_videoram;
extern data8_t *bosco_radarattr;
READ8_HANDLER( bosco_videoram_r );
WRITE8_HANDLER( bosco_videoram_w );
WRITE8_HANDLER( bosco_scrollx_w );
WRITE8_HANDLER( bosco_scrolly_w );
WRITE8_HANDLER( bosco_starcontrol_w );
WRITE8_HANDLER( bosco_starclr_w );
WRITE8_HANDLER( bosco_starblink_w );
VIDEO_START( bosco );
VIDEO_UPDATE( bosco );
PALETTE_INIT( bosco );
VIDEO_EOF( bosco );	/* update starfield */

/* vidhrdw/galaga.c */
extern data8_t *galaga_videoram;
extern data8_t *galaga_ram1,*galaga_ram2,*galaga_ram3;
PALETTE_INIT( galaga );
READ8_HANDLER( galaga_videoram_r );
WRITE8_HANDLER( galaga_videoram_w );
WRITE8_HANDLER( galaga_starcontrol_w );
WRITE8_HANDLER( gatsbee_bank_w );
VIDEO_START( galaga );
VIDEO_UPDATE( galaga );
VIDEO_EOF( galaga );	/* update starfield */

/* XEVIOUS */
extern data8_t *xevious_fg_videoram,*xevious_fg_colorram;
extern data8_t *xevious_bg_videoram,*xevious_bg_colorram;
extern data8_t *xevious_sr1,*xevious_sr2,*xevious_sr3;
READ8_HANDLER( xevious_fg_videoram_r );
READ8_HANDLER( xevious_fg_colorram_r );
READ8_HANDLER( xevious_bg_videoram_r );
READ8_HANDLER( xevious_bg_colorram_r );
WRITE8_HANDLER( xevious_fg_videoram_w );
WRITE8_HANDLER( xevious_fg_colorram_w );
WRITE8_HANDLER( xevious_bg_videoram_w );
WRITE8_HANDLER( xevious_bg_colorram_w );
WRITE8_HANDLER( xevious_vh_latch_w );
WRITE8_HANDLER( xevious_bs_w );
READ8_HANDLER( xevious_bb_r );
VIDEO_START( xevious );
PALETTE_INIT( xevious );
VIDEO_UPDATE( xevious );


/* BATTLES */
void battles_customio_init(void);
READ8_HANDLER( battles_customio0_r );
READ8_HANDLER( battles_customio_data0_r );
READ8_HANDLER( battles_customio3_r );
READ8_HANDLER( battles_customio_data3_r );
READ8_HANDLER( battles_input_port_r );

WRITE8_HANDLER( battles_customio0_w );
WRITE8_HANDLER( battles_customio_data0_w );
WRITE8_HANDLER( battles_customio3_w );
WRITE8_HANDLER( battles_customio_data3_w );
WRITE8_HANDLER( battles_CPU4_coin_w );
WRITE8_HANDLER( battles_noise_sound_w );

INTERRUPT_GEN( battles_interrupt_4 );

PALETTE_INIT( battles );



/* DIG DUG */
extern data8_t *digdug_videoram,*digdug_objram, *digdug_posram, *digdug_flpram;
READ8_HANDLER( digdug_videoram_r );
WRITE8_HANDLER( digdug_videoram_w );
WRITE8_HANDLER( digdug_PORT_w );
VIDEO_START( digdug );
VIDEO_UPDATE( digdug );
PALETTE_INIT( digdug );
