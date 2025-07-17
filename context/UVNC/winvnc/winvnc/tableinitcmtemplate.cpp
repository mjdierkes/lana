/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
//  If the source code for the program is not available from the place from
//  which you received this file, check
//  https://uvnc.com/
//
////////////////////////////////////////////////////////////////////////////


/*
 * tableinitcmtemplate.c - template for initialising lookup tables for
 * translation from a colour map to true colour.
 *
 * This file shouldn't be compiled. It is included multiple times by
 * translate.c, each time with a different definition of the macro OUT.
 * For each value of OUT, this file defines a function which allocates an
 * appropriately sized lookup table and initialises it.
 *
 * I know this code isn't nice to read because of all the macros, but
 * efficiency is important here.
 */

#if !defined(OUTVNC)
#error "This file shouldn't be compiled."
#error "It is included as part of translate.c"
#endif

#define OUT_T CONCAT2E(CARD,OUTVNC)
#define SwapOUTVNC(x) CONCAT2E(Swap,OUTVNC) (x)
#define rfbInitColourMapSingleTableOUTVNC \
				CONCAT2E(rfbInitColourMapSingleTable,OUTVNC)

// THIS CODE HAS BEEN MODIFIED FROM THE ORIGINAL UNIX SOURCE
// TO WORK FOR ULTRAVNC SERVER. THE PALETTE SHOULD REALLY BE RETRIEVED
// FROM THE VNCDESKTOP OBJECT, RATHER THAN FROM THE OS DIRECTLY

static void
rfbInitColourMapSingleTableOUTVNC (char **table,
								rfbPixelFormat *in,
								rfbPixelFormat *out)
{
	vnclog.Print(LL_ALL, VNCLOG("rfbInitColourMapSingleTable called\n"));

	// ALLOCATE SPACE FOR COLOUR TABLE

    unsigned int nEntries = 1 << in->bitsPerPixel;

	// Allocate the table
    if (*table) free(*table);
    *table = (char *)malloc(nEntries * sizeof(OUT_T));
	if (*table == NULL)
	{
		vnclog.Print(LL_INTERR, VNCLOG("failed to allocate translation table\n"));
		return;
	}

	// Obtain the system palette
	bool create_dc = false;
	HDC hDC = GetDcMirror();
	if (hDC == NULL)
	{
		vnclog.Print(LL_ALL, VNCLOG("Using video Palette\n"));
		hDC = GetDC(NULL);
	}
	else
	{
		vnclog.Print(LL_ALL, VNCLOG("Using mirror video Palette\n"));
		create_dc = true;
	}

	PALETTEENTRY palette[256];
  UINT entries = ::GetSystemPaletteEntries(hDC,	0, 256, palette);
	vnclog.Print(LL_INTINFO, VNCLOG("got %u palette entries\n"), GetLastError());
	if (create_dc) DeleteDC(hDC);
	else ReleaseDC(NULL, hDC);

  // - Set the rest of the palette to something nasty but usable
  unsigned int i;
  for (i=entries;i<256;i++) {
    palette[i].peRed = i % 2 ? 255 : 0;
    palette[i].peGreen = i/2 % 2 ? 255 : 0;
    palette[i].peBlue = i/4 % 2 ? 255 : 0;
  }

	// COLOUR TRANSLATION

	// We now have the colour table intact. Map it into a translation table
  int r, g, b;
  OUT_T *t = (OUT_T *)*table;

  for (i = 0; i < nEntries; i++)
	{
		// Split down the RGB data
		r = palette[i].peRed;
		g = palette[i].peGreen;
		b = palette[i].peBlue;

		// Now translate it
		t[i] = ((((r * out->redMax + 127) / 255) << out->redShift) |
			(((g * out->greenMax + 127) / 255) << out->greenShift) |
			(((b * out->blueMax + 127) / 255) << out->blueShift));
#if (OUTVNC != 8)
		if (out->bigEndian != in->bigEndian)
		{
			t[i] = SwapOUTVNC(t[i]);
		}
#endif
	}

	vnclog.Print(LL_ALL, VNCLOG("rfbInitColourMapSingleTable done\n"));
}

#undef OUT_T
#undef SwapOUT
#undef rfbInitColourMapSingleTableOUTVNC
