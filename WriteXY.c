void WriteChar (uint16_t sx, uint16_t sy, uint16_t ch)   /* UINT16_T for character */
{
	uint8_t* fontbmp_ptr;
	if (ch >= 0 && ch <= 255)			            // Character in font range lower half
	{
	     fontbmp_ptr = (uint8_t*)&Arial_Narrow_23_LOWHALF[0];   // Point at low half of font
	} else {                                                    // Character in font upper half range
	     fontbmp_ptr = (uint8_t*)&Arial_Narrow_23_TOPHALF[0];   // Point at top half of font
		 ch -= 256;				            // Take 256 fron character as it's top half of font
	}
	fontbmp_ptr +=  (FONTCHAR_PITCH * Ch);			    // Set font to specific character
    for (y = 0; y < FONTHEIGHT; y++)                           // For each line in height
    {
        uint8_t b = *fontbmp_ptr++;				// Fetch the byte from font	
        for (x = 0; x < FONTWIDTH; x++)                        // For each font pixel across
        {
            RGB565 col;
            col = ((b & 0x80) == 0x80) ? TXTCOLOR : BKGNDCOLOR;
			// >>>> WRITE THE COLOR TO SCREEN <<<<
            b = b << 1;					     // Shift 1 bit left
           if ( (((x + 1) % 8) == 0) && (x + 1) < FONTWIDTH)   // Do we need to load next font byte
           {
               b = *fontbmp_ptr++;                            // Fetch next font byte
           }
        }
    }
}
