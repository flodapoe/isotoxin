#pragma once

#include "toolset/toolset.h"

template<typename STR> struct wraptranslate;
template<> struct wraptranslate<ts::wsptr> : public ts::wsptr
{
    wraptranslate(const ts::wsptr &t):ts::wsptr(t) {}
    wraptranslate<ts::wstr_c> operator / (const ts::wsptr&r) const;
};
template<> struct wraptranslate<ts::wstr_c> : public ts::wstr_c
{
    wraptranslate( const ts::wstr_c & t ):ts::wstr_c(t) {}
    wraptranslate &operator / (const ts::wsptr&r)
    {
        int ins = this->find_pos('$');
        if (ASSERT(ins >=0))
            this->replace(ins,1, r);
        return *this;
    }
};
inline wraptranslate<ts::wstr_c> wraptranslate<ts::wsptr>::operator / (const ts::wsptr&r) const
{
    return wraptranslate<ts::wstr_c>(*this) / r;
}


#ifndef _FINAL
INLINE const wraptranslate<ts::wsptr> __translation(const ts::wsptr &txt)
{
    WARNING("Translation not found! Use rasp loc. (%s)", ts::str_c(txt).cstr());
    return wraptranslate<ts::wsptr>(txt);
}
#endif
const wraptranslate<ts::wsptr> __translation(const ts::wsptr &txt, int tag);

#define TTT( txt, ... ) __translation(CONSTWSTR(txt),__VA_ARGS__)

ts::wstr_c lasterror();


INLINE time_t now()
{
    time_t t;
    _time64(&t);
    return t;
}

template<typename TCHARACTER> ts::str_t<TCHARACTER> maketag_color( ts::TSCOLOR c )
{
    ts::str_t<TCHARACTER> s( CONSTSTR(TCHARACTER,"<color=#") );
    s.append_as_hex(ts::RED(c))
    .append_as_hex(ts::GREEN(c))
    .append_as_hex(ts::BLUE(c))
    .append_as_hex(ts::ALPHA(c))
    .append(CONSTSTR(TCHARACTER,">"));
    return s;
}

template<typename TCHARACTER> ts::str_t<TCHARACTER> colorize( const ts::sptr<TCHARACTER> &t, ts::TSCOLOR c)
{
    return maketag_color<TCHARACTER>(c).append(t).append(CONSTSTR(TCHARACTER,"</color>"));
}


template<typename TCHARACTER> ts::str_t<TCHARACTER> maketag_outline(ts::TSCOLOR c)
{
    ts::str_t<TCHARACTER> s(CONSTSTR(TCHARACTER, "<outline=#"));
    s.append_as_hex(ts::RED(c))
        .append_as_hex(ts::GREEN(c))
        .append_as_hex(ts::BLUE(c))
        .append_as_hex(ts::ALPHA(c))
        .append(CONSTSTR(TCHARACTER, ">"));
    return s;
}
template<typename TCHARACTER> ts::str_t<TCHARACTER> maketag_shadow(ts::TSCOLOR c, int len = 2)
{
    ts::str_t<TCHARACTER> s(CONSTSTR(TCHARACTER, "<shadow="));
    s.append_as_int(len).append(CONSTSTR(TCHARACTER, ";#"))
        .append_as_hex(ts::RED(c))
        .append_as_hex(ts::GREEN(c))
        .append_as_hex(ts::BLUE(c))
        .append_as_hex(ts::ALPHA(c))
        .append(CONSTSTR(TCHARACTER, ">"));
    return s;
}


void text_convert_to_bbcode(ts::wstr_c &text);
void text_close_bbcode(ts::wstr_c &text);
void text_convert_char_tags(ts::wstr_c &text);
void text_adapt_user_input(ts::wstr_c &text); // before print
void text_prepare_for_edit(ts::wstr_c &text);

typedef ts::sstr_t<4> SLANGID;
typedef ts::array_inplace_t<SLANGID, 0> SLANGIDS;

SLANGID detect_language();
menu_c list_langs( SLANGID curlng, MENUHANDLER h );

bool new_version();
bool new_version( const ts::asptr &current, const ts::asptr &newver );

class sound_capture_handler_c
{
protected:

    s3::Format capturefmt;
    bool capture = false;

    sound_capture_handler_c();
    ~sound_capture_handler_c();

    void start_capture();
    void stop_capture();

public:
    s3::Format &getfmt() { return capturefmt; }
    bool is_capture() const { return capture; };
    virtual void datahandler(const void *data, int size) = 0;
    virtual s3::Format *formats(int &count) { count = 0; return nullptr; };

};



// isotoxin gmsgs


