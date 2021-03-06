#pragma once

struct dialog_msgbox_params_s
{
    rtitle_e etitle = title_app;
    ts::wstr_c desc;
    ts::wstr_c ok_button_text;
    ts::wstr_c cancel_button_text;
    ts::wstr_c custom_button_text;
    MENUHANDLER on_ok_h;
    ts::str_c on_ok_par;
    MENUHANDLER on_cancel_h;
    ts::str_c on_cancel_par;
    MENUHANDLER on_custom_h;
    ts::str_c on_custom_par;
    ts::bitmap_c bmp;
    menu_c menu;
    bool bcancel_ = false;
    bool bok_ = true;
    bool bcustom_ = false;

    dialog_msgbox_params_s() {}
    
    dialog_msgbox_params_s(rtitle_e tt,
                           const ts::wstr_c &desc
                           ) :etitle(tt), desc(desc)  {}
    dialog_msgbox_params_s( rtitle_e tt,
                            const ts::wstr_c &desc, const ts::bitmap_c &ibmp
                            ) :etitle( tt ), desc( desc ), bmp( ibmp ) {}

    dialog_msgbox_params_s& bok(const ts::wsptr &t) {ok_button_text = t; return *this;}
    dialog_msgbox_params_s& bcancel(bool f=true, const ts::wsptr &t = ts::wsptr()) {bcancel_ = f; cancel_button_text = t; return *this;}
    dialog_msgbox_params_s& bcustom( bool f = true, const ts::wsptr &t = ts::wsptr() ) { bcustom_ = f; custom_button_text = t; return *this; }
    dialog_msgbox_params_s& on_ok(MENUHANDLER mh, const ts::str_c &par) {on_ok_h = mh; on_ok_par = par; return *this;}
    dialog_msgbox_params_s& on_cancel(MENUHANDLER mh, const ts::str_c &par) {on_cancel_h = mh; on_cancel_par = par; return *this;}
    dialog_msgbox_params_s& on_custom( MENUHANDLER mh, const ts::str_c &par ) { on_custom_h = mh; on_custom_par = par; return *this; }

    dialog_msgbox_params_s& checkboxes( const menu_c m ) { menu = m; return *this; }

    RID summon( bool mainparent );
};

class dialog_msgbox_c;
template<> struct MAKE_ROOT<dialog_msgbox_c> : public _PROOT(dialog_msgbox_c)
{
    dialog_msgbox_params_s prms;
    MAKE_ROOT(bool mainparent, const dialog_msgbox_params_s &prms) : _PROOT(dialog_msgbox_c)(), prms(prms) { init( rect_sys_e(RS_NORMAL | (mainparent ? RS_MAINPARENT : 0)) ); }
    ~MAKE_ROOT() {}
};


class dialog_msgbox_c : public gui_isodialog_c
{
    typedef gui_isodialog_c super;
    dialog_msgbox_params_s m_params;
    ts::array_inplace_t<bcreate_s, 0> m_buttons;
    int height = 190;
    ts::uint32 cbv = 0;
    bool center_text = true;

    bool copy_text(RID, GUIPARAM);
    bool on_enter_press_func(RID, GUIPARAM);

    void updrect_msgbox(const void *, int r, const ts::ivec2 &p);

protected:

    /*virtual*/ ts::UPDATE_RECTANGLE getrectupdate() override { return DELEGATE(this, updrect_msgbox); }

    /*virtual*/ void created() override;
    /*virtual*/ void getbutton(bcreate_s &bcr) override;
    /*virtual*/ int additions(ts::irect & border) override;

    bool on_edit(const ts::wstr_c &);
    /*virtual*/ void on_confirm() override;
    /*virtual*/ void on_close() override;
    bool on_custom(RID, GUIPARAM);
    bool checkboxes( RID, GUIPARAM );
    bool checkboxes1( RID, GUIPARAM );
public:
    dialog_msgbox_c(MAKE_ROOT<dialog_msgbox_c> &data);
    ~dialog_msgbox_c();

