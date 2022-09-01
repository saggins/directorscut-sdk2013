"GameMenu"
{
	"1"
	{
		"label" "#GameUI_GameMenu_ResumeGame"
		"command" "ResumeGame"
		"InGameOrder" "10"
		"OnlyInGame" "1"
	}
	"1_1"
	{
		"label" "===================="
		"command" ""
		"InGameOrder" "11"
		"OnlyInGame" "1"
	}
	"2"	
	{
		"label" "MAP STAGE"
		"command" "engine map stage"
		"InGameOrder" "20"
		"notmulti" "1"
	}
	"3"
	{
		"label" "MAP STAGE DARK"
		"command" "engine map stage_projection"
		"InGameOrder" "30"
		"notmulti" "1"
	}
	"4"
	{
		"label" "MAP SDK VEHICLES"
		"command" "engine map sdk_vehicles"
		"InGameOrder" "40"
		"notmulti" "1"
	}
	"4_1"
	{
		"label" "===================="
		"command" ""
		"InGameOrder" "41"
	}
	"5"
	{
		"label" "TOGGLE IMGUI VISIBILITY"
		"command" "engine dx_imgui_toggle"
		"InGameOrder" "50"
		"OnlyInGame" "1"
	}
	"6"
	{
		"label" "TOGGLE IMGUI INPUT"
		"command" "engine dx_imgui_input_toggle"
		"InGameOrder" "60"
		"OnlyInGame" "1"
	}
	"6_1"
	{
		"label" "===================="
		"command" ""
		"InGameOrder" "61"
		"OnlyInGame" "1"
	}
	"7"
	{
		"label" "OPEN CONSOLE"
		"command" "engine showconsole"
		"InGameOrder" "70"
	}
	"8"
	{
		"label" "#GameUI_GameMenu_Options"
		"command" "OpenOptionsDialog"
		"InGameOrder" "80"
	}
	"8_1"
	{
		"label" "===================="
		"command" ""
		"InGameOrder" "81"
	}
	"9"
	{
		"label" "#GameUI_GameMenu_Disconnect"
		"command" "Disconnect"
		"InGameOrder" "90"
		"OnlyInGame" "1"
	}
	"10"
	{
		"label" "#GameUI_GameMenu_Quit"
		"command" "Quit"
		"InGameOrder" "100"
	}
}

