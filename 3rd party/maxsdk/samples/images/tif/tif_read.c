/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_read.c,v 1.6 2000/08/14 16:21:54 warmerda Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 * Scanline-oriented Read Support
 */
#include "tiffiop.h"
#include <stdio.h>
#include <assert.h>

#include "StrTab.h"

static	int TIFFFillStrip(TIFF*, tstrip_t);
static	int TIFFFillTile(TIFF*, ttile_t);
static	int TIFFStartStrip(TIFF*, tstrip_t);
static	int TIFFStartTile(TIFF*, ttile_t);
static	int TIFFCheckRead(TIFF*, int);

#define	NOSTRIP	((tstrip_t) -1)			/* undefined state */
#define	NOTILE	((ttile_t) -1)			/* undefined state */

/*
 * Seek to a random row+sample in a file.
 */
static int
TIFFSeek(TIFF* tif, uint32 row, tsample_t sample)
{
	register TIFFDirectory *td = &tif->tif_dir;
	tstrip_t strip;

	if (row >= td->td_imagelength) {	/* out of range */
		TIFFError(tif->tif_name, GetString(IDS_SCAN_OOR),//"%lu: Row out of range, max %lu",
		    (u_long) row, (u_long) td->td_imagelength);
		return (0);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFError(tif->tif_name,
			    GetString(IDS_SCAN_OOR),//"%lu: Sample out of range, max %lu",
			    (u_long) sample, (u_long) td->td_samplesperpixel);
			return (0);
		}
		strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
	} else
		strip = row / td->td_rowsperstrip;
	if (strip != tif->tif_curstrip) { 	/* different strip, refill */
		if (!TIFFFillStrip(tif, strip))
			return (0);
	} else if (row < tif->tif_row) {
		/*
		 * Moving backwards within the same strip: backup
		 * to the start and then decode forward (below).
		 *
		 * NB: If you're planning on lots of random access within a
		 * strip, it's better to just read and decode the entire
		 * strip, and then access the decoded data in a random fashion.
		 */
		if (!TIFFStartStrip(tif, strip))
			return (0);
	}
	if (row != tif->tif_row) {
		/*
		 * Seek forward to the desired row.
		 */
		if (!(*tif->tif_seek)(tif, row - tif->tif_row))
			return (0);
		tif->tif_row = row;
	}
	return (1);
}

int
TIFFReadScanline(TIFF* tif, tdata_t buf, uint32 row, tsample_t sample)
{
	int e;

	if (!TIFFCheckRead(tif, 0))
		return (-1);
	if( (e = TIFFSeek(tif, row, sample)) != 0) {
		/*
		 * Decompress desired row into user buffer.
		 */
		e = (*tif->tif_decoderow)
		    (tif, (tidata_t) buf, tif->tif_scanlinesize, sample);
		tif->tif_row++;
		if (e)
			(*tif->tif_postdecode)(tif, (tidata_t) buf,
			    tif->tif_scanlinesize);
	}
	return (e > 0 ? 1 : -1);
}

/*
 * Read a strip of data and decompress the specified
 * amount into the user-supplied buffer.
 */
tsize_t
TIFFReadEncodedStrip(TIFF* tif, tstrip_t strip, tdata_t buf, tsize_t size)
{
	TIFFDirectory *td = &tif->tif_dir;
	uint32 nrows;
	tsize_t stripsize;
        tstrip_t sep_strip, strips_per_sep;

	if (!TIFFCheckRead(tif, 0))
		return (-1);
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, GetString(IDS_SCAN_STRIP_OOR),//"%ld: Strip out of range, max %ld",
		    (long) strip, (long) td->td_nstrips);
		return (-1);
	}
	/*
	 * Calculate the strip size according to the number of
	 * rows in the strip (check for truncated last strip on any
         * of the separations).
	 */
        if( td->td_rowsperstrip >= td->td_imagelength )
            strips_per_sep = 1;
        else
            strips_per_sep = (td->td_imagelength+td->td_rowsperstrip-1)
                / td->td_rowsperstrip;

        sep_strip = strip % strips_per_sep;

	if (sep_strip != strips_per_sep-1 ||
	    (nrows = td->td_imagelength % td->td_rowsperstrip) == 0)
		nrows = td->td_rowsperstrip;

	stripsize = TIFFVStripSize(tif, nrows);
	if (size == (tsize_t) -1)
		size = stripsize;
	else if (size > stripsize)
		size = stripsize;
	if (TIFFFillStrip(tif, strip) && (*tif->tif_decodestrip)(tif,
	    (tidata_t) buf, size, (tsample_t)(strip / td->td_stripsperimage))) {
		(*tif->tif_postdecode)(tif, (tidata_t) buf, size);
		return (size);
	} else
		return ((tsize_t) -1);
}

