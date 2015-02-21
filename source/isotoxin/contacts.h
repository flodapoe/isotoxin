#pragma once

enum message_type_app_e : unsigned
{
    MTA_MESSAGE = MT_MESSAGE,
    MTA_ACTION = MT_ACTION,
    MTA_FRIEND_REQUEST = MT_FRIEND_REQUEST,
    MTA_INCOMING_CALL = MT_INCOMING_CALL,
    MTA_CALL_STOP = MT_CALL_STOP,
    MTA_CALL_ACCEPTED__ = MT_CALL_ACCEPTED,

    // not saved to db
    MTA_SPECIAL,
    MTA_DATE_SEPARATOR,

    message_type_app_values = 99,
    // saved to db

    // hard order!!!!
    MTA_INCOMING_CALL_CANCELED,
    MTA_ACCEPT_OK,   // to acceptor
    MTA_ACCEPTED,    // to requester
    MTA_OLD_REQUEST, // 103
    MTA_INCOMING_CALL_REJECTED,
    MTA_CALL_ACCEPTED, // incoming or outgoing call accepted - same effect
    MTA_HANGUP,
    MTA_UNDELIVERED_MESSAGE,
    MTA_RECV_FILE,
    MTA_SEND_FILE,
};

static_assert( MTA_FRIEND_REQUEST == 2, "!" );
static_assert(MTA_INCOMING_CALL_CANCELED == 100, "!");
static_assert( MTA_OLD_REQUEST == 103, "!" );
static_assert( MTA_INCOMING_CALL_REJECTED == 104, "!" );

INLINE bool is_special_mt(message_type_app_e mt)
{
    if (mt == MTA_UNDELIVERED_MESSAGE) return false;
    return mt > message_type_app_values;
    //switch(mt)
    //{
    //    case MTA_OLD_REQUEST:
    //    case MTA_INCOMING_CALL_CANCELED:
    //    case MTA_INCOMING_CALL_REJECTED:
    //        return true;
    //}
    //return false;
}

struct contact_key_s
{
    DUMMY(contact_key_s);

    int contactid;  // protocol contact id. 0 - group if protoid < 0, self proto if protoid > 0
    int protoid;    // 0 - metacontact. all visible contacts are metacontacts with history and conversation / >0 - protocol contact

    explicit contact_key_s( const ts::asptr&s )
    {
        ts::ref_cast<int64>(*this) = ts::pstr_c(s).as_num<int64>();
    }
    explicit contact_key_s(int contactid = 0, int protoid = 0):contactid(contactid), protoid(protoid) {}

    bool is_meta() const {return protoid == 0 && contactid > 0;}
    bool is_group() const {return contactid == 0 && protoid < 0;}
    int get_group_id() const { return is_group() ? protoid : 0; }
    bool is_self() const {return ts::ref_cast<int64>(*this) == 0; }
    bool is_empty() const {return ts::ref_cast<int64>(*this) == 0; }

    bool operator==(const contact_key_s&oc) const { return ts::ref_cast<int64>(*this) == ts::ref_cast<int64>(oc); }
    bool operator!=(const contact_key_s&oc) const { return ts::ref_cast<int64>(*this) != ts::ref_cast<int64>(oc); }
    int operator()(const contact_key_s&oc) const 
    { 
        return (int)ts::isign( ts::ref_cast<int64>(oc) - ts::ref_cast<int64>(*this) );
    }

    ts::tmp_str_c as_str() const
    {
        ts::tmp_str_c s;
        s.set_as_num<int64>( ts::ref_cast<int64>(*this) );
        return s;
    }
};

INLINE unsigned GetHash(const contact_key_s &ck)
{
    return ts::GetHash(ck.contactid) ^ ts::hash_func(ts::GetHash(ck.protoid));
}

