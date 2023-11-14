//
//  RedMediaUtil.swift
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/20.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import Foundation
@objcMembers
class ReuseBox: NSObject {
    weak var reusePlayer: RedPlayerController?
    var reuseUrl: String
    
    init(reusePlayer: RedPlayerController, url: String) {
        self.reusePlayer = reusePlayer
        self.reuseUrl = url
    }
}

@objcMembers
class RedMediaUtil: NSObject {
    static let shared = RedMediaUtil()
    var reuseBox: ReuseBox?
    
    static var cachePath: String {
        let cacheDir = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true).first!
        let cachePath = cacheDir.appending("/RedPlayer")
        return cachePath
    }
    
    static func imageWithColor(hexColor: String, text: String, size: CGSize) -> UIImage {
        let color = UIColor(hex: hexColor)
        let rect = CGRect(origin: .zero, size: size)
        let textAttributes: [NSAttributedString.Key: Any] = [
            .font: UIFont.systemFont(ofSize: 20),
            .foregroundColor: UIColor.white
        ]

        UIGraphicsBeginImageContext(size)

        let context = UIGraphicsGetCurrentContext()!
        context.setFillColor(color.cgColor)
        context.fill(rect)

        let textSize = text.size(withAttributes: textAttributes)
        let textRect = CGRect(x: (size.width - textSize.width) / 2, y: (size.height - textSize.height) / 2, width: textSize.width, height: textSize.height)
        text.draw(in: textRect, withAttributes: textAttributes)

        let image = UIGraphicsGetImageFromCurrentImageContext()!
        UIGraphicsEndImageContext()

        return image
    }
    
    static func encodeBase64(originalString: String) -> String? {
        let data = originalString.data(using: .utf8)
        let base64EncodedString = data?.base64EncodedString()
        return base64EncodedString
    }
    
    static func decodeBase64(base64EncodedString: String) -> String? {
        if let data = Data(base64Encoded: base64EncodedString) {
            let originalString = String(data: data, encoding: .utf8)
            return originalString
        }
        return nil
    }
    
    static func isValidJson(_ jsonString: String) -> Bool {
        let jsonData = jsonString.data(using: .utf8)
        do {
            let jsonObject = try JSONSerialization.jsonObject(with: jsonData!, options: [])
            if JSONSerialization.isValidJSONObject(jsonObject) {
                return true
            } else {
                return false
            }
        } catch {
            return false
        }
    }
    
    static func isValidURL(_ urlString: String) -> Bool {
        if let url = URL(string: urlString) {
            return url.scheme != nil && url.host != nil
        }
        return false
    }
}




extension UIColor {
    convenience init(hex: String) {
        let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        var int = UInt64()
        let scanner = Scanner(string: hex)
        scanner.scanHexInt64(&int)
        let a, r, g, b: UInt64
        switch hex.count {
        case 3: // RGB (12-bit)
            (a, r, g, b) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
        case 6: // RGB (24-bit)
            (a, r, g, b) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
        case 8: // ARGB (32-bit)
            (a, r, g, b) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
        default:
            (a, r, g, b) = (1, 1, 1, 0)
        }
        self.init(red: CGFloat(r) / 255, green: CGFloat(g) / 255, blue: CGFloat(b) / 255, alpha: CGFloat(a) / 255)
    }
}
