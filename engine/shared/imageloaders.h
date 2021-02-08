//-------------------------------------------------------------------------------------------------
// Image loading routines
//-------------------------------------------------------------------------------------------------

#pragma once

#include "../../common/q_types.h"

namespace img
{
	// Returns a 32-bit buffer (8-bit PCX only, 24/32-bit TGA only)
	byte		*LoadPCX(const byte *pBuffer, int nBufLen, int &width, int &height);
	byte		*LoadTGA(const byte *pBuffer, int nBufLen, int &width, int &height);

	// Creates a 24-bit colormap from a provided PCX buffer
	qboolean	CreateColormapFromPCX(const byte *pBuffer, int nBufLen, uint *pColormap);

	// Return true if the first 8 bytes of the buffer match the fixed PNG ID
	bool TestPNG( const byte *buf );

	byte *LoadPNG( byte *buf, int &width, int &height );

	// Write a 24-bit PNG using stdio
	bool WritePNG24( int width, int height, byte *buffer, FILE *handle );

	void VerticalFlip( byte *image, int w, int h, int bytes_per_pixel );
}
