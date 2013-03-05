
function decoding_job(song, player){
	this.song = song;
	this.transcode_query;
	this.status = "";
	this.progress = 0.0;
	
	this.update_decoding_status = function (player, dec_job){
		return function(){
			var decdoing_status_list = document.getElementById("decoding_status");
		
			if(this.results[0] == null){
				alert("No decoding JOB WTF! Server must be borked");
				return 0;
			}
			
			var decoding_status = document.getElementById("decoding_" +this.results[0].input_source_id);
			if(decoding_status == null){
				decoding_status =  document.createElement('span');
				decoding_status.id = "decoding_" +this.results[0].input_source_id;
				decdoing_status_list.appendChild(decoding_status);
			}
			decoding_status.innerHTML = this.results[0].status + " Progress: " + this.results[0].progress;
			if(parseFloat(this.results[0].progress) < 100.0){
				setTimeout(
				function(query){
					return function(){
						query.reset();
						load_query(query);
					}
				}(this),3000);
				
			}else{
				dec_job.song.sources = new Array();
				var source = {
					source_id : this.results[0].output_source_id,
				 	source_type : this.results[0].output_type
				};
				dec_job.song.sources.push(source);
				
				if(player.audio_obj.playing === 0 && dec_job.song.song_id === player.audio_obj.next_to_play.song_id){
					player.audio_obj.play_song(dec_job.song);
				}
				
				decdoing_status_list.removeChild(decoding_status);
				player.decoding_jobs.splice(player.decoding_jobs.indexOf(dec_job),1);
			}
				
		}
	}(player, this);
	
	this.transcode_query = new music_query(player.domain, 0, "transcode", this.update_decoding_status);
	this.transcode_query.song_id = song.song_id;
	
	//Add to decdoing_job array
	player.decoding_jobs.push(this);
	
	load_query(this.transcode_query);
}

function sources_query_loaded(results){
	if (results.length > 0){
		//Copy sources to song
		this.song.sources = results;

		//Is audio playing
		if(this.player.audio_obj.playing === 0){
			this.player.audio_obj.play_song(this.song);
		}
		
	}else{
		//Decode song no source avalible
		new decoding_job(this.song, this.player);
	}
}

function audio_obj(player){
	this.player = player;
	//Domain for sources and decoding queries
	this.domain = player.playlist.query.hostname;
	//Is audio playing
	this.playing = 0;
	//Info about playing song
	this.song_backgroundColor;
	this.playing_index = -1;
	this.next_to_play = null;
	
	//Audio Element
	this.audio_ele = new Audio;
	this.audio_ele.addEventListener('timeupdate', 
	function (player) {
		return function(e){
			player.update_time();
		}
	}(player), false);
	this.audio_ele.addEventListener("progress", 
	function(audio_obj) {
		return function(event){
			if(this.buffered && this.duration){
				//music_ui_ctx.buffer_elem.innerHTML = "Buffering:" + parseInt(this.buffered.end(0) / this.duration * 100) + "%";
			}
		}
	}(this),false);
	this.audio_ele.addEventListener("loadstart", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Starting to load Audio";
					audio_obj.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("loaddata", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Loading Audio";
					audio_obj.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("loadmetadata", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Loaded metadata";
					audio_obj.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("canPlay", 
			function(audio_obj) {
				return function(event){
					//audio_obj.buffer_elem.innerHTML = "Loaded enough to play";
				}
	}(this),false);
	this.audio_ele.addEventListener("canPlayThru", 
			function(audio_obj) {
				return function(event){
					//music_ui_ctx.buffer_elem.innerHTML = "Done loading. 100% downloaded";
				}
	}(this),false);
	//Play next song when finished current
	this.audio_ele.addEventListener('ended', function(player){
			return function(event){
				player.next_button.click();
			}
	}(player), false);


	this.play_song = function (song){
		if(typeof(song) !== "object"){
			alert("passed play_song undefined song");
			return 0;
		}
		//unhighlight current song
		//unhighlight_song(music_ui_ctx);
		//stop current song
		this.playing = 0;
		
		this.next_to_play = song;
		if(this.player.find_playable_source(song, this.domain) === 0){
			//Abort playing
			this.playing = 0;
			return 0;
		}
	
		this.playing = 1;
		
		this.audio_ele.src = "http://"+this.domain+"/music/play/source_id/" + song.sources[0].source_id;
		this.audio_ele.load;
		
		//scroll to song
		//var song_listing = document.getElementById("song_" + song.song_id);
		//song_listing.scrollIntoView();
		//var scrollBack = (music_ui_ctx.songs_div.scrollHeight - music_ui_ctx.songs_div.scrollTop <= music_ui_ctx.songs_div.clientHeight) ? 0 : music_ui_ctx.songs_div.clientHeight/4;
		//music_ui_ctx.songs_div.scrollTop -= scrollBack;
	    //Highlight Song
		//this.song_backgroundColor = song_listing.style.backgroundColor;
		//song_listing.style.backgroundColor = "black";
		
	
	
		
		
		//Change status
		player.song_info_div.innerHTML = "Title: " + song.song_title + "<br />Artist: " + song.artist_name + "<br />Album: " + song.album_name;
		
		//Change Play button to pause
		player.play_button.src = "svg/pause.svg";
		player.play_button.onclick = function (){
			return function(){
				this.stop_song();
			};
		};
	
		
		for(var i = 1;i <= 4; i++){
			var song_next = player.playlist.query.results[this.playing_index + i];
			if(typeof song_next !== 'undefined'){
				this.player.find_playable_source(song_next, this.domain);
			}else{
				break;
			}
		}
	}
	
	this.stop_song = function (player){
		var audio_ele = document.createElement("audio");
		//Change Play button back
		player.play_button.src = "svg/play.svg";
		player.play_button.onclick = function (player){
			return function(){
				//Change Play button to pause
				player.audio_ele.play();
				player.play_button.src = "svg/pause.svg";
				player.play_button.onclick = function (player){
					return function(){
						stop_song(player);
					};
				}(mplayer);
				
			}
		}(player)
		
		player.audio_ele.pause();
		
	}
}