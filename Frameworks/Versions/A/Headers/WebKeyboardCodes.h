///
/// @file WebKeyboardCodes.h
///
/// @brief The header for the KeyCodes enumeration.
///
/// @author
///
/// This file is a part of Awesomium, a Web UI bridge for native apps.
///
/// Website: <http://www.awesomium.com>
///
/// Copyright (C) 2014 Awesomium Technologies LLC. All rights reserved.
/// Awesomium is a trademark of Awesomium Technologies LLC.
///
#ifndef AWESOMIUM_WEB_KEYBOARD_CODES_H_
#define AWESOMIUM_WEB_KEYBOARD_CODES_H_
#pragma once

namespace Awesomium {

  /// Namespace containing all the key-code definitions for WebKeyboardEvent.
  /// Most of these correspond directly to the key-code values on Windows.
  namespace KeyCodes {

    // AK_BACK (08) BACKSPACE key
    const int AK_BACK = 0x08;

    // AK_TAB (09) TAB key
    const int AK_TAB = 0x09;

    // AK_CLEAR (0C) CLEAR key
    const int AK_CLEAR = 0x0C;

    // AK_RETURN (0D)
    const int AK_RETURN = 0x0D;

    // AK_SHIFT (10) SHIFT key
    const int AK_SHIFT = 0x10;

    // AK_CONTROL (11) CTRL key
    const int AK_CONTROL = 0x11;

    // AK_MENU (12) ALT key
    const int AK_MENU = 0x12;

    // AK_PAUSE (13) PAUSE key
    const int AK_PAUSE = 0x13;

    // AK_CAPITAL (14) CAPS LOCK key
    const int AK_CAPITAL = 0x14;

    // AK_KANA (15) Input Method Editor (IME) Kana mode
    const int AK_KANA = 0x15;

    // AK_HANGUEL (15) IME Hanguel mode (maintained for compatibility; use AK_HANGUL)
    // AK_HANGUL (15) IME Hangul mode
    const int AK_HANGUL = 0x15;

    // AK_JUNJA (17) IME Junja mode
    const int AK_JUNJA = 0x17;

    // AK_FINAL (18) IME final mode
    const int AK_FINAL = 0x18;
    
    // AK_HANJA (19) IME Hanja mode
    const int AK_HANJA = 0x19;
    
    // AK_KANJI (19) IME Kanji mode
    const int AK_KANJI = 0x19;
    
    // AK_ESCAPE (1B) ESC key
    const int AK_ESCAPE = 0x1B;
    
    // AK_CONVERT (1C) IME convert
    const int AK_CONVERT = 0x1C;
    
    // AK_NONCONVERT (1D) IME nonconvert
    const int AK_NONCONVERT = 0x1D;
    
    // AK_ACCEPT (1E) IME accept
    const int AK_ACCEPT = 0x1E;
    
    // AK_MODECHANGE (1F) IME mode change request
    const int AK_MODECHANGE = 0x1F;
    
    // AK_SPACE (20) SPACEBAR
    const int AK_SPACE = 0x20;
    
    // AK_PRIOR (21) PAGE UP key
    const int AK_PRIOR = 0x21;
    
    // AK_NEXT (22) PAGE DOWN key
    const int AK_NEXT = 0x22;
    
    // AK_END (23) END key
    const int AK_END = 0x23;
    
    // AK_HOME (24) HOME key
    const int AK_HOME = 0x24;
    
    // AK_LEFT (25) LEFT ARROW key
    const int AK_LEFT = 0x25;
    
    // AK_UP (26) UP ARROW key
    const int AK_UP = 0x26;
    
    // AK_RIGHT (27) RIGHT ARROW key
    const int AK_RIGHT = 0x27;
    
    // AK_DOWN (28) DOWN ARROW key
    const int AK_DOWN = 0x28;
    
    // AK_SELECT (29) SELECT key
    const int AK_SELECT = 0x29;
    
    // AK_PRINT (2A) PRINT key
    const int AK_PRINT = 0x2A;
    
    // AK_EXECUTE (2B) EXECUTE key
    const int AK_EXECUTE = 0x2B;
    
    // AK_SNAPSHOT (2C) PRINT SCREEN key
    const int AK_SNAPSHOT = 0x2C;
    
    // AK_INSERT (2D) INS key
    const int AK_INSERT = 0x2D;
    
    // AK_DELETE (2E) DEL key
    const int AK_DELETE = 0x2E;
    
    // AK_HELP (2F) HELP key
    const int AK_HELP = 0x2F;
    
