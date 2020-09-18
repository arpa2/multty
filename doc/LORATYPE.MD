# LoreType: TTY over LoRaWAN

> *Piecing a screen together like a jigsaw puzzle.
> Aim to be so compact to accommodate an MTU.
> This means that we can be as small as desired,
> but might expand in later messages.  We require
> compaction to accommodate bandwidth constraints.*

The LoRaWAN translates an application's pseudoterminal into JigSee coding as described below; together with a terminal this mimics a terminal, like an xterm in 2, 16 or 256 colours.  The JigSee material is pretty much compatible with rendering the terminal, but it is more compact and greatly simplifies the work for the terminal.

LoRaWAN features:

  * Terminals may detach/reattach their LoRaWAN channel
  * Flexible adaptation to low MTU of LoRaWAN channels
  * Low traffic rates to match 1% budgets of LoRaWAN
  * Suggest listen-then-send for better interactivity
  * Designed for end-to-end encryption of LoRaWAN
  * LoRaWAN ports can each run a different terminal

Terminal features:

  * Detachable terminal service on remote servers
  * Colour modes for monochrome, 16 and 256 colours
  * Attributes for bold, underline, italics, blinking
  * Any terminal size, may be dynamically resized
  * Mouse support for signaling and displaying
  * Scrolling support for signaling and displaying
  * Eventually-correct updates to character data
  * Handling options for reverse video and faint print

**Positions:** Imagine a display at 80x25.  Positions take ceil(log2(80x25)) bits or 11 bits (look below for 14 or more extra instructions).  For 160x50 it'd be 13 bits (including 14 or more extra instructions).  Adding 3 bits and rounded up to a number of bytes, we now have the size of a big-endian integer for a position on the screen to work at.  So, 13 bits nicely round up to 16 bits and since ASCII codes 0x00-0x1f are meaningless, they end up starting the next position.  No such position at the start of a write indicates continuation from where we were before, and works best after recent acknowledged screen updating.  For 11 bits in 80x25 displays, we add 3 to find 14.  We therefore initiate positioning sequences with 0x00-0x07 in that case; we might use that cleverly for something more direct; but it ought to be more useful than a position change.

**OverlayCRC32:** Compute a CRC32 for various ranges and overlay them.  Or for characters, and refresh the range that overlaps.  Overlap can be in 2D (or more?) like 8x4 to cover rectangles with the 32 bits.  For 80x25 or 160x50 displays we'd have as many bits, so 400 or 1600 bits.  We can compress by overlapping CRC32 codes in the same place (and then would have to refresh them at the same time).  This yields regions to request, which is compact and makes sense.  Assuming wrap-around for the CRC codes, 100 bits, would be usable for multiples of 2x2 rectangles from 80x25 and 4x4 from 160x50.  In 25 bits, we'd be asking 4x4 or 8x8 rectangles.  We may also hash these regions as a whole instead of overlaying their character hashes at the same position.  To improve the chances of proper displaying, we can grab a seed from somewhere that we hash into the CRC32 checksum before the actual character data.  We can inquire so often that we gain complete certainty.  *This can be as compact as our channel wants us to be.*

**CharData:** Characters have a UTF-8 code, and are sent as such.  Non-interfering codes follow; characters inherit the current display attributes and if not monochrome they have a foreground and background colour in 16 or 256 colours.  After clearing, inserting or noting a portion of the screen, the cursor is at the start of that position, and wraps at its end.  This includes scrolling.  An absolute position forgets this portioning of the screen and the whole width counts as the active column.

**CharMove:** After a character is displayed, the position moves to the next position.  At the end of the line, printing continues on the next line in the beginning of the active column.  Beyond the last line, the screen jumps to the home position and active full width.  Codes 0x00-0x1f also end printing and column restriction, and the code gives the highest 5 bits for a new position/instuction.

**CharColour2:** Prefix unexpected UTF-8 top bit 10 to a bit that sets reverse video, one bit to set faint display, and 4 attribute bits in the low half.  [Uses 1 byte for attr/colour changes.]

**CharColour16:** Prefix unexpected UTF-8 top bits 10 to a selector 00 (4 bit attrs in low half, a byte follows with 4 bit bg/high and 4 bit fg/low), 01 (4 bit attrs in low half), 10 (4 bit foreground colour in low half), 11 (4 bit background colour in low half).  [Uses 1-2 bytes for attr/colour changes.]

**CharColour256:** Prefix unexpected UTF-8 top bits 10 to flags fg-byte-follows (1st if 2 options) and bg-byte-follows (2nd of 2 options), then 4 bits of attr value.  [Uses 1-3 bytes for attr/colour changes.]

