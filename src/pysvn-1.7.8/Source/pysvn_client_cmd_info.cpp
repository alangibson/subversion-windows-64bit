//
// ====================================================================
// Copyright (c) 2003-2009 Barry A Scott.  All rights reserved.
//
// This software is licensed as described in the file LICENSE.txt,
// which you should have received as part of this distribution.
//
// ====================================================================
//
//
//  pysvn_client_cmd_info.cpp
//
#if defined( _MSC_VER )
// disable warning C4786: symbol greater than 255 character,
// nessesary to ignore as <map> causes lots of warning
#pragma warning(disable: 4786)
#endif

#include "pysvn.hpp"
#include "pysvn_svnenv.hpp"
#include "svn_path.h"
#include "svn_config.h"
#include "svn_sorts.h"
#include "pysvn_static_strings.hpp"

static const char *g_utf_8 = "utf-8";

#if defined( PYSVN_HAS_CLIENT_ANNOTATE4 )
class AnnotatedLineInfo
{
public:
    AnnotatedLineInfo
        (
        apr_int64_t line_no,
        svn_revnum_t revision,
        const char *author,
        const char *date,
        svn_revnum_t merged_revision,
        const char *merged_author,
        const char *merged_date,
        const char *merged_path,
        const char *line
        )
    : m_line_no( line_no )
    , m_revision( revision )
    , m_author()
    , m_date()
    , m_merged_revision( merged_revision)
    , m_merged_author()
    , m_merged_date()
    , m_merged_path()
    , m_line()
    {
    if( author != NULL )
        m_author = author;

    if( date != NULL )
        m_date = date;

    if( merged_author != NULL )
        m_merged_author = merged_author;

    if( merged_date != NULL )
        m_merged_date = merged_date;

    if( merged_path != NULL )
        m_merged_path = merged_path;

    if( line != NULL )
        m_line = line;
    }

    ~AnnotatedLineInfo()
    {
    }

    AnnotatedLineInfo( const AnnotatedLineInfo &other )
    : m_line_no( other.m_line_no )
    , m_revision( other.m_revision )
    , m_author( other.m_author )
    , m_date( other.m_date )
    , m_merged_revision( other.m_merged_revision )
    , m_merged_author( other.m_merged_author )
    , m_merged_date( other.m_merged_date )
    , m_merged_path( other.m_merged_path )
    , m_line( other.m_line )
    {
    }

    apr_int64_t m_line_no;
    svn_revnum_t m_revision;
    std::string m_author;
    std::string m_date;
    svn_revnum_t m_merged_revision;
    std::string m_merged_author;
    std::string m_merged_date;
    std::string m_merged_path;
    std::string m_line;
};

static svn_error_t *annotate_receiver
    (
    void *baton,
    apr_int64_t line_no,
    svn_revnum_t revision,
    const char *author,
    const char *date,
    svn_revnum_t merged_revision,
    const char *merged_author,
    const char *merged_date,
    const char *merged_path,
    const char *line,
    apr_pool_t *pool
    )
{
    // There are cases when the author has been passed as NULL
    // protect against NULL passed for any of the strings
    if( author == NULL )
        author = "";
    if( date == NULL )
        date = "";
    if( merged_author == NULL )
        merged_author = "";
    if( merged_date == NULL )
        merged_date = "";
    if( merged_path == NULL )
        merged_path = "";
    if( line == NULL )
        line = "";

    std::list<AnnotatedLineInfo> *entries = (std::list<AnnotatedLineInfo> *)baton;
    entries->push_back( AnnotatedLineInfo(
        line_no,
        revision, author, date,
        merged_revision, merged_author, merged_date, merged_path,
        line ) );

    return NULL;
}
#else
class AnnotatedLineInfo
{
public:
    AnnotatedLineInfo
        (
        apr_int64_t line_no,
        svn_revnum_t revision,
        const char *author,
        const char *date,
        const char *line
        )
    : m_line_no( line_no )
    , m_revision( revision )
    , m_author( author )
    , m_date( date )
    , m_line( line )
    {
    }
    
    ~AnnotatedLineInfo()
    {
    }

    AnnotatedLineInfo( const AnnotatedLineInfo &other )
    : m_line_no( other.m_line_no )
    , m_revision( other.m_revision )
    , m_author( other.m_author )
    , m_date( other.m_date )
    , m_line( other.m_line )
    {
    }

    apr_int64_t m_line_no;
    svn_revnum_t m_revision;
    std::string m_author;
    std::string m_date;
    std::string m_line;
};

