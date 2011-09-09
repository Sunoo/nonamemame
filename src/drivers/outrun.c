/*
**	2xMC68000 + Z80
**	YM2151 + Custom PCM
**
**	Out Run
**	Super Hang-On
**	Super Hang-On Limited Edition
**	Turbo Out Run
**
**	Additional infos :
**
**	It has been said that Super Hang On runs on Out Run hardware, this is not exactly the case !
**	Super Hang On use sega video board 171-5480 with the following specifications :
**
**	Sega custom IC on 171-5480
**
**	315-5196, 315-5197 and 315-5242.
**
**	Pals 	: 315-5213
**		: 315-5251
**
*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "system16.h"

static void set_fg_page( int data ){
	sys16_fg_page[0] = data>>12;
	sys16_fg_page[1] = (data>>8)&0xf;
	sys16_fg_page[2] = (data>>4)&0xf;
	sys16_fg_page[3] = data&0xf;
}

static void set_bg_page( int data ){
	sys16_bg_page[0] = data>>12;
	sys16_bg_page[1] = (data>>8)&0xf;
	sys16_bg_page[2] = (data>>4)&0xf;
	sys16_bg_page[3] = data&0xf;
}

static void set_fg_page1( int data ){
	sys16_fg_page[1] = data>>12;
	sys16_fg_page[0] = (data>>8)&0xf;
	sys16_fg_page[3] = (data>>4)&0xf;
	sys16_fg_page[2] = data&0xf;
}

static void set_bg_page1( int data ){
	sys16_bg_page[1] = data>>12;
	sys16_bg_page[0] = (data>>8)&0xf;
	sys16_bg_page[3] = (data>>4)&0xf;
	sys16_bg_page[2] = data&0xf;
}

#if 0
static void set_fg2_page( int data ){
	sys16_fg2_page[0] = data>>12;
	sys16_fg2_page[1] = (data>>8)&0xf;
	sys16_fg2_page[2] = (data>>4)&0xf;
	sys16_fg2_page[3] = data&0xf;
}

static void set_bg2_page( int data ){
	sys16_bg2_page[0] = data>>12;
	sys16_bg2_page[1] = (data>>8)&0xf;
	sys16_bg2_page[2] = (data>>4)&0xf;
	sys16_bg2_page[3] = data&0xf;
}
#endif

/* hang-on's accel/brake are really both analog controls, but I've added them
as digital as well to see what works better */
#define HANGON_DIGITAL_CONTROLS

static READ16_HANDLER( ho_io_x_r ){ return input_port_0_r( offset ); }
#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( ho_io_y_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xffff;	// both
		case 2:	return 0x00ff;  // brake
		case 1:	return 0xff00;  // accel
		case 0:	return 0x0000;  // neither
	}
	return 0x0000;
}
#else
static READ16_HANDLER( ho_io_y_r ){ return (input_port_1_r( offset ) << 8) + input_port_5_r( offset ); }
#endif

//	outrun: generate_gr_screen(0x200,0x800,0,0,3,0x8000);
static void generate_gr_screen(
	int w,int bitmap_width,int skip,
	int start_color,int end_color, int source_size )
{
	UINT8 *buf = malloc( source_size );
	UINT8 *buf_base = buf;
	if( buf ){
		UINT8 *gr = memory_region(REGION_GFX3);
		UINT8 *grr = NULL;
	    int i,j,k;
	    int center_offset=0;
		sys16_gr_bitmap_width = bitmap_width;

		memcpy(buf,gr,source_size);
		memset(gr,0,256*bitmap_width);

		if (w!=sys16_gr_bitmap_width){
			if (skip>0) // needs mirrored RHS
				grr=gr;
			else {
				center_offset= bitmap_width-w;
				gr+=center_offset/2;
			}
		}

		for (i=0; i<256; i++){ // build gr_bitmap
			UINT8 last_bit;
			UINT8 color_data[4];
			color_data[0]=start_color;
			color_data[1]=start_color+1;
			color_data[2]=start_color+2;
			color_data[3]=start_color+3;

			last_bit = ((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
			for (j=0; j<w/8; j++){
				for (k=0; k<8; k++){
					UINT8 bit=((buf[0]&0x80)==0)|(((buf[0x4000]&0x80)==0)<<1);
					if (bit!=last_bit && bit==0 && i>1){ // color flipped to 0,advance color[0]
						if (color_data[0]+end_color <= end_color){
							color_data[0]+=end_color;
						}
						else{
							color_data[0]-=end_color;
						}
					}
					*gr++ = color_data[bit];
					last_bit=bit;
					buf[0] <<= 1; buf[0x4000] <<= 1;
				}
				buf++;
			}

			if (grr!=NULL){ // need mirrored RHS
				const UINT8 *temp = gr-1-skip;
				for (j=0; j<w-skip; j++){
					*gr++ = *temp--;
				}
				for (j=0; j<skip; j++) *gr++ = 0;
			}
			else {
				gr+=center_offset;
			}
		}

		i=1;
		while ( (1<<i) < sys16_gr_bitmap_width ) i++;
		sys16_gr_bitmap_width=i; // power of 2
		free(buf_base);
	}
}

static data16_t coinctrl;

static WRITE16_HANDLER( sys16_3d_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x10;
		coin_counter_w(0,coinctrl & 0x01);
		/* bit 6 is also used (0 in fantzone) */

		/* Hang-On, Super Hang-On, Space Harrier, Enduro Racer */
		set_led_status(0,coinctrl & 0x04);

		/* Space Harrier */
		set_led_status(1,coinctrl & 0x08);
	}
}

#if 0
static WRITE16_HANDLER( sound_command_nmi_w ){
	if( ACCESSING_LSB ){
		soundlatch_w( 0,data&0xff );
		cpu_set_nmi_line(1, PULSE_LINE);
	}
}
#endif

static READ16_HANDLER( sys16_coinctrl_r ){
	return coinctrl;
}

#if 0
static WRITE16_HANDLER( sys16_coinctrl_w )
{
	if( ACCESSING_LSB ){
		coinctrl = data&0xff;
		sys16_refreshenable = coinctrl & 0x20;
		coin_counter_w(0,coinctrl & 0x01);
		set_led_status(0,coinctrl & 0x04);
		set_led_status(1,coinctrl & 0x08);
		/* bit 6 is also used (1 most of the time; 0 in dduxbl, sdi, wb3;
		   tturf has it normally 1 but 0 after coin insertion) */
		/* eswat sets bit 4 */
	}
}
#endif

static INTERRUPT_GEN( sys16_interrupt ){
	if(sys16_custom_irq) sys16_custom_irq();
	cpu_set_irq_line(cpu_getactivecpu(), 4, HOLD_LINE); /* Interrupt vector 4, used by VBlank */
}

static ADDRESS_MAP_START( sound_readport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x01, 0x01) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xc0, 0xc0) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writeport, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(YM2151_data_port_0_w)
ADDRESS_MAP_END

static data16_t *shared_ram;
static READ16_HANDLER( shared_ram_r ){
	return shared_ram[offset];
}
static WRITE16_HANDLER( shared_ram_w ){
	COMBINE_DATA( &shared_ram[offset] );
}

static unsigned char *sound_shared_ram;
static READ16_HANDLER( sound_shared_ram_r )
{
	return (sound_shared_ram[offset*2] << 8) +
			sound_shared_ram[offset*2+1];
}

static WRITE16_HANDLER( sound_shared_ram_w )
{
	if( ACCESSING_LSB ){
		sound_shared_ram[offset*2+1] = data&0xff;
	}
	if( ACCESSING_MSB ){
		sound_shared_ram[offset*2] = data>>8;
	}
}

static READ_HANDLER( sound2_shared_ram_r ){
	return sound_shared_ram[offset];
}
static WRITE_HANDLER( sound2_shared_ram_w ){
	sound_shared_ram[offset] = data;
}


