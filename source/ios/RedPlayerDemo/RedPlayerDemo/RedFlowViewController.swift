//
//  RedFlowViewController.swift
//  RedPlayerDemo
//
//  Created by zijie on 2023/12/19.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit
import AVKit
import Masonry

public struct RedFlowItem {
    public var width: Int
    public var height: Int
    public var title: String = ""
    public var color: String = ""
    public var videoUrl: String?
    public var coverName: String?
    public var isJson: Bool = false
    public var isLocal: Bool = false
    public var coverImage: UIImage?
    public init(width: Int, height: Int, title: String, color: String, videoUrl: String, isJson: Bool, isLocal: Bool, coverName: String) {
        self.width = width
        self.height = height
        self.title = title
        self.color = color
        self.isJson = isJson
        if isLocal {
            self.videoUrl = videoUrl
        } else {
            if isJson {
                if RedMediaUtil.isValidJson(videoUrl) {
                    self.videoUrl = videoUrl
                } else {
                    self.videoUrl = RedMediaUtil.decodeBase64(base64EncodedString: videoUrl)
                }
            } else {
                if RedMediaUtil.isValidURL(videoUrl) {
                    self.videoUrl = videoUrl
                } else {
                    self.videoUrl = RedMediaUtil.decodeBase64(base64EncodedString: videoUrl)
                }
            }
        }
        self.isLocal = isLocal
        if let cover = UIImage(named: coverName) {
            self.coverImage = cover
        } else {
            self.coverImage = RedMediaUtil.imageWithColor(hexColor: color, text: title, size: CGSize(width: width, height: height))
        }
    }
}

/**
 RedFlowVC.
 */
@objc
@objcMembers
open class RedFlowViewController: UIViewController, RedFlowLayoutDelegate, UICollectionViewDataSource, UICollectionViewDelegate {
    private let sceneName = "flow_view"
    private var isDataAdded = false
    private var dataAddTimes = 0
    private var isDecelerating = false
    private var playingCell: RedFlowCell?
    private var videoCoverMap: [String: AnyObject] = [:]
    private lazy var videoMap: [[String: AnyObject]] = {
        if let filePath = Bundle.main.path(forResource: "video_source", ofType: "plist"),
            let videoMap = NSArray(contentsOf: URL(fileURLWithPath: filePath)) as? [[String: AnyObject]] {
            return videoMap
        }
        return []
    }()
    
    private var _items: [RedFlowItem]?
    open var items: [RedFlowItem] {
        get {
            _items ?? [RedFlowItem]()
        }
        set {
            _items = newValue
        }
    }
    
    public lazy var collectionView: UICollectionView = {
        let layout = RedFlowLayout()
        layout.delegate = self
        layout.cellPadding = 5
        layout.numberOfColumns = 2
        let collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
        collectionView.dataSource = self
        collectionView.delegate = self
        collectionView.register(
            RedFlowCell.self,
            forCellWithReuseIdentifier: kRedFlowCellIdentifier
        )
        return collectionView
    }()
    
    override open func viewDidLoad() {
        super.viewDidLoad()
        self.title = "Video Samples"
        self.view.addSubview(collectionView)
        collectionView.mas_makeConstraints { make in
            make?.top.equalTo()(self.view.mas_safeAreaLayoutGuideTop)
            make?.left.right()?.bottom()?.equalTo()(self.view)
        }
        collectionView.contentInset = UIEdgeInsets(
            top: 0,
            left: 5,
            bottom: 0,
            right: 5
        )
        appendVideoData()
        self.collectionView.reloadData()
    }
    
    
    private func appendVideoData() {
        // online videos
        for videoData in videoMap {
            if var width = videoData["width"] as? Int,
               var height = videoData["height"] as? Int,
               let title = videoData["title"] as? String,
               let color = videoData["color"] as? String,
               let videoUrl = videoData["videoUrl"] as? String,
               let isJson = videoData["isJson"] as? Bool,
               let coverName = videoData["coverName"] as? String {
                width = Int.random(in: 20...30) * 10
                height = Int.random(in: 20...40) * 10
                let redFlowItem = RedFlowItem(width: width, height: height, title: title, color: color, videoUrl: videoUrl, isJson: isJson, isLocal: false, coverName: coverName)
                self.items.append(redFlowItem)
            }
        }
        // local bundle videos
        let bundleVideoFolder = Bundle.main.bundlePath.appending("/TestVideos")
        let fileManager = FileManager.default
        if let files = try? fileManager.contentsOfDirectory(atPath: bundleVideoFolder).sorted() {
            for fileName in files {
                let width = Int.random(in: 200...300)
                let height = Int.random(in: 200...400)
                let title = "\(fileName)[LOCAL]"
                let color = "10BAA5"
                let videoUrl = bundleVideoFolder.appending("/\(fileName)")
                let isJson = false
                let redFlowItem = RedFlowItem(width: width, height: height, title: title, color: color, videoUrl: videoUrl, isJson: isJson, isLocal: true, coverName: "")
                self.items.append(redFlowItem)
            }
        }
    }
    deinit {
        stopPlayingCell()
        closePreloadVideos()
    }
    
