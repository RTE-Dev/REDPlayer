//
//  RedAlertController.swift
//  RedPlayerDemo
//
//  Created by zijie on 2024/1/19.
//  Copyright © 2024 Xiaohongshu. All rights reserved.
//

import UIKit
class RedAlertView: UIView {
    let urlVodDefault = "aHR0cDovL3Nucy12aWRlby1hbC54aHNjZG4uY29tL3NwZWN0cnVtLzAxZTI5NjRlNDE2YmUwNzcwMTgzNzAwMzgxMWI0ZTM5YzlfNTEzLm1wNA=="
    let urlLiveDefault = "aHR0cDovL2xpdmUtcGxheS54aHNjZG4uY29tL2xpdmUvNTY5MDcxNDQ3Mjc2ODQwMjY4LmZsdg=="
    let jsonDefault = "ewogICAgInN0cmVhbSI6IHsKICAgICAgICAiYXYxIjogW10sCiAgICAgICAgImgyNjUiOiBbCiAgICAgICAgICAgIHsKICAgICAgICAgICAgICAgICJtYXN0ZXJfdXJsIjogImh0dHA6Ly9zbnMtdmlkZW8tYmQueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzgvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM4YWQ2NTM3OV84Lm1wNCIsCiAgICAgICAgICAgICAgICAid2VpZ2h0IjogNjIsCiAgICAgICAgICAgICAgICAiYmFja3VwX3VybHMiOiBbCiAgICAgICAgICAgICAgICAgICAgImh0dHA6Ly9zbnMtdmlkZW8tcWMueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzgvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM4YWQ2NTM3OV84Lm1wND9zaWduPTgxMzQyZGQwZGM1YjViYmViY2I0N2NmYTVmNzQ1YmY1JnQ9NjViNTI3NTQiLAogICAgICAgICAgICAgICAgICAgICJodHRwOi8vc25zLXZpZGVvLWh3Lnhoc2Nkbi5jb20vc3RyZWFtLzExMC84LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOGFkNjUzNzlfOC5tcDQiLAogICAgICAgICAgICAgICAgICAgICJodHRwOi8vc25zLXZpZGVvLWFsLnhoc2Nkbi5jb20vc3RyZWFtLzExMC84LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOGFkNjUzNzlfOC5tcDQiLAogICAgICAgICAgICAgICAgICAgICJodHRwOi8vc25zLXZpZGVvLWh3Lnhoc2Nkbi5uZXQvc3RyZWFtLzExMC84LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOGFkNjUzNzlfOC5tcDQiCiAgICAgICAgICAgICAgICBdLAogICAgICAgICAgICAgICAgImhlaWdodCI6IDk2MCwKICAgICAgICAgICAgICAgICJ3aWR0aCI6IDcyMCwKICAgICAgICAgICAgICAgICJhdmdfYml0cmF0ZSI6IDM4MDYzNAogICAgICAgICAgICB9CiAgICAgICAgXSwKICAgICAgICAiaDI2NCI6IFsKICAgICAgICAgICAgewogICAgICAgICAgICAgICAgIm1hc3Rlcl91cmwiOiAiaHR0cDovL3Nucy12aWRlby1iZC54aHNjZG4uY29tL3N0cmVhbS8xMTAvMjU4LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOTVmODg0MmJfMjU4Lm1wNCIsCiAgICAgICAgICAgICAgICAid2VpZ2h0IjogNjIsCiAgICAgICAgICAgICAgICAiYmFja3VwX3VybHMiOiBbCiAgICAgICAgICAgICAgICAgICAgImh0dHA6Ly9zbnMtdmlkZW8tcWMueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzI1OC8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4Yzk1Zjg4NDJiXzI1OC5tcDQ/c2lnbj0yOGUxMzk3OWE2MGJhMjRiODk4ZGE0NzEyM2M5NmRkMiZ0PTY1YjUyNzU0IiwKICAgICAgICAgICAgICAgICAgICAiaHR0cDovL3Nucy12aWRlby1ody54aHNjZG4uY29tL3N0cmVhbS8xMTAvMjU4LzAxZTU4M2NiNmUwZmVkNWEwMTAzNzAwMzhjOTVmODg0MmJfMjU4Lm1wNCIsCiAgICAgICAgICAgICAgICAgICAgImh0dHA6Ly9zbnMtdmlkZW8tYWwueGhzY2RuLmNvbS9zdHJlYW0vMTEwLzI1OC8wMWU1ODNjYjZlMGZlZDVhMDEwMzcwMDM4Yzk1Zjg4NDJiXzI1OC5tcDQiLAogICAgICAgICAgICAgICAgICAgICJodHRwOi8vc25zLXZpZGVvLWh3Lnhoc2Nkbi5uZXQvc3RyZWFtLzExMC8yNTgvMDFlNTgzY2I2ZTBmZWQ1YTAxMDM3MDAzOGM5NWY4ODQyYl8yNTgubXA0IgogICAgICAgICAgICAgICAgXSwKICAgICAgICAgICAgICAgICJoZWlnaHQiOiA5NjAsCiAgICAgICAgICAgICAgICAid2lkdGgiOiA3MjAsCiAgICAgICAgICAgICAgICAiYXZnX2JpdHJhdGUiOiA1ODA4MTQKICAgICAgICAgICAgfQogICAgICAgIF0KICAgIH0KfQ=="
    var playButtonTapped: ((String, Bool) -> Void)?
    var closeButtonTapped: (() -> Void)?
    var isLive = false
    let titleLabel: UILabel = {
        let label = UILabel()
        label.font = .boldSystemFont(ofSize: 18)
        label.textAlignment = .center
        return label
    }()
    