static tsize_t
TIFFReadRawStrip1(TIFF* tif,
    tstrip_t strip, tdata_t buf, tsize_t size, const TCHAR* module)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (!isMapped(tif)) {
		tsize_t cc;

		if (!SeekOK(tif, td->td_stripoffset[strip])) {
			TIFFError(module,
			    GetString(IDS_SCAN_SEEK_ERR),//"%s: Seek error at scanline %lu, strip %lu",
			    tif->tif_name,
			    (u_long) tif->tif_row, (u_long) strip);
			return (-1);
		}
		cc = TIFFReadFile(tif, buf, size);
		if (cc != size) {
			TIFFError(module,
		GetString(IDS_SCAN_READ_ERR),//"%s: Read error at scanline %lu; got %lu bytes, expected %lu",
			    tif->tif_name,
			    (u_long) tif->tif_row,
			    (u_long) cc,
			    (u_long) size);
			return (-1);
		}
	} else {
		if (td->td_stripoffset[strip] + size > tif->tif_size) {
			TIFFError(module,
    GetString(IDS_SCAN_READ_ERR),//"%s: Read error at scanline %lu; got %lu bytes, expected %lu",
			    tif->tif_name,
			    (u_long) tif->tif_row,
			    (u_long) strip,
			    (u_long) tif->tif_size - td->td_stripoffset[strip],
			    (u_long) size);
			return (-1);
		}
		_TIFFmemcpy(buf, tif->tif_base + td->td_stripoffset[strip], size);
	}
	return (size);
}

/*
 * Read a strip of data from the file.
 */
tsize_t
TIFFReadRawStrip(TIFF* tif, tstrip_t strip, tdata_t buf, tsize_t size)
{
	static const TCHAR module[] = _T("TIFFReadRawStrip");
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t bytecount;

	if (!TIFFCheckRead(tif, 0))
		return ((tsize_t) -1);
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, GetString(IDS_SCAN_STRIP_OOR),//"%lu: Strip out of range, max %lu",
		    (u_long) strip, (u_long) td->td_nstrips);
		return ((tsize_t) -1);
	}
	bytecount = td->td_stripbytecount[strip];
	if (bytecount <= 0) {
		TIFFError(tif->tif_name,
		    GetString(IDS_SCAN_INVALID_SBC),//"%lu: Invalid strip byte count, strip %lu",
		    (u_long) bytecount, (u_long) strip);
		return ((tsize_t) -1);
	}
	if (size != (tsize_t)-1 && size < bytecount)
		bytecount = size;
	return (TIFFReadRawStrip1(tif, strip, buf, bytecount, module));
}

/*
 * Read the specified strip and setup for decoding. 
 * The data buffer is expanded, as necessary, to
 * hold the strip's data.
 */
