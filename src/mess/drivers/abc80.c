/*
	abc80.c

	MESS Driver by Curt Coder

	Luxor ABC 80
	------------
	(c) 1978 Luxor Datorer AB, Sweden

	CPU:			Z80 @ 3 MHz
	ROM:			32 KB
	RAM:			16 KB, 1 KB frame buffer
	CRTC:			74S263/74S262 (UK)
	Resolution:		240x240
	Colors:			2

	http://www.devili.iki.fi/Computers/Luxor/
	http://hem.passagen.se/mani/abc/
*/

/*

	TODO:

	- hook up 1.5MHZ to CTC triggers 0-2
	- use MAME CRTC6845 implementation
	- connect CTC to DART/SIO
	- port mirroring
	- COM port DIP switch

*/

#include "driver.h"
#include "video/generic.h"
#include "includes/centroni.h"
#include "devices/basicdsk.h"
#include "devices/cassette.h"
#include "devices/printer.h"
#include "machine/z80pio.h"
#include "sound/sn76477.h"

#define X01 12000000.0

static tilemap *bg_tilemap;

/* vidhrdw */

static WRITE8_HANDLER( abc80_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}

static TILE_GET_INFO(abc80_get_tile_info)
{
	int attr = videoram[tile_index];
	int bank = 0;	// TODO: bank 1 is graphics mode, add a [40][25] array to support it, also to videoram_w
	int code = attr & 0x7f;
	int color = (attr & 0x80) ? 1 : 0;

	SET_TILE_INFO(bank, code, color, 0)
}

static UINT32 abc80_tilemap_scan( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	/* logical (col,row) -> memory offset */
	return ((row & 0x07) << 7) + (row >> 3) * num_cols + col;
}

VIDEO_START( abc80 )
{
	bg_tilemap = tilemap_create(abc80_get_tile_info, abc80_tilemap_scan, 
		TILEMAP_OPAQUE, 6, 10, 40, 24);
}

VIDEO_UPDATE( abc80 )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	return 0;
}

/* Read/Write Handlers */

/* CTC */

static READ8_HANDLER( io_reset_r )
{
	// reset i/o devices
	return 0;
}

/* ABC 80 - Sound */

/*
  Bit Name     Description
   0  SYSENA   1 On, 0 Off (inverted)
   1  EXTVCO   00 High freq, 01 Low freq
   2  VCOSEL   10 SLF cntrl, 11 SLF ctrl
   3  MIXSELB  000 VCO, 001 Noise, 010 SLF
   4  MIXSELA  011 VCO+Noise, 100 SLF+Noise, 101 SLF+VCO
   5  MIXSELC  110 SLF+VCO+Noise, 111 Quiet
   6  ENVSEL2  00 VCO, 01 Rakt igenom
   7  ENVSEL1  10 Monovippa, 11 VCO alt.pol.
*/

static void abc80_sound_w(UINT8 data)
{
	// TODO: beep sound won't stop after it starts

	SN76477_enable_w(0, ~data & 0x01);

	SN76477_vco_voltage_w(0, (data & 0x02) ? 2.5 : 0);
	SN76477_vco_w(0, (data & 0x04) ? 1 : 0);

	SN76477_mixer_a_w(0, (data & 0x10) ? 1 : 0);
	SN76477_mixer_b_w(0, (data & 0x08) ? 1 : 0);
	SN76477_mixer_c_w(0, (data & 0x20) ? 1 : 0);

	SN76477_envelope_1_w(0, (data & 0x80) ? 1 : 0);
	SN76477_envelope_2_w(0, (data & 0x40) ? 1 : 0);
}

static READ8_HANDLER( keyboard_r )
{
	return 0;
}

/* ABC 80 - Keyboard */

static const UINT8 abc80_keycodes[7][8] = {
{0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38}, 
{0x39, 0x30, 0x2B, 0x60, 0x3C, 0x71, 0x77, 0x65}, 
{0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x7D}, 
{0x7E, 0x0D, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68}, 
{0x6A, 0x6B, 0x6C, 0x7C, 0x7B, 0x27, 0x08, 0x7A}, 
{0x78, 0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E}, 
{0x2D, 0x09, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}
};

static void abc80_keyboard_scan(void)
{
	UINT8 keycode = 0;
	UINT8 data;
	int irow, icol;

	for (irow = 0; irow < 7; irow++)
	{
		data = readinputport(irow);
		if (data != 0)
		{
			UINT8 ibit = 1;
			for (icol = 0; icol < 8; icol++)
			{
				if (data & ibit)
					keycode = abc80_keycodes[irow][icol];
				ibit <<= 1;
			}			
		}
	}
	
	/* shift, upper case */
//	if (readinputport(7) & 0x07)
	
	/* ctrl */
//	if (readinputport(7) & 0x08)
	
	if (keycode != 0)
		z80pio_d_w(0, 0, keycode | 0x80);
}

