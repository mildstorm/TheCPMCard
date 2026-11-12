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
	virtual void device_post_load() override ATTR_COLD;
	virtual void device_add_mconfig(machine_config &config) override ATTR_COLD;

	// overrides of standard a2bus slot functions
	virtual uint8_t read_cnxx(uint8_t offset) override;
	virtual void write_cnxx(uint8_t offset, uint8_t data) override;
	virtual void reset_from_bus() override;
	virtual void bus_irq_w(int state) override;

private:
	required_device<z80_device> m_z80;
	cpu_device *m_maincpu = nullptr;
	emu_timer *m_rtc_timer = nullptr;

	bool m_z80_owns_bus;                // U22 ownership toggle (card asserts /DMA)
	bool m_wait;                        // Z80 /WAIT
	uint8_t m_bank;                     // U27 bank latch (Z80 D0)
	bool m_m1_half;                     // accumulated half T-state of the M1 stretch
	bool m_tick;                        // U27 tick flip-flop (true = interrupt pending)
	bool m_apple_irq;                   // Apple /IRQ (true = asserted)
	std::unique_ptr<uint8_t[]> m_ram;   // 64K card DRAM
	uint8_t m_prom[0x100];              // U26 page translation PROM (upper half)

	// card DRAM is selected in bank 1 and always for $F000-$FFFF
	bool is_local(offs_t offset) const { return m_bank || offset >= 0xf000; }
	uint16_t z80_to_apple(offs_t offset) const { return (m_prom[(offset >> 8) ^ 0x80] << 8) | (offset & 0xff); }

	void set_wait(bool state);
	void toggle_owner();
	bool apple_bus_wait();
	void update_int();
	TIMER_CALLBACK_MEMBER(rtc_tick);
	int irq_ack(device_t &device, int irqline);

	uint8_t z80_r(offs_t offset);
	uint8_t z80_m1_r(offs_t offset);
	void z80_w(offs_t offset, uint8_t data);
	void z80_mem(address_map &map) ATTR_COLD;
	void z80_m1(address_map &map) ATTR_COLD;

	uint8_t z80_io_r(offs_t offset);
	void z80_io_w(offs_t offset, uint8_t data);
	void z80_io(address_map &map) ATTR_COLD;
};

// device type definition
DECLARE_DEVICE_TYPE(A2BUS_CPMCARD, a2bus_cpmcard_device)

#endif // MAME_BUS_A2BUS_A2CPMCARD_H
