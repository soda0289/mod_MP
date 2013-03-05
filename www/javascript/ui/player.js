function shuffle_playlist(music_ui_ctx){
	
	//var shuffle_text_elem = document.createElement("span");
	
	//music_ui_ctx.player_status.appendChild(shuffle_text_elem);
	
	
	shuffle_text_elem.innerHTML = "shuffled";
}

function player(playlist, music_ui_ctx){	
	
	this.domain = music_ui_ctx.domain;
	
	//Setup player div and set playlist
	this.player_div = document.createElement("div");
	this.player_div.style.height = "66px";
	this.player_div.id = "player";
	this.player_div.style.fontSize = "14";
	
	this.playlist = playlist;
	
	//Create decoding jobs array
	this.decoding_jobs = [];
	
	this.audio_obj = new audio_obj(this);
	
	this.find_playable_source = function(song, domain){
		if(!('sources' in song) || song.sources.length < 1){
			//song has no sources
			//Fetch sources
			var sources_query = new music_query(domain, 0, "sources", sources_query_loaded);
			sources_query.song = song;
			sources_query.player = this;
			sources_query.source_type = "ogg";
			sources_query.song_id = song.song_id;
		
			load_query(sources_query);
			
			return 0;
		}else{
			return song.sources.length;
		}

	}
	
	this.get_next_song_index = function (){
		this.audio_obj.playing_index += 1;
		if(this.audio_obj.playing_index === this.playlist.query.results.length){
			this.audio_obj.playing_index = 0;
		}
		return this.audio_obj.playing_index;
	}

	//Setup Buttons
	this.play_button = document.createElement("img");
	this.play_button.id = "play_pause";
	this.play_button.src = "svg/play.svg";
	this.play_button.style.display = "block-inline";
	this.play_button.onclick = function (player){
		return function(event){
			//Play first song
			player.audio_obj.playing_index = 0;
			player.audio_obj.play_song(player.playlist.query.results[0]);
		}
	}(this);
	this.player_div.appendChild(this.play_button);
	
	
	this.previous_button = document.createElement("img");
	this.previous_button.id = "previous_button";
	this.previous_button.src = "svg/previous.svg";
	this.previous_button.style.display = "block-inline";
	//this.previous_button.style.vertical-align = "middle";
	this.previous_button.onclick = function (music_ui_ctx){
		return function(event){
			unhighlight_song(music_ui_ctx);
			music_ui_ctx.playing = 0;
			
			music_ui_ctx.playing_index -= 1;
			if(music_ui_ctx.playing_index< 0){
				music_ui_ctx.playing_index = playlist.query.results.length -1;
			}
			play_song(playlist.query.results[music_ui_ctx.playing_index], music_ui_ctx);
		}
	}(playlist, music_ui_ctx);
	this.player_div.appendChild(this.previous_button);
	
	this.next_button = document.createElement("img");
	this.next_button.id = "next_button";
	this.next_button.src = "svg/next.svg";
	this.next_button.style.display = "block-inline";
	this.next_button.onclick = function (player){
		return function(event){
			//play next song
			player.audio_obj.play_song(player.playlist.query.results[player.get_next_song_index(music_ui_ctx)]);
		}
	}(this);
	this.player_div.appendChild(this.next_button);
	
	this.shuffle = document.createElement("img");
	this.shuffle.id = "shuffle_button";
	this.shuffle.src = "svg/shuffle.svg";
	this.shuffle.style.display = "block-inline";
	this.shuffle.onclick = function (playlist){
		return function(event){
			playlist.shuffle();
		}
	}(playlist);
	this.player_div.appendChild(this.shuffle);
	
	
	this.song_info_div = document.createElement("div");
	this.song_info_div.id = "song_info"
	this.song_info_div.style.display = "inline-block";
	this.player_div.appendChild(this.song_info_div);


	//	Status boxes
	this.player_status = document.createElement("div");
	this.player_status.id = "player_status";
	this.player_status.style.display = "inline-block";
	
	this.time_elem = document.createElement("span");
	this.player_status.appendChild(this.time_elem);
	
	this.buffer_elem = document.createElement("span");
	this.player_status.appendChild(this.buffer_elem);
	
	this.shuffled_elem = document.createElement("span");
	this.player_status.appendChild(this.shuffled_elem);
	
	this.player_div.appendChild(this.player_status);
	
	this.decoding_status_div = document.createElement("div");
	this.decoding_status_div.id = "decoding_status";
	this.decoding_status_div.style.overflowY = "scroll";
	this.decoding_status_div.style.display = "inline-block";
	
	this.player_div.appendChild(this.decoding_status_div);
	
	this.div = this.player_div;
	
	this.update_time = function (){
		var current_seconds = ( parseInt(this.audio_obj.audio_ele.currentTime % 60) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.currentTime % 60):parseInt(this.audio_obj.audio_ele.currentTime % 60);
		var duration_seconds = ( parseInt(this.audio_obj.audio_ele.duration % 60) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.duration % 60):parseInt(this.audio_obj.audio_ele.duration % 60);
		
		this.time_elem.innerHTML = parseInt(this.audio_obj.audio_ele.currentTime / 60) + ":" +  current_seconds  + "/" + parseInt(this.audio_obj.audio_ele.duration /60) + ":" + duration_seconds;	
	}

	
}