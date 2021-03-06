#include "toolset.h"

namespace ts
{
    // TGA constants
#define TARGA_TRUECOLOR_IMAGE		2
#define TARGA_BW_IMAGE				3
#define TARGA_TRUECOLOR_RLE_IMAGE	10
#define TARGA_BW_RLE_IMAGE			11

#pragma pack(push, 1)
    struct TGA_Header
    {
        uint8 IDLength;
        uint8 ColorMapType;
        uint8 ImageType;
        uint16 ColorMapStart;
        uint16 ColorMapLength;
        uint8 ColorMapDepth;
        uint16 XOffset;
        uint16 YOffset;
        uint16 Width;
        uint16 Height;
        uint8 Depth;
        uint8 ImageDescriptor;
    };
#pragma pack(pop)

    struct tgaread_s
    {
        TGA_Header *header;
        const uint8 *pixels;
        int buflen;
        int shrinkx;
    };

    static bool tgadatareader(img_reader_s &r, void * buf, int pitch)
    {
        tgaread_s & tgar = ref_cast<tgaread_s>(r.data);

        bool instant_stop = false;
        int tgapitch = tgar.header->Depth / 8 * tgar.header->Width;
        uint8 *tdata;
        int inc;
        if (tgar.header->ImageDescriptor & 0x20) //top-down orientation
        {
            tdata = (uint8 *)buf;
            inc = pitch;
        }
        else //bottom-up orientation
        {
            tdata = (uint8 *)buf + (r.size.y - 1)*pitch;
            inc = -pitch;
        }
        if (tgar.header->ImageType == TARGA_TRUECOLOR_RLE_IMAGE)
        {
            int data_length = tgar.buflen - sizeof(TGA_Header);
            const uint8 *pixels_end = tgar.pixels + data_length;

            TSCOLOR color = 0xFF000000;
            uint8 depth_bytes = tgar.header->Depth / 8;

            shrink_on_read_c sor( ivec2( (int)tgar.header->Width, (int)tgar.header->Height ), tgar.shrinkx );
            sor.do_job( tdata, inc, depth_bytes, [&]( int y, uint8 *row ) {

                if ( r.progress && r.progress( y, tgar.header->Height ) )
                {
                    instant_stop = true;
                    return true;
                }

                uint8 *row_end = row + tgar.header->Width * depth_bytes;

                while ( tgar.pixels < pixels_end )
                {
                    uint8 rle_var = *tgar.pixels++;
                    if ( rle_var > 127 )
                    {
                        int repeat_count = rle_var - 127;
                        memcpy( &color, tgar.pixels, depth_bytes );
                        tgar.pixels += depth_bytes;
                        ASSERT( tgar.pixels <= pixels_end );

                        while ( repeat_count-- )
                        {
                            memcpy( row, &color, depth_bytes );
                            row += depth_bytes;
                        }
                        ASSERT( row <= row_end );
                        if ( row >= row_end )
                            return false;
                    }
                    else {
                        int series_size = ( rle_var + 1 ) * depth_bytes;
                        ASSERT( row + series_size <= row_end );
                        memcpy( row, tgar.pixels, series_size );
                        tgar.pixels += series_size; ASSERT( tgar.pixels <= pixels_end );
                        row += series_size;

                        if ( row >= row_end )
                            return false;
                    }
                }
                return false;
            } );
        }
        else
        {
            if (tgapitch == pitch && inc > 0 && tgar.shrinkx == 0)
            {
                memcpy(tdata, tgar.pixels, tgapitch * r.size.y);

            } else
            {
                int szrow = tgar.header->Width * r.bitpp / 8;
                shrink_on_read_c sor( ivec2( (int)tgar.header->Width, (int)tgar.header->Height ), tgar.shrinkx );
                sor.do_job( tdata, inc, r.bitpp/8, [&]( int y, uint8 *row ) {

                    if ( r.progress && r.progress( y, tgar.header->Height ) )
                    {
                        instant_stop = true;
                        return true;
                    }

                    memcpy( row, tgar.pixels, szrow );
                    tgar.pixels += tgapitch;
                    return false;
                } );
            }
        }

        if ( r.progress )
            r.progress( tgar.header->Height, tgar.header->Height );

        return !instant_stop;
    }

    image_read_func img_reader_s::detect_tga_format(const void *sourcebuf, aint sourcebufsize, const ivec2& limitsize )
    {
        if (sourcebufsize < sizeof(TGA_Header)) return nullptr;

        TGA_Header * header = (TGA_Header*)sourcebuf;
        if (header->ImageType != TARGA_TRUECOLOR_IMAGE && header->ImageType != TARGA_BW_IMAGE && header->ImageType != TARGA_TRUECOLOR_RLE_IMAGE) return nullptr;
        bitpp = header->Depth;
        if (bitpp != 8 && bitpp != 24 && bitpp != 32) return nullptr;

        tgaread_s & tgar = ref_cast<tgaread_s>(data);
        tgar.header = header;
        tgar.pixels = (uint8*)(header + 1) + header->IDLength;

        size.x = header->Width;
        size.y = header->Height;

        if (header->ImageType != TARGA_TRUECOLOR_RLE_IMAGE)
        {
            // addition size check
            int64 datasize = (int64)header->Width * (int64)header->Height * (int64)header->Depth / 8;
            if (datasize < 1 || datasize > (aint)(sourcebufsize - sizeof(TGA_Header)))
                return nullptr;
        }

        tgar.shrinkx = 0;

        if ( limitsize >> 0 )
        {
            while ( size > limitsize )
            {
                size.x >>= 1;
                size.y >>= 1;
                ++tgar.shrinkx;
            }
        }

        if ( size >> 0 );
            else return nullptr;


        tgar.buflen = (int)sourcebufsize;
        return tgadatareader;
    }


    bool save_to_tga_format(buf_c &buf, const bmpcore_exbody_s &bmp, int options)
    {
        buf.clear();
        return false;
    }



} // namespace ts