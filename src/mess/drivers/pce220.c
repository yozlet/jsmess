/***************************************************************************

    Sharp PC-E220

    preliminary driver by Angelo Salese
    improvements by Sandro Ronco

    Notes:
    - checks for a "machine language" string at some point, nothing in the
      current dump match it

    More info:
      http://wwwhomes.uni-bielefeld.de/achim/pc-e220.html
      http://www.akiyan.com/pc-g850_technical_data (in Japanese)

****************************************************************************/

#define ADDRESS_MAP_MODERN

#include "emu.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "sound/beep.h"
#include "machine/pce220_ser.h"
#include "rendlay.h"

// Interrupt flags
#define IRQ_FLAG_KEY		0x01
#define IRQ_FLAG_ON			0x02
#define IRQ_FLAG_TIMER		0x04

class pce220_state : public driver_device
{
public:
	pce220_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		  m_maincpu(*this, "maincpu"),
		  m_ram(*this, RAM_TAG),
		  m_beep(*this, BEEPER_TAG),
		  m_serial(*this, PCE220SERIAL_TAG)
		{ }

	required_device<cpu_device> m_maincpu;
	required_device<device_t> m_ram;
	required_device<device_t> m_beep;
	required_device<pce220_serial_device> m_serial;

	// HD61202 LCD controller
	UINT8 m_lcd_index_row;
	UINT8 m_lcd_index_col;
	UINT8 m_lcd_start_line;
	UINT8 m_lcd_on;

	//basic machine
	UINT8 m_bank_num;
	UINT8 m_irq_mask;
	UINT8 m_irq_flag;
	UINT8 m_timer_status;
	UINT16 m_kb_matrix;
	UINT8 *m_vram;

	UINT8 m_port15;
	UINT8 m_port18;

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	DECLARE_READ8_MEMBER( lcd_status_r );
	DECLARE_WRITE8_MEMBER( lcd_control_w );
	DECLARE_READ8_MEMBER( lcd_data_r );
	DECLARE_WRITE8_MEMBER( lcd_data_w );
	DECLARE_READ8_MEMBER( rom_bank_r );
	DECLARE_WRITE8_MEMBER( rom_bank_w );
	DECLARE_WRITE8_MEMBER( ram_bank_w );
	DECLARE_READ8_MEMBER( timer_r );
	DECLARE_WRITE8_MEMBER( timer_w );
	DECLARE_WRITE8_MEMBER( boot_bank_w );
	DECLARE_READ8_MEMBER( port15_r );
	DECLARE_WRITE8_MEMBER( port15_w );
	DECLARE_READ8_MEMBER( port18_r );
	DECLARE_WRITE8_MEMBER( port18_w );
	DECLARE_READ8_MEMBER( port1f_r );
	DECLARE_WRITE8_MEMBER( kb_matrix_w );
	DECLARE_READ8_MEMBER( kb_r );
	DECLARE_READ8_MEMBER( irq_status_r );
	DECLARE_WRITE8_MEMBER( irq_ack_w );
	DECLARE_WRITE8_MEMBER( irq_mask_w );
};

class pcg850v_state : public pce220_state
{
public:
	pcg850v_state(const machine_config &mconfig, device_type type, const char *tag)
		: pce220_state(mconfig, type, tag)
		{ }

	UINT8 m_g850v_bank_num;
	UINT8 m_lcd_effects;
	UINT8 m_lcd_contrast;
	UINT8 m_lcd_read_mode;

	virtual void machine_start();
	virtual void machine_reset();
	virtual bool screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect);
	DECLARE_READ8_MEMBER( g850v_bank_r );
	DECLARE_WRITE8_MEMBER( g850v_bank_w );
	DECLARE_READ8_MEMBER( g850v_lcd_status_r );
	DECLARE_WRITE8_MEMBER( g850v_lcd_control_w );
	DECLARE_READ8_MEMBER( g850v_lcd_data_r );
	DECLARE_WRITE8_MEMBER( g850v_lcd_data_w );
};

bool pce220_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	if (m_lcd_on)
	{
		for (int y = 0; y < 4; y++)
		{
			int row_pos = 0;

			for (int x = 0; x < 12; x++)
			{
				for (int xi = 0; xi < 5; xi++)
				{
					for (int yi = 0; yi < 8; yi++)
					{
						//first 12 columns
						int panel1_addr = ((m_lcd_start_line>>3) + y)*0x40 + row_pos;
						*BITMAP_ADDR16(&bitmap, y*8 + yi, x*6 + xi) = (m_vram[panel1_addr & 0x1ff] >> yi) & 1;

						//last 12 columns
						int panel2_addr = ((m_lcd_start_line>>3) + y + 4)*0x40 + (59-row_pos);
						*BITMAP_ADDR16(&bitmap, y*8 + yi, (x+12)*6 + xi) = (m_vram[panel2_addr & 0x1ff] >> yi) & 1;
					}

					row_pos++;
				}
			}
		}
	}
	else
	{
		bitmap_fill(&bitmap, &cliprect, 0);
	}
/*
    // TODO: LCD Symbols
    popmessage("%x %x %x %x",   m_vram[((m_lcd_start_line>>3)*0x40 + 0x03c) & 0x1ff],
                                m_vram[((m_lcd_start_line>>3)*0x40 + 0x0fc) & 0x1ff],
                                m_vram[((m_lcd_start_line>>3)*0x40 + 0x13c) & 0x1ff],
                                m_vram[((m_lcd_start_line>>3)*0x40 + 0x1fc) & 0x1ff]);
*/
    return 0;
}

