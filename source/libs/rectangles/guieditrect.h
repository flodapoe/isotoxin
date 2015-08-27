/*
    (C) 2010-2015 TAV, ROTKAERMOTA
*/
#pragma once

class application_c;
extern application_c *g_app;

#define SEPARATORS L" \n\t:;\"',./?`~!@#$%^&*()-+=\\|{}"
#define IS_WORDB(c) (c==0||ts::CHARz_find(SEPARATORS,c)>=0)

class gui_textedit_c : public gui_control_c
{
    DUMMY( gui_textedit_c );

    GM_RECEIVER( gui_textedit_c, GM_HEARTBEAT );

    void ctx_menu_cut(const ts::str_c &);
    void ctx_menu_paste(const ts::str_c &);
    void ctx_menu_copy(const ts::str_c &);
    void ctx_menu_del(const ts::str_c &);
    void ctx_menu_selall(const ts::str_c &);
    bool ctxclosehandler(RID, GUIPARAM);

    ts::wstr_c placeholder;
	ts::drawable_bitmap_c texture;
    sbhelper_s sbhelper;
	ts::TSCOLOR caret_color         = ts::ARGB(0,0,0),
                color               = ts::ARGB(0,0,0),
                selection_color     = ts::ARGB(255,255,0),
                selection_bg_color  = ts::ARGB(100,100,255),
                outline_color       = 0, //ts::ARGB(0,0,0),
                placeholder_color   = ts::ARGB(0xa0,0xa0,0xa0);
	const ts::font_desc_c *font = &ts::g_default_text_font;
	ts::wchar password_char = 0;

public:
	struct active_element_s
	{
		int advance = 0;

        static active_element_s * fromchar( ts::wchar ch ) // hint! any valid pointers of normally allocated objects has always lower bit 0
        {
            return (active_element_s*)(((UINT_PTR)ch << 16)|1);
        }
        bool is_char() const
        {
            return ((UINT_PTR)this & 1); // hint! lower bit of valid pointer is always 0
        }
        ts::wchar as_char() const
        {
            return ((UINT_PTR)this >> 16);
        }

        virtual void release()
        {
            if (ASSERT(!is_char() && 0 != (UINT_PTR)this))
            {
                TSDEL( this );
            }
        }

        virtual bool hand_cursor() const {return false;}
        virtual ts::wstr_c to_wstr() const = 0;
        virtual ts::str_c to_utf8() const = 0;
        virtual void update_advance(ts::font_c *f) {};
        virtual void setup( const ts::ivec2 &pos, ts::glyph_image_s &gi ) = 0;

        active_element_s() {}
        virtual ~active_element_s() {}
	};
private:

	class text_element_c
	{
		text_element_c(const text_element_c &) UNUSED;
		void operator=(const text_element_c &) UNUSED;

	public:
		active_element_s *p;

		text_element_c(active_element_s *p) : p(p) {}//absorb constructor
		text_element_c(ts::wchar ch) {p = active_element_s::fromchar(ch);}
		~text_element_c() {if (!p->is_char()) p->release();}

        void operator=(active_element_s *_p)
        {
            if (!p->is_char()) p->release();
            p = _p;
        }

		bool operator==(ts::wchar ch) const {return p->is_char() && p->as_char() == ch;}

		ts::wchar get_char_fast() const {return p->as_char();}
		bool     is_char() const {return p->is_char();}
		ts::wchar get_char() const {return p->is_char() ? p->as_char() : 0;}
		int advance(ts::font_c *f) const {return p->is_char() ? (*f)[p->as_char()].advance : p->advance;}
		void update_advance(ts::font_c *f) {if (!p->is_char()) p->update_advance(f);}
	};
	ts::array_inplace_t<text_element_c,512> text; // text
	ts::tbuf_t<ts::ivec2> lines; // lines of text
	int start_sel, caret_line, caret_offset, scroll_left;
	float wheel_scroll_step_size = 50.0f;
	int caret_width = 2, baseline_offset = 0, chars_limit = 0;
	int margin_left = 0, margin_right = 0, margin_top = 0, margin_bottom = 0;
    ts::irect caretrect = ts::irect(0);


