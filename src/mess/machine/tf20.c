/***************************************************************************

    Epson TF-20

    Dual floppy drive with HX-20 factory option

    Skeleton driver, not working

***************************************************************************/

#include "driver.h"
#include "tf20.h"
#include "cpu/z80/z80.h"
#include "devices/messram.h"
#include "machine/upd7201.h"
#include "machine/nec765.h"
#include "devices/flopdrv.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define XTAL_CR1	XTAL_8MHz
#define XTAL_CR2	XTAL_4_9152MHz


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _tf20_state tf20_state;
struct _tf20_state
{
	const device_config *ram;
	const device_config *upd765a;
	const device_config *upd7201;
};


/*****************************************************************************
    INLINE FUNCTIONS
*****************************************************************************/

INLINE tf20_state *get_safe_token(const device_config *device)
{
	assert(device != NULL);
	assert(device->token != NULL);
	assert(device->type == TF20);

	return (tf20_state *)device->token;
}


/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/* a read from this location disables the rom */
static READ8_HANDLER( tf20_rom_disable )
{
	tf20_state *tf20 = get_safe_token(space->cpu->owner);
	const address_space *prg = cpu_get_address_space(space->cpu, ADDRESS_SPACE_PROGRAM);

	memory_install_readwrite8_handler(prg, 0x0000, 0x7fff, 0, 0, SMH_BANK(21), SMH_BANK(21));
	memory_set_bankptr(space->machine, 21, messram_get_ptr(tf20->ram));

	return 0xff;
}

static READ8_HANDLER( tf20_dip_r )
{
	logerror("%s: tf20_dip_r\n", cpuexec_describe_context(space->machine));

	return 0xff;
}

static WRITE8_HANDLER( tf20_fdc_control_w )
{
	logerror("%s: tf20_fdc_control_w %02x\n", cpuexec_describe_context(space->machine), data);

	/* bit 0, motor on signal */
}

