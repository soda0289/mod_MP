
function audio_obj(player, music_ui_ctx){
	this.player = player;
	//Domain for sources and decoding queries
	this.domain = music_ui_ctx.domain;
	//Is audio playing
	this.playing = 0;
	//Info about playing song

	this.next_to_play = null;
	
	//Audio Element
	this.audio_ele = new Audio();
	//Detect error
	this.audio_ele.addEventListener('error', 
			function (player) {
				return function(e){
					alert("error loading audio");
				};
			}(player),
	false);
	
	this.audio_ele.addEventListener('timeupdate', 
	function (player) {
		return function(e){
			player.update_time();
		};
	}(player), false);
	this.audio_ele.addEventListener("progress", 
	function(audio_obj) {
		return function(event){
			if(this.buffered && this.duration){
				//music_ui_ctx.buffer_elem.innerHTML = "Buffering:" + parseInt(this.buffered.end(0) / this.duration * 100) + "%";
			}
		};
	}(this),false);
	this.audio_ele.addEventListener("loadstart", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Starting to load Audio";
					audio_obj.playing = 1;
					this.play();
				};
	}(this),false);
	this.audio_ele.addEventListener("loaddata", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Loading Audio";
					audio_obj.playing = 1;
					this.play();
				};
	}(this),false);
	this.audio_ele.addEventListener("loadmetadata", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Loaded metadata";
					audio_obj.playing = 1;
					this.play();
				};
	}(this),false);
	this.audio_ele.addEventListener("canPlay", 
			function(audio_obj) {
				return function(event){
					//audio_obj.buffer_elem.innerHTML = "Loaded enough to play";
				};
	}(this),false);
	this.audio_ele.addEventListener("canPlayThru", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Done loading. 100% downloaded";
				};
	}(this),false);
	//Play next song when finished current
	this.audio_ele.addEventListener('ended', function(player){
			return function(event){
				player.next_button.click();
			};
	}(player), false);

	this.play_song = function (song){
		if(typeof(song) !== "object"){
			console.error("ERROR: \n Audio Object (play_song): Song is not an object");
			return 0;
		}
		
		var playlist = this.player.playlist;
		
		

		//stop current song
		this.playing = 0;
		
		//Find playable audio file for this browser
		this.next_to_play = song;
		if(this.player.find_playable_source(song, this.domain) === 0){
			//Abort playing
			this.playing = 0;
			return 0;
		}
		//Set audio object to playing
		this.playing = 1;
		
		player.change_song(song);
		
		//Set url for audio and start loading
		this.audio_ele.src = "http://"+this.domain+"/music/play/source_id/" + song.sources[0].source_id;
		this.audio_ele.load();
		
		//Change status
		player.song_info_div.innerHTML = "Title: " + song.song_title + "<br />Artist: " + song.artist_name + "<br />Album: " + song.album_name;
		
		//Change Play button to pause
		player.play_button.src = "svg/pause.svg";
		player.play_button.onclick = function (audio_obj){
			return function(){
				audio_obj.stop_song();
			};
		}(this);
	
		//Decode next 4 songs in playlist
		player.decode_next_songs(4);
	};
	
	this.stop_song = function (){
		//Change Play button back
		this.player.play_button.src = "svg/play.svg";
		this.player.play_button.onclick = function (player){
			return function(){
				//Change Play button to pause
				player.audio_obj.audio_ele.play();
				player.play_button.src = "svg/pause.svg";
				player.play_button.onclick = function (player){
					return function(){
						player.audio_obj.stop_song();
					};
				}(player);
				
			};
		}(this.player);
		
		this.player.audio_obj.audio_ele.pause();
		
	};
}