bool pcg850v_state::screen_update(screen_device &screen, bitmap_t &bitmap, const rectangle &cliprect)
{
	int color0 = 0;
	int color1 = 1;

	if (m_lcd_on)
	{
		switch (m_lcd_effects)
		{
		case 0x01:		//mirror effect
			//TODO
			break;
		case 0x05:		//black mask effect
			color0 = color1 = 1;
			break;
		case 0x07:		//reverse effect
			color0 = 1;
			color1 = 0;
			break;
		case 0x08:		//contrast effect
			//TODO: use the max contrast value
			break;
		case 0x0e:		//white mask effect
			color0 = color1 = 0;
			break;
		}

		for (int y = 0; y < 6; y++)
		{
			int row_pos = 0;

			for (int x = 0; x < 24; x++)
			{
				for (int xi = 0; xi < 6; xi++)
				{
					for (int yi = 0; yi < 8; yi++)
					{
						int addr = ((m_lcd_start_line>>3) + y)*0x100 + row_pos;
						*BITMAP_ADDR16(&bitmap, y*8 + yi, x*6 + xi) = ((m_vram[addr & 0x7ff] >> yi) & 1 ) ? color1 : color0;
					}

					row_pos++;
				}
			}
		}
	}
	else
	{
		bitmap_fill(&bitmap, &cliprect, 0);
	}

    return 0;
}

READ8_MEMBER( pce220_state::lcd_status_r )
{
	/*
    x--- ---- Busy (not emulated)
    --x- ---- LCD on/off
    ---x ---- Reset
    */
	UINT8 data = 0;

	data &= (m_lcd_on<<5);

	return data;
}

WRITE8_MEMBER( pce220_state::lcd_control_w )
{
	if((data & 0xfe) == 0x3e)		//Display on/off
		m_lcd_on = data & 0x01;
	if((data & 0xb8) == 0xb8)		//Set page
		m_lcd_index_row = data & 0x07;
	if((data & 0xc0) == 0x40)		//Set address
		m_lcd_index_col = data & 0x3f;
	if((data & 0xc0) == 0xc0)		//Set display start line
		m_lcd_start_line = data & 0x3f;
}

READ8_MEMBER( pce220_state::lcd_data_r )
{
	return m_vram[(m_lcd_index_row*0x40 + m_lcd_index_col - 1) & 0x1ff];
}

WRITE8_MEMBER( pce220_state::lcd_data_w )
{
	m_vram[(m_lcd_index_row*0x40 + m_lcd_index_col) & 0x1ff] = data;

	m_lcd_index_col++;
}

READ8_MEMBER( pce220_state::rom_bank_r )
{
	return m_bank_num;
}

WRITE8_MEMBER( pce220_state::rom_bank_w )
{
	UINT8 bank4 = data & 0x07; // bits 0,1,2
	UINT8 bank3 = (data & 0x70) >> 4; // bits 4,5,6

	m_bank_num = data;

	memory_set_bank(machine(), "bank3", bank3);
	memory_set_bank(machine(), "bank4", bank4);
}

WRITE8_MEMBER( pce220_state::ram_bank_w )
{
	UINT8 bank = BIT(data,2);

	memory_set_bank(machine(), "bank1", bank);
	memory_set_bank(machine(), "bank2", bank);
}

READ8_MEMBER( pce220_state::timer_r )
{
	return m_timer_status;
}

WRITE8_MEMBER( pce220_state::timer_w )
{
	m_timer_status = data & 1;
}

WRITE8_MEMBER( pce220_state::boot_bank_w )
{
	// set to 1 after boot for restore the ram in the first bank
	if (data & 0x01)
	{
		address_space *space_prg = m_maincpu->memory().space(AS_PROGRAM);
		space_prg->install_write_bank(0x0000, 0x3fff, "bank1");
		memory_set_bank(machine(), "bank1", 0);
	}
}

READ8_MEMBER( pce220_state::port15_r )
{
	/*
    x--- ---- XIN input enabled
    ---- ---0
    */
	return m_port15;
}

WRITE8_MEMBER( pce220_state::port15_w )
{
	m_serial->enable_interface(BIT(data, 7));

	m_port15 = data;
}

READ8_MEMBER( pce220_state::port18_r )
{
	/*
    x--- ---- XOUT/TXD
    ---- --x- DOUT
    ---- ---x BUSY/CTS
    */

	return m_port18;
}

WRITE8_MEMBER( pce220_state::port18_w )
{
	beep_set_state(m_beep, BIT(data, 7));

	m_serial->out_busy(BIT(data, 0));
	m_serial->out_dout(BIT(data, 1));
	m_serial->out_xout(BIT(data, 7));

	m_port18 = data;
}

READ8_MEMBER( pce220_state::port1f_r )
{
	/*
    x--- ---- ON - resp. break key status (?)
    ---- -x-- XIN/RXD
    ---- --x- ACK/RTS
    ---- ---x DIN
    */

	UINT8 data = 0;

	data |= m_serial->in_din()<<0;
	data |= m_serial->in_ack()<<1;
	data |= m_serial->in_xin()<<2;

	data |= input_port_read(machine(), "ON")<<7;

	return data;
}

