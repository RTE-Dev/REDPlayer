Pod::Spec.new do |s|
  s.name             = 'RedPlayer'
  s.version          = '0.0.12'
  s.summary          = 'RedPlayer'
  s.description      = 'RedPlayer SDK'
  s.homepage         = 'https://github.com/RTE-Dev/RedPlayer/source/ios/XYRedPlayer/Submodules/RedPlayer'
  s.license          = { :type => 'LGPL', :file => 'LICENSE' }
  s.author           = { 'zijie' => 'zijie@xiaohongshu.com' }
  s.source           = { :git => 'git@github.com/RTE-Dev/RedPlayer.git', :tag => s.version.to_s }

  s.ios.deployment_target = '10.0'
  #s.vendored_framework = 'RedPlayer/Sources/*.framework'
  s.source_files = 'redplayer/base/**/*','redplayer/Interface/**/*','redplayer/ios/**/*','redplayer/RedCore/**/*'
  s.public_header_files = 'redplayer/'
  #s.resources = "RedPlayer/Resouces/**.*"

  s.subspec 'redbase' do |a|
    a.source_files = 'redbase/include/*.h','redbase/src/ios','redbase/src/*.*'
    a.public_header_files = 'redbase/include/*.h','redbase/src/ios/*.h'
  end
  
  s.subspec 'reddecoder' do |a|
    a.vendored_framework = 'extra/reddecoder/ios/*.framework'
  end
  
  s.subspec 'reddownload' do |a|
    a.vendored_framework = 'extra/reddownload/ios/*.framework'
  end
  
  s.subspec 'redrender' do |a|
    a.vendored_framework = 'extra/redrender/ios/*.framework'
  end
  
  s.subspec 'redsource' do |a|
    a.vendored_framework = 'extra/redsource/ios/*.framework'
  end
  
  s.subspec 'redstrategycenter' do |a|
    a.vendored_framework = 'extra/redstrategycenter/ios/*.framework'
  end
  
  
  s.dependency "XYMediaFfmpeg"
  s.dependency "opensoundtouch"

end
