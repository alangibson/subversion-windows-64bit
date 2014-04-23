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
//  pysvn_client_cmd_diff.cpp
//
#if defined( _MSC_VER )
// disable warning C4786: symbol greater than 255 character,
// nessesary to ignore as <map> causes lots of warning
#pragma warning(disable: 4786)
#endif

#include "pysvn.hpp"
#include "pysvn_static_strings.hpp"

#if defined( PYSVN_HAS_CLIENT_COPY4 ) || defined( PYSVN_HAS_CLIENT_COPY5 ) 
Py::Object pysvn_client::cmd_copy2( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true, name_sources },
    { true, name_dest_url_or_path },
    { false, name_copy_as_child },
    { false, name_make_parents },
    { false, name_revprops },
#if defined( PYSVN_HAS_CLIENT_COPY5 )
    { false, name_ignore_externals },
#endif
    { false, NULL }
    };
    FunctionArguments args( "copy2", args_desc, a_args, a_kws );
    args.check();

    SvnPool pool( m_context );
    pysvn_commit_info_t *commit_info = NULL;

    std::string type_error_message;
    try
    {
        type_error_message = "expecting list for sources (arg 1)";
        Py::List list_all_sources = args.getArg( name_sources );

        apr_array_header_t *all_sources =
            apr_array_make( pool, list_all_sources.length(), sizeof(svn_client_copy_source_t *) );

        for( unsigned int index=0; index<list_all_sources.length(); index++ )
        {
            Py::Tuple tuple_src_rev_pegrev( list_all_sources[ index ] );

            std::string src_url_or_path;
            svn_opt_revision_t *revision = reinterpret_cast<svn_opt_revision_t *>( apr_palloc( pool, sizeof( svn_opt_revision_t ) ) );
            svn_opt_revision_t *peg_revision = reinterpret_cast<svn_opt_revision_t *>( apr_palloc( pool, sizeof( svn_opt_revision_t ) ) );

            if( tuple_src_rev_pegrev.length() > 3 )
            {
                std::string msg = "copy2() expecting tuple with 2 or 3 values in sources list";
                throw Py::AttributeError( msg );
            }

            type_error_message = "expecting string for 1st tuple value in sources list";
            Py::String py_src_url_or_path( tuple_src_rev_pegrev[0] );
            src_url_or_path = py_src_url_or_path.as_std_string( name_utf8 );
            std::string norm_src_url_or_path( svnNormalisedIfPath( src_url_or_path, pool ) );
            bool is_url = is_svn_url( norm_src_url_or_path );

            if( tuple_src_rev_pegrev.length() >= 2 )
            {
                Py::Object obj( tuple_src_rev_pegrev[1] );

                if( pysvn_revision::check( obj ) )
                {
                    pysvn_revision *rev = static_cast<pysvn_revision *>( obj.ptr() );
                    *revision = rev->getSvnRevision();

                    revisionKindCompatibleCheck( is_url, *revision,
                        "sources list 2nd tuple value", "sources list 1st tuple value" );
                }
                else
                {
                    std::string msg = "copy2() expecting revision for 2nd tuple value in sources list";
                    throw Py::AttributeError( msg );
                }
            }
            else
            {
                if( is_url )
                {
                    revision->kind = svn_opt_revision_head;
                }
                else
                {
                    revision->kind = svn_opt_revision_working;
                }
            }

            if( tuple_src_rev_pegrev.length() >= 3 )
            {
                Py::Object obj( tuple_src_rev_pegrev[2] );

                if( pysvn_revision::check( obj ) )
                {
                    pysvn_revision *rev = static_cast<pysvn_revision *>( obj.ptr() );
                    *peg_revision = rev->getSvnRevision();

                    revisionKindCompatibleCheck( is_url, *peg_revision,
                        "sources list 2nd tuple value", "sources list 1st tuple value" );
                }
                else
                {
                    std::string msg = "copy2() expecting revision for 3rd tuple value in sources list";
                    throw Py::AttributeError( msg );
                }
            }
            else
            {
                *peg_revision = *revision;
            }

            svn_client_copy_source_t *source = reinterpret_cast<svn_client_copy_source_t *>( apr_palloc( pool, sizeof(*source) ) );
            source->path = apr_pstrdup( pool, norm_src_url_or_path.c_str() );
            source->revision = revision;
            source->peg_revision = peg_revision;

            APR_ARRAY_PUSH( all_sources, svn_client_copy_source_t *) = source;
        }

        type_error_message = "expecting string for dest_url_or_path";
        Py::String dest_path( args.getUtf8String( name_dest_url_or_path ) );

        type_error_message = "expecting boolean for keyword copy_as_child";
        bool copy_as_child = args.getBoolean( name_copy_as_child, false );

        type_error_message = "expecting boolean for keyword make_parents";
        bool make_parents = args.getBoolean( name_make_parents, false );

        apr_hash_t *revprops = NULL;
        if( args.hasArg( name_revprops ) )
        {
            Py::Object py_revprop = args.getArg( name_revprops );
            if( !py_revprop.isNone() )
            {
                revprops = hashOfStringsFromDistOfStrings( py_revprop, pool );
            }
        }

#if defined( PYSVN_HAS_CLIENT_COPY5 )
        type_error_message = "expecting boolean for keyword ignore_externals";
        bool ignore_externals = args.getBoolean( name_ignore_externals, false );
#endif
        try
        {
            std::string norm_dest_path( svnNormalisedIfPath( dest_path, pool ) );

            checkThreadPermission();

            PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_COPY5 )
            svn_error_t *error = svn_client_copy5
                (
                &commit_info,
                all_sources,
                norm_dest_path.c_str(),
                copy_as_child,
                make_parents,
                ignore_externals,
                revprops,
                m_context,
                pool
                );
#else
            svn_error_t *error = svn_client_copy4
                (
                &commit_info,
                all_sources,
                norm_dest_path.c_str(),
                copy_as_child,
                make_parents,
                revprops,
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
    }
    catch( Py::TypeError & )
    {
        throw Py::TypeError( type_error_message );
    }

    return toObject( commit_info, m_commit_info_style );
}
#endif