WRITE8_MEMBER( pce220_state::kb_matrix_w )
{
	switch(offset)
	{
	case 0:
		m_kb_matrix = (m_kb_matrix & 0x300) | data;
		break;
	case 1:
		m_kb_matrix = (m_kb_matrix & 0xff) | ((data&0x03)<<8);
		break;
	}
}

READ8_MEMBER( pce220_state::kb_r )
{
	UINT8 data = 0x00;

	if (m_kb_matrix & 0x01)
		data |= input_port_read(machine(), "LINE0");
	if (m_kb_matrix & 0x02)
		data |= input_port_read(machine(), "LINE1");
	if (m_kb_matrix & 0x04)
		data |= input_port_read(machine(), "LINE2");
	if (m_kb_matrix & 0x08)
		data |= input_port_read(machine(), "LINE3");
	if (m_kb_matrix & 0x10)
		data |= input_port_read(machine(), "LINE4");
	if (m_kb_matrix & 0x20)
		data |= input_port_read(machine(), "LINE5");
	if (m_kb_matrix & 0x40)
		data |= input_port_read(machine(), "LINE6");
	if (m_kb_matrix & 0x80)
		data |= input_port_read(machine(), "LINE7");
	if (m_kb_matrix & 0x100)
		data |= input_port_read(machine(), "LINE8");
	if (m_kb_matrix & 0x200)
		data |= input_port_read(machine(), "LINE9");

	return data;
}

READ8_MEMBER( pce220_state::irq_status_r )
{
	/*
    ---- -x-- timer
    ---- --x- ON-Key
    ---- ---x keyboard
    */
	return m_irq_flag;
}

WRITE8_MEMBER( pce220_state::irq_ack_w )
{
	m_irq_flag &= ~data;
}

WRITE8_MEMBER( pce220_state::irq_mask_w )
{
	m_irq_mask = data;
}

READ8_MEMBER( pcg850v_state::g850v_bank_r )
{
	return m_g850v_bank_num;
}

WRITE8_MEMBER( pcg850v_state::g850v_bank_w )
{
	address_space *space_prg = m_maincpu->memory().space(AS_PROGRAM);

	if (data < 0x16)
	{
		space_prg->install_read_bank(0xc000, 0xffff, "bank4");
		memory_set_bank(machine(), "bank4", data);
	}
	else
	{
		space_prg->unmap_read(0xc000, 0xffff);
	}

	m_g850v_bank_num = data;
}

READ8_MEMBER( pcg850v_state::g850v_lcd_status_r )
{
	/*
    x--- ---- Busy (not emulated)
    --x- ---- LCD on/off
    */
	UINT8 data = 0;

	data &= (m_lcd_on<<5);

	return data;
}

WRITE8_MEMBER( pcg850v_state::g850v_lcd_control_w )
{
	if ((data & 0xf0) == 0x00)			// LCD column LSB
	{
		m_lcd_index_col = (m_lcd_index_col & 0xf0) | (data & 0x0f);
	}
	else if ((data & 0xf0) == 0x10)		// LCD column MSB
	{
		m_lcd_index_col = (m_lcd_index_col & 0x0f) | ((data<<4) & 0xf0);
	}
	else if ((data & 0xf0) == 0x20)		// LCD on/off
	{
		m_lcd_on = BIT(data, 0);
	}
	else if ((data & 0xc0) == 0x40)		// display start line
	{
		m_lcd_start_line = data & 0x3f;
	}
	else if ((data & 0xe0) == 0x80)		// contrast level
	{
		m_lcd_contrast = data;
	}
	else if ((data & 0xf0) == 0xa0)		// display effects
	{
		m_lcd_effects = data & 0x0f;
	}
	else if ((data & 0xf0) == 0xb0)		// set row
	{
		m_lcd_index_row = data & 0x07;
	}
	else if ((data & 0xf0) == 0xe0)		// display controls
	{
		if (BIT(data, 1))
			m_lcd_contrast = 0;

		m_lcd_read_mode = BIT(data, 2);
	}
	else
	{
		logerror( "PC-G850V: Unknown LCD command 0x%02x\n", data );
	}
}

READ8_MEMBER( pcg850v_state::g850v_lcd_data_r )
{
	UINT8 data = m_vram[(m_lcd_index_row*0x100 + m_lcd_index_col - 1) & 0x7ff];

	if (m_lcd_read_mode == 1)
		m_lcd_index_col++;

	return data;
}

WRITE8_MEMBER( pcg850v_state::g850v_lcd_data_w )
{
	m_vram[(m_lcd_index_row*0x100 + m_lcd_index_col) & 0x7ff] = data;

	m_lcd_index_col++;
}


static ADDRESS_MAP_START(pce220_mem, AS_PROGRAM, 8, pce220_state)
	AM_RANGE(0x0000, 0x3fff) AM_RAMBANK("bank1")
	AM_RANGE(0x4000, 0x7fff) AM_RAMBANK("bank2")
	AM_RANGE(0x8000, 0xbfff) AM_ROMBANK("bank3")
	AM_RANGE(0xc000, 0xffff) AM_ROMBANK("bank4")
ADDRESS_MAP_END

