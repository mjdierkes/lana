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
// xzInStream streams from a compressed data stream ("underlying"),
// decompressing with zlib on the fly.
//
#ifdef _XZ
#ifndef __RDR_xzInStream_H__
#define __RDR_xzInStream_H__

#pragma once

#include "InStream.h"

#define LZMA_API_STATIC
#include <stdint.h>

#ifdef _VCPKG
#include  <lzma.h>
#else
#include "../xz/src/liblzma/api/lzma.h"
#endif

namespace rdr {

  class xzInStream : public InStream {

  public:

    xzInStream(int bufSize=0);
    virtual ~xzInStream();

    void setUnderlying(InStream* is, int bytesIn);
    void reset();
    int pos();

  private:

    void ensure_stream_codec();

    int overrun(int itemSize, int nItems);
    void decompress();

    InStream* underlying;
    int bufSize;
    int offset;
    lzma_stream* ls;
    int bytesIn;
    U8* start;
  };

} // end of namespace rdr

#endif
#endif
