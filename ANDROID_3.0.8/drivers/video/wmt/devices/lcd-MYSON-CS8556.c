/*++
 * Copyright c 2012  WonderMedia  Technologies, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * WonderMedia Technologies, Inc.
 * 4F, 533, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C
--*/

#define LCD_MYSON_CS8556_C
// #define DEBUG
/*----------------------- DEPENDENCE -----------------------------------------*/
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include "../lcd.h"
#include "../edid.h"

/*----------------------- PRIVATE TYPE --------------------------------------*/


/*----------------------- PRIVATE MACRO --------------------------------------*/
#define CS8556_NAME          "CS8556"

#define TVFORMAT_PROC_FILE "tvformat"

#define DEBUG

#ifdef DEBUG
#define DBG(fmt, args...) printk(KERN_DEBUG "[%s]: " fmt, __FUNCTION__ , ## args)
#define INFO(fmt, args...) printk(KERN_INFO "[" CS8556_NAME "] " fmt , ## args)
#else
#define DBG(fmt, args...)
#define INFO(fmt, args...)
#endif

#define ERROR(fmt,args...) printk(KERN_ERR "[" CS8556_NAME "] " fmt , ## args)
#define WARNING(fmt,args...) printk(KERN_WARNING "[" CS8556_NAME "] " fmt , ## args)

/*----------------------- PRIVATE CONSTANTS ----------------------------------*/
#define CS8556_I2C_ADDR 0x3d

#define MAX_PIXEL_NUM  1280*1024

#define ASPECT_16_10 0
#define ASPECT_16_9  1
#define ASPECT_4_3   2
#define ASPECT_5_4   3

/*----------------------- PRIVATE VARIABLE ----------------------------------*/
static int s_cs8556_init = 0;
static cs8556_output_format_t s_cs8556_output_format = VGA;
static int s_tvformat_is_set = 0;
static struct proc_dir_entry *s_tvformat_proc_file;
static struct i2c_client *s_cs8556_client;