static int
TIFFFillStrip(TIFF* tif, tstrip_t strip)
{
	static const TCHAR module[] = _T("TIFFFillStrip");
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t bytecount;

	bytecount = td->td_stripbytecount[strip];
	if (bytecount <= 0) {
		TIFFError(tif->tif_name,
		    GetString(IDS_SCAN_INVALID_SBC),//"%lu: Invalid strip byte count, strip %lu",
		    (u_long) bytecount, (u_long) strip);
		return (0);
	}
	if (isMapped(tif) &&
	    (isFillOrder(tif, td->td_fillorder) || (tif->tif_flags & TIFF_NOBITREV))) {
		/*
		 * The image is mapped into memory and we either don't
		 * need to flip bits or the compression routine is going
		 * to handle this operation itself.  In this case, avoid
		 * copying the raw data and instead just reference the
		 * data from the memory mapped file image.  This assumes
		 * that the decompression routines do not modify the
		 * contents of the raw data buffer (if they try to,
		 * the application will get a fault since the file is
		 * mapped read-only).
		 */
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_flags &= ~TIFF_MYBUFFER;
		if ( td->td_stripoffset[strip] + bytecount > tif->tif_size) {
			/*
			 * This error message might seem strange, but it's
			 * what would happen if a read were done instead.
			 */
			TIFFError(module,
		    GetString(IDS_SCAN_READ_ERR),//"%s: Read error on strip %lu; got %lu bytes, expected %lu",
			    tif->tif_name,
			    (u_long) strip,
			    (u_long) tif->tif_size - td->td_stripoffset[strip],
			    (u_long) bytecount);
			tif->tif_curstrip = NOSTRIP;
			return (0);
		}
		tif->tif_rawdatasize = bytecount;
		tif->tif_rawdata = tif->tif_base + td->td_stripoffset[strip];
	} else {
		/*
		 * Expand raw data buffer, if needed, to
		 * hold data strip coming from file
		 * (perhaps should set upper bound on
		 *  the size of a buffer we'll use?).
		 */
		if (bytecount > tif->tif_rawdatasize) {
			tif->tif_curstrip = NOSTRIP;
			if ((tif->tif_flags & TIFF_MYBUFFER) == 0) {
				TIFFError(module,
				GetString(ISD_SCAN_SMALL_BUFFER),//"%s: Data buffer too small to hold strip %lu",
				    tif->tif_name, (u_long) strip);
				return (0);
			}
			if (!TIFFReadBufferSetup(tif, 0,
			    TIFFroundup(bytecount, 1024)))
				return (0);
		}
		if (TIFFReadRawStrip1(tif, strip, (u_char *)tif->tif_rawdata,
		    bytecount, module) != bytecount)
			return (0);
		if (!isFillOrder(tif, td->td_fillorder) &&
		    (tif->tif_flags & TIFF_NOBITREV) == 0)
			TIFFReverseBits(tif->tif_rawdata, bytecount);
	}
	return (TIFFStartStrip(tif, strip));
}

/*
 * Tile-oriented Read Support
 * Contributed by Nancy Cam (Silicon Graphics).
 */

/*
 * Read and decompress a tile of data.  The
 * tile is selected by the (x,y,z,s) coordinates.
 */
tsize_t
TIFFReadTile(TIFF* tif,
    tdata_t buf, uint32 x, uint32 y, uint32 z, tsample_t s)
{
	if (!TIFFCheckRead(tif, 1) || !TIFFCheckTile(tif, x, y, z, s))
		return (-1);
	return (TIFFReadEncodedTile(tif,
	    TIFFComputeTile(tif, x, y, z, s), buf, (tsize_t) -1));
}

/*
 * Read a tile of data and decompress the specified
 * amount into the user-supplied buffer.
 */
tsize_t
TIFFReadEncodedTile(TIFF* tif, ttile_t tile, tdata_t buf, tsize_t size)
{
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t tilesize = tif->tif_tilesize;

	if (!TIFFCheckRead(tif, 1))
		return (-1);
	if (tile >= td->td_nstrips) {
		TIFFError(tif->tif_name, GetString(IDS_SCAN_TILE_OOR),//"%ld: Tile out of range, max %ld",
		    (long) tile, (u_long) td->td_nstrips);
		return (-1);
	}
	if (size == (tsize_t) -1)
		size = tilesize;
	else if (size > tilesize)
		size = tilesize;
	if (TIFFFillTile(tif, tile) && (*tif->tif_decodetile)(tif,
	    (tidata_t) buf, size, (tsample_t)(tile/td->td_stripsperimage))) {
		(*tif->tif_postdecode)(tif, (tidata_t) buf, size);
		return (size);
	} else
		return (-1);
}

static tsize_t
TIFFReadRawTile1(TIFF* tif,
    ttile_t tile, tdata_t buf, tsize_t size, const TCHAR* module)
{
	TIFFDirectory *td = &tif->tif_dir;

	if (!isMapped(tif)) {
		tsize_t cc;

		if (!SeekOK(tif, td->td_stripoffset[tile])) {
			TIFFError(module,
			    GetString(IDS_SCAN_SEEK_TILE),//"%s: Seek error at row %ld, col %ld, tile %ld",
			    tif->tif_name,
			    (long) tif->tif_row,
			    (long) tif->tif_col,
			    (long) tile);
			return ((tsize_t) -1);
		}
		cc = TIFFReadFile(tif, buf, size);
		if (cc != size) {
			TIFFError(module,
	    GetString(IDS_SCAN_READ_COL_ERR),//"%s: Read error at row %ld, col %ld; got %lu bytes, expected %lu",
			    tif->tif_name,
			    (long) tif->tif_row,
			    (long) tif->tif_col,
			    (u_long) cc,
			    (u_long) size);
			return ((tsize_t) -1);
		}
	} else {
		if (td->td_stripoffset[tile] + size > tif->tif_size) {
			TIFFError(module,
				GetString(IDS_SCAN_READ_COL_ERR),//"%s: Read error at row %ld, col %ld; got %lu bytes, expected %lu",
			    tif->tif_name,
			    (long) tif->tif_row,
			    (long) tif->tif_col,
			    (long) tile,
			    (u_long) tif->tif_size - td->td_stripoffset[tile],
			    (u_long) size);
			return ((tsize_t) -1);
		}
		_TIFFmemcpy(buf, tif->tif_base + td->td_stripoffset[tile], size);
	}
	return (size);
}

