/***************************************************************************

        Sanyo PHC-25

        Skeleton driver.

****************************************************************************/

/*

	http://www.geocities.jp/sanyo_phc_25/

	Z80 @ 4 MHz
	MC6847 video
	3x 8KB bios ROM
	1x 4KB chargen ROM
	16KB RAM
	6KB video RAM

*/

/*

	TODO:

	- RAM should be 16KB
	- MC6847 mode selection lines
	- keyboard
	- cassette

*/

#include "emu.h"
#include "includes/phc25.h"
#include "cpu/z80/z80.h"
#include "devices/cassette.h"
#include "devices/messram.h"
#include "machine/ctronics.h"
#include "video/m6847.h"
#include "sound/ay8910.h"

/* Read/Write Handlers */

static READ8_HANDLER( port40_r )
{
	/*

		bit		description

		0		
		1		
		2		
		3		
		4		vertical sync
		5		cassette read
		6		centronics busy
		7		horizontal sync

	*/

	phc25_state *state = (phc25_state *)space->machine->driver_data;

	UINT8 data = 0;

	/* vertical sync */
	data |= mc6847_fs_r(state->mc6847) << 4;

	/* cassette read */
	data |= (cassette_input(state->cassette) < +0.0) << 5;

	/* centronics busy */
	data |= centronics_busy_r(state->centronics) << 6;

	/* horizontal sync */
	data |= mc6847_hs_r(state->mc6847) << 7;

	return data;
}

static WRITE8_HANDLER( port40_w )
{
	/*

		bit		description

		0		cassette output
		1		cassette motor
		2		MC6847 INT/EXT
		3		centronics strobe
		4		
		5		MC6847 
		6		MC6847 
		7		MC6847 A/G

	*/

	phc25_state *state = (phc25_state *)space->machine->driver_data;

	/* cassette output */
	cassette_output(state->cassette, BIT(data, 0) ? -1.0 : +1.0);

	/* internal/external character generator */
	mc6847_intext_w(state->mc6847, BIT(data, 2));

	/* cassette motor */
	cassette_change_state(state->cassette, CASSETTE_MASK_MOTOR, BIT(data, 1) ? CASSETTE_MOTOR_ENABLED : CASSETTE_MOTOR_DISABLED);

	/* centronics strobe */
	centronics_strobe_w(state->centronics, BIT(data, 3));

	/* alphanumerics/graphics */
	mc6847_ag_w(state->mc6847, BIT(data, 7));
}

/* Memory Maps */

