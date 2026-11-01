// license:BSD-3-Clause
// copyright-holders:Antigravity
/*********************************************************************

    a2cpmcard.cpp

    Implementation of the Advanced Logic Systems (ALS) "The CP/M Card"

    6 MHz Z80B with 64K of on-card DRAM.

    Writing to $Cn00 toggles control between the 6502 and the Z80
    (the 6502 is suspended via DMA while the Z80 runs).

    Bank latch (Z80 OUT to port 00, written by the CP/M 3 BIOS SELMEM
    routine as F0 = bank 0 / FF = bank 1):

      bit 0 = 0 : system bank (also the state after reset)
                  Z80 $0000-$EFFF -> Apple II memory, Apple = Z80 + $9000
                  with the $Cxxx / $Fxxx pages swapped, i.e.
                    Z80 $0000-$2FFF -> Apple $9000-$BFFF
                    Z80 $3000-$3FFF -> Apple $F000-$FFFF
                    Z80 $4000-$5FFF -> Apple $D000-$EFFF
                    Z80 $6000-$6FFF -> Apple $C000-$CFFF (I/O)
                    Z80 $7000-$EFFF -> Apple $0000-$7FFF
      bit 0 = 1 : TPA bank
                  Z80 $0000-$EFFF -> card DRAM

      Z80 $F000-$FFFF is always card DRAM (common memory).

*********************************************************************/

#include "emu.h"
#include "a2cpmcard.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(A2BUS_CPMCARD, a2bus_cpmcard_device, "a2cpmcard", "ALS The CP/M Card")

#define Z80_TAG "z80"

void a2bus_cpmcard_device::z80_mem(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(a2bus_cpmcard_device::z80_r), FUNC(a2bus_cpmcard_device::z80_w));
}

void a2bus_cpmcard_device::z80_io(address_map &map)
{
	// The Z80 puts A on the upper address lines for OUT (n),A, so the
	// full 16-bit I/O space must be decoded (SELMEM uses OUT (00h),A).
	map(0x0000, 0xffff).rw(FUNC(a2bus_cpmcard_device::z80_io_r), FUNC(a2bus_cpmcard_device::z80_io_w));
}

uint8_t a2bus_cpmcard_device::z80_io_r(offs_t offset)
{
	return m_bank_latch;
}

void a2bus_cpmcard_device::z80_io_w(offs_t offset, uint8_t data)
{
	if ((offset & 0xff) == 0x00)
		m_bank_latch = data;
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void a2bus_cpmcard_device::device_add_mconfig(machine_config &config)
{
	// 6 MHz Z80B CPU
	Z80(config, m_z80, 6000000);
	m_z80->set_addrmap(AS_PROGRAM, &a2bus_cpmcard_device::z80_mem);
	m_z80->set_addrmap(AS_IO, &a2bus_cpmcard_device::z80_io);
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_cpmcard_device::a2bus_cpmcard_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_a2bus_card_interface(mconfig, *this),
	m_z80(*this, Z80_TAG),
	m_maincpu_space(nullptr),
	m_maincpu(nullptr),
	m_bEnabled(false),
	m_first_boot(true),
	m_bank_latch(0)
{
	memset(m_ram, 0, sizeof(m_ram));
}

a2bus_cpmcard_device::a2bus_cpmcard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	a2bus_cpmcard_device(mconfig, A2BUS_CPMCARD, tag, owner, clock)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void a2bus_cpmcard_device::device_start()
{
	save_item(NAME(m_bEnabled));
	save_item(NAME(m_first_boot));
	save_item(NAME(m_bank_latch));
	save_item(NAME(m_ram));

	m_maincpu = machine().device<cpu_device>("maincpu");
}

void a2bus_cpmcard_device::device_reset()
{
	m_bEnabled = false;
	m_first_boot = true;
	m_bank_latch = 0x00;
	m_z80->set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
}

void a2bus_cpmcard_device::reset_from_bus()
{
	m_z80->reset();
	device_reset();
}

uint8_t a2bus_cpmcard_device::read_cnxx(uint8_t offset)
{
	return 0x00;
}

void a2bus_cpmcard_device::write_cnxx(uint8_t offset, uint8_t data)
{
	if (!m_bEnabled)
	{
		m_bEnabled = true;
		raise_slot_dma();
		m_z80->set_input_line(INPUT_LINE_HALT, CLEAR_LINE);

		if (m_first_boot)
		{
			// first hand-off after reset: Z80 starts at $0000 (Apple $9000)
			m_first_boot = false;
			m_bank_latch = 0x00;
			m_z80->reset();
		}

		if (m_maincpu)
			m_maincpu->yield();
	}
	else
	{
		m_bEnabled = false;
		lower_slot_dma();
		m_z80->set_input_line(INPUT_LINE_HALT, ASSERT_LINE);
		m_z80->yield();
	}
}

// translate a Z80 address in the system bank to an Apple II address
static inline uint16_t z80_to_apple(uint16_t offset)
{
	uint16_t addr = (offset + 0x9000) & 0xffff;
	// swap the $Cxxx and $Fxxx pages
	if ((addr & 0xf000) == 0xc000)
		addr = (addr & 0x0fff) | 0xf000;
	else if ((addr & 0xf000) == 0xf000)
		addr = (addr & 0x0fff) | 0xc000;
	return addr;
}

uint8_t a2bus_cpmcard_device::z80_r(offs_t offset)
{
	if (offset >= 0xf000 || (m_bank_latch & 1))
		return m_ram[offset];

	return slot_dma_read(z80_to_apple(offset));
}

void a2bus_cpmcard_device::z80_w(offs_t offset, uint8_t data)
{
	if (offset >= 0xf000 || (m_bank_latch & 1))
	{
		m_ram[offset] = data;
		return;
	}

	uint16_t addr = z80_to_apple(offset);
	if ((addr >> 8) == (0xc0 + slotno()))
		write_cnxx(addr & 0xff, data);
	else
		slot_dma_write(addr, data);
}