static ADDRESS_MAP_START( pce220_io , AS_IO, 8, pce220_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_READ(kb_r)
	AM_RANGE(0x11, 0x12) AM_WRITE(kb_matrix_w)
	AM_RANGE(0x13, 0x13) AM_READ_PORT("SHIFT")
	AM_RANGE(0x14, 0x14) AM_READWRITE(timer_r, timer_w)
	AM_RANGE(0x15, 0x15) AM_READWRITE(port15_r, port15_w)
	AM_RANGE(0x16, 0x16) AM_READWRITE(irq_status_r, irq_ack_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(irq_mask_w)
	AM_RANGE(0x18, 0x18) AM_READWRITE(port18_r, port18_w)
	AM_RANGE(0x19, 0x19) AM_READWRITE(rom_bank_r, rom_bank_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(boot_bank_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(ram_bank_w)
	AM_RANGE(0x1c, 0x1c) AM_WRITENOP //peripheral reset
	AM_RANGE(0x1d, 0x1d) AM_READ_PORT("BATTERY")
	AM_RANGE(0x1e, 0x1e) AM_WRITENOP //???
	AM_RANGE(0x1f, 0x1f) AM_READ(port1f_r)
	AM_RANGE(0x58, 0x58) AM_WRITE(lcd_control_w)
	AM_RANGE(0x59, 0x59) AM_READ(lcd_status_r)
	AM_RANGE(0x5a, 0x5a) AM_WRITE(lcd_data_w)
	AM_RANGE(0x5b, 0x5b) AM_READ(lcd_data_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pcg850v_io , AS_IO, 8, pcg850v_state)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
	AM_RANGE(0x10, 0x10) AM_READ(kb_r)
	AM_RANGE(0x11, 0x12) AM_WRITE(kb_matrix_w)
	AM_RANGE(0x13, 0x13) AM_READ_PORT("SHIFT")
	AM_RANGE(0x14, 0x14) AM_READWRITE(timer_r, timer_w)
	AM_RANGE(0x15, 0x15) AM_READWRITE(port15_r, port15_w)
	AM_RANGE(0x16, 0x16) AM_READWRITE(irq_status_r, irq_ack_w)
	AM_RANGE(0x17, 0x17) AM_WRITE(irq_mask_w)
	AM_RANGE(0x18, 0x18) AM_READWRITE(port18_r, port18_w)
	AM_RANGE(0x19, 0x19) AM_READWRITE(rom_bank_r, rom_bank_w)
	AM_RANGE(0x1a, 0x1a) AM_WRITE(boot_bank_w)
	AM_RANGE(0x1b, 0x1b) AM_WRITE(ram_bank_w)
	AM_RANGE(0x1c, 0x1c) AM_WRITENOP //peripheral reset
	AM_RANGE(0x1d, 0x1d) AM_READ_PORT("BATTERY")
	AM_RANGE(0x1e, 0x1e) AM_WRITENOP //???
	AM_RANGE(0x1f, 0x1f) AM_READ(port1f_r)
	AM_RANGE(0x40, 0x40) AM_READWRITE(g850v_lcd_status_r, g850v_lcd_control_w)
	AM_RANGE(0x41, 0x41) AM_READWRITE(g850v_lcd_data_r, g850v_lcd_data_w)
	AM_RANGE(0x69, 0x69) AM_READWRITE(g850v_bank_r, g850v_bank_w)
ADDRESS_MAP_END

static INPUT_CHANGED( kb_irq )
{
	pce220_state *state = field.machine().driver_data<pce220_state>();

	if (state->m_irq_mask & IRQ_FLAG_KEY)
	{
		device_set_input_line( state->m_maincpu, 0, newval ? ASSERT_LINE : CLEAR_LINE );

		state->m_irq_flag = (state->m_irq_flag & 0xfe) | (newval & 0x01);
	}
}

static INPUT_CHANGED( on_irq )
{
	pce220_state *state = field.machine().driver_data<pce220_state>();

	if (state->m_irq_mask & IRQ_FLAG_ON)
	{
		device_set_input_line( state->m_maincpu, 0, newval ? ASSERT_LINE : CLEAR_LINE );

		state->m_irq_flag = (state->m_irq_flag & 0xfd) | ((newval & 0x01)<<1);
	}
}

/* Input ports */
static INPUT_PORTS_START( pce220 )
	PORT_START("BATTERY")
	PORT_CONFNAME( 0x01, 0x00, "Battery Status" )
	PORT_CONFSETTING( 0x00, DEF_STR( Normal ) )
	PORT_CONFSETTING( 0x01, "Low Battery" )

	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F10)			PORT_NAME("OFF")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Q)			PORT_NAME("Q")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Q')	PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_W)			PORT_NAME("W")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('W')	PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_E)			PORT_NAME("E")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('E')	PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_R)			PORT_NAME("R")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('R')	PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_T)			PORT_NAME("T")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('T')	PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Y)			PORT_NAME("Y")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Y')	PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_U)			PORT_NAME("U")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('U')	PORT_CHAR('\'')
	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_A)			PORT_NAME("A")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('A')	PORT_CHAR('[')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_S)			PORT_NAME("S")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('S')	PORT_CHAR(']')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_D)			PORT_NAME("D")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('D')	PORT_CHAR('{')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F)			PORT_NAME("F")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('F')	PORT_CHAR('}')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_G)			PORT_NAME("G")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('G')	PORT_CHAR('\\')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_H)			PORT_NAME("H")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('H')	PORT_CHAR('|')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_J)			PORT_NAME("J")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('J')	PORT_CHAR('`')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_K)			PORT_NAME("K")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('K')	PORT_CHAR('_')
	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Z)			PORT_NAME("Z")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_X)			PORT_NAME("X")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_C)			PORT_NAME("C")  		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('C')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_V)			PORT_NAME("V")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_B)			PORT_NAME("B")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_N)			PORT_NAME("N")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('N')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_M)			PORT_NAME("M")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_COMMA)		PORT_NAME(",")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(',')	PORT_CHAR('?')
	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F1)			PORT_NAME("CAL")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F2)			PORT_NAME("BAS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_CAPSLOCK)		PORT_NAME("CAPS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_HOME)			PORT_NAME("ANS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_TAB)			PORT_NAME("TAB")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_SPACE)		PORT_NAME("SPACE")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(' ')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_DOWN)			PORT_NAME("DOWN")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_UP)			PORT_NAME("UP") 		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LEFT)			PORT_NAME("LEFT")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_RIGHT)		PORT_NAME("RIGHT")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F4)			PORT_NAME("CONS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_0)			PORT_NAME("0")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_STOP)			PORT_NAME(".")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('.')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_BACKSLASH)	PORT_NAME("+/-")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_PLUS_PAD)		PORT_NAME("+")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('+')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ENTER)		PORT_NAME("RET")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(13)
	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_L)			PORT_NAME("L")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('L')	PORT_CHAR('=')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_COLON)		PORT_NAME(";")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(';')	PORT_CHAR(':')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_DEL)			PORT_NAME("DEL")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_1)			PORT_NAME("1")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('1')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_2)			PORT_NAME("2")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('2')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_3)			PORT_NAME("3")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('3')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_MINUS)		PORT_NAME("-")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('-')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F5)			PORT_NAME("M+") 		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_I)			PORT_NAME("I")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('I')	PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_O)			PORT_NAME("O")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('O')	PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_INSERT)		PORT_NAME("INS")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_4)			PORT_NAME("4")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_5)			PORT_NAME("5")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('5')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_6)			PORT_NAME("6")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('6')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ASTERISK)		PORT_NAME("*")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('*')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F6)			PORT_NAME("RM") 		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_P)			PORT_NAME("P")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('P')	PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_BACKSPACE)	PORT_NAME("BS") 		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(BACKSPACE))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F7)			PORT_NAME("n!") 		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_7)			PORT_NAME("7")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('7')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_8)			PORT_NAME("8")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('8')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_9)			PORT_NAME("9")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('9')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_SLASH)		PORT_NAME("/")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('/')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_NAME(")")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(')')
	PORT_START("LINE8")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_1_PAD)		PORT_NAME("hyp")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_2_PAD)		PORT_NAME("DEG")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_3_PAD)		PORT_NAME("y^x")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('^')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_4_PAD)		PORT_NAME("sqrt")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_5_PAD)		PORT_NAME("x^2")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_OPENBRACE)	PORT_NAME("(")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_6_PAD)		PORT_NAME("1/x")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_7_PAD)		PORT_NAME("MDF")		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LCONTROL)		PORT_NAME("2nd")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_8_PAD)		PORT_NAME("sin")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_9_PAD)		PORT_NAME("cos")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_0_PAD)		PORT_NAME("ln") 		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F8)			PORT_NAME("log")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F9)			PORT_NAME("tan")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F11)			PORT_NAME("FSE")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ESC)			PORT_NAME("CCE")		PORT_CHANGED( kb_irq, NULL )
	PORT_START("SHIFT")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LSHIFT)		PORT_NAME("Shift")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START("ON")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_PGUP)			PORT_NAME("ON") 		PORT_CHANGED( on_irq, NULL )
