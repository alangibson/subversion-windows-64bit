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
//  pysvn_client_cmd_prop.cpp
//
#if defined( _MSC_VER )
// disable warning C4786: symbol greater than 255 character,
// nessesary to ignore as <map> causes lots of warning
#pragma warning(disable: 4786)
#endif

#include "pysvn.hpp"
#include "pysvn_static_strings.hpp"

Py::Object pysvn_client::cmd_propdel( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_prop_name },
    { true,  name_url_or_path },
    { false, name_revision },
    { false, name_recurse },
#if defined( PYSVN_HAS_CLIENT_PROPSET2 )
    { false, name_skip_checks },
#endif
#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    { false, name_depth },
    { false, name_base_revision_for_url },
    { false, name_changelists },
    { false, name_revprops },
#endif
    { false, NULL }
    };
    FunctionArguments args( "propdel", args_desc, a_args, a_kws );
    args.check();

    std::string propname( args.getUtf8String( name_prop_name ) );
    std::string path( args.getUtf8String( name_url_or_path ) );

    svn_opt_revision_t revision;
    if( is_svn_url( path ) )
        revision = args.getRevision( name_revision, svn_opt_revision_head );
    else
        revision = args.getRevision( name_revision, svn_opt_revision_working );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    svn_revnum_t base_revision_for_url;
    if( is_svn_url( path ) )
        base_revision_for_url = args.getInteger( name_base_revision_for_url, 0 );
    else
        base_revision_for_url = args.getInteger( name_base_revision_for_url, SVN_INVALID_REVNUM );

    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_files, svn_depth_infinity, svn_depth_empty );
#else
    bool recurse = args.getBoolean( name_recurse, false );
#endif
#if defined( PYSVN_HAS_CLIENT_PROPSET2 )
    bool skip_checks = args.getBoolean( name_skip_checks, false );
#endif

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    pysvn_commit_info_t *commit_info = NULL;

    apr_hash_t *revprops = NULL;
    if( args.hasArg( name_revprops ) )
    {
        Py::Object py_revprop = args.getArg( name_revprops );
        if( !py_revprop.isNone() )
        {
            revprops = hashOfStringsFromDistOfStrings( py_revprop, pool );
        }
    }
#endif

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
        svn_error_t *error = svn_client_propset3
            (
            &commit_info,
            propname.c_str(),
            NULL, // value = NULL
            norm_path.c_str(),
            depth,
            skip_checks,
            base_revision_for_url,
            changelists,
            revprops,
            m_context.ctx(),
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_PROPSET2 )
        svn_error_t *error = svn_client_propset2
            (
            propname.c_str(),
            NULL, // value = NULL
            norm_path.c_str(),
            recurse,
            skip_checks,
            m_context.ctx(),
            pool
            );
#else
        svn_error_t *error = svn_client_propset
            (
            propname.c_str(),
            NULL, // value = NULL
            norm_path.c_str(),
            recurse,
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

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    return toObject( commit_info, m_commit_info_style );
#else
    return Py::None();
#endif
}

Py::Object pysvn_client::cmd_propget( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_prop_name },
    { true,  name_url_or_path },
    { false, name_revision },
    { false, name_recurse },
#if defined( PYSVN_HAS_CLIENT_PROPGET2 )
    { false, name_peg_revision },
#endif
#if defined( PYSVN_HAS_CLIENT_PROPGET3 )
    { false, name_depth },
    { false, name_changelists },
#endif
    { false, NULL }
    };
    FunctionArguments args( "propget", args_desc, a_args, a_kws );
    args.check();

    std::string propname( args.getUtf8String( name_prop_name ) );
    std::string path( args.getUtf8String( name_url_or_path ) );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPGET3 )
    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_files, svn_depth_infinity, svn_depth_empty );
#else
    bool recurse = args.getBoolean( name_recurse, false );
#endif
    svn_opt_revision_t revision;
    if( is_svn_url( path ) )
        revision = args.getRevision( name_revision, svn_opt_revision_head );
    else
        revision = args.getRevision( name_revision, svn_opt_revision_working );
#if defined( PYSVN_HAS_CLIENT_PROPGET2 )
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, revision );
#endif

    bool is_url = is_svn_url( path );
#if defined( PYSVN_HAS_CLIENT_PROPGET2 )
    revisionKindCompatibleCheck( is_url, peg_revision, name_peg_revision, name_url_or_path );
#endif
    revisionKindCompatibleCheck( is_url, revision, name_revision, name_url_or_path );

    apr_hash_t *props = NULL;

