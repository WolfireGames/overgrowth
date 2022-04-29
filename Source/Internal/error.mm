//-----------------------------------------------------------------------------
//           Name: error.mm
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

#include <map>
#include <string>

#import <AppKit/NSAlert.h>
#import <AppKit/NSTextField.h>
#import <AppKit/NSPanel.h>

extern std::map<std::string, int> error_message_history;

std::string QueryString( std::string title, std::string contents )
{
	bool old_mouse = Input::Instance()->GetGrabMouse();
    Input::Instance()->SetGrabMouse(false);
	UIShowCursor(1);
	
	NSString *contentsString = [NSString stringWithUTF8String:contents.c_str()];
	NSAlert *alert = [NSAlert alertWithMessageText:contentsString
									 defaultButton:@"OK"
								   alternateButton:@"Cancel"
									   otherButton:nil
						 informativeTextWithFormat:@""];
	
	NSTextField *input = [[[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)] autorelease];
	[input setStringValue:@""];
	[alert setAccessoryView:input];
	[[alert window] makeFirstResponder:input];
	NSInteger button = [alert runModal];
	
	Input::Instance()->SetGrabMouse(old_mouse);
	UIShowCursor(0);
	if (button == NSAlertDefaultReturn) {
		[input validateEditing];
		std::string return_value = [[input stringValue] UTF8String];
		return return_value;
	} else if (button == NSAlertAlternateReturn) {
		return "";
	} else {
		return "";
	}
}

// Basic alert code in case we need it
/*NSAlert *alert = [NSAlert alertWithMessageText:@"Could not open connection to Xgrid server." 
 defaultButton:@"OK" alternateButton:nil otherButton:nil
 informativeTextWithFormat:
 @"Check that controller host name was entered correctly, and that it is "
 @"available."];
 [alert runModal];*/
 
bool Query(std::string title, std::string contents)
{
	bool old_mouse = Input::Instance()->GetGrabMouse();
	Input::Instance()->SetGrabMouse(false);
	UIShowCursor(1);
	
	NSString *contentsString = [NSString stringWithUTF8String:contents.c_str()];
	NSAlert *alert = [NSAlert alertWithMessageText:contentsString
									 defaultButton:@"Yes"
								   alternateButton:@"No"
									   otherButton:nil
						 informativeTextWithFormat:@""];
	
	NSInteger button = [alert runModal];
	
	Input::Instance()->SetGrabMouse(old_mouse);
	UIShowCursor(0);
	return (button == NSAlertDefaultReturn);
}

ErrorResponse DisplayError(const char* title, const char* contents, ErrorType type, bool allow_repetition)
{
    LOGI << "Displaying message: " << title << ", " << contents << std::endl;

    if( config["no_dialogues"].toBool() )
    {
        return _continue; 
    }

	if(!allow_repetition && error_message_history[contents]) {
		return _continue;
	}
	error_message_history[contents]++;

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
		exit(1);
	} else {
		Input::Instance()->SetGrabMouse(old_mouse);
		UIShowCursor(0);
		return _retry;
	}
}

ErrorResponse DisplayFormatError(ErrorType type,
                           bool allow_repetition,
                           const char* title,
                           const char* fmtcontents, 
                           ... ) {
    static const int kBufSize = 2048;
    char err_buf[kBufSize];
    va_list args;
    va_start(args, fmtcontents);
    VFormatString(err_buf, kBufSize, fmtcontents, args);
    va_end(args);
    return DisplayError(title, err_buf, type, allow_repetition);
}
