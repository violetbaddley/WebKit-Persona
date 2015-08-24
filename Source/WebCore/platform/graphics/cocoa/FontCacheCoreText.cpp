/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
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

#include "config.h"
#include "FontCache.h"

#include "CoreTextSPI.h"
#include "Font.h"

#include <CoreText/SFNTLayoutTypes.h>

#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

static inline void appendRawTrueTypeFeature(CFMutableArrayRef features, int type, int selector)
{
    RetainPtr<CFNumberRef> typeNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &type));
    RetainPtr<CFNumberRef> selectorNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &selector));
    CFTypeRef featureKeys[] = { kCTFontFeatureTypeIdentifierKey, kCTFontFeatureSelectorIdentifierKey };
    CFTypeRef featureValues[] = { typeNumber.get(), selectorNumber.get() };
    RetainPtr<CFDictionaryRef> feature = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, featureKeys, featureValues, WTF_ARRAY_LENGTH(featureKeys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    CFArrayAppendValue(features, feature.get());
}

static inline void appendTrueTypeFeature(CFMutableArrayRef features, const FontFeature& feature)
{
    if (equalIgnoringASCIICase(feature.tag(), "liga") || equalIgnoringASCIICase(feature.tag(), "clig")) {
        if (feature.enabled()) {
            appendRawTrueTypeFeature(features, kLigaturesType, kCommonLigaturesOnSelector);
            appendRawTrueTypeFeature(features, kLigaturesType, kContextualLigaturesOnSelector);
        } else {
            appendRawTrueTypeFeature(features, kLigaturesType, kCommonLigaturesOffSelector);
            appendRawTrueTypeFeature(features, kLigaturesType, kContextualLigaturesOffSelector);
        }
    } else if (equalIgnoringASCIICase(feature.tag(), "dlig")) {
        if (feature.enabled())
            appendRawTrueTypeFeature(features, kLigaturesType, kRareLigaturesOnSelector);
        else
            appendRawTrueTypeFeature(features, kLigaturesType, kRareLigaturesOffSelector);
    } else if (equalIgnoringASCIICase(feature.tag(), "hlig")) {
        if (feature.enabled())
            appendRawTrueTypeFeature(features, kLigaturesType, kHistoricalLigaturesOnSelector);
        else
            appendRawTrueTypeFeature(features, kLigaturesType, kHistoricalLigaturesOffSelector);
    } else if (equalIgnoringASCIICase(feature.tag(), "calt")) {
        if (feature.enabled())
            appendRawTrueTypeFeature(features, kContextualAlternatesType, kContextualAlternatesOnSelector);
        else
            appendRawTrueTypeFeature(features, kContextualAlternatesType, kContextualAlternatesOffSelector);
    } else if (equalIgnoringASCIICase(feature.tag(), "subs") && feature.enabled())
        appendRawTrueTypeFeature(features, kVerticalPositionType, kInferiorsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "sups") && feature.enabled())
        appendRawTrueTypeFeature(features, kVerticalPositionType, kSuperiorsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "smcp") && feature.enabled())
        appendRawTrueTypeFeature(features, kLowerCaseType, kLowerCaseSmallCapsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "c2sc") && feature.enabled())
        appendRawTrueTypeFeature(features, kUpperCaseType, kUpperCaseSmallCapsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "pcap") && feature.enabled())
        appendRawTrueTypeFeature(features, kLowerCaseType, kLowerCasePetiteCapsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "c2pc") && feature.enabled())
        appendRawTrueTypeFeature(features, kUpperCaseType, kUpperCasePetiteCapsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "unic") && feature.enabled())
        appendRawTrueTypeFeature(features, kLetterCaseType, 14);
    else if (equalIgnoringASCIICase(feature.tag(), "titl") && feature.enabled())
        appendRawTrueTypeFeature(features, kStyleOptionsType, kTitlingCapsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "lnum") && feature.enabled())
        appendRawTrueTypeFeature(features, kNumberCaseType, kUpperCaseNumbersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "onum") && feature.enabled())
        appendRawTrueTypeFeature(features, kNumberCaseType, kLowerCaseNumbersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "pnum") && feature.enabled())
        appendRawTrueTypeFeature(features, kNumberSpacingType, kProportionalNumbersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "tnum") && feature.enabled())
        appendRawTrueTypeFeature(features, kNumberSpacingType, kMonospacedNumbersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "frac") && feature.enabled())
        appendRawTrueTypeFeature(features, kFractionsType, kDiagonalFractionsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "afrc") && feature.enabled())
        appendRawTrueTypeFeature(features, kFractionsType, kVerticalFractionsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "ordn") && feature.enabled())
        appendRawTrueTypeFeature(features, kVerticalPositionType, kOrdinalsSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "zero") && feature.enabled())
        appendRawTrueTypeFeature(features, kTypographicExtrasType, kSlashedZeroOnSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "hist") && feature.enabled())
        appendRawTrueTypeFeature(features, kLigaturesType, kHistoricalLigaturesOnSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "jp78") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kJIS1978CharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "jp83") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kJIS1983CharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "jp90") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kJIS1990CharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "jp04") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kJIS2004CharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "smpl") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kSimplifiedCharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "trad") && feature.enabled())
        appendRawTrueTypeFeature(features, kCharacterShapeType, kTraditionalCharactersSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "fwid") && feature.enabled())
        appendRawTrueTypeFeature(features, kTextSpacingType, kMonospacedTextSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "pwid") && feature.enabled())
        appendRawTrueTypeFeature(features, kTextSpacingType, kProportionalTextSelector);
    else if (equalIgnoringASCIICase(feature.tag(), "ruby") && feature.enabled())
        appendRawTrueTypeFeature(features, kRubyKanaType, kRubyKanaOnSelector);
}