    static dialog_msgbox_params_s mb_info(const ts::wstr_c &text);
    static dialog_msgbox_params_s mb_info(const ts::wsptr &text);
    static dialog_msgbox_params_s mb_warning(const ts::wstr_c &text);
    static dialog_msgbox_params_s mb_warning(const ts::wsptr &text);
    static dialog_msgbox_params_s mb_error(const ts::wstr_c &text);
    static dialog_msgbox_params_s mb_error(const ts::wsptr &text);
    static dialog_msgbox_params_s mb_qrcode(const ts::wstr_c &text);
    static dialog_msgbox_params_s mb_avatar( const ts::wstr_c &text, const ts::bitmap_c &bmp );

    /*virtual*/ ts::ivec2 get_min_size() const override;
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};



class dialog_pb_c;
class pb_job_c
{
    DECLARE_EYELET(pb_job_c);
public:
    pb_job_c() {}
    virtual ~pb_job_c() {}

    virtual void on_create(dialog_pb_c *pb) { }; // progress bar gui created
    virtual void on_close() {}; // progress bar gui is about to close
};

struct dialog_pb_params_s
{
    ts::iweak_ptr<pb_job_c> pbproc;
    ts::wstr_c title;
    ts::wstr_c desc;
    ts::wstr_c ok_button_text;
    ts::wstr_c cancel_button_text;
    MENUHANDLER on_ok_h;
    ts::str_c on_ok_par;
    MENUHANDLER on_cancel_h;
    ts::str_c on_cancel_par;
    bool bcancel_ = false;
    bool bok_ = false;

    dialog_pb_params_s() {}
    dialog_pb_params_s( const ts::wstr_c &title, const ts::wstr_c &desc ) :title(title), desc(desc) {}


    dialog_pb_params_s& bok(bool f, const ts::wsptr &t) { bok_ = f; ok_button_text = t; return *this; }
    dialog_pb_params_s& bcancel(bool f = true, const ts::wsptr &t = ts::wsptr()) { bcancel_ = f; cancel_button_text = t; return *this; }
    dialog_pb_params_s& on_ok(MENUHANDLER mh, const ts::str_c &par) { on_ok_h = mh; on_ok_par = par; return *this; }
    dialog_pb_params_s& on_cancel(MENUHANDLER mh, const ts::str_c &par) { on_cancel_h = mh; on_cancel_par = par; return *this; }

    dialog_pb_params_s& setpbproc(pb_job_c *pbp) { pbproc = pbp; return *this; }
};

template<> struct MAKE_ROOT<dialog_pb_c> : public _PROOT(dialog_pb_c)
{
    dialog_pb_params_s prms;
    MAKE_ROOT(bool, const dialog_pb_params_s &prms) : _PROOT(dialog_pb_c)(), prms(prms) { init(RS_NORMAL_MAINPARENT); }
    ~MAKE_ROOT() {}
};


class dialog_pb_c : public gui_isodialog_c
{
    typedef gui_isodialog_c super;
    dialog_pb_params_s m_params;
    ts::array_inplace_t<bcreate_s, 0> m_buttons;

protected:
    /*virtual*/ void created() override;
    /*virtual*/ void getbutton(bcreate_s &bcr) override;
    /*virtual*/ int additions(ts::irect & border) override;

    bool on_edit(const ts::wstr_c &);
    /*virtual*/ void on_confirm() override;
    /*virtual*/ void on_close() override;

    /*virtual*/ void tabselected(ts::uint32 mask) override;

public:
    dialog_pb_c(MAKE_ROOT<dialog_pb_c> &data);
    ~dialog_pb_c();
    static dialog_pb_params_s params(
        const ts::wstr_c &title,
        const ts::wstr_c &desc) {
        return dialog_pb_params_s(title, desc);
    }

    /*virtual*/ ts::uint32 caption_buttons() const override { return 0; }
    /*virtual*/ ts::wstr_c get_name() const override { return m_params.title; }
    /*virtual*/ ts::ivec2 get_min_size() const override { return ts::ivec2(500, 190); }

    //sqhandler_i
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;

