//
//  RedExtendedButton.swift
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/22.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit

@objcMembers
class RedExtendedButton: UIButton {
    var extendedTouchArea: CGFloat

    init(extendedTouchArea: CGFloat) {
        self.extendedTouchArea = extendedTouchArea
        super.init(frame: .zero)
    }

    required init?(coder: NSCoder) {
        self.extendedTouchArea = 0
        super.init(coder: coder)
    }

    override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
        let area = self.bounds.insetBy(dx: -extendedTouchArea, dy: -extendedTouchArea)
        return area.contains(point)
    }
}
