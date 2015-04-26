//
//  PersonaBridge.h
//  WebCore
//
//  Created by Talus Baddley on 2015-4-4.
//
//

#ifdef __OBJC__
#import <Foundation/Foundation.h>
#endif
#import "PlatformCookieJar.h"

namespace WebCore {
    class Document;
    class Page;
}

namespace PersonaBridgeFunctions {
    WTF::String cookiesForDOM(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url);
    WTF::String cookieRequestHeaderFieldValue(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url);
    void setCookiesFromDOM(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url, const WTF::String& cookieString);
    
    bool getRawCookies(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url, Vector<WebCore::Cookie>& rawCookies);
    void deleteCookie(WebCore::Page *page, const WebCore::URL& url, const WTF::String& cookieName);
    
}


