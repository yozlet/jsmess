/**********************************************************************

    Motorola MC6845 and compatible CRT controller emulation

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

**********************************************************************/

#ifndef __MC6845__
#define __MC6845__


#define MC6845		DEVICE_GET_INFO_NAME(mc6845)
#define MC6845_1	DEVICE_GET_INFO_NAME(mc6845_1)
#define R6545_1		DEVICE_GET_INFO_NAME(r6545_1)
#define C6545_1 	DEVICE_GET_INFO_NAME(c6545_1)
#define H46505	 	DEVICE_GET_INFO_NAME(h46505)
#define HD6845 		DEVICE_GET_INFO_NAME(hd6845)
#define SY6545_1 	DEVICE_GET_INFO_NAME(sy6545_1)


/* callback definitions */
typedef void * (*mc6845_begin_update_func)(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);
#define MC6845_BEGIN_UPDATE(name)	void *name(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect)

typedef void (*mc6845_update_row_func)(const device_config *device, bitmap_t *bitmap,
					   				   const rectangle *cliprect, UINT16 ma, UINT8 ra,
					   				   UINT16 y, UINT8 x_count, INT8 cursor_x, void *param);
#define MC6845_UPDATE_ROW(name)		void name(const device_config *device, bitmap_t *bitmap,	\
					   						  const rectangle *cliprect, UINT16 ma, UINT8 ra,					\
					   						  UINT16 y, UINT8 x_count, INT8 cursor_x, void *param)

typedef void (*mc6845_end_update_func)(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect, void *param);
#define MC6845_END_UPDATE(name)		void name(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect, void *param)

typedef void (*mc6845_on_de_changed_func)(const device_config *device, int display_enabled);
#define MC6845_ON_DE_CHANGED(name)	void name(const device_config *device, int display_enabled)

typedef void (*mc6845_on_hsync_changed_func)(const device_config *device, int hsync);
#define MC6845_ON_HSYNC_CHANGED(name)	void name(const device_config *device, int hsync)

typedef void (*mc6845_on_vsync_changed_func)(const device_config *device, int vsync);
#define MC6845_ON_VSYNC_CHANGED(name)	void name(const device_config *device, int vsync)


/* interface */
typedef struct _mc6845_interface mc6845_interface;
struct _mc6845_interface
{
	const char *screen_tag;		/* screen we are acting on */
	int clock;					/* the clock (pin 21) of the chip */
	int hpixels_per_column;		/* number of pixels per video memory address */

	/* if specified, this gets called before any pixel update,
       optionally return a pointer that will be passed to the
       update and tear down callbacks */
	mc6845_begin_update_func		begin_update;

	/* this gets called for every row, the driver must output
       x_count * hpixels_per_column pixels.
       cursor_x indicates the character position where the cursor is, or -1
       if there is no cursor on this row */
	mc6845_update_row_func			update_row;

	/* if specified, this gets called after all row updating is complete */
	mc6845_end_update_func			end_update;

	/* if specified, this gets called for every change of the disply enable pin (pin 18) */
	mc6845_on_de_changed_func		on_de_changed;

	/* if specified, this gets called for every change of the HSYNC pin (pin 39) */
	mc6845_on_hsync_changed_func	on_hsync_changed;

	/* if specified, this gets called for every change of the VSYNC pin (pin 40) */
	mc6845_on_vsync_changed_func	on_vsync_changed;
};


/* device interface */
DEVICE_GET_INFO( mc6845 );
DEVICE_GET_INFO( mc6845_1 );
DEVICE_GET_INFO( r6545_1 );
DEVICE_GET_INFO( c6545_1 );
DEVICE_GET_INFO( h46505 );
DEVICE_GET_INFO( hd6845 );
DEVICE_GET_INFO( sy6545_1 );

/* select one of the registers for reading or writing */
WRITE8_DEVICE_HANDLER( mc6845_address_w );

/* read from the status register */
READ8_DEVICE_HANDLER( mc6845_status_r );

/* read from the currently selected register */
READ8_DEVICE_HANDLER( mc6845_register_r );

/* write to the currently selected register */
WRITE8_DEVICE_HANDLER( mc6845_register_w );

/* return the current value on the MA0-MA13 pins */
UINT16 mc6845_get_ma(const device_config *device);

/* return the current value on the RA0-RA4 pins */
UINT8 mc6845_get_ra(const device_config *device);

/* simulates the LO->HI clocking of the light pen pin (pin 3) */
void mc6845_assert_light_pen_input(const device_config *device);

/* updates the screen -- this will call begin_update(),
   followed by update_row() reapeatedly and after all row
   updating is complete, end_update() */
void mc6845_update(const device_config *device, bitmap_t *bitmap, const rectangle *cliprect);


#endif
