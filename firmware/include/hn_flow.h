/*
 * hn_flow.h - the flow switch on J1, read two ways at once.
 *
 * The switch is a dry contact between the flow node and ground. The board gives
 * that node a 1 Mohm pull-up to BATT+, a 100 nF filter, and TWO paths to the
 * MCU: D5 through 100 ohm (digital) and A2 through 330 ohm (analogue).
 *
 * Reading it as a logic level answers "open or closed". Reading the same node
 * as a voltage answers something the logic level cannot: whether the node is
 * sitting at a rail at all. Water across the contacts, a corroded pin or a
 * chafed cable presents a resistance rather than a short, and puts the node in
 * mid-air where a digital read still returns a confident - and wrong - answer.
 *
 * One consequence of the board's RC has to be handled in firmware: the node
 * takes ~120 ms to climb back through the 1 Mohm pull-up after the contact
 * opens, and it is genuinely at mid-rail for that whole time. A single look
 * would report a normal end-of-fill as a resistive fault. So a mid-rail result
 * is re-read once after the node has had time to settle, and only a second
 * mid-rail result counts - a transient passes, a fault does not.
 *
 * WHAT THIS CANNOT DO, and why it is reported honestly rather than guessed:
 * an open dry contact and an unplugged connector are the same circuit. There is
 * no measurement that separates them, so a clean "open" is reported as
 * HN_PRESENCE_UNCONFIRMED, not as "sensor present". A closed contact, or a
 * fault voltage, is positive proof the harness is there. Fixing this properly
 * is a hardware change - an end-of-line resistor in the switch housing - and it
 * is recorded in docs/HARDWARE.md.
 */
#ifndef HN_FLOW_H
#define HN_FLOW_H

#include "hn_reading.h"

void hn_flow_begin();
void hn_flow_read(hn_flow_reading_t &r);

#endif /* HN_FLOW_H */
