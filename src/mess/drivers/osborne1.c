/***************************************************************************

    Osborne-1 driver file

The Osborne-1 memory is divided into 3 "banks".

Bank 1 simply consists of 64KB of RAM. The upper 4KB is used for the lower 8
bit of video RAM entries.

Bank 2 holds the BIOS ROM and I/O area. Only addresses 0000-3FFF are used
by bank 2. Bank 2 is divided as follows:
3000-3FFF Unused
2C00-2C03 Video PIA
2A00-2A01 Serial interface
2900-2903 488 PIA
2201-2280 Keyboard
2100-2103 Floppy
1000-1FFF Unused
0000-0FFF BIOS ROM

Bank 3 has the ninth bit needed to complete the full Video RAM. These bits
are stored at F000-FFFF. Only the highest bit is used.

On bootup bank 2 is active.

The actual banking is done through I/O ports 00-03.
00 - Have both bank 2 and bank 1 active. This seems to be the power up default.
01 - Only have bank 1 active.
02 - Have both bank 2 and bank 3 active. (Not 100% sure, also bank 1 from 4000-EFFF?)
03 - Have both bank 2 and bank 1 active.

TODO:
  - Verify frequency of the beep/audio alarm.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "devices/basicdsk.h"
#include "cpu/z80/z80daisy.h"
#include "includes/osborne1.h"

#define MAIN_CLOCK	15974400

static ADDRESS_MAP_START( osborne1_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x0FFF ) AM_READWRITE( SMH_BANK1, osborne1_0000_w )
	AM_RANGE( 0x1000, 0x1FFF ) AM_READWRITE( SMH_BANK2, osborne1_1000_w )
	AM_RANGE( 0x2000, 0x2FFF ) AM_READWRITE( osborne1_2000_r, osborne1_2000_w )
	AM_RANGE( 0x3000, 0x3FFF ) AM_READWRITE( SMH_BANK3, osborne1_3000_w )
	AM_RANGE( 0x4000, 0xEFFF ) AM_RAM
	AM_RANGE( 0xF000, 0xFFFF ) AM_READWRITE( SMH_BANK4, osborne1_videoram_w )
ADDRESS_MAP_END

static ADDRESS_MAP_START( osborne1_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE( 0x00, 0x03 ) AM_WRITE( osborne1_bankswitch_w )
ADDRESS_MAP_END

static INPUT_PORTS_START( osborne1 )
	PORT_START	/* 0 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("|") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\'") PORT_CODE(KEYCODE_QUOTE)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("RETURN") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_RSHIFT) PORT_CODE(KEYCODE_LSHIFT)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CTRL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("TAB") PORT_CODE(KEYCODE_TAB)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC)
	PORT_START	/* 1 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_START	/* 2 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CURSOR LEFT") PORT_CODE(KEYCODE_LEFT)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CURSOR UP") PORT_CODE(KEYCODE_UP)
	PORT_START	/* 3 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)
	PORT_START	/* 4 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_START	/* 5 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
	PORT_START	/* 6 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("= +") PORT_CODE(KEYCODE_EQUALS)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_BACKSLASH)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_SLASH)
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CURSOR DOWN") PORT_CODE(KEYCODE_DOWN)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("CURSOR RIGHT") PORT_CODE(KEYCODE_RIGHT)
	PORT_START	/* 7 */
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_DIPNAME(0x08, 0, "ALPHA LOCK") PORT_CODE(KEYCODE_CAPSLOCK) PORT_TOGGLE
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

static PALETTE_INIT( osborne1 ) {
	palette_set_color_rgb( machine, 0, 0, 0, 0 );	/* Black */
	palette_set_color_rgb( machine, 1, 0, 255, 0 );	/* Full */
	palette_set_color_rgb( machine, 2, 0, 128, 0 );	/* Dimmed */
}

static MACHINE_DRIVER_START( osborne1 )
	MDRV_CPU_ADD_TAG( "main", Z80, MAIN_CLOCK/4 )
	MDRV_CPU_PROGRAM_MAP( osborne1_mem, 0 )
	MDRV_CPU_IO_MAP( osborne1_io, 0 )
	MDRV_CPU_CONFIG( osborne1_daisy_chain )

	MDRV_MACHINE_RESET( osborne1 )

	MDRV_SCREEN_ADD("main", RASTER)
	MDRV_SCREEN_FORMAT( BITMAP_FORMAT_INDEXED16 )
	MDRV_SCREEN_RAW_PARAMS( MAIN_CLOCK/2, 512, 0, 416, 260, 0, 240 )
	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )
	MDRV_PALETTE_LENGTH( 3 )
	MDRV_PALETTE_INIT( osborne1 )

	MDRV_SPEAKER_STANDARD_MONO( "mono" )
	MDRV_SOUND_ADD( BEEP, 0 )
	MDRV_SOUND_ROUTE( ALL_OUTPUTS, "mono", 1.00 )
MACHINE_DRIVER_END

ROM_START( osborne1 )
	ROM_REGION(0x1000, REGION_CPU1, 0)
	ROM_SYSTEM_BIOS( 0, "ver144", "BIOS version 1.44" )
	ROMX_LOAD( "osb144.bin", 0x0000, 0x1000, CRC(c0596b14) SHA1(ee6a9cc9be3ddc5949d3379351c1d58a175ce9ac), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "verA", "BIOS version A" )
	ROMX_LOAD( "osba.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "ver12", "BIOS version 1.2" )
	ROMX_LOAD( "osb12.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "ver121", "BIOS version 1.2.1" )
	ROMX_LOAD( "osb121.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "ver13", "BIOS version 1.3" )
	ROMX_LOAD( "osb13.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(5) )
	ROM_SYSTEM_BIOS( 5, "ver14", "BISO version 1.4" )
	ROMX_LOAD( "osb14.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(6) )
	ROM_SYSTEM_BIOS( 6, "ver143", "BIOS version 1.43" )
	ROMX_LOAD( "osb143.bin", 0x0000, 0x1000, NO_DUMP, ROM_BIOS(7) )
	ROM_REGION( 0x800, REGION_GFX1, 0 )
	ROM_LOAD( "osbchr.bin", 0x0000, 0x800, BAD_DUMP CRC(6c1eab0d) SHA1(b04459d377a70abc9155a5486003cb795342c801) )
ROM_END

static void osborne1_floppy_getinfo( const mess_device_class *devclass, UINT32 state, union devinfo *info ) {
	switch( state ) {
	case MESS_DEVINFO_INT_COUNT:				info->i = 2; break;
	case MESS_DEVINFO_PTR_LOAD:				info->load = device_load_osborne1_floppy; break;
	case MESS_DEVINFO_STR_FILE_EXTENSIONS:	strcpy( info->s = device_temp_str(), "img" ); break;
	default:							legacybasicdsk_device_getinfo( devclass, state, info ); break;
	}
}

SYSTEM_CONFIG_START( osborne1 )
	CONFIG_DEVICE( osborne1_floppy_getinfo )
	CONFIG_RAM_DEFAULT( 68 * 1024 )		/* 64KB Main RAM and 4Kbit video attribute RAM */
SYSTEM_CONFIG_END

/*    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG      COMPANY     FULLNAME        FLAGS */
COMP( 1981, osborne1,   0,      0,      osborne1,   osborne1,   osborne1,   osborne1,   "Osborne",  "Osborne-1",    0 )