    // (30) 0 key
    const int AK_0 = 0x30;
    
    // (31) 1 key
    const int AK_1 = 0x31;
    
    // (32) 2 key
    const int AK_2 = 0x32;
    
    // (33) 3 key
    const int AK_3 = 0x33;
    
    // (34) 4 key
    const int AK_4 = 0x34;
    
    // (35) 5 key;
    const int AK_5 = 0x35;
    
    // (36) 6 key
    const int AK_6 = 0x36;
    
    // (37) 7 key
    const int AK_7 = 0x37;
    
    // (38) 8 key
    const int AK_8 = 0x38;
    
    // (39) 9 key
    const int AK_9 = 0x39;
    
    // (41) A key
    const int AK_A = 0x41;
    
    // (42) B key
    const int AK_B = 0x42;
    
    // (43) C key
    const int AK_C = 0x43;
    
    // (44) D key
    const int AK_D = 0x44;
    
    // (45) E key
    const int AK_E = 0x45;
    
    // (46) F key
    const int AK_F = 0x46;
    
    // (47) G key
    const int AK_G = 0x47;
    
    // (48) H key
    const int AK_H = 0x48;
    
    // (49) I key
    const int AK_I = 0x49;
    
    // (4A) J key
    const int AK_J = 0x4A;
    
    // (4B) K key
    const int AK_K = 0x4B;
    
    // (4C) L key
    const int AK_L = 0x4C;
    
    // (4D) M key
    const int AK_M = 0x4D;
    
    // (4E) N key
    const int AK_N = 0x4E;
    
    // (4F) O key
    const int AK_O = 0x4F;
    
    // (50) P key
    const int AK_P = 0x50;
    
    // (51) Q key
    const int AK_Q = 0x51;
    
    // (52) R key
    const int AK_R = 0x52;
    
    // (53) S key
    const int AK_S = 0x53;
    
    // (54) T key
    const int AK_T = 0x54;
    
    // (55) U key
    const int AK_U = 0x55;
    
    // (56) V key
    const int AK_V = 0x56;
    
    // (57) W key
    const int AK_W = 0x57;
    
    // (58) X key
    const int AK_X = 0x58;
    
    // (59) Y key
    const int AK_Y = 0x59;
    
    // (5A) Z key
    const int AK_Z = 0x5A;
    
    // AK_LWIN (5B) Left Windows key (Microsoft Natural keyboard)
    const int AK_LWIN = 0x5B;
    
    // AK_RWIN (5C) Right Windows key (Natural keyboard)
    const int AK_RWIN = 0x5C;
    
    // AK_APPS (5D) Applications key (Natural keyboard)
    const int AK_APPS = 0x5D;
    
    // AK_SLEEP (5F) Computer Sleep key
    const int AK_SLEEP = 0x5F;
    
    // AK_NUMPAD0 (60) Numeric keypad 0 key
    const int AK_NUMPAD0 = 0x60;
    
    // AK_NUMPAD1 (61) Numeric keypad 1 key
    const int AK_NUMPAD1 = 0x61;
    
    // AK_NUMPAD2 (62) Numeric keypad 2 key
    const int AK_NUMPAD2 = 0x62;
    
    // AK_NUMPAD3 (63) Numeric keypad 3 key
    const int AK_NUMPAD3 = 0x63;
    
    // AK_NUMPAD4 (64) Numeric keypad 4 key
    const int AK_NUMPAD4 = 0x64;
    
    // AK_NUMPAD5 (65) Numeric keypad 5 key
    const int AK_NUMPAD5 = 0x65;
    
    // AK_NUMPAD6 (66) Numeric keypad 6 key
    const int AK_NUMPAD6 = 0x66;
    
    // AK_NUMPAD7 (67) Numeric keypad 7 key
    const int AK_NUMPAD7 = 0x67;
    
    // AK_NUMPAD8 (68) Numeric keypad 8 key
    const int AK_NUMPAD8 = 0x68;
    
    // AK_NUMPAD9 (69) Numeric keypad 9 key
    const int AK_NUMPAD9 = 0x69;
    
    // AK_MULTIPLY (6A) Multiply key
    const int AK_MULTIPLY = 0x6A;
    
    // AK_ADD (6B) Add key
    const int AK_ADD = 0x6B;
    
    // AK_SEPARATOR (6C) Separator key
    const int AK_SEPARATOR = 0x6C;
    
    // AK_SUBTRACT (6D) Subtract key
    const int AK_SUBTRACT = 0x6D;
    