static svn_error_t *annotate_receiver
    (
    void *baton,
    apr_int64_t line_no,
    svn_revnum_t revision,
    const char *author,
    const char *date,
    const char *line,
    apr_pool_t *pool
    )
{
    // There are cases when the author has been passed as NULL
    // protect against NULL passed for any of the strings
    if( author == NULL )
        author = "";
    if( date == NULL )
        date = "";
    if( line == NULL )
        line = "";

    std::list<AnnotatedLineInfo> *entries = (std::list<AnnotatedLineInfo> *)baton;
    entries->push_back( AnnotatedLineInfo( line_no, revision, author, date, line ) );

    return NULL;
}
#endif

Py::Object pysvn_client::cmd_annotate( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_url_or_path },
    { false, name_revision_start },
    { false, name_revision_end },
#if defined( PYSVN_HAS_CLIENT_ANNOTATE2 )
    { false, name_peg_revision },
#endif
#if defined( PYSVN_HAS_CLIENT_ANNOTATE3 )
    { false, name_ignore_space },
    { false, name_ignore_eol_style },
    { false, name_ignore_mime_type },
#endif
#if defined( PYSVN_HAS_CLIENT_ANNOTATE4 )
    { false, name_include_merged_revisions },
#endif
    { false, NULL }
    };
    FunctionArguments args( "annotate", args_desc, a_args, a_kws );
    args.check();

    std::string path( args.getUtf8String( name_url_or_path, empty_string ) );
    svn_opt_revision_t revision_start = args.getRevision( name_revision_start, svn_opt_revision_number );
    svn_opt_revision_t revision_end = args.getRevision( name_revision_end, svn_opt_revision_head );
#if defined( PYSVN_HAS_CLIENT_ANNOTATE2 )
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, revision_end );
#endif
#if defined( PYSVN_HAS_CLIENT_ANNOTATE3 )
    svn_diff_file_ignore_space_t ignore_space = svn_diff_file_ignore_space_none;
    if( args.hasArg( name_ignore_space ) )
    {
        Py::ExtensionObject< pysvn_enum_value<svn_diff_file_ignore_space_t> > py_ignore_space( args.getArg( name_ignore_space ) );
        ignore_space = svn_diff_file_ignore_space_t( py_ignore_space.extensionObject()->m_value );
    }

    svn_boolean_t ignore_eol_style = args.getBoolean( name_ignore_eol_style, false );
    svn_boolean_t ignore_mime_type = args.getBoolean( name_ignore_mime_type, false );
#endif
#if defined( PYSVN_HAS_CLIENT_ANNOTATE4 )
    svn_boolean_t include_merged_revisions = args.getBoolean( name_include_merged_revisions, false );
#endif
    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_ANNOTATE3 )
    svn_diff_file_options_t *diff_options = svn_diff_file_options_create( pool );
    diff_options->ignore_space = ignore_space;
    diff_options->ignore_eol_style = ignore_eol_style;
#endif

    bool is_url = is_svn_url( path );
#if defined( PYSVN_HAS_CLIENT_ANNOTATE2 )
    revisionKindCompatibleCheck( is_url, peg_revision, name_peg_revision, name_url_or_path );
#endif
    revisionKindCompatibleCheck( is_url, revision_start, name_revision_start, name_url_or_path );
    revisionKindCompatibleCheck( is_url, revision_end, name_revision_end, name_url_or_path );

    std::list<AnnotatedLineInfo> all_entries;

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_ANNOTATE4 )
        svn_error_t *error = svn_client_blame4
            (
            norm_path.c_str(),
            &peg_revision,
            &revision_start,
            &revision_end,
            diff_options,
            ignore_mime_type,
            include_merged_revisions,
            annotate_receiver,
            &all_entries,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_ANNOTATE3 )
        svn_error_t *error = svn_client_blame3
            (
            norm_path.c_str(),
            &peg_revision,
            &revision_start,
            &revision_end,
            diff_options,
            ignore_mime_type,
            annotate_receiver,
            &all_entries,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_ANNOTATE2 )
        svn_error_t *error = svn_client_blame2
            (
            norm_path.c_str(),
            &peg_revision,
            &revision_start,
            &revision_end,
            annotate_receiver,
            &all_entries,
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_blame
            (
            norm_path.c_str(),
            &revision_start,
            &revision_end,
            annotate_receiver,
            &all_entries,
            m_context,
            pool
            );
#endif
        permission.allowThisThread();
        if( error != NULL )
        {
            throw SvnException( error );
        }
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
    }

    // convert the entries into python objects
    Py::List entries_list;
    std::list<AnnotatedLineInfo>::const_iterator entry_it = all_entries.begin();
    while( entry_it != all_entries.end() )
    {
        const AnnotatedLineInfo &entry = *entry_it;
        ++entry_it;

        Py::Dict entry_dict;
        entry_dict[name_author] = Py::String( entry.m_author, name_utf8 );
        entry_dict[name_date] = Py::String( entry.m_date );
        entry_dict[name_line] = Py::String( entry.m_line );
        entry_dict[name_number] = Py::Int( long( entry.m_line_no ) );
        entry_dict[name_revision] = Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, entry.m_revision ) );

        entries_list.append( entry_dict );
    }

    return entries_list;
}