INLINE ts::asptr  calc_message_skin(message_type_app_e mt, contact_key_s sender)
{
    if (MTA_OLD_REQUEST == mt)
        return CONSTASTR("invite");
    if (MTA_ACCEPTED == mt || MTA_ACCEPT_OK == mt)
        return CONSTASTR("invite");
    if (MTA_RECV_FILE == mt || MTA_SEND_FILE == mt)
        return CONSTASTR("file");
    bool by_self = sender.is_self();
    return by_self ? CONSTASTR("mine") : CONSTASTR("other");
}

// initialization: [POST_INIT]
struct post_s
{
    DUMMY(post_s);
    post_s() {}
#if _USE_32BIT_TIME_T
#error "time_t must be 64 bit"
#endif

    static const int type_size_bits = 8;
    static const int options_size_bits = 8;

    uint64 utag;
    time_t time = 0;
    ts::wstr_c message;
    contact_key_s sender;
    contact_key_s receiver;
    unsigned type : type_size_bits;
    unsigned options : options_size_bits;

    message_type_app_e mt() const {return (message_type_app_e)type;}
};
class gui_contact_item_c;
class contact_c : public ts::shared_object
{
    DUMMY(contact_c);
    contact_key_s key;
    ts::shared_ptr<contact_c> metacontact; // valid for non-meta contacts
    ts::str_c pubid;
    ts::wstr_c name;
    ts::wstr_c customname;
    ts::wstr_c statusmsg;
    ts::drawable_bitmap_c avatar;
    time_t readtime = 0; // all messages after readtime considered unread

    contact_state_e state = CS_OFFLINE;
    contact_online_state_e ostate = COS_ONLINE;
    contact_gender_e gender = CSEX_UNKNOWN;

    typedef ts::flags_t<ts::uint32, 0xFFFF> Options;
    Options opts;

    ts::array_shared_t<contact_c, 0> subcontacts; // valid for meta contact
    ts::array_inplace_t<post_s, 128> history; // valid for meta contact

    ts::wstr_c compile_pubid() const;
    ts::wstr_c compile_name() const;
    ts::wstr_c compile_statusmsg() const;

    void setmeta(contact_c *metac)
    {
        /*if (ASSERT((metac->key.is_self() && key.is_group()) || (!is_meta() && !key.is_self() && !key.is_group()))) */
        ASSERT(metac->subpresent(key));
        metacontact = metac;
    }

public:
    ts::safe_ptr<gui_contact_item_c> gui_item;

    static const ts::flags32_s::BITS F_DEFALUT = SETBIT(0);
    //static const ts::flags32_s::BITS F_ACCEPT = SETBIT(0);
    //static const ts::flags32_s::BITS F_REJECT = SETBIT(1);

    static const ts::flags32_s::BITS F_PROTOHIT = SETBIT(27);
    static const ts::flags32_s::BITS F_CALLTONE = SETBIT(28);
    static const ts::flags32_s::BITS F_AV_INPROGRESS = SETBIT(29);
    static const ts::flags32_s::BITS F_RINGTONE = SETBIT(30);
    static const ts::flags32_s::BITS F_RINGTONE_BLINK = SETBIT(31);

    int operator()(const contact_c&oc) const { return key(oc.key); }

    const contact_key_s& getkey() const {return key;}

    contact_c( const contact_key_s &key );
    contact_c();
    ~contact_c();
    void prepare4die()
    {
        metacontact = nullptr; // dec ref count
        subdelall();
    }

    void reselect(bool);

    bool is_protohit( bool strong );
    void protohit(bool f);

    bool achtung() const;

    bool is_meta() const { return key.is_meta() && getmeta() == nullptr; };
    bool is_multicontact() const { return is_meta() || getkey().is_group() || getkey().is_self(); }

    Options get_options() const {return opts;}
    Options &options() {return opts;}

    bool subpresent( const contact_key_s&key ) const
    {
        for( contact_c *c : subcontacts )
            if (c->getkey() == key) return true;
        return false;
    }