    private func delayPlayingFirstCell() {
        do {
            try AVAudioSession.sharedInstance().setCategory(.playback)
            try AVAudioSession.sharedInstance().setActive(true)
        } catch {
            debugPrint(error)
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            self.scrollViewEndDecelerating()
        }
    }
    
    public func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
        items.count
    }
    
    public func collectionView(collectionView: UICollectionView, heightForImageAtIndexPath indexPath: IndexPath, withWidth: CGFloat) -> CGFloat {
        let boundingRect = CGRect(
                    x: 0,
                    y: 0,
                    width: withWidth,
                    height: CGFloat(MAXFLOAT)
        )
        let rect = AVMakeRect(
            aspectRatio: CGSize(width: items[indexPath.item].width, height: items[indexPath.item].height),
            insideRect: boundingRect
        )
        return rect.size.height
    }
    
    public func collectionView(collectionView: UICollectionView, heightForAnnotationAtIndexPath indexPath: IndexPath, withWidth: CGFloat) -> CGFloat {
        let text = items[indexPath.item].title
        let font = UIFont.systemFont(ofSize: UIFont.systemFontSize)
        let rect = text.boundingRect(
            with: CGSize(width: withWidth, height: CGFloat(MAXFLOAT)),
            options: .usesLineFragmentOrigin,
            attributes: [NSAttributedString.Key.font: font],
            context: nil
        )
        let textHeight = ceil(rect.height)
        
        return RedFlowCell.annotationPadding * 2 + textHeight
    }
    
    public func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
        guard let cell = collectionView.dequeueReusableCell(withReuseIdentifier: kRedFlowCellIdentifier, for: indexPath) as? RedFlowCell else {
            return RedFlowCell()
        }
        var item = items[indexPath.item]
        cell.imageView.image = item.coverImage
        if item.isLocal {
            DispatchQueue.global().async {
                let url = NSURL(fileURLWithPath: item.videoUrl ?? "") as URL
                let asset = AVURLAsset(url: url)
                let generator = AVAssetImageGenerator(asset: asset)
                do {
                    let imageRef = try generator.copyCGImage(at: CMTimeMake(value: 0, timescale: 1), actualTime: nil)
                    let vImage = UIImage(cgImage: imageRef)
                    item.coverImage = vImage
                    self.items[indexPath.item] = item
                    DispatchQueue.main.async {
                        cell.imageView.image = vImage
                    }
                } catch {
                    cell.imageView.image = item.coverImage
                    print("Error generating image: \(error)")
                }
            }
        }
        cell.descriptionLabel.text = item.title
        return cell
    }
    
    public func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
        let item = items[indexPath.item]
        let playUrl = item.videoUrl
        if let cell = self.collectionView.cellForItem(at: indexPath) as? RedFlowCell,
           cell === playingCell,
           let player = cell.player,
           let playUrl = playUrl {
            let reuseBox = ReuseBox(reusePlayer: player, url: playUrl)
            RedMediaUtil.shared.reuseBox = reuseBox
        } else {
            stopPlayingCell()
        }
        var videoDictArray = [[String: Any]]()
        for item in self.items {
            let videoDict = ["fileName":item.title, 
                             "videoUrl": item.videoUrl ?? "",
                             "isJson": item.isJson,
                             "coverImage": item.coverImage ?? UIImage()] as [String : Any]
            videoDictArray.append(videoDict)
        }

        RedDemoPlayerViewController.present(from: self, withTitle: "", url: playUrl, isJson: item.isJson, isLive: false, playList: videoDictArray, playListIndex: indexPath.item, completion: nil) {
            self.playingCell = nil
            self.scrollViewEndDecelerating()
        }
    }

    public func scrollViewDidScroll(_ scrollView: UIScrollView) {
        let visibleCells = self.collectionView.visibleCells
        if let playingCell = playingCell,
           !visibleCells.contains(playingCell) {
            stopPlayingCell()
        }
        let offsetY = scrollView.contentOffset.y
        let contentHeight = scrollView.contentSize.height
        if contentHeight > 0 && offsetY > contentHeight - scrollView.frame.size.height && !isDataAdded && dataAddTimes < 5 {
            appendVideoData()
            stopPlayingCell()
            self.collectionView.reloadData()
            isDataAdded = true
            dataAddTimes = dataAddTimes + 1
        }
    }
    
    public func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
        self.scrollViewEndDecelerating()
    }
    
    public func scrollViewWillBeginDecelerating(_ scrollView: UIScrollView) {
        isDecelerating = true
    }

    public func scrollViewDidEndDragging(_ scrollView: UIScrollView, willDecelerate decelerate: Bool) {
        guard isDecelerating == false else {
            return
        }
        if decelerate == false {
            self.scrollViewEndDecelerating()
        }
    }
    
    private func scrollViewEndDecelerating() {
        isDecelerating = false
        let visibleIndexPaths = self.collectionView.indexPathsForVisibleItems
        let sortedIndexPaths = visibleIndexPaths.sorted { $0.row < $1.row }
        let visibleRect = CGRect(origin: self.collectionView.contentOffset, size: self.collectionView.bounds.size)
        
        for indexPath in sortedIndexPaths {
            guard let cell = self.collectionView.cellForItem(at: indexPath) as? RedFlowCell else { continue }
            let item = items[indexPath.item]
            if let playUrl = item.videoUrl,
               visibleRect.contains(cell.frame) {
                if cell === playingCell {
                    return
                } else if playingCell != nil {
                    stopPlayingCell()
                }
                cell.startPlayer(url: playUrl, isJson: item.isJson)
                playingCell = cell
                UIView.animate(withDuration: 0.3, animations: {
                    cell.transform = CGAffineTransform(scaleX: 0.95, y: 0.95)
                }) { _ in
                    UIView.animate(withDuration: 0.3) {
                        cell.transform = CGAffineTransform.identity
                    }
                }
                break
            }
        }
        isDataAdded = false
    }
    
    private func stopPlayingCell() {
        if playingCell?.player != RedMediaUtil.shared.reuseBox?.reusePlayer {
            playingCell?.stopPlayer()
            playingCell = nil
        }
    }
    
    override public func viewWillLayoutSubviews() {
        super.viewWillLayoutSubviews()
    }
    
    open override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        if RedPlayerSettings.sharedInstance().autoPreload { // 预载
            preloadVideos()
        }
        delayPlayingFirstCell()
    }
    override open func viewDidDisappear(_ animated: Bool) {
        super.viewDidDisappear(animated)
        stopPlayingCell()
    }
    
    // MARK: Preload
    private func preloadVideos() {
        videoMap.forEach { videoInfo in
            if let videoUrl = videoInfo["videoUrl"] as? String,
               let isJson = videoInfo["isJson"] as? Bool {
                if isJson {
                    RedPreloadManager.preloadVideoJson(videoUrl, forScene: sceneName)
                } else {
                    if let videoURL = URL(string: videoUrl) {
                        RedPreloadManager.preloadVideoURL(videoURL, forScene: sceneName)
                    }
                }
            }
        }
    }
    
    private func closePreloadVideos() {
        RedPreloadManager.closePreload(forScene: sceneName)
    }
}
