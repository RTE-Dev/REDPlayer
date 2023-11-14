//
//  RedFlowCell.swift
//  RedPlayerDemo
//
//  Created by zijie on 2023/12/26.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit
import RedPlayer

let kRedFlowCellIdentifier = "RedFlowLayout.redFlowCell"

public class RedFlowCell: UICollectionViewCell {
    
    static let annotationPadding: CGFloat = 4

    private var _imageView: UIImageView?
    public var imageView: UIImageView {
        if let imageView = _imageView {
            return imageView
        }
        let imageView = UIImageView(frame: bounds)
        _imageView = imageView
        
        contentView.addSubview(imageView)
        self.addConstraintsForImageView()
        
        imageView.contentMode = .scaleAspectFill
        imageView.layer.cornerRadius = 10
        imageView.layer.masksToBounds = true
        imageView.layer.shouldRasterize = true
        imageView.layer.rasterizationScale = UIScreen.main.scale
        
        return imageView
    }

    private var _descriptionLabel: UILabel?
    public var descriptionLabel: UILabel {
        if let descriptionLabel = _descriptionLabel {
            return descriptionLabel
        }
        let descriptionLabel = UILabel()
        _descriptionLabel = descriptionLabel
        
        contentView.addSubview(descriptionLabel)
        
        descriptionLabel.numberOfLines = 0
        descriptionLabel.font = UIFont.systemFont(ofSize: 12)
        
        self.addConstraintsForLabel()
        
        return descriptionLabel
    }
    
    private var imageViewHeightLayoutConstraint: NSLayoutConstraint?
    private var imageHeight: CGFloat?
    
    public var player: RedPlayerController?

    public func startPlayer(url: String, isJson: Bool) {
        let renderType = RedPlayerSettings.sharedInstance().renderType
        player = RedPlayerController(contentURLString: "", with: renderType == .sampleBuffer ? .sampleBufferDisplayLayer : (renderType == .openGLES ? .openGL : .metal))
        if isJson {
            player?.setContentString(url)
        } else if let playUrl = URL(string: url) {
            player?.setContentURL(playUrl)
        }
        player?.setEnableHDR(RedPlayerSettings.sharedInstance().hdrSwitch)
        player?.setEnableVTB(!RedPlayerSettings.sharedInstance().softDecoder)
        player?.setVideoCacheDir(RedMediaUtil.cachePath)
        player?.view.frame = self.imageView.frame
        player?.scalingMode = .aspectFill
        player?.setMute(true)
        player?.setLoop(0)
        player?.shouldAutoplay = true
        player?.prepareToPlay()
        if let playerView = player?.view {
            playerView.backgroundColor = .black
            self.addSubview(playerView)
        }
    }
    
    public func stopPlayer() {
        player?.shutdown()
        player = nil
        RedMediaUtil.shared.reuseBox = nil
    }
    
    override public func apply(_ layoutAttributes: UICollectionViewLayoutAttributes) {
        super.apply(layoutAttributes)
        if let attributes = layoutAttributes as? RedFlowLayoutAttributes {
            imageHeight = attributes.imageHeight
            if let imageViewHeightLayoutConstraint = self.imageViewHeightLayoutConstraint {
                imageViewHeightLayoutConstraint.constant = attributes.imageHeight
            }
        }
    }
}


extension RedFlowCell {
    func addConstraintsForImageView() {
        imageView.translatesAutoresizingMaskIntoConstraints = false
        
        contentView.addConstraint(
            NSLayoutConstraint(
                item: imageView,
                attribute: .top,
                relatedBy: .equal,
                toItem: contentView,
                attribute: NSLayoutConstraint.Attribute.top,
                multiplier: 1,
                constant: 0
            )
        )
        contentView.addConstraint(
            NSLayoutConstraint(
                item: imageView,
                attribute: .leading,
                relatedBy: .equal,
                toItem: contentView,
                attribute: .leading,
                multiplier: 1,
                constant: 0
            )
        )
        contentView.addConstraint(
            NSLayoutConstraint(
                item: imageView,
                attribute: .trailing,
                relatedBy: .equal,
                toItem: contentView,
                attribute: .trailing,
                multiplier: 1,
                constant: 0
            )
        )
        if let imageHeight = imageHeight {
            let imageViewHeightLayoutConstraint =
                NSLayoutConstraint(
                    item: imageView,
                    attribute: .height,
                    relatedBy: .equal,
                    toItem: nil,
                    attribute: .notAnAttribute,
                    multiplier: 1,
                    constant: imageHeight
            )
            imageView.addConstraint(imageViewHeightLayoutConstraint)
            self.imageViewHeightLayoutConstraint = imageViewHeightLayoutConstraint
        }
    }
    
    func addConstraintsForLabel() {
        descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
        
        contentView.addConstraint(
            NSLayoutConstraint(
                item: descriptionLabel,
                attribute: .top,
                relatedBy: .equal,
                toItem: imageView,
                attribute: .bottom,
                multiplier: 1,
                constant: RedFlowCell.annotationPadding
            )
        )
        contentView.addConstraint(
            NSLayoutConstraint(
                item: descriptionLabel,
                attribute: .left,
                relatedBy: .equal,
                toItem: contentView,
                attribute: .left,
                multiplier: 1,
                constant: RedFlowCell.annotationPadding
            )
        )
        contentView.addConstraint(
            NSLayoutConstraint(
                item: descriptionLabel,
                attribute: .right,
                relatedBy: .equal,
                toItem: contentView,
                attribute: .right,
                multiplier: 1,
                constant: RedFlowCell.annotationPadding
            )
        )
        let bottomConstraint = NSLayoutConstraint(
            item: contentView,
            attribute: .bottom,
            relatedBy: .greaterThanOrEqual,
            toItem: descriptionLabel,
            attribute: .bottom,
            multiplier: 1,
            constant: RedFlowCell.annotationPadding
        )
        bottomConstraint.priority = UILayoutPriority(750)
        contentView.addConstraint(bottomConstraint)
    }
}
