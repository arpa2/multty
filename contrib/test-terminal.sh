#!/bin/bash
#
# Test codes on a terminal.  They are not usually shown.
#
# From: Rick van Rein <rick@openfortress.nl>


# MulTTY escapes are needed for:
#
# NUL, SOH, STX, ETX, EOT, ENQ, ACK, SO, SI, DLE, DC1, DC2,
# DC3, DC4, NAK, SYN, ETB, CAN, EM, FS, GS, RS, US, DEL.

LETRS=( '41' '42' '43' '44' )
CODES=( '00' '01' '02' '03' '04' '05' '06' '0e' '0f' '10' '11' '12' '13' '14' '15' '16' '17' '18' '19' '1c' '1d' '1e' '1f' '7f' )
DIGTS=( '30' '31' '32' '33' )

for HEX in ${LETRS[@]} ${CODES[@]} ${DIGTS[@]}
do
	printf "Hex code 0x${HEX} prints >>\\x${HEX}<<\\n"
done