Py::Object pysvn_client::cmd_info( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_path },
    { false, NULL }
    };
    FunctionArguments args( "info", args_desc, a_args, a_kws );
    args.check();

    std::string path( args.getUtf8String( name_path ) );

    SvnPool pool( m_context );

    const svn_wc_entry_t *entry = NULL;

    try
    {
        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        svn_wc_adm_access_t *adm_access = NULL;

#if defined( PYSVN_HAS_WC_ADM_PROBE_OPEN3 )
        const char *c_norm_path = svn_dirent_internal_style( path.c_str(), pool );
        std::string norm_path( c_norm_path );
        svn_error_t *error = svn_wc_adm_probe_open3( &adm_access, NULL, norm_path.c_str(), false, 0, NULL, NULL, pool );
#else
        std::string norm_path( svnNormalisedPath( path, pool ) );
        svn_error_t *error = svn_wc_adm_probe_open( &adm_access, NULL, norm_path.c_str(), false, false, pool );
#endif

        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );

        permission.allowOtherThreads();
        error = svn_wc_entry( &entry, norm_path.c_str(), adm_access, false, pool );
        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );
        
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
        return Py::None();       // needed to remove warning about return value missing
    }

    if( entry == NULL )
        return Py::None();

    return toObject( *entry, pool, m_wrapper_entry );
}

#if defined( PYSVN_HAS_CLIENT_INFO )
class InfoReceiveBaton
{
public:
    InfoReceiveBaton
        (
        PythonAllowThreads *permission,
        Py::List &info_list,
        const DictWrapper &wrapper_info,
        const DictWrapper &wrapper_lock,
        const DictWrapper &wrapper_wc_info
        )
    : m_permission( permission )
    , m_info_list( info_list )
    , m_wrapper_info( wrapper_info )
    , m_wrapper_lock( wrapper_lock )
    , m_wrapper_wc_info( wrapper_wc_info )
    {}
    ~InfoReceiveBaton()
    {}

    PythonAllowThreads  *m_permission;
    Py::List            &m_info_list;
    const DictWrapper   &m_wrapper_info;
    const DictWrapper   &m_wrapper_lock;
    const DictWrapper   &m_wrapper_wc_info;
};

extern "C"
{
static svn_error_t *info_receiver_c( void *baton_, const char *path, const svn_info_t *info, apr_pool_t *pool )
{
    InfoReceiveBaton *baton = reinterpret_cast<InfoReceiveBaton *>( baton_ );

    PythonDisallowThreads callback_permission( baton->m_permission );

    if( path != NULL )
    {
        std::string std_path( path );
        if( std_path.empty() )
        {
            std_path = ".";
        }
        Py::String py_path( std_path );

        Py::Tuple py_pair( 2 );
        py_pair[0] = py_path;
        py_pair[1] = toObject(
                    *info,
                    baton->m_wrapper_info,
                    baton->m_wrapper_lock,
                    baton->m_wrapper_wc_info );

        baton->m_info_list.append( py_pair );
    }

    return SVN_NO_ERROR;
}
}

Py::Object pysvn_client::cmd_info2( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_url_or_path },
    { false, name_revision },
    { false, name_peg_revision},
    { false, name_recurse },
#if defined( PYSVN_HAS_CLIENT_INFO2 )
    { false, name_depth },
    { false, name_changelists },
#endif
    { false, NULL }
    };
    FunctionArguments args( "info2", args_desc, a_args, a_kws );
    args.check();

    std::string path( args.getUtf8String( name_url_or_path ) );

    svn_opt_revision_kind kind = svn_opt_revision_unspecified;
    if( is_svn_url( path ) )
        kind = svn_opt_revision_head;

    svn_opt_revision_t revision = args.getRevision( name_revision, kind );
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, revision );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_INFO2 )
    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_infinity, svn_depth_infinity, svn_depth_empty );
#else
    bool recurse = args.getBoolean( name_recurse, true );
#endif

    bool is_url = is_svn_url( path );
    revisionKindCompatibleCheck( is_url, peg_revision, name_peg_revision, name_url_or_path );
    revisionKindCompatibleCheck( is_url, revision, name_revision, name_url_or_path );

    Py::List info_list;

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        InfoReceiveBaton info_baton( &permission, info_list, m_wrapper_info, m_wrapper_lock, m_wrapper_wc_info );

#if defined( PYSVN_HAS_CLIENT_INFO2 )
        svn_error_t *error = 
            svn_client_info2
                (
                norm_path.c_str(),
                &peg_revision,
                &revision,
                info_receiver_c,
                reinterpret_cast<void *>( &info_baton ),
                depth,
                changelists,
                m_context,
                pool
                );
