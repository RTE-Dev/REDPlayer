Pod::Spec.new do |s|
  s.name             = 'opensoundtouch'
  s.version          = '0.0.2'
  s.summary          = 'opensoundtouch'
  s.description      = 'opensoundtouch dynamic library'
  s.homepage         = 'https://github.com/RTE-Dev/RedPlayer/source/ios/RedPlayerDemo/Submodules/opensoundtouch'
  s.license          = { :type => 'LGPL', :file => 'LICENSE' }
  s.author           = { 'chengyifeng' => 'chengyifeng@xiaohongshu.com' }
  s.source           = { :git => 'git@github.com/RTE-Dev/RedPlayer.git', :tag => s.version.to_s }

  s.ios.deployment_target = '10.0'

  s.vendored_framework = 'Sources/*.framework'

end