static inline void appendOpenTypeFeature(CFMutableArrayRef features, const FontFeature& feature)
{
#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000)
    RetainPtr<CFStringRef> featureKey = feature.tag().string().createCFString();
    int rawFeatureValue = feature.value();
    RetainPtr<CFNumberRef> featureValue = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &rawFeatureValue));
    CFTypeRef featureDictionaryKeys[] = { kCTFontOpenTypeFeatureTag, kCTFontOpenTypeFeatureValue };
    CFTypeRef featureDictionaryValues[] = { featureKey.get(), featureValue.get() };
    RetainPtr<CFDictionaryRef> featureDictionary = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, featureDictionaryKeys, featureDictionaryValues, WTF_ARRAY_LENGTH(featureDictionaryValues), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    CFArrayAppendValue(features, featureDictionary.get());
#else
    UNUSED_PARAM(features);
    UNUSED_PARAM(feature);
#endif
}

RetainPtr<CTFontRef> preparePlatformFont(CTFontRef originalFont, TextRenderingMode textRenderingMode, const FontFeatureSettings* features)
{
    if (!originalFont || ((!features || !features->size()) && (textRenderingMode != OptimizeLegibility)))
        return originalFont;

    RetainPtr<CFMutableDictionaryRef> attributes = adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    if (features && features->size()) {
        RetainPtr<CFMutableArrayRef> featureArray = adoptCF(CFArrayCreateMutable(kCFAllocatorDefault, features->size(), &kCFTypeArrayCallBacks));
        for (size_t i = 0; i < features->size(); ++i) {
            appendTrueTypeFeature(featureArray.get(), features->at(i));
            appendOpenTypeFeature(featureArray.get(), features->at(i));
        }
        CFDictionaryAddValue(attributes.get(), kCTFontFeatureSettingsAttribute, featureArray.get());
    }
    if (textRenderingMode == OptimizeLegibility) {
        CGFloat size = CTFontGetSize(originalFont);
        RetainPtr<CFNumberRef> sizeNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberCGFloatType, &size));
        CFDictionaryAddValue(attributes.get(), kCTFontOpticalSizeAttribute, sizeNumber.get());
    }
    RetainPtr<CTFontDescriptorRef> descriptor = adoptCF(CTFontDescriptorCreateWithAttributes(attributes.get()));
    return adoptCF(CTFontCreateCopyWithAttributes(originalFont, CTFontGetSize(originalFont), nullptr, descriptor.get()));
}