#else
        svn_error_t *error = 
            svn_client_info
                (
                norm_path.c_str(),
                &peg_revision,
                &revision,
                info_receiver_c,
                reinterpret_cast<void *>( &info_baton ),
                recurse,
                m_context,
                pool
                );
#endif
        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
    }

    return info_list;
}
#endif

#if defined( PYSVN_HAS_CLIENT_LOG4 )
class Log4Baton
{
public:
    Log4Baton( PythonAllowThreads *permission, SvnPool &pool, Py::List &log_list )
        : m_permission( permission )
        , m_pool( pool )
        , m_now( apr_time_now() )
        , m_wrapper_log( NULL )
        , m_wrapper_log_changed_path( NULL )
        , m_log_list( log_list )
        , m_has_children( false )
        {}
    ~Log4Baton()
        {}

    PythonAllowThreads  *m_permission;
    SvnPool             &m_pool;
    apr_time_t          m_now;
    DictWrapper         *m_wrapper_log;
    DictWrapper         *m_wrapper_log_changed_path;
    Py::List            &m_log_list;
    bool                m_has_children;
};

static svn_error_t *log4Receiver
    (
    void *baton_,
    svn_log_entry_t *log_entry,
    apr_pool_t *pool
    )
{
    Log4Baton *baton = reinterpret_cast<Log4Baton *>( baton_ );

    if( log_entry->revision == 0 )
    {
        // skip this blank entry
        // as the svn log command does
        return NULL;
    }

    PythonDisallowThreads callback_permission( baton->m_permission );

    Py::Dict entry_dict;

    Py::Object revprops;
    if( log_entry->revprops == NULL )
    {
        revprops = Py::None();
    }
    else
    {
        revprops = propsToObject( log_entry->revprops, baton->m_pool );
        Py::Dict revprops_dict;
        revprops_dict = revprops;

        if( revprops_dict.hasKey( "svn:date" ) )
        {
            Py::String date( revprops_dict[ "svn:date" ] );
            Py::Object int_date = toObject( convertStringToTime( date.as_std_string( g_utf_8 ), baton->m_now, baton->m_pool ) );
            revprops_dict[ "svn:date" ] = int_date;
            entry_dict[ name_date ] = int_date;
        }
        if( revprops_dict.hasKey( "svn:author" ) )
        {
            entry_dict[ name_author ] = revprops_dict[ "svn:author" ];
        }
        if( revprops_dict.hasKey( "svn:log" ) )
        {
            Py::String message( revprops_dict[ "svn:log" ] );
            revprops_dict[ "svn:log" ] = message;
            entry_dict[ name_message ] = message;
        }
    }
    entry_dict[ name_revprops ] = revprops;
    entry_dict[ name_revision ] = Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, log_entry->revision ) );

    Py::List changed_paths_list;
#if defined( PYSVN_HAS_CLIENT_LOG5 )
    if( log_entry->changed_paths2 != NULL )
    {
        for( apr_hash_index_t *hi = apr_hash_first( pool, log_entry->changed_paths2 );
                hi != NULL;
                    hi = apr_hash_next( hi ) )
        {
            Py::Dict changed_entry_dict;

            char *path = NULL;
            void *val = NULL;
            apr_hash_this( hi, (const void **) &path, NULL, &val );

            svn_log_changed_path_t *log_item = reinterpret_cast<svn_log_changed_path_t *> (val);

            changed_entry_dict[ name_path ] = Py::String( path );

            char action[2]; action[0] = log_item->action; action[1] = 0;
            changed_entry_dict[ name_action ] = Py::String( action );

            changed_entry_dict[ name_copyfrom_path ] = utf8_string_or_none( log_item->copyfrom_path );

            if( SVN_IS_VALID_REVNUM( log_item->copyfrom_rev ) )
                changed_entry_dict[ name_copyfrom_revision ] =
                    Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, log_item->copyfrom_rev ) );
            else
                changed_entry_dict[ name_copyfrom_revision ] = Py::None();

            changed_paths_list.append( baton->m_wrapper_log_changed_path->wrapDict( changed_entry_dict ) );
        }
    }
#else
    if( log_entry->changed_paths != NULL )
    {
        for( apr_hash_index_t *hi = apr_hash_first( pool, log_entry->changed_paths );
                hi != NULL;
                    hi = apr_hash_next( hi ) )
        {
            Py::Dict changed_entry_dict;

            char *path = NULL;
            void *val = NULL;
            apr_hash_this( hi, (const void **) &path, NULL, &val );

            svn_log_changed_path_t *log_item = reinterpret_cast<svn_log_changed_path_t *> (val);

            changed_entry_dict[ name_path ] = Py::String( path );

            char action[2]; action[0] = log_item->action; action[1] = 0;
            changed_entry_dict[ name_action ] = Py::String( action );

            changed_entry_dict[ name_copyfrom_path ] = utf8_string_or_none( log_item->copyfrom_path );

            if( SVN_IS_VALID_REVNUM( log_item->copyfrom_rev ) )
                changed_entry_dict[ name_copyfrom_revision ] =
                    Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, log_item->copyfrom_rev ) );
            else
                changed_entry_dict[ name_copyfrom_revision ] = Py::None();

            changed_paths_list.append( baton->m_wrapper_log_changed_path->wrapDict( changed_entry_dict ) );
        }
    }
