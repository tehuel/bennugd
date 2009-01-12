/*
 *  Copyright � 2006-2009 SplinterGU (Fenix/Bennugd)
 *  Copyright � 2002-2006 Fenix Team (Fenix)
 *  Copyright � 1999-2002 Jos� Luis Cebri�n Pag�e (Fenix)
 *
 *  This file is part of Bennu - Game Development
 *
 *  Bennu is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Bennu is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

/* --------------------------------------------------------------------------- */

#include "mod_map.h"

/* --------------------------------------------------------------------------- */

/*
 * TYPE DEFINITIONS
 */

typedef struct
{
    uint8_t Manufacturer;
    uint8_t Version;
    uint8_t Encoding;
    uint8_t BitsPerPixel;
    int16_t Xmin, Ymin, Xmax, Ymax;
    int16_t HDpi, VDpi;
    uint8_t Colormap[48];
    uint8_t Reserved;
    uint8_t NPlanes;
    int16_t BytesPerLine;
    int16_t PaletteInfo;
    int16_t HscreenSize;
    int16_t VscreenSize;
    uint8_t Filler[54];
} PCXheader ;

/* --------------------------------------------------------------------------- */

static uint8_t colors[256 * 3] ;

/* --------------------------------------------------------------------------- */

/*
 *  FUNCTION : gr_read_pcx
 *
 *  Returns GRAPH newly created from FILENAME.  GRAPH is created in LIB 0
 *
 *  PARAMS:
 *      CONST CHAR * filename: File to be read
 *
 *  RETURN VALUE:
 *      pointer to the newly created GRAPH
 *
 */

GRAPH * gr_read_pcx( const char * filename )
{
    PCXheader header ;
    file *    file ;
    int       width, height, x, y, p, count ;
    GRAPH *   bitmap ;
    uint8_t *   ptr, ch ;

    file = file_open( filename, "rb" ) ;
    if ( !file ) return NULL ;

    file_read( file, &header, sizeof( header ) ) ;

    /* Arrange the data for big-endian machines */
    ARRANGE_WORD( &header.Xmax );
    ARRANGE_WORD( &header.Xmin );
    ARRANGE_WORD( &header.Ymax );
    ARRANGE_WORD( &header.Ymin );
    ARRANGE_WORD( &header.BytesPerLine );
    ARRANGE_WORD( &header.PaletteInfo );
    ARRANGE_WORD( &header.HDpi );
    ARRANGE_WORD( &header.VDpi );
    ARRANGE_WORD( &header.HscreenSize );
    ARRANGE_WORD( &header.VscreenSize );

    width  = header.Xmax - header.Xmin + 1 ;
    height = header.Ymax - header.Ymin + 1 ;

    bitmap = bitmap_new( 0, width, height, ( header.BitsPerPixel == 8 ) ? 8 : 16 ) ;
    if ( !bitmap )
    {
        file_close( file );
        return NULL;
    }

    if ( width > header.BytesPerLine )
    {
        bitmap_destroy( bitmap );
        file_close( file );
        return NULL;
    }

    if ( header.BitsPerPixel == 8 )
    {
        for ( y = 0 ; y < height ; y++ )
            for ( p = 0 ; p < header.NPlanes ; p++ )
            {
                ptr = ( uint8_t * )bitmap->data + bitmap->pitch * y ;
                for ( x = 0 ; x < header.BytesPerLine ; )
                {
                    if ( file_read( file, &ch, 1 ) < 1 )
                    {
                        bitmap_destroy( bitmap );
                        file_close( file );
                        return NULL;
                    }
                    if (( ch & 0xC0 ) == 0xC0 )
                    {
                        count = ( ch & 0x3F ) ;
                        file_read( file, &ch, 1 ) ;
                    }
                    else
                    {
                        count = 1 ;
                    }
                    while ( count-- )
                    {
                        *ptr = ch ;
                        x++ ;
                        ptr += header.NPlanes ;
                    }
                }
            }

        if ( file_read( file, &ch, 1 ) == 1 && ch == 0x0c )
        {
            if ( file_read( file, colors, sizeof( colors ) ) )
            {
                bitmap->format->palette = pal_new_rgb(( uint8_t * )colors );
                pal_refresh( bitmap->format->palette );
                if ( !sys_pixel_format->palette )
                {
                    sys_pixel_format->palette = pal_new( bitmap->format->palette ) ;
                    palette_changed = 1 ;
                }
            }
        }
    }
    else
    {
        bitmap_destroy( bitmap );
        file_close( file );
        return NULL;
    }

    bitmap->modified = 0 ;
    bitmap_analize( bitmap );

    return bitmap ;
}

/* --------------------------------------------------------------------------- */

int gr_load_pcx( const char * mapname )
{
    GRAPH * gr = gr_read_pcx( mapname ) ;
    if ( !gr ) return 0 ;
    gr->code = bitmap_next_code() ;
    grlib_add_map( 0, gr ) ;
    return gr->code ;
}

/* --------------------------------------------------------------------------- */
