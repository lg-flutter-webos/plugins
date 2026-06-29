#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Bitmap.hpp"
#include "Debug.hpp"

Bitmap::Bitmap( const char* fn, unsigned int lines, bool bgr )
    : ImageFactory()
    , m_block( nullptr )
    , m_lines( lines )
    , m_alpha( true )
    , m_sema( 0 )
{
    if (!infoFile(fn))
    {
      return;
    }
    bool alpha = (bpp_ == 32) ? true : false;

    if (!decodeFile(fn,32))
    {
      return;
    }

    //RGBA to BGRA
    int size = width_ * height_;
    uint32_t* buf32 = (uint32_t*)buf_;
    for (int i = 0; i < size; i++)
    {
      uint8_t* buf = (uint8_t*)buf32++;
      uint8_t r = buf[0];
      buf[0] = buf[2];
      buf[2] = r;
    }

    // Expand to 4x4 aligned GBRA buffer
    uint8_t* aligned_buf = buf_;
    int aligned_width = ((width_+3)>>2)<<2;
    int aligned_height = ((height_+3)>>2)<<2;

    if (aligned_width != width_ || aligned_height != height_)
    {
      size_t size = aligned_width * aligned_height * 4;
      aligned_buf = (uint8_t*)::malloc(size);
      ::memset(aligned_buf,0xff,size);

      uint8_t* src = buf_;
      uint8_t* dst = aligned_buf;
      int src_stride = width_ * 4;
      int dst_stride = aligned_width * 4;
      for (int i = 0; i < height_; i++)
      {
        ::memcpy(dst,src,src_stride);
        src += src_stride;
        dst += dst_stride;
      }

      ::free(buf_);
      buf_ = aligned_buf;
      width_ = aligned_width;
      height_ = aligned_height;
    }

    // Ready..
    m_alpha = alpha;
    m_block = m_data = (uint32_t*)buf_;
    m_size.x = width_;
    m_size.y = height_;
    m_linesLeft = m_size.y / 4;
    for( int i=0; i<m_size.y/4; i++ )
    {
        m_sema.unlock();
    }
}

Bitmap::Bitmap( const v2i& size )
    : ImageFactory()
    , m_data( new uint32_t[size.x*size.y] )
    , m_block( nullptr )
    , m_lines( 1 )
    , m_linesLeft( size.y / 4 )
    , m_size( size )
    , m_sema( 0 )
{
}

Bitmap::Bitmap( const Bitmap& src, unsigned int lines )
    : ImageFactory()
    , m_lines( lines )
    , m_alpha( src.Alpha() )
    , m_sema( 0 )
{
}

Bitmap::~Bitmap()
{
  if (!buf_) {
    delete[] m_data;
  }
}

void Bitmap::Write( const char* fn )
{
    writePNG(fn,(uint8_t*)m_data,m_size.x,m_size.y,32);
}

const uint32_t* Bitmap::NextBlock( unsigned int& lines, bool& done )
{
    std::lock_guard<std::mutex> lock( m_lock );
    lines = std::min( m_lines, m_linesLeft );
    auto ret = m_block;
    m_sema.lock();
    m_block += m_size.x * 4 * lines;
    m_linesLeft -= lines;
    done = m_linesLeft == 0;
    return ret;
}
