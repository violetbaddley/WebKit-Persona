/*
 * Copyright (C) 2012-2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WKGeolocationProviderIOS.h"

#if PLATFORM(IOS)

#import "APISecurityOrigin.h"
#import "GeolocationPermissionRequestProxy.h"
#import "WKUIDelegatePrivate.h"
#import "WKWebView.h"
#import "WebGeolocationManagerProxy.h"
#import "WebProcessPool.h"
#import <WebCore/GeolocationPosition.h>
#import <WebCore/URL.h>
#import <WebGeolocationPosition.h>
#import <wtf/Assertions.h>
#import <wtf/HashSet.h>
#import <wtf/PassRefPtr.h>
#import <wtf/RefPtr.h>
#import <wtf/RetainPtr.h>

// FIXME: Remove use of WebKit1 from WebKit2
#import <WebKit/WebGeolocationCoreLocationProvider.h>
#import <WebKit/WebAllowDenyPolicyListener.h>

using namespace WebCore;
using namespace WebKit;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

@interface WKGeolocationProviderIOS (WebGeolocationCoreLocationUpdateListener) <WebGeolocationCoreLocationUpdateListener>
@end

@interface WKWebAllowDenyPolicyListener : NSObject<WebAllowDenyPolicyListener>
- (id)initWithPermissionRequestProxy:(PassRefPtr<GeolocationPermissionRequestProxy>)permissionRequestProxy;
- (void)denyOnlyThisRequest NO_RETURN_DUE_TO_ASSERT;
@end

namespace WebKit {
void decidePolicyForGeolocationRequestFromOrigin(SecurityOrigin*, const String& urlString, id<WebAllowDenyPolicyListener>, UIWindow*);
};

struct GeolocationRequestData {
    RefPtr<SecurityOrigin> origin;
    RefPtr<WebFrameProxy> frame;
    RefPtr<GeolocationPermissionRequestProxy> permissionRequest;
    RetainPtr<WKWebView> view;
};

@implementation WKGeolocationProviderIOS {
    RefPtr<WebGeolocationManagerProxy> _geolocationManager;
    RetainPtr<WebGeolocationCoreLocationProvider> _coreLocationProvider;
    BOOL _isWebCoreGeolocationActive;
    RefPtr<WebGeolocationPosition> _lastActivePosition;
    Vector<GeolocationRequestData> _requestsWaitingForCoreLocationAuthorization;
}

#pragma mark - WKGeolocationProvider callbacks implementation.

static void startUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    WKGeolocationProviderIOS *geolocationProvider = reinterpret_cast<WKGeolocationProviderIOS*>(const_cast<void*>(clientInfo));
    ASSERT([geolocationProvider isKindOfClass:[WKGeolocationProviderIOS class]]);
    [geolocationProvider _startUpdating];
}

static void stopUpdatingCallback(WKGeolocationManagerRef geolocationManager, const void* clientInfo)
{
    WKGeolocationProviderIOS *geolocationProvider = reinterpret_cast<WKGeolocationProviderIOS*>(const_cast<void*>(clientInfo));
    ASSERT([geolocationProvider isKindOfClass:[WKGeolocationProviderIOS class]]);
    [geolocationProvider _stopUpdating];
}

static void setEnableHighAccuracy(WKGeolocationManagerRef geolocationManager, bool enable, const void* clientInfo)
{
    WKGeolocationProviderIOS *geolocationProvider = reinterpret_cast<WKGeolocationProviderIOS*>(const_cast<void*>(clientInfo));
    ASSERT([geolocationProvider isKindOfClass:[WKGeolocationProviderIOS class]]);
    [geolocationProvider _setEnableHighAccuracy:enable];
}

-(void)_startUpdating
{
    _isWebCoreGeolocationActive = YES;
    [_coreLocationProvider start];

    // If we have the last position, it is from the initialization or warm up. It is the last known
    // good position so we can return it directly.
    if (_lastActivePosition)
        _geolocationManager->providerDidChangePosition(_lastActivePosition.get());
}

-(void)_stopUpdating
{
    _isWebCoreGeolocationActive = NO;
    [_coreLocationProvider stop];
    _lastActivePosition.clear();
}

-(void)_setEnableHighAccuracy:(BOOL)enableHighAccuracy
{
    [_coreLocationProvider setEnableHighAccuracy:enableHighAccuracy];
}

#pragma mark - Public API implementation.

-(id)init
{
    ASSERT_NOT_REACHED();
    [self release];
    return nil;
}

-(id)initWithProcessPool:(WebProcessPool&)processPool
{
    self = [super init];
    if (!self)
        return nil;
    _geolocationManager = processPool.supplement<WebGeolocationManagerProxy>();
    WKGeolocationProvider providerCallback = {
        kWKGeolocationProviderCurrentVersion,
        self,
        startUpdatingCallback,
        stopUpdatingCallback,
        setEnableHighAccuracy
    };
    _geolocationManager->initializeProvider(reinterpret_cast<WKGeolocationProviderBase*>(&providerCallback));
    _coreLocationProvider = adoptNS([[WebGeolocationCoreLocationProvider alloc] initWithListener:self]);
    return self;
}

-(void)decidePolicyForGeolocationRequestFromOrigin:(SecurityOrigin&)origin frame:(WebFrameProxy&)frame request:(GeolocationPermissionRequestProxy&)permissionRequest view:(WKWebView*)contentView
{
    // Step 1: ask the user if the app can use Geolocation.
    GeolocationRequestData geolocationRequestData;
    geolocationRequestData.origin = &origin;
    geolocationRequestData.frame = &frame;
    geolocationRequestData.permissionRequest = &permissionRequest;
    geolocationRequestData.view = contentView;
    _requestsWaitingForCoreLocationAuthorization.append(geolocationRequestData);
    [_coreLocationProvider requestGeolocationAuthorization];
}
@end

#pragma mark - WebGeolocationCoreLocationUpdateListener implementation.

@implementation WKGeolocationProviderIOS (WebGeolocationCoreLocationUpdateListener)

- (void)geolocationAuthorizationGranted
{
    // Step 2: ask the user if the this particular page can use gelocation.
    Vector<GeolocationRequestData> requests = WTF::move(_requestsWaitingForCoreLocationAuthorization);
    for (const auto& request : requests) {
        bool requiresUserAuthorization = true;

        id<WKUIDelegatePrivate> uiDelegate = static_cast<id <WKUIDelegatePrivate>>([request.view UIDelegate]);
        if ([uiDelegate respondsToSelector:@selector(_webView:shouldRequestGeolocationAuthorizationForURL:isMainFrame:mainFrameURL:)]) {
            const WebFrameProxy* mainFrame = request.frame->page()->mainFrame();
            bool isMainFrame = request.frame == mainFrame;
            URL requestFrameURL(URL(), request.frame->url());
            URL mainFrameURL(URL(), mainFrame->url());
            requiresUserAuthorization = [uiDelegate _webView:request.view.get()
                 shouldRequestGeolocationAuthorizationForURL:requestFrameURL
                                                 isMainFrame:isMainFrame
                                                mainFrameURL:mainFrameURL];
        }

        if (requiresUserAuthorization) {
            RetainPtr<WKWebAllowDenyPolicyListener> policyListener = adoptNS([[WKWebAllowDenyPolicyListener alloc] initWithPermissionRequestProxy:request.permissionRequest.get()]);
            decidePolicyForGeolocationRequestFromOrigin(request.origin.get(), request.frame->url(), policyListener.get(), [request.view window]);
        } else
            request.permissionRequest->allow();
    }
}

- (void)geolocationAuthorizationDenied
{
    Vector<GeolocationRequestData> requests = WTF::move(_requestsWaitingForCoreLocationAuthorization);
    for (const auto& requestData : requests)
        requestData.permissionRequest->deny();
}

- (void)positionChanged:(GeolocationPosition*)position
{
    _lastActivePosition = WebGeolocationPosition::create(position->timestamp(), position->latitude(), position->longitude(), position->accuracy(), position->canProvideAltitude(), position->altitude(), position->canProvideAltitudeAccuracy(), position->altitudeAccuracy(), position->canProvideHeading(), position->heading(), position->canProvideSpeed(), position->speed());
    _geolocationManager->providerDidChangePosition(_lastActivePosition.get());
}

- (void)errorOccurred:(NSString *)errorMessage
{
    _geolocationManager->providerDidFailToDeterminePosition(errorMessage);
}

- (void)resetGeolocation
{
    _geolocationManager->resetPermissions();
}

@end

# pragma mark - Implementation of WKWebAllowDenyPolicyListener
@implementation WKWebAllowDenyPolicyListener {
    RefPtr<GeolocationPermissionRequestProxy> _permissionRequestProxy;
}

- (id)initWithPermissionRequestProxy:(PassRefPtr<GeolocationPermissionRequestProxy>)permissionRequestProxy
{
    self = [super init];
    if (!self)
        return nil;

    _permissionRequestProxy = permissionRequestProxy;
    return self;
}

- (void)allow
{
    _permissionRequestProxy->allow();
}

- (void)deny
{
    _permissionRequestProxy->deny();
}

- (void)denyOnlyThisRequest
{
    // The method denyOnlyThisRequest is iAd specific for WebKit1.
    ASSERT_NOT_REACHED();
}

- (BOOL)shouldClearCache
{
    return NO;
}
@end

#pragma clang diagnostic pop

#endif // PLATFORM(IOS)