ROM_START( shangon )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code - protected */
	ROM_LOAD16_BYTE( "ic133", 0x000000, 0x10000, CRC(e52721fe) SHA1(21f0aa14d0cbda3d762bca86efe089646031aef5) )
	ROM_LOAD16_BYTE( "ic118", 0x000001, 0x10000, CRC(5fee09f6) SHA1(b97a08ba75d8c617aceff2b03c1f2bfcb16181ef) )
	ROM_LOAD16_BYTE( "ic132", 0x020000, 0x10000, CRC(5d55d65f) SHA1(d02d76b98d74746b078b0f49f0320b8be48e4c47) )
	ROM_LOAD16_BYTE( "ic117", 0x020001, 0x10000, CRC(b967e8c3) SHA1(00224b337b162daff03bbfabdcf8541025220d46) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     			0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(              	0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10676.7",   0x020001, 0x010000, CRC(25ebf2c5) SHA1(abcf673ae4e280417dd9f46d18c0ec7c0e4802ae) )
	ROM_RELOAD(              	0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10683.15",  0x020000, 0x010000, CRC(6365d2e9) SHA1(688e2ba194e859f86cd3486c2575ebae257e975a) )
	ROM_RELOAD(              	0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ep10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ep10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ep10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ep10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ep10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ep10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ep10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ep10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ep10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ic88", 0x0000, 0x08000, CRC(1254efa6) SHA1(997770ccdd776de6e335a6d8b1e15d200cbd4410) )

	ROM_LOAD( "ep10643.66", 0x10000, 0x08000, CRC(06f55364) SHA1(fd685795e12541e3d0059d383fab293b3980d247) )
	ROM_LOAD( "ic67",       0x18000, 0x08000, CRC(731f5cf8) SHA1(5a62a94e15a15d5470f32112e4b2b05675983720) ) /* Possibly a bad dump.... */
	ROM_LOAD( "ep10644.67", 0x18000, 0x08000, CRC(b41d541d) SHA1(28bbfa5edaa4a5901c74074354ba6f14d8f42ff6) ) /* from other set */
	ROM_LOAD( "ep10645.68", 0x20000, 0x08000, CRC(a60dabff) SHA1(bbef0fb0d7837cc7efc866226bfa2bd7fab06459) )
	ROM_LOAD( "ep10646.69", 0x28000, 0x08000, CRC(473cc411) SHA1(04ca2d047eb59581cd5d76e0ac6eca8b19eef497) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU  - protected */
	ROM_LOAD16_BYTE( "ep10640.76", 0x00000, 0x10000, CRC(02be68db) SHA1(8c9f98ee49db54ee53b721ecf53f91737ae6cd73) )
	ROM_LOAD16_BYTE( "ep10638.58", 0x00001, 0x10000, CRC(f13e8bee) SHA1(1c16c018f58f1fb49e240314a7e97a947087fad9) )
	ROM_LOAD16_BYTE( "ic75", 0x20000, 0x10000, CRC(1627c224) SHA1(76a308fc15e5ac52947fffadee38fa9b3c4b1e8a) ) /* Possibly a bad dump.... */
	ROM_LOAD16_BYTE( "ep10641.75", 0x20000, 0x10000, CRC(38c3f808) SHA1(36fae99b56980ef33853170afe10b363cd41c053) ) /* from other set */
	ROM_LOAD16_BYTE( "ep10639.57", 0x20001, 0x10000, CRC(8cdbcde8) SHA1(0bcb4df96ee16db3dd4ce52fccd939f48a4bc1a0) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "mp10642.47", 0x0000, 0x8000, CRC(7836bcc3) SHA1(26f308bf96224311ddf685799d7aa29aac42dd2f) )
ROM_END


ROM_START( shangona )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code - protected */
	ROM_LOAD16_BYTE( "ep10886.133", 0x000000, 0x10000, CRC(8be3cd36) SHA1(de96481807e782ca441d51e99f1a221bdee7d170) )
	ROM_LOAD16_BYTE( "ep10884.118", 0x000001, 0x10000, CRC(cb06150d) SHA1(840dada0cdeec444b554e6c1f2bdacc1047be567) )
	ROM_LOAD16_BYTE( "ep10887.132", 0x020000, 0x10000, CRC(8d248bb0) SHA1(7d8ed61609fd0df203255e7d046d9d30983f1dcd) )
	ROM_LOAD16_BYTE( "ep10884.117", 0x020001, 0x10000, CRC(70795f26) SHA1(332921b0a6534c4cbfe76ff957c721cc80d341b0) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0140000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10794.8",	0x000001, 0x020000, CRC(7c958e63) SHA1(ef79614e94280607a6cdf6e13db051accfd2add0) )
	ROM_RELOAD(     		0x100001, 0x020000 )
	ROM_LOAD16_BYTE( "ep10798.16",  0x000000, 0x020000, CRC(7d58f807) SHA1(783c9929d27a0270b3f7d5eb799cee6b2e5b7ae5) )
	ROM_RELOAD(              	0x100000, 0x020000 )
	ROM_LOAD16_BYTE( "ep10795.6",   0x040001, 0x020000, CRC(d9d31f8c) SHA1(3ce07b83e3aa2d8834c1a449fa31e003df5486a3) )
	ROM_LOAD16_BYTE( "ep10799.14",  0x040000, 0x020000, CRC(96d90d3d) SHA1(6572cbce8f98a1a7a8e59b0c502ab274f78d2819) )
	ROM_LOAD16_BYTE( "ep10796.4",   0x080001, 0x020000, CRC(fb48957c) SHA1(86a66bcf38686be5537a1361d390ecbbccdddc11) )
	ROM_LOAD16_BYTE( "ep10800.12",  0x080000, 0x020000, CRC(feaff98e) SHA1(20e38f9039079f64919d750a2e1382503d437463) )
	ROM_LOAD16_BYTE( "ep10797.2",   0x0c0001, 0x020000, CRC(27f2870d) SHA1(40a34e4555885bf3c6a42e472b80d11c3bd4dcba) )
	ROM_LOAD16_BYTE( "ep10801.10",  0x0c0000, 0x020000, CRC(12781795) SHA1(44bf6f657f32b9fab119557eb73c2fbf78700204) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ep10649c.88", 0x0000, 0x08000, CRC(f6c1ce71) SHA1(12299f7e5378a56be3a31cce3b8b74e48744f33a) )

	ROM_LOAD( "ep10643.66", 0x10000, 0x08000, CRC(06f55364) SHA1(fd685795e12541e3d0059d383fab293b3980d247) )
	ROM_LOAD( "ep10644.67", 0x18000, 0x08000, CRC(b41d541d) SHA1(28bbfa5edaa4a5901c74074354ba6f14d8f42ff6) )
	ROM_LOAD( "ep10645.68", 0x20000, 0x08000, CRC(a60dabff) SHA1(bbef0fb0d7837cc7efc866226bfa2bd7fab06459) )
	ROM_LOAD( "ep10646.69", 0x28000, 0x08000, CRC(473cc411) SHA1(04ca2d047eb59581cd5d76e0ac6eca8b19eef497) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU  - protected */
	ROM_LOAD16_BYTE( "ep10792.76", 0x00000, 0x10000, CRC(16299d25) SHA1(b14d5feef3e6889320d51ffca36801f4c9c4d5f8) )
	ROM_LOAD16_BYTE( "ep10790.58", 0x00001, 0x10000, CRC(2246cbc1) SHA1(c192b1ddf4c848adb564c7c87d5413d62ed650d7) )
	ROM_LOAD16_BYTE( "ep10793.75", 0x20000, 0x10000, CRC(d9525427) SHA1(cdb24db9f7a293f20fd8becc4afe84fd6abd678a) )
	ROM_LOAD16_BYTE( "ep10791.57", 0x20001, 0x10000, CRC(5faf4cbe) SHA1(41659a961e6469d9233849c3c587cd5a0a141344) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "mp10642.47", 0x0000, 0x8000, CRC(7836bcc3) SHA1(26f308bf96224311ddf685799d7aa29aac42dd2f) )
ROM_END


ROM_START(shangnle )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code - protected */
	ROM_LOAD16_BYTE( "ep13944.133", 0x000000, 0x10000, CRC(989a80db) SHA1(5026e5cf52d4fd85a0bab6c4ea7a34cf266b2a3b) )
	ROM_LOAD16_BYTE( "ep13943.118", 0x000001, 0x10000, CRC(426e3050) SHA1(f332ea76285b4e1361d818cbe5aab0640b4185c3) )
	ROM_LOAD16_BYTE( "ep10899.132", 0x020000, 0x10000, CRC(bb3faa37) SHA1(ccf3352255503fd6619e6e116d187a8cd1ff75e6) )
	ROM_LOAD16_BYTE( "ep10897.117", 0x020001, 0x10000, CRC(5f087eb1) SHA1(bdfcc39e92087057acc4e91741a03e7ba57824c1) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     		0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(              	0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "ep13945.7",   0x020001, 0x010000, CRC(fbb1eef9) SHA1(2798df2f25706e0d3be01d945274f478d7e5a2ae) )
	ROM_RELOAD(              	0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "ep13946.15",  0x020000, 0x010000, CRC(03144930) SHA1(c20f4883ee2de35cd0b67949de0e41464f2c5fae) )
	ROM_RELOAD(              	0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ep10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ep10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ep10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ep10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ep10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ep10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ep10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ep10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ep10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x30000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "ep10649c.88", 0x0000, 0x08000, CRC(f6c1ce71) SHA1(12299f7e5378a56be3a31cce3b8b74e48744f33a) )

	ROM_LOAD( "ep10643.66", 0x10000, 0x08000, CRC(06f55364) SHA1(fd685795e12541e3d0059d383fab293b3980d247) )
	ROM_LOAD( "ep10644.67", 0x18000, 0x08000, CRC(b41d541d) SHA1(28bbfa5edaa4a5901c74074354ba6f14d8f42ff6) )
	ROM_LOAD( "ep10645.68", 0x20000, 0x08000, CRC(a60dabff) SHA1(bbef0fb0d7837cc7efc866226bfa2bd7fab06459) )
	ROM_LOAD( "ep10646.69", 0x28000, 0x08000, CRC(473cc411) SHA1(04ca2d047eb59581cd5d76e0ac6eca8b19eef497) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU  - protected */
	ROM_LOAD16_BYTE( "ep10640.76", 0x00000, 0x10000, CRC(02be68db) SHA1(8c9f98ee49db54ee53b721ecf53f91737ae6cd73) )
	ROM_LOAD16_BYTE( "ep10638.58", 0x00001, 0x10000, CRC(f13e8bee) SHA1(1c16c018f58f1fb49e240314a7e97a947087fad9) )
	ROM_LOAD16_BYTE( "ep10641.75", 0x20000, 0x10000, CRC(38c3f808) SHA1(36fae99b56980ef33853170afe10b363cd41c053) )
	ROM_LOAD16_BYTE( "ep10639.57", 0x20001, 0x10000, CRC(8cdbcde8) SHA1(0bcb4df96ee16db3dd4ce52fccd939f48a4bc1a0) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "ep10642.47", 0x0000, 0x8000, CRC(7836bcc3) SHA1(26f308bf96224311ddf685799d7aa29aac42dd2f) )
ROM_END

ROM_START( shangonb )
	ROM_REGION( 0x030000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "s-hangon.30", 0x000000, 0x10000, CRC(d95e82fc) SHA1(bc6cd0b0ac98a9c53f2e22ac086521704ab59e4d) )
	ROM_LOAD16_BYTE( "s-hangon.32", 0x000001, 0x10000, CRC(2ee4b4fb) SHA1(ba4042ab6e533c16c3cde848248d75e484be113f) )
	ROM_LOAD16_BYTE( "s-hangon.29", 0x020000, 0x8000, CRC(12ee8716) SHA1(8e798d23d22f85cd046641184d104c17b27995b2) )
	ROM_LOAD16_BYTE( "s-hangon.31", 0x020001, 0x8000, CRC(155e0cfd) SHA1(e51734351c887fe3920c881f57abdfbb7d075f57) )

	ROM_REGION( 0x18000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "ep10652.54",        0x00000, 0x08000, CRC(260286f9) SHA1(dc7c8d2c6ef924a937328685eed19bda1c8b1819) )
	ROM_LOAD( "ep10651.55",        0x08000, 0x08000, CRC(c609ee7b) SHA1(c6dacf81cbfe7e5df1f9a967cf571be1dcf1c429) )
	ROM_LOAD( "ep10650.56",        0x10000, 0x08000, CRC(b236a403) SHA1(af02b8122794c083a66f2ab35d2c73b84b2df0be) )

	ROM_REGION( 0x0120000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "ep10675.8",	0x000001, 0x010000, CRC(d6ac012b) SHA1(305023b1a0a9d84cfc081ffc2ad7578b53d562f2) )
	ROM_RELOAD(     		0x100001, 0x010000 )
	ROM_LOAD16_BYTE( "ep10682.16",  0x000000, 0x010000, CRC(d9d83250) SHA1(f8ca3197edcdf53643a5b335c3c044ddc1310cd4) )
	ROM_RELOAD(     		0x100000, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.20", 0x020001, 0x010000, CRC(eef23b3d) SHA1(2416fa9991afbdddf25d469082e53858289550db) )
	ROM_RELOAD(           		      0x0e0001, 0x010000 )
	ROM_LOAD16_BYTE( "s-hangon.14", 0x020000, 0x010000, CRC(0f26d131) SHA1(0d8b6eb8b8aae0aa8f0fa0c31dc91ad0e610be3e) )
	ROM_RELOAD(              		  0x0e0000, 0x010000 )
	ROM_LOAD16_BYTE( "ep10677.6",   0x040001, 0x010000, CRC(8a57b8d6) SHA1(df1a31559dd2d1e7c2c9d800bf97526bdf3e84e6) )
	ROM_LOAD16_BYTE( "ep10684.14",  0x040000, 0x010000, CRC(3aff8910) SHA1(4b41a49a7f02363424e814b37edce9a7a44a112e) )
	ROM_LOAD16_BYTE( "ep10678.5",   0x060001, 0x010000, CRC(af473098) SHA1(a2afaba1cbf672949dc50e407b46d7e9ae183774) )
	ROM_LOAD16_BYTE( "ep10685.13",  0x060000, 0x010000, CRC(80bafeef) SHA1(f01bcf65485e60f34e533295a896fca0b92e5b14) )
	ROM_LOAD16_BYTE( "ep10679.4",   0x080001, 0x010000, CRC(03bc4878) SHA1(548fc58bcc620204e30fa12fa4c4f0a3f6a1e4c0) )
	ROM_LOAD16_BYTE( "ep10686.12",  0x080000, 0x010000, CRC(274b734e) SHA1(906fa528659bc17c9b4744cec52f7096711adce8) )
	ROM_LOAD16_BYTE( "ep10680.3",   0x0a0001, 0x010000, CRC(9f0677ed) SHA1(5964642b70bfad418da44f2d91476f887b021f74) )
	ROM_LOAD16_BYTE( "ep10687.11",  0x0a0000, 0x010000, CRC(508a4701) SHA1(d17aea2aadc2e2cd65d81bf91feb3ef6923d5c0b) )
	ROM_LOAD16_BYTE( "ep10681.2",   0x0c0001, 0x010000, CRC(b176ea72) SHA1(7ec0eb0f13398d014c2e235773ded00351edb3e2) )
	ROM_LOAD16_BYTE( "ep10688.10",  0x0c0000, 0x010000, CRC(42fcd51d) SHA1(0eacb3527dc21746e5b901fcac83f2764a0f9e2c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "s-hangon.03", 0x0000, 0x08000, CRC(83347dc0) SHA1(079bb750edd6372750a207764e8c84bb6abf2f79) )

	ROM_REGION( 0x20000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "s-hangon.02", 0x00000, 0x10000, CRC(da08ca2b) SHA1(2c94c127efd66f6cf86b25e2653637818a99aed1) )
	ROM_LOAD( "s-hangon.01", 0x10000, 0x10000, CRC(8b10e601) SHA1(75e9bcdd3f096be9bed672d61064b9240690deec) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "s-hangon.09", 0x00000, 0x10000, CRC(070c8059) SHA1(a18c5e9473b6634f6e7165300e39029335b41ba3) )
	ROM_LOAD16_BYTE( "s-hangon.05", 0x00001, 0x10000, CRC(9916c54b) SHA1(41a7c5a9bdb1e3feae8fadf1ac5f51fab6376157) )
	ROM_LOAD16_BYTE( "s-hangon.08", 0x20000, 0x10000, CRC(000ad595) SHA1(eb80e798159c09bc5142a7ea8b9b0f895976b0d4) )
	ROM_LOAD16_BYTE( "s-hangon.04", 0x20001, 0x10000, CRC(8f8f4af0) SHA1(1dac21b7df6ec6874d36a07e30de7129b7f7f33a) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "s-hangon.26", 0x0000, 0x8000, CRC(1bbe4fc8) SHA1(30f7f301e4d10d3b254d12bf3d32e5371661a566) )
ROM_END

// Outrun hardware
ROM_START( outrun )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380a", 0x000000, 0x10000, CRC(434fadbc) SHA1(83c861d331e69ef4f2452c313ae4b5ea9d8b7948) )
	ROM_LOAD16_BYTE( "10382a", 0x000001, 0x10000, CRC(1ddcc04e) SHA1(945d207d8d602d7fdb6d25f6b93c9c0b995e8d5a) )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, CRC(be8c412b) SHA1(bf3ff05bbf81bdd44567f3b9bb4919ed4a499624) )
	ROM_LOAD16_BYTE( "10383a", 0x020001, 0x10000, CRC(dcc586e7) SHA1(d6e1de6b562359574d94b88ce6101646c506e701) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, CRC(0a1c98de) SHA1(e683ab79bfc6f8c60974912f07ef8173b167f448) )
	ROM_LOAD( "10372", 0x20000, 0x20000, CRC(1640ad1f) SHA1(3223d536c61f349be8de9ee32fc210f61fe822e2) )
	ROM_LOAD( "10373", 0x40000, 0x20000, CRC(339f8e64) SHA1(0d79b584e9e6c838cb0a9d2e8a611913265a06d2) )
	ROM_LOAD( "10374", 0x60000, 0x20000, CRC(22744340) SHA1(0997687641b0fc5c81343220166777dd74cc12a8) )
	ROM_LOAD( "10375", 0x80000, 0x20000, CRC(62a472bd) SHA1(e753df3ad378e6d4e22d5978626fc2370826bcf0) )
	ROM_LOAD( "10376", 0xa0000, 0x20000, CRC(8337ace7) SHA1(6bb3739a730c8d026fe9a6e831fc7c9fc13eae4f) )
	ROM_LOAD( "10377", 0xc0000, 0x20000, CRC(c86daecb) SHA1(78f61038f54ca56aab1f73d662fbaf07e396075f) )
	ROM_LOAD( "10378", 0xe0000, 0x20000, CRC(544068fd) SHA1(54fe03d2d80548ebba9c2578ece5d0f89593cb3e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, CRC(a10abaa9) SHA1(01c8a819587a66d2ee4d255656e36fa0904377b0) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) // twice??
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, CRC(e28a5baf) SHA1(f715bde96c73ed47035acf5a41630fdeb41bb2f9) )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, CRC(da131c81) SHA1(57d5219bd0e2fd886217e37e8773fd76be9b40eb) )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, CRC(ba9ec82a) SHA1(2136c9572e26b7ae6de402c0cd53174407cc6018) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, CRC(22794426) SHA1(a554d4b68e71861a0d0da4d031b3b811b246f082) )
ROM_END

/*

Sega Outrun Japan version
-------------------------

CPU Board (837-6063)
---------
EPR10173 - IC66 - 5826
EPR10174 - IC67 - 1817
EPR10175 - IC68 - EAE0
EPR10176 - IC69 - 05F3
EPR10178 - IC86 - 5494
EPR10179 - IC87 - E63D
EPR10180 - IC88 - 14C5
EPR10181 - IC89 - 999E

EPR10183 - IC115 - 089E
EPR10184 - IC116 - 1CD2
EPR10258 - IC117 - 40FE
EPR10259 - IC118 - 9CBF
EPR10261 - IC130 - 7DCE
EPR10262 - IC131 - 43C1
EPR10263 - IC132 - 905E
EPR10264 - IC133 - 8498

Video Board (834-6065 Revision A)
-----------
EPR10194 - IC26 - 8C35
EPR10195 - IC27 - 4012
EPR10196 - IC28 - C4D8
EPR10197 - IC29 - FD47
EPR10198 - IC30 - BF34
EPR10199 - IC31 - DD89
EPR10200 - IC32 - A707
EPR10201 - IC33 - E288

EPR10203 - IC38 - 3703
EPR10204 - IC39 - 861B
EPR10205 - IC40 - 36C5
EPR10206 - IC41 - 5F40
EPR10207 - IC42 - F500
EPR10208 - IC43 - D932
EPR10209 - IC44 - D464
EPR10210 - IC45 - 4D74

EPR10212 - IC52 - 707D
EPR10213 - IC53 - 8204
EPR10214 - IC54 - 79C4
EPR10215 - IC55 - 0236
EPR10216 - IC56 - 5738
EPR10217 - IC57 - E265
EPR10218 - IC58 - 9571
EPR10219 - IC59 - A8C9

EPR10221 - IC66 - 224E
EPR10222 - IC67 - 4677
EPR10223 - IC68 - D3BF
EPR10224 - IC69 - 03A7
EPR10225 - IC70 - 1215
EPR10226 - IC71 - C3B8
EPR10227 - IC72 - 5595
EPR10228 - IC73 - 934B

*/

ROM_START( outrundx )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "epr10183.bin", 0x000000, 0x8000, CRC(3d992396) SHA1(8cef43799b71cfd36d3fea140afff7fe0bafcfc1) )
	ROM_LOAD16_BYTE( "epr10261.bin", 0x000001, 0x8000, CRC(1d034847) SHA1(664b24c13f7885403328906682213e38c1ad994e) )
	ROM_LOAD16_BYTE( "epr10184.bin", 0x010000, 0x8000, CRC(1a73dc46) SHA1(70f31619e80eb3d70747e7006e135c8bc0a31675) )
	ROM_LOAD16_BYTE( "epr10262.bin", 0x010001, 0x8000, CRC(5386b6b3) SHA1(a554ed1b4e07811c4accc59c063baa42949b6670) )
	ROM_LOAD16_BYTE( "epr10258.bin", 0x020000, 0x8000, CRC(39408e4f) SHA1(4f7f8b393dfb1e1935d595ae55a6913a27b02f80) )
	ROM_LOAD16_BYTE( "epr10263.bin", 0x020001, 0x8000, CRC(eda65fd6) SHA1(dd9c072856edffff3e73423f22ab40c5893bd26f) )
	ROM_LOAD16_BYTE( "epr10259.bin", 0x030000, 0x8000, CRC(95100b1a) SHA1(d2a5eb97623321b6c943bc435de26bf5d39ea312) )
	ROM_LOAD16_BYTE( "epr10264.bin", 0x030001, 0x8000, CRC(cc94b102) SHA1(29dc7e2a8509d0b5d30e2fb9404e0517b97f64e8) )


	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "epr10194.bin", 0x00000, 0x08000, CRC(f0eda3bd) SHA1(173e10a10372d42da81e6eb48c3e23a117638c0c) )
	ROM_LOAD( "epr10195.bin", 0x08000, 0x08000, CRC(0de75cdd) SHA1(a97faea76aca663ccbbde327f3d1d8ae256649d3) )
	ROM_LOAD( "epr10196.bin", 0x10000, 0x08000, CRC(8688bb59) SHA1(0aaa90c5101aa1db00db776a15a0a525587dfc43) )
	ROM_LOAD( "epr10197.bin", 0x18000, 0x08000, CRC(009165a6) SHA1(987b91e8c5c54bb7c4520b13a72f1f47c34278f4) )
	ROM_LOAD( "epr10198.bin", 0x20000, 0x08000, CRC(d894992e) SHA1(451469f743a0019b8797d16ba7b26a267d13fe06) )
	ROM_LOAD( "epr10199.bin", 0x28000, 0x08000, CRC(86376af6) SHA1(971f4b0d9a01ca7ffb50cefbe1ab41b703a4a41a) )
	ROM_LOAD( "epr10200.bin", 0x30000, 0x08000, CRC(1e5d4f73) SHA1(79deddf4461dad5784441c2839894207b7d2ecac) )
	ROM_LOAD( "epr10201.bin", 0x38000, 0x08000, CRC(1d9d4b9c) SHA1(3264b66c87aa7de4c140450b96adbe3071231d4a) )
	ROM_LOAD( "epr10203.bin", 0x40000, 0x08000, CRC(8445a622) SHA1(1187dee7db09a42446fc75872d49936310141eb8) )
	ROM_LOAD( "epr10204.bin", 0x48000, 0x08000, CRC(5f4b5abb) SHA1(f81637b2eb6a4bde76c43eedfad7e5375594c7bd) )
	ROM_LOAD( "epr10205.bin", 0x50000, 0x08000, CRC(74bd93ca) SHA1(6a02ea3b977e56cfd61302afa2abf6c2dc766ba7) )
	ROM_LOAD( "epr10206.bin", 0x58000, 0x08000, CRC(954542c5) SHA1(3c67e3568c04ba083f4aacad2e8857cdd16b3b2f) )
	ROM_LOAD( "epr10207.bin", 0x60000, 0x08000, CRC(ca61cea4) SHA1(7c39e2863f5c7be290522acdaf046b1dab7a3542) )
	ROM_LOAD( "epr10208.bin", 0x68000, 0x08000, CRC(6830b7fa) SHA1(3ece1971a4f025104ebd026da6751caea9aa8a64) )
	ROM_LOAD( "epr10209.bin", 0x70000, 0x08000, CRC(5c15419e) SHA1(7b4e9c0cb430afae7f927c0224021add0a627251) )
	ROM_LOAD( "epr10210.bin", 0x78000, 0x08000, CRC(39422931) SHA1(8d8a3f4597945c92aebd20c0784180696b6c9c1c) )
	ROM_LOAD( "epr10212.bin", 0x80000, 0x08000, CRC(dee7e731) SHA1(f09d18f8d8405025b87dd01488ad2098e28410b0) )
	ROM_LOAD( "epr10213.bin", 0x88000, 0x08000, CRC(1d1b22f0) SHA1(d3b1c36d08c4b7b08f9969a521e62eebd5b2238d) )
	ROM_LOAD( "epr10214.bin", 0x90000, 0x08000, CRC(57527e18) SHA1(4cc95c4b741f495e5b9c3b9d4d9ab9a6fded9aeb) )
	ROM_LOAD( "epr10215.bin", 0x98000, 0x08000, CRC(69be5a6c) SHA1(2daac5877a71de04878f231f03361f697552431f) )
	ROM_LOAD( "epr10216.bin", 0xa0000, 0x08000, CRC(d394134d) SHA1(42f768a9c9eb9f556d197548c35b3a0cd5414734) )
	ROM_LOAD( "epr10217.bin", 0xa8000, 0x08000, CRC(bf2c9b76) SHA1(248e273255968115a60855b1fffcce1dbeacc3d4) )
	ROM_LOAD( "epr10218.bin", 0xb0000, 0x08000, CRC(db4bdb39) SHA1(b4661611b28e7ff1c721565175038cfd1e99d383) )
	ROM_LOAD( "epr10219.bin", 0xb8000, 0x08000, CRC(e73b9224) SHA1(1904a71a0c18ab2a3a5929e72b1c215dbb0fa213) )
	ROM_LOAD( "epr10221.bin", 0xc0000, 0x08000, CRC(43431387) SHA1(a28896e888bc4d4f67babd49003d663c1ceabb71) )
	ROM_LOAD( "epr10222.bin", 0xc8000, 0x08000, CRC(a254c706) SHA1(e2801a0a7fd5546a48cd53ad7e4743d821d985ff) )
	ROM_LOAD( "epr10223.bin", 0xd0000, 0x08000, CRC(3850690e) SHA1(0f92743f848edc8deaeeef3afca5f662ceba61e7) )
	ROM_LOAD( "epr10224.bin", 0xd8000, 0x08000, CRC(5cffc346) SHA1(0481f864bb584c96cd92c260a62c0c1d4030bde8) )
	ROM_LOAD( "epr10225.bin", 0xe0000, 0x08000, CRC(0a5d1f2b) SHA1(43d9c7539b6cebbac3395a4ba71a702300c9e644) )
	ROM_LOAD( "epr10226.bin", 0xe8000, 0x08000, CRC(5a452474) SHA1(6789a33b55a1693ec9cc196b3ebd220b14169e08) )
	ROM_LOAD( "epr10227.bin", 0xf0000, 0x08000, CRC(c7def392) SHA1(fa7d1245eefdc3abb9520118bbb0d025ca62901e) )
	ROM_LOAD( "epr10228.bin", 0xf8000, 0x08000, CRC(25803978) SHA1(1a18922aeb516e8deb026d52e3cdcc4e69385af5) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, CRC(a10abaa9) SHA1(01c8a819587a66d2ee4d255656e36fa0904377b0) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) // twice??
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "epr10173.bin", 0x000000, 0x8000, CRC(6c2775c0) SHA1(2dd3a4e7f7b8808da74fbd53423a83775afff5d5) )
	ROM_LOAD16_BYTE( "epr10178.bin", 0x000001, 0x8000, CRC(6d36be05) SHA1(02527701451bbdfa14280ef4db6f4d540e6ee470) )
	ROM_LOAD16_BYTE( "epr10174.bin", 0x010000, 0x8000, CRC(aae7efad) SHA1(bbc68daafc8bb61d0b065baa3a3583e95de4d9ad) )
	ROM_LOAD16_BYTE( "epr10179.bin", 0x010001, 0x8000, CRC(180fd041) SHA1(87f1566cff1bded7642e260b8337a278052727bb) )
	ROM_LOAD16_BYTE( "epr10175.bin", 0x020000, 0x8000, CRC(31c76063) SHA1(a3069c5443e7f87c38a69530f00ccc6e9a8eac42) )
	ROM_LOAD16_BYTE( "epr10180.bin", 0x020001, 0x8000, CRC(4713b264) SHA1(ab498b5232520657bae841927ee74994a6fb1c4e) )
	ROM_LOAD16_BYTE( "epr10176.bin", 0x030000, 0x8000, CRC(a7811f90) SHA1(a2ac49f0947ddddbbdaa90ebdefd232fdbf27c35) )
	ROM_LOAD16_BYTE( "epr10181.bin", 0x030001, 0x8000, CRC(e009a04d) SHA1(f3253a0feb6acd08238e025e7ab8b5cb175d1c67) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, CRC(22794426) SHA1(a554d4b68e71861a0d0da4d031b3b811b246f082) )
ROM_END


ROM_START( outruna )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "10380b", 0x000000, 0x10000, CRC(1f6cadad) SHA1(31e870f307f44eb4f293b607123b623beee2bc3c) )
	ROM_LOAD16_BYTE( "10382b", 0x000001, 0x10000, CRC(c4c3fa1a) SHA1(69236cf9f27691dee290c79db1fc9b5e73ea77d7) )
	ROM_LOAD16_BYTE( "10381a", 0x020000, 0x10000, CRC(be8c412b) SHA1(bf3ff05bbf81bdd44567f3b9bb4919ed4a499624) )
	ROM_LOAD16_BYTE( "10383b", 0x020001, 0x10000, CRC(10a2014a) SHA1(1970895145ad8b5735f66ed8c837d9d453ce9b23) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "10371", 0x00000, 0x20000, CRC(0a1c98de) SHA1(e683ab79bfc6f8c60974912f07ef8173b167f448) )
	ROM_LOAD( "10372", 0x20000, 0x20000, CRC(1640ad1f) SHA1(3223d536c61f349be8de9ee32fc210f61fe822e2) )
	ROM_LOAD( "10373", 0x40000, 0x20000, CRC(339f8e64) SHA1(0d79b584e9e6c838cb0a9d2e8a611913265a06d2) )
	ROM_LOAD( "10374", 0x60000, 0x20000, CRC(22744340) SHA1(0997687641b0fc5c81343220166777dd74cc12a8) )
	ROM_LOAD( "10375", 0x80000, 0x20000, CRC(62a472bd) SHA1(e753df3ad378e6d4e22d5978626fc2370826bcf0) )
	ROM_LOAD( "10376", 0xa0000, 0x20000, CRC(8337ace7) SHA1(6bb3739a730c8d026fe9a6e831fc7c9fc13eae4f) )
	ROM_LOAD( "10377", 0xc0000, 0x20000, CRC(c86daecb) SHA1(78f61038f54ca56aab1f73d662fbaf07e396075f) )
	ROM_LOAD( "10378", 0xe0000, 0x20000, CRC(544068fd) SHA1(54fe03d2d80548ebba9c2578ece5d0f89593cb3e) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "10187",       0x00000, 0x8000, CRC(a10abaa9) SHA1(01c8a819587a66d2ee4d255656e36fa0904377b0) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) // twice??
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "10327a", 0x00000, 0x10000, CRC(e28a5baf) SHA1(f715bde96c73ed47035acf5a41630fdeb41bb2f9) )
	ROM_LOAD16_BYTE( "10329a", 0x00001, 0x10000, CRC(da131c81) SHA1(57d5219bd0e2fd886217e37e8773fd76be9b40eb) )
	ROM_LOAD16_BYTE( "10328a", 0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "10330a", 0x20001, 0x10000, CRC(ba9ec82a) SHA1(2136c9572e26b7ae6de402c0cd53174407cc6018) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "10185", 0x0000, 0x8000, CRC(22794426) SHA1(a554d4b68e71861a0d0da4d031b3b811b246f082) )
ROM_END


ROM_START( outrunb )
	ROM_REGION( 0x040000, REGION_CPU1, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "orun_mn.rom", 0x000000, 0x10000, CRC(cddceea2) SHA1(34cb4ca61c941e96e585f3cd2aed79bdde67f8eb) )
	ROM_LOAD16_BYTE( "orun_ml.rom", 0x000001, 0x10000, CRC(9cfc07d5) SHA1(b1b5992ff99e4158bb008684e694e088a4b282c6) )
	ROM_LOAD16_BYTE( "orun_mm.rom", 0x020000, 0x10000, CRC(3092d857) SHA1(8ebfeab9217b80a7983a4f8eb7bb7d3387d791b3) )
	ROM_LOAD16_BYTE( "orun_mk.rom", 0x020001, 0x10000, CRC(30a1c496) SHA1(734c82930197e6e8cd2bea145aedda6b3c1145d0) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "10268", 0x00000, 0x08000, CRC(95344b04) SHA1(b3480714b11fc49b449660431f85d4ba92f799ba) )
	ROM_LOAD( "10232", 0x08000, 0x08000, CRC(776ba1eb) SHA1(e3477961d19e694c97643066534a1f720e0c4327) )
	ROM_LOAD( "10267", 0x10000, 0x08000, CRC(a85bb823) SHA1(a7e0143dee5a47e679fd5155e58e717813912692) )
	ROM_LOAD( "10231", 0x18000, 0x08000, CRC(8908bcbf) SHA1(8e1237b640a6f26bdcbfd5e201dadb2687c4febb) )
	ROM_LOAD( "10266", 0x20000, 0x08000, CRC(9f6f1a74) SHA1(09164e858ebeedcff4d389524ddf89e7c216dcae) )
	ROM_LOAD( "10230", 0x28000, 0x08000, CRC(686f5e50) SHA1(03697b892f911177968aa40de6c5f464eb0258e7) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD( "orun_1.rom", 	0x00000, 0x10000, CRC(77377e00) SHA1(4f376b05692f33d529f4c058dac989136c808ca1) )
	ROM_LOAD( "orun_17.rom", 	0x10000, 0x10000, CRC(4f784236) SHA1(1fb610fd29d3ddd8c5d4892ae215386b18552e6f) )
	ROM_LOAD( "orun_2.rom", 	0x20000, 0x10000, CRC(2c0e7277) SHA1(cf14d1ca1fba2e2687998c04ad2ab8c629917412) )
	ROM_LOAD( "orun_18.rom", 	0x30000, 0x10000, CRC(8d459356) SHA1(143914b1ac074708fed1d89072f915424aeb841e) )
	ROM_LOAD( "orun_3.rom", 	0x40000, 0x10000, CRC(69ecc975) SHA1(3560e9a31fc71e263a6ff61224b8db2b17836075) )
	ROM_LOAD( "orun_19.rom", 	0x50000, 0x10000, CRC(ee4f7154) SHA1(3a84c1b19d9dfcd5310e9cee90c0d4562a4a7786) )
	ROM_LOAD( "orun_4.rom", 	0x60000, 0x10000, CRC(54761e57) SHA1(dc0fc645eb998675ab9fe683d63d4ee57ae23693) )
	ROM_LOAD( "orun_20.rom",	0x70000, 0x10000, CRC(c2825654) SHA1(566ecb6e3dc76300351e54e4c0f24b9c2a6c710c) )
	ROM_LOAD( "orun_5.rom", 	0x80000, 0x10000, CRC(b6a8d0e2) SHA1(6184700dbe2c8c9c91f220e246501b7a865e4a05) )
	ROM_LOAD( "orun_21.rom", 	0x90000, 0x10000, CRC(e9880aa3) SHA1(cc47f631e758bd856bbc6d010fe230f9b1ed29de) )
	ROM_LOAD( "orun_6.rom", 	0xa0000, 0x10000, CRC(a00d0676) SHA1(c2ab29a7489c6f774ce26ef023758215ea3f7050) )
	ROM_LOAD( "orun_22.rom",	0xb0000, 0x10000, CRC(ef7d06fe) SHA1(541b5ba45f4140e2cc29a9da2592b476d414af5d) )
	ROM_LOAD( "orun_7.rom", 	0xc0000, 0x10000, CRC(d632d8a2) SHA1(27ca6faaa073bd01b2be959dba0359f93e8c1ec1) )
	ROM_LOAD( "orun_23.rom", 	0xd0000, 0x10000, CRC(dc286dc2) SHA1(eaa245b81f8a324988f617467fc3134a39b59c65) )
	ROM_LOAD( "orun_8.rom", 	0xe0000, 0x10000, CRC(da398368) SHA1(115b2881d2d5ddeda2ce82bb209a2c0b4acfcae4) )
	ROM_LOAD( "orun_24.rom",	0xf0000, 0x10000, CRC(1222af9f) SHA1(2364bd54cbe21dd688efff32e93bf154546c93d6) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "orun_ma.rom", 0x00000, 0x8000, CRC(a3ff797a) SHA1(d97318a0602965cb5059c69a68609691d55a8e41) )

	ROM_REGION( 0x38000, REGION_SOUND1, 0 ) /* Sega PCM sound data */
	ROM_LOAD( "10193",       0x00000, 0x8000, CRC(bcd10dde) SHA1(417ce1d7242884640c5b14f4db8ee57cde7d085d) )
	ROM_RELOAD(              0x30000, 0x8000 ) // twice??
	ROM_LOAD( "10192",       0x08000, 0x8000, CRC(770f1270) SHA1(686bdf44d45c1d6002622f6658f037735382f3e0) )
	ROM_LOAD( "10191",       0x10000, 0x8000, CRC(20a284ab) SHA1(7c9027416d4122791ba53782fe2230cf02b7d506) )
	ROM_LOAD( "10190",       0x18000, 0x8000, CRC(7cab70e2) SHA1(a3c581d2b438630d0d4c39481dcfd85681c9f889) )
	ROM_LOAD( "10189",       0x20000, 0x8000, CRC(01366b54) SHA1(f467a6b807694d5832a985f5381c170d24aaee4e) )
	ROM_LOAD( "10188",       0x28000, 0x8000, CRC(bad30ad9) SHA1(f70dd3a6362c314adef313b064102f7a250401c8) )

	ROM_REGION( 0x40000, REGION_CPU3, 0 ) /* second 68000 CPU */
	ROM_LOAD16_BYTE( "orun_mj.rom", 0x00000, 0x10000, CRC(d7f5aae0) SHA1(0f9b693f078cdbbfeade5a373a94a20110d586ca) )
	ROM_LOAD16_BYTE( "orun_mh.rom", 0x00001, 0x10000, CRC(88c2e78f) SHA1(198cab9133345e4529f7fb52c29974c9a1a84933) )
	ROM_LOAD16_BYTE( "10328a",      0x20000, 0x10000, CRC(d5ec5e5d) SHA1(a4e3cfca4d803e72bc4fcf91ab00e21bf3f8959f) )
	ROM_LOAD16_BYTE( "orun_mg.rom", 0x20001, 0x10000, CRC(74c5fbec) SHA1(a44f1477d830fdb4d6c29351da94776843e5d3e1) )

	ROM_REGION( 0x80000, REGION_GFX3, 0 ) /* Road Graphics  (region size should be gr_bitmapwidth*256, 0 )*/
	ROM_LOAD( "orun_me.rom", 0x0000, 0x8000, CRC(666fe754) SHA1(606090db53d658d7b04dca4748014a411e12f259) )

//	ROM_LOAD( "orun_mf.rom", 0x0000, 0x8000, CRC(ed5bda9c) )	//??
ROM_END

// Turbo Outrun

ROM_START( toutrun )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
// custom cpu 317-0106
	ROM_LOAD16_BYTE( "epr12397.133", 0x000000, 0x10000, CRC(e4b57d7d) SHA1(62be55356c82b38ebebcc87a5458e23300019339) )
	ROM_LOAD16_BYTE( "epr12396.118", 0x000001, 0x10000, CRC(5e7115cb) SHA1(02c9ec91d9afb424e5045671ab0b5499181728c9) )
	ROM_LOAD16_BYTE( "epr12399.132", 0x020000, 0x10000, CRC(62c77b1b) SHA1(004803c68cb1b3e414296ffbf50dc3b33b9ffb9a) )
	ROM_LOAD16_BYTE( "epr12398.117", 0x020001, 0x10000, CRC(18e34520) SHA1(3f10ecb809106b82fd44fd6244d8d8e7f1c8e08d) )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, CRC(f4321eea) SHA1(64334acc82c14bb58b7d51719f34fd81cfb9fc6b) )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, CRC(51d98af0) SHA1(6e7115706bfafb687faa23d55d4a8c8e498a4df2) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, CRC(4de43a6f) SHA1(68909338e1f192ac2699c8a8d24c3f46502dd019) )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, CRC(24607a55) SHA1(69033f2281cd42e88233c23d809b73607fe54853) )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, CRC(1405137a) SHA1(367db88d36852e35c5e839f692be5ea8c8e072d2) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr12307.9",  0x00001, 0x10000, CRC(437dcf09) SHA1(0022ee4d1c3698f77271e570cef98a8a1e5c5d6a) )
	ROM_LOAD16_BYTE( "opr12308.10", 0x00000, 0x10000, CRC(0de70cc2) SHA1(c03f8f8cda72daf64af2878bf254840ac6dd17eb) )
	ROM_LOAD16_BYTE( "opr12309.11", 0x20001, 0x10000, CRC(deb8c242) SHA1(c05d8ced4eafae52c4795fb1471cd66f5903d1aa) )
	ROM_LOAD16_BYTE( "opr12310.12", 0x20000, 0x10000, CRC(45cf157e) SHA1(5d0be2a374a53ea1fe0ba2bf9b2173e96de1eb51) )
	ROM_LOAD16_BYTE( "opr12311.13", 0x40001, 0x10000, CRC(ae2bd639) SHA1(64bb60ae7e3f87fbbce00106ba65c4e6fc1af0e4) )
	ROM_LOAD16_BYTE( "opr12312.14", 0x40000, 0x10000, CRC(626000e7) SHA1(4a7f9e76dd76a3dc56b8257149bc94be3f4f2e87) )
	ROM_LOAD16_BYTE( "opr12313.15", 0x60001, 0x10000, CRC(52870c37) SHA1(3a6836a46d94c0f9115800d206410252a1134c57) )
	ROM_LOAD16_BYTE( "opr12314.16", 0x60000, 0x10000, CRC(40c461ea) SHA1(7bed8f24112dc3c827fd087138fcf2700092aa59) )
	ROM_LOAD16_BYTE( "opr12315.17", 0x80001, 0x10000, CRC(3ff9a3a3) SHA1(0d90fe2669d03bd07a0d3b05934201778e28d54c) )
	ROM_LOAD16_BYTE( "opr12316.18", 0x80000, 0x10000, CRC(8a1e6dc8) SHA1(32f09ec504c2b6772815bad7380a2f738f11746a) )
	ROM_LOAD16_BYTE( "opr12317.19", 0xa0001, 0x10000, CRC(77e382d4) SHA1(5b7912069a46043b7be989d82436add85497d318) )
	ROM_LOAD16_BYTE( "opr12318.20", 0xa0000, 0x10000, CRC(d1afdea9) SHA1(813eccc88d5046992be5b5a0618d32127d16e30b) )
	ROM_LOAD16_BYTE( "opr12320.22", 0xc0001, 0x10000, CRC(7931e446) SHA1(9f2161a689ebad61f6653942e23d9c2bc6170d4a) )
	ROM_LOAD16_BYTE( "opr12321.23", 0xc0000, 0x10000, CRC(830bacd4) SHA1(5a4816969437ee1edca5845006c0b8e9ba365491) )
	ROM_LOAD16_BYTE( "opr12322.24", 0xe0001, 0x10000, CRC(8b812492) SHA1(bf1f9e059c093c0991c7caf1b01c739ed54b8357) )
	ROM_LOAD16_BYTE( "opr12319.25", 0xe0000, 0x10000, CRC(df23baf9) SHA1(f9611391bb3b3b92203fa9f6dd461e3a6e863622) )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, CRC(e8ff7011) SHA1(6eaf3aea507007ea31d507ed7825d905f4b8e7ab) )
	ROM_LOAD( "opr12301.66",    0x10000, 0x10000, CRC(6e78ad15) SHA1(c31ddf434b459cd1a381d2a028beabddd4ed10d2) )
	ROM_LOAD( "opr12302.67",    0x20000, 0x10000, CRC(e72928af) SHA1(40e0b178958cfe97c097fe9d82b5de54bc27a29f) )
	ROM_LOAD( "opr12303.68",    0x30000, 0x10000, CRC(8384205c) SHA1(c1f9d52bc587eab5a97867198e9aa7c19e973429) )
	ROM_LOAD( "opr12304.69",    0x40000, 0x10000, CRC(e1762ac3) SHA1(855f06c082a17d90857e6efa3cf95b0eda0e634d) )
	ROM_LOAD( "opr12305.70",    0x50000, 0x10000, CRC(ba9ce677) SHA1(056781f92450c902e1d279a02bda28337815cba9) )
	ROM_LOAD( "opr12306.71",    0x60000, 0x10000, CRC(e49249fd) SHA1(ff36e4dba4e9d3d354e3dd528edeb50ad9c18ee4) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, CRC(d43a3a84) SHA1(362c98f62c205b6b40b7e8a4ba107745b547b984) )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, CRC(27cdcfd3) SHA1(4fe57db95b109ab1bb1326789e06a3d3aac311cc) )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, CRC(1d9b5677) SHA1(fb6e33acc43fbc7a8d7ac44045439ecdf794fdeb) )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, CRC(0a513671) SHA1(4c13ca3a6f0aa9d06ed80798b466cca0c966a265) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* road */
	ROM_LOAD( "epr12298.11", 0x0, 0x08000, CRC(fc9bc41b) SHA1(9af73e096253cf2c4f283f227530110a4b37fcee) )