    void set_level( float v, const ts::wstr_c &txt );
};



typedef fastdelegate::FastDelegate<bool(const ts::wstr_c &, const ts::str_c &)> TEXTENTEROKFUNC;

struct dialog_entertext_params_s
{
    unique_dialog_e utag = UD_NOT_UNIQUE;
    ts::wstr_c title;
    ts::wstr_c desc;
    ts::wstr_c val;
    ts::str_c  param;
    TEXTENTEROKFUNC okhandler;
    gui_textfield_c::TEXTCHECKFUNC checker = check_always_ok;
    GUIPARAMHANDLER cancelhandler = nullptr;
    RID watch;

    dialog_entertext_params_s() {}
    dialog_entertext_params_s(unique_dialog_e utag,
        const ts::wstr_c &title,
        const ts::wstr_c &desc,
        const ts::wstr_c &val,
        const ts::str_c  &param,
        TEXTENTEROKFUNC okhandler,
        GUIPARAMHANDLER cancelhandler,
        gui_textfield_c::TEXTCHECKFUNC checker,
        RID watch = RID()) :utag(utag), title(title), desc(desc), val(val), param(param), okhandler(okhandler), checker(checker), cancelhandler(cancelhandler), watch(watch) {}
};

class dialog_entertext_c;
template<> struct MAKE_ROOT<dialog_entertext_c> : public _PROOT(dialog_entertext_c)
{
    dialog_entertext_params_s prms;
    MAKE_ROOT(bool mainparent, const dialog_entertext_params_s &prms) :_PROOT(dialog_entertext_c)(), prms(prms) { init( rect_sys_e( RS_NORMAL | (mainparent ? RS_MAINPARENT : 0) ) ); }
    ~MAKE_ROOT() {}
};


class dialog_entertext_c : public gui_isodialog_c
{
    typedef gui_isodialog_c super;
    GM_RECEIVER( dialog_entertext_c, ISOGM_APPRISE );

    guirect_watch_c watch;
    dialog_entertext_params_s m_params;

protected:
    /*virtual*/ int unique_tag() override { return m_params.utag; }
    /*virtual*/ void created() override;
    /*virtual*/ void getbutton(bcreate_s &bcr) override;
    /*virtual*/ int additions(ts::irect & border) override;

    bool on_edit(const ts::wstr_c &, bool);
    /*virtual*/ void on_confirm() override;
    /*virtual*/ void on_close() override;

    bool on_enter_press_func(RID, GUIPARAM);
    bool on_esc_press_func(RID, GUIPARAM);

    bool watchdog(RID, GUIPARAM p);
public:
    dialog_entertext_c(MAKE_ROOT<dialog_entertext_c> &data);
    ~dialog_entertext_c();
    static dialog_entertext_params_s params(
        unique_dialog_e utag,
        const ts::wstr_c &title,
        const ts::wstr_c &desc,
        const ts::wstr_c &val,
        const ts::str_c  &param,
        TEXTENTEROKFUNC okhandler = TEXTENTEROKFUNC(),
        GUIPARAMHANDLER cancelhandler = nullptr,
        gui_textfield_c::TEXTCHECKFUNC checker = check_always_ok,
        RID watch = RID()
        ) {
        return dialog_entertext_params_s(utag, title, desc, val, param, okhandler, cancelhandler, checker, watch);
    }

    /*virtual*/ ts::wstr_c get_name() const override { return m_params.title; }
    /*virtual*/ void tabselected(ts::uint32 mask) override;

    /*virtual*/ ts::ivec2 get_min_size() const override { return ts::ivec2(400, 300); }

    //sqhandler_i
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};


class dialog_about_c : public gui_isodialog_c
{
    typedef gui_isodialog_c super;
    process_animation_s pa;
    bool checking_new_version = false;
    bool check_update_now(RID, GUIPARAM);
    GM_RECEIVER(dialog_about_c, ISOGM_NEWVERSION);