Py::Object pysvn_client::cmd_copy( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_src_url_or_path },
    { true,  name_dest_url_or_path },
    { false, name_src_revision },
    { false, NULL }
    };
    FunctionArguments args( "copy", args_desc, a_args, a_kws );
    args.check();

    SvnPool pool( m_context );
    pysvn_commit_info_t *commit_info = NULL;

    std::string type_error_message;
    try
    {
        type_error_message = "expecting string for src_path (arg 1)";
        Py::String src_path( args.getUtf8String( name_src_url_or_path ) );

        type_error_message = "expecting string for dest_path (arg 2)";
        Py::String dest_path( args.getUtf8String( name_dest_url_or_path ) );

        type_error_message = "expecting revision for keyword src_revision";
        svn_opt_revision_t revision;
        if( is_svn_url( src_path ) )
            revision = args.getRevision( name_src_revision, svn_opt_revision_head );
        else
            revision = args.getRevision( name_src_revision, svn_opt_revision_working );

        try
        {
            std::string norm_src_path( svnNormalisedIfPath( src_path, pool ) );
            std::string norm_dest_path( svnNormalisedIfPath( dest_path, pool ) );

            checkThreadPermission();

            PythonAllowThreads permission( m_context );
#if defined( PYSVN_HAS_CLIENT_COPY3 )
            // behavior changed
            svn_error_t *error = svn_client_copy3
                (
                &commit_info,
                norm_src_path.c_str(),
                &revision,
                norm_dest_path.c_str(),
                m_context,
                pool
                );
#elif defined( PYSVN_HAS_CLIENT_COPY2 )
            svn_error_t *error = svn_client_copy2
                (
                &commit_info,       // commit info type changed
                norm_src_path.c_str(),
                &revision,
                norm_dest_path.c_str(),
                m_context,
                pool
                );
#else
            svn_error_t *error = svn_client_copy
                (
                &commit_info,
                norm_src_path.c_str(),
                &revision,
                norm_dest_path.c_str(),
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
    }
    catch( Py::TypeError & )
    {
        throw Py::TypeError( type_error_message );
    }

    return toObject( commit_info, m_commit_info_style );
}

#if defined( PYSVN_HAS_CLIENT_MOVE5 )
Py::Object pysvn_client::cmd_move2( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true, name_sources },
    { true, name_dest_url_or_path },
    { false, name_force },
    { false, name_move_as_child },
    { false, name_make_parents },
    { false, name_revprops },
    { false, NULL }
    };
    FunctionArguments args( "move2", args_desc, a_args, a_kws );
    args.check();

    SvnPool pool( m_context );
    pysvn_commit_info_t *commit_info = NULL;

    std::string type_error_message;
    try
    {
        type_error_message = "expecting list for sources (arg 1)";
        Py::List list_all_sources = args.getArg( name_sources );

        apr_array_header_t *all_sources =
            apr_array_make( pool, list_all_sources.length(), sizeof(const char *) );

        for( unsigned int index=0; index<list_all_sources.length(); index++ )
        {
            type_error_message = "expecting string in sources list";
            Py::String py_src_url_or_path( list_all_sources[ index ] );

            std::string src_url_or_path;
            src_url_or_path = py_src_url_or_path.as_std_string( name_utf8 );
            std::string norm_src_url_or_path( svnNormalisedIfPath( src_url_or_path, pool ) );

            const char *src_path_copy = apr_pstrdup( pool, norm_src_url_or_path.c_str() );

            APR_ARRAY_PUSH( all_sources, const char *) = src_path_copy;
        }

        type_error_message = "expecting string for dest_url_or_path";
        Py::String dest_path( args.getUtf8String( name_dest_url_or_path ) );

        type_error_message = "expecting boolean for keyword force";
        bool force = args.getBoolean( name_force, false );

        type_error_message = "expecting boolean for keyword move_as_child";
        bool move_as_child = args.getBoolean( name_move_as_child, false );

        type_error_message = "expecting boolean for keyword make_parents";
        bool make_parents = args.getBoolean( name_make_parents, false );

        apr_hash_t *revprops = NULL;
        if( args.hasArg( name_revprops ) )
        {
            Py::Object py_revprop = args.getArg( name_revprops );
            if( !py_revprop.isNone() )
            {
                revprops = hashOfStringsFromDistOfStrings( py_revprop, pool );
            }
        }

        try
        {
            std::string norm_dest_path( svnNormalisedIfPath( dest_path, pool ) );

            checkThreadPermission();
            PythonAllowThreads permission( m_context );

            // behavior changed
            svn_error_t *error = svn_client_move5
                (
                &commit_info,
                all_sources,
                norm_dest_path.c_str(),
                force,
                move_as_child,
                make_parents,
                revprops,
                m_context,
                pool
                );
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
    }
    catch( Py::TypeError & )
    {
        throw Py::TypeError( type_error_message );
    }

    return toObject( commit_info, m_commit_info_style );
}
#endif

