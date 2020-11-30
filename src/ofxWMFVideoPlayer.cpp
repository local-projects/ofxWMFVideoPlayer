// ofxWMFVideoPlayer addon written by Philippe Laulheret for Second Story (secondstory.com)
// MIT Licensing


#include "ofxWMFVideoPlayer.h"

#include "ofLog.h"
#include "ofxWMFVideoPlayerUtils.h"

typedef std::pair<HWND, ofxWMFVideoPlayer *> PlayerItem;
list<PlayerItem>                             g_WMFVideoPlayers;


LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
// Message handlers


ofxWMFVideoPlayer *findPlayers( HWND hwnd )
{
    for( PlayerItem e : g_WMFVideoPlayers ) {
        if( e.first == hwnd )
            return e.second;
    }

    return NULL;
}


int ofxWMFVideoPlayer::_instanceCount = 0;


ofxWMFVideoPlayer::ofxWMFVideoPlayer()
    : _player( NULL )
{
    if( _instanceCount == 0 ) {
        if( !ofIsGLProgrammableRenderer() ) {
            if( wglewIsSupported( "WGL_NV_DX_interop" ) ) {
                ofLogVerbose( "ofxWMFVideoPlayer" ) << "WGL_NV_DX_interop supported";
            }
            else {
                ofLogError( "ofxWMFVideoPlayer" ) << "WGL_NV_DX_interop not supported. Upgrade your graphc drivers and try again.";
                return;
            }
        }


        HRESULT hr = MFStartup( MF_VERSION );
        if( !SUCCEEDED( hr ) ) {
            ofLog( OF_LOG_ERROR, "ofxWMFVideoPlayer: Error while loading MF" );
        }
    }

    _id = _instanceCount;
    _instanceCount++;
    this->InitInstance();

    bLoading = false;
    bRunning = false;
    _waitForLoadedToPlay = false;
    _sharedTextureCreated = false;
    _wantToSetVolume = false;
    _currentVolume = 1.0;
    _frameRate = 0.0f;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << "Created with id " << _id;
}


ofxWMFVideoPlayer::~ofxWMFVideoPlayer()
{
    if( _thread && _thread->joinable() ) {
        bRunning = false;
        _condition.notify_all();
        _thread->join();
    }
    _thread.reset();

    if( _player ) {
        _player->Shutdown();
        SafeRelease( &_player );
    }

    cout << "Player " << _id << " Terminated" << endl;
    _instanceCount--;
    if( _instanceCount == 0 ) {
        MFShutdown();

        cout << "Shutting down MF" << endl;
    }


    ofLogVerbose( "ofxWMFVideoPlayer" ) << "Terminated id " << _id;
}

void ofxWMFVideoPlayer::forceExit() const
{
    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Force Exit";

    if( _instanceCount != 0 ) {
        cout << "Shutting down MF some ofxWMFVideoPlayer remains" << endl;
        MFShutdown();
    }
}

bool ofxWMFVideoPlayer::loadMovie( string name )
{
    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Load Movie: " << name;

    loadEventSent = false;
    bLoaded = false;

    if( !_player ) {
        ofLogError( "ofxWMFVideoPlayer" ) << _id << " Player not created. Can't open the movie.";
        return false;
    }

    _player->Stop();

    if( !openUrl( std::move( name ) ) )
        return false;

    _frameRate = 0.0; // reset frameRate as the new movie loaded might have a different value than previous one

    createSharedTexture();

    // _waitForLoadedToPlay = false;

    return _sharedTextureCreated;
}

bool ofxWMFVideoPlayer::loadMovieAsync( string name )
{
    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Load Movie Async: " << name;

    loadEventSent = false;
    bLoaded = false;

    if( !_player ) {
        ofLogError( "ofxWMFVideoPlayer" ) << _id << " Player not created. Can't open the movie.";
        return false;
    }

    if( !_thread ) {
        bRunning = true;
        _thread = std::make_shared<std::thread>( &ofxWMFVideoPlayer::threadFn, this );
    }

    _player->Stop();

    std::lock_guard<std::mutex> lock( _mutex );
    bLoading = true;
    _async_name = std::move( name );
    _condition.notify_one();

    return true;
}