enum isogmsg_e
{
    ISOGM_SEND_MESSAGE = GM_COUNT,  // signal to send message
    ISOGM_MESSAGE,                  // message itself // MULTIPASS!!!
    ISOGM_PROFILE_TABLE_LOADED,
    ISOGM_PROFILE_TABLE_SAVED,
    ISOGM_UPDATE_CONTACT,           // new or update contact - signal from protocol
    ISOGM_UPDATE_CONTACT_V,         // visual update (add to list, or update caption)
    ISOGM_SELECT_CONTACT,
    ISOGM_CMD_RESULT,
    ISOGM_PROTO_LOADED,
    ISOGM_INCOMING_MESSAGE,
    ISOGM_DELIVERED,
    ISOGM_FILE,
    ISOGM_CHANGED_PROFILEPARAM,
    ISOGM_NOTICE,
    ISOGM_SUMMON_POST,              // ������ � conversation post_s � ������ ��� ����
    ISOGM_AV,
    ISOGM_CALL_STOPED,
    ISOGM_METACREATE,
    ISOGM_APPRISE,
    ISOGM_NEWVERSION,
    ISOGM_SOMEUNREAD,

    ISOGM_COUNT,
};

template<> struct gmsg<ISOGM_NEWVERSION> : public gmsgbase
{
    gmsg(ts::asptr ver) :gmsgbase(ISOGM_NEWVERSION), ver(ver) {}
    ts::sstr_t<-16> ver;
};

enum profile_table_e;
template<> struct gmsg<ISOGM_PROFILE_TABLE_LOADED> : public gmsgbase
{
    gmsg(profile_table_e tabi) :gmsgbase(ISOGM_PROFILE_TABLE_LOADED),tabi(tabi) {}
    profile_table_e tabi;
};
template<> struct gmsg<ISOGM_PROFILE_TABLE_SAVED> : public gmsgbase
{
    gmsg(profile_table_e tabi) :gmsgbase(ISOGM_PROFILE_TABLE_SAVED), tabi(tabi) {}
    profile_table_e tabi;
};

template<> struct gmsg<ISOGM_DELIVERED> : public gmsgbase
{
    gmsg(uint64 utag) :gmsgbase(ISOGM_DELIVERED), utag(utag) {}
    uint64 utag;
};

template<> struct gmsg<ISOGM_CMD_RESULT> : public gmsgbase
{
    gmsg(commands_e cmd, cmd_result_e rslt) :gmsgbase(ISOGM_CMD_RESULT), cmd(cmd), rslt(rslt) {}
    commands_e cmd;
    cmd_result_e rslt;
};


template<> struct gmsg<ISOGM_PROTO_LOADED> : public gmsgbase
{
    gmsg(int id) :gmsgbase(ISOGM_PROTO_LOADED), id(id)
    {
    }
    int id;
};

struct post_s;
template<> struct gmsg<ISOGM_SUMMON_POST> : public gmsgbase
{
    gmsg(const post_s &post, rectengine_c ** unread = nullptr) :gmsgbase(ISOGM_SUMMON_POST), post(post), unread(unread), replace_post(false)
    {
    }
    gmsg(const post_s &post, bool replace_post) :gmsgbase(ISOGM_SUMMON_POST), post(post), unread(nullptr), replace_post(replace_post)
    {
    }
    rectengine_c **unread; // ���� ����� ����� ������ ��� readtime �������� �������� - ���� ��������� ������������� ������� ���������� - �� ���� ��������������
    const post_s &post;
    bool replace_post;
    void operator =(gmsg &) UNUSED;
};


enum profileparam_e
{
    PP_USERNAME,
    PP_USERSTATUSMSG,
    PP_GENDER,
    PP_ONLINESTATUS,
    PP_AVATAR,
    PP_MSGOPTIONS,

    PP_MICDEVICE,
};
template<> struct gmsg<ISOGM_CHANGED_PROFILEPARAM> : public gmsgbase
{
    gmsg(profileparam_e pp, const ts::wstr_c &s) :gmsgbase(ISOGM_CHANGED_PROFILEPARAM), pp(pp), s(s) {}
    gmsg(profileparam_e pp) :gmsgbase(ISOGM_CHANGED_PROFILEPARAM), pp(pp) {}
    profileparam_e pp;
    ts::wstr_c s;
};
//

bool check_profile_name(const ts::wstr_c &);

bool check_netaddr( const ts::asptr & );

void path_expand_env(ts::wstr_c &path);

struct data_block_s
{
    const void *data;
    int datasize;
    data_block_s(const void *data, int datasize) :data(data), datasize(datasize) {}
};

struct ipcw : public ts::tmp_buf_c
{
    ipcw( commands_e cmd )
    {
        tappend<data_header_s>( data_header_s(cmd) );
    }