// PIO

static UINT8 abc80_pio_r(UINT16 offset)
{
	switch (offset)
	{
	case 0:
	{
		UINT8 data;
		data = z80pio_d_r(0, 0);
//		z80pio_d_w(0, 0, data & ~0x80);
		return data;
	}
	case 1:
		return z80pio_c_r(0, 0);
	case 2:
		return z80pio_d_r(0, 1);
	case 3:
		return z80pio_c_r(0, 1);
	}

	return 0xff;
}

static void abc80_pio_w(UINT16 offset, UINT8 data)
{
	switch (offset)
	{
	case 0:
		z80pio_d_w(0, 0, data);
		break;
	case 1:
		z80pio_c_w(0, 0, data);
		break;
	case 2:
		z80pio_d_w(0, 1, data);
		break;
	case 3:
		z80pio_c_w(0, 1, data);
		break;
	}
}

static READ8_HANDLER( abc80_io_r )
{
	if (offset & 0x10)
		return abc80_pio_r(offset & 0x03);

	switch (offset)
	{
	case 0x00: /* ABC Bus - Data in */
		break;
	case 0x01: /* ABC Bus - Status in */
		break;
	case 0x07: /* ABC Bus - Reset */
		break;
	}
	return 0xff;
}

static WRITE8_HANDLER( abc80_io_w )
{
	if (offset & 0x10)
	{
		abc80_pio_w(offset & 0x03, data);
		return;
	}

	switch (offset)
	{
	case 0x00: /* ABC Bus - Data out */
		break;
	case 0x01: /* ABC Bus - Channel select */
		break;
	case 0x02: /* ABC Bus - Command C1 */
		break;
	case 0x03: /* ABC Bus - Command C2 */
		break;
	case 0x04: /* ABC Bus - Command C3 */
		break;
	case 0x05: /* ABC Bus - Command C4 */
		break;
	case 0x06: /* SN76477 Sound */
		abc80_sound_w(data);
		break;
	}
}

/* Memory Maps */

static ADDRESS_MAP_START( abc80_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )
	AM_RANGE(0x0000, 0x3fff) AM_ROM // ROM Basic
//	AM_RANGE(0x6000, 0x6fff) AM_ROM // ROM Floppy
//	AM_RANGE(0x7800, 0x7bff) AM_ROM // ROM Printer
	AM_RANGE(0x7c00, 0x7fff) AM_RAM AM_WRITE(abc80_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x8000, 0xbfff) AM_RAM	// RAM Expanded
	AM_RANGE(0xc000, 0xffff) AM_RAM	// RAM Internal
ADDRESS_MAP_END