ROM_END


ROM_START( toutruna )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 code */
// custom cpu 317-0106
	ROM_LOAD16_BYTE( "epr12410.133", 0x000000, 0x10000, CRC(aa74f3e9) SHA1(2daf6b17317542063c0a40beea5b45c797192591) )
	ROM_LOAD16_BYTE( "epr12409.118", 0x000001, 0x10000, CRC(c11c8ef7) SHA1(4c1c5100d7fd728642d58e4bf45fba48695841e3) )
	ROM_LOAD16_BYTE( "epr12412.132", 0x020000, 0x10000, CRC(b0534647) SHA1(40f2260ff0d0ac662d118cc7280bb26006ee75e9) )
	ROM_LOAD16_BYTE( "epr12411.117", 0x020001, 0x10000, CRC(12bb0d83) SHA1(4aa1b724b2a7258fff7aa1971582950b3163c0db) )
	ROM_LOAD16_BYTE( "epr12293.131", 0x040000, 0x10000, CRC(f4321eea) SHA1(64334acc82c14bb58b7d51719f34fd81cfb9fc6b) )
	ROM_LOAD16_BYTE( "epr12292.116", 0x040001, 0x10000, CRC(51d98af0) SHA1(6e7115706bfafb687faa23d55d4a8c8e498a4df2) )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "opr12323.102", 0x00000, 0x10000, CRC(4de43a6f) SHA1(68909338e1f192ac2699c8a8d24c3f46502dd019) )
	ROM_LOAD( "opr12324.103", 0x10000, 0x10000, CRC(24607a55) SHA1(69033f2281cd42e88233c23d809b73607fe54853) )
	ROM_LOAD( "opr12325.104", 0x20000, 0x10000, CRC(1405137a) SHA1(367db88d36852e35c5e839f692be5ea8c8e072d2) )

	ROM_REGION( 0x100000, REGION_GFX2, 0 ) /* sprites */
	ROM_LOAD16_BYTE( "opr12307.9",  0x00001, 0x10000, CRC(437dcf09) SHA1(0022ee4d1c3698f77271e570cef98a8a1e5c5d6a) )
	ROM_LOAD16_BYTE( "opr12308.10", 0x00000, 0x10000, CRC(0de70cc2) SHA1(c03f8f8cda72daf64af2878bf254840ac6dd17eb) )
	ROM_LOAD16_BYTE( "opr12309.11", 0x20001, 0x10000, CRC(deb8c242) SHA1(c05d8ced4eafae52c4795fb1471cd66f5903d1aa) )
	ROM_LOAD16_BYTE( "opr12310.12", 0x20000, 0x10000, CRC(45cf157e) SHA1(5d0be2a374a53ea1fe0ba2bf9b2173e96de1eb51) )
	ROM_LOAD16_BYTE( "opr12311.13", 0x40001, 0x10000, CRC(ae2bd639) SHA1(64bb60ae7e3f87fbbce00106ba65c4e6fc1af0e4) )
	ROM_LOAD16_BYTE( "opr12312.14", 0x40000, 0x10000, CRC(626000e7) SHA1(4a7f9e76dd76a3dc56b8257149bc94be3f4f2e87) )
	ROM_LOAD16_BYTE( "opr12313.15", 0x60001, 0x10000, CRC(52870c37) SHA1(3a6836a46d94c0f9115800d206410252a1134c57) )
	ROM_LOAD16_BYTE( "opr12314.16", 0x60000, 0x10000, CRC(40c461ea) SHA1(7bed8f24112dc3c827fd087138fcf2700092aa59) )
	ROM_LOAD16_BYTE( "opr12315.17", 0x80001, 0x10000, CRC(3ff9a3a3) SHA1(0d90fe2669d03bd07a0d3b05934201778e28d54c) )
	ROM_LOAD16_BYTE( "opr12316.18", 0x80000, 0x10000, CRC(8a1e6dc8) SHA1(32f09ec504c2b6772815bad7380a2f738f11746a) )
	ROM_LOAD16_BYTE( "opr12317.19", 0xa0001, 0x10000, CRC(77e382d4) SHA1(5b7912069a46043b7be989d82436add85497d318) )
	ROM_LOAD16_BYTE( "opr12318.20", 0xa0000, 0x10000, CRC(d1afdea9) SHA1(813eccc88d5046992be5b5a0618d32127d16e30b) )
	ROM_LOAD16_BYTE( "opr12320.22", 0xc0001, 0x10000, CRC(7931e446) SHA1(9f2161a689ebad61f6653942e23d9c2bc6170d4a) )
	ROM_LOAD16_BYTE( "opr12321.23", 0xc0000, 0x10000, CRC(830bacd4) SHA1(5a4816969437ee1edca5845006c0b8e9ba365491) )
	ROM_LOAD16_BYTE( "opr12322.24", 0xe0001, 0x10000, CRC(8b812492) SHA1(bf1f9e059c093c0991c7caf1b01c739ed54b8357) )
	ROM_LOAD16_BYTE( "opr12319.25", 0xe0000, 0x10000, CRC(df23baf9) SHA1(f9611391bb3b3b92203fa9f6dd461e3a6e863622) )

	ROM_REGION( 0x70000, REGION_CPU2, 0 ) /* sound CPU */
	ROM_LOAD( "epr12300.88",	0x00000, 0x10000, CRC(e8ff7011) SHA1(6eaf3aea507007ea31d507ed7825d905f4b8e7ab) )
	ROM_LOAD( "opr12301.66",    0x10000, 0x10000, CRC(6e78ad15) SHA1(c31ddf434b459cd1a381d2a028beabddd4ed10d2) )
	ROM_LOAD( "opr12302.67",    0x20000, 0x10000, CRC(e72928af) SHA1(40e0b178958cfe97c097fe9d82b5de54bc27a29f) )
	ROM_LOAD( "opr12303.68",    0x30000, 0x10000, CRC(8384205c) SHA1(c1f9d52bc587eab5a97867198e9aa7c19e973429) )
	ROM_LOAD( "opr12304.69",    0x40000, 0x10000, CRC(e1762ac3) SHA1(855f06c082a17d90857e6efa3cf95b0eda0e634d) )
	ROM_LOAD( "opr12305.70",    0x50000, 0x10000, CRC(ba9ce677) SHA1(056781f92450c902e1d279a02bda28337815cba9) )
	ROM_LOAD( "opr12306.71",    0x60000, 0x10000, CRC(e49249fd) SHA1(ff36e4dba4e9d3d354e3dd528edeb50ad9c18ee4) )

	ROM_REGION( 0x100000, REGION_CPU3, 0 ) /* 68000 code */
	ROM_LOAD16_BYTE( "opr12295.76", 0x000000, 0x10000, CRC(d43a3a84) SHA1(362c98f62c205b6b40b7e8a4ba107745b547b984) )
	ROM_LOAD16_BYTE( "opr12294.58", 0x000001, 0x10000, CRC(27cdcfd3) SHA1(4fe57db95b109ab1bb1326789e06a3d3aac311cc) )
	ROM_LOAD16_BYTE( "opr12297.75", 0x020000, 0x10000, CRC(1d9b5677) SHA1(fb6e33acc43fbc7a8d7ac44045439ecdf794fdeb) )
	ROM_LOAD16_BYTE( "opr12296.57", 0x020001, 0x10000, CRC(0a513671) SHA1(4c13ca3a6f0aa9d06ed80798b466cca0c966a265) )

	ROM_REGION( 0x40000, REGION_GFX3, 0 ) /* road */
	ROM_LOAD( "epr12298.11", 0x0, 0x08000, CRC(fc9bc41b) SHA1(9af73e096253cf2c4f283f227530110a4b37fcee) )