/* serial output signal (to the host computer) */
READ_LINE_DEVICE_HANDLER( tf20_rxs_r )
{
	logerror("%s: tf20_rxs_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

READ_LINE_DEVICE_HANDLER( tf20_pins_r )
{
	logerror("%s: tf20_pins_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

/* serial input signal (from host computer) */
WRITE_LINE_DEVICE_HANDLER( tf20_txs_w )
{
	logerror("%s: tf20_rxd1_w %u\n", cpuexec_describe_context(device->machine), state);
}

WRITE_LINE_DEVICE_HANDLER( tf20_pouts_w )
{
	logerror("%s: tf20_pouts_w %u\n", cpuexec_describe_context(device->machine), state);
}

/* serial output signal (to another terminal) */
READ_LINE_DEVICE_HANDLER( tf20_txc_r )
{
	logerror("%s: tf20_txc_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

/* serial input signal (from another terminal) */
WRITE_LINE_DEVICE_HANDLER( tf20_rxc_w )
{
	logerror("%s: tf20_rxc_w %u\n", cpuexec_describe_context(device->machine), state);
}

READ_LINE_DEVICE_HANDLER( tf20_poutc_r )
{
	logerror("%s: tf20_poutc_r\n", cpuexec_describe_context(device->machine));

	return 0;
}

WRITE_LINE_DEVICE_HANDLER( tf20_pinc_w )
{
	logerror("%s: tf20_pinc_w %u\n", cpuexec_describe_context(device->machine), state);
}


/*****************************************************************************
    ADDRESS MAPS
*****************************************************************************/

static ADDRESS_MAP_START( tf20_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(21)
	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(22)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tf20_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0xf0, 0xf3) AM_DEVREADWRITE("3a", upd7201_ba_cd_r, upd7201_ba_cd_w)
	AM_RANGE(0xf6, 0xf6) AM_READ(tf20_rom_disable)
	AM_RANGE(0xf7, 0xf7) AM_READ(tf20_dip_r)
	AM_RANGE(0xf8, 0xf8) AM_WRITE(tf20_fdc_control_w)
	AM_RANGE(0xfa, 0xfa) AM_DEVREAD("5a", nec765_status_r)
	AM_RANGE(0xfb, 0xfb) AM_DEVREADWRITE("5a", nec765_data_r, nec765_data_w)
ADDRESS_MAP_END


/*****************************************************************************
    MACHINE CONFIG
*****************************************************************************/

static UPD7201_INTERFACE( tf20_upd7201_intf )
{
	DEVCB_NULL,				/* interrupt: nc */
	{
		{
			XTAL_CR2 / 128,		/* receive clock: 38400 baud (default) */
			XTAL_CR2 / 128,		/* transmit clock: 38400 baud (default) */
			DEVCB_NULL,			/* receive DRQ */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_NULL,			/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output: nc */
		}, {
			XTAL_CR2 / 128,		/* receive clock: 38400 baud (default) */
			XTAL_CR2 / 128,		/* transmit clock: 38400 baud (default) */
			DEVCB_NULL,			/* receive DRQ: nc */
			DEVCB_NULL,			/* transmit DRQ */
			DEVCB_NULL,			/* receive data */
			DEVCB_NULL,			/* transmit data */
			DEVCB_LINE_GND,		/* clear to send */
			DEVCB_LINE_GND,		/* data carrier detect */
			DEVCB_NULL,			/* ready to send */
			DEVCB_NULL,			/* data terminal ready: nc */
			DEVCB_NULL,			/* wait */
			DEVCB_NULL			/* sync output: nc */
		}
	}
};

static const nec765_interface tf20_nec765a_intf =
{
	DEVCB_CPU_INPUT_LINE("tf20", INPUT_LINE_IRQ0),
	NULL,
	NULL,
	NEC765_RDY_PIN_CONNECTED,
	{FLOPPY_0, FLOPPY_1, NULL, NULL}
};

static const floppy_config tf20_floppy_config =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_DRIVE_DS_80,
	FLOPPY_OPTIONS_NAME(default),
	DO_NOT_KEEP_GEOMETRY
};

static MACHINE_DRIVER_START( tf20 )
	MDRV_CPU_ADD("tf20", Z80, XTAL_CR1 / 2) /* uPD780C */
	MDRV_CPU_PROGRAM_MAP(tf20_mem)
	MDRV_CPU_IO_MAP(tf20_io)

	/* 64k internal ram */
	MDRV_RAM_ADD("ram")
	MDRV_RAM_DEFAULT_SIZE("64k")

	/* upd765a floppy controller */
	MDRV_NEC765A_ADD("5a", tf20_nec765a_intf)

	/* upd7201 serial interface */
	MDRV_UPD7201_ADD("3a", XTAL_CR1 / 2, tf20_upd7201_intf)

	/* 2 floppy drives */
	MDRV_FLOPPY_2_DRIVES_ADD(tf20_floppy_config)
MACHINE_DRIVER_END


/***************************************************************************
    ROM DEFINITIONS
***************************************************************************/

ROM_START( tf20 )
	ROM_REGION(0x0800, "tf20", ROMREGION_LOADBYNAME)
	ROM_LOAD("tfx.15e", 0x0000, 0x0800, CRC(af34f084) SHA1(c9bdf393f757ba5d8f838108ceb2b079be1d616e))
ROM_END


/*****************************************************************************
    DEVICE INTERFACE
*****************************************************************************/

static DEVICE_START( tf20 )
{
	tf20_state *tf20 = get_safe_token(device);

	/* locate child devices */
	tf20->ram = device_find_child_by_tag(device, "ram");
	tf20->upd765a = device_find_child_by_tag(device, "5a");
	tf20->upd7201 = device_find_child_by_tag(device, "3a");

	/* enable second half of ram */
	memory_set_bankptr(device->machine, 22, messram_get_ptr(tf20->ram) + 0x8000);
}

static DEVICE_RESET( tf20 )
{
	tf20_state *tf20 = get_safe_token(device);

	const device_config *cpu = device_find_child_by_tag(device, "tf20");
	const address_space *prg = cpu_get_address_space(cpu, ADDRESS_SPACE_PROGRAM);

	/* enable rom */
	memory_install_readwrite8_handler(prg, 0x0000, 0x07ff, 0, 0x7800, SMH_BANK(21), SMH_NOP);
	memory_set_bankptr(device->machine, 21, cpu->region);
}

DEVICE_GET_INFO( tf20 )
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_TOKEN_BYTES:			info->i = sizeof(tf20_state);					break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:	info->i = 0;									break;
		case DEVINFO_INT_CLASS:					info->i = DEVICE_CLASS_OTHER;					break;

		/* --- the following bits of info are returned as pointers --- */
		case DEVINFO_PTR_MACHINE_CONFIG:		info->machine_config = MACHINE_DRIVER_NAME(tf20);	break;
		case DEVINFO_PTR_ROM_REGION:			info->romregion = ROM_NAME(tf20); 				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_FCT_START:					info->start = DEVICE_START_NAME(tf20);			break;
		case DEVINFO_FCT_STOP:					/* Nothing */									break;
		case DEVINFO_FCT_RESET:					info->reset = DEVICE_RESET_NAME(tf20);			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME:					strcpy(info->s, "TF-20");						break;
		case DEVINFO_STR_FAMILY:				strcpy(info->s, "Floppy drive");				break;
		case DEVINFO_STR_VERSION:				strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:			strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:				strcpy(info->s, "Copyright MESS Team");			break;
	}
}
