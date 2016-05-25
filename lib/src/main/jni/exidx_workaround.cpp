//
// Created by sevar on 16-5-25.
//
// SV workaround for java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate symbol "__exidx_end"
// Happens on NDKs later than r9d
// https://code.google.com/p/android/issues/detail?id=109071
// http://stackoverflow.com/questions/13555376/cocos2d-x-game-fresh-off-the-play-store-cant-even-open

#ifdef __cplusplus
    extern "C" {
#endif

void __exidx_start() {}
void __exidx_end()   {}

#ifdef __cplusplus
    }
#endif