ROM_END

/***************************************************************************/

#if 0
static READ16_HANDLER( or_io_joy_r ){
	return (input_port_5_r( offset ) << 8) + input_port_6_r( offset );
}
#endif

#ifdef HANGON_DIGITAL_CONTROLS
static READ16_HANDLER( or_io_brake_r ){
	int data = input_port_1_r( offset );

	switch(data & 3)
	{
		case 3:	return 0xff00;	// both
		case 1:	return 0xff00;  // brake
		case 2:	return 0x0000;  // accel
		case 0:	return 0x0000;  // neither
	}
	return 0x0000;
}

static READ16_HANDLER( or_io_acc_steer_r ){
	int data = input_port_1_r( offset );
	int ret = input_port_0_r( offset ) << 8;

	switch(data & 3)
	{
		case 3:	return 0x00 | ret;	// both
		case 1:	return 0x00 | ret;  // brake
		case 2:	return 0xff | ret;  // accel
		case 0:	return 0x00 | ret ;  // neither
	}
	return 0x00 | ret;
}
#else
static READ16_HANDLER( or_io_acc_steer_r ){ return (input_port_0_r( offset ) << 8) + input_port_1_r( offset ); }
static READ16_HANDLER( or_io_brake_r ){ return input_port_5_r( offset ) << 8; }
#endif