INPUT_PORTS_END

static INPUT_PORTS_START( pcg850v )
	PORT_START("BATTERY")
	PORT_CONFNAME( 0x01, 0x00, "Battery Status" )
	PORT_CONFSETTING( 0x00, DEF_STR( Normal ) )
	PORT_CONFSETTING( 0x01, "Low Battery" )

	PORT_START("LINE0")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F10)			PORT_NAME("OFF")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Q)			PORT_NAME("Q")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Q')	PORT_CHAR('!')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_W)			PORT_NAME("W")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('W')	PORT_CHAR('"')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_E)			PORT_NAME("E")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('E')	PORT_CHAR('#')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_R)			PORT_NAME("R")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('R')	PORT_CHAR('$')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_T)			PORT_NAME("T")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('T')	PORT_CHAR('%')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Y)			PORT_NAME("Y")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Y')	PORT_CHAR('&')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_U)			PORT_NAME("U")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('U')	PORT_CHAR('\'')
	PORT_START("LINE1")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_A)			PORT_NAME("A")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('A')	PORT_CHAR('[')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_S)			PORT_NAME("S")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('S')	PORT_CHAR(']')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_D)			PORT_NAME("D")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('D')	PORT_CHAR('{')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F)			PORT_NAME("F")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('F')	PORT_CHAR('}')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_G)			PORT_NAME("G")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('G')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_H)			PORT_NAME("H")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('H')	PORT_CHAR('|')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_J)			PORT_NAME("J")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('J')	PORT_CHAR('`')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_K)			PORT_NAME("K")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('K')	PORT_CHAR('_')
	PORT_START("LINE2")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_Z)			PORT_NAME("Z")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('Z')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_X)			PORT_NAME("X")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('X')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_C)			PORT_NAME("C")  		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('C')
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_V)			PORT_NAME("V")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('V')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_B)			PORT_NAME("B")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('B')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_N)			PORT_NAME("N")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('N')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_M)			PORT_NAME("M")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('M')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_COMMA)		PORT_NAME(",")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(',')	PORT_CHAR('?')
	PORT_START("LINE3")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F1)			PORT_NAME("BAS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F2)			PORT_NAME("TEXT")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_CAPSLOCK)		PORT_NAME("CAPS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_HOME)			PORT_NAME("KANA")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_TAB)			PORT_NAME("TAB")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_SPACE)		PORT_NAME("SPACE")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(' ')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_DOWN)			PORT_NAME("DOWN")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_UP)			PORT_NAME("UP") 		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_START("LINE4")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LEFT)			PORT_NAME("LEFT")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_RIGHT)		PORT_NAME("RIGHT")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F4)			PORT_NAME("CONS")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_0)			PORT_NAME("0")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_STOP)			PORT_NAME(".")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('.')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_EQUALS)		PORT_NAME("=")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('=')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_PLUS_PAD)		PORT_NAME("+")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('+')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ENTER)		PORT_NAME("RET")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(13)
	PORT_START("LINE5")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_L)			PORT_NAME("L")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('L')	PORT_CHAR('=')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_COLON)		PORT_NAME(";")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(';')	PORT_CHAR(':')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_DEL)			PORT_NAME("DEL")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_1)			PORT_NAME("1")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('1')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_2)			PORT_NAME("2")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('2')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_3)			PORT_NAME("3")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('3')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_MINUS)		PORT_NAME("-")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('-')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F5)			PORT_NAME("M+") 		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE6")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_I)			PORT_NAME("I")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('I')	PORT_CHAR('<')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_O)			PORT_NAME("O")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('O')	PORT_CHAR('>')
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_INSERT)		PORT_NAME("INS")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_4)			PORT_NAME("4")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('4')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_5)			PORT_NAME("5")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('5')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_6)			PORT_NAME("6")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('6')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ASTERISK)		PORT_NAME("*")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('*')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F6)			PORT_NAME("RM") 		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE7")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_P)			PORT_NAME("P")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('P')	PORT_CHAR('@')
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_BACKSPACE)	PORT_NAME("BS") 		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_MAMEKEY(BACKSPACE))
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F7)			PORT_NAME("pi") 		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_7)			PORT_NAME("7")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('7')
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_8)			PORT_NAME("8")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('8')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_9)			PORT_NAME("9")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('9')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_SLASH)		PORT_NAME("/")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('/')
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_CLOSEBRACE)	PORT_NAME(")")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(')')
	PORT_START("LINE8")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_1_PAD)		PORT_NAME("nPr")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_2_PAD)		PORT_NAME("DEG")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_3_PAD)		PORT_NAME("SQR")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_4_PAD)		PORT_NAME("SQU")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_5_PAD)		PORT_NAME("x^y")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('^')
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_OPENBRACE)	PORT_NAME("(")			PORT_CHANGED( kb_irq, NULL )	PORT_CHAR('(')
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_6_PAD)		PORT_NAME("1/x")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_7_PAD)		PORT_NAME("MDF")		PORT_CHANGED( kb_irq, NULL )
	PORT_START("LINE9")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LCONTROL)		PORT_NAME("2nd")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_8_PAD)		PORT_NAME("sin")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_9_PAD)		PORT_NAME("cos")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_0_PAD)		PORT_NAME("ln") 		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F8)			PORT_NAME("log")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F9)			PORT_NAME("tan")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_F11)			PORT_NAME("FSE")		PORT_CHANGED( kb_irq, NULL )
	PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_ESC)			PORT_NAME("CCE")		PORT_CHANGED( kb_irq, NULL )
	PORT_START("SHIFT")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_LSHIFT)		PORT_NAME("Shift")		PORT_CHANGED( kb_irq, NULL )	PORT_CHAR(UCHAR_SHIFT_1)
	PORT_START("ON")
	PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD)	PORT_CODE(KEYCODE_PGUP)			PORT_NAME("ON") 		PORT_CHANGED( on_irq, NULL )
