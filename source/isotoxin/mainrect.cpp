#include "isotoxin.h"

mainrect_c::mainrect_c(MAKE_ROOT<mainrect_c> &data):gui_control_c(data)
{
    rrect = data.rect;
    mrect = cfg().get<ts::irect>( CONSTASTR("main_rect_monitor"), ts::wnd_get_max_size(data.rect) );
}

mainrect_c::~mainrect_c() 
{
    if (gui) gui->delete_event( DELEGATE(this,saverectpos) );
    cfg().onclosedie( DELEGATE(this, onclosesave) );
}

ts::uint32 mainrect_c::gm_handler( gmsg<ISOGM_APPRISE> & )
{
    if (g_app->F_MODAL_ENTER_PASSWORD) return 0;

    if (getprops().is_collapsed())
        MODIFY(*this).decollapse();
    if (getroot()) getroot()->set_system_focus(true);
    return 0;
}

ts::uint32 mainrect_c::gm_handler(gmsg<ISOGM_CHANGED_SETTINGS> &ch)
{
    if (ch.pass == 0)
        if (PP_ONLINESTATUS == ch.sp)
        {
            ts::irect r(0,0,100,100); // just redraw left top 100px square of main rect. (we hope that app icon less then 100px)
            getengine().redraw( &r );
        }

    return 0;
}


/*virtual*/ ts::wstr_c mainrect_c::get_name() const
{
    if (name.is_empty())
        const_cast<ts::wstr_c &>(name).set(ts::wsptr(g_sysconf.name)).replace_all(CONSTWSTR(APPNAME), APPNAME_CAPTION);
    return name;
}

/*virtual*/ void mainrect_c::created()
{
    defaultthrdraw = DTHRO_BORDER | /*DTHRO_CENTER_HOLE |*/ DTHRO_CAPTION | DTHRO_CAPTION_TEXT;
    set_theme_rect(CONSTASTR("mainrect"), false);
    __super::created();
    gui->make_app_buttons( m_rid );

    auto uiroot = [](RID p)->RID
    {
        gui_hgroup_c &g = MAKE_VISIBLE_CHILD<gui_hgroup_c>(p);
        g.allow_move_splitter(true);
        g.leech(TSNEW(leech_fill_parent_s));
        g.leech(TSNEW(leech_save_proportions_s, CONSTASTR("main_splitter"), CONSTASTR("7060,12940")));
        return g.getrid();
    };

    RID hg = uiroot(m_rid);
    RID cl = MAKE_CHILD<gui_contactlist_c>( hg );
    RID chat = MAKE_CHILD<gui_conversation_c>( hg );
    hg.call_restore_signal();
    
    gmsg<ISOGM_SELECT_CONTACT>(&contacts().get_self(), 0).send(); // 1st selected item, yo

    g_app->F_ALLOW_AUTOUPDATE = !g_app->F_READONLY_MODE;


    if (const theme_rect_s *tr = themerect())
    {
        if (tr->captextadd.x >= 18)
        {
            int sz = tr->captextadd.x - 2;

            ts::buf_c svgb; svgb.load_from_file( CONSTWSTR("icon.svg") );
            ts::abp_c gen;
            ts::str_c svgs( svgb.cstr() );
            svgs.replace_all( CONSTASTR("[scale]"), ts::amake<float>( (float)sz * 0.01f ) );
            gen.set(CONSTASTR("svg")).set_value( svgs );

            gen.set(CONSTASTR("color")).set_value( make_color( GET_THEME_VALUE(state_online_color) ) );
            gen.set(CONSTASTR("color-hover")).set_value(make_color( GET_THEME_VALUE(state_away_color) ) );
            gen.set(CONSTASTR("color-press")).set_value(make_color( GET_THEME_VALUE(state_dnd_color) ) );
            gen.set(CONSTASTR("color-disabled")).set_value(make_color( 0 ) );
            gen.set(CONSTASTR("size")).set_value( ts::amake(ts::ivec2(sz)) );
            colors_map_s cmap;
            icons.clear();
            if (generated_button_data_s *g = generated_button_data_s::generate( &gen, cmap, false ))
            {
                icons = g->src;
                TSDEL(g);
            }
        }
    }
}

ts::uint32 mainrect_c::gm_handler(gmsg<GM_HEARTBEAT> &)
{
    // check monitor

    if (0 != (checktick & 1) && !getprops().is_maximized())
    {
        ts::irect cmrect = ts::wnd_get_max_size(rrect);
        if (cmrect != mrect || getprops().screenrect() != rrect)
        {
            ts::ivec2 newc = rrect.center() - mrect.center() + cmrect.center();
            ts::irect newr = ts::irect::from_center_and_size(newc, rrect.size());
            newr.movein(cmrect);
            MODIFY(*this).pos(newr.lt).size(newr.size());
        }
    }
    ++checktick;

    return 0;
}

void mainrect_c::onclosesave()
{
    cfg().param(CONSTASTR("main_rect_pos"), ts::amake<ts::ivec2>(rrect.lt));
    cfg().param(CONSTASTR("main_rect_size"), ts::amake<ts::ivec2>(rrect.size()));
    cfg().param(CONSTASTR("main_rect_monitor"), ts::amake<ts::irect>(mrect));
}

bool mainrect_c::saverectpos(RID,GUIPARAM)
{
    onclosesave();
    return true;
}

/*virtual*/ bool mainrect_c::sq_evt(system_query_e qp, RID rid, evt_data_s &data)
{
    if (qp == SQ_RECT_CHANGED)
    {
        cfg().onclosereg( DELEGATE(this, onclosesave) );
        if (data.changed.manual)
        {
            rrect = getprops().screenrect();
            mrect = ts::wnd_get_max_size(rrect);
            DEFERRED_UNIQUE_CALL( 1.0, DELEGATE(this,saverectpos), nullptr );
        }
    }

    if (__super::sq_evt(qp, rid, data)) return true;

	switch( qp )
	{
	case SQ_DRAW:
        if (const theme_rect_s *tr = themerect())
		if (icons.info().sz.x > 0)
        {
            ts::irect cr = tr->captionrect( getprops().currentszrect(), getprops().is_maximized() );
            int st = g_app->F_OFFLINE_ICON ? contact_online_state_check : contacts().get_self().get_ostate();
            ts::irect ir;
            ir.lt.x = 0;
            ir.rb.x = icons.info().sz.x;
            ir.lt.y = st * ir.rb.x;
            ir.rb.y = ir.lt.y + ir.rb.x;
            cr.lt.y += tr->captextadd.y;

            getengine().begin_draw();
            getengine().draw( cr.lt, icons.extbody(), ir, true );
            getengine().end_draw();
        }
		break;
	}

    return false;
}