static ADDRESS_MAP_START( abc80_io_map, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_WRITE(abc80_io_w)
	AM_RANGE(0x00, 0xff) AM_READ(abc80_io_r)
/*
	AM_RANGE(0x06, 0x06) AM_WRITE(sound_w)
	AM_RANGE(0x07, 0x07) AM_READ(io_reset_r)
	AM_RANGE(0x38, 0x38) AM_READ(status_r)
	AM_RANGE(0x38, 0x3b) AM_WRITE(pio_w)
*/	
ADDRESS_MAP_END

/* Input Ports */

INPUT_PORTS_START( abc80 )
	PORT_START /* 0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("1 !") PORT_CODE(KEYCODE_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("2 \"") PORT_CODE(KEYCODE_2)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("3 #") PORT_CODE(KEYCODE_3)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("4 \xC2\xA4") PORT_CODE(KEYCODE_4)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("5 %") PORT_CODE(KEYCODE_5)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("6 &") PORT_CODE(KEYCODE_6)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("7 /") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("8 (") PORT_CODE(KEYCODE_8)

	PORT_START /* 1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("9 )") PORT_CODE(KEYCODE_9)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0 =") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("+ ?") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xC3\xA9 \xC3\x89") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("< >") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("q Q") PORT_CODE(KEYCODE_Q)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("w W") PORT_CODE(KEYCODE_W)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("e E") PORT_CODE(KEYCODE_E)

	PORT_START /* 2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("r R") PORT_CODE(KEYCODE_R)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("t T") PORT_CODE(KEYCODE_T)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("y Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("u U") PORT_CODE(KEYCODE_U)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("i I") PORT_CODE(KEYCODE_I)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("o O") PORT_CODE(KEYCODE_O)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("p P") PORT_CODE(KEYCODE_P)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xC3\xA5 \xC3\x85") PORT_CODE(KEYCODE_OPENBRACE)

	PORT_START /* 3 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xC3\xBC \xC3\x9C") PORT_CODE(KEYCODE_CLOSEBRACE)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("a A") PORT_CODE(KEYCODE_A)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("s S") PORT_CODE(KEYCODE_S)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("d D") PORT_CODE(KEYCODE_D)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("f F") PORT_CODE(KEYCODE_F)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("g G") PORT_CODE(KEYCODE_G)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("h H") PORT_CODE(KEYCODE_H)

	PORT_START /* 4 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("j J") PORT_CODE(KEYCODE_J)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("l L") PORT_CODE(KEYCODE_L)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xC3\xB6 \xC3\x96") PORT_CODE(KEYCODE_COLON)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("\xC3\xA4 \xC3\x84") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("' *") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("z Z") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("x X") PORT_CODE(KEYCODE_X)

	PORT_START /* 5 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("c C") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("v V") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("b B") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("n N") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("m M") PORT_CODE(KEYCODE_M)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(", ;") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(". :") PORT_CODE(KEYCODE_STOP)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("- _") PORT_CODE(KEYCODE_SLASH)

	PORT_START /* 6 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)

	PORT_START /* 7 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LEFT SHIFT") PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RIGHT SHIFT") PORT_CODE(KEYCODE_RSHIFT)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("UPPER CASE") PORT_CODE(KEYCODE_CAPSLOCK)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED)   
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED)   
INPUT_PORTS_END

/* Graphics Layouts */

static gfx_layout charlayout_abc80 =
{
	6, 10,
	128,
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8
};

/* Graphics Decode Info */

static gfx_decode gfxdecodeinfo_abc80[] =
{
	{ REGION_GFX1, 0,     &charlayout_abc80, 0, 2 },	// normal characters
	{ REGION_GFX1, 0x500, &charlayout_abc80, 0, 2 },	// graphics characters
	{ -1 }
};

/* Z80 Interfaces */

/*
	PIO Channel A

	0  R	Keyboard
	1  R
	2  R
	3  R
	4  R
	5  R
	6  R
	7  R	Strobe

	PIO Channel B
	
	0  R	RS-232C RxD
	1  R	RS-232C _CTS
	2  R	RS-232C _DCD
	3  W	RS-232C TxD
	4  W	RS-232C _RTS
	5  W	Cassette Memory Control Relay
	6  W	Cassette Memory 1
	7  R	Cassette Memory 5
*/

/* Sound Interfaces */

static struct SN76477interface sn76477_interface =
{
	RES_K(47),		//  4  noise_res		R26 47k
	RES_K(330),		//  5  filter_res		R24 330k
	CAP_P(390),		//  6  filter_cap		C52 390p
	RES_K(47),		//  7  decay_res		R23 47k
	CAP_U(10),		//  8  attack_decay_cap		C50 10u/35V
	RES_K(2.2),		// 10  attack_res		R21 2.2k
	RES_K(33),		// 11  amplitude_res		R19 33k
	RES_K(10),		// 12  feedback_res		R18 10k
	0,			// 16  vco_voltage		0V or 2.5V
	CAP_N(10) ,		// 17  vco_cap			C48 10n
	RES_K(100),		// 18  vco_res			R20 100k
	0,			// 19  pitch_voltage		N/C
	RES_K(220),		// 20  slf_res			R22 220k
	CAP_U(1),		// 21  slf_cap			C51 1u/35V
	CAP_U(0.1),		// 23  oneshot_cap		C53 0.1u
	RES_K(330)		// 24  oneshot_res		R25 330k
};

/* Interrupt Generators */

INTERRUPT_GEN( abc80_nmi_interrupt )
{
	abc80_keyboard_scan();
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

/* Machine Initialization */

static void abc80_pio_interrupt( int state )
{
	cpunum_set_input_line(0, 0, (state ? HOLD_LINE : CLEAR_LINE));
}

static z80pio_interface abc80_pio_interface = 
{
	abc80_pio_interrupt,
	NULL,
	NULL
};

static MACHINE_START( abc80 )
{
	z80pio_init(0, &abc80_pio_interface);
}

/* Machine Drivers */

static MACHINE_DRIVER_START( abc80 )
	// basic machine hardware
	MDRV_CPU_ADD(Z80, 11980800/2/2)	// 2.9952 MHz
	MDRV_CPU_PROGRAM_MAP(abc80_map, 0)
	MDRV_CPU_IO_MAP(abc80_io_map, 0)

	MDRV_CPU_PERIODIC_INT(abc80_nmi_interrupt, 50)

	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_MACHINE_START(abc80)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(40*6, 24*10)
	MDRV_SCREEN_VISIBLE_AREA(0, 40*6-1, 0, 24*10-1)
	MDRV_GFXDECODE(gfxdecodeinfo_abc80)
	
	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)
	MDRV_VIDEO_START(abc80)
	MDRV_VIDEO_UPDATE(abc80)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477, 0)
	MDRV_SOUND_CONFIG(sn76477_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

/* Devices */

int device_load_abc80_floppy(mess_image *image)
{
	int size, tracks, heads, sectors;

	if (image_has_been_created(image))
		return INIT_FAIL;

	size = image_length (image);
	switch (size)
	{
	case 80*1024: /* Scandia Metric FD2 */
		tracks = 40;
		heads = 1;
		sectors = 8;
		break;
	case 160*1024: /* ABC 830 */
		tracks = 40;
		heads = 1;
		sectors = 16;
		break;
	case 640*1024: /* ABC 832/834 */
		tracks = 80;
		heads = 2;
		sectors = 16;
		break;
	case 1001*1024: /* ABC 838 */
		tracks = 77;
		heads = 2;
		sectors = 26;
		break;
	default:
		return INIT_FAIL;
	}

	if (device_load_basicdsk_floppy(image)==INIT_PASS)
	{
		/* sector id's 0-9 */
		/* drive, tracks, heads, sectors per track, sector length, dir_sector, dir_length, first sector id */
		basicdsk_set_geometry(image, tracks, heads, sectors, 256, 0, 0, FALSE);
		return INIT_PASS;
	}

	return INIT_FAIL;
}

/* ROMs */

ROM_START( abc80 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "za3508.a2", 0x0000, 0x1000, CRC(e2afbf48) SHA1(9883396edd334835a844dcaa792d29599a8c67b9) )
	ROM_LOAD( "za3509.a3", 0x1000, 0x1000, CRC(d224412a) SHA1(30968054bba7c2aecb4d54864b75a446c1b8fdb1) )
	ROM_LOAD( "za3506.a4", 0x2000, 0x1000, CRC(1502ba5b) SHA1(5df45909c2c4296e5701c6c99dfaa9b10b3a729b) )
	ROM_LOAD( "za3507.a5", 0x3000, 0x1000, CRC(bc8860b7) SHA1(28b6cf7f5a4f81e017c2af091c3719657f981710) )

	ROM_REGION( 0x0a00, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "chargen", 0x0000, 0x0a00, BAD_DUMP CRC(9e064e91) SHA1(354783c8f2865f73dc55918c9810c66f3aca751f) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "abc8011.k5", 0x0000, 0x0080, NO_DUMP ) // 82S129 256x4 horizontal sync
	ROM_LOAD( "abc8012.j3", 0x0080, 0x0080, NO_DUMP ) // 82S129 256x4 chargen 74S263 column address
	ROM_LOAD( "abc8013.e7", 0x0100, 0x0080, NO_DUMP ) // 82S129 256x4 address decoder
	ROM_LOAD( "abc8021.k2", 0x0180, 0x0100, NO_DUMP ) // 82S131 512x4 vertical sync, videoram
	ROM_LOAD( "abc8022.k1", 0x0280, 0x0100, NO_DUMP ) // 82S131 512x4 chargen 74S263 row address
ROM_END

/* System Configuration */

static void abc80_printer_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* printer */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		default:										printer_device_getinfo(devclass, state, info); break;
	}
}

#if 0
static void abc80_cassette_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	// cassette
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_CASSETTE_FORMATS:				info->p = (void *) abc80_cassette_formats; break;

		default:										cassette_device_getinfo(devclass, state, info); break;
	}
}
#endif

static void abc80_floppy_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_abc80_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "dsk"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START( abc80 )
	CONFIG_RAM_DEFAULT	( 16 * 1024)
	CONFIG_RAM		( 32 * 1024)
	CONFIG_DEVICE(abc80_printer_getinfo)
//	CONFIG_DEVICE(abc80_cassette_getinfo)
	CONFIG_DEVICE(abc80_floppy_getinfo)
SYSTEM_CONFIG_END

/* Drivers */

/*    YEAR	NAME  PARENT  COMPAT MACHINE  INPUT	INIT CONFIG	 COMPANY             FULLNAME */
COMP( 1978, abc80,   0,       0, abc80,   abc80,  0, abc80,  "Luxor Datorer AB", "ABC 80", GAME_NOT_WORKING )