/*
 * Read a tile of data from the file.
 */
tsize_t
TIFFReadRawTile(TIFF* tif, ttile_t tile, tdata_t buf, tsize_t size)
{
	static const TCHAR module[] = _T("TIFFReadRawTile");
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t bytecount;

	if (!TIFFCheckRead(tif, 1))
		return ((tsize_t) -1);
	if (tile >= td->td_nstrips) {
		TIFFError(tif->tif_name, GetString(IDS_SCAN_TILE_OOR),//"%lu: Tile out of range, max %lu",
		    (u_long) tile, (u_long) td->td_nstrips);
		return ((tsize_t) -1);
	}
	bytecount = td->td_stripbytecount[tile];
	if (size != (tsize_t) -1 && size < bytecount)
		bytecount = size;
	return (TIFFReadRawTile1(tif, tile, buf, bytecount, module));
}

/*
 * Read the specified tile and setup for decoding. 
 * The data buffer is expanded, as necessary, to
 * hold the tile's data.
 */
static int
TIFFFillTile(TIFF* tif, ttile_t tile)
{
	static const TCHAR module[] = _T("TIFFFillTile");
	TIFFDirectory *td = &tif->tif_dir;
	tsize_t bytecount;

	bytecount = td->td_stripbytecount[tile];
	if (bytecount <= 0) {
		TIFFError(tif->tif_name,
		    GetString(IDS_SCAN_INVALID_TILE_BC),//"%lu: Invalid tile byte count, tile %lu",
		    (u_long) bytecount, (u_long) tile);
		return (0);
	}
	if (isMapped(tif) &&
	    (isFillOrder(tif, td->td_fillorder) || (tif->tif_flags & TIFF_NOBITREV))) {
		/*
		 * The image is mapped into memory and we either don't
		 * need to flip bits or the compression routine is going
		 * to handle this operation itself.  In this case, avoid
		 * copying the raw data and instead just reference the
		 * data from the memory mapped file image.  This assumes
		 * that the decompression routines do not modify the
		 * contents of the raw data buffer (if they try to,
		 * the application will get a fault since the file is
		 * mapped read-only).
		 */
		if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_flags &= ~TIFF_MYBUFFER;
		if ( td->td_stripoffset[tile] + bytecount > tif->tif_size) {
			tif->tif_curtile = NOTILE;
			return (0);
		}
		tif->tif_rawdatasize = bytecount;
		tif->tif_rawdata = tif->tif_base + td->td_stripoffset[tile];
	} else {
		/*
		 * Expand raw data buffer, if needed, to
		 * hold data tile coming from file
		 * (perhaps should set upper bound on
		 *  the size of a buffer we'll use?).
		 */
		if (bytecount > tif->tif_rawdatasize) {
			tif->tif_curtile = NOTILE;
			if ((tif->tif_flags & TIFF_MYBUFFER) == 0) {
				TIFFError(module,
				GetString(IDS_SCAN_TILE_SMALL_BUFF),//"%s: Data buffer too small to hold tile %ld",
				    tif->tif_name, (long) tile);
				return (0);
			}
			if (!TIFFReadBufferSetup(tif, 0,
			    TIFFroundup(bytecount, 1024)))
				return (0);
		}
		if (TIFFReadRawTile1(tif, tile, (u_char *)tif->tif_rawdata,
		    bytecount, module) != bytecount)
			return (0);
		if (!isFillOrder(tif, td->td_fillorder) &&
		    (tif->tif_flags & TIFF_NOBITREV) == 0)
			TIFFReverseBits(tif->tif_rawdata, bytecount);
	}
	return (TIFFStartTile(tif, tile));
}

