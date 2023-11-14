# API v1.0.0

### 1. Introduction

This API documentation includes two parts: the first part is the API interface documentation of the RedPlayer SDK, and the second part is the API interface documentation of RedPreload SDK.


## 2. RedPlayer API


### 2.1 setOnPreparedListener
- **API Description:** set the mediaplayer prepared listener

#### 2.1.1 input params

Name						|Type			|Description
|:----						|:---			|:---	
listener			|OnPreparedListener		|listen the mediaplayer prepared state



### 2.2 setOnCompletionListener
- **API Description:** set the mediaplayer play complete listener

#### 2.2.1 input params

Name						|Type			|Description  
|:----						|:---		                        |:------		
listener					|OnCompletionListener				|listen the mediaplayer play complete event


### 2.3 setOnBufferingUpdateListener
- **API Description:** set the mediaplayer buffering updating listener

#### 2.3.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
listener	                |OnBufferingUpdateListener	|listen the mediaplayer buffering updating event


### 2.4 setOnSeekCompleteListener
- **API Description:** set the mediaplayer seek complete listener

#### 2.4.1 input params

Name						|Type	            |Description  
|:----						|:---			    |:---	
listener		|OnSeekCompleteListener			|listen the mediaplayer seek complete event

### 2.5 setOnVideoSizeChangedListener
- **API Description:** set the video size changed listener

#### 2.5.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
listener	                |OnVideoSizeChangedListener		|listen the video size change event


### 2.6 setOnErrorListener
- **API Description:** set the mediaplayer error listener

#### 2.6.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
listener				    |OnErrorListener	    |listen the mediaplayer error state


### 2.7 setOnInfoListener
- **API Description:** set the mediaplayer info listener

#### 2.7.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
listener					|OnInfoListener				|listen the mediaplayer info event

#### 2.7.2  output result

Name						|Type		 	|Description  
|:----						|:---		            |:------		
what						|Integer		  		|event type
args						|Bundle		  			|event extra info

### 2.8 setLooping
- **API Description:** set the mediaplayer play loop

#### 2.8.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
looping					    |Boolean	    | media player loop play

### 2.9 setSpeed
- **API Description:** set the mediaplayer play speed

#### 2.9.1 input params

Name					|Type			|Description  
|:----					|:---			|:---	
speed					|Float		    | media player play speed [0.75,1.0,1.25,1.5,2.0]


### 2.10 setSurface
- **API Description:** set the mediaplayer render surface

#### 2.10.1 input params

Name						|Type			|Description  
|:----						|:---			|:---	
surface					    |Surface	    | media player render surface


### 2.11 setDisplay
- **API Description:** set the mediaplayer render surface holder

#### 2.11.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
surfaceHolder					|SurfaceHolder					| media player render surface holder

### 2.12 setEnableMediaCodec
- **API Description:** set the mediaplayer render surface holder

#### 2.12.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
enable					|Boolean				| media player use media codec

### 2.13 setVideoCacheDir
- **API Description:** set the mediaplayer stream download path

#### 2.13.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
path					|String		  			| media player stream download path

### 2.14 setDataSource
- **API Description:** set the mediaplayer data source

#### 2.14.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
json					|String		  			| media player play json 
path					|String		  			| media player play url 
header					|Map<String, String>		  			| media player play url's header

#### 2.14.2 json style demo

[json demo](JSON.md)

### 2.15 setVolume
- **API Description:** set the mediaplayer left/right volume

#### 2.15.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
leftVolume					|Float		  	| media player left volume 
rightVolume					|Float          | media player right volume 

### 2.16 prepareAsync
- **API Description:** preapre the mediaplayer in async

### 2.17 start
- **API Description:** start the mediaplayer

### 2.18 pause
- **API Description:** pause the mediaplayer

### 2.19 stop
- **API Description:** stop the mediaplayer

### 2.20 reset
- **API Description:** reset the mediaplayer

### 2.21 release
- **API Description:** release the mediaplayer


### 2.22 seekTo
- **API Description:** set the mediaplayer left/right volume

#### 2.22.1 input params