FontWeight fontWeightFromCoreText(CGFloat weight)
{
    if (weight < -0.6)
        return FontWeight100;
    if (weight < -0.365)
        return FontWeight200;
    if (weight < -0.115)
        return FontWeight300;
    if (weight <  0.130)
        return FontWeight400;
    if (weight <  0.235)
        return FontWeight500;
    if (weight <  0.350)
        return FontWeight600;
    if (weight <  0.500)
        return FontWeight700;
    if (weight <  0.700)
        return FontWeight800;
    return FontWeight900;
}

static inline FontTraitsMask toTraitsMask(CTFontSymbolicTraits ctFontTraits, CGFloat weight)
{
    FontTraitsMask weightMask;
    switch (fontWeightFromCoreText(weight)) {
    case FontWeight100:
        weightMask = FontWeight100Mask;
        break;
    case FontWeight200:
        weightMask = FontWeight200Mask;
        break;
    case FontWeight300:
        weightMask = FontWeight300Mask;
        break;
    case FontWeight400:
        weightMask = FontWeight400Mask;
        break;
    case FontWeight500:
        weightMask = FontWeight500Mask;
        break;
    case FontWeight600:
        weightMask = FontWeight600Mask;
        break;
    case FontWeight700:
        weightMask = FontWeight700Mask;
        break;
    case FontWeight800:
        weightMask = FontWeight800Mask;
        break;
    case FontWeight900:
        weightMask = FontWeight900Mask;
        break;
    }
    return static_cast<FontTraitsMask>(((ctFontTraits & kCTFontTraitItalic) ? FontStyleItalicMask : FontStyleNormalMask)
        | FontVariantNormalMask | weightMask);
}

bool isFontWeightBold(FontWeight fontWeight)
{
    return fontWeight >= FontWeight600;
}

uint16_t toCoreTextFontWeight(FontWeight fontWeight)
{
    static const int coreTextFontWeights[] = {
        100, // FontWeight100
        200, // FontWeight200
        300, // FontWeight300
        400, // FontWeight400
        500, // FontWeight500
        600, // FontWeight600
        700, // FontWeight700
        800, // FontWeight800
        900, // FontWeight900
    };
    return coreTextFontWeights[fontWeight];
}

RefPtr<Font> FontCache::similarFont(const FontDescription& description)
{
    // Attempt to find an appropriate font using a match based on the presence of keywords in
    // the requested names. For example, we'll match any name that contains "Arabic" to Geeza Pro.
    RefPtr<Font> font;
    for (unsigned i = 0; i < description.familyCount(); ++i) {
        const AtomicString& family = description.familyAt(i);
        if (family.isEmpty())
            continue;

#if PLATFORM(IOS)
        // Substitute the default monospace font for well-known monospace fonts.
        static NeverDestroyed<AtomicString> monaco("monaco", AtomicString::ConstructFromLiteral);
        static NeverDestroyed<AtomicString> menlo("menlo", AtomicString::ConstructFromLiteral);
        static NeverDestroyed<AtomicString> courier("courier", AtomicString::ConstructFromLiteral);
        if (equalIgnoringCase(family, monaco) || equalIgnoringCase(family, menlo)) {
            font = fontForFamily(description, courier);
            continue;
        }

        // Substitute Verdana for Lucida Grande.
        static NeverDestroyed<AtomicString> lucidaGrande("lucida grande", AtomicString::ConstructFromLiteral);
        static NeverDestroyed<AtomicString> verdana("verdana", AtomicString::ConstructFromLiteral);
        if (equalIgnoringCase(family, lucidaGrande)) {
            font = fontForFamily(description, verdana);
            continue;
        }
#endif

        static NeverDestroyed<String> arabic(ASCIILiteral("Arabic"));
        static NeverDestroyed<String> pashto(ASCIILiteral("Pashto"));
        static NeverDestroyed<String> urdu(ASCIILiteral("Urdu"));
        static String* matchWords[3] = { &arabic.get(), &pashto.get(), &urdu.get() };
        static NeverDestroyed<AtomicString> geezaPlain("GeezaPro", AtomicString::ConstructFromLiteral);
        static NeverDestroyed<AtomicString> geezaBold("GeezaPro-Bold", AtomicString::ConstructFromLiteral);
        for (String* matchWord : matchWords) {
            if (family.contains(*matchWord, false))
                font = fontForFamily(description, isFontWeightBold(description.weight()) ? geezaBold : geezaPlain);
        }
    }

    return font.release();
}

