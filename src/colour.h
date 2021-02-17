/* Generated colour table.  Edits may be lost.
 *
 * To pick a colour, use colour(N)
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef colour_table_type
#define colour_table_type int
#endif

colour_table_type colour_table [] = {
	196,	// [38;5;196mANSI code 196 for R=5[0m
	 46,	// [38;5;46mANSI code  46 for G=5[0m
	 21,	// [38;5;21mANSI code  21 for B=5[0m
	214,	// [38;5;214mANSI code 214 for R=5,G=3[0m
	199,	// [38;5;199mANSI code 199 for R=5,B=3[0m
	 49,	// [38;5;49mANSI code  49 for G=5,B=3[0m
	220,	// [38;5;220mANSI code 220 for R=5,G=4[0m
	200,	// [38;5;200mANSI code 200 for R=5,B=4[0m
	 50,	// [38;5;50mANSI code  50 for G=5,B=4[0m
	208,	// [38;5;208mANSI code 208 for R=5,G=2[0m
	198,	// [38;5;198mANSI code 198 for R=5,B=2[0m
	 48,	// [38;5;48mANSI code  48 for G=5,B=2[0m
	124,	// [38;5;124mANSI code 124 for R=3[0m
	 34,	// [38;5;34mANSI code  34 for G=3[0m
	 19,	// [38;5;19mANSI code  19 for B=3[0m
	148,	// [38;5;148mANSI code 148 for R=3,G=4[0m
	128,	// [38;5;128mANSI code 128 for R=3,B=4[0m
	 38,	// [38;5;38mANSI code  38 for G=3,B=4[0m
	136,	// [38;5;136mANSI code 136 for R=3,G=2[0m
	126,	// [38;5;126mANSI code 126 for R=3,B=2[0m
	 36,	// [38;5;36mANSI code  36 for G=3,B=2[0m
	154,	// [38;5;154mANSI code 154 for R=3,G=5[0m
	129,	// [38;5;129mANSI code 129 for R=3,B=5[0m
	 39,	// [38;5;39mANSI code  39 for G=3,B=5[0m
	160,	// [38;5;160mANSI code 160 for R=4[0m
	 40,	// [38;5;40mANSI code  40 for G=4[0m
	 20,	// [38;5;20mANSI code  20 for B=4[0m
	172,	// [38;5;172mANSI code 172 for R=4,G=2[0m
	162,	// [38;5;162mANSI code 162 for R=4,B=2[0m
	 42,	// [38;5;42mANSI code  42 for G=4,B=2[0m
	190,	// [38;5;190mANSI code 190 for R=4,G=5[0m
	165,	// [38;5;165mANSI code 165 for R=4,B=5[0m
	 45,	// [38;5;45mANSI code  45 for G=4,B=5[0m
	178,	// [38;5;178mANSI code 178 for R=4,G=3[0m
	163,	// [38;5;163mANSI code 163 for R=4,B=3[0m
	 43,	// [38;5;43mANSI code  43 for G=4,B=3[0m
	 88,	// [38;5;88mANSI code  88 for R=2[0m
	 28,	// [38;5;28mANSI code  28 for G=2[0m
	 18,	// [38;5;18mANSI code  18 for B=2[0m
	118,	// [38;5;118mANSI code 118 for R=2,G=5[0m
	 93,	// [38;5;93mANSI code  93 for R=2,B=5[0m
	 33,	// [38;5;33mANSI code  33 for G=2,B=5[0m
	106,	// [38;5;106mANSI code 106 for R=2,G=3[0m
	 91,	// [38;5;91mANSI code  91 for R=2,B=3[0m
	 31,	// [38;5;31mANSI code  31 for G=2,B=3[0m
	112,	// [38;5;112mANSI code 112 for R=2,G=4[0m
	 92,	// [38;5;92mANSI code  92 for R=2,B=4[0m
	 32,	// [38;5;32mANSI code  32 for G=2,B=4[0m
};

#define colour_table_size (sizeof (colour_table) / sizeof (colour_table_type))

#define colour(N) (colour_table [ (N) % colour_table_size ])

