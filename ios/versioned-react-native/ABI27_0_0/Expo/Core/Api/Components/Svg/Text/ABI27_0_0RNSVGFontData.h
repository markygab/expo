#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#import "ABI27_0_0RNSVGTextProperties.h"
#import "ABI27_0_0RNSVGPropHelper.h"

@interface ABI27_0_0RNSVGFontData : NSObject {
@public
    double fontSize;
    NSString * fontSize_;
    NSString *fontFamily;
    enum ABI27_0_0RNSVGFontStyle fontStyle;
    NSDictionary * fontData;
    enum ABI27_0_0RNSVGFontWeight fontWeight;
    NSString *fontFeatureSettings;
    enum ABI27_0_0RNSVGFontVariantLigatures fontVariantLigatures;
    enum ABI27_0_0RNSVGTextAnchor textAnchor;
    enum ABI27_0_0RNSVGTextDecoration textDecoration;
    double kerning;
    double wordSpacing;
    double letterSpacing;
    bool manualKerning;
}

+ (instancetype)Defaults;

+ (double)toAbsoluteWithNSString:(NSString *)string
                           scale:(double)scale
                        fontSize:(double)fontSize;

+ (instancetype)initWithNSDictionary:(NSDictionary *)font
                              parent:(ABI27_0_0RNSVGFontData *)parent
                               scale:(double)scale;

@end

#define ABI27_0_0RNSVGFontData_DEFAULT_FONT_SIZE 12.0
