//
//  RedTabBar.swift
//  RedPlayerDemo
//
//  Created by zijie on 2023/12/27.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import Foundation

public class RedTabBar: UITabBar {
    public override func layoutSubviews() {
        super.layoutSubviews()
        guard let count = self.items?.count else {
            return
        }
        let btnW = self.frame.size.width / CGFloat(count)
        let btnH = self.frame.size.height
        for (index, tabBarBtn) in self.subviews.enumerated() {
            if let clazz = NSClassFromString("UITabBarButton"),
               tabBarBtn.isKind(of: clazz) {
                tabBarBtn.frame = CGRect(x: btnW * CGFloat(index - 1), y: 0, width: btnW, height: btnH)
            }
        }
    }
}
