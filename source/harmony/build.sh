# HAR compile shell:
hvigorw="/Applications/DevEco-Studio.app/Contents/tools/hvigor/bin/hvigorw" ## replace your hvigorw path
ohpm="/Applications/DevEco-Studio.app/Contents/tools/ohpm/bin/ohpm" ## replace your ohpm path

rm -rf ./redplayerproxy/build
rm -rf oh_modules
rm -rf entry/oh_modules
mkdir output
rm -rf output/redplayerproxy.har
hvigorw clean --no-daemon
hvigorw assembleHar --mode module -p module=redplayerproxy@default -p product=default --no-daemon
cp redplayerproxy/build/default/outputs/default/redplayerproxy.har output/redplayerproxy.har
ohpm install
