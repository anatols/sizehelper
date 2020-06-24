//-------------------------------------------------------------------------------
//	PiPL resource
//-------------------------------------------------------------------------------

resource 'PiPL' (0x3e80, "WheelBrush" " PiPL", purgeable)
{
    {
	    Kind { Extension },
	    Name { "WheelBrush" "..." },
	    Category { "Anatoliy Samara" },
	    Version { 0x10000 },

		// ClassID, eventID, aete ID, uniqueString:
	    HasTerminology { 'sdK8', 'wbrE', 0x3e80, "Anatoliy Samara" " " "WheelBrush" },
		/* classID, eventID, aete ID, uniqueString (presence of uniqueString makes it
		   non-AppleScript */

		EnableInfo { "true" },

		CodeWin32X86 { "ENTRYPOINT1" },

	}
};
