HotKeySet("{PAUSE}", "TogglePause")

Run("C:/Program Files (x86)/Annosoft/Lipsync Tool 4.0/LipsyncTool4.0.exe")
WinWaitActive("Reminder")
Send("{ENTER}")
WinWaitActive("The Lipsync Tool!")
$size = WinGetPos("The Lipsync Tool!")
;MsgBox(0,"Size","Position = " & $size[0] & " " & $size[1])
Dim $click_pos[2]
$click_pos[0] = 746 + $size[0]
$click_pos[1] = 435 + $size[1]
For $i = 10 to 0 Step -1
	ProcessSound($i)
Next
dim $done = False
While $done == False
	MouseClick("primary", $click_pos[0], $click_pos[1], 1, 0)
	MouseWheel("down", 3)
	For $i = 10 to 2 Step -1
		ProcessSound($i)
	Next
WEnd

Func TogglePause()
    $rc = msgbox(1,"Paused","Click Ok to continue or Cancel to stop program")
    if $rc = 2 then exit
EndFunc
	
Func ProcessSound($val)
	MouseClick("primary", $click_pos[0], $click_pos[1]+$val*18, 2, 0)
	WinWaitActive("Progress")
	WinWaitClose("Progress")
	MouseClick("primary", 389 + $size[0],63 + $size[1], 1, 0)
	Send("^e")
	WinWaitActive("Export format")
	Send("{ENTER}")
	WinWaitActive("Save As")
	Send("!s")
	WinWaitClose("Save As")
EndFunc