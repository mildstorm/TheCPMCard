// license:BSD-3-Clause
// copyright-holders:Antigravity
/*********************************************************************

    a2cpmcard.h

    Implementation of the Advanced Logic Systems (ALS) "The CP/M Card"

*********************************************************************/

#ifndef MAME_BUS_A2BUS_A2CPMCARD_H
#define MAME_BUS_A2BUS_A2CPMCARD_H

#pragma once

#include "a2bus.h"
#include "cpu/z80/z80.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

class a2bus_cpmcard_device:
	public device_t,
	public device_a2bus_card_interface
{
public:
	// construction/destruction
	a2bus_cpmcard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

protected:
	a2bus_cpmcard_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock);

	virtual void device_start() override ATTR_COLD;
	virtual void device_reset() override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	// overrides of standard a2bus slot functions
	virtual uint8_t read_cnxx(uint8_t offset) override;
	virtual void write_cnxx(uint8_t offset, uint8_t data) override;
	virtual void reset_from_bus() override;

private:
	required_device<z80_device> m_z80;
	address_space *m_maincpu_space;
	cpu_device *m_maincpu;

	bool m_bEnabled;
	bool m_first_boot;
	uint8_t m_bank_latch;
	uint8_t m_ram[0x10000];

	uint8_t z80_r(offs_t offset);
	void z80_w(offs_t offset, uint8_t data);
	void z80_mem(address_map &map) ATTR_COLD;

	uint8_t z80_io_r(offs_t offset);
	void z80_io_w(offs_t offset, uint8_t data);
	void z80_io(address_map &map) ATTR_COLD;

	void hook_maincpu_0303(offs_t offset);
};

// device type definition
DECLARE_DEVICE_TYPE(A2BUS_CPMCARD, a2bus_cpmcard_device)

#endif // MAME_BUS_A2BUS_A2CPMCARD_H
