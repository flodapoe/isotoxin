#include "toolset.h"
#include "platform.h"
#include "glyphscache.h"

#ifndef TS_SKIP_FREETYPE
#ifdef _MSC_VER
#pragma pack(push,1)
#endif
#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef _MSC_VER
#pragma pack(pop)
#endif
#include <freetype/ftmodapi.h>
#endif

//-V:idata:807

namespace ts
{

font_c::~font_c()
{
	for (aint i=0; i<ARRAY_SIZE(glyphs); ++i)
		if (glyphs[i])
		{
			if (glyphs[i]->outlined) MM_FREE(glyphs[i]->outlined);
			MM_FREE(glyphs[i]);
		}
}

glyph_s &font_c::operator[](wchar c)
{
	if (glyphs[c]) return *glyphs[c];

	FT_Set_Pixel_Sizes( face, font_params.size.x, font_params.size.y );
#if LCD_RENDER_MODE
	CHECK(FT_Load_Char( face, c, font_params.flags | FT_LOAD_RENDER | FT_LOAD_TARGET_LCD ) == 0);
#else
    CHECK(FT_Load_Char(face, c, font_params.flags | FT_LOAD_RENDER) == 0);
#endif

	FT_Bitmap &b = face->glyph->bitmap;

	//some checks
#if LCD_RENDER_MODE
    ASSERT(b.num_grays == 256 && b.pixel_mode == FT_PIXEL_MODE_LCD);
    int pixelwidth = b.width / 3;
    int bytewidth = pixelwidth * 4;
#else
    ASSERT(b.num_grays == 256 && b.pixel_mode == FT_PIXEL_MODE_GRAY);
    ASSERT((unsigned)b.pitch == b.width);//?
    int pixelwidth = b.width;
    int bytewidth = pixelwidth;
#endif
    ASSERT(b.pitch >= 0);
    ASSERT(face->glyph->format == FT_GLYPH_FORMAT_BITMAP);

	glyphs[c] = (glyph_s*)MM_ALLOC(sizeof(glyph_s) + bytewidth*b.rows);

	//fill glyph fields
	glyphs[c]->left	   = face->glyph->bitmap_left;
	glyphs[c]->top	   = face->glyph->bitmap_top;
	glyphs[c]->advance = (face->glyph->advance.x + 32) >> 6; // +32 need to round integer number of pixels due glyph->advance is in fixed point 26.6
	glyphs[c]->width   = pixelwidth;
	glyphs[c]->height  = b.rows;
	glyphs[c]->char_index = FT_Get_Char_Index(face, c);
	glyphs[c]->outlined = nullptr;

#if LCD_RENDER_MODE
    const uint8 *src = (const uint8*)b.buffer;
    uint8 *dst = (uint8*)(glyphs[c] + 1);
    img_helper_copy(dst, src, imgdesc_s(ts::ivec2(pixelwidth, static_cast<int>(b.rows)), 32), imgdesc_s(ts::ivec2(pixelwidth, static_cast<int>(b.rows)), 24, static_cast<int16>(b.pitch)));
#else
    char *dst = (char*)(glyphs[c] + 1);
    memcpy( dst, b.buffer, bytewidth*b.rows );
#endif

	return *glyphs[c];
}

inline unsigned calc_hash(const font_params_s& fp)
{
    aint sz = sizeof(fp.size) + sizeof(fp.flags) + sizeof(fp.additional_line_spacing) + +sizeof(fp.outline_radius) + +sizeof(fp.outline_shift);
    aint dsz = sz + fp.fontname.get_length() + fp.filename.get_length() * 2;
    uint8 *d = (uint8 *)_alloca( dsz );
    memcpy(d, fp.fontname.cstr(), fp.fontname.get_length());
    memcpy(d + fp.fontname.get_length(), fp.filename.cstr(), fp.filename.get_length() * 2);
    memcpy(d + fp.fontname.get_length() + fp.filename.get_length() * 2, &fp.size, sz);

    return calc_hash(d, dsz);
}

str_c font_c::makename_bold()
{
    token<char> t( fontname, '.' );
    pstr_c rawfn = *t;
    ++t;
    bool itl = false;
    for( ;t; ++t)
        if ( t->equals(CONSTASTR("italic")) ) itl = true;

    str_c r( rawfn, CONSTASTR(".bold") ); if (itl) r.append(CONSTASTR(".italic"));
    return r;
}
str_c font_c::makename_light()
{
    token<char> t(fontname, '.');
    pstr_c rawfn = *t;
    ++t;
    bool itl = false;
    for (; t; ++t)
        if (t->equals(CONSTASTR("italic"))) itl = true;

    str_c r(rawfn, CONSTASTR(".light"));
    if (itl) r.append(CONSTASTR(".italic"));
    return r;
}
str_c font_c::makename_italic()
{
    token<char> t(fontname, '.');
    pstr_c rawfn = *t;
    ++t;
    bool bld = false;
    bool lit = false;
    for (; t; ++t)
    {
        if (t->equals(CONSTASTR("bold")) ) bld = true; else
        if (t->equals(CONSTASTR("light"))) lit = true;
    }

    if (bld)
        return str_c(rawfn, CONSTASTR(".bold.italic"));
    if (lit)
        return str_c(rawfn, CONSTASTR(".light.italic"));

    return str_c(rawfn, CONSTASTR(".italic"));
}

namespace
{

