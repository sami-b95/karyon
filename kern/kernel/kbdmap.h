#ifndef _KBDMAP_H_
#define _KBDMAP_H_

#include <kernel/types.h>

/** Keyboard map (inspired by Pépin OS) **/

uchar_t *kbd_map[] = {
	"\xff", "\xff", "\xff", "\xff",
	"\x1b", "\x1b", "\x1b", "\x1b",	/*      esc     (0x01)  */
	"1", "!", "1", "1",
	"2", "@", "2", "2",
	"3", "#", "3", "3",
	"4", "$", "4", "4",
	"5", "%", "5", "5",
	"6", "^", "6", "6",
	"7", "&", "7", "7",
	"8", "*", "8", "8",
	"9", "(", "9", "9",
	"0", ")", "0", "0",
	"-", "_", "-", "-",
	"=", "+", "=", "=",
	"\x08", "\x08", "\x7F", "\x08",	/*      backspace       */
	"\x09", "\x09", "\x09", "\x09",	/*      tab     */
	"q", "Q", "q", "q",
	"w", "W", "w", "w",
	"e", "E", "e", "e",
	"r", "R", "r", "r",
	"t", "T", "t", "t",
	"y", "Y", "y", "y",
	"u", "U", "u", "u",
	"i", "I", "i", "i",
	"o", "O", "o", "o",
	"p", "P", "p", "p",
	"[", "{", "[", "[",
	"]", "}", "]", "]",
	"\r", "\r", "\r", "\r",	/*      enter   */
	"\xff", "\xff", "\xff", "\xff",	/*      ctrl    */
	"a", "A", "a", "a",
	"s", "S", "s", "s",
	"d", "D", "d", "d",
	"f", "F", "f", "f",
	"g", "G", "g", "g",
	"h", "H", "h", "h",
	"j", "J", "j", "j",
	"k", "K", "k", "k",
	"l", "L", "l", "l",
	";", ":", ";", ";",
	"\x27", "\x22", "\x27", "\x27",	/*      '"      */
	"`", "~", "`", "`",	/*      `~      */
	"\xff", "\xff", "\xff", "\xff",	/*      Lshift  (0x2a)  */
	"\\", "|", "\\", "\\",
	"z", "Z", "z", "z",
	"x", "X", "x", "x",
	"c", "C", "c", "c",
	"v", "V", "v", "v",
	"b", "B", "b", "b",
	"n", "N", "n", "n",
	"m", "M", "m", "m",
	"\x2c", "\x3c", "\x2c", "\x2C",	/*      ,<      */
	"\x2e", "\x3e", "\x2e", "\x2e",	/*      .>      */
	"\x2f", "\x3f", "\x2f", "\x2f",	/*      /?      */
	"\xff", "\xff", "\xff", "\xff",	/*      Rshift  (0x36)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x37)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x38)  */
	" ", " ", " ", " ",	/*      space   */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3a)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3b)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3c)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3d)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3e)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x3f)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x40)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x41)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x42)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x43)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x44)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x45)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x46)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x47)  */
	"\eOA", "\eOA", "\eOA", "\eOA",	/*      Up arrow  (0x48)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x49)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x4a)  */
	"\eOD", "\eOD", "\eOD", "\eOD",	/*      Left arrow  (0x4b)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x4c)  */
	"\eOC", "\eOC", "\eOC", "\eOC",	/*      Right arrow  (0x4d)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x4e)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x4f)  */
	"\eOB", "\eOB", "\eOB", "\eOB",	/*      Down arrow  (0x50)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x51)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x52)  */
	"\x7f", "\x7f", "\x7f", "\x7f",	/*      (0x53)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x54)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x55)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x56)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x57)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x58)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x59)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5a)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5b)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5c)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5d)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5e)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x5f)  */
	"\xff", "\xff", "\xff", "\xff",	/*      (0x60)  */
	"\xff", "\xff", "\xff", "\xff"	/*      (0x61)  */
};

#endif