static int selected_analog;

static READ16_HANDLER( outrun_analog_r )
{
	switch (selected_analog)
	{
		default:
		case 0: return or_io_acc_steer_r(0,0) >> 8;
		case 1: return or_io_acc_steer_r(0,0) & 0xff;
		case 2: return or_io_brake_r(0,0) >> 8;
		case 3: return or_io_brake_r(0,0) & 0xff;
	}
}

static WRITE16_HANDLER( outrun_analog_select_w )
{
	if ( ACCESSING_LSB )
	{
		selected_analog = (data & 0x0c) >> 2;
	}
}

static int or_gear=0;

static READ16_HANDLER( or_io_service_r )
{
	int ret=input_port_2_r( offset );
	int data=input_port_1_r( offset );
	if(data & 4) or_gear=0;
	else if(data & 8) or_gear=1;

	if(or_gear) ret|=0x10;
	else ret&=0xef;

	return ret;
}

static WRITE16_HANDLER( outrun_sound_write_w )
{
	sound_shared_ram[0]=data&0xff;
}


static WRITE16_HANDLER( outrun_ctrl2_w )
{
	if( ACCESSING_LSB ){
		/* bit 0 always 1? */
		set_led_status(0,data & 0x04);
		set_led_status(1,data & 0x02);	/* brakes */
		coin_counter_w(0,data & 0x10);
	}
}
static unsigned char ctrl1;

