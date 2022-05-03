//-----------------------------------------------------------------------------
//           Name: os_dialogs_mac.mm
//      Developer: Wolfire Games LLC
//    Description: 
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include <Internal/error.h>
#include <Internal/config.h>

#include <UserInput/input.h>
#include <Logging/logdata.h>

#import <AppKit/NSAlert.h>
#import <AppKit/NSTextField.h>
#import <AppKit/NSPanel.h>

#include <map>
#include <string>

ErrorResponse OSDisplayError(const char* title, const char* contents, ErrorType type)
{
	bool old_mouse = Input::Instance()->GetGrabMouse();
	Input::Instance()->SetGrabMouse(false);
	UIShowCursor(1);

	NSString *errStr = [NSString stringWithUTF8String:title];
	NSString *messageStr = [NSString stringWithUTF8String:contents];
	
	NSAlert *alert = [NSAlert alertWithMessageText:errStr
									 defaultButton:type == _ok_cancel_retry ? @"Continue" : @"Ok"
								   alternateButton:type == _ok ? nil : @"Cancel"
									   otherButton:type == _ok_cancel_retry ? @"Retry" : nil
						 informativeTextWithFormat:messageStr];

	NSInteger button = [alert runModal];

	if (button == NSAlertDefaultReturn) {
		Input::Instance()->SetGrabMouse(old_mouse);
		UIShowCursor(0);
		return _continue;
	} else if (button == NSAlertAlternateReturn) {
		return _er_exit;
	} else {
		Input::Instance()->SetGrabMouse(old_mouse);
		UIShowCursor(0);
		return _retry;
	}
}