static unsigned char s_RGB888_To_Dsub_Offset0[]={
	0x0F,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xDB,0x05,0x38,0x00,0x40,0x00,0xDB,0x05,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x1F,0x03,0x03,0x00,0x1C,0x00,0x1F,0x03,0x00,0x00,0x78,0x00,0x1E,0x00,0x37,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xDB,0x05,0x38,0x00,0x40,0x00,0xDB,0x05,0x00,0x00,0x00,0x10,0x00,0x00,0x08,0x03,
	0x1F,0x03,0x03,0x00,0x1C,0x00,0x1F,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0x11,0x0D,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x01,0x08,0x00,0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char s_RGB888_To_Dsub_Offset1[]={
	0x80,0x10,0x80,0x00,0x00,0x00,0x99,0x11,0x2A,0x00,0x70,0x30,0x2A,0x71,0x9C,0x00,
	0x10,0x2A,0x85,0x82,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,
	0x41,0x01,0x98,0x08,0xCC,0x00,0x4C,0x08,0x00,0x00,0x00,0x00,0x40,0x00,0x40,0x00,
	0x00,0x01,0xEE,0x02,0x3A,0x07,0x65,0x04,0x14,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x06,0x40,0x07,0x30,0x15,0x10,0x31,0x02,0x33,0x12,0x34,0x52,0x38,0x42,0x39,0x62,
	0x48,0x12,0x64,0x04,0x66,0x14,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,0x00,0x10,
	0x7E,0x10,0x00,0xF4,0x10,0x76,0x08,0x00,0x8A,0x00,0x4F,0x11,0x4F,0x4F,0x81,0x00,
	0x03,0x00,0x89,0x45,0x01,0x45,0x00,0x00,0x03,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char s_CS8556_Original_Offset0[]={
	0xF0,0x7F,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x00,0x00,0x02,0x01,0x00,0x00,0x01,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static unsigned char s_RGB888_To_NTSC_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x59,0x03,0x3D,0x00,0x7E,0x00,0x49,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x0C,0x02,0x05,0x00,0x21,0x00,0x03,0x02,0x00,0x00,0x7A,0x00,0x23,0x00,0x16,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xB3,0x06,0x7F,0x00,0x00,0x01,0xA4,0x06,0x00,0x00,0x05,0x50,0x00,0x01,0x07,0x01,
	0x0C,0x02,0x02,0x00,0x12,0x00,0x07,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x00,0x00 
};

static unsigned char s_RGB888_To_PAL_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5F,0x03,0x3F,0x00,0x7D,0x00,0x53,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x70,0x02,0x04,0x00,0x2E,0x00,0x62,0x02,0x00,0x00,0x84,0x00,0x2B,0x00,0x36,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xBF,0x06,0x7F,0x00,0xFE,0x00,0xA4,0x06,0x00,0x00,0x2D,0x11,0x3C,0x01,0x3A,0x01,
	0x70,0x02,0x04,0x00,0x12,0x00,0x34,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x40,0x01
};

/*
static unsigned char s_RGB888_To_NTSC_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x59,0x03,0x3D,0x00,0x7B,0x00,0x49,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x0C,0x02,0x05,0x00,0x22,0x00,0x03,0x02,0x00,0x00,0x7A,0x00,0x23,0x00,0x16,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xB3,0x06,0x7F,0x00,0x00,0x01,0xA4,0x06,0x00,0x00,0x05,0x50,0x00,0x01,0x07,0x01,
	0x0C,0x02,0x02,0x00,0x12,0x00,0x07,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x00,0x00 
};

static unsigned char s_RGB888_To_PAL_Offset0[]={
	0x01,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x02,0x02,0x00,0x00,0x00,0x00,
	0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x5F,0x03,0x3F,0x00,0x7B,0x00,0x53,0x03,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,
	0x70,0x02,0x04,0x00,0x2C,0x00,0x62,0x02,0x00,0x00,0x84,0x00,0x2B,0x00,0x36,0x00,
	0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xBF,0x06,0x7F,0x00,0xFE,0x00,0xA4,0x06,0x00,0x00,0x2D,0x11,0x3C,0x01,0x3B,0x01,
	0x70,0x02,0x04,0x00,0x12,0x00,0x34,0x01,0x00,0x00,0x70,0x70,0x70,0x00,0x00,0x00,
	0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x41,0x18,0x09,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x80,0x80,0x00,0x00,0x80,0x80,0x00,0x00,
	0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0x80,0x00,0x80,0x00,0x80,0x00,0x80,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x01,0x24,0x1A,0x00,0x01,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x04,0x01,0xA4,0x06,0x0B,0x00,0x07,0x01,0xF0,0x00,0x00,0x00,0x00,0x04,0x40,0x01
};
*/


static vpp_timing_t s_640_480_60fps_timing = { /* 640x480@60 */
	25175000,				/* pixel clock */
	0,	/* option */
	96, 48, 640, 16,		/* H sync, bp, pixel, fp */
	2, 33, 480, 10			/* V sync, bp, line, fp */
};

static vpp_timing_t s_640_480_72fps_timing = { /* 640x480@72 */
	31500000,				/* pixel clock */
	0,	/* option */
	40, 128, 640, 24,		/* H sync, bp, pixel, fp */
	3, 28, 480, 9			/* V sync, bp, line, fp */
};

static vpp_timing_t s_640_480_75fps_timing = { /* 640x480@75 */
	31500000,				/* pixel clock */
	0,	/* option */
	64, 120, 640, 16,		/* H sync, bp, pixel, fp */
	3, 16, 480, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_640_480_85fps_timing = { /* 640x480@85 */
	36000000,				/* pixel clock */
	0,	/* option */
	56, 80, 640, 56,		/* H sync, bp, pixel, fp */
	3, 25, 480, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_800_600_60fps_timing = { /* 800x600@60 */
	40000000,				/* pixel clock */
	0,	/* option */
	128, 88, 800, 40,		/* H sync, bp, pixel, fp */
	4, 23, 600, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_800_600_72fps_timing = { /* 800x600@72 */
	50000000,				/* pixel clock */
	0,	/* option */
	120, 64, 800, 56,		/* H sync, bp, pixel, fp */
	6, 23, 600, 37			/* V sync, bp, line, fp */
};

static vpp_timing_t s_800_600_75fps_timing = { /* 800x600@75 */
	49500000,				/* pixel clock */
	0,	/* option */
	80, 160, 800, 16,		/* H sync, bp, pixel, fp */
	3, 21, 600, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_800_600_85fps_timing = { /* 800x600@85 */
	56250000,				/* pixel clock */
	0,	/* option */
	64, 152, 800, 32,		/* H sync, bp, pixel, fp */
	3, 27, 600, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1024_768_70fps_timing = { /* 1024x768@70 */
	75000000,				/* pixel clock */
	0, /* option */
	136, 144, 1024, 24,		/* H sync, bp, pixel, fp */
	6, 29, 768, 3			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1024_768_75fps_timing = { /* 1024x768@75 */
	78750000,				/* pixel clock */
	0, /* option */
	96, 176, 1024, 16,		/* H sync, bp, pixel, fp */
	3, 28, 768, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1024_768_85fps_timing = { /* 1024x768@80 */
	94500000,				/* pixel clock */
	0, /* option */
	96, 208, 1024, 48,		/* H sync, bp, pixel, fp */
	3, 36, 768, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1280_720_60fps_timing = {  /* 1280x720@60 */
	74250000,				/* pixel clock */
	0, /* option */
	40, 220, 1280, 110,		/* H sync, bp, pixel, fp */
	5, 20, 720, 5			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1440_900_60fps_timing = { /* 1440x900@60 */
	106500000,				/* pixel clock */
	0,	/* option */
	152, 232, 1440, 80,		/* H sync, bp, pixel, fp */
	6, 25, 900, 3			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1280_960_60fps_timing = { /* 1280x960@60 */
	108000000,				/* pixel clock */
	0, /* option */
	112, 312, 1280, 96,		/* H sync, bp, pixel, fp */
	3, 36, 960, 1			/* V sync, bp, line, fp */
};

static vpp_timing_t s_1280_960_85fps_timing = { /* 1280x960@85 */
	148500000,				/* pixel clock */
	0, /* option */
	160, 224, 1280, 64,		/* H sync, bp, pixel, fp */
	3, 47, 960, 1			/* V sync, bp, line, fp */
};


static vpp_timing_t s_1280_1024_60fps_timing = { /* 1280x1024@60 */
	108000000,				/* pixel clock */
	0, /* option */
	112, 248, 1280, 48,		/* H sync, bp, pixel, fp */
	3, 38, 1024, 1			/* V sync, bp, line, fp */
};

/*----------------------- PRIVATE TYPE  --------------------------------------*/
/* typedef  xxxx lcd_xxx_t; *//*Example*/

/*----------EXPORTED PRIVATE VARIABLES are defined in lcd.h  -------------*/


/*----------------------- INTERNAL PRIVATE VARIABLES - -----------------------*/
/* int  lcd_xxx;        *//*Example*/
lcd_parm_t lcd_cs8556_vga_parm = {
	.name = "MYSON CS8556 VGA",
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 65000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 136,				/* horizontal sync pulse */
		.hbp = 160,					/* horizontal back porch */
		.hpixel = 1024,				/* horizontal pixel */
		.hfp = 24,					/* horizontal front porch */

		.vsync = 6,					/* vertical sync pulse */
		.vbp = 29,					/* vertical back porch */
		.vpixel = 768,				/* vertical pixel */
		.vfp = 3,					/* vertical front porch */
	},

	.initial = lcd_cs8556_initial,
};

lcd_parm_t lcd_cs8556_ntsc_parm = {
	.name = "MYSON CS8556 NTSC",
	.fps = 60,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 27000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 62,				/* horizontal sync pulse */
		.hbp = 60,					/* horizontal back porch */
		.hpixel = 720,				/* horizontal pixel */
		.hfp = 16,					/* horizontal front porch */

		.vsync = 6,					/* vertical sync pulse */
		.vbp = 30,					/* vertical back porch */
		.vpixel = 480,				/* vertical pixel */
		.vfp = 9,					/* vertical front porch */
	},

	.initial = lcd_cs8556_initial,
};

lcd_parm_t lcd_cs8556_pal_parm = {
	.name = "MYSON CS8556 PAL",
	.fps = 50,						/* frame per second */
	.bits_per_pixel = 24,
	.capability = 0,
	.timing = {
		.pixel_clock = 27000000,	/* pixel clock */
		.option = 0,				/* option flags */

		.hsync = 64,				/* horizontal sync pulse */
		.hbp = 68,					/* horizontal back porch */
		.hpixel = 720,				/* horizontal pixel */
		.hfp = 12,					/* horizontal front porch */

		.vsync = 5,					/* vertical sync pulse */
		.vbp = 39,					/* vertical back porch */
		.vpixel = 576,				/* vertical pixel */
		.vfp = 5,					/* vertical front porch */
	},

	.initial = lcd_cs8556_initial,
};

/*--------------------- INTERNAL PRIVATE FUNCTIONS ---------------------------*/
/* void lcd_xxx(void); *//*Example*/

static int I2CMultiPageRead(unsigned char maddr, unsigned char page, unsigned char saddr, int number, unsigned char *value)
{
	int ret;
	unsigned char wbuf[2];
	struct i2c_msg rd[2];

	wbuf[0] = page;
	wbuf[1] = saddr;

	rd[0].addr  = maddr;
	rd[0].flags = 0;
	rd[0].len   = 2;
	rd[0].buf   = wbuf;

	rd[1].addr  = maddr;
	rd[1].flags = I2C_M_RD;
	rd[1].len   = number;
	rd[1].buf   = value;

	ret = i2c_transfer(s_cs8556_client->adapter, rd, ARRAY_SIZE(rd));

	if (ret != ARRAY_SIZE(rd))
	{
		ERROR("I2CMultiPageRead() fail\n");
		return -1;
	}

	return 0;
}

static int I2CMultiPageWrite(unsigned char maddr, unsigned char page, unsigned char saddr, int number, unsigned char *value)
{
	int ret;
	unsigned char *pbuf;
	struct i2c_msg wr[1];

	pbuf = kmalloc(number + 2, GFP_KERNEL);
    if(!pbuf)
    {
        ERROR("I2CMultiPageWrite() alloc memory fail\n");
        return -1;
    }

	*pbuf = page;
	*(pbuf + 1) = saddr;

	memcpy(pbuf + 2, value, number);

	wr[0].addr  = maddr;
	wr[0].flags = 0;
	wr[0].len   = number + 2;
	wr[0].buf   = pbuf;

	ret = i2c_transfer(s_cs8556_client->adapter, wr, ARRAY_SIZE(wr));

	if (ret != ARRAY_SIZE(wr))
	{
		ERROR("I2CMultiPageWrite() fail\n");
		kfree(pbuf);
		return -1;
	}

	kfree(pbuf);
	return 0 ;
}

static int I2CRead(unsigned char addr, unsigned int index, char *pdata, int len)
{
	int ret;
	unsigned char wbuf[1];
	struct i2c_msg rd[2];

	addr = (addr >> 1);

	wbuf[0] = index;
	wbuf[1] = 0x0;

	rd[0].addr  = addr;
	rd[0].flags = 0;
	rd[0].len   = 1;
	rd[0].buf   = wbuf;

	rd[1].addr  = addr;
	rd[1].flags = I2C_M_RD;
	rd[1].len   = len;
	rd[1].buf   = (unsigned char *)pdata;

	ret = i2c_transfer(s_cs8556_client->adapter, rd, ARRAY_SIZE(rd));

	if (ret != ARRAY_SIZE(rd))
	{
		ERROR("I2CRead() fail \n");
		return -1;
	}

	return 0;
}

static int get_aspect(unsigned int resx,unsigned int resy)
{
	int val;

	val = resx * 10 / resy;

	if(val > 16)
		//1920x1080
		//2048x1152
		return ASPECT_16_9;
	else if(val == 16)
		//1680x1050
		//1920x1200
		//2560x1600
		return ASPECT_16_10;
	else if(val >= 13)
		//1440x1080
		//1600x1200
		//1400x1050
		//1792x1344
		//1856x1392
		//1920x1440
		return ASPECT_4_3;
	else
		return ASPECT_5_4;
}

static void vga_show_timing(void)
{
	vpp_timing_t *t;

	t = &lcd_cs8556_vga_parm.timing;

	INFO("VGA Timing: pixclk %d, fps %d, option 0x%x, hsync %d, hbp %d, hpixel %d, hfp %d, vsync %d, vbp %d, vpixel %d, vfp %d\n",
			t->pixel_clock, lcd_cs8556_vga_parm.fps, t->option,
			t->hsync, t->hbp, t->hpixel, t->hfp,
			t->vsync, t->vbp, t->vpixel, t->vfp);
}

int cs8556_vga_get_edid(void)
{
	int i,cnt;
	char edid[128*EDID_BLOCK_MAX] = {0};
	edid_info_t edid_info;
	int fps, remain, aspect;
	vpp_timing_t timing;
	unsigned int htotal, vtotal, pixeltotal;
	edid_timing_t sta_timing[8];

	memset(edid,0x0,128*EDID_BLOCK_MAX);
	if( I2CRead(0xA0,0,&edid[0],128) )
	{
		DBG_ERR("read edid\n");
		vga_show_timing();
		return 1;
	}

	if( edid_checksum(edid,128) )
	{
		DBG_ERR("checksum\n");
		vga_show_timing();
		return 1;
	}

	cnt = edid[0x7E];
	if( cnt >= 3 )
		cnt = 3;

	for(i=1;i<=cnt;i++)
		I2CRead(0xA0,0x80*i,&edid[128*i],128);

/*
	for(i = 0; i < 256;)
	{
		printk("0x%02X,", edid[i]);
		if((++i) % 16 == 0)
			printk("\n");
	}
*/

	memset(&edid_info, 0, sizeof(edid_info_t));
	edid_parse(edid,&edid_info);

	memcpy(&timing, &edid_info.detail_timing[0], sizeof(vpp_timing_t));

	if( timing.pixel_clock == 0 || timing.hpixel == 0 || timing.vpixel == 0)
	{
		ERROR("could not find detail timing of EDID\n");
		vga_show_timing();
		return 1;
	}

	//INFO("VGA EDID  : pixclk %d, option 0x%x, hsync %d, hbp %d, hpixel %d, hfp %d, vsync %d, vbp %d, vpixel %d, vfp %d\n",
	//			timing.pixel_clock, timing.option, timing.hsync, timing.hbp, timing.hpixel, timing.hfp,
	//			timing.vsync, timing.vbp, timing.vpixel, timing.vfp);

	//INFO("sta timing -----------------\n");
	memcpy(&sta_timing, &edid_info.standard_timing, sizeof(edid_timing_t) * 8);
	/*
	for(i = 0; i < 8; i++)
	INFO("VGA STA %d   : resx %d, resy %d, frq %d\n",
				i, sta_timing[i].resx, sta_timing[i].resy, sta_timing[i].freq);
	*/
	htotal = timing.hsync + timing.hbp + timing.hpixel + timing.hfp;
	vtotal = timing.vsync + timing.vbp + timing.vpixel + timing.vfp;
	pixeltotal = htotal * vtotal;

	fps = timing.pixel_clock /pixeltotal;
	remain = timing.pixel_clock %pixeltotal;
	//INFO("htotal = %d, vtotal = %d, pixeltotal = %d\n", htotal, vtotal, pixeltotal);
	//INFO("fps = %d, remain = %d\n", fps, remain);

	if((remain != 0) && (((remain * 10) / pixeltotal)/10 > 5))
		fps++;

	timing.option = 0;

	if(timing.hpixel == 640 && timing.vpixel == 480)
	{
		if(fps == 60)
		{
			memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_60fps_timing, sizeof(vpp_timing_t));
			return 0;
		}
		else if(fps == 73)
		{
			memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_72fps_timing, sizeof(vpp_timing_t));
			lcd_cs8556_vga_parm.fps = fps;
			return 0;
		}
	}
/*
	if(timing.hpixel == 1920 && timing.vpixel == 1080)
	{
		if(timing.pixel_clock == 148500000)
			timing.pixel_clock = 150000000;
	}
*/
	if(timing.vpixel < 480) //for CRT
	{
		for(i = 0; i < 8; i++)
		{
			if(sta_timing[i].resx == 1280 && sta_timing[i].resy == 1024)
			{
				switch(sta_timing[i].freq)
				{
					case 60:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1280_1024_60fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 60;
					return 0;
				}
			}
			if(sta_timing[i].resx == 0 && sta_timing[i].resy == 0)
				break;
		}

		for(i = 0; i < 8; i++)
		{
			if(sta_timing[i].resx == 1280 && sta_timing[i].resy == 960)
			{
				switch(sta_timing[i].freq)
				{
					case 85:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1280_960_85fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 85;
					return 0;

					case 60:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1280_960_60fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 60;
					return 0;
				}
			}
			if(sta_timing[i].resx == 0 && sta_timing[i].resy == 0)
				break;
		}

		for(i = 0; i < 8; i++)
		{
			if(sta_timing[i].resx == 1024 && sta_timing[i].resy == 768)
			{
				switch(sta_timing[i].freq)
				{
					case 85:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1024_768_85fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 85;
						return 0;
					case 75:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1024_768_75fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 75;
						return 0;
					case 70:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_1024_768_70fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 70;
						return 0;
					case 60:
					return 0;
				}
			}
			if(sta_timing[i].resx == 0 && sta_timing[i].resy == 0)
				break;
		}

		for(i = 0; i < 8; i++)
		{
			if(sta_timing[i].resx == 800 && sta_timing[i].resy == 600)
			{
				switch(sta_timing[i].freq)
				{
					case 85:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_800_600_85fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 85;
						return 0;
					case 75:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_800_600_75fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 75;
						return 0;
					case 72:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_800_600_72fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 72;
						return 0;
					case 60:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_800_600_60fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 60;
						return 0;
				}
			}

			if(sta_timing[i].resx == 0 && sta_timing[i].resy == 0)
				break;
		}

		for(i = 0; i < 8; i++)
		{
			if(sta_timing[i].resx == 640 && sta_timing[i].resy == 480)
			{
				switch(sta_timing[i].freq)
				{
					case 85:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_85fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 85;
						return 0;
					case 75:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_75fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 75;
						return 0;
					case 72:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_72fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 72;
						return 0;

					case 60:
						memcpy(&lcd_cs8556_vga_parm.timing, &s_640_480_60fps_timing, sizeof(vpp_timing_t));
						lcd_cs8556_vga_parm.fps = 60;
						return 0;
				}
			}

			if(sta_timing[i].resx == 0 && sta_timing[i].resy == 0)
				break;
		}

		//default
		memcpy(&lcd_cs8556_vga_parm.timing, &s_1024_768_75fps_timing, sizeof(vpp_timing_t));
		lcd_cs8556_vga_parm.fps = 75;
		return 0;
	}//end if timing.vpixel

	if(timing.hpixel * timing.vpixel  > MAX_PIXEL_NUM)
	{
		aspect = get_aspect(timing.hpixel ,  timing.vpixel);
		switch(aspect)
		{
			case ASPECT_16_9:
				memcpy(&timing, &s_1280_720_60fps_timing, sizeof(vpp_timing_t));
			break;

			case ASPECT_16_10:
				memcpy(&timing, &s_1440_900_60fps_timing, sizeof(vpp_timing_t));
			break;

			case ASPECT_4_3:
				memcpy(&timing, &s_1280_960_60fps_timing, sizeof(vpp_timing_t));
			break;

			default:
				memcpy(&timing, &s_1280_1024_60fps_timing, sizeof(vpp_timing_t));
			break;
		}
	}

	memcpy(&lcd_cs8556_vga_parm.timing, &timing, sizeof(vpp_timing_t));

	lcd_cs8556_vga_parm.fps = fps;

	//vga_show_timing();
	return 0;
}

/************************** i2c device struct definition **************************/
static int __devinit cs8556_i2c_probe(struct i2c_client *i2c,
    const struct i2c_device_id *id)
{
    INFO("cs8556_i2c_probe\n");

    return 0;
}

static int __devexit cs8556_i2c_remove(struct i2c_client *client)
{
	INFO("cs8556_i2c_remove\n");

	return 0;
}


static const struct i2c_device_id cs8556_i2c_id[] = {
	{CS8556_NAME, 0},
	{ },
};
MODULE_DEVICE_TABLE(i2c, cs8556_i2c_id);

static struct i2c_board_info __initdata cs8556_i2c_board_info[] = {
    {
        I2C_BOARD_INFO(CS8556_NAME, CS8556_I2C_ADDR),
    },
};

static struct i2c_driver cs8556_i2c_driver = {
    .driver = {
    	.name = CS8556_NAME,
    	.owner = THIS_MODULE,
    },
    .probe = cs8556_i2c_probe,
    .remove = __devexit_p(cs8556_i2c_remove),
    .id_table = cs8556_i2c_id,
};

/*----------------------- Function Body --------------------------------------*/
static int tvformat_proc_write( struct file   *file,
                           const char    *buffer,
                           unsigned long count,
                           void          *data )
{
/*
    char value[20];
    int len = count;
	int ret;
	cs8556_output_format_t old_format;
	 
    if( len >= sizeof(value))
        len = sizeof(value) - 1;

    if(copy_from_user(value, buffer, len))
        return -EFAULT;

    value[len] = '\0';

	old_format = s_cs8556_output_format;
	
    if(!strnicmp(value, "ntsc", 4))
		s_cs8556_output_format = NTSC;
	else
		s_cs8556_output_format = PAL;

	if(s_cs8556_output_format != old_format)
	{
		//cs8556_disable();
		//vpp_init();
		cs8556_reconfig();
		switch(s_cs8556_output_format)
		{
			case NTSC:
				ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
				if(ret)
					ERROR("I2C write offset0 fail\n");

			break;

			default:
				ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
				if(ret)
					ERROR("I2C write offset0 fail\n");		
			break;
		}
	}
*/	
    return count;
}

static int tvformat_proc_read(char *page, char **start, off_t off, int count,
    int *eof, void *data) {
    
	int len;

	switch(s_cs8556_output_format)
	{
		case NTSC:
			len = sprintf(page, "%s\n", "NTSC");
		break;

		case PAL:
			len = sprintf(page, "%s\n", "PAL");
		break;
		
		default:
			len = sprintf(page, "%s\n", "VGA");
		break;
	}
	
	
	return len;
}

static struct proc_dir_entry * create_tvformat_proc_file(void)
{
    s_tvformat_proc_file = create_proc_entry(TVFORMAT_PROC_FILE, 0666, NULL);

    if(s_tvformat_proc_file != NULL )
    {
        s_tvformat_proc_file->write_proc = tvformat_proc_write;
		s_tvformat_proc_file->read_proc =  tvformat_proc_read;
    }
    else
        printk("[lcd-MYSON-CS8556]Can not create /proc/%s file", TVFORMAT_PROC_FILE);
	
    return s_tvformat_proc_file;
}

void lcd_cs8556_initial(void)
{
	int ret;
	//int i;
	unsigned char rbuf[256] = {0};
	struct i2c_adapter *adapter = NULL;
	char busNo[8] = {0};
	int len = 8;
	int num = 1;

	if(s_cs8556_init)
		return;

	s_cs8556_init = 1;

	if(wmt_getsyspara("wmt.cs8556.i2c", busNo, &len) == 0)
	{
	if(strlen(busNo) > 0)
		num = busNo[0] - '0';
	}

	adapter = i2c_get_adapter(num);

	if (adapter == NULL)
	{
		ERROR("Can not get i2c adapter, client address error\n");
		return;
	}

	s_cs8556_client = i2c_new_device(adapter, cs8556_i2c_board_info);
	if (s_cs8556_client == NULL)
	{
		ERROR("allocate i2c client failed\n");
		return;
	}
    i2c_put_adapter(adapter);

	ret = i2c_add_driver(&cs8556_i2c_driver);
	if (ret != 0)
	{
		ERROR("Failed to register CS8556 I2C driver: %d\n", ret);
		i2c_unregister_device(s_cs8556_client);
		return;
	}

	ret = I2CMultiPageRead(CS8556_I2C_ADDR, 0x00, 0x00, 256, rbuf);
	if(!ret)
	{
	/*
		INFO("CS8556 Read offset0 data as follows:\n");
		for(i = 0; i < 256;)
		{
			printk("0x%02X,", rbuf[i]);
			if((++i) % 16 == 0)
			printk("\n");
		}
	*/
	}
	else
	{
		ERROR("I2C address 0x%02X is not found\n", CS8556_I2C_ADDR);
		return;
	}

	switch(s_cs8556_output_format)
	{
		case VGA:
	if(memcmp(rbuf, s_RGB888_To_Dsub_Offset0, 0x10))
	{
		ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_Dsub_Offset0);
		if(ret)
		{
			ERROR("I2C write offset0 fail\n");
			return;
		}

		ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x01, 0x00, 256, s_RGB888_To_Dsub_Offset1);
		if(ret)
		{
			ERROR("I2C write offset1 fail\n");
			return;
		}
	}
		break;

		case NTSC:
			if(memcmp(rbuf, s_RGB888_To_NTSC_Offset0, 0x50))
			{
				ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
				if(ret)
				{
					ERROR("I2C write offset0 fail\n");
					return;
				}
			}
		break;

		default:
			if(memcmp(rbuf, s_RGB888_To_PAL_Offset0, 0x50))
			{
				ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
				if(ret)
				{
					ERROR("I2C write offset0 fail\n");
					return;
				}
			}			
		break;		
	}

	
	/*
	ret = I2CMultiPageRead(CS8556_I2C_ADDR, 0x01, 0x00, 256, rbuf);

	if(!ret)
	{

		INFO("Read offset1 data as follows:\n");
		for(i = 0; i < 256;)
		{
			printk("0x%02X,", rbuf[i]);
			if((++i) % 16 == 0)
				printk("\n");
		}

	}
	else
		return;
	*/

	if(s_cs8556_output_format == VGA)
		cs8556_vga_get_edid();

		create_tvformat_proc_file();

	return;
}

int cs8556_disable(void)
{
	int ret;

	if(s_cs8556_init == 0)
		return 0;

	ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 5, s_CS8556_Original_Offset0);
	if(ret)
	{
		ERROR("Suspend: I2C write offset0 fail\n");
		return -1;
	}

	return 0;
}

int cs8556_enable(void)
{
	int ret;

	if(s_cs8556_init == 0)
		return 0;

	switch(s_cs8556_output_format)
	{
		case VGA:
	ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_Dsub_Offset0);
	if(ret)
	{
		ERROR("Resume: I2C write offset0 fail\n");
		return -1;
	}

	ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x01, 0x00, 256, s_RGB888_To_Dsub_Offset1);
	if(ret)
	{
		ERROR("Resume: I2C write offset1 fail\n");
		return -1;
	}
		break;

		case NTSC:
			ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_NTSC_Offset0);
			if(ret)
			{
				ERROR("Resume: I2C write offset0 fail\n");
				return -1;
			}			
		break;

		default:
			ret = I2CMultiPageWrite(CS8556_I2C_ADDR, 0x00, 0x00, 256, s_RGB888_To_PAL_Offset0);
			if(ret)
			{
				ERROR("Resume: I2C write offset0 fail\n");
				return -1;
			}
		break;
	}

	return 0;
}

void cs8556_get_timing(vpp_timing_t *t)
{
	switch(s_cs8556_output_format)
	{
		case VGA:
			memcpy(t, &lcd_cs8556_vga_parm.timing, sizeof(vpp_timing_t));
		break;

		case NTSC:
			memcpy(t, &lcd_cs8556_ntsc_parm.timing, sizeof(vpp_timing_t));
		break;

		default:
			memcpy(t, &lcd_cs8556_pal_parm.timing, sizeof(vpp_timing_t));
		break;
	}	
}

vpp_timing_t *cs8556_get_timing2(void)
{
	switch(s_cs8556_output_format)
	{
		case VGA:
			return &lcd_cs8556_vga_parm.timing;
		break;

		case NTSC:
			return &lcd_cs8556_ntsc_parm.timing;
		break;

		default:
			return &lcd_cs8556_pal_parm.timing;
		break;
	}	

}

int cs8556_get_fps(void)
{
	switch(s_cs8556_output_format)
	{
		case VGA:
			return lcd_cs8556_vga_parm.fps;
		case NTSC:
			return lcd_cs8556_ntsc_parm.fps;
		default:
			return lcd_cs8556_pal_parm.fps;
	}	
}

cs8556_output_format_t cs8556_get_output_format(void)
{
	return s_cs8556_output_format;
} 

int cs8556_tvformat_is_set(void)
{
	return s_tvformat_is_set;
}

lcd_parm_t *lcd_cs8556_get_parm(int arg)
{
	switch(s_cs8556_output_format)
	{
		case VGA:
			return &lcd_cs8556_vga_parm;
		case NTSC:
			return &lcd_cs8556_ntsc_parm;
		default:
			return &lcd_cs8556_pal_parm;
	}
}

int lcd_cs8556_init(void)
{
	char varbuf[100];
	int varlen = 100;

	if(wmt_getsyspara("wmt.display.tvformat", varbuf, &varlen) == 0)
	{
		s_tvformat_is_set = 1;
		
		if(!strnicmp(varbuf, "ntsc", 4))
			s_cs8556_output_format = NTSC;
		else if(!strnicmp(varbuf, "pal", 3))
			s_cs8556_output_format = PAL;
		else if(!strnicmp(varbuf, "vga", 3))
			s_cs8556_output_format = VGA;
		else
			s_tvformat_is_set = 0;		
	}

	return lcd_panel_register(LCD_MYSON_CS8556,(void *) lcd_cs8556_get_parm);

} /* End of lcd_cs8556_init */
module_init(lcd_cs8556_init);

/*--------------------End of Function Body -----------------------------------*/
#undef LCD_MYSON_CS8556_C