void ofxWMFVideoPlayer::threadFn()
{
    while( bRunning ) {
        std::unique_lock<std::mutex> lock( _mutex );
        _condition.wait( lock, [&]() { return !( _async_name.empty() && bRunning ); } );

        if( bRunning ) {
            const std::string name = _async_name;
            _async_name.clear();

            bLoading = true;
            bOpened = false;

            lock.unlock();

            const bool success = openUrl( name );

            lock.lock();

            bOpened = success;
        }
    }
}

bool ofxWMFVideoPlayer::openUrl( std::string name ) const
{
    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Open Url: " << name;

    _name = std::filesystem::path( name ).filename().string();

    const auto fileAttr = GetFileAttributesA( ofToDataPath( name ).c_str() );
    if( fileAttr == INVALID_FILE_ATTRIBUTES )
        return false;

    string       s = ofToDataPath( name );
    std::wstring w( s.length(), L' ' );
    std::copy( s.begin(), s.end(), w.begin() );

    return SUCCEEDED( _player->OpenURL( w.c_str() ) );
}

void ofxWMFVideoPlayer::createSharedTexture()
{
    if( !_sharedTextureCreated ) {
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Create Shared Texture";

        _width = _player->getWidth();
        _height = _player->getHeight();

        _tex.allocate( _width, _height, GL_RGB, true );

        _sharedTextureCreated = _player->m_pEVRPresenter->createSharedTexture( _width, _height, _tex.texData.textureID );
    }
    else if( ( _width != _player->getWidth() ) || ( _height != _player->getHeight() ) ) {
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Recreate Shared Texture";

        _player->m_pEVRPresenter->releaseSharedTexture();
        _sharedTextureCreated = false;

        _width = _player->getWidth();
        _height = _player->getHeight();

        _tex.allocate( _width, _height, GL_RGB, true );

        _sharedTextureCreated = _player->m_pEVRPresenter->createSharedTexture( _width, _height, _tex.texData.textureID );
    }
}

void ofxWMFVideoPlayer::deleteSharedTexture()
{
    if( !_player )
        return;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Delete Shared Texture";

    _player->m_pEVRPresenter->releaseSharedTexture();
    _sharedTextureCreated = false;
}


void ofxWMFVideoPlayer::draw( int x, int y, int w, int h )
{
    if( !_player )
        return;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Draw";

    _player->m_pEVRPresenter->lockSharedTexture();
    _tex.draw( x, y, w, h );
    _player->m_pEVRPresenter->unlockSharedTexture();
}

void ofxWMFVideoPlayer::bind()
{
    if( !_player )
        return;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Bind";

    _player->m_pEVRPresenter->lockSharedTexture();
    //_tex.setTextureWrap(GL_CLAMP_TO_BORDER, GL_CLAMP_TO_BORDER);
    _tex.bind();
}
void ofxWMFVideoPlayer::unbind()
{
    if( !_player )
        return;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Unbind";

    _tex.unbind();
    _player->m_pEVRPresenter->unlockSharedTexture();
}


bool ofxWMFVideoPlayer::isPlaying() const
{
    return _player->GetState() == Started;
}
bool ofxWMFVideoPlayer::isStopped() const
{
    return ( _player->GetState() == Stopped || _player->GetState() == Paused );
}

bool ofxWMFVideoPlayer::isPaused() const
{
    return _player->GetState() == Paused;
}


void ofxWMFVideoPlayer::close()
{
    if( !_player )
        return;

    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Close: " << _name;

    _player->Shutdown();
    _currentVolume = 1.0;
    _wantToSetVolume = false;
}
void ofxWMFVideoPlayer::update()
{
    if( !_player )
        return;

    ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Update";

    std::unique_lock<std::mutex> lock( _mutex );
    if( bLoading && bOpened ) {
        ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Finished loading asynchronously: " << _name;

        bLoading = false;
        _frameRate = 0.0; // reset frameRate as the new movie loaded might have a different value than previous one

        createSharedTexture();
    }

    if( bOpened && _waitForLoadedToPlay ) {
        ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Attempting to play: " << _name;
        switch( _player->Play() ) {
        case S_OK:
            ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Success!";
            _waitForLoadedToPlay = false;
            break;
        case MF_E_INVALIDREQUEST:
            ofLogWarning( "ofxWMFVideoPlayer" ) << _id << " Could not play due to invalid request: " << _name;
            break;
        case E_UNEXPECTED:
            ofLogWarning( "ofxWMFVideoPlayer" ) << _id << " Could not play due to unexpected error: " << _name;
            break;
        default:
            ofLogWarning( "ofxWMFVideoPlayer" ) << _id << " Could not play due to unknown error: " << _name;
            break;
        }
    }
    lock.unlock();

    if( ( _wantToSetVolume ) ) {
        _player->setVolume( _currentVolume );
    }
}