/*
 * Setup the raw data buffer in preparation for
 * reading a strip of raw data.  If the buffer
 * is specified as zero, then a buffer of appropriate
 * size is allocated by the library.  Otherwise,
 * the client must guarantee that the buffer is
 * large enough to hold any individual strip of
 * raw data.
 */
int
TIFFReadBufferSetup(TIFF* tif, tdata_t bp, tsize_t size)
{
	static const TCHAR module[] = _T("TIFFReadBufferSetup");

	if (tif->tif_rawdata) {
		if (tif->tif_flags & TIFF_MYBUFFER)
			_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
	}
	if (bp) {
		tif->tif_rawdatasize = size;
		tif->tif_rawdata = (tidata_t) bp;
		tif->tif_flags &= ~TIFF_MYBUFFER;
	} else {
		tif->tif_rawdatasize = TIFFroundup(size, 1024);
		tif->tif_rawdata = (tidata_t) _TIFFmalloc(tif->tif_rawdatasize);
		tif->tif_flags |= TIFF_MYBUFFER;
	}
	if (tif->tif_rawdata == NULL) {
		TIFFError(module,
		    GetString(IDS_SCAN_NO_SPACE),//"%s: No space for data buffer at scanline %ld",
		    tif->tif_name, (long) tif->tif_row);
		tif->tif_rawdatasize = 0;
		return (0);
	}
	return (1);
}

/*
 * Set state to appear as if a
 * strip has just been read in.
 */
static int
TIFFStartStrip(TIFF* tif, tstrip_t strip)
{
	TIFFDirectory *td = &tif->tif_dir;

	if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
		if (!(*tif->tif_setupdecode)(tif))
			return (0);
		tif->tif_flags |= TIFF_CODERSETUP;
	}
	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_rawcc = td->td_stripbytecount[strip];
	return ((*tif->tif_predecode)(tif,
			(tsample_t)(strip / td->td_stripsperimage)));
}

/*
 * Set state to appear as if a
 * tile has just been read in.
 */
static int
TIFFStartTile(TIFF* tif, ttile_t tile)
{
	TIFFDirectory *td = &tif->tif_dir;

	if ((tif->tif_flags & TIFF_CODERSETUP) == 0) {
		if (!(*tif->tif_setupdecode)(tif))
			return (0);
		tif->tif_flags |= TIFF_CODERSETUP;
	}
	tif->tif_curtile = tile;
	tif->tif_row =
	    (tile % TIFFhowmany(td->td_imagewidth, td->td_tilewidth)) *
		td->td_tilelength;
	tif->tif_col =
	    (tile % TIFFhowmany(td->td_imagelength, td->td_tilelength)) *
		td->td_tilewidth;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_rawcc = td->td_stripbytecount[tile];
	return ((*tif->tif_predecode)(tif,
			(tsample_t)(tile/td->td_stripsperimage)));
}

static int
TIFFCheckRead(TIFF* tif, int tiles)
{
	if (tif->tif_mode == O_WRONLY) {
		TIFFError(tif->tif_name, GetString(IDS_SCAN_FILE_BAD_OPEN));//"File not open for reading");
		return (0);
	}
	if (tiles ^ isTiled(tif)) {
		TIFFError(tif->tif_name, tiles ?
			GetString(IDS_NO_TILE_FROM_STRIP) : //"Can not read tiles from a stripped image" :
		    GetString(IDS_NO_SCAN_FROM_TILE));//"Can not read scanlines from a tiled image");
		return (0);
	}
	return (1);
}

void
_TIFFNoPostDecode(TIFF* tif, tidata_t buf, tsize_t cc)
{
    (void) tif; (void) buf; (void) cc;
}

void
_TIFFSwab16BitData(TIFF* tif, tidata_t buf, tsize_t cc)
{
    (void) tif;
    assert((cc & 1) == 0);
    TIFFSwabArrayOfShort((uint16*) buf, cc/2);
}

void
_TIFFSwab32BitData(TIFF* tif, tidata_t buf, tsize_t cc)
{
    (void) tif;
    assert((cc & 3) == 0);
    TIFFSwabArrayOfLong((uint32*) buf, cc/4);
}

void
_TIFFSwab64BitData(TIFF* tif, tidata_t buf, tsize_t cc)
{
    (void) tif;
    assert((cc & 7) == 0);
    TIFFSwabArrayOfDouble((double*) buf, cc/8);
}