    bool updanim(RID, GUIPARAM);

protected:
    /*virtual*/ int unique_tag() override { return UD_ABOUT; }
    /*virtual*/ void created() override;
    /*virtual*/ void getbutton(bcreate_s &bcr) override;
    /*virtual*/ int additions(ts::irect & border) override;

    void updrect_about(const void *rr, int r, const ts::ivec2 &p);
    /*virtual*/ ts::UPDATE_RECTANGLE getrectupdate() override { return DELEGATE(this, updrect_about); }

public:
    dialog_about_c(initial_rect_data_s &data);
    ~dialog_about_c();

    /*virtual*/ ts::wstr_c get_name() const override;
    /*virtual*/ ts::ivec2 get_min_size() const override;

    //sqhandler_i
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

class incoming_call_panel_c;
template<> struct MAKE_ROOT<incoming_call_panel_c> : public _PROOT(incoming_call_panel_c)
{
    contact_c *c;
    MAKE_ROOT(contact_c *c) :_PROOT(incoming_call_panel_c)(), c(c) { init(RS_TOOL); }
    ~MAKE_ROOT();
};

class incoming_call_panel_c : public gui_control_c
{
    typedef gui_control_c super;
    GM_RECEIVER(incoming_call_panel_c, ISOGM_CALL_STOPED);
    GM_RECEIVER(incoming_call_panel_c, GM_UI_EVENT);
    GM_RECEIVER(incoming_call_panel_c, ISOGM_KILL_CALL_PANEL);

    ts::shared_ptr<contact_c> sender;
    ts::shared_ptr<button_desc_s> buttons[4];
    ts::ivec2 sz = ts::ivec2(0);
    ts::ivec2 tsz = ts::ivec2(0);
    ts::str_c aname;
    int nbuttons = 0;

    const theme_image_s *image = nullptr;

    bool video_supported = false;

    bool b_accept_call_video(RID, GUIPARAM);
    bool b_accept_call(RID, GUIPARAM);
    bool b_reject_call(RID, GUIPARAM);
    bool b_ignore_call(RID, GUIPARAM);

protected:
    /*virtual*/ void created() override;

public:
    incoming_call_panel_c(MAKE_ROOT<incoming_call_panel_c> &data);
    ~incoming_call_panel_c();

    /*virtual*/ ts::ivec2 get_min_size() const override;

    //sqhandler_i
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

class incoming_msg_panel_c;
template<> struct MAKE_ROOT<incoming_msg_panel_c> : public _PROOT( incoming_msg_panel_c )
{
    post_s post;
    ts::shared_ptr<contact_root_c> hist;
    ts::shared_ptr<contact_c> sender;
    ts::str_c text;
    MAKE_ROOT( contact_root_c *h, contact_c *s, const post_s &p ) :_PROOT( incoming_msg_panel_c )( ), hist( h ), sender(s), post(p) { init( RS_TOOL ); }
    ~MAKE_ROOT();
};

class incoming_msg_panel_c : public gui_control_c
{
    typedef gui_control_c super;

    GM_RECEIVER( incoming_msg_panel_c, GM_UI_EVENT );
    GM_RECEIVER( incoming_msg_panel_c, ISOGM_UPDATE_MESSAGE_NOTIFICATION );

    ts::shared_ptr<contact_root_c> hist;
    ts::shared_ptr<contact_c> sender;
    post_s post;

    ts::ivec2 sz = ts::ivec2(300,100);
    ts::ivec2 avarect = ts::ivec2( 60, 60 );

    ts::safe_ptr<gui_message_item_c> msgitm;

    ts::Time appeartime = ts::Time::current();

    bool dip = false;

protected:
    /*virtual*/ void created() override;

    bool endoflife( RID, GUIPARAM );
    bool tick( RID, GUIPARAM );

public:
    int tgt_y = 0;
    incoming_msg_panel_c( MAKE_ROOT<incoming_msg_panel_c> &data );
    ~incoming_msg_panel_c();

    /*virtual*/ ts::ivec2 get_min_size() const override;

    //sqhandler_i
    /*virtual*/ bool sq_evt( system_query_e qp, RID rid, evt_data_s &data ) override;
};