Py::Object pysvn_client::cmd_move( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_src_url_or_path },
    { true,  name_dest_url_or_path },
    { false, name_force },
    { false, NULL }
    };
    FunctionArguments args( "move", args_desc, a_args, a_kws );
    args.check();

    SvnPool pool( m_context );
    pysvn_commit_info_t *commit_info = NULL;

    std::string type_error_message;
    try
    {
        type_error_message = "expecting string for src_url_or_path (arg 1)";
        Py::String src_path( args.getUtf8String( name_src_url_or_path ) );

        type_error_message = "expecting string for dest_url_or_path (arg 2)";
        Py::String dest_path( args.getUtf8String( name_dest_url_or_path ) );

#ifndef PYSVN_HAS_CLIENT_MOVE2
        svn_opt_revision_t revision;
        revision.kind = svn_opt_revision_head;
#endif

        type_error_message = "expecting boolean for keyword force";
        bool force = args.getBoolean( name_force, false );

        try
        {
            std::string norm_src_path( svnNormalisedIfPath( src_path, pool ) );
            std::string norm_dest_path( svnNormalisedIfPath( dest_path, pool ) );

            checkThreadPermission();

            PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_MOVE4 )
            // behavior changed
            svn_error_t *error = svn_client_move4
                (
                &commit_info,
                norm_src_path.c_str(),
                norm_dest_path.c_str(),
                force,
                m_context,
                pool
                );
#elif defined( PYSVN_HAS_CLIENT_MOVE3 )
            svn_error_t *error = svn_client_move3
                (
                &commit_info,               // changed type
                norm_src_path.c_str(),
                norm_dest_path.c_str(),
                force,
                m_context,
                pool
                );
#elif defined( PYSVN_HAS_CLIENT_MOVE2 )
            svn_error_t *error = svn_client_move2
                (
                &commit_info,
                norm_src_path.c_str(),
                norm_dest_path.c_str(),
                force,
                m_context,
                pool
                );
#else
            svn_error_t *error = svn_client_move
                (
                &commit_info,
                norm_src_path.c_str(),
                &revision,
                norm_dest_path.c_str(),
                force,
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
    }
    catch( Py::TypeError & )
    {
        throw Py::TypeError( type_error_message );
    }

    return toObject( commit_info, m_commit_info_style );
}
