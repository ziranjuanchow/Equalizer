/* Copyright (c) 2005-2008, Stefan Eilemann <eile@equalizergraphics.com>
                          , Makhinya Maxim
   All rights reserved. */

#include "glXWindow.h"
#include "global.h"

namespace eq
{


GLXWindow::GLXWindow( Window* parent )
    : OSWindow( parent )
    , _xDrawable ( 0 )
    , _glXContext( 0 )
{
    
}

GLXWindow::~GLXWindow( )
{
    
}

//---------------------------------------------------------------------------
// GLX init
//---------------------------------------------------------------------------
#ifdef GLX
static Bool WaitForNotify( Display*, XEvent *e, char *arg )
{ return (e->type == MapNotify) && (e->xmap.window == (::Window)arg); }
#endif

bool GLXWindow::configInit( )
{
#ifdef GLX
    XVisualInfo* visualInfo = chooseXVisualInfo();
    if( !visualInfo )
        return false;

    GLXContext context = createGLXContext( visualInfo );
    setGLXContext( context );

    if( !context )
        return false;

    const bool success = configInitGLXDrawable( visualInfo );
    XFree( visualInfo );

    if( success && !_xDrawable )
    {
        setErrorMessage( "configInitGLXDrawable did set no X11 drawable" );
        return false;
    }

    return success;    
#else
    setErrorMessage( "Client library compiled without GLX support" );
    return false;
#endif
}


XVisualInfo* GLXWindow::chooseXVisualInfo()
{
#ifdef GLX
    Display* display = getPipe()->getXDisplay();
    if( !display )
    {
        setErrorMessage( "Pipe has no X11 display connection" );
        return 0;
    }

    // build attribute list
    std::vector<int> attributes;
    attributes.push_back( GLX_RGBA );

    const int colorSize = getIAttribute( Window::IATTR_PLANES_COLOR );
    if( colorSize > 0 || colorSize == eq::AUTO )
    {
        attributes.push_back( GLX_RED_SIZE );
        attributes.push_back( colorSize>0 ? colorSize : 1 );
        attributes.push_back( GLX_GREEN_SIZE );
        attributes.push_back( colorSize>0 ? colorSize : 1 );
        attributes.push_back( GLX_BLUE_SIZE );
        attributes.push_back( colorSize>0 ? colorSize : 1 );
    }
    const int alphaSize = getIAttribute( Window::IATTR_PLANES_ALPHA );
    if( alphaSize > 0 || alphaSize == AUTO )
    {
        attributes.push_back( GLX_ALPHA_SIZE );
        attributes.push_back( alphaSize>0 ? alphaSize : 1 );
    }
    const int depthSize = getIAttribute( Window::IATTR_PLANES_DEPTH );
    if( depthSize > 0  || depthSize == AUTO )
    {
        attributes.push_back( GLX_DEPTH_SIZE );
        attributes.push_back( depthSize>0 ? depthSize : 1 );
    }
    const int stencilSize = getIAttribute( Window::IATTR_PLANES_STENCIL );
    if( stencilSize > 0 || stencilSize == AUTO )
    {
        attributes.push_back( GLX_STENCIL_SIZE );
        attributes.push_back( stencilSize>0 ? stencilSize : 1 );
    }
    const int accumSize = getIAttribute( Window::IATTR_PLANES_ACCUM );
    const int accumAlpha = getIAttribute( Window::IATTR_PLANES_ACCUM_ALPHA );
    if( accumSize >= 0 )
    {
        attributes.push_back( GLX_ACCUM_RED_SIZE );
        attributes.push_back( accumSize );
        attributes.push_back( GLX_ACCUM_GREEN_SIZE );
        attributes.push_back( accumSize );
        attributes.push_back( GLX_ACCUM_BLUE_SIZE );
        attributes.push_back( accumSize );
        attributes.push_back( GLX_ACCUM_ALPHA_SIZE );
        attributes.push_back( accumAlpha >= 0 ? accumAlpha : accumSize );
    }
    else if( accumAlpha >= 0 )
    {
        attributes.push_back( GLX_ACCUM_ALPHA_SIZE );
        attributes.push_back( accumAlpha );
    }

    const int samplesSize  = getIAttribute( Window::IATTR_PLANES_SAMPLES );
    if( samplesSize >= 0 )
    {
        attributes.push_back( GLX_SAMPLE_BUFFERS );
        attributes.push_back( 1 );
        attributes.push_back( GLX_SAMPLES );
        attributes.push_back( samplesSize );
    }
    
#ifdef DARWIN
    // WAR: glDrawBuffer( GL_BACK ) renders only to the left back buffer on a
    // stereo visual on Darwin which creates ugly flickering on mono configs
    if( getIAttribute( Window::IATTR_HINT_STEREO ) == ON )
        attributes.push_back( GLX_STEREO );
#else
    if( getIAttribute( Window::IATTR_HINT_STEREO ) == ON ||
        ( getIAttribute( Window::IATTR_HINT_STEREO )   == AUTO && 
          getIAttribute( Window::IATTR_HINT_DRAWABLE ) == WINDOW ))

        attributes.push_back( GLX_STEREO );
#endif
    if( getIAttribute( Window::IATTR_HINT_DOUBLEBUFFER ) == ON ||
        ( getIAttribute( Window::IATTR_HINT_DOUBLEBUFFER ) == AUTO && 
          getIAttribute( Window::IATTR_HINT_DRAWABLE )     == WINDOW ))

        attributes.push_back( GLX_DOUBLEBUFFER );

    attributes.push_back( None );

    // build backoff list, least important attribute last
    std::vector<int> backoffAttributes;
    if( getIAttribute( Window::IATTR_HINT_DRAWABLE ) == WINDOW )
    {
        if( getIAttribute( Window::IATTR_HINT_DOUBLEBUFFER ) == AUTO )
            backoffAttributes.push_back( GLX_DOUBLEBUFFER );

#ifndef DARWIN
        if( getIAttribute( Window::IATTR_HINT_STEREO ) == AUTO )
            backoffAttributes.push_back( GLX_STEREO );
#endif
    }

    if( stencilSize == AUTO )
        backoffAttributes.push_back( GLX_STENCIL_SIZE );


    // Choose visual
    const int    screen  = DefaultScreen( display );
    XVisualInfo *visInfo = glXChooseVisual( display, screen, 
                                            &attributes.front( ));

    while( !visInfo && !backoffAttributes.empty( ))
    {   // Gradually remove backoff attributes
        const int attribute = backoffAttributes.back();
        backoffAttributes.pop_back();

        std::vector<int>::iterator iter = find( attributes.begin(), attributes.end(),
                                           attribute );
        EQASSERT( iter != attributes.end( ));
        if( *iter == GLX_STENCIL_SIZE ) // two-elem attribute
            attributes.erase( iter, iter+2 );
        else                            // one-elem attribute
            attributes.erase( iter );

        visInfo = glXChooseVisual( display, screen, &attributes.front( ));
    }

    if ( !visInfo )
        setErrorMessage( "Could not find a matching visual" );
    else
        EQINFO << "Using visual 0x" << std::hex << visInfo->visualid
               << std::dec << std::endl;

    return visInfo;
#else
    setErrorMessage( "Client library compiled without GLX support" );
    return 0;
#endif
}

GLXContext GLXWindow::createGLXContext( XVisualInfo* visualInfo )
{
#ifdef GLX
    if( !visualInfo )
    {
        setErrorMessage( "No visual info given" );
        return 0;
    }

    Pipe*    pipe    = getPipe();
    Display* display = pipe->getXDisplay();
    if( !display )
    {
        setErrorMessage( "Pipe has no X11 display connection" );
        return 0;
    }

    Window* firstWindow = pipe->getWindows()[0];
    const GLXWindow* osFirstWindow = static_cast<const GLXWindow*>( firstWindow->getOSWindow());

    GLXContext shareCtx = osFirstWindow->getGLXContext();
    GLXContext  context = glXCreateContext(display, visualInfo, shareCtx, True);

    if ( !context )
    {
        setErrorMessage( "Could not create OpenGL context" );
        return 0;
    }

    return context;
#else
    setErrorMessage( "Client library compiled without GLX support" );
    return 0;
#endif
}

bool GLXWindow::configInitGLXDrawable( XVisualInfo* visualInfo )
{
    switch( getIAttribute( Window::IATTR_HINT_DRAWABLE ))
    {
        case PBUFFER:
            return configInitGLXPBuffer( visualInfo );

        default:
            EQWARN << "Unknown drawable type " 
                   << getIAttribute(Window::IATTR_HINT_DRAWABLE ) << ", using window" 
                   << std::endl;
            // no break;
        case UNDEFINED:
        case WINDOW:
            return configInitGLXWindow( visualInfo );
    }
}

bool GLXWindow::configInitGLXWindow( XVisualInfo* visualInfo )
{
#ifdef GLX
    EQASSERT( getIAttribute( Window::IATTR_HINT_DRAWABLE ) != PBUFFER )

    if( !visualInfo )
    {
        setErrorMessage( "No visual info given" );
        return false;
    }

    Pipe*    pipe    = getPipe();
    Display* display = pipe->getXDisplay();
    if( !display )
    {
        setErrorMessage( "Pipe has no X11 display connection" );
        return false;
    }

    const int            screen = DefaultScreen( display );
    XID                  parent = RootWindow( display, screen );
    XSetWindowAttributes wa;
    wa.colormap          = XCreateColormap( display, parent, visualInfo->visual,
                                            AllocNone );
    wa.background_pixmap = None;
    wa.border_pixel      = 0;
    wa.event_mask        = StructureNotifyMask | VisibilityChangeMask |
                           ExposureMask | KeyPressMask | KeyReleaseMask |
                           PointerMotionMask | ButtonPressMask |
                           ButtonReleaseMask;

    if( getIAttribute( Window::IATTR_HINT_DECORATION ) != OFF )
        wa.override_redirect = False;
    else
        wa.override_redirect = True;

    PixelViewport pvp = _getAbsPVP();
    if( getIAttribute( Window::IATTR_HINT_FULLSCREEN ) == ON )
    {
        wa.override_redirect = True;
        pvp.h = DisplayHeight( display, screen );
        pvp.w = DisplayWidth( display, screen );
        pvp.x = 0;
        pvp.y = 0;
        _setAbsPVP( pvp );
    }

    XID drawable = XCreateWindow( display, parent, 
                                  pvp.x, pvp.y, pvp.w, pvp.h,
                                  0, visualInfo->depth, InputOutput,
                                  visualInfo->visual, 
                                  CWBackPixmap | CWBorderPixel |
                                  CWEventMask | CWColormap | CWOverrideRedirect,
                                  &wa );
    
    if ( !drawable )
    {
        setErrorMessage( "Could not create window" );
        return false;
    }   

    std::stringstream windowTitle;

    const std::string& name = getName();
    if( name.empty( ))
    {
        windowTitle << "Equalizer";
#ifndef NDEBUG
        windowTitle << " (" << getpid() << ")";
#endif
    }
    else
        windowTitle << name;

    XStoreName( display, drawable, windowTitle.str().c_str( ));

    // Register for close window request from the window manager
    Atom deleteAtom = XInternAtom( display, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( display, drawable, &deleteAtom, 1 );

    // map and wait for MapNotify event
    XMapWindow( display, drawable );

    XEvent event;
    XIfEvent( display, &event, WaitForNotify, (XPointer)(drawable) );

    XMoveResizeWindow( display, drawable, pvp.x, pvp.y, pvp.w, pvp.h );
    XFlush( display );

    // Grab keyboard focus in fullscreen mode
    if( getIAttribute( Window::IATTR_HINT_FULLSCREEN ) == ON )
        XGrabKeyboard( display, drawable, True, GrabModeAsync, GrabModeAsync, 
                       CurrentTime );

    setXDrawable( drawable );

    EQINFO << "Created X11 drawable " << drawable << std::endl;
    return true;
#else
    setErrorMessage( "Client library compiled without GLX support" );
    return false;
#endif
}

bool GLXWindow::configInitGLXPBuffer( XVisualInfo* visualInfo )
{
#ifdef GLX
    EQASSERT( getIAttribute( Window::IATTR_HINT_DRAWABLE ) == PBUFFER )

    if( !visualInfo )
    {
        setErrorMessage( "No visual info given" );
        return false;
    }

    const Pipe*    pipe    = getPipe();
    EQASSERT( pipe );

    Display* display = pipe->getXDisplay();
    if( !display )
    {
        setErrorMessage( "Pipe has no X11 display connection" );
        return false;
    }

    // Check for GLX >= 1.3
    int major = 0;
    int minor = 0;
    if( !glXQueryVersion( display, &major, &minor ))
    {
        setErrorMessage( "Can't get GLX version" );
        return false;
    }

    if( major < 1 || (major == 1 && minor < 3 ))
    {
        setErrorMessage( "Need at least GLX 1.3" );
        return false;
    }

    // Find FB config for X visual
    const int    screen   = DefaultScreen( display );
    int          nConfigs = 0;
    GLXFBConfig* configs  = glXGetFBConfigs( display, screen, &nConfigs );
    GLXFBConfig  config   = 0;

    for( int i = 0; i < nConfigs; ++i )
    {
        int visualID;
        if( glXGetFBConfigAttrib( display, configs[i], GLX_VISUAL_ID, 
                                  &visualID ) == 0 )
        {
            if( visualID == static_cast< int >( visualInfo->visualid ))
            {
                config = configs[i];
                break;
            }
        }
    }

    if( !config )
    {
        setErrorMessage( "Can't find FBConfig for visual" );
        return false;
    }

    // Create PBuffer
    const PixelViewport& pvp = _getAbsPVP();
    const int attributes[] = { GLX_PBUFFER_WIDTH, pvp.w,
                               GLX_PBUFFER_HEIGHT, pvp.h,
                               GLX_LARGEST_PBUFFER, True,
                               GLX_PRESERVED_CONTENTS, True,
                               0 };

    XID pbuffer = glXCreatePbuffer( display, config, attributes );
    if ( !pbuffer )
    {
        setErrorMessage( "Could not create PBuffer" );
        return false;
    }   
   
    XFlush( display );
    setXDrawable( pbuffer );

    EQINFO << "Created X11 PBuffer " << pbuffer << std::endl;
    return true;
#else
    setErrorMessage( "Client library compiled without GLX support" );
    return false;
#endif
}


void GLXWindow::setXDrawable( XID drawable )
{
#ifdef GLX
    if( _xDrawable == drawable )
        return;

    if( _xDrawable )
        exitEventHandler();
    _xDrawable = drawable;
    if( _xDrawable )
        initEventHandler();

    if( _xDrawable && _glXContext )
        _initializeGLData();
    else
        _clearGLData();

    if( !drawable )
        return;

    // query pixel viewport of window
    Pipe*    pipe    = getPipe();
    EQASSERT( pipe );

    Display *display = pipe->getXDisplay();
    EQASSERT( display );

    PixelViewport pvp;
    if( getIAttribute( Window::IATTR_HINT_DRAWABLE ) == PBUFFER )
    {
        pvp.x = 0;
        pvp.y = 0;
        
        unsigned value = 0;
        glXQueryDrawable( display, drawable, GLX_WIDTH,  &value );
        pvp.w = static_cast< int32_t >( value );

        value = 0;
        glXQueryDrawable( display, drawable, GLX_HEIGHT, &value );
        pvp.h = static_cast< int32_t >( value );
    }
    else
    {
        XWindowAttributes wa;
        XGetWindowAttributes( display, drawable, &wa );
    
        // Window position is relative to parent: translate to absolute coords
        ::Window root, parent, *children;
        unsigned nChildren;
    
        XQueryTree( display, drawable, &root, &parent, &children, &nChildren );
        if( children != 0 ) XFree( children );

        int x,y;
        ::Window childReturn;
        XTranslateCoordinates( display, parent, root, wa.x, wa.y, &x, &y,
                               &childReturn );

        pvp.x = x;
        pvp.y = y;
        pvp.w = wa.width;
        pvp.h = wa.height;
    }

    setPixelViewport( pvp );
#endif // GLX
}


void GLXWindow::setGLXContext( GLXContext context )
{
#ifdef GLX
    _glXContext = context;

    if( _xDrawable && _glXContext )
        _initializeGLData();
    else
        _clearGLData();
#endif
}

void GLXWindow::configExit( )
{
#ifdef GLX
    Pipe*    pipe    = getPipe();
    EQASSERT( pipe );

    Display *display = pipe->getXDisplay();
    if( !display ) 
        return;

    glXMakeCurrent( display, None, 0 );

    GLXContext context  = getGLXContext();
    XID        drawable = getXDrawable();

    setGLXContext( 0 );
    setXDrawable( 0 );

    if( context )
        glXDestroyContext( display, context );

    if( drawable )
    {
        if( getIAttribute( Window::IATTR_HINT_DRAWABLE ) == PBUFFER )
            glXDestroyPbuffer( display, drawable );
        else
            XDestroyWindow( display, drawable );
    }

    EQINFO << "Destroyed GLX context and X drawable " << std::endl;
#endif
}

void GLXWindow::makeCurrent() const
{
#ifdef GLX
    Pipe*    pipe    = getPipe();

    EQASSERT( pipe );
    EQASSERT( pipe->getXDisplay( ));

    glXMakeCurrent( pipe->getXDisplay(), _xDrawable, _glXContext );
#endif
}

void GLXWindow::swapBuffers()
{
#ifdef GLX
    Pipe*    pipe    = getPipe();

    EQASSERT( pipe );
    EQASSERT( pipe->getXDisplay( ));

    glXSwapBuffers( pipe->getXDisplay(), _xDrawable );
#endif
}

bool GLXWindow::isInitialized() const
{
    return _xDrawable && _glXContext;
}

bool GLXWindow::checkConfigInit() const
{
    return true;
}

}
