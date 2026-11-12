// license:BSD-3-Clause
// copyright-holders:Antigravity
/*********************************************************************

    a2cpmcard.cpp

    Implementation of the Advanced Logic Systems (ALS) "The CP/M Card"
    (PCB "DIGITAL RESEARCH / Advanced Logic Systems", 08-83,
    schematic "CP/M CARD" rev A, 05-04-83)

    Board:
      U15       Zilog Z0840006PSC (Z80B, 6 MHz)
      Y1        12.000 MHz crystal (Z80 clock = 12 MHz / 2)
      U2-U5,
      U7-U10    8x M5K4164S-15 (64K x 1 DRAM) = 64K card RAM
      U26       4K bipolar PROM (Raytheon R29623, 20 pin): Z80 -> Apple
                address translation, one entry per 256-byte page
      U20       74LS244 Z80 A0-A7 -> Apple A0-A7
      U16/U17   74LS373 / 74LS244 Apple data bus buffers
      U19/U25   74S257 DRAM row/column multiplexer
      U22       74S113 6502/Z80 ownership toggle + Z80 clock divider
      U27       74LS74 bank latch + 62 Hz tick flip-flop
      U1        4020 counter (Apple PHI1 / 2^14 = ~62 Hz tick)
      There is no firmware ROM on the card: everything is loaded from
      disk by the 6502.

    6502 <-> Z80 hand-off (U22, U28, U30, U21, U31):
      Every write to $Cnxx (/IOSEL and R/W low) toggles bus ownership,
      whether it comes from the 6502 or from the Z80 through DMA.
      While the Z80 owns the bus the card asserts /DMA.  The Z80 is never
      stopped as a whole: /WAIT is only asserted when it accesses Apple
      memory (not LOCAL) while the 6502 owns the bus, so it keeps running
      out of card DRAM.  The Z80 comes out of reset fetching $0000 of
      bank 0 (Apple $9000) and therefore waits until the first hand-off.
      Reads of $Cnxx are not decoded (floating bus).

    Memory map:
      bank 0 (D0 = 0, reset state): Z80 $0000-$EFFF -> Apple II memory
             through the U26 page translation PROM.
      bank 1 (D0 = 1): Z80 $0000-$EFFF -> card DRAM.
      Z80 $F000-$FFFF is card DRAM in both banks (common memory).

    Bank latch (U27, D = D0):
      Clocked by every Z80 I/O write (/IORQ and /WR), the port address
      is NOT decoded.  Cleared by /RESET.  The CP/M 3 BIOS SELMEM writes
      F0 (bank 0) / FF (bank 1) to port 00.

    Z80 I/O read (any port, U28 + U13):
      D0 = Apple /IRQ, D1-D7 = 1 (RP3 pull-ups).

    Real-time clock interrupt (U1, U27, U29, U13):
      U1 (4020, free running) divides Apple PHI1 by 2^14 (~62.3 Hz).
      Each tick clears the U27 flip-flop; Z80 /INT = tick pending OR
      Apple /IRQ (slot pin 30, wired-OR of every IRQ source).
      Interrupt acknowledge (/M1 + /IORQ) presets the flip-flop and U13
      drives D3 low, the rest of the bus floats high: the vector byte is
      $F7 = RST 30h in IM 0.  The CP/M 3.01C BIOS runs in IM 0 and
      installs its clock handler ($FB94: 60 ticks -> SCB seconds,
      minutes, hours, days) at both $0030 and $0038 of bank 1.

    Z80 clock (Y1, U6, U22, U28, U29, U14):
      U22 divides the 12 MHz oscillator by two.
      bank 1: U22 is clocked by S12M, the 12 MHz clock with one pulse
              removed by U6 on every opcode fetch (/M1 and /RAS, not
              refresh), so each M1 cycle is 4.5 T-states long.
      bank 0: U22 is clocked by the plain 12 MHz clock but its K input
              is gated with Apple PHI0, so the Z80 clock only runs while
              PHI0 is high (exactly 50% of every Apple cycle, long cycle
              included), i.e. 3 MHz on average.  The phase between Y1
              and the Apple 14M crystal is arbitrary, so the 0.5 clock
              scale is the exact deterministic equivalent.
      The M1 half T-state is accumulated: every second opcode fetch in
      bank 1 costs one extra T-state.

    U26 has not been dumped.  Its contents are synthesized from the
    behaviour of the CP/M 3.01C boot loader / BIOS:
        Z80 $0000-$2FFF -> Apple $9000-$BFFF
        Z80 $3000-$3FFF -> Apple $F000-$FFFF
        Z80 $4000-$5FFF -> Apple $D000-$EFFF
        Z80 $6000-$6FFF -> Apple $C000-$CFFF (I/O)
        Z80 $7000-$EFFF -> Apple $0000-$7FFF
    i.e. Apple = Z80 + $9000 with the $Cxxx / $Fxxx pages swapped.
    The PROM sees Z80 A15 through an inverter (U12), so its index is
    (Z80 A15-A8) ^ $80; pin 19 (A8 of the PROM) is tied high, so only
    the upper 256 bytes are used.

*********************************************************************/

