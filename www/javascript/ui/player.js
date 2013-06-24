

function player(playlist, music_ui_ctx){	
	//playlist.player = this;
	
	this.domain = music_ui_ctx.domain;
	//Playing index of songs in playlist
	this.playing_index = -1;
	
	//Setup player div and set playlist
	this.player_div = document.createElement("div");
	this.player_div.style.height = "66px";
	this.player_div.id = "player";
	this.player_div.style.fontSize = "14";
	
	this.playlist = playlist;
	
	//Create decoding jobs array
	this.decoding_jobs = [];
	
	this.audio_obj = new audio_obj(this, music_ui_ctx);
	
	this.find_playable_source = function(song, domain){
		if(!('sources' in song) || song.sources.length < 1){
			//song has no sources
			//Fetch sources
			var sources_parameters = new query_parameters("sources");
			sources_parameters.source_type = "ogg";
			sources_parameters.song_id.push(song.song_id);
			
			var sources_query = new music_query(domain, sources_parameters, sources_query_loaded);
			sources_query.song = song;
			sources_query.player = this;
		
			sources_query.load();
			
			return 0;
		}else{
			return song.sources.length;
		}

	}
	
	this.get_next_song_index = function (){
		this.playing_index += 1;
		if(this.playing_index === this.playlist.songs.length){
			this.playing_index = 0;
		}
		return this.playing_index;
	}
	
	this.get_prev_song_index = function (){
		this.playing_index -= 1;
		if(this.playing_index < 0){
			this.playing_index === this.playlist.songs.length;
		}
		return this.playing_index;
	}

	//Setup Buttons
	this.play_button = document.createElement("img");
	this.play_button.id = "play_pause";
	this.play_button.src = "svg/play.svg";
	this.play_button.style.display = "block-inline";
	this.play_button.onclick = function (player){
		return function(event){
			//Play first song
			player.audio_obj.play_song(player.playlist.songs[player.get_next_song_index()]);
		}
	}(this);
	this.player_div.appendChild(this.play_button);
	
	
	this.previous_button = document.createElement("img");
	this.previous_button.id = "previous_button";
	this.previous_button.src = "svg/previous.svg";
	this.previous_button.style.display = "block-inline";
	//this.previous_button.style.vertical-align = "middle";
	this.previous_button.onclick = function (player){
		return function(event){
			//Unhilight song
			player.playlist.songs_table.deselect_row(player.playing_index);
			//play prev song
			var prev_song_index = player.get_prev_song_index();
			var song = player.playlist.songs[prev_song_index];
			player.audio_obj.play_song(song);
		}
	}(this);
	this.player_div.appendChild(this.previous_button);
	
	this.next_button = document.createElement("img");
	this.next_button.id = "next_button";
	this.next_button.src = "svg/next.svg";
	this.next_button.style.display = "block-inline";
	this.next_button.onclick = function (player){
		return function(event){
			//Unhilight song
			player.playlist.songs_table.deselect_row(player.playing_index);
			//play next song
			var next_song_index = player.get_next_song_index();
			var song = player.playlist.songs[next_song_index]
			player.audio_obj.play_song(song);
		}
	}(this);
	this.player_div.appendChild(this.next_button);
	
	this.shuffle = document.createElement("img");
	this.shuffle.id = "shuffle_button";
	this.shuffle.src = "svg/shuffle.svg";
	this.shuffle.style.display = "block-inline";
	this.shuffle.onclick = function (player_if){
		return function(event){
			player_if.playlist.shuffle();
		}
	}(this);
	
	this.player_div.appendChild(this.shuffle);
	
	//Song info
	this.song_info_div = document.createElement("div");
	this.song_info_div.id = "song_info"
	this.song_info_div.style.display = "inline-block";
	this.player_div.appendChild(this.song_info_div);


	//	Status boxes
	this.player_status = document.createElement("div");
	this.player_status.id = "player_status";
	this.player_status.style.display = "inline-block";
	//Time in song
	this.time_elem = document.createElement("span");
	this.player_status.appendChild(this.time_elem);
	//Buffering status
	this.buffer_elem = document.createElement("span");
	this.player_status.appendChild(this.buffer_elem);
	//Shuffled
	this.shuffled_elem = document.createElement("span");
	this.player_status.appendChild(this.shuffled_elem);
	
	this.player_div.appendChild(this.player_status);
	
	//Decoding status div
	this.decoding_status_div = document.createElement("div");
	this.decoding_status_div.id = "decoding_status";
	this.decoding_status_div.style.overflowY = "scroll";
	this.decoding_status_div.style.display = "inline-block";
	
	this.player_div.appendChild(this.decoding_status_div);
	
	this.theme_change_div = document.createElement("div");
	this.theme_change_div.id = "theme_change";
	this.theme_change_div.style.display = "inline-block";
	this.theme_change_div.innerHTML = "Dark/Lite";
	this.theme_change_div.onclick = function(){
		var stylesheet = document.getElementById("stylesheet");
		if(stylesheet.href === stylesheet.baseURI + "style_white.css"){
			stylesheet.href = "/style_black.css";
		}else{
			stylesheet.href = "/style_white.css";
		}
	};
	
	this.player_div.appendChild(this.theme_change_div);
	
	
	this.div = this.player_div;
	
	this.update_time = function (){
		var current_seconds = ( parseInt(this.audio_obj.audio_ele.currentTime % 60) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.currentTime % 60):parseInt(this.audio_obj.audio_ele.currentTime % 60);
		var duration_seconds = ( parseInt(this.audio_obj.audio_ele.duration % 60) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.duration % 60):parseInt(this.audio_obj.audio_ele.duration % 60);
		
		this.time_elem.innerHTML = parseInt(this.audio_obj.audio_ele.currentTime / 60) + ":" +  current_seconds  + "/" + parseInt(this.audio_obj.audio_ele.duration /60) + ":" + duration_seconds;	
	}

	
}