static ADDRESS_MAP_START( phc25_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x5fff) AM_ROM AM_REGION(Z80_TAG, 0)
	AM_RANGE(0x6000, 0x77ff) AM_RAM AM_BASE_MEMBER(phc25_state, video_ram)
	AM_RANGE(0x7800, 0x7fff) AM_RAM
	AM_RANGE(0x8000, 0xbfff) AM_RAM
	AM_RANGE(0xe000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( phc25_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x00, 0x00) AM_DEVWRITE(CENTRONICS_TAG, centronics_data_w)
	AM_RANGE(0x40, 0x40) AM_READWRITE(port40_r, port40_w)
	AM_RANGE(0x80, 0x80) AM_READ_PORT("KEY0")
	AM_RANGE(0x81, 0x81) AM_READ_PORT("KEY1")
	AM_RANGE(0x82, 0x82) AM_READ_PORT("KEY2")
	AM_RANGE(0x83, 0x83) AM_READ_PORT("KEY3")
	AM_RANGE(0x84, 0x84) AM_READ_PORT("KEY4")
	AM_RANGE(0x85, 0x85) AM_READ_PORT("KEY5")
	AM_RANGE(0x86, 0x86) AM_READ_PORT("KEY6")
	AM_RANGE(0x87, 0x87) AM_READ_PORT("KEY7")
	AM_RANGE(0x88, 0x88) AM_READ_PORT("KEY8")
	AM_RANGE(0xc0, 0xc0) AM_DEVWRITE(AY8910_TAG, ay8910_address_w)
	AM_RANGE(0xc1, 0xc1) AM_DEVREADWRITE(AY8910_TAG, ay8910_r, ay8910_data_w)
ADDRESS_MAP_END

/* Input Ports */
/*static const int KeyTable[] =
{
0	0xE2, 0xBA, VK_DELETE, VK_UP,    'X',        'S',      'W',   '1',
1	0xBF, 0xBB, VK_RETURN, VK_DOWN,  'Z',        'A',      'Q',   VK_ESCAPE,
2	0,    0xDB, 0xDE,      VK_LEFT,  'V',        'F',      'R',   '3',
3	' ',  0xDD, 0xDC,      VK_RIGHT, 'C',        'D',      'E',   '2',
4	0,    'P',  '0',       VK_F3,    'N',        'H',      'Y',   '5',
5	0,    0xC0, 0xBD,      VK_F4,    'B',        'G',      'T',   '4',
6	0,    'O',  '9',       VK_F2,    'M',        'J',      'U',   '6',
7	0xBE, 'L',  '8',       VK_F1,    0xBC,       'K',      'I',   '7',
8	0,    0,    VK_F5,     0,        VK_CONTROL, VK_SHIFT, VK_F6, 0
};
*/
INPUT_PORTS_START( phc25 )
	PORT_START("KEY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x91") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))	
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("INS DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("KEY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x93") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("KEY2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x90") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("\xE2\x86\x92") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')

	PORT_START("KEY4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F3") PORT_CODE(KEYCODE_F3) PORT_CHAR(UCHAR_MAMEKEY(F3))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F4") PORT_CODE(KEYCODE_F4) PORT_CHAR(UCHAR_MAMEKEY(F4))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F2") PORT_CODE(KEYCODE_F2) PORT_CHAR(UCHAR_MAMEKEY(F2))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("KEY7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('\'')
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) PORT_CHAR(UCHAR_MAMEKEY(F1))
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD )

	PORT_START("KEY8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL))
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

/* Video */

static READ8_DEVICE_HANDLER( phc25_video_ram_r )
{
	phc25_state *state = (phc25_state *)device->machine->driver_data;

	return state->video_ram[offset & 0x17ff];
}

static WRITE_LINE_DEVICE_HANDLER( phc25_fs_w )
{
	cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_IRQ0, state ? CLEAR_LINE : ASSERT_LINE);
}

static UINT8 phc25_char_rom_r(running_machine *machine, UINT8 ch, int line)
{
	phc25_state *state = (phc25_state *)machine->driver_data;

	return state->char_rom[((ch-2) * 12) + line + 4];
}

static const mc6847_interface mc6847_intf =
{
	DEVCB_HANDLER(phc25_video_ram_r),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_LINE(phc25_fs_w),
	DEVCB_NULL,
	DEVCB_NULL
};

static VIDEO_START( pal )
{
	phc25_state *state = (phc25_state *)machine->driver_data;

	/* find devices */
	state->mc6847 = devtag_get_device(machine, MC6847_TAG);

	/* find memory regions */
	state->char_rom = memory_region(machine, Z80_TAG) + 0x5000;
}

static VIDEO_START( ntsc )
{
	phc25_state *state = (phc25_state *)machine->driver_data;

	/* find devices */
	state->mc6847 = devtag_get_device(machine, MC6847_TAG);

	/* find memory regions */
	state->char_rom = memory_region(machine, "chargen");
}

static VIDEO_UPDATE( phc25 )
{
	phc25_state *state = (phc25_state *)screen->machine->driver_data;

	return mc6847_update(state->mc6847, bitmap, cliprect);
}

/* AY-3-8910 Interface */

static const ay8910_interface ay8910_intf =
{
	AY8910_LEGACY_OUTPUT,
	AY8910_DEFAULT_LOADS,
	DEVCB_INPUT_PORT("JOY0"),
	DEVCB_INPUT_PORT("JOY1"),
	DEVCB_NULL,
	DEVCB_NULL
};

/* Cassette Configuration */

static const cassette_config phc25_cassette_config =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED)
};

/* Machine Initialization */

