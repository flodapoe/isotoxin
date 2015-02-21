#pragma once

struct dialog_addcontact_params_s
{
    ts::wstr_c title;
    ts::str_c pubid;
    contact_key_s key;

    dialog_addcontact_params_s() {}
    dialog_addcontact_params_s(const ts::wstr_c &title, const ts::str_c &pubid, const contact_key_s &key ) :title(title), pubid(pubid), key(key)  {}
};

class dialog_addcontact_c;
template<> struct MAKE_ROOT<dialog_addcontact_c> : public _PROOT(dialog_addcontact_c)
{
    dialog_addcontact_params_s prms;
    MAKE_ROOT(drawchecker &dch) : _PROOT(dialog_addcontact_c)(dch) { init(false); }
    MAKE_ROOT(drawchecker &dch, const dialog_addcontact_params_s &prms) :_PROOT(dialog_addcontact_c)(dch), prms(prms) { init(false); }
    ~MAKE_ROOT() {}
};

class dialog_addcontact_c : public gui_isodialog_c
{
    GM_RECEIVER(dialog_addcontact_c, ISOGM_CMD_RESULT);

    int apid = 0;
    ts::wstr_c publicid;
    ts::wstr_c invitemessage;

    dialog_addcontact_params_s inparam;

protected:
    /*virtual*/ int unique_tag() { return UD_ADDCONTACT; }
    /*virtual*/ void created() override;
    /*virtual*/ void getbutton(bcreate_s &bcr) override;
    /*virtual*/ int additions(ts::irect & border) override;
    /*virtual*/ void on_confirm() override;
    
    bool hidectl(RID,GUIPARAM);

    bool public_id_handler( const ts::wstr_c & );
    bool invite_message_handler( const ts::wstr_c & );
    void network_selected( const ts::str_c & );
public:
    dialog_addcontact_c(MAKE_ROOT<dialog_addcontact_c> &data);
    ~dialog_addcontact_c();

    /*virtual*/ ts::wstr_c get_name() const override;
    /*virtual*/ ts::ivec2 get_min_size() const override;
    /*virtual*/ bool sq_evt(system_query_e qp, RID rid, evt_data_s &data) override;
};