    contact_c * subgetadd(const contact_key_s&key)
    {
        for (contact_c *c : subcontacts)
            if (c->getkey() == key) return c;
        contact_c *c = TSNEW( contact_c, key );
        subcontacts.add(c);
        c->setmeta( this );
        return c;
    }
    contact_c * subget(const contact_key_s&key)
    {
        for (contact_c *c : subcontacts)
            if (c->getkey() == key) return c;
        return nullptr;
    }
    int subcount() const {return subcontacts.size();}
    contact_c * subget(int indx)
    {
        return subcontacts.get(indx);
    }

    contact_c * subget_default() const;
    contact_c * subget_for_send() const;

    void subadd( contact_c *c )
    { 
        if (ASSERT(key.is_meta() && !subpresent(c->getkey())))
        {
            subcontacts.add(c);
            c->setmeta( this );
        }
    }
    bool subdel( contact_c *c )
    {
        if (ASSERT(is_meta() && subpresent(c->getkey())))
        {
            c->prepare4die();
            subcontacts.find_remove_slow(c);
        }
        return subcontacts.size() == 0;
    }
    void subdelall()
    {
        for (contact_c *c : subcontacts)
            c->prepare4die();
        subcontacts.clear();
    }
    void subclear() // do not use it!!! this function used only while metacontact creation
    {
        subcontacts.clear();
    }

    template<typename ITR> void subiterate( ITR itr )
    {
        for (contact_c *c : subcontacts)
            itr(c);
    }

    contact_c *getmeta() {return metacontact;}
    const contact_c *getmeta() const {return metacontact;}

    const contact_c *get_historian() const { return getmeta() ? getmeta() : this; }
    contact_c *get_historian() { return getmeta() ? getmeta() : this; }

    const post_s& get_history(int index) const { return history.get(index); }
    int history_size() const {return history.size();}

    time_t nowtime() const
    {
        time_t time = now();
        if (history.size() && history.last().time >= time) time = history.last().time + 1;
        return time;
    }
    void make_time_unique(time_t &t);

    post_s& add_history()
    {
        post_s &p = history.add();
        p.time = nowtime();
        return p;
    }
    post_s& add_history(time_t t)
    {
        int cnt = history.size();
        for (int i=0;i<cnt;++i)
        {
            if ( t < history.get(i).time )
            {
                post_s &p = history.insert(i);
                p.time = t;
                return p;
            }
        }
        post_s &p = history.add();
        p.time = t;
        return p;
    }

    void load_history(int n_last_items);
    void unload_history()
    {
        history.clear();
    }

    const post_s *get_post_by_utag(uint64 utg) const
    {
        for( const post_s &p : history )
            if (p.utag == utg)
                return &p;
        return nullptr;
    }

    template<typename F> void iterate_history( F f ) const
    {
        for( const post_s &p : history )
            if (f(p)) return;
    }
    template<typename F> void iterate_history(F f)
    {
        for (post_s &p : history)
            if (f(p)) return;
    }
    template<typename F> int iterate_history_changetime(F f) // return last iterated post index
    {
        int r = -1;
        ts::tmp_array_inplace_t<post_s, 4> temp;
        for (int i = 0; i<history.size();)
        {
            r = i;
            post_s &p = history.get(i);
            time_t t = p.time;
            bool cont = f(p);
            if (t != p.time)
            {
                temp.add( p );
                history.remove_slow(i);
                if (!cont) break;
                continue;
            }
            if (!cont) break;
            ++i;
        }
        for( post_s &p : temp )
        {
            post_s *x = &add_history(p.time);
            *x = p;
            r = x - history.begin();
        }

        return r;
    }

    void recalc_unread();

    void send_file(const ts::wstr_c &fn);

    void set_pubid( const ts::str_c &pubid_ ) { pubid = pubid_; }
    void set_name( const ts::wstr_c &name_ ) { name = name_; }
    void set_customname( const ts::wstr_c &name_ ) { customname = name_; }
    void set_statusmsg( const ts::wstr_c &statusmsg_ ) { statusmsg = statusmsg_; }
    void set_state( contact_state_e st ) { state = st; }
    void set_ostate( contact_online_state_e ost ) { ostate = ost; }
    void set_gender( contact_gender_e g ) { gender = g; }

