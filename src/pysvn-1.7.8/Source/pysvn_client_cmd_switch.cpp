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

Py::Object pysvn_client::cmd_relocate( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_from_url },
    { true,  name_to_url },
    { true,  name_path },
    { false, name_recurse },
    { false, NULL }
    };
    FunctionArguments args( "relocate", args_desc, a_args, a_kws );
    args.check();

    std::string from_url( args.getUtf8String( name_from_url ) );
    std::string to_url( args.getUtf8String( name_to_url ) );
    std::string path( args.getUtf8String( name_path ) );

    bool recurse = args.getBoolean( name_recurse, true );

    SvnPool pool( m_context );

    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );
        std::string norm_to_url( svnNormalisedIfPath( to_url, pool ) );
        std::string norm_from_url( svnNormalisedIfPath( from_url, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

        svn_error_t *error = svn_client_relocate
            (
            norm_path.c_str(),
            norm_from_url.c_str(),
            norm_to_url.c_str(),
            recurse,
            m_context,
            pool
            );
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

    return Py::None();
}

Py::Object pysvn_client::cmd_switch( const Py::Tuple &a_args, const Py::Dict &a_kws )
{
    static argument_description args_desc[] =
    {
    { true,  name_path },
    { true,  name_url },
    { false, name_recurse },
    { false, name_revision },
#if defined( PYSVN_HAS_CLIENT_SWITCH2 )
    { false, name_depth },
    { false, name_peg_revision },
    { false, name_depth_is_sticky },
    { false, name_ignore_externals },
    { false, name_allow_unver_obstructions },
#endif
    { false, NULL }
    };
    FunctionArguments args( "switch", args_desc, a_args, a_kws );
    args.check();

    std::string path( args.getUtf8String( name_path ) );
    std::string url( args.getUtf8String( name_url ) );
    svn_opt_revision_t revision = args.getRevision( name_revision, svn_opt_revision_head );
#if defined( PYSVN_HAS_CLIENT_SWITCH2 )
    svn_depth_t depth = args.getDepth( name_depth, name_recurse, svn_depth_infinity, svn_depth_infinity, svn_depth_files );
    svn_opt_revision_t peg_revision = args.getRevision( name_peg_revision, revision );

    svn_boolean_t depth_is_sticky = args.getBoolean( name_depth_is_sticky, false );
    svn_boolean_t ignore_externals = args.getBoolean( name_ignore_externals, false );
    svn_boolean_t allow_unver_obstructions = args.getBoolean( name_allow_unver_obstructions, false );
#else
    bool recurse = args.getBoolean( name_recurse, true );
#endif
    SvnPool pool( m_context );

    svn_revnum_t revnum = 0;
    try
    {
        std::string norm_path( svnNormalisedIfPath( path, pool ) );
        std::string norm_url( svnNormalisedIfPath( url, pool ) );

        checkThreadPermission();

        PythonAllowThreads permission( m_context );

#if defined( PYSVN_HAS_CLIENT_SWITCH2 )
        svn_error_t *error = svn_client_switch2
            (
            &revnum,
            norm_path.c_str(),
            norm_url.c_str(),
            &peg_revision,
            &revision,
            depth,
            depth_is_sticky,
            ignore_externals,
            allow_unver_obstructions,
            m_context,
            pool
            );
#else
        svn_error_t *error = svn_client_switch
            (
            &revnum,
            norm_path.c_str(),
            norm_url.c_str(),
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

    return Py::asObject( new pysvn_revision( svn_opt_revision_number, 0, revnum ) );
}
