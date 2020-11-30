#pragma once


// ofxWMFVideoPlayer addon written by Philippe Laulheret for Second Story (secondstory.com)
// Based upon Windows SDK samples
// MIT Licensing


#include "EVRPresenter.h"
#include "ofMain.h"
#include "ofxWMFVideoPlayerUtils.h"


class ofxWMFVideoPlayer;


class CPlayer;
class ofxWMFVideoPlayer /*: public ofBaseVideoPlayer */ {

  private:
    static int _instanceCount;


    HWND _hwndPlayer;

    BOOL bRepaintClient;


    int _width;
    int _height;


    bool  _waitForLoadedToPlay;
    bool  _isLooping;
    bool  _wantToSetVolume;
    float _currentVolume;

    bool _sharedTextureCreated;

    ofTexture _tex;
    ofPixels  _pixels;

    BOOL InitInstance();


    void OnPlayerEvent( HWND hwnd, WPARAM pUnkPtr );

    bool loadEventSent;
    bool bLoaded;


    float _frameRate;

    void threadFn();
    bool openUrl( std::string name ) const;
    void createSharedTexture();
    void deleteSharedTexture();

    std::shared_ptr<std::thread> _thread;
    std::mutex                   _mutex;
    std::condition_variable      _condition;
    volatile bool                bRunning;
    volatile bool                bLoading;
    volatile bool                bOpened;
    std::string                  _async_name;

    mutable std::string _name;

  public:
    CPlayer *_player;

    int _id;

    ofxWMFVideoPlayer();
    ~ofxWMFVideoPlayer();

    bool               loadMovie( string name );
    bool               loadMovieAsync( string name );
    const std::string &getMovie() const { return _name; }

    void close();
    void update();

    void play();
    void stop();
    void pause();
    void setPaused( bool bPause );

    float getPosition() const;
    float getDuration() const;
    float getFrameRate();

    void setPosition( float pos ) const;

    void  setVolume( float vol );
    float getVolume() const;

    float getHeight() const;
    float getWidth() const;

    bool isPlaying() const;
    bool isStopped() const;
    bool isPaused() const;

    void setLoop( bool isLooping );
    bool isLooping() { return _isLooping; }

    void bind();
    void unbind();

    void lock();
    void unlock();

    ofEvent<bool> videoLoadEvent;


    void setLoopState( ofLoopType loopType );
    bool getIsMovieDone();

    bool isLoaded() const;

    unsigned char *      getPixels();
    ofPixels &           getPixelsRef() { return _pixels; }
    ofTexture *          getTexture() { return &_tex; };
    static bool          setPixelFormat( ofPixelFormat pixelFormat );
    static ofPixelFormat getPixelFormat();

    static bool isFrameNew();


    void draw( int x, int y, int w, int h );
    void draw( int x, int y ) { draw( x, y, getWidth(), getHeight() ); }

    HWND    getHandle() { return _hwndPlayer; }
    LRESULT WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

    void forceExit() const;
};