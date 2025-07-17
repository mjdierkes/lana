/////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2002-2024 UltraVNC Team Members. All Rights Reserved.
//  Copyright (C) 2002 RealVNC Ltd. All Rights Reserved.
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


//
// xzOutStream streams to a compressed data stream (underlying), compressing
// with zlib on the fly.
//
#ifdef _XZ
#ifndef __RDR_xzOutStream_H__
#define __RDR_xzOutStream_H__

#include "OutStream.h"

#define LZMA_API_STATIC
#ifndef _VS2008
#include <stdint.h>
#endif
#ifdef _XZ
#ifdef _VCPKG
#include  <lzma.h>
#else
#include "../xz/src/liblzma/api/lzma.h"
#endif
#endif

namespace rdr {

  class xzOutStream : public OutStream {

  public:

    // adzm - 2010-07 - Custom compression level
    xzOutStream(OutStream* os=0, int bufSize=0);
    virtual ~xzOutStream();

	void SetCompressLevel(int compression);

    void setUnderlying(OutStream* os);
    void flush();
    int length();

  private:

    void ensure_stream_codec();

    int overrun(int itemSize, int nItems);

    OutStream* underlying;
    int bufSize;
    int offset;
#ifdef _XZ
	lzma_stream* ls;
	lzma_options_lzma ls_options;
#endif
    U8* start;
  };

} // end of namespace rdr

#endif
#endif