    // AK_DECIMAL (6E) Decimal key
    const int AK_DECIMAL = 0x6E;
    
    // AK_DIVIDE (6F) Divide key
    const int AK_DIVIDE = 0x6F;
    
    // AK_F1 (70) F1 key
    const int AK_F1 = 0x70;
    
    // AK_F2 (71) F2 key
    const int AK_F2 = 0x71;
    
    // AK_F3 (72) F3 key
    const int AK_F3 = 0x72;
    
    // AK_F4 (73) F4 key
    const int AK_F4 = 0x73;
    
    // AK_F5 (74) F5 key
    const int AK_F5 = 0x74;
    
    // AK_F6 (75) F6 key
    const int AK_F6 = 0x75;
    
    // AK_F7 (76) F7 key
    const int AK_F7 = 0x76;
    
    // AK_F8 (77) F8 key
    const int AK_F8 = 0x77;
    
    // AK_F9 (78) F9 key
    const int AK_F9 = 0x78;
    
    // AK_F10 (79) F10 key
    const int AK_F10 = 0x79;
    
    // AK_F11 (7A) F11 key
    const int AK_F11 = 0x7A;
    
    // AK_F12 (7B) F12 key
    const int AK_F12 = 0x7B;
    
    // AK_F13 (7C) F13 key
    const int AK_F13 = 0x7C;
    
    // AK_F14 (7D) F14 key
    const int AK_F14 = 0x7D;
    
    // AK_F15 (7E) F15 key
    const int AK_F15 = 0x7E;
    
    // AK_F16 (7F) F16 key
    const int AK_F16 = 0x7F;
    
    // AK_F17 (80H) F17 key
    const int AK_F17 = 0x80;
    
    // AK_F18 (81H) F18 key
    const int AK_F18 = 0x81;
    
    // AK_F19 (82H) F19 key
    const int AK_F19 = 0x82;
    
    // AK_F20 (83H) F20 key
    const int AK_F20 = 0x83;
    
    // AK_F21 (84H) F21 key
    const int AK_F21 = 0x84;
    
    // AK_F22 (85H) F22 key
    const int AK_F22 = 0x85;
    
    // AK_F23 (86H) F23 key
    const int AK_F23 = 0x86;
    
    // AK_F24 (87H) F24 key
    const int AK_F24 = 0x87;
    
    // AK_NUMLOCK (90) NUM LOCK key
    const int AK_NUMLOCK = 0x90;
    
    // AK_SCROLL (91) SCROLL LOCK key
    const int AK_SCROLL = 0x91;
    
    // AK_LSHIFT (A0) Left SHIFT key
    const int AK_LSHIFT = 0xA0;
    
    // AK_RSHIFT (A1) Right SHIFT key
    const int AK_RSHIFT = 0xA1;
    
    // AK_LCONTROL (A2) Left CONTROL key
    const int AK_LCONTROL = 0xA2;
    
    // AK_RCONTROL (A3) Right CONTROL key
    const int AK_RCONTROL = 0xA3;
    
    // AK_LMENU (A4) Left MENU key
    const int AK_LMENU = 0xA4;
    
    // AK_RMENU (A5) Right MENU key
    const int AK_RMENU = 0xA5;
    
    // AK_BROWSER_BACK (A6) Windows 2000/XP: Browser Back key
    const int AK_BROWSER_BACK = 0xA6;
    
    // AK_BROWSER_FORWARD (A7) Windows 2000/XP: Browser Forward key
    const int AK_BROWSER_FORWARD = 0xA7;
    
    // AK_BROWSER_REFRESH (A8) Windows 2000/XP: Browser Refresh key
    const int AK_BROWSER_REFRESH = 0xA8;
    
    // AK_BROWSER_STOP (A9) Windows 2000/XP: Browser Stop key
    const int AK_BROWSER_STOP = 0xA9;
    
    // AK_BROWSER_SEARCH (AA) Windows 2000/XP: Browser Search key
    const int AK_BROWSER_SEARCH = 0xAA;
    
    // AK_BROWSER_FAVORITES (AB) Windows 2000/XP: Browser Favorites key
    const int AK_BROWSER_FAVORITES = 0xAB;
    
    // AK_BROWSER_HOME (AC) Windows 2000/XP: Browser Start and Home key
    const int AK_BROWSER_HOME = 0xAC;
    
    // AK_VOLUME_MUTE (AD) Windows 2000/XP: Volume Mute key
    const int AK_VOLUME_MUTE = 0xAD;
    
