//
//  MainViewController.swift
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/19.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit
import PhotosUI
import MobileCoreServices

class ButtonTableViewCell: UITableViewCell {
    
    let iconImageView: UIImageView = {
        let imageView = UIImageView()
        imageView.contentMode = .scaleAspectFit
        imageView.tintColor = .label
        imageView.translatesAutoresizingMaskIntoConstraints = false
        return imageView
    }()
    
    let titleLabel: UILabel = {
        let label = UILabel()
        label.font = UIFont.systemFont(ofSize: 20)
        label.translatesAutoresizingMaskIntoConstraints = false
        return label
    }()
    
    override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
        super.init(style: style, reuseIdentifier: reuseIdentifier)
        
        contentView.addSubview(iconImageView)
        contentView.addSubview(titleLabel)
        
        NSLayoutConstraint.activate([
            iconImageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16),
            iconImageView.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            iconImageView.widthAnchor.constraint(equalToConstant: CGRectGetHeight(self.bounds)),
            iconImageView.heightAnchor.constraint(equalToConstant: CGRectGetHeight(self.bounds)),
            
            titleLabel.leadingAnchor.constraint(equalTo: iconImageView.trailingAnchor, constant: 16),
            titleLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
            titleLabel.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -16)
        ])
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
}

class MainViewController: UIViewController,
                          PHPickerViewControllerDelegate,
                          UIImagePickerControllerDelegate,
                          UIDocumentPickerDelegate,
                          UINavigationControllerDelegate {
    private var phVideoCopyIndex: Int = 0
    private var fileNames = [String]()
    private var movieUrls = [URL]()
    private var coverImages = [UIImage]()
    private var inputAlertView: RedAlertView?
    private var inputAlertViewBgView: UIView?
    private var tapGesture: UITapGestureRecognizer?
    let tableView = UITableView()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        setupUI()
    }
    
    func setupUI() {
        let tableView = UITableView(frame: view.bounds, style: .plain)
        tableView.translatesAutoresizingMaskIntoConstraints = false
        tableView.dataSource = self
        tableView.delegate = self
        tableView.separatorStyle = .none
        tableView.register(ButtonTableViewCell.self, forCellReuseIdentifier: "ButtonTableViewCell")
        
        view.addSubview(tableView)
        
    }

    func playURLResource() {
        inputAlertViewBgView = UIView(frame: self.view.bounds)
        if let bgView = inputAlertViewBgView {
            bgView.backgroundColor = UIColor.gray.withAlphaComponent(0.2)
            self.view.addSubview(bgView)
        }

        inputAlertView = RedAlertView(frame: CGRect(x: self.view.bounds.size.width / 2, y: self.view.bounds.size.height / 2, width: 0, height: 0))
        inputAlertView?.translatesAutoresizingMaskIntoConstraints = false
        if let alertView = inputAlertView {
            alertView.playButtonTapped = { [weak self] input, isJSON in
                let message = isJSON ? "JSON" : "URL"
                var isValid = true
                if isJSON {
                    isValid = RedMediaUtil.isValidJson(input)
                } else {
                    isValid = RedMediaUtil.isValidURL(input)
                }
                if !isValid {
                    let alertController = UIAlertController(title: "Warning", message: "Please make sure the input \(message) is correct.", preferredStyle: .alert)
                    let okAction = UIAlertAction(title: "OK", style: .default) { (action) in
                        alertController.dismiss(animated: true, completion: nil)
                    }
                    alertController.addAction(okAction)
                    self?.present(alertController, animated: true, completion: nil)
                    return
                }
                RedDemoPlayerViewController.present(from: self, withTitle: "", url: input, isJson: isJSON, playList: nil, playListIndex: 0, completion: nil, close: nil)
            }
            alertView.closeButtonTapped = { [weak self] in
                self?.closeAlertView()
            }
            self.view.addSubview(alertView)
            let alertWidth = CGRectGetWidth(self.view.bounds) - 20

            UIView.animate(withDuration: 0.3) {
                NSLayoutConstraint.activate([
                    alertView.topAnchor.constraint(equalTo: self.view.safeAreaLayoutGuide.topAnchor),
                    alertView.centerXAnchor.constraint(equalTo: self.view.centerXAnchor),
                    alertView.widthAnchor.constraint(equalToConstant: alertWidth),
                    alertView.heightAnchor.constraint(equalToConstant: 360)
                ])
                alertView.layoutIfNeeded();

            } completion: { _ in
                self.tapGesture = UITapGestureRecognizer(target: self, action: #selector(self.dismissAlertView(_:)))
                if let tap = self.tapGesture {
                    self.view.addGestureRecognizer(tap)
                }
            }
        }
    }

    @objc func dismissAlertView(_ gesture: UITapGestureRecognizer) {
        guard let alertView = inputAlertView else {
            return
        }
        
        if alertView.urlTextView.isFirstResponder {
            alertView.urlTextView.resignFirstResponder()
        } else if alertView.jsonTextView.isFirstResponder {
            alertView.jsonTextView.resignFirstResponder()
        } else {
            let location = gesture.location(in: self.view)
            if !alertView.frame.contains(location) {
                closeAlertView()
            }
        }
    }

    private func closeAlertView() {
        inputAlertView?.urlTextView.resignFirstResponder()
        inputAlertView?.jsonTextView.resignFirstResponder()
        inputAlertViewBgView?.removeFromSuperview()
        inputAlertViewBgView = nil
        inputAlertView?.removeFromSuperview()
        inputAlertView = nil
        if let tap = self.tapGesture {
            self.view.removeGestureRecognizer(tap)
            self.tapGesture = nil
        }
    }
    
    func playLocalVideo() {
        let alertController = UIAlertController(title: "Play Local Resources", message: nil, preferredStyle: .actionSheet)
        let albumAction = UIAlertAction(title: "From Album", style: .default) { (_) in
            self.openPhotoLibrary()
        }
        let fileAction = UIAlertAction(title: "From Documents", style: .default) { (_) in
            self.openDocumentLibrary()
        }
        let cancelAction = UIAlertAction(title: "Cancel", style: .cancel, handler: nil)
        alertController.addAction(albumAction)
        alertController.addAction(fileAction)
        alertController.addAction(cancelAction)
        self.present(alertController, animated: true, completion: nil)
    }
    
    private func openPhotoLibrary() {
        if #available(iOS 14, *) {
            var configuration = PHPickerConfiguration()
            configuration.selectionLimit = 0
            configuration.filter = .videos
            let picker = PHPickerViewController(configuration: configuration)
            picker.delegate = self
            self.present(picker, animated: true, completion: nil)
        } else {
            if UIImagePickerController.isSourceTypeAvailable(.photoLibrary) {
                let imagePicker = UIImagePickerController()
                imagePicker.delegate = self
                imagePicker.sourceType = .photoLibrary
                imagePicker.mediaTypes = [kUTTypeMovie as String]
                self.present(imagePicker, animated: true, completion: nil)
            }
        }
    }
    
    private func openDocumentLibrary() {
        let types = [kUTTypeMovie as String]
        let documentPicker = UIDocumentPickerViewController(documentTypes: types, in: .import)
        documentPicker.delegate = self
        documentPicker.allowsMultipleSelection = true
        documentPicker.modalPresentationStyle = .formSheet
        self.present(documentPicker, animated: true, completion: nil)
    }

    
    private func handlePickedVideo(url: URL, completion: @escaping (String, URL, UIImage) -> Void) {
        // Save the video to the local tmp sandbox directory, store the sandbox path
        let tmpDirectory = NSTemporaryDirectory().appending("/PHLibrary")
        if !FileManager.default.fileExists(atPath: tmpDirectory) {
            try? FileManager.default.createDirectory(atPath: tmpDirectory, withIntermediateDirectories: true, attributes: nil)
        }
        let filePath = tmpDirectory.appending(url.lastPathComponent)
        try? FileManager.default.copyItem(at: url, to: URL(fileURLWithPath: filePath))
        let newUrl = URL(fileURLWithPath: filePath)
        let fileName = url.lastPathComponent
        // Get the video cover image
        let asset = AVURLAsset(url: newUrl)
        let generator = AVAssetImageGenerator(asset: asset)
        let imageRef = try? generator.copyCGImage(at: .zero, actualTime: nil)
        let coverImage = imageRef.map { UIImage(cgImage: $0) } ?? RedMediaUtil.imageWithColor(hexColor: "FFD3B6", text: fileName, size: CGSize(width: 100, height: 100))
        completion(fileName, newUrl, coverImage)
    }

    
    private func checkIfOpenPlayer(totalNum: Int) {
        phVideoCopyIndex += 1
        if phVideoCopyIndex == totalNum {
            phVideoCopyIndex = 0
            if self.movieUrls.count > 0 {
                // Build dictionary array
                let firstUrl = self.movieUrls[0].absoluteString
                var videoDictArray = [[String: Any]]()
                for i in 0..<self.movieUrls.count {
                    let videoDict = ["fileName":self.fileNames[i],
                                     "videoUrl": self.movieUrls[i].absoluteString,
                                     "isJson": false,
                                     "coverImage": self.coverImages[i]] as [String : Any]
                    videoDictArray.append(videoDict)
                }
                self.fileNames.removeAll()
                self.movieUrls.removeAll()
                self.coverImages.removeAll()

                DispatchQueue.main.async {
                    self.dismiss(animated: true) {
                        RedDemoPlayerViewController.present(from: self, withTitle: "", url: firstUrl, isJson: false, playList: videoDictArray, playListIndex: 0, completion: nil, close: nil)
                    }
                }
            }
        }
    }

    func showRedFlowViewController() {
        self.navigationController?.pushViewController(RedFlowViewController(), animated: true)
    }
    
    // MARK: - PHPickerViewControllerDelegate
    @available(iOS 14, *)
    func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
        // Handle picked videos
        let itemCount = results.count
        for result in results {
            result.itemProvider.loadFileRepresentation(forTypeIdentifier: kUTTypeMovie as String) { url, error in
                if let url = url {
                    self.handlePickedVideo(url: url) { fileName, newUrl, coverImage in
                        self.fileNames.append(fileName)
                        self.movieUrls.append(newUrl)
                        self.coverImages.append(coverImage)
                        self.checkIfOpenPlayer(totalNum: itemCount)
                    }
                }
            }
        }
        picker.dismiss(animated: true, completion: nil)
    }

    // MARK: - UIImagePickerControllerDelegate
    func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey : Any]) {
        if let url = info[.mediaURL] as? URL {
            self.handlePickedVideo(url: url) { fileName, newUrl, coverImage in
                self.fileNames.append(fileName)
                self.movieUrls.append(newUrl)
                self.coverImages.append(coverImage)
                self.checkIfOpenPlayer(totalNum: 1)
            }
        }
        picker.dismiss(animated: true, completion: nil)
    }

    
    func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
        picker.dismiss(animated: true, completion: nil)
    }
    // MARK: - UIDocumentPickerDelegate
    func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
        let itemCount = urls.count
        if urls.count > 0 {
            for url in urls {
                self.handlePickedVideo(url: url) { fileName, newUrl, coverImage in
                    self.fileNames.append(fileName)
                    self.movieUrls.append(newUrl)
                    self.coverImages.append(coverImage)
                    self.checkIfOpenPlayer(totalNum: itemCount)
                }
            }
        }
    }
}

extension MainViewController: UITableViewDataSource {
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return 3
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: "ButtonTableViewCell", for: indexPath) as! ButtonTableViewCell
        
        switch indexPath.row {
        case 0:
            cell.iconImageView.image = UIImage(named: "link")?.withRenderingMode(.alwaysTemplate)
            cell.titleLabel.text = "Play URL / Json"
        case 1:
            cell.iconImageView.image = UIImage(named: "album")?.withRenderingMode(.alwaysTemplate)
            cell.titleLabel.text = "Play Local Resources"
        case 2:
            cell.iconImageView.image = UIImage(named: "flow_icon")?.withRenderingMode(.alwaysTemplate)
            cell.titleLabel.text = "Video Samples"
        default:
            break
        }
        
        return cell
    }
    
}

extension MainViewController: UITableViewDelegate {
    
    func tableView(_ tableView: UITableView, heightForRowAt indexPath: IndexPath) -> CGFloat {
        return 80
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        tableView.deselectRow(at: indexPath, animated: true)
        
        switch indexPath.row {
        case 0:
            playURLResource()
        case 1:
            playLocalVideo()
        case 2:
            showRedFlowViewController()
        default:
            break
        }
    }
}