bool ofxWMFVideoPlayer::getIsMovieDone()
{
    bool bIsDone = false;
    if( getPosition() >= 0.99f )
        bIsDone = true;

    return bIsDone;
}

bool ofxWMFVideoPlayer::isLoaded() const
{
    if( !_player )
        return false;

    const PlayerState ps = _player->GetState();

    const auto isLoaded = ps == Paused || ps == Stopped || ps == Started;
    if( isLoaded )
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Is Loaded";
    else
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Is not loaded yet";

    return isLoaded;
}

unsigned char *ofxWMFVideoPlayer::getPixels()
{
    if( _tex.isAllocated() ) {
        _tex.readToPixels( _pixels );
        return _pixels.getPixels();
    }
    return nullptr;
}

bool ofxWMFVideoPlayer::setPixelFormat( ofPixelFormat pixelFormat )
{
    return ( pixelFormat == OF_PIXELS_RGB );
}

ofPixelFormat ofxWMFVideoPlayer::getPixelFormat()
{
    return OF_PIXELS_RGB;
}

bool ofxWMFVideoPlayer::isFrameNew()
{
    return true; // TODO fix this
}

void ofxWMFVideoPlayer::setPaused( bool bPause )
{
    if( bPause == true )
        pause();
    else
        play();
}

void ofxWMFVideoPlayer::play()
{
    if( !_player )
        return;

    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Play: " << _name;

    std::lock_guard<std::mutex> lock( _mutex );
    _waitForLoadedToPlay = bLoading || !( _player->Play() == S_OK );
}

void ofxWMFVideoPlayer::stop()
{
    if( !_player )
        return;

    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Stop: " << _name;

    _waitForLoadedToPlay = false;
    _player->Stop();
}

void ofxWMFVideoPlayer::pause()
{
    if( !_player )
        return;

    ofLogNotice( "ofxWMFVideoPlayer" ) << _id << " Pause: " << _name;

    _waitForLoadedToPlay = false;
    _player->Pause();
}

void ofxWMFVideoPlayer::setLoopState( ofLoopType loopType )
{
    switch( loopType ) {
    case OF_LOOP_NONE:
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Disable looping";
        setLoop( false );
        break;
    case OF_LOOP_NORMAL:
        ofLogVerbose( "ofxWMFVideoPlayer" ) << _id << " Enable looping";
        setLoop( true );
        break;
    default:
        ofLogError( "ofxWMFVideoPlayer" ) << _id << " Loop type not supported: " << loopType;
        break;
    }
}

float ofxWMFVideoPlayer::getPosition() const
{
    return ( _player->getPosition() / getDuration() );

    // this returns it in seconds
    //	return _player->getPosition();
}

float ofxWMFVideoPlayer::getDuration() const
{
    return _player->getDuration();
}

void ofxWMFVideoPlayer::setPosition( float pos ) const
{
    _player->setPosition( pos * getDuration() );
}

void ofxWMFVideoPlayer::setVolume( float vol )
{
    if( ( _player ) && ( _player->GetState() != OpenPending ) && ( _player->GetState() != Closing ) && ( _player->GetState() != Closed ) ) {
        _player->setVolume( vol );
        _wantToSetVolume = false;
    }
    else {
        _wantToSetVolume = true;
    }
    _currentVolume = vol;
}

float ofxWMFVideoPlayer::getVolume() const
{
    if( !_player )
        return _currentVolume;

    return _player->getVolume();
}

float ofxWMFVideoPlayer::getFrameRate()
{
    if( !_player )
        return 0.0f;
    if( _frameRate == 0.0f )
        _frameRate = _player->getFrameRate();
    return _frameRate;
}