Vector<FontTraitsMask> FontCache::getTraitsInFamily(const AtomicString& familyName)
{
    RetainPtr<CFStringRef> familyNameStr = familyName.string().createCFString();
    CFTypeRef keys[] = { kCTFontFamilyNameAttribute };
    CFTypeRef values[] = { familyNameStr.get() };
    RetainPtr<CFDictionaryRef> attributes = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    RetainPtr<CTFontDescriptorRef> fontDescriptor = adoptCF(CTFontDescriptorCreateWithAttributes(attributes.get()));
    RetainPtr<CFArrayRef> matchedDescriptors = adoptCF(CTFontDescriptorCreateMatchingFontDescriptors(fontDescriptor.get(), nullptr));
    if (!matchedDescriptors)
        return { };

    CFIndex numMatches = CFArrayGetCount(matchedDescriptors.get());
    if (!numMatches)
        return { };

    Vector<FontTraitsMask> traitsMasks;
    traitsMasks.reserveInitialCapacity(numMatches);
    for (CFIndex i = 0; i < numMatches; ++i) {
        RetainPtr<CFDictionaryRef> traits = adoptCF((CFDictionaryRef)CTFontDescriptorCopyAttribute((CTFontDescriptorRef)CFArrayGetValueAtIndex(matchedDescriptors.get(), i), kCTFontTraitsAttribute));
        CFNumberRef resultRef = (CFNumberRef)CFDictionaryGetValue(traits.get(), kCTFontSymbolicTrait);
        CFNumberRef weightRef = (CFNumberRef)CFDictionaryGetValue(traits.get(), kCTFontWeightTrait);
        if (resultRef && weightRef) {
            CTFontSymbolicTraits symbolicTraits;
            CFNumberGetValue(resultRef, kCFNumberIntType, &symbolicTraits);
            CGFloat weight = 0;
            CFNumberGetValue(weightRef, kCFNumberCGFloatType, &weight);
            traitsMasks.uncheckedAppend(toTraitsMask(symbolicTraits, weight));
        }
    }
    return traitsMasks;
}

static void invalidateFontCache()
{
    if (!isMainThread()) {
        callOnMainThread([] {
            invalidateFontCache();
        });
        return;
    }

    FontCache::singleton().invalidate();

    platformInvalidateFontCache();
}

static void fontCacheRegisteredFontsChangedNotificationCallback(CFNotificationCenterRef, void* observer, CFStringRef name, const void *, CFDictionaryRef)
{
    ASSERT_UNUSED(observer, observer == &FontCache::singleton());
    ASSERT_UNUSED(name, CFEqual(name, kCTFontManagerRegisteredFontsChangedNotification));

    invalidateFontCache();
}

void FontCache::platformInit()
{
    CFNotificationCenterAddObserver(CFNotificationCenterGetLocalCenter(), this, fontCacheRegisteredFontsChangedNotificationCallback, kCTFontManagerRegisteredFontsChangedNotification, 0, CFNotificationSuspensionBehaviorDeliverImmediately);
}