#endif

    entry_dict[ name_changed_paths ] = changed_paths_list;
    entry_dict[ "has_children" ] = Py::Int( log_entry->has_children != 0 ? 1 : 0 );

    baton->m_log_list.append( baton->m_wrapper_log->wrapDict( entry_dict ) );

    return NULL;
}

// PYSVN_HAS_CLIENT_LOG4 version
Py::Object pysvn_client::cmd_log( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_url_or_path },
    { false, name_revision_start },
    { false, name_revision_end },
    { false, name_discover_changed_paths },
    { false, name_strict_node_history },
    { false, name_limit },
    { false, name_peg_revision },
    { false, name_include_merged_revisions },
    { false, name_revprops },
    { false, NULL }
    };
    FunctionArguments args( "log", args_desc, a_args, a_kws );
    args.check();

    SvnPool pool( m_context );

    svn_opt_revision_t revision_start = args.getRevision( name_revision_start, svn_opt_revision_head );
    svn_opt_revision_t revision_end = args.getRevision( name_revision_end, svn_opt_revision_number );
    bool discover_changed_paths = args.getBoolean( name_discover_changed_paths, false );
    bool strict_node_history = args.getBoolean( name_strict_node_history, true );
    int limit = args.getInteger( name_limit, 0 );
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, svn_opt_revision_unspecified );

    svn_boolean_t include_merged_revisions = args.getBoolean( name_include_merged_revisions, false );
    apr_array_header_t *revprops = NULL;
    if( args.hasArg( name_revprops ) )
    {
        Py::Object py_revprop = args.getArg( name_revprops );
        if( !py_revprop.isNone() )
        {
            revprops = arrayOfStringsFromListOfStrings( py_revprop, pool );
        }
    }

    Py::Object url_or_path_obj = args.getArg( name_url_or_path );
    Py::List url_or_path_list;
    if( url_or_path_obj.isList() )
    {
        url_or_path_list = url_or_path_obj;
    }
    else
    {
        Py::List py_list;
        py_list.append( url_or_path_obj );
        url_or_path_list = py_list;
    }

    for( size_t i=0; i<url_or_path_list.size(); i++ )
    {
        Py::Bytes py_path( asUtf8Bytes( url_or_path_list[ i ] ) );
        std::string path( py_path.as_std_string() );
        bool is_url = is_svn_url( path );

        revisionKindCompatibleCheck( is_url, peg_revision, name_peg_revision, name_url_or_path );
        revisionKindCompatibleCheck( is_url, revision_start, name_revision_start, name_url_or_path );
        revisionKindCompatibleCheck( is_url, revision_end, name_revision_end, name_url_or_path );
    }

    apr_array_header_t *targets = targetsFromStringOrList( url_or_path_list, pool );

    Py::List log_list;

    try
    {
        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        Log4Baton baton( &permission, pool, log_list );
        baton.m_wrapper_log = &m_wrapper_log;
        baton.m_wrapper_log_changed_path = &m_wrapper_log_changed_path;

#if defined( PYSVN_HAS_CLIENT_LOG5 )
        apr_array_header_t *revision_ranges = apr_array_make( pool, 0, sizeof(svn_opt_revision_range_t *) );
        svn_opt_revision_range_t *range = reinterpret_cast<svn_opt_revision_range_t *>( apr_palloc( pool, sizeof(*range) ) );

        range->start = revision_start;
        range->end = revision_end;

        APR_ARRAY_PUSH( revision_ranges, svn_opt_revision_range_t * ) = range;

        svn_error_t *error = svn_client_log5
            (
            targets,
            &peg_revision,
            revision_ranges,
            limit,
            discover_changed_paths,
            strict_node_history,
            include_merged_revisions,
            revprops,
            log4Receiver,
            reinterpret_cast<void *>( &baton ),
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_log4
            (
            targets,
            &peg_revision,
            &revision_start,
            &revision_end,
            limit,
            discover_changed_paths,
            strict_node_history,
            include_merged_revisions,
            revprops,
            log4Receiver,
            reinterpret_cast<void *>( &baton ),
            m_context,
            pool
            );
#endif
        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
    }

    return log_list;
}
#else
class LogChangePathInfo
{
public:
    LogChangePathInfo( const char *path, svn_log_changed_path_t *info )
    : m_path( path )
    , m_action( info->action )
    , m_copy_from_path( info->copyfrom_path != NULL ? info->copyfrom_path : "" )
    , m_copy_from_revision( info->copyfrom_rev )
    {
    }