Name						|Type		 	|Description  
|:----						|:---			|:---	
position					|Long		  			| seek the position to play

### 2.23 getAudioCodecInfo
- **API Description:** get the mediaplayer audio codec info

#### 2.23.1  output result

Type		 	|Description  
|:---		    |:------		
|String		  			|the audio codec name


### 2.24 getVideoCodecInfo
- **API Description:** get the mediaplayer video codec info

#### 2.24.1  output result

Type		 	|Description  
|:---		|:------	
|String		  			| the video codec name


### 2.25 getVideoWidth
- **API Description:** get the video width info

#### 2.25.1  output result

Type		 	|Description  
|:---		|:------		
|Integer		  			| the video width 

### 2.26 getVideoHeight
- **API Description:** get the video height info

#### 2.26.1  output result

Type		 	|Description  
|:---		|:------	
|Integer		  			| the video height 

### 2.27 getCurrentPosition
- **API Description:** get the mediaplayer current play position

#### 2.27.1  output result

Type		 	|Description  
|:---		|:------	
|Long		  			| the mediaplayer play current position 


### 2.28 getDuration
- **API Description:** get the video duration info

#### 2.28.1  output result

Type		 	|Description  
|:---		|:------		
|Long		  			| the video duration


### 2.29 getSpeed
- **API Description:** get the mediaplayer play speed

#### 2.29.1  output result

Type		 	|Description  
|:---		|:------
|Float		  			| the mediaplayer play speed

### 2.30 getPlayUrl
- **API Description:** get the mediaplayer final play url

#### 2.30.1  output result

Type		 	|Description  
|:---		|:------	
|String		  			| the mediaplayer final play url

### 2.31 getVideoFileFps
- **API Description:** get the video fps

#### 2.31.1  output result

Type		 	|Description  
|:---		|:------
|Float		  			| the mediaplayer final play url


### 2.32 getBitRate
- **API Description:** get the video bit rate

#### 2.32.1  output result

Type		 	|Description  
|:---		|:------
|Long		  			| the video bit rate


### 2.33 getSeekCostTimeMs
- **API Description:** get the mediaplayer seek time cost

#### 2.33.1  output result

Type		 	|Description  
|:---		    |:------	
|Long		    | the mediaplayer seek time cost

### 2.34 isPlaying
- **API Description:** get the mediaplayer is in playing state

#### 2.34.1  output result

Type		 	|Description  
|:---		    |:------
|Boolean        | the mediaplayer is in playing state




## 3. RedPreload API

### 3.1 addPreloadCacheListener
- **API Description:** add the media preload state listener

#### 3.1.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
listener	                |PreloadCacheListener		|the  media preload state listener

### 3.2 removePreloadCacheListener
- **API Description:** remove the media preload state listener

#### 3.2.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
listener	                |PreloadCacheListener		|the  media preload state listener

### 3.3 start
- **API Description:** start the media preload tasks

#### 3.3.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
requests	                |List<VideoCacheRequest>  |the preload all tasks

### 3.4 stop
- **API Description:** stop all media preload tasks

### 3.5 release
- **API Description:** stop all media preload tasks and release preload inner resource

### 3.6 getAllCacheFilePaths
- **API Description:** get the special dir's all preload files
#### 3.6.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
cacheDir	                |String         |the special cache dir 
#### 3.6.2  output result
Type		 	      |Description  
|:---		          |:------
|Array<String>        | all files in the special cache dir 

### 3.7 deleteCacheFile
- **API Description:** delete the special dir's preload file
#### 3.7.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
cacheDir	                |String         |the special cache dir 
url                         |String         |the perload task's url
#### 3.7.2  output result
Type		 	      |Description  
|:---		          |:------
|Int                  | 0: delete success; != 0: error code 

### 3.8 getCacheFileRealSize
- **API Description:** get the special preload file real download size
#### 3.8.1  input params
Name						|Type			|Description
|:----						|:---			|:---	
cacheDir	                |String         |the special cache dir 
url                         |String         |the perload task's url
#### 3.8.2  output result
Type		 	      |Description  
|:---		          |:------
|Long                 | the preload file real download size 