static MACHINE_START( phc25 )
{
	phc25_state *state = (phc25_state *)machine->driver_data;

	/* find devices */
	state->centronics = devtag_get_device(machine, CENTRONICS_TAG);
	state->cassette = devtag_get_device(machine, CASSETTE_TAG);

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static MACHINE_DRIVER_START( phc25 )
	MDRV_DRIVER_DATA(phc25_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, 4000000)
    MDRV_CPU_PROGRAM_MAP(phc25_mem)
    MDRV_CPU_IO_MAP(phc25_io)

    MDRV_MACHINE_START(phc25)

    /* video hardware */
    MDRV_VIDEO_UPDATE(phc25)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910_TAG, AY8910, 1996750)
	MDRV_SOUND_CONFIG(ay8910_intf)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)

	/* devices */
	MDRV_CASSETTE_ADD(CASSETTE_TAG, phc25_cassette_config)
	MDRV_CENTRONICS_ADD(CENTRONICS_TAG, standard_centronics)
	
	/* internal ram */
	MDRV_RAM_ADD("messram")
	MDRV_RAM_DEFAULT_SIZE("16K")
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pal )
	MDRV_IMPORT_FROM(phc25)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(M6847_PAL_FRAMES_PER_SECOND)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)
    MDRV_PALETTE_LENGTH(16)

	MDRV_MC6847_ADD(MC6847_TAG, mc6847_intf)
    MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_PAL)
	MDRV_MC6847_CHAR_ROM(phc25_char_rom_r)

	MDRV_VIDEO_START(pal)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ntsc )
	MDRV_IMPORT_FROM(phc25)

    /* video hardware */
    MDRV_SCREEN_ADD(SCREEN_TAG, RASTER)
    MDRV_SCREEN_REFRESH_RATE(M6847_NTSC_FRAMES_PER_SECOND)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_RGB32)
	MDRV_SCREEN_SIZE(320, 25+192+26)
	MDRV_SCREEN_VISIBLE_AREA(0, 319, 1, 239)
    MDRV_PALETTE_LENGTH(16)

	MDRV_MC6847_ADD(MC6847_TAG, mc6847_intf)
    MDRV_MC6847_TYPE(M6847_VERSION_ORIGINAL_NTSC)
	MDRV_MC6847_CHAR_ROM(phc25_char_rom_r)

	MDRV_VIDEO_START(ntsc)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( phc25 )
    ROM_REGION( 0x6000, Z80_TAG, 0 )
	ROM_LOAD( "phc25rom.0", 0x0000, 0x2000, CRC(fa28336b) SHA1(582376bee455e124de24ba4ac02326c8a592fa5a) )
	ROM_LOAD( "phc25rom.1", 0x2000, 0x2000, CRC(38fd578b) SHA1(dc3db78c0cdc89f1605200d39535be65a4091705) )
	ROM_LOAD( "phc25rom.2", 0x4000, 0x2000, CRC(54392b27) SHA1(1587827fe9438780b50164727ce3fdea1b98078a) )
ROM_END

ROM_START( phc25j )
    ROM_REGION( 0x6000, Z80_TAG, 0 )
	ROM_LOAD( "022 00aa.ic", 0x0000, 0x2000, NO_DUMP )
	ROM_LOAD( "022 01aa.ic", 0x2000, 0x2000, NO_DUMP )
	ROM_LOAD( "022 02aa.ic", 0x4000, 0x2000, NO_DUMP )

    ROM_REGION( 0x1000, "chargen", 0 )
	ROM_LOAD( "022 04a.ic", 0x0000, 0x1000, NO_DUMP )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT  MACHINE INPUT   INIT    COMPANY     FULLNAME			FLAGS */
COMP( 1983, phc25,	0,		0,		pal,	phc25,	0,		"Sanyo",	"PHC-25 (Europe)",	GAME_NOT_WORKING )
COMP( 1983, phc25j,	phc25,	0,		ntsc,	phc25,	0,		"Sanyo",	"PHC-25 (Japan)",	GAME_NOT_WORKING )