    LogChangePathInfo( const LogChangePathInfo &other )
    : m_path( other.m_path )
    , m_action( other.m_action )
    , m_copy_from_path( other.m_copy_from_path )
    , m_copy_from_revision( other.m_copy_from_revision )
    {
    }

    std::string     m_path;
    char            m_action;
    std::string     m_copy_from_path;
    svn_revnum_t    m_copy_from_revision;
};

class LogEntryInfo
{
public:
    LogEntryInfo
        (
        svn_revnum_t revision,
        const char *author,
        const char *date,
        const char *message
        )
    : m_revision( revision )
    , m_author( author )
    , m_date( date )
    , m_message( message )
    , m_changed_paths()
    {
    }
    
    ~LogEntryInfo()
    {
    }

    LogEntryInfo( const LogEntryInfo &other )
    : m_revision( other.m_revision )
    , m_author( other.m_author )
    , m_date( other.m_date )
    , m_message( other.m_message )
    , m_changed_paths( other.m_changed_paths )
    {
    }

    svn_revnum_t m_revision;
    std::string m_author;
    std::string m_date;
    std::string m_message;
    std::list<LogChangePathInfo> m_changed_paths;
};

static svn_error_t *logReceiver
    (
    void *baton,
    apr_hash_t *changedPaths,
    svn_revnum_t rev,
    const char *author,
    const char *date,
    const char *msg,
    apr_pool_t *pool
    )
{
    if( rev == 0 )
    {
        // skip this blank entry
        // as the svn log command does
        return NULL;
    }

    std::list<LogEntryInfo> *entries = (std::list<LogEntryInfo> *)baton;

    if( author == NULL )
        author = "";
    if( date == NULL )
        date = "";
    if( msg == NULL )
        msg = "";

    entries->push_back( LogEntryInfo( rev, author, date, msg ) );

    if( changedPaths != NULL )
    {
        LogEntryInfo &entry = entries->back();

        for( apr_hash_index_t *hi = apr_hash_first( pool, changedPaths );
                hi != NULL;
                    hi = apr_hash_next( hi ) )
        {
            char *path = NULL;
            void *val = NULL;
            apr_hash_this( hi, (const void **) &path, NULL, &val );

            svn_log_changed_path_t *log_item = reinterpret_cast<svn_log_changed_path_t *> (val);

            entry.m_changed_paths.push_back( LogChangePathInfo( path, log_item ) );
        }
    }

    return NULL;
}

// PYSVN_HAS_CLIENT_LOG, PYSVN_HAS_CLIENT_LOG2, PYSVN_HAS_CLIENT_LOG3 version
Py::Object pysvn_client::cmd_log( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_url_or_path },
    { false, name_revision_start },
    { false, name_revision_end },
    { false, name_discover_changed_paths },
    { false, name_strict_node_history },
#if defined( PYSVN_HAS_CLIENT_LOG2 ) || defined( PYSVN_HAS_CLIENT_LOG3 )
    { false, name_limit },
#endif
#if defined( PYSVN_HAS_CLIENT_LOG3 )
    { false, name_peg_revision },
#endif
#if defined( PYSVN_HAS_CLIENT_LOG4 )
    { false, name_include_merged_revisions },
    { false, name_revprops },
#endif
    { false, NULL }
    };
    FunctionArguments args( "log", args_desc, a_args, a_kws );
    args.check();

    svn_opt_revision_t revision_start = args.getRevision( name_revision_start, svn_opt_revision_head );
    svn_opt_revision_t revision_end = args.getRevision( name_revision_end, svn_opt_revision_number );
    bool discover_changed_paths = args.getBoolean( name_discover_changed_paths, false );
    bool strict_node_history = args.getBoolean( name_strict_node_history, true );
    int limit = args.getInteger( name_limit, 0 );
#if defined( PYSVN_HAS_CLIENT_LOG3 )
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, svn_opt_revision_unspecified );
#endif
#if defined( PYSVN_HAS_CLIENT_LOG4 )
    svn_boolean_t include_merged_revisions = args.getBoolean( name_include_merged_revisions, false );
    apr_array_header_t *revprops = NULL;
    Py::Object py_revprop = args.getArg( name_revprops );
    if( py_revprop is not None )
    {
        revprops = arrayOfStringsFromListOfStrings( py_revprop. pool );
    }