    const ts::str_c &get_pubid() const {return pubid;};
    
    const ts::str_c get_pubid_desc() const {return (pubid.is_empty() && is_meta()) ? compile_pubid() : ts::wstr_c(pubid);};
    ts::wstr_c get_description() const { ts::wstr_c t = get_name(); if (t.is_empty()) t.set(get_pubid_desc()); return t; }
    ts::wstr_c get_name(bool metacompile = true) const {return (name.is_empty() && is_meta() && metacompile) ? compile_name() : name;};
    ts::wstr_c get_customname() const { return customname;}
    ts::wstr_c get_statusmsg(bool metacompile = true) const {return (statusmsg.is_empty() && is_meta() && metacompile) ? compile_statusmsg() : statusmsg;};
    contact_state_e get_state() const {return state;}
    contact_online_state_e get_ostate() const {return ostate;}
    contact_gender_e get_gender() const {return gender;}

    contact_state_e get_meta_state() const;
    contact_online_state_e get_meta_ostate() const;
    contact_gender_e get_meta_gender() const;

    bool authorized() const { return get_state() == CS_OFFLINE || get_state() == CS_ONLINE; }

    time_t get_readtime() const { return is_multicontact() ? readtime : 0;}
    void set_readtime(time_t t) { readtime = t; }

    const post_s * fix_history( message_type_app_e oldt, message_type_app_e newt, const contact_key_s& sender = contact_key_s() /* self - empty - no matter */, time_t update_time = 0 /* 0 - no need update */ );

    bool b_accept(RID, GUIPARAM);
    bool b_reject(RID, GUIPARAM);
    bool b_resend(RID, GUIPARAM);
    bool b_kill(RID, GUIPARAM);
    bool b_load(RID, GUIPARAM);
    bool b_accept_call(RID, GUIPARAM);
    bool b_reject_call(RID, GUIPARAM);
    bool b_hangup(RID, GUIPARAM);
    bool b_call(RID, GUIPARAM);
    bool b_cancel_call(RID, GUIPARAM);

    bool b_receive_file(RID, GUIPARAM);
    bool b_receive_file_as(RID, GUIPARAM);
    bool b_refuse_file(RID, GUIPARAM);
    
    

    bool flashing_proc(RID, GUIPARAM);
    bool ringtone(bool activate = true, bool play_stop_snd = true);
    bool is_ringtone() const { return opts.unmasked().is(F_RINGTONE); }
    bool is_ringtone_blink() const { return opts.unmasked().is(F_RINGTONE_BLINK); }
    bool is_av() const { return opts.unmasked().is(F_AV_INPROGRESS); }
    void av( bool f = true );

    bool is_calltone() const { return opts.unmasked().is(F_CALLTONE); }
    bool calltone( bool f = true, bool call_accepted = false );

    bool is_filein() const;

};

contact_c *get_historian(contact_c *sender, contact_c * receiver);

template<> struct gmsg<ISOGM_UPDATE_CONTACT> : public gmsgbase
{
    gmsg() :gmsgbase(ISOGM_UPDATE_CONTACT) {}
    contact_key_s key;
    int mask;
    ts::str_c pubid;
    ts::str_c name;
    ts::str_c statusmsg;
    contact_state_e state = CS_INVITE_SEND;
    contact_online_state_e ostate = COS_ONLINE;
    contact_gender_e gender = CSEX_UNKNOWN;
};

template<> struct gmsg<ISOGM_UPDATE_CONTACT_V> : public gmsgbase
{
    gmsg(contact_c *c) :gmsgbase(ISOGM_UPDATE_CONTACT_V),contact(c) {}
    ts::shared_ptr<contact_c> contact;
};


template<> struct gmsg<ISOGM_INCOMING_MESSAGE> : public gmsgbase
{
    gmsg() :gmsgbase(ISOGM_INCOMING_MESSAGE) {}
    contact_key_s sender;
    time_t create_time;
    message_type_app_e mt;
    ts::str_c msgutf8;
};