**Attribute bits:** We have 4 bits in the encodings for commonly seen attributes bold, italics, underline, blink.  Note that reverse video can be implemented on the server by swapping colours; faint can also be implemented as a colour effect; the only exception is CharColour2, for which these bits are part of its specific encoding.  Blinking is generally a nuisance, but less disruptive if it gently dims and brightens the colours.  The cursor and/or mouse pointer are not necessarily drawn this way, that is a local choice.

**VarSized:** It is possible to send character data with size variations (colour changes, UTF-8) if we simply define a column and start writing in it for as far as we can get.  Columns can be any width, and start at any position.  So, we might exploit codes 0x00-0x1f to move back so many positions on the following line, relative to the last char written.

**MouseData:** Involves a current position along with button presses or wheel motions.  May switch off.  Remote FrameBuffer sends a position and a button event.  We have 3 bits for 3 buttons in the beginning of a position (if context tells us that this is mouse data).  This might also be sent as a key sequence instead of using a special format.

**ScrollData:** Scrolling is not necessarily a mouse event, but it may be.  It can be requested in terms of lines up/down, as a key sequence.  The application may respond by scrolling the screen up or down in a "position" instruction.

**WindowResize:** This is another change of the terminal that can be relayed as a key sequence.

**Instructions:** An update of the screen may involve instructions, such as scroll up/down/left/right by N positions (4 or 4N or Nu+Nd+Nl+Nr); such as clearing the distance between the last two positions inclusive (1), the whole current line (1) or preceding/remaining line (2), removing the current line shifting content up/down (2) or making the current line empty inserting a line shifting up/down (2); such as having or not having a status line (2).  These 14 or more instructions may be mixed into the codes for positions, which are already a ceil(log2()) and therefore somewhat open for extra codes.  The number of scroll positions (the N-factor) can be used for filling up leftover space.  Example: 80x25 needs 11 bits, and leaves 48 values.  No extra bit is needed because the minimum of 14 is covered.  The remaining 34 codes are split into 8 multi-scroll codes and 2 extra, assigned in the order up, down, left, right.  So we can move up or down by 1 to 9 positions and left or right by 1 to 8 positions.  Implementation of these filler codes: subtract the offset for "1 up" and then the lower 2 bits represent 00 up, 01 down, 10 left, 11 right and find N-1 in the higher bits.  Note that the other codes may be negative fixed values after the same subtraction, that can be pleasant for coding.  **TODO:** Perhaps explicitly indicate changes-done annex query OverlayCRC32, possibly a variant restricted to the inclusive region between the last two positions.

**SendKeys:** Terminal sends keys or mouse actions along with a screen hash, and makes it fit into a single LoRaWAN MTU.  May repeat with no keys until it is pretty sure that it got all.  Look at Remote FrameBuffer for encoding ideas.

**SendScreen:** Application sends screen whenever it updates, and marks where terminal lags behind.  Lagging may be due to being unresponsive or because keys were sent with a wrong idea of the screen makeup.  There is no principal coordination between SendKeys and SendScreen.  There is no default screen colour/attributes byte, just the last thing in use.  One position may be assigned as a cursor for the mouse, another as the screen cursor.  Rendering is local, but either may be off.

**Dirty:** Application sets a character position as dirty if any of the character, colour or attributes changes.  This flag is reset when the character is sent and acknowledged as having arrived.  When scheduling a sending sequence, it may want to set a cleansed flag to stave off any further dirty notifications that might result from writing close to the character, but not on the character itself.  On the other hand, it would be simpler to completely forbid and ignore dirty notifications by asking not to send it while parts still need to be written.  The last write would have to set a flag that the server thinks it's done because all dirty flags have been written out.

**Upstream:** SendKeys with as many keys as will fit, possibly followed by MTU-friendly OverlayCRC32.  Both parts are optional.  A key sequence switches to the OverlayCRC32 material.  Mouse motions, scrolling and window resizing are all sent as key sequences, for instance as escape sequences.  When just connected, the terminal demands a usage mode, including CharColour mode, which attributes are supported, whether scrolling, erasing, insert/removing or a status line is available and what the (initial) character layout is.  No codes will be allocated for absent functions.

**Downstream:** Any sequential mixture of Position/Instruction, CharColour/Attribute and UTF-8 characters.  Each is recognised by its special initiation codes.  There are no escape sequences to interpret in downstream direction.  **TODO:** Mouse is locally handled, but how about cursor position and visibility?