Vector<String> FontCache::systemFontFamilies()
{
    // FIXME: <rdar://problem/21890188>
    RetainPtr<CFDictionaryRef> attributes = adoptCF(CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    auto emptyFontDescriptor = adoptCF(CTFontDescriptorCreateWithAttributes(attributes.get()));
    auto matchedDescriptors = adoptCF(CTFontDescriptorCreateMatchingFontDescriptors(emptyFontDescriptor.get(), nullptr));
    if (!matchedDescriptors)
        return { };

    CFIndex numMatches = CFArrayGetCount(matchedDescriptors.get());
    if (!numMatches)
        return { };

    HashSet<String> visited;
    for (CFIndex i = 0; i < numMatches; ++i) {
        auto fontDescriptor = static_cast<CTFontDescriptorRef>(CFArrayGetValueAtIndex(matchedDescriptors.get(), i));
        if (auto familyName = adoptCF(static_cast<CFStringRef>(CTFontDescriptorCopyAttribute(fontDescriptor, kCTFontFamilyNameAttribute))))
            visited.add(familyName.get());
    }

    Vector<String> result;
    copyToVector(visited, result);
    return result;
}

static CTFontSymbolicTraits computeTraits(const FontDescription& fontDescription)
{
    CTFontSymbolicTraits traits = 0;
    if (fontDescription.italic())
        traits |= kCTFontTraitItalic;
    if (isFontWeightBold(fontDescription.weight()))
        traits |= kCTFontTraitBold;
    return traits;
}

SynthesisPair computeNecessarySynthesis(CTFontRef font, const FontDescription& fontDescription, bool isPlatformFont)
{
#if PLATFORM(IOS)
    if (CTFontIsAppleColorEmoji(font))
        return SynthesisPair(false, false);
#endif

    if (isPlatformFont)
        return SynthesisPair(false, false);

    CTFontSymbolicTraits desiredTraits = computeTraits(fontDescription);

    CTFontSymbolicTraits actualTraits = 0;
    if (isFontWeightBold(fontDescription.weight()) || fontDescription.italic())
        actualTraits = CTFontGetSymbolicTraits(font);

    bool needsSyntheticBold = (fontDescription.fontSynthesis() & FontSynthesisWeight) && (desiredTraits & kCTFontTraitBold) && !(actualTraits & kCTFontTraitBold);
    bool needsSyntheticOblique = (fontDescription.fontSynthesis() & FontSynthesisStyle) && (desiredTraits & kCTFontTraitItalic) && !(actualTraits & kCTFontTraitItalic);

    return SynthesisPair(needsSyntheticBold, needsSyntheticOblique);
}

typedef HashSet<String, CaseFoldingHash> Whitelist;
static Whitelist& fontWhitelist()
{
    static NeverDestroyed<Whitelist> whitelist;
    return whitelist;
}

void FontCache::setFontWhitelist(const Vector<String>& inputWhitelist)
{
    Whitelist& whitelist = fontWhitelist();
    whitelist.clear();
    for (auto& item : inputWhitelist)
        whitelist.add(item);
}

#if ENABLE(PLATFORM_FONT_LOOKUP)
static RetainPtr<CTFontRef> platformFontLookupWithFamily(const AtomicString& family, CTFontSymbolicTraits requestedTraits, FontWeight weight, const FontFeatureSettings* featureSettings, TextRenderingMode textRenderingMode, float size)
{
    const auto& whitelist = fontWhitelist();
    if (whitelist.size() && !whitelist.contains(family))
        return nullptr;

    auto foundFont = adoptCF(CTFontCreateForCSS(family.string().createCFString().get(), toCoreTextFontWeight(weight), requestedTraits, size));
    return preparePlatformFont(foundFont.get(), textRenderingMode, featureSettings);
}
#endif

static RetainPtr<CTFontRef> fontWithFamily(const AtomicString& family, CTFontSymbolicTraits desiredTraits, FontWeight weight, const FontFeatureSettings* featureSettings, TextRenderingMode textRenderingMode, float size)
{
    if (family.isEmpty())
        return nullptr;
    if (auto specialCase = platformFontWithFamilySpecialCase(family, weight, desiredTraits, size))
        return specialCase;
#if ENABLE(PLATFORM_FONT_LOOKUP)
    return platformFontLookupWithFamily(family, desiredTraits, weight, featureSettings, textRenderingMode, size);
#else
    return platformFontWithFamily(family, desiredTraits, weight, featureSettings, textRenderingMode, size);
#endif
}

#if PLATFORM(MAC)
static bool shouldAutoActivateFontIfNeeded(const AtomicString& family)
{
#ifndef NDEBUG
    // This cache is not thread safe so the following assertion is there to
    // make sure this function is always called from the same thread.
    static ThreadIdentifier initThreadId = currentThread();
    ASSERT(currentThread() == initThreadId);
#endif

    static NeverDestroyed<HashSet<AtomicString>> knownFamilies;
    static const unsigned maxCacheSize = 128;
    ASSERT(knownFamilies.get().size() <= maxCacheSize);
    if (knownFamilies.get().size() == maxCacheSize)
        knownFamilies.get().remove(knownFamilies.get().begin());

    // Only attempt to auto-activate fonts once for performance reasons.
    return knownFamilies.get().add(family).isNewEntry;
}
#endif

std::unique_ptr<FontPlatformData> FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    CTFontSymbolicTraits traits = computeTraits(fontDescription);
    float size = fontDescription.computedPixelSize();

    RetainPtr<CTFontRef> font = fontWithFamily(family, traits, fontDescription.weight(), fontDescription.featureSettings(), fontDescription.textRenderingMode(), size);

#if PLATFORM(MAC)
    if (!font) {
        if (!shouldAutoActivateFontIfNeeded(family))
            return nullptr;

        // Auto activate the font before looking for it a second time.
        // Ignore the result because we want to use our own algorithm to actually find the font.
        CFRelease(CTFontCreateWithName(family.string().createCFString().get(), size, nullptr));

        font = fontWithFamily(family, traits, fontDescription.weight(), fontDescription.featureSettings(), fontDescription.textRenderingMode(), size);
    }
#endif

    if (!font)
        return nullptr;

    bool syntheticBold, syntheticOblique;
    std::tie(syntheticBold, syntheticOblique) = computeNecessarySynthesis(font.get(), fontDescription).boldObliquePair();

    return std::make_unique<FontPlatformData>(font.get(), size, syntheticBold, syntheticOblique, fontDescription.orientation(), fontDescription.widthVariant(), fontDescription.textRenderingMode());
}