#endif

    Py::Object url_or_path_obj = args.getArg( name_url_or_path );
    Py::List url_or_path_list;
    if( url_or_path_obj.isList() )
    {
        url_or_path_list = url_or_path_obj;
    }
    else
    {
        Py::List py_list;
        py_list.append( url_or_path_obj );
        url_or_path_list = py_list;
    }

    for( size_t i=0; i<url_or_path_list.size(); i++ )
    {
        Py::Bytes py_path( asUtf8Bytes( url_or_path_list[ i ] ) );
        std::string path( py_path.as_std_string() );
        bool is_url = is_svn_url( path );

        // std::cout << "peg_revision "    << peg_revision.kind    << " " << peg_revision.value.number     << std::endl;
        // std::cout << "revision_start "  << revision_start.kind  << " " << revision_start.value.number   << std::endl;
        // std::cout << "revision_end "    << revision_end.kind    << " " << revision_end.value.number     << std::endl;

#if defined( PYSVN_HAS_CLIENT_LOG3 )
        revisionKindCompatibleCheck( is_url, peg_revision, name_peg_revision, name_url_or_path );
#endif
        revisionKindCompatibleCheck( is_url, revision_start, name_revision_start, name_url_or_path );
        revisionKindCompatibleCheck( is_url, revision_end, name_revision_end, name_url_or_path );
    }

    SvnPool pool( m_context );

    apr_array_header_t *targets = targetsFromStringOrList( url_or_path_list, pool );

#if defined( PYSVN_HAS_CLIENT_LOG4 )
    Log4Baton baton( permission, pool );
#else
    std::list<LogEntryInfo> all_entries;
#endif

    try
    {
        checkThreadPermission();

        PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_LOG4 )
        svn_error_t *error = svn_client_log4
            (
            targets,
            &peg_revision,
            &revision_start,
            &revision_end,
            limit,
            discover_changed_paths,
            strict_node_history,
            include_merged_revisions,
            revprops,
            logReceiver,
            &all_entries,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_LOG3 )
        svn_error_t *error = svn_client_log3
            (
            targets,
            &peg_revision,
            &revision_start,
            &revision_end,
            limit,
            discover_changed_paths,
            strict_node_history,
            logReceiver,
            &all_entries,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_LOG2 )
        svn_error_t *error = svn_client_log2
            (
            targets,
            &revision_start,
            &revision_end,
            limit,
            discover_changed_paths,
            strict_node_history,
            logReceiver,
            &all_entries,
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_log
            (
            targets,
            &revision_start,
            &revision_end,
            discover_changed_paths,
            strict_node_history,
            logReceiver,
            &all_entries,
            m_context,
            pool
            );
#endif
        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
    }

    apr_time_t now = apr_time_now();

    // convert the entries into python objects
    Py::List entries_list;
    std::list<LogEntryInfo>::const_iterator entry_it = all_entries.begin();
    while( entry_it != all_entries.end() )
    {
        const LogEntryInfo &entry = *entry_it;
        ++entry_it;

        Py::Dict entry_dict;
        entry_dict[name_author] = Py::String( entry.m_author, name_utf8 );
        entry_dict[name_date] = toObject( convertStringToTime( entry.m_date, now, pool ) );
        entry_dict[name_message] = Py::String( entry.m_message, name_utf8 );
        entry_dict[name_revision] = Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, entry.m_revision ) );

        Py::List changed_paths_list;
        std::list<LogChangePathInfo>::const_iterator changed_paths_it = entry.m_changed_paths.begin();
        while( changed_paths_it != entry.m_changed_paths.end() )
        {
            const LogChangePathInfo &change_entry = *changed_paths_it;
            ++changed_paths_it;

            Py::Dict changed_entry_dict;
            changed_entry_dict[name_path] = Py::String( change_entry.m_path, name_utf8 );
            changed_entry_dict[name_action] = Py::String( &change_entry.m_action, 1 );
            changed_entry_dict[name_copyfrom_path] = utf8_string_or_none( change_entry.m_copy_from_path );

            if( SVN_IS_VALID_REVNUM( change_entry.m_copy_from_revision ) )
                changed_entry_dict[name_copyfrom_revision] = Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, change_entry.m_copy_from_revision ) );
            else
                changed_entry_dict[name_copyfrom_revision] = Py::None();

            changed_paths_list.append( m_wrapper_log_changed_path.wrapDict( changed_entry_dict ) );
        }

        entry_dict[name_changed_paths] = changed_paths_list;

        entries_list.append( m_wrapper_log.wrapDict( entry_dict ) );
    }

    return entries_list;
}
#endif

struct StatusEntriesBaton
{
    apr_pool_t* pool;
    apr_hash_t* hash;
};