    bool focus() const;
    bool selection_disallowed() const;

    bool show_caret() const
    {
        return flags.is(F_CARET_SHOW) && is_enabled() && focus();
    }

    ts::ivec2 size() const
    {
        return get_client_area().size();
    }

    bool summoncontextmenu();
    void redraw(bool redtraw_texture = true);
    void prepare_texture();
    void run_heartbeat();
    bool invert_caret(RID, GUIPARAM);

    bool kbd_processing_(system_query_e qp, ts::wchar charcode, int scan);

	//void onChangeScrollPos(float f) {scrollTo((int)f);}

	int text_el_advance(int index) const { if (index >= text.size()) return 0; return !password_char ? text[index].advance((*font)) : (*(*font))[password_char].advance;}
	bool text_replace(int pos, int num, const ts::wsptr &str, active_element_s **el, int len, bool updateCaretPos = true);
	bool text_replace(int pos, int num, const ts::wsptr &str, bool updateCaretPos = true);
	bool text_replace(int cp, const ts::wsptr &str) {return start_sel == -1 ? text_replace(cp, 0, str) : text_replace(ts::tmin(cp, start_sel), ts::tabs(start_sel-cp), str);}
	bool text_erase(int pos, int num) {return text_replace(pos, num, ts::wstr_c(), nullptr, 0);}
	bool text_erase(int cp) {return text_erase(ts::tmin(cp, start_sel), ts::tabs(start_sel-cp));}
	ts::wstr_c text_substr(int start, int count) const;
    ts::str_c text_substr_utf8(int start, int count) const;
	void scroll_to_caret();
	bool cut_(int cp, bool copy2clipboard = true);//used internally
	bool copy_(int cp);//used internally
	void paste_(int cp);//used internally
	bool prepare_lines(int startChar = 0); // split text to lines

    struct kbd_press_callback_s
    {
        GUIPARAMHANDLER handler;
        int scancode; // negative means ctrl+scancode
    };
    ts::array_inplace_t<kbd_press_callback_s, 1> kbdhandlers;

    NUMGEN_START(fff, 0);
#define DECLAREBIT( fn ) static const ts::flags32_s::BITS fn = FLAGS_FREEBITSTART << NUMGEN_NEXT(fff)
    DECLAREBIT( F_READONLY );
    DECLAREBIT( F_TEXTUREDIRTY );
    DECLAREBIT( F_LINESDIRTY );
    DECLAREBIT( F_NOFOCUS );
    DECLAREBIT( F_TRANSPARENT_ME ); // skip mouse clicks except active elements
    DECLAREBIT( F_MULTILINE );
    DECLAREBIT( F_HEARTBEAT );
    DECLAREBIT( F_CARET_SHOW ); // blinking flag
    DECLAREBIT( F_HANDCURSOR );
    DECLAREBIT( F_ARROW_CURSOR );
    DECLAREBIT( F_DISABLE_CARET ); // no caret - no selections
    DECLAREBIT( F_SBALWAYS );
    DECLAREBIT( F_SBHL );
    DECLAREBIT( F_IGNOREFOCUSCHANGE );
    DECLAREBIT( F_PREV_SB_VIS );

protected:
    DECLAREBIT( F_TEXTEDIT_FREBITSTART ); // free bit start for child
    FREE_BIT_START_CHECK( FLAGS_FREEBITSTART, NUMGEN_NEXT(fff) - 1 );
#undef DECLAREBIT
    gui_textedit_c() {}
public:

	gui_textedit_c(initial_rect_data_s &data);
	~gui_textedit_c();

    void set_font(const ts::font_desc_c *f)
    {
        font = &safe_font(f);
    }

    typedef fastdelegate::FastDelegate<bool (const ts::wstr_c &)> TEXTCHECKFUNC;
	TEXTCHECKFUNC check_text_func; // check/update text callback