    struct font_face_s
    {
	    FT_Face face;
	    blob_c font_file_buffer; // FreeType requirement: buffer of font must be valid until FT_Done_Face, so keep buffer here
	    hashmap_t<font_params_s, font_c> fonts_cache;

	    ~font_face_s()
	    {
		    FT_Done_Face( face );
	    }
    };

    struct scaled_image_key_s
    {
        wstr_c name;
        ivec2 scale;
        bool operator==(const scaled_image_key_s &s) const { return name == s.name && scale == s.scale; }
    };
    static unsigned calc_hash(const scaled_image_key_s &i)
    {
        return calc_hash(i.name) ^ calc_hash(&i.scale, sizeof(i.scale));
    }
    struct scaled_image_container_s : scaled_image_s
    {
        bitmap_c bitmap;
    };

    struct internal_data_s
    {
        FT_Library ftlibrary;
        hashmap_t<wstr_c, font_face_s> font_faces_cache;
	    hashmap_t<scaled_image_key_s, scaled_image_container_s> scaled_images_cache;
	    hashmap_t<str_c, font_params_s> fonts;
	    wstrings_c fonts_dirs;
	    wstrings_c images_dirs;
	    int font_cache_sig;
	    internal_data_s() :font_cache_sig(0)
        {
            //FreeType
            FT_Init_FreeType(&ftlibrary);
#ifdef TT_INTERPRETER_VERSION_40
            FT_UInt     interpreter_version = TT_INTERPRETER_VERSION_40;
            FT_Property_Set( ftlibrary, "truetype", "interpreter-version", &interpreter_version );
#endif // TT_INTERPRETER_VERSION_40

        }
        ~internal_data_s()
        {
            // clear hash tables before kill FreeType
            font_faces_cache.clear();
            scaled_images_cache.clear();
            fonts.clear();
            FT_Done_FreeType( ftlibrary );
        }
    };
}

static_setup<internal_data_s> idata;

void set_fonts_dir(const wsptr &dir, bool add)
{
    if (!add)
        idata().fonts_dirs.clear();
    idata().fonts_dirs.add(dir);
    wstr_c & l = idata().fonts_dirs.get( idata().fonts_dirs.size() - 1 );
    if (!__is_slash(l.get_last_char()))
        l.append_char(NATIVE_SLASH);
}
void set_images_dir(const wsptr &dir, bool add) // for parser
{
    if (!add)
        idata().images_dirs.clear();
	idata().images_dirs.add(dir);

    wstr_c & l = idata().images_dirs.get(idata().images_dirs.size() - 1);
    if (!__is_slash(l.get_last_char()))
        l.append_char(NATIVE_SLASH);
}

blob_c load_image( const wsptr&fn )
{
    return g_fileop->load( idata().images_dirs, fn );
}

bmpcore_exbody_s get_image(const wsptr&name)
{
    scaled_image_key_s sik = { wstr_c(name), ivec2(100) };
    scaled_image_container_s *i = idata().scaled_images_cache.get(sik);
    bmpcore_exbody_s eb;
    if (i)
    {
        eb.m_body = i->pixels;
        eb.m_info.sz.x = i->width;
        eb.m_info.sz.y = i->height;
        eb.m_info.pitch = (uint16)i->pitch;
        eb.m_info.bitpp = 32;
    }
    return eb;
}

void add_image(const wsptr&name, const bitmap_c&bmp, const irect& rect)
{
    bool added;
    scaled_image_key_s sik = { wstr_c(name), ivec2(100)};
    scaled_image_container_s &i = idata().scaled_images_cache.add(sik, added);
    i.bitmap = bmp;
    i.width = rect.width();
    i.height = rect.height();
    i.pitch = bmp.info().pitch;
    i.pixels = bmp.body( rect.lt );
}

void add_image(const wsptr&name, const uint8* data, const imgdesc_s &imgdesc, bool copyin)
{
    bool added;
    scaled_image_key_s sik = { wstr_c(name), ivec2(100) };
    scaled_image_container_s &i = idata().scaled_images_cache.add(sik, added);

    ASSERT(imgdesc.bytepp() == 4);

    if (copyin)
    {
        i.bitmap.create_ARGB(imgdesc.sz);
        img_helper_copy(i.bitmap.body(),data, i.bitmap.info(), imgdesc);

        i.pitch = i.bitmap.info().pitch;
        i.pixels = i.bitmap.body();
    } else
    {
        i.bitmap.clear();
        i.pitch = imgdesc.pitch;
        i.pixels = data;
    }

    i.width = imgdesc.sz.x;
    i.height = imgdesc.sz.y;
}

//const wstr_c &filename, const str_c &fontname, const ivec2 & size, bool hinting = true, int additional_line_spacing = 0, float outline_radius = .2f, float outline_shift = 0

font_c &font_c::buildfont(const str_c &fontname, const font_params_s&fprs)
{
    //int scale = 100;
    const ivec2 &size = fprs.size; //scale ? (fprs.size * (ui_scale(100) * scale/* + 50*/) + 5000) / 10000 : fprs.size;
    bool hinting = fprs.flags != 0;
    int additional_line_spacing = ui_scale(fprs.additional_line_spacing);
    float outline_radius = fprs.outline_radius;
    float outline_shift = fprs.outline_shift;

    internal_data_s &idta = idata();
    wstr_c face(fprs.filename);
	//face.makeLowerCase();
	if (face.find_pos('.') < 0) face.append(CONSTWSTR(".otf"));
	bool added;
	font_face_s &ff = idta.font_faces_cache.add(face, added);
	if (added) // if just added - do initialization
	{
		blob_c buf = g_fileop->load(idta.fonts_dirs, face);
		while (!buf)
		{

#ifdef _WIN32

            // cant load font?
            // try load it from system dir...

            swstr_t<MAX_PATH_LENGTH +32> sysdir( MAX_PATH_LENGTH,false);
            GetWindowsDirectoryW(sysdir.str(), MAX_PATH_LENGTH ); sysdir.set_length();
            if (sysdir.get_last_char() != '\\') sysdir.append_char('\\');
            sysdir.append(CONSTWSTR("fonts\\"));
            sysdir.append(face);
            buf = g_fileop->load(sysdir);
            if (buf) break;
#endif
#ifdef _NIX
            // use nix_fonts.conf to find system linux fonts
            ts::blob_c alsb = g_fileop->load(CONSTWSTR("nix_fonts.conf"));
            ts::wstrmap_c als(ts::from_utf8(alsb.cstr()));
            ts::wstr_c nfn = als.get(face,face);
            buf = g_fileop->load(nfn);
            if (buf) break;

#endif // _NIX

            ERROR("Font '%s' not found. Check path! Current path is: %s", to_str(face).cstr(), to_str(idta.fonts_dirs.join(CONSTWSTR("\r\n"))).cstr());

            // ... or load any other

			auto it = idta.font_faces_cache.begin();
			if (it.key() == face) ++it;
			buf = it->font_file_buffer;
            break; //-V612
		}

		CHECK(FT_New_Memory_Face( idta.ftlibrary, (FT_Byte*)buf.data(), (FT_Long)buf.size(), 0, &ff.face ) == 0);
		ff.font_file_buffer = buf;

		//some checks
		ASSERT(FT_IS_SCALABLE(ff.face));
	}

	font_params_s fp(size, hinting ? 0 : FT_LOAD_NO_HINTING, additional_line_spacing, outline_radius, outline_shift);
    fp.fontname = fontname;
    fp.filename = fprs.filename;
	font_c &f = ff.fonts_cache.add(fp, added);
	if (added)
	{
        f.fontname = fontname;
		f.face = ff.face;
		f.font_params = fp;

		//FT_Set_Pixel_Sizes( f.face, size.x, size.y );
		//f.ascender  = (f.face->size->metrics.ascender  + 32) >> 6;
		//f.descender = (f.face->size->metrics.descender + 32) >> 6;
		//f.height    = (f.face->size->metrics.height    + 32) >> 6;
		float design_to_device_K = (float)size.y/f.face->units_per_EM; // K from font to pixels
		f.ascender  =  lceil(design_to_device_K * f.face->ascender);
		f.descender = ts::lround(design_to_device_K * f.face->descender);
		f.height    = ts::lround(design_to_device_K * f.face->height);
		f.underline_add_y  = ts::lround(design_to_device_K * f.face->underline_position);
		f.uline_thickness = float(design_to_device_K * f.face->underline_thickness);

		memset(f.glyphs, 0, sizeof(f.glyphs));
	}

	return f;
}

int font_c::kerning_ci(int left, int right)
{
	if (!FT_HAS_KERNING(face)) return 0;

	FT_Vector delta;
	/* // optimized version below
	FT_Set_Pixel_Sizes( face, fontParams.size.x, fontParams.size.y );
	FT_Get_Kerning( face, FT_Get_Char_Index(face, left), FT_Get_Char_Index(face, right), FT_KERNING_DEFAULT, &delta );
	return delta.x >> 6;*/
	FT_Get_Kerning( face, left, right, FT_KERNING_UNSCALED, &delta );
	return ts::lround((float)font_params.size.x/face->units_per_EM * delta.x);
}

scaled_image_s *scaled_image_s::load(const wsptr &filename_, const ivec2 &scale)
{
	scaled_image_key_s sik = {wstr_c(filename_), scale};
	bool added;
	scaled_image_container_s &i = idata().scaled_images_cache.add(sik, added);
	if (added)
	{
		i.width = i.height = i.pitch = 0;
		do
		{
            auto loadimage = [](const wsptr& filename_) -> blob_c
            {
                if (filename_.s[0] == '/')
                {
                    tmp_wstr_c fn;
                    fn.set(filename_.skip(1));
                    if (fn.find_pos('.') < 0) fn.append(CONSTWSTR(".png"));
                    return g_fileop->load(fn);
                }

                tmp_wstr_c fn( filename_ );
                if (fn.find_pos('.') < 0) fn.append(CONSTWSTR(".png"));
                return g_fileop->load(idata().images_dirs, fn);
            };
            blob_c buf = loadimage(filename_);
			if (buf && i.bitmap.load_from_file(buf.data(), buf.size()))
            {
				switch (i.bitmap.info().bitpp)
				{
				case 24:
                    {
                        bitmap_c tmp;
                        tmp = i.bitmap.extbody();
                        i.bitmap = tmp;
                    }
				case 32:
					i.width  = tmax(1, (i.bitmap.info().sz.x * scale.x + 50) / 100);
					i.height = tmax(1, (i.bitmap.info().sz.y * scale.y + 50) / 100);
					if (i.width != i.bitmap.info().sz.x || i.height != i.bitmap.info().sz.y)
					{
						bitmap_c temp;
						i.bitmap.resize_to(temp, ref_cast<ivec2>(i.width, i.height), FILTER_LANCZOS3);
						i.bitmap = temp;
					}
                    i.bitmap.premultiply();
                    i.pitch = i.bitmap.info().pitch;
					i.pixels = i.bitmap.body();
					break;
				default:
					WARNING("Unsupported format for scaled image '%s' : %i bpp", to_str(filename_).cstr(), i.bitmap.info().bitpp);
					break;
				}
				break;//ok
			}
			WARNING("Can't load scaled image '%s'", to_str(filename_).cstr());
		} while (false);
	}
	return &i;
}

void font_params_s::setup(const asptr &fparams)
{
    token<char> t(fparams, ',');

    filename = to_wstr(*t);

    ++t; token<char> tt(*t, '/');

    size.x = tt->as_int();
    ++tt; size.y = tt->as_int(size.x);
    ++t; flags = t ? t->as_int(1) : 1;
    ++t; additional_line_spacing = t ? t->as_int() : 0;
    ++t; outline_radius = t ? t->as_float(.2f) : .2f;
    ++t; outline_shift = t ? tmin(t->as_float(.99f), .99f) : 0;
    dirty = true;
}

void add_font(const asptr &name, const font_params_s &fparams)
{
    font_params_s &fp = idata().fonts.add(name);
    fp = fparams;
}

font_c *font_desc_c::get_font() const
{
	if (font && !CHECK(fontsig == idata().font_cache_sig))
		const_cast<font_desc_c*>(this)->update();
    if(!CHECK(font))
    {
        if (this == &g_default_text_font) DEBUG_BREAK();
        return g_default_text_font.get_font();
    }
	return font;
}

bool font_desc_c::assign(const str_c &fontname_)
{
    fontname = fontname_;
    update();
    return true;
}

void font_desc_c::update()
{
    font_params_s *fp_ = idata().fonts.get(fontname);
    if (!CHECK(fp_, "Font not found: " << fontname))
    {
        fontname = CONSTASTR("default");
        fp_ = idata().fonts.get(fontname);
    }
    fp = *fp_;

    if (fp.dirty || fontsig != idata().font_cache_sig)
    {
        font = &font_c::buildfont(fontname, fp);
        fontsig = idata().font_cache_sig;
        fp.dirty = false;
    }
}


void clear_glyphs_cache()
{
	for (auto it = idata().font_faces_cache.begin(); it; ++it)
		it->fonts_cache.clear();

	idata().scaled_images_cache.clear(); // also clear images cache
	idata().font_cache_sig++; // increment sig to invalidate all exist font_desc_c (avoid access to broken pointer to font)
}

} // namespace ts