#include "emu.h"
#include "a2cpmcard.h"

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

DEFINE_DEVICE_TYPE(A2BUS_CPMCARD, a2bus_cpmcard_device, "a2cpmcard", "ALS The CP/M Card")

#define Z80_TAG "z80"

// Apple PHI1: 14.31818 MHz / 14, with one long (16 x 14M) cycle per line of 65
static constexpr double APPLE_PHI1 = 14'318'181.8 * 65.0 / 912.0;

void a2bus_cpmcard_device::z80_mem(address_map &map)
{
	map(0x0000, 0xffff).rw(FUNC(a2bus_cpmcard_device::z80_r), FUNC(a2bus_cpmcard_device::z80_w));
}

void a2bus_cpmcard_device::z80_m1(address_map &map)
{
	map(0x0000, 0xffff).r(FUNC(a2bus_cpmcard_device::z80_m1_r));
}

void a2bus_cpmcard_device::z80_io(address_map &map)
{
	// no address decoding at all on the I/O side
	map(0x0000, 0xffff).rw(FUNC(a2bus_cpmcard_device::z80_io_r), FUNC(a2bus_cpmcard_device::z80_io_w));
}

//-------------------------------------------------
//  device_add_mconfig - add device configuration
//-------------------------------------------------

void a2bus_cpmcard_device::device_add_mconfig(machine_config &config)
{
	// Z80B clocked from the 12 MHz crystal / 2
	Z80(config, m_z80, 12_MHz_XTAL / 2);
	m_z80->set_memory_map(&a2bus_cpmcard_device::z80_mem);
	m_z80->set_m1_map(&a2bus_cpmcard_device::z80_m1);
	m_z80->set_io_map(&a2bus_cpmcard_device::z80_io);
	m_z80->set_irq_acknowledge_callback(FUNC(a2bus_cpmcard_device::irq_ack));
}

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

a2bus_cpmcard_device::a2bus_cpmcard_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, uint32_t clock) :
	device_t(mconfig, type, tag, owner, clock),
	device_a2bus_card_interface(mconfig, *this),
	m_z80(*this, Z80_TAG),
	m_z80_owns_bus(false),
	m_wait(false),
	m_bank(0),
	m_m1_half(false),
	m_tick(false),
	m_apple_irq(false)
{
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
	m_ram = std::make_unique<uint8_t[]>(0x10000);
	std::fill_n(m_ram.get(), 0x10000, 0);

	// synthesize the (undumped) U26 page translation PROM, see header
	for (int page = 0; page < 0x100; page++)
	{
		uint8_t apple = (page + 0x90) & 0xff;
		if ((apple & 0xf0) == 0xc0)
			apple = (apple & 0x0f) | 0xf0;
		else if ((apple & 0xf0) == 0xf0)
			apple = (apple & 0x0f) | 0xc0;
		m_prom[page ^ 0x80] = apple;    // PROM sees /A15
	}

	m_maincpu = machine().device<cpu_device>("maincpu");

	// U1 4020 Q14, free running from Apple PHI1
	m_rtc_timer = timer_alloc(FUNC(a2bus_cpmcard_device::rtc_tick), this);
	const attotime q14 = attotime::from_hz(APPLE_PHI1 / 16384.0);
	m_rtc_timer->adjust(q14, 0, q14);

	save_item(NAME(m_z80_owns_bus));
	save_item(NAME(m_wait));
	save_item(NAME(m_bank));
	save_item(NAME(m_m1_half));
	save_item(NAME(m_tick));
	save_item(NAME(m_apple_irq));
	save_pointer(NAME(m_ram), 0x10000);
}

void a2bus_cpmcard_device::device_post_load()
{
	m_z80->set_clock_scale(m_bank ? 1.0 : 0.5);
}

void a2bus_cpmcard_device::device_reset()
{
	m_z80_owns_bus = false;
	m_bank = 0;
	m_m1_half = false;
	m_tick = false;
	m_z80->set_clock_scale(0.5);
	set_wait(false);
	update_int();
}