static WRITE16_HANDLER( outrun_ctrl1_w )
{
	if(ACCESSING_LSB) {
		int changed = data ^ ctrl1;
		ctrl1 = data;
		if(changed & 1) {
			if(data & 1) {
				cpu_set_halt_line(2, CLEAR_LINE);
				cpu_set_reset_line(2, PULSE_LINE);
			} else
				cpu_set_halt_line(2, ASSERT_LINE);
		}

//		sys16_kill_set(data & 0x20);

		/* bit 0 always 1? */
		/* bits 2-3 continuously change: 00-01-10-11; this is the same that
		   gets written to 140030 so is probably input related */
	}
}

static void outrun_reset(void)
{
       cpu_set_reset_line(2, PULSE_LINE);
}



static ADDRESS_MAP_START( outrun_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060900, 0x060907) AM_READ(sound_shared_ram_r)		//???
	AM_RANGE(0x060000, 0x067fff) AM_READ(SYS16_MRA16_EXTRAM2)

	AM_RANGE(0x100000, 0x10ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x110000, 0x110fff) AM_READ(SYS16_MRA16_TEXTRAM)

	AM_RANGE(0x130000, 0x130fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0x120000, 0x121fff) AM_READ(SYS16_MRA16_PALETTERAM)

	AM_RANGE(0x140010, 0x140011) AM_READ(or_io_service_r)
	AM_RANGE(0x140014, 0x140015) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0x140016, 0x140017) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0x140030, 0x140031) AM_READ(outrun_analog_r)

	AM_RANGE(0x200000, 0x23ffff) AM_READ(SYS16_CPU3ROM16_r)
	AM_RANGE(0x260000, 0x267fff) AM_READ(shared_ram_r)
	AM_RANGE(0xe00000, 0xe00001) AM_READ(SYS16_CPU2_RESET_HACK)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060900, 0x060907) AM_WRITE(sound_shared_ram_w)		//???
	AM_RANGE(0x060000, 0x067fff) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0x100000, 0x10ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x110000, 0x110fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x130000, 0x130fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0x120000, 0x121fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0x140004, 0x140005) AM_WRITE(outrun_ctrl1_w)
	AM_RANGE(0x140020, 0x140021) AM_WRITE(outrun_ctrl2_w)
	AM_RANGE(0x140030, 0x140031) AM_WRITE(outrun_analog_select_w)
	AM_RANGE(0x200000, 0x23ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x260000, 0x267fff) AM_WRITE(shared_ram_w) AM_BASE(&shared_ram)
	AM_RANGE(0xffff06, 0xffff07) AM_WRITE(outrun_sound_write_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x060000, 0x067fff) AM_READ(shared_ram_r)
	AM_RANGE(0x080000, 0x09ffff) AM_READ(SYS16_MRA16_EXTRAM)		// gr
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x060000, 0x067fff) AM_WRITE(shared_ram_w)
	AM_RANGE(0x080000, 0x09ffff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)		// gr