    let urlTextView: UITextView = {
        let textView = UITextView()
        textView.text = "Enter URL"
        textView.textColor = .lightGray
        textView.layer.borderWidth = 1
        textView.layer.borderColor = UIColor.gray.cgColor
        textView.layer.cornerRadius = 5

        return textView
    }()
    
    let jsonTextView: UITextView = {
        let textView = UITextView()
        textView.text = "Enter JSON"
        textView.textColor = .lightGray
        textView.layer.borderWidth = 1
        textView.layer.borderColor = UIColor.gray.cgColor
        textView.layer.cornerRadius = 5
        return textView
    }()
    
    let urlButton: UIButton = {
        let button = createButton(title: "URL Demo", titleColor: .white, font: .boldSystemFont(ofSize: 14), backgroundColor: .lightGray, shadowColor: .black, shadowOffset: CGSize(width: 0, height: 2), shadowOpacity: 0.5, shadowRadius: 4)

        return button
    }()
    
    let jsonButton: UIButton = {
        let button = createButton(title: "JSON Demo", titleColor: .white, font: .boldSystemFont(ofSize: 14), backgroundColor: .lightGray, shadowColor: .black, shadowOffset: CGSize(width: 0, height: 2), shadowOpacity: 0.5, shadowRadius: 4)
        return button
    }()
    
    let clearButton: UIButton = {
        let button = createButton(title: "Clear", titleColor: .white, font: .boldSystemFont(ofSize: 20), backgroundColor: .gray, shadowColor: .black, shadowOffset: CGSize(width: 0, height: 2), shadowOpacity: 0.5, shadowRadius: 4)
        return button
    }()
    
    let closeButton: UIButton = {
        let button = createButton(title: "Close", titleColor: .white, font: .boldSystemFont(ofSize: 20), backgroundColor: .gray, shadowColor: .black, shadowOffset: CGSize(width: 0, height: 2), shadowOpacity: 0.5, shadowRadius: 4)
        return button
    }()
    
    let playButton: UIButton = {
        let button = createButton(title: "Play", titleColor: .white, font: .boldSystemFont(ofSize: 20), backgroundColor: .red, shadowColor: .black, shadowOffset: CGSize(width: 0, height: 2), shadowOpacity: 0.5, shadowRadius: 4)
        return button
    }()
    
    static func createButton(title: String,
                      titleColor: UIColor,
                      font: UIFont,
                      backgroundColor: UIColor,
                      shadowColor: UIColor,
                      shadowOffset: CGSize,
                      shadowOpacity: Float,
                      shadowRadius: CGFloat) -> UIButton {
        let button = UIButton(type: .system)
        button.setTitle(title, for: .normal)
        button.setTitleColor(titleColor, for: .normal)
        button.titleLabel?.font = font
        button.backgroundColor = backgroundColor
        
        button.layer.shadowColor = shadowColor.cgColor
        button.layer.shadowOffset = shadowOffset
        button.layer.shadowOpacity = shadowOpacity
        button.layer.shadowRadius = shadowRadius
        button.layer.masksToBounds = false
        
        return button
    }