    template<typename T> ipcw & operator<<(const T &v) { TS_STATIC_CHECK(std::is_pod<T>::value, "T is not pod"); tappend<T>(v); return *this; }
    template<> ipcw & operator<<(const data_block_s &d) { tappend<int>(d.datasize); append_buf(d.data,d.datasize); return *this; }
    template<> ipcw & operator<<(const ts::asptr &s) { tappend<unsigned short>((unsigned short)s.l); append_buf(s.s,s.l); return *this; }
    template<> ipcw & operator<<(const ts::wsptr &s) { tappend<unsigned short>((unsigned short)s.l); append_buf(s.s,s.l*sizeof(ts::wchar)); return *this; }
    template<> ipcw & operator<<(const ts::str_c &s) { tappend<unsigned short>((unsigned short)s.get_length()); append_buf(s.cstr(), s.get_length()); return *this; }
    template<> ipcw & operator<<(const ts::wstr_c &s) { tappend<unsigned short>((unsigned short)s.get_length()); append_buf(s.cstr(), s.get_length()*sizeof(ts::wchar)); return *this; }
    template<> ipcw & operator<<(const ts::blob_c &b) { tappend<int>((int)b.size()); append_buf(b); return *this; }
};

struct isotoxin_ipc_s
{
    typedef fastdelegate::FastDelegate< bool( ipcr )  > datahandler_func;
    typedef fastdelegate::FastDelegate< bool () > idlejob_func;

    HANDLE plughost;
    ipc::ipc_junction_s junct;
    ts::str_c tag;
    int version = 0;
    bool ipc_ok = false;
    isotoxin_ipc_s(const ts::str_c &tag, datahandler_func);
    ~isotoxin_ipc_s();

    datahandler_func datahandler;

    void wait_loop( idlejob_func idlefunc );
    void something_happens() { junct.idlejob(); }

    static ipc::ipc_result_e handshake_func( void *, void *, int );
    static ipc::ipc_result_e processor_func( void *, void *, int );
    static ipc::ipc_result_e wait_func( void * );
    void send( const ipcw &data )
    {
        junct.send(data.data(), data.size());
    }

};

// leeches

struct leech_fill_parent_s : public autoparam_i
{
    leech_fill_parent_s() {}
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_fill_parent_rect_s : public autoparam_i
{
    ts::irect mrect;
    leech_fill_parent_rect_s(const ts::irect &mrect):mrect(mrect) {}
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_dock_left_s : public autoparam_i
{
    int width;
    leech_dock_left_s(int width) :width(width) {}
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_dock_top_s : public autoparam_i
{
    int height;
    leech_dock_top_s(int height) :height(height) {}
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_dock_bottom_center_s : public autoparam_i
{
    int width;
    int height;
    int x_space;
    int y_space;
    int index;
    int num;
    leech_dock_bottom_center_s(int width, int height, int x_space = 0, int y_space = 0, int index = 0, int num = 1) :width(width), height(height), x_space(x_space), y_space(y_space), index(index), num(num){}
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_dock_right_center_s : public autoparam_i
{
    int width;
    int height;
    int x_space;
    int y_space;
    int index;
    int num;
    leech_dock_right_center_s(int width, int height, int x_space = 0, int y_space = 0, int index = 0, int num = 1) :width(width), height(height), x_space(x_space), y_space(y_space), index(index), num(num) {}
    void update_ctl_pos();
    /*virtual*/ void i_leeched( guirect_c &to ) override { __super::i_leeched(to); update_ctl_pos(); };
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_at_right : public autoparam_i
{
    int space;
    guirect_c::sptr_t of;
    leech_at_right(guirect_c *of, int space) :of(of), space(space)
    {
        of->leech(this);
    }
    /*virtual*/ void i_leeched(guirect_c &to) override
    {
        if (&to != of) __super::i_leeched(to);
        if (&to == owner)
        {
            evt_data_s d;
            sq_evt(SQ_RECT_CHANGED, of->getrid(), d);
        }
    };
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_at_left : public autoparam_i
{
    int space;
    guirect_c::sptr_t of;
    leech_at_left(guirect_c *of, int space) :of(of), space(space)
    {
        of->leech(this);
    }
    /*virtual*/ void i_leeched(guirect_c &to) override
    {
        if (&to != of) __super::i_leeched(to);
        if (&to == owner)
        {
            evt_data_s d;
            sq_evt(SQ_RECT_CHANGED, of->getrid(), d);
        }
    };
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};


struct leech_save_proportions_s : public autoparam_i
{
    static const int divider = 10000;
    ts::str_c cfgname;
    ts::str_c defaultv;
    leech_save_proportions_s(const ts::asptr& cfgname, const ts::asptr& defvals) :cfgname(cfgname), defaultv(defvals)
    {
    }
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};

struct leech_save_size_s : public autoparam_i
{
    ts::str_c cfgname;
    ts::ivec2 defaultsz;
    leech_save_size_s(const ts::asptr& cfgname, const ts::ivec2& defsz) :cfgname(cfgname), defaultsz(defsz)
    {
    }
    virtual bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};