#if defined( PYSVN_HAS_CLIENT_PROPGET3 )
    svn_revnum_t actual_revnum = 0;
#endif

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPGET3 )
        svn_error_t *error = svn_client_propget3
            (
            &props,
            propname.c_str(),
            norm_path.c_str(),
            &peg_revision,
            &revision,
            &actual_revnum,
            depth,
            changelists,
            m_context,
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_PROPGET2 )
        svn_error_t *error = svn_client_propget2
            (
            &props,
            propname.c_str(),
            norm_path.c_str(),
            &peg_revision,
            &revision,
            recurse,
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_propget
            (
            &props,
            propname.c_str(),
            norm_path.c_str(),
            &revision,
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

    return propsToObject( props, pool );
}

#if defined( PYSVN_HAS_CLIENT_PROPLIST3 )
class ProplistReceiveBaton
{
public:
    ProplistReceiveBaton( PythonAllowThreads *permission, SvnPool &pool, Py::List &prop_list )
        : m_permission( permission )
        , m_pool( pool )
        , m_prop_list( prop_list )
        {}
    ~ProplistReceiveBaton()
        {}

    PythonAllowThreads  *m_permission;
    SvnPool             &m_pool;

    Py::List            &m_prop_list;
};

extern "C"
{
svn_error_t *proplist_receiver_c
    (
    void *baton_,
    const char *path,
    apr_hash_t *prop_hash,
    apr_pool_t *pool
    )
{
    ProplistReceiveBaton *baton = reinterpret_cast<ProplistReceiveBaton *>( baton_ );

    PythonDisallowThreads callback_permission( baton->m_permission );

    Py::Dict prop_dict;

    Py::Tuple py_tuple( 2 );
    py_tuple[0] = Py::String( path );
    py_tuple[1] = propsToObject( prop_hash, baton->m_pool );

    baton->m_prop_list.append( py_tuple );

    return SVN_NO_ERROR;
}
}
#endif

Py::Object pysvn_client::cmd_proplist( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_url_or_path },
    { false, name_revision },
    { false, name_recurse },
#if defined( PYSVN_HAS_CLIENT_PROPLIST2 )
    { false, name_peg_revision },
#endif
#if defined( PYSVN_HAS_CLIENT_PROPLIST3 )
    { false, name_depth },
    { false, name_changelists },
#endif
    { false, NULL }
    };
    FunctionArguments args( "proplist", args_desc, a_args, a_kws );
    args.check();

    Py::List path_list( toListOfStrings( args.getArg( name_url_or_path ) ) );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPLIST3 )
    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_empty, svn_depth_infinity, svn_depth_empty );
#else
    bool recurse = args.getBoolean( name_recurse, false );
#endif
    bool is_revision_setup = false;
    bool is_url = false;

    svn_opt_revision_t revision_url;
    svn_opt_revision_t revision_file;
    if( args.hasArg( name_revision ) )
    {
        revision_url = args.getRevision( name_revision );
        revision_file = revision_url;
    }
    else
    {
        revision_url.kind = svn_opt_revision_head;
        revision_file.kind = svn_opt_revision_working;
    }

#if defined( PYSVN_HAS_CLIENT_PROPLIST2 )
    svn_opt_revision_t peg_revision_url;
    svn_opt_revision_t peg_revision_file;
    if( args.hasArg( name_peg_revision ) )
    {
        peg_revision_url = args.getRevision( name_peg_revision );
        peg_revision_file = peg_revision_url;
    }
    else
    {
        peg_revision_url = revision_url;
        peg_revision_file = revision_file;
    }