#if defined( PYSVN_HAS_CLIENT_STATUS4 )
static svn_error_t *StatusEntriesFunc
    (
    void *baton,
    const char *path,
    svn_wc_status2_t *status,
    apr_pool_t *pool
    )
{
    svn_wc_status2_t *stat;
    StatusEntriesBaton *seb = reinterpret_cast<StatusEntriesBaton *>( baton );

    path = apr_pstrdup( seb->pool, path );
    stat = svn_wc_dup_status2( status, seb->pool );
    apr_hash_set( seb->hash, path, APR_HASH_KEY_STRING, stat );
    return SVN_NO_ERROR;
}
#elif defined( PYSVN_HAS_CLIENT_STATUS2 )
static void StatusEntriesFunc
    (
    void *baton,
    const char *path,
    svn_wc_status2_t *status
    )
{
    svn_wc_status2_t *stat;
    StatusEntriesBaton *seb = reinterpret_cast<StatusEntriesBaton *>( baton );

    path = apr_pstrdup( seb->pool, path );
    stat = svn_wc_dup_status2( status, seb->pool );
    apr_hash_set( seb->hash, path, APR_HASH_KEY_STRING, stat );
}
#else
static void StatusEntriesFunc
    (
    void *baton,
    const char *path,
    svn_wc_status_t *status
    )
{
    svn_wc_status_t *stat;
    StatusEntriesBaton *seb = reinterpret_cast<StatusEntriesBaton *>( baton );

    path = apr_pstrdup( seb->pool, path );
    stat = svn_wc_dup_status( status, seb->pool );
    apr_hash_set( seb->hash, path, APR_HASH_KEY_STRING, stat );
}
#endif

Py::Object pysvn_client::cmd_status( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_path },
    { false, name_recurse },
    { false, name_get_all },
    { false, name_update },
    { false, name_ignore },
#if defined( PYSVN_HAS_CLIENT_STATUS2 )
    { false, name_ignore_externals },
#endif
#if defined( PYSVN_HAS_CLIENT_STATUS3 )
    { false, name_depth },
    { false, name_changelists },
#endif
    { false, NULL }
    };
    FunctionArguments args( "status", args_desc, a_args, a_kws );
    args.check();

    Py::String path( args.getUtf8String( name_path ) );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_STATUS3 )
    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_infinity, svn_depth_infinity, svn_depth_immediates );
#else
    bool recurse = args.getBoolean( name_recurse, true );
#endif
    bool get_all = args.getBoolean( name_get_all, true );
    bool update = args.getBoolean( name_update, false );
    bool ignore = args.getBoolean( name_ignore, false );
#if defined( PYSVN_HAS_CLIENT_STATUS2 )
    bool ignore_externals = args.getBoolean( name_ignore_externals, false );
#endif

    apr_hash_t *status_hash = NULL;

    Py::List entries_list;
    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        svn_revnum_t revnum;
        svn_opt_revision_t rev = { svn_opt_revision_head, {0} };

        StatusEntriesBaton baton;

        status_hash = apr_hash_make( pool );
        baton.hash = status_hash;
        baton.pool = pool;

#if defined( PYSVN_HAS_CLIENT_STATUS4 )
        svn_error_t *error = svn_client_status4
            (
            &revnum,            // revnum
            norm_path.c_str(),  // path
            &rev,
            StatusEntriesFunc,  // status func
            &baton,             // status baton
            depth,
            get_all,
            update,
            !ignore,
            ignore_externals,
            changelists,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_STATUS3 )
        svn_error_t *error = svn_client_status3
            (
            &revnum,            // revnum
            norm_path.c_str(),  // path
            &rev,
            StatusEntriesFunc,  // status func
            &baton,             // status baton
            depth,
            get_all,
            update,
            !ignore,
            ignore_externals,
            changelists,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_STATUS2 )
        svn_error_t *error = svn_client_status2
            (
            &revnum,            // revnum
            norm_path.c_str(),  // path
            &rev,
            StatusEntriesFunc,  // status func
            &baton,             // status baton
            recurse,
            get_all,
            update,
            !ignore,
            ignore_externals,
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_status
            (
            &revnum,            // revnum
            norm_path.c_str(),  // path
            &rev,
            StatusEntriesFunc,  // status func
            &baton,             // status baton
            recurse,
            get_all,
            update,
            !ignore,
            m_context,
            pool
            );
#endif
        permission.allowThisThread();
        if( error != NULL )
            throw SvnException( error );
    }
    catch( SvnException &e )
    {
        // use callback error over ClientException
        m_context.checkForError( m_module.client_error );

        throw_client_error( e );
    }

    apr_array_header_t *statusarray = svn_sort__hash( status_hash, svn_sort_compare_items_as_paths, pool );

    // Loop over array, printing each name/status-structure
    for (int i = statusarray->nelts-1; i >= 0; i--)
    {
        const svn_sort__item_t *item = &APR_ARRAY_IDX( statusarray, i, const svn_sort__item_t );
        pysvn_wc_status_t *status = (pysvn_wc_status_t *)item->value;

        entries_list.append( toObject(
                Py::String( osNormalisedPath( (const char *)item->key, pool ), "UTF-8" ),
                *status,
                pool,
                m_wrapper_status,
                m_wrapper_entry,
                m_wrapper_lock ) );
    }

    return entries_list;
}
