//
//  RedFlowLayout.swift
//  RedPlayerDemo
//
//  Created by zijie on 2023/12/26.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit

/**
 CollectionViewLayoutAttributes.
 */
public class RedFlowLayoutAttributes: UICollectionViewLayoutAttributes {
    /**
     Image height to be set to contstraint in collection view cell.
     */
    public var imageHeight: CGFloat = 0
    
    override public func copy(with zone: NSZone? = nil) -> Any {

        guard let copy = super.copy(with: zone) as? RedFlowLayoutAttributes else {
            return copy()
        }
        copy.imageHeight = imageHeight
        return copy
    }
    
    override public func isEqual(_ object: Any?) -> Bool {
        if let attributes = object as? RedFlowLayoutAttributes {
            if attributes.imageHeight == imageHeight {
                return super.isEqual(object)
            }
        }
        return false
    }
}

@objc public protocol RedFlowLayoutDelegate {
    /**
     Size for section header. Optional.
     
     @param collectionView - collectionView
     @param section - section for section header view
     
     Returns size for section header view.
     */
    @objc 
    optional func collectionView(
        collectionView: UICollectionView,
        sizeForSectionHeaderViewForSection section: Int
    ) -> CGSize
    /**
     Size for section footer. Optional.
     
     @param collectionView - collectionView
     @param section - section for section footer view
     
     Returns size for section footer view.
     */
    @objc 
    optional func collectionView(
        collectionView: UICollectionView,
        sizeForSectionFooterViewForSection section: Int
    ) -> CGSize
    /**
     Height for image view in cell.
     
     @param collectionView - collectionView
     @param indexPath - index path for cell
     
     Returns height of image view.
     */
    func collectionView(
        collectionView: UICollectionView,
        heightForImageAtIndexPath indexPath: IndexPath,
        withWidth: CGFloat
    ) -> CGFloat
    
    /**
     Height for annotation view (label) in cell.
     
     @param collectionView - collectionView
     @param indexPath - index path for cell
     
     Returns height of annotation view.
     */
    func collectionView(
        collectionView: UICollectionView,
        heightForAnnotationAtIndexPath indexPath: IndexPath,
        withWidth: CGFloat
    ) -> CGFloat
}

/**
 RedFlowLayout.
 */
public class RedFlowLayout: UICollectionViewLayout {
    
    /**
     Delegate.
     */
    public weak var delegate: RedFlowLayoutDelegate?
    /**
     Number of columns.
     */
    public var numberOfColumns: Int = 1
    /**
     Cell padding.
     */
    public var cellPadding: CGFloat = 0
    
    
    private var cache = [RedFlowLayoutAttributes]()
    private var contentHeight: CGFloat = 0
    private var contentWidth: CGFloat {
        let bounds = collectionView.bounds
        let insets = collectionView.contentInset
        return bounds.width - insets.left - insets.right
    }
    
    override public var collectionViewContentSize: CGSize {
        CGSize(
            width: contentWidth,
            height: contentHeight
        )
    }
    
    override public class var layoutAttributesClass: AnyClass {
        RedFlowLayoutAttributes.self
    }
    
    override public var collectionView: UICollectionView {
        guard let collectionView = super.collectionView else {
            fatalError("collectionView is not available")
        }
        return collectionView
    }
    
    private var numberOfSections: Int {
        collectionView.numberOfSections
    }
    
    private func numberOfItems(inSection section: Int) -> Int {
        collectionView.numberOfItems(inSection: section)
    }
    
    /**
     Invalidates layout.
     */
    override public func invalidateLayout() {
        cache.removeAll()
        contentHeight = 0
        
        super.invalidateLayout()
    }
    
    override public func prepare() {
        if cache.isEmpty {
            let collumnWidth = contentWidth / CGFloat(numberOfColumns)
            let cellWidth = collumnWidth - (cellPadding * 2)
            
            var xOffsets = [CGFloat]()
            
            for collumn in 0..<numberOfColumns {
                xOffsets.append(CGFloat(collumn) * collumnWidth)
            }
            
            for section in 0..<numberOfSections {
                let numberOfItems = self.numberOfItems(inSection: section)
                
                if let headerSize = delegate?.collectionView?(
                    collectionView: collectionView,
                    sizeForSectionHeaderViewForSection: section
                    ) {
                    let headerX = (contentWidth - headerSize.width) / 2
                    let headerFrame = CGRect(
                        origin: CGPoint(
                            x: headerX,
                            y: contentHeight
                        ),
                        size: headerSize
                    )
                    let headerAttributes = RedFlowLayoutAttributes(
                        forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
                        with: IndexPath(item: 0, section: section)
                    )
                    headerAttributes.frame = headerFrame
                    cache.append(headerAttributes)
                    
                    contentHeight = headerFrame.maxY
                }
                
                var yOffsets = [CGFloat](
                    repeating: contentHeight,
                    count: numberOfColumns
                )
                
                for item in 0..<numberOfItems {
                    let indexPath = IndexPath(item: item, section: section)
                    
                    let column = yOffsets.firstIndex(of: yOffsets.min() ?? 0) ?? 0
                    
                    let imageHeight = delegate?.collectionView(
                        collectionView: collectionView,
                        heightForImageAtIndexPath: indexPath,
                        withWidth: cellWidth
                    ) ?? 0
                    let annotationHeight = delegate?.collectionView(
                        collectionView: collectionView,
                        heightForAnnotationAtIndexPath: indexPath,
                        withWidth: cellWidth
                    ) ?? 0
                    let cellHeight = cellPadding + imageHeight + annotationHeight + cellPadding
                    
                    let frame = CGRect(
                        x: xOffsets[column],
                        y: yOffsets[column],
                        width: collumnWidth,
                        height: cellHeight
                    )
                    
                    let insetFrame = frame.insetBy(dx: cellPadding, dy: cellPadding)
                    let attributes = RedFlowLayoutAttributes(
                        forCellWith: indexPath
                    )
                    attributes.frame = insetFrame
                    attributes.imageHeight = imageHeight
                    cache.append(attributes)
                    
                    contentHeight = max(contentHeight, frame.maxY)
                    yOffsets[column] = yOffsets[column] + cellHeight
                }
                
                if let footerSize = delegate?.collectionView?(
                    collectionView: collectionView,
                    sizeForSectionFooterViewForSection: section
                    ) {
                    let footerX = (contentWidth - footerSize.width) / 2
                    let footerFrame = CGRect(
                        origin: CGPoint(
                            x: footerX,
                            y: contentHeight
                        ),
                        size: footerSize
                    )
                    let footerAttributes = RedFlowLayoutAttributes(
                        forSupplementaryViewOfKind: UICollectionView.elementKindSectionFooter,
                        with: IndexPath(item: 0, section: section)
                    )
                    footerAttributes.frame = footerFrame
                    cache.append(footerAttributes)
                    
                    contentHeight = footerFrame.maxY
                }
            }
        }
    }
    
    override public func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
        
        var layoutAttributes = [UICollectionViewLayoutAttributes]()
        
        for attributes in cache {
            if attributes.frame.intersects(rect) {
                layoutAttributes.append(attributes)
            }
        }
        
        return layoutAttributes
    }
}