    public init(frame: CGRect, isLive: Bool) {
        super.init(frame: frame)
        self.isLive = isLive
        self.backgroundColor = .systemBackground
        self.layer.cornerRadius = 10
        self.layer.shadowColor = UIColor.lightGray.cgColor
        self.layer.shadowOffset = CGSize(width: 0, height: 1)
        self.layer.shadowOpacity = 0.3
        self.layer.shadowRadius = 4.0
        let stackViewH1 = UIStackView(arrangedSubviews: isLive ? [urlButton, clearButton] : [urlButton, jsonButton, clearButton])
        let stackViewH2 = UIStackView(arrangedSubviews: [closeButton, playButton])
        let stackView = UIStackView(arrangedSubviews: isLive ? [titleLabel, urlTextView, stackViewH1, stackViewH2] : [titleLabel, urlTextView, jsonTextView, stackViewH1, stackViewH2])

        stackViewH1.axis = .horizontal
        stackViewH1.distribution = .fillEqually
        stackViewH1.spacing = 10
        stackViewH1.translatesAutoresizingMaskIntoConstraints = false

        stackViewH2.axis = .horizontal
        stackViewH2.distribution = .fillEqually
        stackViewH2.spacing = 10
        stackViewH2.translatesAutoresizingMaskIntoConstraints = false
        
        stackView.axis = .vertical
        stackView.distribution = .fillProportionally
        stackView.spacing = 10
        stackView.translatesAutoresizingMaskIntoConstraints = false
        self.addSubview(stackView)
        
        if isLive {
            titleLabel.text = "Enter Live URL"
            urlTextView.heightAnchor.constraint(equalToConstant: 200).isActive = true
        } else {
            titleLabel.text = "Enter VOD URL or JSON"
            urlTextView.heightAnchor.constraint(equalToConstant: 60).isActive = true
            jsonTextView.heightAnchor.constraint(equalToConstant: 140).isActive = true
        }
        titleLabel.heightAnchor.constraint(equalToConstant: 20).isActive = true

        NSLayoutConstraint.activate([
            stackView.topAnchor.constraint(equalTo: self.topAnchor, constant: 10),
            stackView.bottomAnchor.constraint(equalTo: self.bottomAnchor, constant: -10),
            stackView.leadingAnchor.constraint(equalTo: self.leadingAnchor, constant: 10),
            stackView.trailingAnchor.constraint(equalTo: self.trailingAnchor, constant: -10)
        ])

        urlTextView.delegate = self
        jsonTextView.delegate = self
        urlButton.addTarget(self, action: #selector(urlButtonAction), for: .touchUpInside)
        jsonButton.addTarget(self, action: #selector(jsonButtonAction), for: .touchUpInside)
        clearButton.addTarget(self, action: #selector(clearButtonAction), for: .touchUpInside)
        closeButton.addTarget(self, action: #selector(closeButtonAction), for: .touchUpInside)
        playButton.addTarget(self, action: #selector(playButtonAction), for: .touchUpInside)
    }
    
    @objc func clearButtonAction() {
        disableInput(textView: urlTextView, content: "Enter URL")
        disableInput(textView: jsonTextView, content: "Enter JSON")
        urlTextView.isEditable = true
        jsonTextView.isEditable = true
    }
    
    @objc func urlButtonAction() {
        disableInput(textView: jsonTextView, content: "Please clear the JSON field first")
        enableInput(textView: urlTextView, content: RedMediaUtil.decodeBase64(base64EncodedString: isLive ? urlLiveDefault : urlVodDefault) ?? "")
    }
    
    @objc func jsonButtonAction() {
        disableInput(textView: urlTextView, content: "Please clear the URL field first")
        enableInput(textView: jsonTextView, content: RedMediaUtil.decodeBase64(base64EncodedString: jsonDefault) ?? "")
    }
    
    @objc func closeButtonAction() {
        closeButtonTapped?()
    }
    
    @objc func playButtonAction() {
        let isJSON = !jsonTextView.text.isEmpty && jsonTextView.textColor != .lightGray
        let isUrl = !urlTextView.text.isEmpty && urlTextView.textColor != .lightGray
        if isJSON || isUrl {
            playButtonTapped?(isUrl ? urlTextView.text : jsonTextView.text , isJSON)
        }
    }
    
    private func enableInput(textView: UITextView, content: String) {
        textView.isEditable = true
        textView.text = content
        textView.becomeFirstResponder()
        textView.textColor = .label
    }
    
    private func disableInput(textView: UITextView, content: String) {
        textView.isEditable = false
        textView.text = content
        textView.resignFirstResponder()
        textView.textColor = .lightGray
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
}

extension RedAlertView: UITextViewDelegate {
    func textViewDidBeginEditing(_ textView: UITextView) {
        if textView.textColor == .lightGray,
           textView.text.hasPrefix("Enter"){
            textView.text = nil
        }
        textView.textColor = .label
    }
    
    func textViewDidEndEditing(_ textView: UITextView) {
        if textView.text.isEmpty {
            textView.text = textView == urlTextView ? "Enter URL" : "Enter JSON"
            textView.textColor = .lightGray
        }
    }
    
    func textViewDidChange(_ textView: UITextView) {
        if textView == urlTextView {
            jsonTextView.isEditable = textView.text.isEmpty
            jsonTextView.text = textView.text.isEmpty ? "Enter JSON" : "Please clear the URL field first"

        } else if textView == jsonTextView {
            urlTextView.isEditable = textView.text.isEmpty
            urlTextView.text = textView.text.isEmpty ? "Enter URL" : "Please clear the JSON field first"
        }
    }
}