    void register_kbd_callback( GUIPARAMHANDLER handler, int scancode, bool ctrl )
    {
        kbd_press_callback_s &cb = kbdhandlers.add();
        cb.handler = handler;
        cb.scancode = ctrl ? -scancode : scancode;
    }

    virtual void cb_scrollbar_width(int w) {}

	bool is_multiline() const {return flags.is(F_MULTILINE);}
    bool is_vsb() const
    {
        if (is_multiline())
        {
            if (flags.is(F_SBALWAYS)) return true;
            return sbhelper.datasize > size().y;
        }

        return false;
    }
    void set_multiline(bool ml = true) { flags.init(F_MULTILINE, ml); }
	bool is_readonly() const {return flags.is(F_READONLY|F_DISABLED);}
    bool is_disabled_caret() const { return flags.is(F_DISABLE_CARET);}
	void set_readonly(bool ro = true) {flags.init(F_READONLY, ro);}
    void disable_caret(bool dc = true) {flags.init(F_DISABLE_CARET, dc);}
    void arrow_cursor(bool f = true) {flags.init(F_ARROW_CURSOR, f);}
	bool is_empty() const {return text.size() == 0;}
    void set_chars_limit( int cl ) { chars_limit = cl; }
    void set_margins( int left = 0, int rite = 0, int top = 0 ) { margin_left = left; margin_right = rite; margin_top = top; }

    void set_placeholder(const ts::wstr_c &t);
    void set_placeholder(const ts::wstr_c &t, ts::TSCOLOR phcolor) { placeholder_color = phcolor; set_placeholder(t); }

	void set_text(const ts::wstr_c &text, bool setCaretToEnd = false);
    ts::wstr_c get_text_and_fix_pos(int *pos0, int *pos1) const;
	ts::wstr_c get_text() const {return text_substr(0, text.size());}
    ts::str_c get_text_utf8() const {return text_substr_utf8(0, text.size());}
	void insert_text(const ts::wstr_c &t) {text_replace(get_caret_char_index(), t);} //insert text at current cursor pos
	void set_color(ts::TSCOLOR c) { caret_color = color = c; redraw(); }
	void set_password_char(ts::wchar pc) { password_char = pc; }

	int lines_count() const;
	void remove_lines(int n);
	void remove_lines_over(int linesLimit) {int r = lines_count() - linesLimit; if (r > 0) remove_lines(r);}

	void reset_sel() {start_sel = -1; redraw();}
    int sel_size() const { return start_sel < 0 ? 0 : ts::tabs(start_sel-get_caret_char_index()); }

	int  scroll_top() const {return -sbhelper.shift;}
	void scroll_to(int y) {sbhelper.shift = -y; redraw();}

	// caret
	ts::ivec2 get_caret_pos() const;
	void set_caret_pos(ts::ivec2 p); // set caret pos by click pos
	void set_caret_pos(int cp); // set caret pos by index of char of text
	ts::ivec2 get_char_pos(int pos) const; // returns caret pos (offset and line) of char of text
	int get_caret_char_index() const { return lines.get(caret_line).x+caret_offset; }

	// clipboard
    void end()
    {
        caret_offset = lines.last().delta();
        caret_line = lines.count() - 1;
        scroll_to_caret();
    }

    void backspace() { if (text.size() == 0) return; text_erase(get_caret_char_index()-1, 1); }
    void del() {cut_(get_caret_char_index(),false);}
	void cut() {cut_(get_caret_char_index());}
	void copy() {copy_(get_caret_char_index());}
	void paste() {paste_(get_caret_char_index());}
    void selectword();
    void selectall()
    {
        if (flags.is(F_DISABLE_CARET)) return;
        start_sel = 0;
        set_caret_pos(text.size());
        redraw();
    }

	active_element_s *under_mouse_active_element = nullptr;
	ts::ivec2 under_mouse_active_element_pos;
	void insert_active_element(active_element_s *ae, int cp);
	void insert_active_element(active_element_s *ae) {insert_active_element(ae, get_caret_char_index());}

    /*virtual*/ void created() override;
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;

    const ts::font_desc_c &get_font() const { ASSERT(font); return *font; }
};