float ofxWMFVideoPlayer::getHeight() const
{
    return _player->getHeight();
}
float ofxWMFVideoPlayer::getWidth() const
{
    return _player->getWidth();
}

void ofxWMFVideoPlayer::setLoop( bool isLooping )
{
    _isLooping = isLooping;

    if( _player )
        _player->setLooping( isLooping );
}


//-----------------------------------
// Prvate Functions
//-----------------------------------


// Handler for Media Session events.
void ofxWMFVideoPlayer::OnPlayerEvent( HWND hwnd, WPARAM pUnkPtr )
{
    HRESULT     hr = _player->HandleEvent( pUnkPtr );
    PlayerState state;

    if( _player->GetState() == 3 ) {

        if( !loadEventSent ) {
            bLoaded = true;
            ofNotifyEvent( videoLoadEvent, bLoaded, this );
            loadEventSent = true;
        }
    }

    if( FAILED( hr ) ) {
        if( !loadEventSent ) {
            bLoaded = false;
            ofNotifyEvent( videoLoadEvent, bLoaded, this );
            loadEventSent = true;
        }

        ofLogError( "ofxWMFVideoPlayer" ) << _id << " An error occurred.";
    }
}


LRESULT CALLBACK WndProcDummy( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{


    switch( message ) {

    case WM_CREATE: {
        return DefWindowProc( hwnd, message, wParam, lParam );
    }
    default: {
        ofxWMFVideoPlayer *myPlayer = findPlayers( hwnd );
        if( !myPlayer )
            return DefWindowProc( hwnd, message, wParam, lParam );
        return myPlayer->WndProc( hwnd, message, wParam, lParam );
    }
    }
    return 0;
}


LRESULT ofxWMFVideoPlayer::WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch( message ) {

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;


    case WM_APP_PLAYER_EVENT:
        OnPlayerEvent( hwnd, wParam );
        break;

    default:
        return DefWindowProc( hwnd, message, wParam, lParam );
    }
    return 0;
}


//  Create the application window.
BOOL ofxWMFVideoPlayer::InitInstance()
{
    PCWSTR     szWindowClass = L"MFBASICPLAYBACK";
    HWND       hwnd;
    WNDCLASSEX wcex;

    //   g_hInstance = hInst; // Store the instance handle.

    // Register the window class.
    ZeroMemory( &wcex, sizeof( WNDCLASSEX ) );
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;

    wcex.lpfnWndProc = WndProcDummy;
    //  wcex.hInstance      = hInst;
    wcex.hbrBackground = ( HBRUSH )( BLACK_BRUSH );
    // wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_MFPLAYBACK);
    wcex.lpszClassName = szWindowClass;
    // wcex.lpszClassName = "MFBASICPLAYBACK"; // CHANGED


    if( RegisterClassEx( &wcex ) == 0 ) {
        // return FALSE;
    }


    // Create the application window.
    hwnd = CreateWindow( szWindowClass, L"", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL );
    // hwnd = CreateWindow("MFBASICPLAYBACK", "", WS_OVERLAPPEDWINDOW,
    //	CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL); // CHANGED

    if( hwnd == 0 ) {
        return FALSE;
    }


    g_WMFVideoPlayers.push_back( std::pair<HWND, ofxWMFVideoPlayer *>( hwnd, this ) );
    HRESULT hr = CPlayer::CreateInstance( hwnd, hwnd, &_player );


    LONG style2 = ::GetWindowLong( hwnd, GWL_STYLE );
    style2 &= ~WS_DLGFRAME;
    style2 &= ~WS_CAPTION;
    style2 &= ~WS_BORDER;
    style2 &= WS_POPUP;
    LONG exstyle2 = ::GetWindowLong( hwnd, GWL_EXSTYLE );
    exstyle2 &= ~WS_EX_DLGMODALFRAME;
    ::SetWindowLong( hwnd, GWL_STYLE, style2 );
    ::SetWindowLong( hwnd, GWL_EXSTYLE, exstyle2 );


    _hwndPlayer = hwnd;


    UpdateWindow( hwnd );


    return TRUE;
}

void ofxWMFVideoPlayer::lock()
{
    _player->m_pEVRPresenter->lockSharedTexture();
}

void ofxWMFVideoPlayer::unlock()
{
    _player->m_pEVRPresenter->unlockSharedTexture();
}