#endif

    Py::List list_of_proplists;

    for( Py::List::size_type i=0; i<path_list.length(); i++ )
    {
        Py::Bytes path_str( asUtf8Bytes( path_list[i] ) );
        std::string path( path_str.as_std_string() );
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        svn_opt_revision_t revision;
        svn_opt_revision_t peg_revision;
        if( !is_revision_setup )
            if( is_svn_url( path ) )
            {
                revision = revision_url;
#if defined( PYSVN_HAS_CLIENT_PROPLIST2 )
                peg_revision = peg_revision_url;
#endif
                is_url = true;
            }
            else
            {
                revision = revision_file;
#if defined( PYSVN_HAS_CLIENT_PROPLIST2 )
                peg_revision = peg_revision_file;
#endif
            }
        else
            if( is_svn_url( path ) && !is_url )
            {
                throw Py::AttributeError( "cannot mix URL and PATH in name_path" );
            }

        try
        {
            const char *norm_path_c_str= norm_path.c_str();
            checkThreadPermission();

            PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPLIST3 )
            ProplistReceiveBaton proplist_baton( &permission, pool, list_of_proplists );
            svn_error_t *error = svn_client_proplist3
                (
                norm_path_c_str,
                &peg_revision,
                &revision,
                depth,
                changelists,
                proplist_receiver_c,
                reinterpret_cast<void *>( &proplist_baton ),
                m_context,
                pool
                );
#elif defined( PYSVN_HAS_CLIENT_PROPLIST2 )
            apr_array_header_t *props = NULL;
            svn_error_t *error = svn_client_proplist2
                (
                &props,
                norm_path_c_str,
                &peg_revision,
                &revision,
                recurse,
                m_context,
                pool
                );
#else
            apr_array_header_t *props = NULL;
            svn_error_t *error = svn_client_proplist
                (
                &props,
                norm_path_c_str,
                &revision,
                recurse,
                m_context,
                pool
                );
#endif
            permission.allowThisThread();
            if( error != NULL )
                throw SvnException( error );

#if !defined( PYSVN_HAS_CLIENT_PROPLIST3 )
            proplistToObject( list_of_proplists, props, pool );
#endif
        }
        catch( SvnException &e )
        {
            // use callback error over ClientException
            m_context.checkForError( m_module.client_error );

            throw_client_error( e );
        }
    }
    
    return list_of_proplists;
}

Py::Object pysvn_client::cmd_propset( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_prop_name },
    { true,  name_prop_value },
    { true,  name_url_or_path },
    { false, name_revision },
    { false, name_recurse },
#if defined( PYSVN_HAS_CLIENT_PROPSET2 )
    { false, name_skip_checks },
#endif
#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    { false, name_depth },
    { false, name_base_revision_for_url },
    { false, name_changelists },
    { false, name_revprops },
#endif
    { false, NULL }
    };
    FunctionArguments args( "propset", args_desc, a_args, a_kws );
    args.check();

    std::string propname( args.getUtf8String( name_prop_name ) );
    std::string propval( args.getUtf8String( name_prop_value ) );
    std::string path( args.getUtf8String( name_url_or_path ) );

    svn_opt_revision_t revision;
    if( is_svn_url( path ) )
        revision = args.getRevision( name_revision, svn_opt_revision_head );
    else
        revision = args.getRevision( name_revision, svn_opt_revision_working );

    SvnPool pool( m_context );

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    apr_array_header_t *changelists = NULL;

    if( args.hasArg( name_changelists ) )
    {
        changelists = arrayOfStringsFromListOfStrings( args.getArg( name_changelists ), pool );
    }

    svn_revnum_t base_revision_for_url;
    if( is_svn_url( path ) )
        base_revision_for_url = args.getInteger( name_base_revision_for_url, 0 );
    else
        base_revision_for_url = args.getInteger( name_base_revision_for_url, SVN_INVALID_REVNUM );
    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_empty, svn_depth_infinity, svn_depth_empty );

    apr_hash_t *revprops = NULL;
    if( args.hasArg( name_revprops ) )
    {
        Py::Object py_revprop = args.getArg( name_revprops );
        if( !py_revprop.isNone() )
        {
            revprops = hashOfStringsFromDistOfStrings( py_revprop, pool );
        }
    }
#else
    bool recurse = args.getBoolean( name_recurse, false );
#endif
#if defined( PYSVN_HAS_CLIENT_PROPSET2 )
    bool skip_checks = args.getBoolean( name_skip_checks, false );
#endif

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    pysvn_commit_info_t *commit_info = NULL;
#endif

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        const svn_string_t *svn_propval = svn_string_ncreate( propval.c_str(), propval.size(), pool );

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
        svn_error_t *error = svn_client_propset3
            (
            &commit_info,
            propname.c_str(),
            svn_propval,
            norm_path.c_str(),
            depth,
            skip_checks,
            base_revision_for_url,
            changelists,
            revprops,
            m_context.ctx(),
            pool
            );
#elif defined( PYSVN_HAS_CLIENT_PROPSET2 )
        svn_error_t *error = svn_client_propset2
            (
            propname.c_str(),
            svn_propval,
            norm_path.c_str(),
            recurse,
            skip_checks,
            m_context.ctx(),
            pool
            );
#else
        svn_error_t *error = svn_client_propset
            (
            propname.c_str(),
            svn_propval,
            norm_path.c_str(),
            recurse,
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

#if defined( PYSVN_HAS_CLIENT_PROPSET3 )
    return toObject( commit_info, m_commit_info_style );
#else
    return Py::None();
#endif
}