typedef HashSet<RetainPtr<CTFontRef>, WTF::RetainPtrObjectHash<CTFontRef>, WTF::RetainPtrObjectHashTraits<CTFontRef>> FallbackDedupSet;
static FallbackDedupSet& fallbackDedupSet()
{
    static NeverDestroyed<FallbackDedupSet> dedupSet;
    return dedupSet.get();
}

void FontCache::platformPurgeInactiveFontData()
{
    Vector<CTFontRef> toRemove;
    for (auto& font : fallbackDedupSet()) {
        if (CFGetRetainCount(font.get()) == 1)
            toRemove.append(font.get());
    }
    for (auto& font : toRemove)
        fallbackDedupSet().remove(font);
}

RefPtr<Font> FontCache::systemFallbackForCharacters(const FontDescription& description, const Font* originalFontData, bool isPlatformFont, const UChar* characters, unsigned length)
{
#if PLATFORM(IOS)
    if (length && requiresCustomFallbackFont(*characters)) {
        auto* fallback = getCustomFallbackFont(*characters, description);
        if (!fallback)
            return nullptr;
        return fontForPlatformData(*fallback);
    }
#endif

    const FontPlatformData& platformData = originalFontData->platformData();
    RetainPtr<CTFontRef> result = platformLookupFallbackFont(platformData.font(), description.weight(), description.locale(), characters, length);
    result = preparePlatformFont(result.get(), description.textRenderingMode(), description.featureSettings());
    if (!result)
        return lastResortFallbackFont(description);

    // FontCascade::drawGlyphBuffer() requires that there are no duplicate Font objects which refer to the same thing. This is enforced in
    // FontCache::fontForPlatformData(), where our equality check is based on hashing the FontPlatformData, whose hash includes the raw CoreText
    // font pointer.
    CTFontRef substituteFont = fallbackDedupSet().add(result).iterator->get();

    bool syntheticBold, syntheticOblique;
    std::tie(syntheticBold, syntheticOblique) = computeNecessarySynthesis(substituteFont, description, isPlatformFont).boldObliquePair();

    FontPlatformData alternateFont(substituteFont, platformData.size(), syntheticBold, syntheticOblique, platformData.m_orientation, platformData.m_widthVariant, platformData.m_textRenderingMode);

    return fontForPlatformData(alternateFont);
}

}