    // AK_VOLUME_DOWN (AE) Windows 2000/XP: Volume Down key
    const int AK_VOLUME_DOWN = 0xAE;
    
    // AK_VOLUME_UP (AF) Windows 2000/XP: Volume Up key
    const int AK_VOLUME_UP = 0xAF;
    
    // AK_MEDIA_NEXT_TRACK (B0) Windows 2000/XP: Next Track key
    const int AK_MEDIA_NEXT_TRACK = 0xB0;
    
    // AK_MEDIA_PREV_TRACK (B1) Windows 2000/XP: Previous Track key
    const int AK_MEDIA_PREV_TRACK = 0xB1;
    
    // AK_MEDIA_STOP (B2) Windows 2000/XP: Stop Media key
    const int AK_MEDIA_STOP = 0xB2;
    
    // AK_MEDIA_PLAY_PAUSE (B3) Windows 2000/XP: Play/Pause Media key
    const int AK_MEDIA_PLAY_PAUSE = 0xB3;
    
    // AK_LAUNCH_MAIL (B4) Windows 2000/XP: Start Mail key
    const int AK_MEDIA_LAUNCH_MAIL = 0xB4;
    
    // AK_LAUNCH_MEDIA_SELECT (B5) Windows 2000/XP: Select Media key
    const int AK_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5;
    
    // AK_LAUNCH_APP1 (B6) Windows 2000/XP: Start Application 1 key
    const int AK_MEDIA_LAUNCH_APP1 = 0xB6;
    
    // AK_LAUNCH_APP2 (B7) Windows 2000/XP: Start Application 2 key
    const int AK_MEDIA_LAUNCH_APP2 = 0xB7;
    
    // AK_OEM_1 (BA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
    const int AK_OEM_1 = 0xBA;
    
    // AK_OEM_PLUS (BB) Windows 2000/XP: For any country/region, the '+' key
    const int AK_OEM_PLUS = 0xBB;
    
    // AK_OEM_COMMA (BC) Windows 2000/XP: For any country/region, the ',' key
    const int AK_OEM_COMMA = 0xBC;
    
    // AK_OEM_MINUS (BD) Windows 2000/XP: For any country/region, the '-' key
    const int AK_OEM_MINUS = 0xBD;
    
    // AK_OEM_PERIOD (BE) Windows 2000/XP: For any country/region, the '.' key
    const int AK_OEM_PERIOD = 0xBE;
    
    // AK_OEM_2 (BF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
    const int AK_OEM_2 = 0xBF;
    
    // AK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
    const int AK_OEM_3 = 0xC0;
    
    // AK_OEM_4 (DB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
    const int AK_OEM_4 = 0xDB;
    
    // AK_OEM_5 (DC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
    const int AK_OEM_5 = 0xDC;
    
    // AK_OEM_6 (DD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
    const int AK_OEM_6 = 0xDD;
    
    // AK_OEM_7 (DE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
    const int AK_OEM_7 = 0xDE;
    
    // AK_OEM_8 (DF) Used for miscellaneous characters; it can vary by keyboard.
    const int AK_OEM_8 = 0xDF;
    
    // AK_OEM_102 (E2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
    const int AK_OEM_102 = 0xE2;
    
    // AK_PROCESSKEY (E5) Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
    const int AK_PROCESSKEY = 0xE5;
    
    // AK_PACKET (E7) Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes. The AK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT,SendInput, WM_KEYDOWN, and WM_KEYUP
    const int AK_PACKET = 0xE7;
    
    // AK_ATTN (F6) Attn key
    const int AK_ATTN = 0xF6;
    
    // AK_CRSEL (F7) CrSel key
    const int AK_CRSEL = 0xF7;
    
    // AK_EXSEL (F8) ExSel key
    const int AK_EXSEL = 0xF8;
    
    // AK_EREOF (F9) Erase EOF key
    const int AK_EREOF = 0xF9;
    
    // AK_PLAY (FA) Play key
    const int AK_PLAY = 0xFA;
    
    // AK_ZOOM (FB) Zoom key
    const int AK_ZOOM = 0xFB;

    // AK_NONAME (FC) Reserved for future use
    const int AK_NONAME = 0xFC;
    
    // AK_PA1 (FD) PA1 key
    const int AK_PA1 = 0xFD;
    
    // AK_OEM_CLEAR (FE) Clear key
    const int AK_OEM_CLEAR = 0xFE;
    
    const int AK_UNKNOWN = 0;
  }
}

#endif  // AWESOMIUM_WEB_KEYBOARD_CODES_H_
