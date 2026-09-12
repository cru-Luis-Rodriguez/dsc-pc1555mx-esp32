# Using a multimeter — for this project, from zero

Written for someone who has never used one. Everything in `panel-bringup.md` refers
back to the numbered procedures here (**P1**–**P4**), so read this once first.

Nothing in this project involves dangerous voltages **except the wall plug on the
transformer**. The panel side is 12–19 V, which you can touch without harm. The risk
here is not injury, it's misreading — and then throwing out a working panel.

---

## 0. Two bits of naming that trip people up

**`P1`–`P4` are procedure numbers in this document. They are not places on the panel.**
When another doc says "use P1", it means "set the meter up for DC volts as described in
section 5 here." Nothing on the hardware is labeled P1.

**`COM` means two different things** in this project:

| Where | What it is |
|---|---|
| **On the multimeter** | The socket the **black probe plugs into**. This is the one meant whenever these docs say "black probe → `COM`". |
| **On the alarm panel** | The zone common terminals (`Z1 COM Z2 Z3 COM Z4 …`). Only used in the Stage 1b zone-loop test. |

They are unrelated. For every voltage measurement, `COM` means the meter's socket.

---

## 0b. This project's meter — Southwire 21005N

| | |
|---|---|
| Type | AC clamp meter with DMM functions |
| Ranging | **Auto-ranging** — ignore every "set the range to 20" instruction below and in the other docs; it picks the scale itself |
| Jacks | **Only two**: `COM` (black lead) and `V` (red lead) |
| DC volts | Dial position **`V⎓`** — V with a straight line over dashes |
| AC volts | Dial position **`V∼`** — V with a wavy line |
| Resistance / continuity | The `Ω` position, same two jacks |
| Current | Measured through the **clamp jaw**, not the probes |

**The 10 A jack hazard in §1 does not apply to this meter** — it has no current jack,
because current goes through the clamp. One less way to go wrong.

**Remembering which V is which:** AC alternates, so it's drawn as a **wave** (`∼`).
DC is steady, so it's drawn as a **straight line** (`⎓`). Flat means DC.

---

## 1. Your meter has three parts

**The display.** Shows a number and a unit. The unit matters enormously — `13.80 V`
and `13.80 mV` differ by a factor of a thousand. Always read the letters, not just the
digits.

**The dial.** Selects *what* you're measuring. You will only ever need four positions
for this project. See section 3.

**The jacks (holes) at the bottom.** Usually three:

| Jack | Marking | Use |
|---|---|---|
| Left or bottom | `COM` | **Black probe goes here. Always. Every test.** |
| Middle | `VΩmA`, `V/Ω/mA`, or `mAVΩ` | **Red probe goes here for every test in this project.** |
| The other one | `10A`, `20A`, or `A` | High current only. **Never use it here.** |

> ⚠️ **The one way to damage something.** If the red probe is in the `10A` jack and you
> touch the probes across a power supply, you create a dead short through the meter.
> That blows the meter's internal fuse and can damage what you're testing. Look at
> where your red probe is plugged in before every measurement. It belongs in the
> **middle** jack for all of P1–P4.

---

## 2. Prove the meter works before you trust it

**Do this first, every session.** It is the single most useful habit for a beginner,
because it separates "the panel is dead" from "my meter is set up wrong" — and you
cannot tell those apart from a reading of zero.

1. Black probe in `COM`, red probe in the middle jack.
2. Dial to **DC volts** (see below), range `20` if your meter has numbered ranges.
3. Touch the probes to the ends of any fresh battery — an AA, or a 9 V.
4. You should read about `1.5` for an AA, or `9` for a 9 V battery.

If you get `0`, or nothing changes, then *something about your setup is wrong* — probe
in the wrong jack, dial in the wrong place, dead meter battery, or a blown fuse. Fix it
now. Do not go measure the panel and conclude it's dead.

Then check continuity: dial to the `•))` position and **touch the two probe tips
together**. It should beep and read near `0`. If it doesn't beep, your meter may not
have that feature — use resistance (P3) instead and look for a reading near zero.

---

## 3. The four dial positions you need

Markings vary by manufacturer. Find yours:

| What you need | Dial says | Symbol |
|---|---|---|
| **DC volts** — batteries, the panel's 13.8 V | `DCV`, `V⎓`, or `V` with a solid line over dashes | ⎓ |
| **AC volts** — the transformer output | `ACV`, `V~`, or `V` with a wavy line | ∼ |
| **Resistance (ohms)** — zone loops | `Ω` | Ω |
| **Continuity** — is this wire connected? | `•))` — looks like sound waves, often shares the `Ω` position | •)) |

**Some meters combine AC and DC into one `V` position** with a separate button
(often yellow or blue, marked `SELECT`, `MODE`, or `AC/DC`) to switch between them.
If yours works that way, watch the display — it will show a small `DC` or `AC`
indicator telling you which mode you're in. Getting this wrong is a common beginner
error: **measuring AC volts while set to DC gives a meaningless reading near zero**,
which looks exactly like a dead transformer.