ADDRESS_MAP_END

// Outrun

static ADDRESS_MAP_START( outrun_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf0ff) AM_READ(SegaPCM_r)
	AM_RANGE(0xf100, 0xf7ff) AM_READ(MRA8_NOP)
	AM_RANGE(0xf800, 0xf807) AM_READ(sound2_shared_ram_r)
	AM_RANGE(0xf808, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( outrun_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf0ff) AM_WRITE(SegaPCM_w)
	AM_RANGE(0xf100, 0xf7ff) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf800, 0xf807) AM_WRITE(sound2_shared_ram_w) AM_BASE(&sound_shared_ram)
	AM_RANGE(0xf808, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/***************************************************************************/

static void outrun_update_proc( void ){
	set_fg_page( sys16_textram[0x740] );
	set_bg_page( sys16_textram[0x741] );
	sys16_fg_scrolly = sys16_textram[0x748];
	sys16_bg_scrolly = sys16_textram[0x749];
	sys16_fg_scrollx = sys16_textram[0x74c];
	sys16_bg_scrollx = sys16_textram[0x74d];
}

static MACHINE_INIT( outrun ){
	static int bank[8] = {
		7,0,1,2,
		3,4,5,6
	};
	sys16_obj_bank = bank;
	sys16_spritesystem = sys16_sprite_outrun;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;
	sys16_sprxoffset = -0xc0;
	ctrl1 = 0x20;

// *forced sound cmd (eww)
	if (!strcmp(Machine->gamedrv->name,"outrun")) sys16_patch_code( 0x55ed, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outruna")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrunb")) sys16_patch_code( 0x5661, 0x00);
	if (!strcmp(Machine->gamedrv->name,"outrundx")) sys16_patch_code( 0x559f, 0x00);

     cpu_set_m68k_reset(0, outrun_reset);


	sys16_update_proc = outrun_update_proc;

	sys16_gr_ver = &sys16_extraram[0];
	sys16_gr_hor = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0xc00/2;

	sys16_gr_palette= 0xf00 / 2;
	sys16_gr_palette_default = 0x800 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x00 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x06 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x00 / 2;

	sys16_gr_second_road = &sys16_extraram[0x8000];
}


static DRIVER_INIT( outrun )
{
	machine_init_sys16_onetime();
	sys16_interleave_sprite_data( 0x100000 );
	generate_gr_screen(512,2048,0,0,3,0x8000);
}

static DRIVER_INIT( outrunb )
{
	data16_t *RAM = (data16_t *)memory_region(REGION_CPU1);
	int i;

	machine_init_sys16_onetime();
/*
  Main Processor
	Comparing the bootleg with the custom bootleg, it seems that:-

  if even bytes &0x28 == 0x20 or 0x08 then they are xored with 0x28
  if odd bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  ie. data lines are switched.
*/

	for( i=0;i<0x40000;i+=2 ){
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		// even byte
		if((even&0x28) == 0x20 || (even&0x28) == 0x08) even^=0x28;

		// odd byte
		if((odd&0xc0) == 0x80 || (odd&0xc0) == 0x40) odd^=0xc0;

		RAM[i/2] = (even<<8)+odd;
	}

/*
  Second Processor

  if even bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0
  if odd bytes &0x0c == 0x04 or 0x08 then they are xored with 0x0c
*/
	RAM = (data16_t *)memory_region(REGION_CPU3);
	for(i=0;i<0x40000;i+=2)
	{
		data16_t word = RAM[i/2];
		UINT8 even = word>>8;
		UINT8 odd = word&0xff;

		// even byte
		if((even&0xc0) == 0x80 || (even&0xc0) == 0x40) even^=0xc0;

		// odd byte
		if((odd&0x0c) == 0x08 || (odd&0x0c) == 0x04) odd^=0x0c;

		RAM[i/2] = (even<<8)+odd;
	}
/*
  Road GFX

	rom orun_me.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

	rom orun_mf.rom
	if bytes &0xc0 == 0x40 or 0x80 then they are xored with 0xc0

  I don't know why there's 2 road roms, but I'm using orun_me.rom
*/
	{
		UINT8 *mem = memory_region(REGION_GFX3);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}

	generate_gr_screen(512,2048,0,0,3,0x8000);
	sys16_interleave_sprite_data( 0x100000 );

/*
  Z80 Code
	rom orun_ma.rom
	if bytes &0x60 == 0x40 or 0x20 then they are xored with 0x60

*/
	{
		UINT8 *mem = memory_region(REGION_CPU2);
		for(i=0;i<0x8000;i++){
			if( (mem[i]&0x60) == 0x20 || (mem[i]&0x60) == 0x40 ) mem[i]^=0x60;
		}
	}
}

/***************************************************************************/

INPUT_PORTS_START( outrun )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_CENTER, 100, 3, 0x48, 0xb8 )
//	PORT_ANALOG( 0xff, 0x7f, IPT_PADDLE , 70, 3, 0x48, 0xb8 )

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
//	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x02, "Up Cockpit" )
	PORT_DIPSETTING(    0x01, "Mini Up" )
	PORT_DIPSETTING(    0x03, "Moving" )
//	PORT_DIPSETTING(    0x00, "No Use" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, "Time" )
	PORT_DIPSETTING(    0x20, "Easy" )
	PORT_DIPSETTING(    0x30, "Normal" )
	PORT_DIPSETTING(    0x10, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0xc0, 0xc0, "Enemies" )
	PORT_DIPSETTING(    0x80, "Easy" )
	PORT_DIPSETTING(    0xc0, "Normal" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x30, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 0x30, 0x90 )

#endif

INPUT_PORTS_END

/***************************************************************************/
static INTERRUPT_GEN( or_interrupt ){
	int intleft=cpu_getiloops();
	if(intleft!=0) cpu_set_irq_line(0, 2, HOLD_LINE);
	else cpu_set_irq_line(0, 4, HOLD_LINE);
}


static MACHINE_DRIVER_START( outrun )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(outrun_readmem,outrun_writemem)
	MDRV_CPU_VBLANK_INT(or_interrupt,2)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(outrun_sound_readmem,outrun_sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(outrun_readmem2,outrun_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)

	MDRV_MACHINE_INIT(outrun)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(4096*ShadowColorsMultiplier)

	MDRV_VIDEO_START(outrun)
	MDRV_VIDEO_UPDATE(outrun)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k)