INPUT_PORTS_END

void pce220_state::machine_start()
{
	UINT8 *rom = machine().region("user1")->base();
	UINT8 *ram = ram_get_ptr(m_ram);

	memory_configure_bank(machine(), "bank1", 0, 2, ram + 0x0000, 0x8000);
	memory_configure_bank(machine(), "bank2", 0, 2, ram + 0x4000, 0x8000);
	memory_configure_bank(machine(), "bank3", 0, 8, rom, 0x4000);
	memory_configure_bank(machine(), "bank4", 0, 8, rom, 0x4000);

	m_vram = (UINT8*)machine().region("lcd_vram")->base();
}

void pcg850v_state::machine_start()
{
	UINT8 *rom = machine().region("user1")->base();
	UINT8 *ram = ram_get_ptr(m_ram);

	memory_configure_bank(machine(), "bank1", 0, 2, ram + 0x0000, 0x8000);
	memory_configure_bank(machine(), "bank2", 0, 2, ram + 0x4000, 0x8000);
	memory_configure_bank(machine(), "bank3", 0, 22, rom, 0x4000);
	memory_configure_bank(machine(), "bank4", 0, 22, rom, 0x4000);

	m_vram = (UINT8*)machine().region("lcd_vram")->base();
}

void pce220_state::machine_reset()
{
	address_space *space = m_maincpu->memory().space(AS_PROGRAM);
	space->unmap_write(0x0000, 0x3fff);

	// install the boot code into the first bank
	memory_set_bankptr(machine(), "bank1", machine().region("user1")->base() + 0x0000);

	m_lcd_index_row = 0;
	m_lcd_index_col = 0;
	m_lcd_start_line = 0;
	m_lcd_on = 0;
	m_bank_num = 0;
	m_irq_mask = 0;
	m_irq_flag = 0;
	m_timer_status = 0;
	m_kb_matrix = 0;
}