### Auto-ranging vs manual ranging

**Auto-ranging meter** — the dial just says `V⎓` with no numbers. The meter picks the
scale itself. Easier. Just read the display and its unit.

**Manual-ranging meter** — the dial has numbers around each position: `200m`, `2`,
`20`, `200`, `600`. You choose. **Rule: pick the smallest number that is still larger
than what you expect to read.**

For this project:

| Measuring | Expect | Use range |
|---|---|---|
| Panel Keybus DC voltage | ~13.8 V | `20` (DC volts) |
| Transformer AC output | ~16–19 V | `200` (AC volts) |
| Zone loop resistance | ~5,600 Ω | `20k` |
| A battery, for the self-test | 1.5–9 V | `20` (DC volts) |

If the display shows `OL` or a lone `1`, your range is too small — go up one.

---

## 4. Reading the display

| Display shows | Means |
|---|---|
| A normal number | That's your reading. Check the unit. |
| `OL`, `.OL`, `----`, or a lone `1` on the left | **Over range / open circuit.** On resistance, this means *no connection* — infinite ohms. Not a broken meter. |
| `0.00` or very near zero on resistance | A dead short — the two points are directly connected. |
| A **negative** number on DC volts | Your probes are backwards. Harmless. Swap them, or just ignore the minus sign — the magnitude is correct. |
| `mV` in the unit | Millivolts. `250 mV` is 0.25 V, not 250 V. |
| `kΩ` in the unit | Thousands of ohms. `5.60 kΩ` is 5,600 Ω — exactly what a healthy zone loop reads. |
| A number that drifts and won't settle | Poor probe contact, or a corroded connection in what you're testing. Press harder and retest. |

---

## 5. The procedures

### P1 — Measuring DC volts

Used for: the panel's Keybus voltage, battery voltage, the buck converter output.

1. Black probe → `COM`. Red probe → middle jack.
2. Dial → **DC volts**. Manual meters: range `20`.
3. Touch the **black** probe to the negative / ground / `−` side.
4. Touch the **red** probe to the positive / `+` side.
5. Hold both firmly still and read the display.

Screw terminals need real contact. Touch the probe tip to the **metal screw head or
the bare wire**, not the plastic body of the terminal block. If the reading jumps
around, you don't have contact.

DC has polarity, which is why black-to-negative matters. If you get it backwards you
just see a minus sign.

### P2 — Measuring AC volts

Used for: the transformer output only.

1. Black probe → `COM`. Red probe → middle jack.
2. Dial → **AC volts**. Manual meters: range `200`.
3. **AC has no polarity** — either probe on either of the two leads.
4. Read the display.

If you get near zero and expected ~16 V, check that you are in **AC** mode and not DC
before concluding the transformer is dead.

### P3 — Measuring resistance (ohms)

Used for: characterizing the zone loops.

> ⚠️ **The circuit must be dead, and the thing you're measuring must be disconnected.**
> Measuring resistance works by pushing a tiny current out of the meter's own battery.
> If the circuit is powered, the reading is garbage and you can damage the meter. If
> the component is still wired into a circuit, you're measuring the whole circuit, not
> the component.

1. **Unplug the transformer. Disconnect the battery.** Confirm the panel is dead by
   measuring its Keybus with P1 — you want `0`.
2. Black probe → `COM`. Red probe → middle jack.
3. Dial → `Ω`. Manual meters: range `20k`.
4. Touch one probe to each of the two points.
5. Read. Resistance has no polarity; probe order doesn't matter.

**Hold the probes by their plastic grips.** At high resistances, your body across the
metal tips becomes part of the circuit and skews the reading.

### P4 — Checking continuity

Used for: "is this wire actually connected end to end?"

1. Panel dead, as in P3.
2. Dial → `•))`.
3. **Touch the probe tips together to confirm it beeps.** Always verify this first.
4. Put one probe at each end of the wire or path you're testing.
5. **Beep** = connected. **Silence** = broken.

Continuity is just a fast pass/fail version of P3. When the answer is ambiguous, use
P3 and read the actual number.

---

## 6. Mistakes to avoid

- **Red probe in the `10A` jack while measuring voltage.** The one genuinely damaging
  error. Check before every measurement.
- **Set to DC while measuring AC**, or the reverse. Gives a near-zero reading that
  looks like dead hardware.
- **Measuring resistance on a live circuit.** Wrong numbers, possible meter damage.
- **Not proving the meter first.** A flat meter battery reads zero on everything and
  looks exactly like a dead panel.
- **Ignoring the unit.** `mV` vs `V`, `Ω` vs `kΩ`.
- **Probing the plastic instead of the metal.** No contact, unstable reading.
- **Probing the transformer's wall plug.** That side is line voltage. Only ever measure
  the two low-voltage output leads.
- **Concluding from a single reading.** If something looks wrong, re-prove the meter on
  a battery, then measure again.