template<> struct gmsg<ISOGM_FILE> : public gmsgbase
{
    gmsg() :gmsgbase(ISOGM_FILE) {}
    contact_key_s sender;
    uint64 utag = 0;
    uint64 filesize = 0;
    uint64 offset = 0;
    ts::wstr_c filename;
    ts::buf0_c data;
    file_control_e fctl = FIC_NONE;
};


template<> struct gmsg<ISOGM_SELECT_CONTACT> : public gmsgbase
{
    gmsg(contact_c *c, bool scrollend = true) :gmsgbase(ISOGM_SELECT_CONTACT), contact(c), scrollend(scrollend) {}
    ts::shared_ptr<contact_c> contact;
    bool scrollend;
};

template<> struct gmsg<ISOGM_AV> : public gmsgbase
{
    gmsg(contact_c *c, bool activated) :gmsgbase(ISOGM_AV), multicontact(c), activated(activated) { ASSERT(c->is_multicontact()); }
    ts::shared_ptr<contact_c> multicontact;
    bool activated;
};

template<> struct gmsg<ISOGM_CALL_STOPED> : public gmsgbase
{
    gmsg(contact_c *c, stop_call_e sc) :gmsgbase(ISOGM_CALL_STOPED), subcontact(c), sc(sc) { ASSERT(!c->is_multicontact()); }
    ts::shared_ptr<contact_c> subcontact;
    stop_call_e sc;
};

uint64 uniq_history_item_tag();
template<> struct gmsg<ISOGM_MESSAGE> : public gmsgbase
{
    time_t create_time = 0;
    post_s post;

    gmsg() :gmsgbase(ISOGM_MESSAGE)
    {
        post.utag = uniq_history_item_tag();
        post.time = 0; // initialized after history add
        post.type = MTA_MESSAGE;
    }
    gmsg(contact_c *sender, contact_c *receiver, message_type_app_e mt = MTA_MESSAGE);

    contact_c *sender = nullptr;
    contact_c *receiver = nullptr;
    bool current = false;

    contact_c *get_historian()
    {
        return ::get_historian(sender, receiver);
    }
};



class contacts_c
{
    GM_RECEIVER(contacts_c, ISOGM_PROFILE_TABLE_LOADED);
    GM_RECEIVER(contacts_c, ISOGM_UPDATE_CONTACT);
    GM_RECEIVER(contacts_c, ISOGM_INCOMING_MESSAGE);
    GM_RECEIVER(contacts_c, ISOGM_FILE);
    GM_RECEIVER(contacts_c, ISOGM_CHANGED_PROFILEPARAM);
    GM_RECEIVER(contacts_c, ISOGM_DELIVERED);
    GM_RECEIVER(contacts_c, ISOGM_NEWVERSION);
    

    ts::array_shared_t<contact_c, 8> arr;
    ts::shared_ptr<contact_c> self;

    int find_free_meta_id() const;
    void del( const contact_key_s&k )
    {
        int index;
        if (arr.find_sorted(index, k))
        {
            contact_c *c = arr.get(index);
            c->prepare4die();
            arr.remove_slow(index);
        }

    }

public:

    contacts_c();
    ~contacts_c();

    ts::str_c find_pubid(int protoid) const;
    contact_c *find_subself(int protoid) const;

    contact_c *create_new_meta();

    void nomore_proto(int id);
    bool present_protoid(int id) const;

    bool present( const contact_key_s&k ) const
    {
        int index;
        return arr.find_sorted(index,k);
    }

    contact_c *find( const contact_key_s&k )
    {
        int index;
        if (arr.find_sorted(index, k)) return arr.get(index);
        return nullptr;
    }
    const contact_c *find(const contact_key_s&k) const
    {
        int index;
        if (arr.find_sorted(index, k)) return arr.get(index);
        return nullptr;
    }

    const contact_c & get_self() const { return *self; }
    contact_c & get_self() { return *self; }

    void kill(const contact_key_s &ck);

    void update_meta();
};

extern ts::static_setup<contacts_c> contacts;