void pcg850v_state::machine_reset()
{
	pce220_state::machine_reset();

	m_g850v_bank_num = 0;
	m_lcd_effects = 0;
	m_lcd_contrast = 0;
	m_lcd_read_mode = 0;
}

static TIMER_DEVICE_CALLBACK(pce220_timer_callback)
{
	pce220_state *state = timer.machine().driver_data<pce220_state>();

	state->m_timer_status = !state->m_timer_status;

	if (state->m_irq_mask & IRQ_FLAG_TIMER)
	{
		device_set_input_line( state->m_maincpu, 0, HOLD_LINE );

		state->m_irq_flag = (state->m_irq_flag & 0xfb) | (state->m_timer_status<<2);
	}
}

static PALETTE_INIT(pce220)
{
	palette_set_color(machine, 0, MAKE_RGB(138, 146, 148));
	palette_set_color(machine, 1, MAKE_RGB(92, 83, 88));
}


static MACHINE_CONFIG_START( pce220, pce220_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, 3072000 ) // CMOS-SC7852
    MCFG_CPU_PROGRAM_MAP(pce220_mem)
    MCFG_CPU_IO_MAP(pce220_io)

    /* video hardware */
	// 4 lines x 24 characters, resp. 144 x 32 pixel
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(24*6, 4*8)
    MCFG_SCREEN_VISIBLE_AREA(0, 24*6-1, 0, 4*8-1)

    MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(pce220)
	MCFG_DEFAULT_LAYOUT(layout_lcd)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_TIMER_ADD_PERIODIC("pce220_timer", pce220_timer_callback, attotime::from_msec(468))

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K") // 32K internal + 32K external card

	MCFG_PCE220_SERIAL_ADD(PCE220SERIAL_TAG)
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( pcg850v, pcg850v_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",Z80, XTAL_8MHz ) // CMOS-SC7852
    MCFG_CPU_PROGRAM_MAP(pce220_mem)
    MCFG_CPU_IO_MAP(pcg850v_io)

    /* video hardware */
	// 6 lines x 24 characters, resp. 144 x 48 pixel
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(144, 48)
    MCFG_SCREEN_VISIBLE_AREA(0, 144-1, 0, 48-1)

    MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(pce220)
	MCFG_DEFAULT_LAYOUT(layout_lcd)

	/* sound hardware */
	MCFG_SPEAKER_STANDARD_MONO("mono")
	MCFG_SOUND_ADD(BEEPER_TAG, BEEP, 0)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MCFG_TIMER_ADD_PERIODIC("pce220_timer", pce220_timer_callback, attotime::from_msec(468))

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("64K") // 32K internal + 32K external card

	MCFG_PCE220_SERIAL_ADD(PCE220SERIAL_TAG)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pce220 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v1", "v 0.1")
	ROM_SYSTEM_BIOS( 1, "v2", "v 0.2")
	ROM_LOAD( "bank0.bin",      0x0000, 0x4000, CRC(1fa94d11) SHA1(24c54347dbb1423388360a359aa09db47d2057b7))
	ROM_LOAD( "bank1.bin",      0x4000, 0x4000, CRC(0f9864b0) SHA1(6b7301c96f1a865e1931d82872a1ed5d1f80644e))
	ROM_LOAD( "bank2.bin",      0x8000, 0x4000, CRC(1625e958) SHA1(090440600d461aa7efe4adbf6e975aa802aabeec))
	ROM_LOAD( "bank3.bin",      0xc000, 0x4000, CRC(ed9a57f8) SHA1(58087dc64103786a40325c0a1e04bd88bfd6da57))
	ROM_LOAD( "bank4.bin",     0x10000, 0x4000, CRC(e37665ae) SHA1(85f5c84f69f79e7ac83b30397b2a1d9629f9eafa))
	ROMX_LOAD( "bank5.bin",     0x14000, 0x4000, CRC(6b116e7a) SHA1(b29f5a070e846541bddc88b5ee9862cc36b88eee),ROM_BIOS(2))
	ROMX_LOAD( "bank5_0.1.bin", 0x14000, 0x4000, CRC(13c26eb4) SHA1(b9cd0efd6b195653b9610e20ad8aab541824a689),ROM_BIOS(1))
	ROMX_LOAD( "bank6.bin",     0x18000, 0x4000, CRC(4fbfbd18) SHA1(e5aab1df172dcb94aa90e7d898eacfc61157ff15),ROM_BIOS(2))
	ROMX_LOAD( "bank6_0.1.bin", 0x18000, 0x4000, CRC(e2cda7a6) SHA1(01b1796d9485fde6994cb5afbe97514b54cfbb3a),ROM_BIOS(1))
	ROMX_LOAD( "bank7.bin",     0x1c000, 0x4000, CRC(5e98b5b6) SHA1(f22d74d6a24f5929efaf2983caabd33859232a94),ROM_BIOS(2))
	ROMX_LOAD( "bank7_0.1.bin", 0x1c000, 0x4000, CRC(d8e821b2) SHA1(18245a75529d2f496cdbdc28cdf40def157b20c0),ROM_BIOS(1))

	ROM_REGION( 0x200, "lcd_vram", ROMREGION_ERASE00)	//HD61202 internal RAM (4096 bits)
ROM_END

ROM_START( pcg850v )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_REGION( 0x58000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "rom00.bin", 0x00000, 0x4000, CRC(c41a7a3e) SHA1(8b85d07e6f2fa048f8a958c261ab3344df750bd2))
	ROM_LOAD( "rom01.bin", 0x04000, 0x4000, CRC(45cafcaf) SHA1(e6142ac2ddb50c90bf1afd03ee2d2741abde8b76))
	ROM_LOAD( "rom02.bin", 0x08000, 0x4000, CRC(c81a804e) SHA1(322b9c6ca5cb7bc41a55c7874838e601ef50644e))
	ROM_LOAD( "rom03.bin", 0x0c000, 0x4000, CRC(9301e937) SHA1(b152186577abf86a7fb535cd63985f2a67e154b9))
	ROM_LOAD( "rom04.bin", 0x10000, 0x4000, CRC(6bf11755) SHA1(fe7458758d26fabfcca34e791fe2425104830b7b))
	ROM_LOAD( "rom05.bin", 0x14000, 0x4000, CRC(8a808f5e) SHA1(448caf0cc30de483d876c31785c8bb27305860b8))
	ROM_LOAD( "rom06.bin", 0x18000, 0x4000, CRC(3902f135) SHA1(40fbd51718a830e3f374843990522e29508376fd))
	ROM_LOAD( "rom07.bin", 0x1c000, 0x4000, CRC(618a7be8) SHA1(5f16e47eeeb9c153bb95c6d6e3af276576b8274e))
	ROM_LOAD( "rom08.bin", 0x20000, 0x4000, CRC(bf05b34d) SHA1(8556d64ddd89191c9dc048280e384f9146d81a16))
	ROM_LOAD( "rom09.bin", 0x24000, 0x4000, CRC(b75dbd0a) SHA1(ca149134a819fa52591ae1733270f8b3c811a18a))
	ROM_LOAD( "rom0a.bin", 0x28000, 0x4000, CRC(e7c5ec8f) SHA1(70cac20d4f01b5b2fdd76837659bc4947a1ebd21))
	ROM_LOAD( "rom0b.bin", 0x2c000, 0x4000, CRC(bdc4f889) SHA1(154d6223e8e750fcbffdb069215fee9492685747))
	ROM_LOAD( "rom0c.bin", 0x30000, 0x4000, CRC(6ef5560a) SHA1(5b2ad8e36354f448fe01acba1d2f69630de345a7))
	ROM_LOAD( "rom0d.bin", 0x34000, 0x4000, CRC(4295a44e) SHA1(0f8dfc759ed17f2b0acc1dc4912e881b1f0f65aa))
	ROM_LOAD( "rom0e.bin", 0x38000, 0x4000, CRC(0c8212e1) SHA1(8145728d87ea5a5e29c0e98c3d2d4278b1070359))
	ROM_LOAD( "rom0f.bin", 0x3c000, 0x4000, CRC(33c30dba) SHA1(296fb6fa3921822c3458acde0962c3a25cb19a1e))
	ROM_LOAD( "rom10.bin", 0x40000, 0x4000, CRC(553a0926) SHA1(2149827f9a2eb879756b3da609e17fc5b1c9bd78))
	ROM_LOAD( "rom11.bin", 0x44000, 0x4000, CRC(385a128d) SHA1(eda1192a75e4cfb50c0ac058f230a84cbfe67d51))
	ROM_LOAD( "rom12.bin", 0x48000, 0x4000, CRC(cd12dfca) SHA1(50a5defdeb4da9b2eac196816559056fc55e1dca))
	ROM_LOAD( "rom13.bin", 0x4c000, 0x4000, CRC(9bca873b) SHA1(4ce43553fb15c0ca866e9582a3ef8dc22a2795b4))
	ROM_LOAD( "rom14.bin", 0x50000, 0x4000, CRC(a2938c40) SHA1(d2c24401eea5e56268ef6eadcc612c8bbaa3342a))
	ROM_LOAD( "rom15.bin", 0x54000, 0x4000, CRC(53a0bf0a) SHA1(bf27baeaf208628fbe3c959d623cc08de21cf9f8))

	ROM_REGION( 0x1000, "lcd_vram", ROMREGION_ERASE00)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT COMPANY   FULLNAME       FLAGS */
COMP( 1991, pce220,  0,       0,	pce220, 	pce220,  0,   "Sharp",   "PC-E220",		GAME_NOT_WORKING )
COMP( 2001, pcg850v, 0,       0,	pcg850v,	pcg850v, 0,   "Sharp",   "PC-G850V",	GAME_NOT_WORKING )

