//
//  RedTabBarController.swift
//  RedPlayerDemo
//
//  Created by zijie on 2023/12/19.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import Foundation
import Masonry

class RedNavigationController: UINavigationController {
    
    override func viewDidLoad() {
        super.viewDidLoad()
        self.view.backgroundColor = .systemBackground
    }
    override var shouldAutorotate: Bool {
        false
    }

    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        .portrait
    }
}

class RedTabBarController: UIViewController {
    private let tabs = ["Home", "Settings"]
    private let tabIcons = ["tab_home_icon", "setting_icon"]
    lazy private var pageViewController: UIPageViewController = {
        let pageViewController = UIPageViewController(transitionStyle: .scroll, navigationOrientation: .horizontal, options: nil)
        pageViewController.dataSource = self
        pageViewController.delegate = self
        return pageViewController
    }()
    
    lazy private var viewControllers: [UIViewController] = {
        let mainVC = MainViewController()
        let settingsVC = RedSettingsViewController()

        return [mainVC, settingsVC]
    }()
    
    lazy private var tabBar: RedTabBar = {
        let tabBar = RedTabBar()
        tabBar.tintColor = .red
        tabBar.delegate = self
        tabBar.translatesAutoresizingMaskIntoConstraints = true
        return tabBar
    }()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupNavigationBar()
        self.view.backgroundColor = .systemBackground
        setupPageViewController()
        setupTabBar()
    }
    
    private func setupNavigationBar() {
        let title = "RedPlayer"
        let textAttributes: [NSAttributedString.Key: Any] = [
            .font: UIFont.boldSystemFont(ofSize: 20)
        ]
        let attributedTitle = NSAttributedString(string: title, attributes: textAttributes)
        let textWidth = attributedTitle.size().width
        let gap = 8.0
        let titleView = UIView(frame: CGRect(x: 0, y: 0, width: textWidth + gap + 22.5, height: 30.0))

        let imageView = UIImageView(image: UIImage(named: "navigation_icon"))
        imageView.contentMode = .scaleAspectFit
        imageView.frame = CGRect(x: 0, y: 0, width: 22.5, height: 30.0)
        titleView.addSubview(imageView)

        let titleLabel = UILabel(frame: CGRect(x: 22.5 + gap, y: 2, width: textWidth, height: 30.0))
        titleLabel.attributedText = attributedTitle
        titleLabel.textAlignment = .left
        titleView.addSubview(titleLabel)

        navigationItem.titleView = titleView
    }
    private func setupPageViewController() {
        guard let vodVC = viewControllers.first else {
            return
        }
        for view in self.pageViewController.view.subviews {
            if let scrollView = view as? UIScrollView {
                scrollView.bounces = false
            }
        }
        pageViewController.setViewControllers([vodVC], direction: .forward, animated: true, completion: nil)
        self.addChild(pageViewController)
        self.view.addSubview(pageViewController.view)
        self.pageViewController.didMove(toParent: self)
    }
    
    private func setupTabBar() {

        self.view.addSubview(tabBar)
        var tabItems: [UITabBarItem] = []
        
        for (index, tab) in tabs.enumerated() {
            let item = UITabBarItem(title: tab, image: UIImage(named: tabIcons[index]), tag: index)
            tabItems.append(item)
        }
        tabBar.mas_makeConstraints { make in
            make?.left.mas_equalTo()(0)
            make?.right.mas_equalTo()(0)
            make?.bottom.mas_equalTo()(0)
            make?.height.mas_equalTo()(70)
        }
        tabBar.setItems(tabItems, animated: true)
        updateTabBarSelection(index: 0)
    }
    
    override func viewDidLayoutSubviews() {
        super.viewDidLayoutSubviews()
        for item in tabBar.items ?? [] {
            item.imageInsets = UIEdgeInsets(top: -15, left: 0, bottom: 0, right: 0)
            item.titlePositionAdjustment = UIOffset(horizontal: 0, vertical: -10)
        }
    }
    
}

// MARK: - UIPageViewControllerDataSource

extension RedTabBarController: UIPageViewControllerDataSource {
    func pageViewController(_ pageViewController: UIPageViewController, viewControllerBefore viewController: UIViewController) -> UIViewController? {
        guard let currentIndex = pageViewController.viewControllers?.first.flatMap({ viewControllers.firstIndex(of: $0) }), currentIndex > 0 else {
                return nil
            }
            let previousIndex = currentIndex - 1
            return viewControllers[previousIndex]
        }

        func pageViewController(_ pageViewController: UIPageViewController, viewControllerAfter viewController: UIViewController) -> UIViewController? {
            guard let currentIndex = pageViewController.viewControllers?.first.flatMap({ viewControllers.firstIndex(of: $0) }), currentIndex < viewControllers.count - 1 else {
                return nil
            }
            let nextIndex = currentIndex + 1
            return viewControllers[nextIndex]
        }
}

// MARK: - UIPageViewControllerDelegate

extension RedTabBarController: UIPageViewControllerDelegate {
    func pageViewController(_ pageViewController: UIPageViewController, willTransitionTo pendingViewControllers: [UIViewController]) {
        guard let vc = pendingViewControllers.first,
              let _ = viewControllers.firstIndex(of: vc) else {
            return
        }
    }
    
    func pageViewController(_ pageViewController: UIPageViewController, didFinishAnimating finished: Bool, previousViewControllers: [UIViewController], transitionCompleted completed: Bool) {
        if completed, let currentViewController = pageViewController.viewControllers?.first {
            if let currentIndex = viewControllers.firstIndex(of: currentViewController) {
                updateTabBarSelection(index: currentIndex)
            }
        }
    }
}

// MARK: - UITabBarDelegate

extension RedTabBarController: UITabBarDelegate {
    func tabBar(_ tabBar: UITabBar, didSelect item: UITabBarItem) {
        if let selectedIndex = tabBar.items?.firstIndex(of: item) {
            // Handle tab selection
            if let currentViewController = pageViewController.viewControllers?.first, let currentIndex = viewControllers.firstIndex(of: currentViewController) {
                let direction: UIPageViewController.NavigationDirection = selectedIndex > currentIndex ? .forward : .reverse
                let selectedViewController = viewControllers[selectedIndex]
                pageViewController.setViewControllers([selectedViewController], direction: direction, animated: true, completion: nil)
                updateTabBarSelection(index: selectedIndex)
            }
        }
    }
    
    private func updateTabBarSelection(index: Int) {
        // Update tab bar UI to reflect the selected tab
        tabBar.selectedItem = tabBar.items?[index]
    }
}

