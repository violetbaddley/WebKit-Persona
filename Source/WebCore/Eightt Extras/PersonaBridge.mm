//
//  PersonaBridge.m
//  WebCore
//
//  Created by Talus Baddley on 2015-4-4.
//
//


#import "config.h"
#import "PersonaBridge.h"
#import <WebKit/WebView.h>

#import "URL.h"
#import "Cookie.h"
#import "Page.h"
//#import "WebFrameInternal.h"
#import "BrowsPersona.h"
#import "EIIGIsolatedCookieWebViewResourceLoadDelegate.h"

@interface PersonaBridge : NSObject

+ (NSString *)cookiesForDOMInPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url;
+ (NSString *)cookieRequestHeaderFieldValueForPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url;
+ (void)setCookies:(NSString *)cookieString fromDOMInPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url;

+ (NSArray *)rawCookiesForPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url;
+ (void)deleteCookie:(NSString *)cookieName withURL:(NSURL *)url forDOMInPage:(WebCore::Page *)page;

@end


namespace PersonaBridgeFunctions {
    WTF::String cookiesForDOM(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url) {
        NSString *cookieString = [PersonaBridge cookiesForDOMInPage:page firstPartyURL:firstParty URL:url];
        return cookieString ? WTF::String(cookieString) : WTF::String();  // We must return the null string instead of either nil or the empty string. Bizarre, yes; but it's JavaScript...
    }
    
    WTF::String cookieRequestHeaderFieldValue(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url) {
        NSString *cookieString = [PersonaBridge cookieRequestHeaderFieldValueForPage:page firstPartyURL:firstParty URL:url];
        return cookieString ? WTF::String(cookieString) : WTF::String();
    }
    
    void setCookiesFromDOM(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url, const WTF::String& cookieString) {
        [PersonaBridge setCookies:cookieString fromDOMInPage:page firstPartyURL:firstParty URL:url];
    }
    
    
    bool getRawCookies(WebCore::Page *page, const WebCore::URL& firstParty, const WebCore::URL& url, Vector<WebCore::Cookie>& rawCookies) {
        NSArray *nsCookies = [PersonaBridge rawCookiesForPage:page firstPartyURL:firstParty URL:url];
        
        rawCookies.clear();  rawCookies.reserveCapacity([nsCookies count]);
        for (NSHTTPCookie *cookie in nsCookies) {
            NSTimeInterval expires = [[cookie expiresDate] timeIntervalSince1970] * 1000;
            rawCookies.uncheckedAppend(WebCore::Cookie([cookie name], [cookie value], [cookie domain], [cookie path], expires,
                                                       [cookie isHTTPOnly], [cookie isSecure], [cookie isSessionOnly]));
        }
        
        return true;
    }
    
    void deleteCookie(WebCore::Page *page, const WebCore::URL& url, const WTF::String& cookieName) {
        [PersonaBridge deleteCookie:cookieName withURL:url forDOMInPage:page];
    }
    
    
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"

//
// Much of this logic shamelessly lifted from CookieJarMac.m
//

@implementation PersonaBridge

+ (BrowsPersona *)personaForPage:(WebCore::Page *)page {
    ASSERT(page->ei_browsPersona() != nil);  // FIXME Remove this after it never happens (or fix it if it does).
    return page->ei_browsPersona();
}


+ (NSString *)cookiesForSession:(BrowsPersona *)persona firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url includingHTTPOnly:(BOOL)includeHTTPOnly {
    NSArray *cookiesForURL = [persona cookiesForRequestAtURL:url];
    if (![cookiesForURL count])
        return nil;
    
    NSMutableArray *flattenedCookies = [NSMutableArray arrayWithCapacity:[cookiesForURL count]];
    for (NSHTTPCookie *cookie in cookiesForURL) {
        if (![[cookie name] length])
            continue;
        
        if (!includeHTTPOnly && [cookie isHTTPOnly])
            continue;
        
        [flattenedCookies addObject:[NSString stringWithFormat:@"%@=%@", [cookie name], [cookie value]]];
        
    }
    
    return [flattenedCookies componentsJoinedByString:@"; "];
    
}


+ (NSString *)cookiesForDOMInPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url {
    if (![self personaForPage:page])
        return nil;
    
    return [self cookiesForSession:[self personaForPage:page]
                     firstPartyURL:firstParty
                               URL:url
                 includingHTTPOnly:NO];
}


+ (NSString *)cookieRequestHeaderFieldValueForPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url {
    if (![self personaForPage:page])
        return nil;
    
    return [self cookiesForSession:[self personaForPage:page]
                     firstPartyURL:firstParty
                               URL:url
                 includingHTTPOnly:YES];
}


+ (void)setCookies:(NSString *)cookieString fromDOMInPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url {
    NSLog(@"Setting cookies “%@” first party URL %@ at URL %@", cookieString, firstParty, url);
    
    BrowsPersona *persona = [self personaForPage:page];
    if (!persona)
        return;
    
    if (![cookieString length])
        return;
    
    NSString *cookiesMaybeWithoutValues = [cookieString containsString:@"="] ? cookieString : [cookieString stringByAppendingString:@"="];
    NSArray *parsedCookies = [NSHTTPCookie cookiesWithResponseHeaderFields:@{ @"Set-Cookie": cookiesMaybeWithoutValues }
                                                                    forURL:url];
    NSMutableArray *filteredCookies = [NSMutableArray arrayWithCapacity:[parsedCookies count]];
    for (NSHTTPCookie *cookie in parsedCookies) {
        if ([[cookie name] length]  &&  ![cookie isHTTPOnly])
            [filteredCookies addObject:cookie];
    }
    
    if ([filteredCookies count] > 1) {
        NSLog(@"A script at “%@” violated syntax rules for setting cookies, possibly attempting to exploit a flaw in +[NSHTTPCookie cookiesWithResponseHeaderFields:forURL:]", firstParty);
        return;
    }
    
    [persona setCookies:filteredCookies forURL:url mainDocumentURL:firstParty];
    
}


+ (NSArray *)rawCookiesForPage:(WebCore::Page *)page firstPartyURL:(NSURL *)firstParty URL:(NSURL *)url {
    return [[self personaForPage:page] cookiesForRequestAtURL:url];
}


+ (void)deleteCookie:(NSString *)cookieName withURL:(NSURL *)url forDOMInPage:(WebCore::Page *)page {
    BrowsPersona *persona = [self personaForPage:page];
    NSArray *potentialDeletions = [persona cookiesForRequestAtURL:url];
    if (!persona)
        return;
    
    for (NSHTTPCookie *cookie in potentialDeletions) {
        if ([[cookie name] isEqual:cookieName])
            [persona removeCookieWithName:[cookie name]
                                   domain:[cookie domain]
                                     path:[cookie path]];
    }
    
}


#pragma clang diagnostic pop

@end