MACHINE_DRIVER_END

#if 0
static MACHINE_DRIVER_START( outruna )

	/* basic machine hardware */
	MDRV_IMPORT_FROM(outrun)

	MDRV_MACHINE_INIT(outruna)
MACHINE_DRIVER_END
#endif

static data16_t *shared_ram2;
static READ16_HANDLER( shared_ram2_r ){
	return shared_ram2[offset];
}
static WRITE16_HANDLER( shared_ram2_w ){
	COMBINE_DATA(&shared_ram2[offset]);
}

static ADDRESS_MAP_START( shangon_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x20c640, 0x20c647) AM_READ(sound_shared_ram_r)
	AM_RANGE(0x20c000, 0x20ffff) AM_READ(SYS16_MRA16_EXTRAM2)
	AM_RANGE(0x400000, 0x40ffff) AM_READ(SYS16_MRA16_TILERAM)
	AM_RANGE(0x410000, 0x410fff) AM_READ(SYS16_MRA16_TEXTRAM)
	AM_RANGE(0x600000, 0x600fff) AM_READ(SYS16_MRA16_SPRITERAM)
	AM_RANGE(0xa00000, 0xa00fff) AM_READ(SYS16_MRA16_PALETTERAM)
	AM_RANGE(0xc68000, 0xc68fff) AM_READ(shared_ram_r)
	AM_RANGE(0xc7c000, 0xc7ffff) AM_READ(shared_ram2_r)
	AM_RANGE(0xe00002, 0xe00003) AM_READ(sys16_coinctrl_r)
	AM_RANGE(0xe01000, 0xe01001) AM_READ(input_port_2_word_r) // service
	AM_RANGE(0xe0100c, 0xe0100d) AM_READ(input_port_4_word_r) // dip2
	AM_RANGE(0xe0100a, 0xe0100b) AM_READ(input_port_3_word_r) // dip1
	AM_RANGE(0xe030f8, 0xe030f9) AM_READ(ho_io_x_r)
	AM_RANGE(0xe030fa, 0xe030fb) AM_READ(ho_io_y_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x20c640, 0x20c647) AM_WRITE(sound_shared_ram_w)
	AM_RANGE(0x20c000, 0x20ffff) AM_WRITE(SYS16_MWA16_EXTRAM2) AM_BASE(&sys16_extraram2)
	AM_RANGE(0x400000, 0x40ffff) AM_WRITE(SYS16_MWA16_TILERAM) AM_BASE(&sys16_tileram)
	AM_RANGE(0x410000, 0x410fff) AM_WRITE(SYS16_MWA16_TEXTRAM) AM_BASE(&sys16_textram)
	AM_RANGE(0x600000, 0x600fff) AM_WRITE(SYS16_MWA16_SPRITERAM) AM_BASE(&sys16_spriteram)
	AM_RANGE(0xa00000, 0xa00fff) AM_WRITE(SYS16_MWA16_PALETTERAM) AM_BASE(&paletteram16)
	AM_RANGE(0xc68000, 0xc68fff) AM_WRITE(shared_ram_w) AM_BASE(&shared_ram)
	AM_RANGE(0xc7c000, 0xc7ffff) AM_WRITE(shared_ram2_w) AM_BASE(&shared_ram2)
	AM_RANGE(0xe00002, 0xe00003) AM_WRITE(sys16_3d_coinctrl_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_readmem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x454000, 0x45401f) AM_READ(SYS16_MRA16_EXTRAM3)
	AM_RANGE(0x7e8000, 0x7e8fff) AM_READ(shared_ram_r)
	AM_RANGE(0x7fc000, 0x7ffbff) AM_READ(shared_ram2_r)
	AM_RANGE(0x7ffc00, 0x7fffff) AM_READ(SYS16_MRA16_EXTRAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_writemem2, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x454000, 0x45401f) AM_WRITE(SYS16_MWA16_EXTRAM3) AM_BASE(&sys16_extraram3)
	AM_RANGE(0x7e8000, 0x7e8fff) AM_WRITE(shared_ram_w)
	AM_RANGE(0x7fc000, 0x7ffbff) AM_WRITE(shared_ram2_w)
	AM_RANGE(0x7ffc00, 0x7fffff) AM_WRITE(SYS16_MWA16_EXTRAM) AM_BASE(&sys16_extraram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_READ(SegaPCM_r)
	AM_RANGE(0xf800, 0xf807) AM_READ(sound2_shared_ram_r)
	AM_RANGE(0xf808, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( shangon_sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xf000, 0xf7ff) AM_WRITE(SegaPCM_w)
	AM_RANGE(0xf800, 0xf807) AM_WRITE(sound2_shared_ram_w) AM_BASE(&sound_shared_ram)
	AM_RANGE(0xf808, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

/***************************************************************************/

static void shangon_update_proc( void ){
	set_bg_page1( sys16_textram[0x74e] );
	set_fg_page1( sys16_textram[0x74f] );
	sys16_fg_scrollx = sys16_textram[0x7fc] & 0x01ff;
	sys16_bg_scrollx = sys16_textram[0x7fd] & 0x01ff;
	sys16_fg_scrolly = sys16_textram[0x792] & 0x00ff;
	sys16_bg_scrolly = sys16_textram[0x793] & 0x01ff;
}

static MACHINE_INIT( shangon ){
	sys16_textmode=1;
	sys16_spritesystem = sys16_sprite_hangon;
	sys16_sprxoffset = -0xc0;
	sys16_fgxoffset = 8;
	sys16_textlayer_lo_min=0;
	sys16_textlayer_lo_max=0;
	sys16_textlayer_hi_min=0;
	sys16_textlayer_hi_max=0xff;

	sys16_patch_code( 0x65bd, 0xf9);
	sys16_patch_code( 0x6677, 0xfa);
	sys16_patch_code( 0x66d5, 0xfb);
	sys16_patch_code( 0x9621, 0xfb);

	sys16_update_proc = shangon_update_proc;

	sys16_gr_ver = shared_ram;
	sys16_gr_hor = sys16_gr_ver+0x200/2;
	sys16_gr_pal = sys16_gr_ver+0x400/2;
	sys16_gr_flip= sys16_gr_ver+0x600/2;

	sys16_gr_palette= 0xf80 / 2;
	sys16_gr_palette_default = 0x70 /2;
	sys16_gr_colorflip[0][0]=0x08 / 2;
	sys16_gr_colorflip[0][1]=0x04 / 2;
	sys16_gr_colorflip[0][2]=0x00 / 2;
	sys16_gr_colorflip[0][3]=0x06 / 2;
	sys16_gr_colorflip[1][0]=0x0a / 2;
	sys16_gr_colorflip[1][1]=0x04 / 2;
	sys16_gr_colorflip[1][2]=0x02 / 2;
	sys16_gr_colorflip[1][3]=0x02 / 2;
}

static DRIVER_INIT( shangon ){
	machine_init_sys16_onetime();
	generate_gr_screen(512,1024,0,0,4,0x8000);

	sys16_patch_z80code( 0x1087, 0x20);
	sys16_patch_z80code( 0x1088, 0x01);
}

static DRIVER_INIT( shangonb ){
	machine_init_sys16_onetime();
	generate_gr_screen(512,1024,8,0,4,0x8000);
}
/***************************************************************************/

INPUT_PORTS_START( shangon )
PORT_START	/* Steering */
	PORT_ANALOG( 0xff, 0x7f, IPT_AD_STICK_X | IPF_REVERSE | IPF_CENTER , 100, 3, 0x42, 0xbd )

#ifdef HANGON_DIGITAL_CONTROLS

PORT_START	/* Buttons */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )

#else

PORT_START	/* Accel / Decel */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif

PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	SYS16_COINAGE

PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, "Easy" )
	PORT_DIPSETTING(    0x06, "Normal" )
	PORT_DIPSETTING(    0x02, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x18, 0x18, "Time Adj." )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x18, "Normal" )
	PORT_DIPSETTING(    0x08, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x20, 0x20, "Play Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )


#ifndef HANGON_DIGITAL_CONTROLS

PORT_START	/* Brake */
	PORT_ANALOG( 0xff, 0x1, IPT_AD_STICK_Y | IPF_PLAYER2 | IPF_CENTER | IPF_REVERSE, 100, 16, 1, 0xa2 )

#endif
INPUT_PORTS_END

/***************************************************************************/

static MACHINE_DRIVER_START( shangon )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(shangon_readmem,shangon_writemem)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
	MDRV_CPU_PROGRAM_MAP(shangon_sound_readmem,shangon_sound_writemem)
	MDRV_CPU_IO_MAP(sound_readport,sound_writeport)

	MDRV_CPU_ADD(M68000, 10000000)
	MDRV_CPU_PROGRAM_MAP(shangon_readmem2,shangon_writemem2)
	MDRV_CPU_VBLANK_INT(sys16_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(100)

	MDRV_MACHINE_INIT(shangon)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(40*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(sys16_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(2048*ShadowColorsMultiplier)

	MDRV_VIDEO_START(hangon)
	MDRV_VIDEO_UPDATE(hangon)

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(YM2151, sys16_ym2151_interface)
	MDRV_SOUND_ADD(SEGAPCM, sys16_segapcm_interface_15k_512)
MACHINE_DRIVER_END

GAMEX(1992, shangon,  0,        shangon,  shangon,  shangon,  ROT0,         "Sega",    "Super Hang-On", GAME_NOT_WORKING )
GAMEX(1991, shangona, shangon,  shangon,  shangon,  shangon,  ROT0,         "Sega",    "Super Hang-On (alt)", GAME_NOT_WORKING )
GAMEX(1991, shangnle, shangon,  shangon,  shangon,  shangon,  ROT0,         "Sega",    "Super Hang-On Limited Edition", GAME_NOT_WORKING )
GAME( 1992, shangonb, shangon,  shangon,  shangon,  shangonb, ROT0,         "bootleg", "Super Hang-On (bootleg)" )

GAME( 1986, outrun,   0,        outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 1)" )
GAME( 1986, outruna,  outrun,   outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (set 2)" )
GAME( 1986, outrunb,  outrun,   outrun,   outrun,   outrunb,  ROT0,         "bootleg", "Out Run (set 3)" )
GAME( 1986, outrundx, outrun,   outrun,   outrun,   outrun,   ROT0,         "Sega",    "Out Run (Deluxe?)" )
GAMEX(1989, toutrun,  0,        outrun,   outrun,   outrun,   ROT0,         "Sega",	   "Turbo Out Run (set 1)", GAME_NOT_WORKING )
GAMEX(1989, toutruna, toutrun,  outrun,   outrun,   outrun,   ROT0,         "Sega",    "Turbo Out Run (set 2)", GAME_NOT_WORKING )