void a2bus_cpmcard_device::reset_from_bus()
{
	// Apple /RESET is wired to the Z80 /RESET
	m_z80->reset();
	device_reset();
}

//-------------------------------------------------
//  bus ownership
//-------------------------------------------------

void a2bus_cpmcard_device::set_wait(bool state)
{
	if (state != m_wait)
	{
		m_wait = state;
		m_z80->set_input_line(Z80_INPUT_LINE_WAIT, state ? ASSERT_LINE : CLEAR_LINE);
	}
}

void a2bus_cpmcard_device::toggle_owner()
{
	m_z80_owns_bus = !m_z80_owns_bus;

	if (m_z80_owns_bus)
	{
		raise_slot_dma();
		set_wait(false);

		// the 6502 stops right away (RDY), let the Z80 catch up
		if (m_maincpu)
			m_maincpu->yield();
	}
	else
	{
		// the Z80 runs on until its next Apple bus access
		lower_slot_dma();
	}
}

// true if the access has to wait for the Apple bus (and will be redone)
bool a2bus_cpmcard_device::apple_bus_wait()
{
	if (m_z80_owns_bus || machine().side_effects_disabled())
		return false;

	set_wait(true);
	m_z80->defer_access();
	return true;
}

uint8_t a2bus_cpmcard_device::read_cnxx(uint8_t offset)
{
	// no ROM on the card and reads of $Cnxx are not decoded
	return get_open_bus();
}

void a2bus_cpmcard_device::write_cnxx(uint8_t offset, uint8_t data)
{
	toggle_owner();
}

//-------------------------------------------------
//  Z80 memory
//-------------------------------------------------

uint8_t a2bus_cpmcard_device::z80_r(offs_t offset)
{
	if (is_local(offset))
		return m_ram[offset];

	if (apple_bus_wait())
		return 0xff;

	return slot_dma_read(z80_to_apple(offset));
}

uint8_t a2bus_cpmcard_device::z80_m1_r(offs_t offset)
{
	// bank 1: U6 removes one 12 MHz pulse per opcode fetch (+0.5 T)
	if (m_bank && !machine().side_effects_disabled())
	{
		m_m1_half = !m_m1_half;
		if (!m_m1_half)
			m_z80->adjust_icount(-1);
	}

	return z80_r(offset);
}

void a2bus_cpmcard_device::z80_w(offs_t offset, uint8_t data)
{
	if (is_local(offset))
	{
		m_ram[offset] = data;
		return;
	}

	if (apple_bus_wait())
		return;

	uint16_t addr = z80_to_apple(offset);
	if ((addr >> 8) == (0xc0 + slotno()))
		write_cnxx(addr & 0xff, data);  // our own /IOSEL, decoded by the Apple
	else
		slot_dma_write(addr, data);
}

//-------------------------------------------------
//  Z80 I/O
//-------------------------------------------------

uint8_t a2bus_cpmcard_device::z80_io_r(offs_t offset)
{
	// U13 puts Apple /IRQ on D0, RP3 pulls the rest of the bus high
	return m_apple_irq ? 0xfe : 0xff;
}

void a2bus_cpmcard_device::z80_io_w(offs_t offset, uint8_t data)
{
	// U27 latches D0 on every I/O write, regardless of the port
	const uint8_t bank = BIT(data, 0);
	if (bank != m_bank)
	{
		m_bank = bank;
		// bank 0 gates the Z80 clock with Apple PHI0 (half speed)
		m_z80->set_clock_scale(m_bank ? 1.0 : 0.5);
	}
}

//-------------------------------------------------
//  interrupts
//-------------------------------------------------

void a2bus_cpmcard_device::update_int()
{
	m_z80->set_input_line(INPUT_LINE_IRQ0, (m_tick || m_apple_irq) ? ASSERT_LINE : CLEAR_LINE);
}

void a2bus_cpmcard_device::bus_irq_w(int state)
{
	m_apple_irq = state != CLEAR_LINE;
	update_int();
}

TIMER_CALLBACK_MEMBER(a2bus_cpmcard_device::rtc_tick)
{
	// U1 Q14 clocks a 0 into the U27 tick flip-flop
	m_tick = true;
	update_int();
}

int a2bus_cpmcard_device::irq_ack(device_t &device, int irqline)
{
	// INTAK presets the tick flip-flop, U13 drives D3 low: RST 30h
	m_tick = false;
	update_int();
	return 0xf7;
}
